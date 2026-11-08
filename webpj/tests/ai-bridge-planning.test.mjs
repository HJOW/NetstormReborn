// AI 다리 전략 계획 raw-ai-bridge-planning.js(RawAiBridgePlanning)를 네 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - 무작위 시작점: ai-bridge-planning-random-x86.tsv 261행(열 = 지도 모드, TYPE, side, 경계 float 비트 4개(left/top/right/bottom), graph, owner, 난수 시작 상태, 결과 XY 8바이트 hex, 난수 종료 상태) — XY/공용 난수/전체 본체 불변.
//  - 연결 판정: ai-bridge-planning-connected-x86.tsv 85행(열 = 단일 가장자리 번호(signed), TYPE, side, graph, owner, 현재 다리 TYPE, 목표 x/y float 비트, 연결 결과) — 실제 E57 `RawPlayerPlacementAnchor.graphFor` 를 연결해 12칸 판정과 본체 불변.
//  - 빈 사각형: ai-bridge-planning-rectangle-x86.tsv 24행(열 = left/top/right/bottom(signed), 지도 모드, 결과) — 단일 칸·반전 사각형·flat 행 경계의 inclusive 반환과 본체 보존.
//  - 재시도 기한: ai-bridge-planning-delay-x86.tsv 28행(열 = +0x350 count, 현재 QWORD 시각 비트, 전체 본체 hex) — 40/120초와 큰 정밀도/무한 시각에서 +0x5d0 만 쓴 전체 본체.
//  - cpppj 계약(graph 하위가 현재 owner/좌표를 바꾸면 변경 후 필드로 연결을 판정하고 해제 토큰은 거부)과 생성자 거부를 옮겼다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E176: AI 다리 계획 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { PlayerPlacementAnchorState, RawPlayerPlacementAnchor } from '../www/core/raw-player-placement-anchor.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiBridgeCandidates } from '../www/core/raw-ai-bridge-candidates.js';
import { RawAiBridgePlanning } from '../www/core/raw-ai-bridge-planning.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E176)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const randomRows = readRows(path.join(FIXTURES, 'ai-bridge-planning-random-x86.tsv')) ?? [];
const connectedRows = readRows(path.join(FIXTURES, 'ai-bridge-planning-connected-x86.tsv')) ?? [];
const rectangleRows = readRows(path.join(FIXTURES, 'ai-bridge-planning-rectangle-x86.tsv')) ?? [];
const delayRows = readRows(path.join(FIXTURES, 'ai-bridge-planning-delay-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-09 WB04-E176)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E176)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실제 8바이트 결과의 부호 0/NaN 도 원문 비트로 비교한다. (2026-11-09 WB04-E176)
function pointHex(point) {
    const raw = new Uint8Array(8);
    put(raw, 0, floatToBits(point.x));
    put(raw, 4, floatToBits(point.y));
    return hex(raw);
}

// 결과를 계산하지 않고 원본 caller 와 동일한 현재 월드 입력만 조립한다. (2026-11-09 WB04-E176)
class PlanningScene {
    // 실제 슬롯을 확보하고 caller 본체의 두 현재 필드만 설치한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x55000000, (bytes) => bytes.fill(0xa5));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.pool = new SidPool(32768, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0 }));
        this.frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        this.surfaces = new Uint16Array(65536);
        this.spots = new Uint8Array(65536);
        this.bridge = 82;
        this.fencemark = 90;
        this.windArcher = 140;
        this.random = new GameRandom(0);
        this.now = 0;
        this.bookkeeping = new SquidPostPopState();
        this.additional = new SquidPostPopList();
        this.finder = new ContainedFinderState();
        this.anchorState = new PlayerPlacementAnchorState();
        this.anchorState.graphReady = 1;
        this.anchorState.invalidGraph = 254;
        this.anchorState.ignoreRestrictions = 0;
        this.anchorState.fenceType = 90;
        this.anchorState.windArcherType = 140;
        // 할당 순서는 원본 격리 입력과 같은 번호다.
        for (let index = 5; index <= 60; index += 1) assert.equal(this.pool.allocate(2), index);
        put(this.memory.bytes(this.token), 0x298, 7);
        put(this.memory.bytes(this.token), 0x260, 2);
    }

    // 같은 현재 원문·지도·코드의 실제 모듈을 만든다. (2026-11-09 WB04-E176)
    candidates() {
        return new RawAiBridgeCandidates(this.memory, this.pool, this.types, this.frames, this.surfaces, this.spots, { bridge: () => this.bridge, fencemark: () => this.fencemark, windArcher: () => this.windArcher });
    }

    // 같은 현재 graph 몸체이며 별도 기대값을 반환하지 않는다. (2026-11-09 WB04-E176)
    graph(sid) {
        const anchor = new RawPlayerPlacementAnchor(this.pool, this.types, this.surfaces, this.bookkeeping, this.additional, this.finder, this.anchorState);
        return anchor.graphFor(sid);
    }

    // 입력 지도 모드와 TYPE/슬롯만 원본 도구와 같은 방식으로 구성한다. (2026-11-09 WB04-E176)
    world(mode, kind, side) {
        // flat 행 순서로 caller 입력만 설치한다(비대칭 입력으로 X/Y 전도와 전체 BYTE 검사를 검증한다).
        for (let y = 0; y < 256; y += 1) {
            for (let x = 0; x < 256; x += 1) {
                const at = y * 256 + x;
                this.surfaces[at] = mode && (mode !== 3 || (x + 3 * y) % 5 !== 0) ? 50 : 0;
                this.spots[at] = mode === 2 ? 0x80 : (mode === 3 && (x + 3 * y) % 5 === 0 ? 0x40 : 0);
            }
        }
        const source = this.pool.allocated(50);
        source.fill(0);
        source[10] = kind;
        source[30] = 7;
        source[34] = 2;
        this.types[kind].flags1 = 0x800;
        this.frames[kind] = new FrameCodeTable([{ side, variant: 65, flags: 0 }]);
    }
}

test('AI 다리 계획: 실제 PE 261행의 무작위 시작점 XY/공용 난수/전체 본체를 대조한다', () => {
    assert.equal(randomRows.length, 261);
    // 독립 caller 입력만 제품에 공급한다.
    randomRows.forEach((row, index) => {
        assert.equal(row.length, 12);
        const scene = new PlanningScene();
        scene.world(Number(row[0]), Number(row[1]), Number(row[2]));
        put(scene.memory.bytes(scene.token), 0x298, Number(row[7]));
        put(scene.memory.bytes(scene.token), 0x260, Number(row[8]));
        scene.random.setState(Number(row[9]));
        const before = hex(scene.memory.bytes(scene.token));
        const planning = new RawAiBridgePlanning(scene.candidates(), scene.random, () => scene.now, undefined);
        const bounds = { left: bitsToFloat(Number(row[3])), top: bitsToFloat(Number(row[4])), right: bitsToFloat(Number(row[5])), bottom: bitsToFloat(Number(row[6])) };
        const point = planning.randomStart(scene.token, bounds);
        assert.equal(pointHex(point), row[10], `XY 행 ${index}`);
        assert.equal(scene.random.state, Number(row[11]), `난수 행 ${index}`);
        assert.equal(hex(scene.memory.bytes(scene.token)), before, `본체 행 ${index}`);
    });
});

test('AI 다리 계획: 실제 PE 85행의 12칸 가장자리 연결을 실제 graph 몸체와 함께 대조한다', () => {
    assert.equal(connectedRows.length, 85);
    // 슬롯/지도 입력만 구성하며 연결 기대를 만들지 않는다.
    connectedRows.forEach((row, index) => {
        assert.equal(row.length, 9);
        const scene = new PlanningScene();
        scene.world(0, Number(row[1]), Number(row[2]));
        put(scene.memory.bytes(scene.token), 0x298, Number(row[3]));
        put(scene.memory.bytes(scene.token), 0x260, Number(row[4]));
        scene.bridge = Number(row[5]);
        const xbits = Number(row[6]);
        const ybits = Number(row[7]);
        const target = scene.pool.allocated(60);
        target.fill(0);
        target[10] = 98;
        put(target, 14, xbits);
        put(target, 18, ybits);
        scene.types[97].flags1 = 0x800;
        const base = scene.pool.allocated(51);
        base.fill(0);
        base[10] = 97;
        base[30] = 7;
        base[34] = 2;
        const xf = bitsToFloat(xbits);
        const yf = bitsToFloat(ybits);
        const x = Math.trunc(xf);
        const y = Math.trunc(yf);
        const gx = Math.trunc(xf + Math.fround(0.9999));
        const gy = Math.trunc(yf + Math.fround(0.9999));
        if (gx >= 0 && gx < 256 && gy >= 0 && gy < 256) scene.surfaces[gy * 256 + gx] = 51;
        const edge = Number(row[0]) | 0;
        // 실제 caller 의 단일 가장자리 배치 좌표이다.
        const edges = [[-2, -3], [-2, 1], [-1, -3], [-1, 1], [0, -3], [0, 1], [-3, -2], [1, -2], [-3, -1], [1, -1], [-3, 0], [1, 0]];
        if (edge >= 0) {
            const cx = x + edges[edge][0];
            const cy = y + edges[edge][1];
            if (cx >= 0 && cx < 256 && cy >= 0 && cy < 256) scene.surfaces[cy * 256 + cx] = 50;
        }
        const before = hex(scene.memory.bytes(scene.token));
        const planning = new RawAiBridgePlanning(scene.candidates(), scene.random, () => scene.now, (sid) => scene.graph(sid));
        const result = planning.connected(scene.token, 60);
        assert.equal(result, Number(row[8]) !== 0, `연결 행 ${index} edge ${edge} 코드 ${row[2]}`);
        assert.equal(hex(scene.memory.bytes(scene.token)), before, `본체 행 ${index}`);
    });
});

test('AI 다리 계획: 실제 PE 24행의 단일 칸·반전 사각형·flat 행 경계 inclusive 반환과 본문 보존을 확인한다', () => {
    assert.equal(rectangleRows.length, 24);
    // 실제 점유 입력만 제품에 공급한다.
    rectangleRows.forEach((row, index) => {
        assert.equal(row.length, 6);
        const scene = new PlanningScene();
        scene.world(Number(row[4]), 82, 'J'.charCodeAt(0));
        const planning = new RawAiBridgePlanning(scene.candidates(), scene.random, () => scene.now, undefined);
        const before = hex(scene.memory.bytes(scene.token));
        assert.equal(planning.rectangleFree(Number(row[0]) | 0, Number(row[1]) | 0, Number(row[2]) | 0, Number(row[3]) | 0), Number(row[5]) !== 0, `사각형 행 ${index}`);
        assert.equal(hex(scene.memory.bytes(scene.token)), before);
    });
});

test('AI 다리 계획: 실제 PE 28행의 현재 QWORD 시각과 전체 count DWORD 로 +0x5d0 만 쓴 전체 본체를 대조한다', () => {
    assert.equal(delayRows.length, 28);
    // 40/120초와 큰 정밀도/무한 시각에서도 독립 원문만 기대대로 쓴다.
    delayRows.forEach((row, index) => {
        assert.equal(row.length, 3);
        const scene = new PlanningScene();
        put(scene.memory.bytes(scene.token), 0x350, Number(row[0]));
        const view = new DataView(new ArrayBuffer(8));
        view.setBigUint64(0, BigInt(row[1]), true);
        scene.now = view.getFloat64(0, true);
        const planning = new RawAiBridgePlanning(scene.candidates(), scene.random, () => scene.now, undefined);
        planning.delayRetry(scene.token);
        assert.equal(hex(scene.memory.bytes(scene.token)), row[2], `재시도 행 ${index}`);
    });
});

test('AI 다리 계획: graph 하위가 현재 owner/좌표를 바꾸면 변경 후 필드로 연결을 판정하고 해제 토큰은 거부한다', () => {
    const scene = new PlanningScene();
    scene.world(0, 82, 'J'.charCodeAt(0));
    const raw = scene.pool.allocated(60);
    raw.fill(0);
    raw[10] = 98;
    put(raw, 14, floatToBits(1));
    put(raw, 18, floatToBits(1));
    scene.surfaces[60 * 256 + 48] = 50;
    let calls = 0;
    const planning = new RawAiBridgePlanning(scene.candidates(), scene.random, () => scene.now, (sid) => {
        assert.equal(sid, 60);
        calls += 1;
        put(raw, 14, floatToBits(50.75));
        put(raw, 18, floatToBits(63.25));
        put(scene.memory.bytes(scene.token), 0x298, 9);
        return 9;
    });
    assert.ok(planning.connected(scene.token, 60) && calls === 1);
    scene.memory.free(scene.token);
    assert.ok(throws(() => planning.delayRetry(scene.token)));
});

test('AI 다리 계획 생성자/연결 계약: 후보·난수·현재 시각 조회기가 없으면 거부하고 graph 조회가 없는 연결 판정을 거부한다', () => {
    const scene = new PlanningScene();
    scene.world(0, 82, 'J'.charCodeAt(0));
    assert.ok(throws(() => new RawAiBridgePlanning(undefined, scene.random, () => 0, undefined)));
    assert.ok(throws(() => new RawAiBridgePlanning(scene.candidates(), undefined, () => 0, undefined)));
    assert.ok(throws(() => new RawAiBridgePlanning(scene.candidates(), scene.random, undefined, undefined)));
    const planning = new RawAiBridgePlanning(scene.candidates(), scene.random, () => 0, undefined);
    assert.ok(throws(() => planning.connected(scene.token, 60)));
    assert.equal(get(scene.memory.bytes(scene.token), 0x298), 7);
});
