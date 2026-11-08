// 신전 Ascend raw-vortex-ascend.js(RawVortexAscend)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: vortexascend-x86.tsv 의 `originals` 행 — 진입점(Candidate/Tick/Count)과 저장된 입력 격자(genus/flags/spot/지지 타입/섬 번호/소유자/난수 시드/후보 수/버퍼 확보 성공/변화)로
//    반환, 게임 난수 상태, 외부 효과 순서(I 섬 조회/N 버퍼 확보/F 해제/D 삭제), 슬롯 60..99 의 상태 바이트, 신전 소유자가 같다. `originalCD` 행은 10.72 라 사용하지 않는다.
//  - 계약: 후보 0/반경 32→64 끝과 버퍼 확보 실패, 미연결 하위 효과·판본/표 크기 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E120: 신전 Ascend 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { GameRandom } from '../www/core/random.js';
import { RawVortexAscend } from '../www/core/raw-vortex-ascend.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E120)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'vortexascend-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 원본 입력의 신전 번호(SID 50)이다. (2026-11-08 WB04-E120)
const SOURCE = 50;
const OWNER = 34;
const OWNERS = [0, 1, 2, 8, 0xffffffff, 1];

// 단일 pool/hash/spot 와 실제 타이머/삭제 효과의 입력 저장소 장면이다. (2026-11-08 WB04-E120)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        hash: new SquidHash(),
        spots: new Uint8Array(65536),
        types: Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 })),
        islands: Array.from({ length: 6 }, () => ({ exists: 0, owner: 0 })),
        random: new GameRandom(0),
        state: { noIslandType: 157 },
        input: [],
        events: [],
        kind: '',
        island: 9,
        allocated: false,
        ascend: null
    };
    // 원본 입력이 읽는 공간 번호를 모두 확보한다.
    for (let sid = 5; sid <= 127; sid += 1) assert.equal(scene.pool.allocate(2), sid);
    // 실제 입력의 버킷 머리 삽입·raw 좌표/타입을 준비한다.
    scene.add = (sid, type, owner) => {
        const raw = scene.pool.physical(sid);
        raw[10] = type;
        raw[OWNER] = owner;
        put(raw, 0, 0x11018000);
        put(raw, 14, floatToBits(50.75));
        put(raw, 18, floatToBits(52.5));
        const bucket = scene.hash.bucket(1, 50.75, 52.5);
        put(raw, 4, bucket, 2);
        scene.hash.setBucket(1, 50.75, 52.5, sid);
    };
    // fixture 입력만 구성한다. 결과는 실제 모듈 Handle/Finder/난수가 정한다.
    scene.prepare = (row) => {
        scene.input = [];
        scene.events = [];
        scene.hash.reset();
        scene.spots.fill(0);
        scene.kind = row[1];
        scene.island = 9;
        scene.random.setState(0);
        scene.allocated = false;
        // 저장된 숫자 입력을 원본 DWORD 비트로 읽는다.
        for (const text of row[2].split(',')) scene.input.push(Number(text) >>> 0);
        // native 의 재사용 메모리도 case 마다 전 슬롯을 0 으로 초기화한다.
        for (let sid = 5; sid <= 127; sid += 1) scene.pool.physical(sid).fill(0);
        // 원본에 넣은 발자국/타입/genus 초기값을 같은 표에 준비한다.
        for (const type of [89, 94, 124, 157]) scene.types[type] = { footX: 1, footY: 1, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 };
        const input = scene.input;
        if (scene.kind === 'Candidate') {
            assert.equal(input.length, 8);
            scene.add(60, 89, input[5]);
            scene.types[89].flags2 = input[0];
            scene.types[89].flags1 = input[1];
            scene.spots[52 * 256 + 50] = input[2];
            if (input[3]) {
                scene.hash.setCell(0, 50, 52, 127);
                scene.pool.physical(127)[10] = input[3];
            }
            scene.island = input[4] | 0;
        } else if (scene.kind === 'Tick') {
            assert.equal(input.length, 6);
            const size = input[1];
            // native 입력과 같은 역순의 머리 삽입이다. 현재 next 는 실제 finder 가 소비한다.
            for (let index = size; index > 0; index -= 1) scene.add(59 + index, 89, 2);
            scene.add(SOURCE, 124, input[3]);
            scene.types[89].flags1 = 0x200000;
            scene.random.setState(input[2]);
        } else {
            assert.equal(input.length, 2);
            const variant = input[1];
            // 각 현재 섬 항목의 존재와 owner 만 입력한다.
            scene.islands.forEach((island, index) => {
                island.exists = (index + variant) % 3 !== 0 ? 1 : 0;
                island.owner = OWNERS[(index + variant) % OWNERS.length];
            });
        }
        const hooks = {
            islandNumber: (x, y) => {
                scene.events.push(`I:${floatToBits(x)}:${floatToBits(y)}`);
                if (scene.kind === 'Candidate') {
                    if (input[7] === 1) scene.pool.physical(60)[OWNER] = 1;
                    if (input[7] === 2) scene.types[89].flags1 = 0;
                    if (input[7] === 3) scene.spots[52 * 256 + 50] = 2;
                }
                return scene.island;
            },
            reserveCandidateBuffer: () => {
                scene.events.push('N:80');
                scene.allocated = input[4] !== 0;
                if (input[5] === 1) scene.pool.physical(SOURCE)[OWNER] = 2;
                if (input[5] === 2) scene.pool.physical(60)[40] = 8;
                return scene.allocated;
            },
            releaseCandidateBuffer: () => scene.events.push(`F:${scene.allocated ? 1 : 0}`),
            destroy: (sid, flags) => { scene.events.push(`D:${sid}:${flags}`); scene.pool.physical(sid)[11] |= 2; }
        };
        scene.ascend = new RawVortexAscend(scene.pool, scene.hash, scene.spots, scene.types, scene.islands, scene.random, scene.state, hooks);
    };
    return scene;
}

test('VortexAscend: 원본 10.78 후보/반경 finder/20개 제한/게임 난수/삭제/섬 소유자 계수가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 각 native 결과를 생산 API 와 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        scene.prepare(row);
        let result;
        if (scene.kind === 'Candidate') result = scene.ascend.candidate(60, 9, scene.input[6]) ? 1 : 0;
        else if (scene.kind === 'Tick') result = scene.ascend.tick(SOURCE, scene.input[0]) ? 1 : 0;
        else result = scene.ascend.countForeignOwnedIslands(scene.input[0]);
        // 입력 행의 명시 효과 순서와 현재 raw 삭제 비트를 관찰한다.
        const events = scene.events.length ? scene.events.join(';') : '-';
        const states = [];
        // native 가 저장한 40개 공간 후보의 상태 바이트를 비교한다.
        for (let sid = 60; sid < 100; sid += 1) states.push(scene.pool.slot(sid)[11]);
        const owner = scene.pool.slot(SOURCE)[OWNER];
        assert.deepEqual([result, scene.random.state, events, states.join(','), owner],
            [Number(row[3]), Number(row[4]), row[5], row[6], Number(row[7])], `Ascend 행 ${count} ${row[1]} ${row[2]}`);
        count += 1;
    }
    assert.equal(count, allRows.length / 2);
    assert.ok(count > 700);
});

test('VortexAscend: 후보 0 의 32→64 끝과 버퍼 확보 실패를 보존하고 미연결 하위 효과·판본/표 크기를 거부한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    const found = rows.find((row) => row[1] === 'Tick' && row[2] === '33,0,0,1,0,0');
    assert.ok(found);
    scene.prepare(found);
    assert.equal(scene.ascend.tick(SOURCE, 33), false);
    assert.equal(scene.random.state, 0);
    const hooks = { islandNumber: () => 9, reserveCandidateBuffer: () => true, releaseCandidateBuffer: () => {}, destroy: () => {} };
    const args = [scene.pool, scene.hash, scene.spots, scene.types, scene.islands, scene.random, scene.state];
    assert.throws(() => new RawVortexAscend(...args, {}), Error);
    assert.throws(() => new RawVortexAscend(...args, { ...hooks, destroy: undefined }), Error);
    assert.throws(() => new RawVortexAscend({ edition: 'cd1072' }, ...args.slice(1), hooks), Error);
    assert.throws(() => new RawVortexAscend(scene.pool, scene.hash, new Uint8Array(10), ...args.slice(3), hooks), Error);
    assert.throws(() => new RawVortexAscend(scene.pool, scene.hash, scene.spots, scene.types.slice(0, 10), ...args.slice(4), hooks), Error);
    const ok = new RawVortexAscend(...args, hooks);
    assert.equal(ok.pool, scene.pool);
    // 공유 noIsland 타입은 후보마다 현재 번호를 읽는다.
    scene.state.sharedNoIslandType = { value: 157 };
    assert.equal(ok.countForeignOwnedIslands(0), scene.islands.filter((island) => island.exists && island.owner).length);
});
