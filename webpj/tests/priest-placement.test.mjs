// 사제 배치 접두 raw-priest-placement.js(RawPriestPlacement·사제 생성 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestplacement-x86.tsv 의 `originals` 1008행 — 모양 9종×좌표 16종×전역/소유자 7종을 실제 PE 의 사제 배치 접두(차단값 초기화·강제/즉시 허용·부호 확장한 소유자와 로컬 비교·모양 여백·지도 경계)로 실행해
//    허용 결과, 전역 차단값, 강제 허용·로컬 플레이어 전역, 타입 genus, 모양/후반 경계 호출 기록(`H:`/`G:`)이 같다. 모양 조회 중 전역 변경(1)과 후반 경계의 전역 변경(2)도 공급한다. `originalCD`·`original1037` 행은 사용하지 않는다.
//  - 계약: 누락 자료·비사제·비유한 입력·잘못된 연결 거부, 원본 값 인자(요청 객체/로컬 비교)의 보존, 사제 생성자와 연결해 자리 탐색의 두 번째 후보에서 채택.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E155: 사제 배치 접두 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { createPlacementState } from '../www/core/raw-canon-placement.js';
import { RawPriestPlacement, makePriestSpawnPlacementHooks } from '../www/core/raw-priest-placement.js';
import { PriestSpawnState, RawPriestSpawn } from '../www/core/raw-priest-spawn.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E155)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestplacement-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 타입 수, 사제 타입 번호이다. (2026-11-09 WB04-E155)
const ROWS = 1008;
const CAPACITY = 24000;
const TYPE_COUNT = 188;
const PRIEST = 158;

// 예외가 나는지 본다. (2026-11-09 WB04-E155)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 새 타입 표이다. (2026-11-09 WB04-E155)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));

test('사제 배치 접두: 원본 10.78 관측 1008행의 허용·전역·사건 기록이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const counts = { allowed: 0, rejected: 0, shapeCalled: 0 };
    // 구현으로 기대 여백이나 허용 여부를 계산하지 않고 원본 관찰만 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 23);
        const events = [];
        const query = { type: Number(row[1]), x: bitsToFloat(Number(row[2])), y: bitsToFloat(Number(row[3])), flags: Number(row[5]), owner: Number(row[4]), mode: Number(row[6]) };
        types[query.type].flags2 = Number(row[7]) >>> 0;
        const state = createPlacementState();
        state.forcePlacement = Number(row[8]) >>> 0;
        state.localPlayer = Number(row[9]) | 0;
        state.blockedRelation = 0xffffffff;
        const rect = { left: bitsToFloat(Number(row[10])), top: bitsToFloat(Number(row[11])), right: bitsToFloat(Number(row[12])), bottom: bitsToFloat(Number(row[13])) };
        const lower = Number(row[14]);
        const change = Number(row[15]);
        const placement = new RawPriestPlacement(pool, types, state, {
            shape: (type, argument, flags) => {
                assert.ok(type === query.type && argument === query.type && flags === query.flags && state.blockedRelation === 0);
                events.push(`H:${type}:${argument}:${flags}`);
                counts.shapeCalled += 1;
                if (change === 1) { state.localPlayer = 17; state.forcePlacement = 1; types[type].flags2 = 0; }
                return rect;
            },
            inspectGeometry: (input, localOwner) => {
                assert.ok(input.type === query.type && input.x === query.x && input.y === query.y && state.blockedRelation === 0);
                events.push(`G:${localOwner ? 1 : 0}:${input.owner}:${input.flags}:${input.mode}:${lower}`);
                state.blockedRelation = 0x77;
                if (change === 2) { state.forcePlacement = 9; state.localPlayer = -7; }
                return lower !== 0;
            }
        });
        const allowed = placement.mayPlace(query);
        const where = row.slice(0, 16).join(' ');
        assert.equal(allowed, Number(row[16]) !== 0, where);
        assert.equal(state.blockedRelation >>> 0, Number(row[18]), where);
        assert.equal(state.forcePlacement >>> 0, Number(row[19]), where);
        assert.equal(state.localPlayer >>> 0, Number(row[20]), where);
        assert.equal(types[query.type].flags2 >>> 0, Number(row[21]), where);
        assert.equal(events.join(';'), row[22], where);
        if (allowed) counts.allowed += 1; else counts.rejected += 1;
    }
    // 허용과 거부, 모양 조회가 모두 실제로 실행되었는지 확인한다.
    assert.ok(counts.allowed > 0 && counts.rejected > 0 && counts.shapeCalled > 0, JSON.stringify(counts));
});

test('사제 배치 접두: 누락 자료·비사제·비유한 입력·잘못된 연결을 거부하고 원본 값 인자를 보존한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    types[PRIEST].flags2 = 0x210000;
    const state = createPlacementState();
    const hooks = { shape: () => ({ left: 0, top: 0, right: 16, bottom: 11 }), inspectGeometry: () => true };
    assert.ok(throws(() => new RawPriestPlacement(pool, types, state, { ...hooks, shape: undefined })));
    assert.ok(throws(() => new RawPriestPlacement(pool, types, state, { ...hooks, inspectGeometry: undefined })));
    assert.ok(throws(() => new RawPriestPlacement(pool, types.slice(0, 1), state, hooks)));
    const events = [];
    const placement = new RawPriestPlacement(pool, types, state, { shape: (...args) => { events.push(args); return hooks.shape(); }, inspectGeometry: hooks.inspectGeometry });
    const query = { type: PRIEST, x: 20, y: 21, flags: 7, owner: 0, mode: 0 };
    assert.ok(throws(() => placement.mayPlace({ ...query, type: 255 })));
    types[PRIEST].flags2 = 0;
    assert.ok(throws(() => placement.mayPlace(query)));
    types[PRIEST].flags2 = 0x210000;
    assert.ok(throws(() => placement.mayPlace({ ...query, x: Infinity })));
    assert.equal(events.length, 0);
    const bad = new RawPriestPlacement(pool, types, state, { shape: () => ({ left: 0, top: 0, right: Number.NaN, bottom: 11 }), inspectGeometry: hooks.inspectGeometry });
    assert.ok(throws(() => bad.mayPlace(query)));
    assert.ok(throws(() => makePriestSpawnPlacementHooks(new SidPool(CAPACITY, { server: false }), placement, {})));
    // 모양 조회 중 호출자의 요청을 바꾸어도 원본 스택 값 인자/로컬 비교 결과는 유지한다.
    const submitted = { type: PRIEST, x: 20, y: 21, flags: 7, owner: 0x108, mode: 1 };
    const local = createPlacementState();
    local.localPlayer = 8;
    let geometryCalls = 0;
    const copied = new RawPriestPlacement(pool, types, local, {
        shape: (type, argument, flags) => {
            assert.ok(type === PRIEST && argument === PRIEST && flags === 7);
            Object.assign(submitted, { type: 82, x: 256, y: 256, flags: 0, owner: 0, mode: 0 });
            local.localPlayer = 0;
            return { left: 0, top: 0, right: 16, bottom: 11 };
        },
        inspectGeometry: (request, localOwner) => {
            assert.ok(request.type === PRIEST && request.x === 20 && request.y === 21 && request.flags === 7 && request.owner === 0x108 && request.mode === 1 && localOwner);
            geometryCalls += 1;
            return true;
        }
    });
    assert.ok(copied.mayPlace(submitted));
    assert.ok(geometryCalls === 1 && submitted.type === 82 && local.localPlayer === 0);
});

test('사제 배치 접두: 사제 생성자에 연결하면 지도 공간이 있는 자리 탐색에서 두 번째 후보를 채택한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    types[PRIEST].flags2 = 0x210000;
    types[PRIEST].maxHitPoints = 200;
    const state = createPlacementState();
    state.localPlayer = 1;
    state.blockedRelation = 0xffffffff;
    let shapeCalls = 0;
    let geometryCalls = 0;
    let pops = 0;
    let born = 0;
    const placement = new RawPriestPlacement(pool, types, state, {
        shape: (type, argument, flags) => { assert.ok(type === PRIEST && argument === PRIEST && flags === 0); shapeCalls += 1; return { left: 0, top: 0, right: 16, bottom: 11 }; },
        inspectGeometry: (query, localOwner) => { assert.ok(localOwner && query.owner === 1 && query.flags === 0 && query.mode === 0); geometryCalls += 1; return geometryCalls === 2; }
    });
    const spots = new Uint8Array(65536).fill(6);
    const spawn = new RawPriestSpawn(pool, types, spots, new PriestSpawnState(), makePriestSpawnPlacementHooks(pool, placement, {
        create: (type) => { born = pool.allocate(); pool.allocated(born)[10] = type; return born; },
        setOwner: (sid, owner) => { assert.ok(sid === born && owner === 1 && get(pool.slot(sid), 12, 2) === 0x8101); },
        pop: (sid, x, y, flags) => { assert.ok(sid === born && x === 21 && y === 21 && flags === 0); pops += 1; },
        notifySurface: () => assert.fail('다리 타입이 아니므로 표면 알림은 없다'),
        checkCarrier: (sid) => { assert.equal(sid, born); }
    }));
    spawn.spawn(20.75, 21.9, PRIEST, 0x8101);
    assert.ok(born === 15000 && shapeCalls === 2 && geometryCalls === 2 && pops === 1 && state.blockedRelation === 0);
});
