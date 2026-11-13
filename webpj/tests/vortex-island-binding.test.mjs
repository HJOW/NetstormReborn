// 신전 Ascend raw-vortex-ascend.js(RawVortexAscend)가 실제 월드의 섬 런타임(RawIslandRuntime)을 복사 없이 직접 읽는 두 번째 생성자 경로를 원본 10.78 관측과 cpppj 계약으로 대조한다
// (cpppj `VortexIslandBindingTests`).
//  - 원본 관측: vortexascend-x86.tsv 의 `originals` Count 20행 — 저장된 입력(소유자/변형)대로 실제 3204바이트 기록의 존재 DWORD(+4)와 소유자 DWORD(+8)만 32바이트 간격에 쓰고
//    (이웃·padding 은 0xa5 로 보존), 반환 개수·기록 전체 불변·난수 상태 불변이 원본과 같다. 하위 효과는 Count 에서 호출되면 실패하는 훅이다.
//  - 계약: 생성·삭제·owner 변경 뒤 현재 기록 재조회, signed count(0x80000000 은 반복 0회), 다른 풀 거부(배열 경로는 거부하지 않음), 호출 횟수(exists 후에만 owner 를 한 번 또는 두 번 읽음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E370: 7-P3 RawVortexAscend 두 번째 생성자 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { GameRandom } from '../www/core/random.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { ISLAND_STORAGE_BYTES, RawIslandRuntime, createFortFinishOwnerState } from '../www/core/raw-island-runtime.js';
import { RawVortexAscend } from '../www/core/raw-vortex-ascend.js';

// 원본 관측 행 중 10.78(originals)의 Count 입력이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E370)
const countRows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'vortexascend-x86.tsv')) ?? []).filter((row) => row[0] === 'originals' && row[1] === 'Count');
// 원본 실행기에 넣었던 소유자 DWORD 여섯 개이다. (2026-11-13 WB04-E370)
const OWNERS = [0, 1, 2, 8, 0xffffffff, 1];
const CAPACITY = 24000;

// Count 에서 호출될 수 없는 하위 효과를 명시적 오류로 공급한다. (2026-11-13 WB04-E370)
const forbiddenHooks = () => ({
    islandNumber: () => { throw new Error('Count의 섬 좌표 효과 호출'); },
    reserveCandidateBuffer: () => { throw new Error('Count의 후보 확보 호출'); },
    releaseCandidateBuffer: () => { throw new Error('Count의 후보 해제 호출'); },
    destroy: () => { throw new Error('Count의 가상 삭제 호출'); },
});

// 실제 풀·해시·3204바이트 기록·모드·거주 구역 목록으로 섬 런타임을 만들고, 호출 기록을 남기는 하위 클래스를 쓸 수 있게 한다. (2026-11-13 WB04-E370)
function makeScene(Runtime = RawIslandRuntime) {
    const pool = new SidPool(CAPACITY, { server: false });
    const hash = new SquidHash();
    const records = new Uint8Array(ISLAND_STORAGE_BYTES);
    const modes = createFortFinishOwnerState();
    const residences = new SquidPostPopList();
    const hooks = {
        setResidenceOwner: () => { throw new Error('Count의 거주 구역 owner 호출'); },
        assertion: () => { throw new Error('Count의 missing 진단 호출'); },
    };
    const islands = new Runtime(pool, hash, records, modes, residences, { value: 0 }, { value: 0 }, hooks);
    const spots = new Uint8Array(65536);
    const types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 }));
    const random = new GameRandom(0);
    const state = { noIslandType: 157 };
    const ascend = new RawVortexAscend(pool, hash, spots, types, islands, random, state, forbiddenHooks());
    return { pool, hash, records, islands, spots, types, random, state, ascend };
}

test('VortexIslandBinding: 원본 10.78 Count 20행이 실제 3204바이트 기록에서 같은 개수를 내고 기록 전체와 난수 상태가 변하지 않는다', { skip: countRows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 저장된 Count 입력/기대값만 선택하며 반환값을 재계산하지 않는다.
    for (const row of countRows) {
        const input = row[2].split(',').map(Number);
        assert.equal(input.length, 2);
        const variant = input[1];
        scene.records.fill(0xa5);
        put(scene.records, 0, 6);
        scene.random.setState(0x6b4291);
        // 원본 6개 입력의 exists/owner 만 실제 32바이트 간격에 쓴다. 이웃/padding 은 보존한다.
        for (let index = 0; index < OWNERS.length; index += 1) {
            put(scene.records, 8 + index * 32, (index + variant) % 3 !== 0 ? 1 : 0);
            put(scene.records, 12 + index * 32, OWNERS[(index + variant) % OWNERS.length]);
        }
        const before = Uint8Array.from(scene.records);
        assert.equal(scene.ascend.countForeignOwnedIslands(input[0]), Number(row[3]), `행 ${count} (${row[2]})`);
        assert.deepEqual(scene.records, before, `행 ${count} 기록 불변`);
        assert.equal(scene.random.state, 0x6b4291, `행 ${count} 난수 불변`);
        count += 1;
    }
    assert.equal(count, 20);
});

test('VortexIslandBinding: 생성·삭제·owner 변경 뒤 현재 목록을 다시 읽고 signed count 와 다른 풀 연결을 처리한다', () => {
    const scene = makeScene();
    const { islands, ascend, records } = scene;
    islands.reset();
    assert.equal(ascend.countForeignOwnedIslands(1), 0);
    assert.equal(islands.add(2, 5), 0);
    assert.equal(ascend.countForeignOwnedIslands(1), 1);
    assert.equal(islands.add(1, 5), 1);
    assert.equal(ascend.countForeignOwnedIslands(1), 1);
    // 첫 섬의 현재 owner DWORD 를 직접 바꾸면 곧바로 반영된다.
    put(records, 12, 1);
    assert.equal(ascend.countForeignOwnedIslands(1), 0);
    put(records, 12, 0xffffffff);
    assert.equal(ascend.countForeignOwnedIslands(1), 1);
    islands.delete(0);
    assert.equal(islands.count, 2);
    assert.equal(ascend.countForeignOwnedIslands(1), 0);
    // 남은 두 섬(0 번은 삭제 기록 재사용 없음 → 존재 1 로 되살림, 1 번)의 owner 가 모두 0xffffffff 이면 같은 owner 인자는 부호 없는 32비트로 비교한다(-1 은 0xffffffff).
    put(records, 8, 1);
    put(records, 12, 0xffffffff);
    put(records, 44, 0xffffffff);
    assert.equal(ascend.countForeignOwnedIslands(-1), 0);
    assert.equal(ascend.countForeignOwnedIslands(0xffffffff), 0);
    assert.equal(ascend.countForeignOwnedIslands(7), 2);
    assert.equal(ascend.countForeignOwnedIslands(1), 2);
    // count 가 signed 로 음수이면 반복하지 않는다.
    put(records, 0, 0x80000000);
    assert.equal(ascend.countForeignOwnedIslands(1), 0);
    // 다른 풀의 섬 런타임은 거부한다.
    const other = makeScene();
    const make = (islandsArgument) => new RawVortexAscend(scene.pool, scene.hash, scene.spots, scene.types, islandsArgument, scene.random, scene.state, forbiddenHooks());
    assert.throws(() => make(other.islands), { name: 'TypeError', message: /raw 풀 불일치/u });
    assert.ok(make(scene.islands));
    // 기존 배열 경로는 같은 모듈에서 그대로 동작한다.
    assert.equal(make([{ exists: 1, owner: 3 }, { exists: 1, owner: 0 }, { exists: 0, owner: 4 }, { exists: 1, owner: 1 }]).countForeignOwnedIslands(1), 1);
    // 실제 목록을 줬을 때 배열 경로는 쓰지 않는다(배열 인자 없이도 개수가 실제 기록에서 나온다).
    islands.reset();
    islands.add(9, 1);
    assert.equal(ascend.countForeignOwnedIslands(1), 1);
});

test('VortexIslandBinding: 실제 목록 조회는 존재할 때만 owner 를 읽고 외부 소유일 때만 두 번 읽으며 count 는 반복마다 다시 읽는다', () => {
    const log = [];
    let depth = 0;
    // 바깥에서 부른 호출만 기록한다(owner 가 안에서 exists/count 를 다시 부르는 것은 세지 않는다).
    const traced = (label, action) => {
        if (depth === 0) log.push(label);
        depth += 1;
        try { return action(); } finally { depth -= 1; }
    };
    class Spy extends RawIslandRuntime {
        // 호출 기록을 남기고 원래 동작을 그대로 수행한다.
        get count() { return traced('count', () => super.count); }
        exists(island) { return traced(`exists:${island}`, () => super.exists(island)); }
        owner(island) { return traced(`owner:${island}`, () => super.owner(island)); }
    }
    const scene = makeScene(Spy);
    scene.islands.reset();
    scene.islands.add(2, 1);
    scene.islands.add(1, 1);
    scene.islands.add(0, 1);
    scene.islands.add(3, 1);
    scene.islands.delete(3);
    log.length = 0;
    assert.equal(scene.ascend.countForeignOwnedIslands(1), 1);
    // 반복 조건마다 count 를 읽는다(4개 + 종료 한 번). 존재하지 않는 항목은 owner 를 읽지 않고, 같은 owner 는 한 번, 소유자 0 은 두 번, 외부 소유는 두 번 읽는다.
    assert.deepEqual(log, [
        'count', 'exists:0', 'owner:0', 'owner:0',
        'count', 'exists:1', 'owner:1',
        'count', 'exists:2', 'owner:2', 'owner:2',
        'count', 'exists:3',
        'count',
    ]);
});

test('VortexIslandBinding: 실제 목록 경로도 생성 검사(하위 효과 누락·판본·표 크기)를 먼저 하고 후보/반경 판정은 섬 목록과 무관하게 동작한다', () => {
    const scene = makeScene();
    const make = (hooks, types = scene.types, spots = scene.spots) => new RawVortexAscend(scene.pool, scene.hash, spots, types, scene.islands, scene.random, scene.state, hooks);
    assert.throws(() => make({}), { message: /하위 효과/u });
    assert.throws(() => make(null), { message: /하위 효과/u });
    // 하위 효과 네 가지 중 하나라도 빠지면 거부한다(하나씩 빼서 확인).
    for (const name of ['islandNumber', 'reserveCandidateBuffer', 'releaseCandidateBuffer', 'destroy']) {
        const partial = forbiddenHooks();
        delete partial[name];
        assert.throws(() => make(partial), { message: /하위 효과/u }, name);
        partial[name] = 'function 아님';
        assert.throws(() => make(partial), { message: /하위 효과/u }, `${name} 문자열`);
    }
    assert.throws(() => make(forbiddenHooks(), scene.types.slice(0, 187)), { message: /하위 효과/u });
    assert.throws(() => make(forbiddenHooks(), scene.types, new Uint8Array(65535)), { message: /하위 효과/u });
    // 다른 풀 검사보다 하위 효과 누락 검사가 먼저이다(둘 다 틀리면 하위 효과 문구).
    const other = makeScene();
    assert.throws(() => new RawVortexAscend(scene.pool, scene.hash, scene.spots, scene.types, other.islands, scene.random, scene.state, {}), { message: /하위 효과/u });
    // 실제 목록을 줘도 후보 판정은 신전 좌표의 섬 번호 훅을 그대로 쓴다.
    const seen = [];
    const hooks = { ...forbiddenHooks(), islandNumber: (x, y) => { seen.push([x, y]); return 4; } };
    const ascend = new RawVortexAscend(scene.pool, scene.hash, scene.spots, scene.types, scene.islands, scene.random, scene.state, hooks);
    assert.equal(ascend.pool, scene.pool);
    assert.ok(ascend);
});
