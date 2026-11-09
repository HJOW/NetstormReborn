// 섬 검색 raw-island-queries.js(RawIslandQueries)를 원본 10.78 관측 4245입력과 cpppj `IslandQueriesTests` 계약으로 대조한다.
//  - 원본 관측 islandqueries-x86.tsv 의 10.78(originals) 4245행: 실제 100기록(신전/존재/현재 owner DWORD/원래 owner BYTE/지역 BYTE/padding)의 입력 12칸에서
//    세 검색(FindOwnerTerritory·FindTerritory·CountOwned)의 반환, 전체 3204 BYTE 의 불변(Adler-32)과 세 모드(fort/special/chaff)·revision 불변을 대조한다.
//  - 계약: 실제 Add/Delete/Reset 과 현재 fort 가 다음 검색에 즉시 반영되고 물리 100기록 밖 접근은 거부된다.
// CD 10.72 는 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E220: 섬 검색 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, adler32, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { ISLAND_NOT_FOUND, RawIslandQueries } from '../www/core/raw-island-queries.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E220)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'islandqueries-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 예외가 나는지 본다. (2026-11-10 WB04-E220)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 월드의 단일 3204 BYTE/모드와 기존 runtime 에 검색 몸체를 연결하는 입력 장면이다. 검색 중 가상 owner/진단 호출은 원본 몸체에 없으므로 뜻밖의 효과가 발생하면 검사를 실패시킨다. (2026-11-10 WB04-E220)
class IslandQueryScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.hash = new SquidHash();
        this.records = createRawIslandStorage();
        this.modes = createFortFinishOwnerState();
        this.residences = new SquidPostPopList();
        this.revision = { value: 0 };
        this.changed = { value: 0 };
        this.runtime = new RawIslandRuntime(this.pool, this.hash, this.records, this.modes, this.residences, this.revision, this.changed, {
            setResidenceOwner: () => { throw new Error('IslandQueries 예상밖 가상 owner'); },
            assertion: () => { throw new Error('IslandQueries 예상밖 진단'); },
        });
        this.queries = new RawIslandQueries(this.runtime, this.modes);
    }

    // 명시 원본 입력만 구성하며 검색 반환/일치 수는 계산하지 않는다. 입력 12칸: 0 count·1 존재 패턴·2 존재값·3 현재 owner·4 원래 owner·5 지역·6 owner 인자·7 지역 인자·8 fort·9 special·10 chaff·11 추가 존재 위치. (2026-11-10 WB04-E220)
    prepare(input) {
        assert.equal(input.length, 12);
        this.records.fill(0xa5);
        put(this.records, 0, input[0]);
        // 실제 100기록의 신전/존재/current DWORD/원래 BYTE/territory BYTE 와 padding 을 각각 배치한다.
        for (let index = 0; index < 100; index += 1) {
            const record = 4 + index * 32;
            put(this.records, record, 0x11110000 + index);
            put(this.records, record + 4, 0);
            put(this.records, record + 8, input[3]);
            put(this.records, record + 12, input[5], 1);
            put(this.records, record + 13, input[4], 1);
            if (input[1] === 1) put(this.records, record + 4, index % 3 ? input[2] : 0);
            if (input[1] === 2) {
                put(this.records, record + 4, index === 0 || index === 2 || index === 99 ? input[2] : 0);
                if (index % 2) {
                    put(this.records, record + 8, (input[3] ^ 0x01000100) >>> 0);
                    put(this.records, record + 13, (input[4] ^ 0x80) & 0xff, 1);
                    put(this.records, record + 12, (input[5] ^ 0x80) & 0xff, 1);
                }
            }
        }
        put(this.records, 4 + input[11] * 32 + 4, input[2]);
        this.modes.fort = input[8] !== 0;
        this.modes.special = input[9] !== 0;
        this.modes.chaffMode = input[10] !== 0;
    }
}

test('섬 검색: 원본 10.78 4245입력의 세 검색 반환과 3204 BYTE·모드·revision 불변이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new IslandQueryScene();
    let count = 0;
    // 원본에서 독립 실행한 모든 검색 입력을 같은 단일 runtime 에 전달한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const input = row[2].split(',').map(Number);
        scene.prepare(input);
        const before = scene.records.slice();
        let result = 0;
        if (row[1] === 'FindOwnerTerritory') result = scene.queries.findOwnerTerritory(input[6] | 0, input[7] | 0);
        else if (row[1] === 'FindTerritory') result = scene.queries.findTerritory(input[7] | 0);
        else if (row[1] === 'CountOwned') result = scene.queries.countOwned(input[6]);
        else assert.fail(`모르는 검색 ${row[1]}`);
        const modes = `${Number(scene.modes.fort)},${Number(scene.modes.special)},${Number(scene.modes.chaffMode)}`;
        const label = `${row[1]} ${row[2]} (#${count})`;
        assert.equal(result, Number(row[3]), `${label} 반환`);
        assert.deepEqual(scene.records, before, `${label} 기록 불변`);
        assert.equal(adler32(scene.records), Number(row[4]), `${label} Adler`);
        assert.equal(modes, row[5], `${label} 모드`);
        assert.ok(scene.revision.value === 0 && scene.changed.value === 0, `${label} 공유 전역 불변`);
        count += 1;
    }
    assert.equal(count, 4245);
});

test('섬 검색 계약: 실제 Add/Delete/Reset 과 현재 fort 가 다음 검색에 즉시 반영되고 물리 100기록 밖 접근을 거부한다', () => {
    const scene = new IslandQueryScene();
    scene.runtime.reset();
    assert.equal(scene.queries.findTerritory(2), ISLAND_NOT_FOUND);
    assert.equal(scene.queries.countOwned(0), 0);
    assert.equal(scene.runtime.add(1, 2), 0);
    assert.equal(scene.runtime.add(2, 2), 1);
    assert.equal(scene.queries.countOwned(1), 1);
    assert.equal(scene.queries.findOwnerTerritory(1, 2), 0);
    // 현재 owner DWORD 가 바뀌면 일반 모드의 원래 owner BYTE 가 같아도 통과하지 않는다.
    put(scene.records, 4 + 8, 2);
    assert.equal(scene.queries.findOwnerTerritory(1, 2), ISLAND_NOT_FOUND);
    assert.equal(scene.queries.countOwned(2), 2);
    // fort 모드는 현재 owner 0 도 허용한다.
    scene.modes.fort = true;
    put(scene.records, 4 + 8, 0);
    assert.equal(scene.queries.findOwnerTerritory(7, 2), 0);
    assert.equal(scene.queries.countOwned(0), 1);
    scene.runtime.delete(0);
    assert.equal(scene.runtime.count, 2);
    assert.equal(scene.queries.findTerritory(2), 1);
    scene.runtime.reset();
    assert.equal(scene.queries.findOwnerTerritory(0, 2), ISLAND_NOT_FOUND);
    assert.equal(scene.queries.countOwned(0), 0);
    // 음수 count(0xffffffff)는 빈 순회이고 물리 100개를 넘는 count 는 거부한다.
    put(scene.records, 0, 0xffffffff);
    assert.equal(scene.queries.findTerritory(2), ISLAND_NOT_FOUND);
    assert.equal(scene.queries.countOwned(0), 0);
    put(scene.records, 0, 101);
    assert.ok(throws(() => scene.queries.countOwned(0)));
});

test('섬 검색 계약: signed 지역/원래 owner BYTE 는 부호 확장해 비교하고 판본이 다른 섬 목록은 받지 않는다', () => {
    const scene = new IslandQueryScene();
    scene.runtime.reset();
    // 지역 255(=signed -1)·원래 owner 255(=-1) 섬이 인자 -1 과 일치한다.
    scene.runtime.add(0xffffffff, 255);
    assert.equal(scene.queries.findTerritory(-1), 0);
    assert.equal(scene.queries.findTerritory(255), ISLAND_NOT_FOUND);
    assert.equal(scene.queries.findOwnerTerritory(-1, -1), 0);
    assert.equal(scene.queries.findOwnerTerritory(255, -1), ISLAND_NOT_FOUND);
    // 판본이 다른 섬 목록은 받지 않는다.
    assert.ok(throws(() => new RawIslandQueries({ pool: { edition: 'cd1072' } }, scene.modes)));
});

test('섬 검색 계약: 패치는 시작할 때의 count 와 fort 모드를 고정해 읽고 반복 중 바뀐 값을 쓰지 않는다', () => {
    const scene = new IslandQueryScene();
    scene.runtime.reset();
    scene.runtime.add(1, 7);
    scene.runtime.add(0, 7);
    // 두 번째 섬은 fort 모드에서만 통과한다(현재 owner 0, 원래 owner 0 이라 인자 5 와 다름). 첫 섬의 owner 조회가 fort 를 켜고 count 를 늘려도 시작 값을 쓴다.
    let nowCount = scene.runtime.count;
    const fake = {
        pool: scene.pool,
        get count() { return nowCount; },
        exists: (island) => scene.runtime.exists(island),
        owner: (island) => { if (island === 0) { scene.modes.fort = true; nowCount = 3; } return scene.runtime.owner(island); },
        bytes: scene.records,
    };
    const queries = new RawIslandQueries(fake, scene.modes);
    assert.equal(queries.findOwnerTerritory(5, 7), ISLAND_NOT_FOUND);
    // 같은 장면에서 시작부터 fort 이면 두 번째 섬이 통과한다.
    scene.modes.fort = true;
    nowCount = 2;
    assert.equal(queries.findOwnerTerritory(5, 7), 1);
});
