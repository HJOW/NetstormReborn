// raw-graph.js(RawGraph)와 실제 SquidPop·SquidPostPop 이 함께 도는 장면의 실패 효과 계약을 cpppj `RawGraphTests.cpp` 의 두 시험으로 대조한다(WB04-E257).
//  - RawGraph_ProjectedPreviewAndActualPopKeepDifferentFailureEffects: 사전 계산(projected)은 void 객체·해시·spot·기록·장부를 바꾸지 않고 거부하지만, 실제 Pop 은 후처리가 실패하기 전에 공간(해시 머리·void 해제·firstPop)과 깊이를 이미 썼고 소수 좌표 재시도는 변화 없음이다.
//  - RawGraph_RegionThenAddFailureRetainsEarlierPopMutation: 영역 통지가 후보를 먼저 제거(그래프 BYTE 254, 표 한 WORD 감소)한 뒤 Add 가 실패해도 앞선 공간 점유(3×3 spot 비트)·영역의 그래프 변경·같은 버킷 root/next 체인을 되돌리지 않는다.
// 같은 cpppj 파일의 나머지 시험은 raw-graph.test.mjs·squid-postpop.test.mjs·squid-destroy-lifecycle.test.mjs 가 이미 대조한다(PORT_MAP 메모). CD 10.72 전용 시험(RefreshesFramesMapAndRawMembershipBetweenCalls)은 웹 범위 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E257)
import assert from 'node:assert/strict';
import test from 'node:test';
import { RawGraph } from '../www/core/raw-graph.js';
import { RAW_POP_RESULT } from '../www/core/raw-squid-pop-parent.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop } from '../www/core/squid-pop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { adler32, floatToBits, get, put } from './raw-support.mjs';

// 표면 타입 flags1 비트와 월드 한 변 칸 수, 유효 그래프 행 수(251)이다. (2026-11-10 WB04-E257)
const SURFACE = 0x800;
const WORLD = 256;
const GRAPH_ROWS = 251;

// 188개 기본 TYPE 표(발자국 1×1, 비용 0)와 빈 프레임 표이다. (2026-11-10 WB04-E257)
const makeTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1, cost: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: 188 }, () => []);

// 255행 그래프 표 입력이다(앞 251행만 같은 값, 나머지 4행은 0). (2026-11-10 WB04-E257)
const fullRecords = (surfaces, inUse, reserved) => Array.from({ length: 255 }, (_, index) => (index < GRAPH_ROWS ? { surfaces, inUse, reserved } : { surfaces: 0, inUse: 0, reserved: 0 }));

// 슬롯을 raw 입력으로 채운다(원문 Node). n = [x, y, footX, footY, flags1, flags2, side, variant, frame, state, extra, graph, level]. (2026-11-10 WB04-E257)
function node(pool, sid, number, n) {
    const bytes = pool.physical(sid);
    bytes.fill(0);
    put(bytes, 0, 0x501dd0);
    put(bytes, 8, 23, 2);
    bytes[10] = number;
    bytes[11] = n[9];
    put(bytes, 14, floatToBits(n[0]));
    put(bytes, 18, floatToBits(n[1]));
    bytes[30] = n[11];
    bytes[33] = n[12];
    bytes[34] = 1;
    put(bytes, 36, n[8], 4);
    bytes[40] = n[10];
}

// 예외가 났는지만 본다. (2026-11-10 WB04-E257)
function throws(operation) {
    try { operation(); } catch { return true; }
    return false;
}

// 전체 표와 전체 스택의 흔적을 Adler 쌍으로 만든다. (2026-11-10 WB04-E257)
function graphHashes(graph) {
    const records = new Uint8Array(graph.records().length * 6);
    // 각 행의 세 WORD 를 차례로 쓴다.
    graph.records().forEach((record, index) => {
        put(records, index * 6, record.surfaces & 0xffff, 2);
        put(records, index * 6 + 2, record.inUse & 0xffff, 2);
        put(records, index * 6 + 4, record.reserved & 0xffff, 2);
    });
    const stack = new Uint8Array(graph.floodStack().length * 4);
    graph.floodStack().forEach((value, index) => put(stack, index * 4, value, 4));
    return [adler32(records), adler32(stack)];
}

test('RawGraph: 사전 계산 거부는 아무것도 쓰지 않지만 실제 Pop 의 후처리 실패는 앞선 공간/깊이 변경을 남긴다', () => {
    const types = makeTypes();
    const frames = makeFrames();
    types[74].flags1 = SURFACE;
    frames[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    const pool = new SidPool(32768, { server: false });
    const sid = pool.allocate(2);
    node(pool, sid, 74, [90, 90, 1, 1, 0x800, 0, 65, 80, 0, 4, 0, 254, 1]);
    const hash = new SquidHash();
    const spots = new Uint8Array(WORLD * WORLD);
    // 정상 251개가 모두 사용 중인 입력은 원본 전역 재구성 경로를 요구한다.
    const graph = new RawGraph(pool, hash, spots, types, frames, { records: fullRecords(1, 1, 71) });
    const state = new SquidPostPopState();
    state.graphsEnabled = true;
    const post = new SquidPostPop(pool, types, state, graph);
    const pop = new SquidPop(pool, hash, spots, { postPop: post });
    const raw = pool.bytes();
    const before = graphHashes(graph);
    // 각 사전 계산은 현재 void 객체에 쓰지 않는다.
    for (const x of [20, 20.25]) assert.ok(throws(() => graph.validatePostPop(sid, false, true, { x, y: 20, level: 1, type: types[74] })));
    assert.deepEqual(pool.bytes(), raw);
    assert.deepEqual(graphHashes(graph), before);
    assert.ok(state.depth === 0 && state.totalCost === 0);
    assert.equal(hash.bucket(1, 20, 20), 0);
    assert.ok(spots.every((value) => value === 0));
    // 004b02d0/004ac560 은 후처리 실패 전에 공간과 state/depth 를 쓴다.
    assert.ok(throws(() => pop.pop(sid, types[74], 1, 1, 20, 20)));
    const current = pool.slot(sid);
    assert.ok(current[11] === 0 && current[33] === 1 && current[40] === 0x80);
    assert.ok(hash.bucket(1, 20, 20) === sid && state.depth === 1 && state.totalCost === 0);
    assert.equal(new DataView(current.buffer, current.byteOffset).getFloat32(14, true), 20);
    assert.deepEqual(graphHashes(graph), before);
    // 소수 좌표로 다시 시도하면 이미 점유한 객체라 변화 없이 끝나고 깊이도 늘지 않는다.
    const failedRaw = pool.bytes();
    assert.equal(pop.pop(sid, types[74], 1, 1, 20.25, 20), RAW_POP_RESULT.unchanged);
    assert.deepEqual(pool.bytes(), failedRaw);
    assert.equal(state.depth, 1);
});

test('RawGraph: 영역 통지 뒤 Add 가 실패해도 앞선 Pop 의 공간 점유와 영역의 그래프 변경은 되돌리지 않는다', () => {
    const types = makeTypes();
    types[74].flags1 = SURFACE;
    types[75].flags1 = SURFACE;
    types[74].flags2 = 8;
    types[74].footX = 3;
    types[74].footY = 3;
    types[75].footX = 3;
    const frames = makeFrames();
    frames[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    frames[75] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    const pool = new SidPool(32768, { server: false });
    const root = pool.allocate(2);
    const candidate = pool.allocate(2);
    node(pool, root, 74, [90, 90, 3, 3, 0x800, 8, 65, 80, 0, 4, 0, 254, 1]);
    node(pool, candidate, 75, [21, 20, 3, 1, 0x800, 0, 65, 80, 0, 0, 0, 0, 1]);
    const hash = new SquidHash();
    hash.setBucket(1, 21, 20, candidate);
    const spots = new Uint8Array(WORLD * WORLD);
    spots[20 * WORLD + 21] = 8;
    // 한 후보를 빼도 어느 그래프도 비지 않는 입력이다.
    const records = fullRecords(2, 1, 71);
    const graph = new RawGraph(pool, hash, spots, types, frames, { records });
    const state = new SquidPostPopState();
    state.graphsEnabled = true;
    const post = new SquidPostPop(pool, types, state, graph);
    const pop = new SquidPop(pool, hash, spots, { postPop: post });
    const raw = pool.bytes();
    const beforeSpots = spots.slice();
    const before = graphHashes(graph);
    // 1단계의 x 20/21 은 같은 2칸 버킷이다.
    const oldHead = hash.bucket(1, 20, 20);
    assert.equal(oldHead, candidate);
    assert.ok(throws(() => pop.pop(root, types[74], 1, 1, 20, 20, 0x201)));
    const current = pool.slot(root);
    assert.ok(current[11] === 0 && current[33] === 1 && current[40] === 0x80 && current[4] === oldHead && current[5] === 0);
    assert.ok(hash.bucket(1, 20, 20) === root && state.depth === 1 && state.totalCost === 0);
    // 원문 최초 발자국의 실제 점유 쓰기만 독립 기대값에 반영한다.
    const expectedSpots = beforeSpots.slice();
    for (let y = 18; y <= 20; y += 1) for (let x = 18; x <= 20; x += 1) expectedSpots[y * WORLD + x] |= 8;
    assert.deepEqual(spots, expectedSpots);
    // 영역이 먼저 후보를 제거하고 같은 버킷의 root/next 체인은 보존한다.
    assert.notDeepEqual(graphHashes(graph), before);
    assert.ok(pool.slot(candidate)[30] === 254 && hash.bucket(1, 21, 20) === root);
    // 전체 표에서 감소한 surface WORD 하나만 달라야 한다.
    const expectedRecords = fullRecords(2, 1, 71);
    expectedRecords[0].surfaces = 1;
    graph.records().forEach((actual, index) => {
        assert.ok(actual.surfaces === expectedRecords[index].surfaces && actual.inUse === expectedRecords[index].inUse && actual.reserved === expectedRecords[index].reserved, `표 ${index}`);
    });
    // 후보 슬롯은 그래프 BYTE 만 254 로 바뀌고 나머지 바이트는 그대로다.
    const offset = candidate * 50;
    const expectedCandidate = raw.slice(offset, offset + 50);
    expectedCandidate[30] = 254;
    assert.deepEqual(pool.slot(candidate), expectedCandidate);
    assert.ok(get(pool.slot(root), 14, 4) === floatToBits(20));
});
