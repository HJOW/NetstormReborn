// AI 다리 경로 raw-ai-bridge-path.js(RawAiBridgePath)를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - 초기화: ai-bridge-initialize-x86.tsv 144입력(두 x87 정밀도 288반환)의 전체 0xb08 바이트. 열 = 소스 x/y 비트, 목표 x/y 비트(NaN/무한대/음수 0 포함), +0x628 사용 가능 수, 본체 채움 BYTE, 전체 본체 hex.
//  - 단계: ai-bridge-step-x86.tsv 600입력. 열 = 상태(+0x5f0), wanted(+0x604), available(+0x628), 하위 모드, 호출 뒤 상태, 완료 반환, 반환 소스 x/y 비트, 첫 하위 진입(없으면 -).
//  - cpppj 계약: 실제 초기화 연결 Step, 소비(pending/다른 TYPE·패턴 건너뜀·첫 일치·DWORD 전체 비교·현재 rate/시각), Extend(대기 시 무배치·배치 뒤 현재 owner/패턴 재조회·무효 결과의 효과 생략·디버그 표식), Advance(시작점 없음/첫 시작점/대기 점/timeout signed 감소와 출력 관문),
//    미연결 하위에서 앞선 효과 보존, 그리고 E176 `RawAiBridgeCandidates` 와 조립하는 두 시험(초기화 + 예산 Step, 시작점 + Advance).
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E177: AI 다리 경로 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiBridgeCandidates } from '../www/core/raw-ai-bridge-candidates.js';
import { RawAiBridgePath } from '../www/core/raw-ai-bridge-path.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E177)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const initializeRows = readRows(path.join(FIXTURES, 'ai-bridge-initialize-x86.tsv')) ?? [];
const stepRows = readRows(path.join(FIXTURES, 'ai-bridge-step-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-09 WB04-E177)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E177)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

const F = Math.fround;

// 같은 실제 본체/풀과 하위 호출의 순서·현재 값 변경을 조립한다. (2026-11-09 WB04-E177)
class BridgePathScene {
    // 실제 부모/내용물 다섯 슬롯을 생성하고 원문 단계 하위만 연결한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x55000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.pool = new SidPool(32768, { server: false });
        this.finder = new ContainedFinderState();
        this.now = 100.00000001;
        this.limit = 256;
        this.bridge = 82;
        this.kind = 1;
        this.startMarker = 93;
        this.endMarker = 94;
        this.targetMarker = 95;
        this.trace = [];
        this.destroyed = [];
        this.mode = 0;
        this.first = { x: 20, y: 21 };
        this.duringDeploy = null;
        this.duringTechnology = null;
        this.duringDestroy = null;
        this.duringRefresh = null;
        this.duringMarker = null;
        // 연속된 실제 SID 의 링크/필드 초기값을 명확히 한다.
        for (let number = 5; number <= 9; number += 1) {
            assert.equal(this.pool.allocate(2), number);
            const raw = this.pool.allocated(number);
            raw.fill(0);
            raw[10] = number === 5 ? 1 : 6;
        }
        this.field(0x260, 1);
        this.field(0x3d8, 7);
        this.setPoint(0x6bc, { x: 101, y: 202 });
        this.setPoint(0x6ac, { x: 11, y: 12 });
        this.field(0x6b4, 3);
        this.field(0x6b8, 2);
        this.field(0x600, floatToBits(0.25));
        this.hooks = {
            initialize: (brain, source, target) => {
                assert.ok(brain === this.token && source.x === 10 && source.y === 20 && target.x === 30 && target.y === 40);
                this.trace.push('initialize');
                if (this.mode === 2) this.field(0x5f0, 0xffffffff);
            },
            prepare: (brain) => { assert.equal(brain, this.token); this.trace.push('prepare'); if (this.mode === 2) this.field(0x5f0, 0xffffffff); return this.mode !== 0; },
            search: (brain) => { assert.equal(brain, this.token); this.trace.push('search'); if (this.mode === 2) this.field(0x5f0, 0xffffffff); return this.mode !== 0; },
            start: (brain) => { assert.equal(brain, this.token); this.trace.push('start'); return this.first; },
            deploy: (brain, type, point, pattern, direction) => {
                assert.ok(brain === this.token && type === this.bridge && point.x === 11 && point.y === 12 && pattern === 3 && direction === 2);
                this.trace.push('deploy');
                if (this.duringDeploy) this.duringDeploy();
            },
            technology: (owner) => { this.trace.push(`technology${owner}`); if (this.duringTechnology) this.duringTechnology(); return 5; },
            destroy: (sid, flags) => { assert.equal(flags, 0); this.trace.push('destroy'); this.destroyed.push(sid); if (this.duringDestroy) this.duringDestroy(); },
            refresh: (brain, parent) => { assert.ok(brain === this.token && parent === 5); this.trace.push('refresh'); if (this.duringRefresh) this.duringRefresh(); },
            marker: (brain, type) => { assert.equal(brain, this.token); this.trace.push(`marker${type}`); if (this.duringMarker) this.duringMarker(); },
            log: (brain, logKind, text) => { assert.ok(brain === this.token && logKind === this.kind && text === 'Bridge timeout\n'); this.trace.push('timeout'); }
        };
    }

    // 지정한 실제 본체 DWORD 를 읽거나(값 없음) 주변 BYTE 를 보존하며 쓴다. (2026-11-09 WB04-E177)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 두 float 의 원문 비트를 기록한다. (2026-11-09 WB04-E177)
    setPoint(at, point) {
        this.field(at, floatToBits(point.x));
        this.field(at + 4, floatToBits(point.y));
    }

    // 다음 링크/패턴/현재 flags 를 지정한 내용물 슬롯에 기록한다. (2026-11-09 WB04-E177)
    content(number, next, pattern, flags) {
        const raw = this.pool.allocated(number);
        put(raw, 4, next, 2);
        put(raw, 12, pattern, 2);
        put(raw, 20, flags, 2);
    }

    // 같은 저장소의 부모 첫 링크를 기록한다. (2026-11-09 WB04-E177)
    head(number) { put(this.pool.allocated(5), 6, number, 2); }

    // 현재 전역 조회기와 같은 풀을 사용하는 복원 모듈을 만든다. (2026-11-09 WB04-E177)
    path() {
        return new RawAiBridgePath(this.memory, this.pool, this.finder, {
            now: () => this.now, mapLimit: () => this.limit, bridgeType: () => this.bridge, logKind: () => this.kind,
            startMarker: () => this.startMarker, endMarker: () => this.endMarker, targetMarker: () => this.targetMarker
        }, this.hooks);
    }
}

test('AI 다리 경로: 실제 PE 초기화 144입력의 전체 0xb08 바이트를 대조한다', () => {
    assert.equal(initializeRows.length, 144);
    // source/target/count/pattern 은 caller 입력이며 전체 기대 본체를 재계산하지 않는다.
    initializeRows.forEach((row, index) => {
        assert.equal(row.length, 7);
        const scene = new BridgePathScene();
        scene.memory.bytes(scene.token).fill(Number(row[5]) & 0xff);
        scene.field(0x628, Number(row[4]));
        // NaN/무한대/음수 0 도 원문 비트로 전달한다.
        const source = { x: bitsToFloat(Number(row[0])), y: bitsToFloat(Number(row[1])) };
        const target = { x: bitsToFloat(Number(row[2])), y: bitsToFloat(Number(row[3])) };
        assert.ok(scene.path().initialize(scene.token, source, target));
        assert.equal(hex(scene.memory.bytes(scene.token)), row[6], `초기화 전체 본체 행 ${index}`);
    });
});

test('AI 다리 경로: 실제 초기화 콜백을 연결했을 때 Step 은 본체 준비 뒤 현재 상태를 한 단계만 증가시킨다', () => {
    const scene = new BridgePathScene();
    scene.field(0x5f0, 1);
    scene.field(0x628, 0);
    scene.hooks.initialize = (token, source, target) => { assert.ok(scene.path().initialize(token, source, target)); };
    const source = { x: F(20.75), y: F(-21.75) };
    assert.ok(!scene.path().step(scene.token, source, { x: F(30.5), y: F(40.5) }));
    assert.ok(scene.field(0x5f0) === 2 && scene.field(0x638) === 20 && scene.field(0x63c) === (-21 >>> 0) && scene.field(0x640) === 30 && scene.field(0x644) === 40 && scene.field(0x6c8) === 0xffffffff);
    assert.ok(source.x === F(20.75) && source.y === F(-21.75) && scene.field(0x6bc) === 0xc61c4000 && scene.field(0x6c0) === 0xc61c4000);
});

test('AI 다리 경로: 실제 PE 600입력의 정상 반환과 하위 진입 순서를 재생한다', () => {
    assert.equal(stepRows.length, 600);
    // 각 행은 독립 본체/원본 입력이며 기대 상태를 재계산하지 않는다.
    stepRows.forEach((columns, index) => {
        assert.equal(columns.length, 9);
        const scene = new BridgePathScene();
        scene.field(0x5f0, Number(columns[0]));
        scene.field(0x604, Number(columns[1]));
        scene.field(0x628, Number(columns[2]));
        scene.mode = Number(columns[3]);
        const source = { x: 10, y: 20 };
        const done = scene.path().step(scene.token, source, { x: 30, y: 40 });
        assert.ok(scene.field(0x5f0) === Number(columns[4]) && done === (Number(columns[5]) !== 0), `상태 행 ${index}`);
        assert.ok(floatToBits(source.x) === Number(columns[6]) && floatToBits(source.y) === Number(columns[7]), `소스 행 ${index}`);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace[0], columns[8], `하위 진입 행 ${index}`);
    });
});

test('AI 다리 경로: pending/다른 TYPE/다른 패턴을 건너뛰며 첫 사용 가능 조각만 삭제한다', () => {
    const scene = new BridgePathScene();
    scene.head(6);
    scene.content(6, 7, 3, 0x140);
    scene.content(7, 8, 4, 0x40);
    scene.content(8, 9, 3, 0x40);
    scene.content(9, 0, 3, 0x40);
    scene.path().consume(scene.token, 5, 3);
    assert.deepEqual(scene.destroyed, [8]);
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, scene.field(0x630), true);
    view.setUint32(4, scene.field(0x634), true);
    assert.equal(view.getFloat64(0, true), scene.now + 0.25);
});

test('AI 다리 경로: 패턴 인자는 DWORD 전체이므로 65536 을 WORD 0 으로 잘라 소비하지 않는다', () => {
    const scene = new BridgePathScene();
    scene.head(6);
    scene.content(6, 0, 0, 0x40);
    scene.field(0x630, 123);
    scene.field(0x634, 456);
    scene.path().consume(scene.token, 5, 65536);
    assert.ok(scene.trace.length === 0 && scene.field(0x630) === 123 && scene.field(0x634) === 456);
});

test('AI 다리 경로: 삭제 효과 뒤 현재 rate 와 double now 를 사용한다', () => {
    const scene = new BridgePathScene();
    scene.head(6);
    scene.content(6, 0, 3, 0x40);
    scene.duringDestroy = () => { scene.now = 123.00000001; scene.field(0x600, floatToBits(0.125)); };
    scene.path().consume(scene.token, 5, 3);
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, scene.field(0x630), true);
    view.setUint32(4, scene.field(0x634), true);
    assert.equal(view.getFloat64(0, true), 123.12500001);
});

test('AI 다리 경로: 상태가 완료되지 않았으면 입력 시작점과 모든 배치/소비 효과를 보존한다', () => {
    const scene = new BridgePathScene();
    scene.field(0x5f0, 4);
    scene.field(0x604, 8);
    scene.field(0x628, 7);
    const result = scene.path().extend(scene.token, { x: 10, y: 20 }, { x: 30, y: 40 });
    assert.ok(result.x === 10 && result.y === 20 && scene.trace.length === 0 && scene.field(0x5f0) === 4);
});

test('AI 다리 경로: 배치 뒤 현재 owner/패턴을 소비하고 다시 현재 owner 로 재고를 갱신한다', () => {
    const scene = new BridgePathScene();
    scene.field(0x5f0, 5);
    scene.head(6);
    scene.content(6, 0, 7, 0x40);
    scene.duringDeploy = () => { scene.field(0x260, 2); scene.field(0x6b4, 7); scene.setPoint(0x6bc, { x: 220, y: 221 }); };
    scene.duringDestroy = () => { scene.field(0x260, 3); };
    const result = scene.path().extend(scene.token, { x: 10, y: 20 }, { x: 30, y: 40 });
    assert.ok(result.x === 101 && result.y === 202 && JSON.stringify(scene.trace) === '["deploy","technology2","destroy","technology3","refresh"]' && scene.field(0x5f0) === 0);
});

test('AI 다리 경로: 무효 결과/경계/NaN 은 상태 0 복귀만 유지하고 배치·기술 조회를 생략한다', () => {
    // 각 좌표는 원본 strict 0..상한 검사에서 독립적으로 거부된다.
    for (const point of [{ x: 0, y: 20 }, { x: 256, y: 20 }, { x: 10, y: 0 }, { x: 10, y: 256 }, { x: -10000, y: -10000 }, { x: Number.NaN, y: 20 }]) {
        const scene = new BridgePathScene();
        scene.field(0x5f0, 5);
        scene.setPoint(0x6bc, point);
        scene.path().extend(scene.token, { x: 10, y: 20 }, { x: 30, y: 40 });
        assert.ok(scene.trace.length === 0 && scene.field(0x5f0) === 0);
    }
});

test('AI 다리 경로: 디버그 표식은 배치/소비/갱신 뒤 현재 종류를 읽으며 둘째 TYPE 도 다시 읽는다', () => {
    const scene = new BridgePathScene();
    scene.field(0x5f0, 5);
    scene.duringRefresh = () => { scene.kind = 29; };
    scene.duringMarker = () => { scene.targetMarker = 99; };
    scene.path().extend(scene.token, { x: 10, y: 20 }, { x: 30, y: 40 });
    assert.equal(JSON.stringify(scene.trace), '["deploy","technology1","technology1","refresh","marker94","marker99"]');
});

test('AI 다리 경로: 시작점 없음은 성공 반환을 유지하고 timeout 을 감소하지 않는다', () => {
    const scene = new BridgePathScene();
    scene.setPoint(0x3d0, { x: -10000, y: -10000 });
    scene.first = { x: -10000, y: -10000 };
    assert.ok(scene.path().advance(scene.token, { x: 30, y: 40 }) && scene.field(0x3d8) === 7 && JSON.stringify(scene.trace) === '["start"]');
});

test('AI 다리 경로: 유효한 첫 시작점은 저장한 뒤 표식을 놓고 기존 timeout 을 유지한다', () => {
    const scene = new BridgePathScene();
    scene.kind = 29;
    assert.ok(scene.path().advance(scene.token, { x: 30, y: 40 }) && scene.field(0x3d0) === floatToBits(20) && scene.field(0x3d8) === 7 && JSON.stringify(scene.trace) === '["start","marker93"]');
});

test('AI 다리 경로: 아직 기다리는 유효 연결점도 성공으로 취급해 timeout 1 을 쓴다', () => {
    const scene = new BridgePathScene();
    scene.setPoint(0x3d0, { x: 10, y: 20 });
    scene.field(0x5f0, 4);
    scene.field(0x604, 8);
    scene.field(0x628, 7);
    assert.ok(scene.path().advance(scene.token, { x: 30, y: 40 }) && scene.field(0x3d8) === 1 && scene.trace.length === 0);
});

test('AI 다리 경로: 무효 결과에서만 signed 감소/실패/원래 출력 관문을 적용한다', () => {
    // 양수 2·1·0·부호 경계의 감김과 signed 판정을 확인한다.
    for (const timeout of [2, 1, 0, 0x80000000]) {
        const scene = new BridgePathScene();
        scene.setPoint(0x3d0, { x: 10, y: 20 });
        scene.field(0x5f0, 5);
        scene.setPoint(0x6bc, { x: -10000, y: -10000 });
        scene.field(0x3d8, timeout);
        scene.field(0xb00, 1);
        const result = scene.path().advance(scene.token, { x: 30, y: 40 });
        const success = timeout === 2 || timeout === 0x80000000;
        assert.ok(result === success && scene.field(0x3d8) === (timeout === 0x80000000 ? 0x7fffffff : 1) && JSON.stringify(scene.trace) === JSON.stringify(success ? [] : ['timeout']));
    }
    const quiet = new BridgePathScene();
    quiet.setPoint(0x3d0, { x: 10, y: 20 });
    quiet.field(0x5f0, 5);
    quiet.setPoint(0x6bc, { x: -10000, y: -10000 });
    quiet.field(0x3d8, 1);
    quiet.hooks.log = undefined;
    assert.ok(!quiet.path().advance(quiet.token, { x: 30, y: 40 }) && quiet.field(0x3d8) === 1);
});

test('AI 다리 경로: 미연결 하위에서 앞선 상태 0 복귀/배치 효과를 취소하지 않는다', () => {
    const scene = new BridgePathScene();
    scene.field(0x5f0, 5);
    scene.hooks.technology = undefined;
    assert.ok(throws(() => scene.path().extend(scene.token, { x: 10, y: 20 }, { x: 30, y: 40 })) && scene.field(0x5f0) === 0 && JSON.stringify(scene.trace) === '["deploy"]');
});

// E176 의 실제 후보 준비/시작점을 같은 본체/지도로 만드는 도우미 장면이다(원본 후보 시험의 지도 구성과 같다). (2026-11-09 WB04-E177)
function candidateScene(mode, kind, side, x, y) {
    const scene = new BridgePathScene();
    const surfaces = new Uint16Array(65536);
    const spots = new Uint8Array(65536);
    const types = Array.from({ length: 188 }, () => ({ flags1: 0 }));
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    // 실제 후보 모듈이 읽는 source 50 까지 확보한다.
    for (let number = 10; number <= 50; number += 1) assert.equal(scene.pool.allocate(2), number);
    for (let cy = 0; cy < 256; cy += 1) {
        for (let cx = 0; cx < 256; cx += 1) {
            let present = mode !== 0 && (mode !== 4 || (cx === x && cy === y));
            if (mode === 3) present = (cx + cy) % 3 !== 0;
            if (present) surfaces[cy * 256 + cx] = 50;
        }
    }
    const raw = scene.pool.allocated(50);
    raw.fill(0);
    raw[10] = kind;
    raw[30] = 7;
    raw[34] = 2;
    types[kind].flags1 = 0x800;
    frames[kind] = new FrameCodeTable([{ side, variant: 65, flags: 0 }]);
    scene.candidates = () => new RawAiBridgeCandidates(scene.memory, scene.pool, types, frames, surfaces, spots, { bridge: () => scene.bridge, fencemark: () => 90, windArcher: () => 140 });
    return scene;
}

test('AI 다리 경로: 실제 초기화와 후보 준비를 Step 에 연결하면 예산 중단 동안 상태 2 를 유지한 뒤에만 검색 3 으로 진행한다', () => {
    const scene = candidateScene(0, 97, 65, 50, 50);
    scene.memory.bytes(scene.token).fill(165);
    for (const [offset, value] of [[0x5f8, 7], [0x5fc, 2], [0x690, 0], [0x694, 0], [0x698, 2], [0x69c, 1], [0x6a0, 50], [0x6a4, 50]]) scene.field(offset, value);
    scene.hooks.initialize = (token, source, target) => { assert.ok(scene.path().initialize(token, source, target)); };
    scene.hooks.prepare = (token) => scene.candidates().prepare(token);
    const path = scene.path();
    scene.field(0x5f0, 1);
    const source = { x: 50, y: 50 };
    assert.ok(!path.step(scene.token, source, { x: 80, y: 80 }) && scene.field(0x5f0) === 2);
    assert.ok(!path.step(scene.token, source, { x: 80, y: 80 }) && scene.field(0x5f0) === 2);
    // 실제 나선 상태가 완료할 때까지 동일 Step 을 호출한다.
    for (let call = 0; call < 16 && scene.field(0x5f0) === 2; call += 1) assert.ok(!path.step(scene.token, source, { x: 80, y: 80 }));
    assert.ok(scene.field(0x5f0) === 3 && source.x === 50 && source.y === 50 && scene.candidates().pool === scene.pool);
});

test('AI 다리 경로: 실제 현재 graph/owner 를 고르는 시작점 탐색을 Advance 에 연결하고 시작점 없음은 기존 timeout 을 변경하지 않는다', () => {
    const scene = candidateScene(1, 97, 80, 50, 50);
    const body = () => scene.memory.bytes(scene.token);
    scene.memory.bytes(scene.token).fill(165);
    scene.field(0x298, 7);
    scene.field(0x260, 2);
    scene.setPoint(0x3d0, { x: -10000, y: -10000 });
    scene.field(0x3d8, 9);
    const bounds = { left: 49.75, top: 61.5, right: 55.25, bottom: 67.5 };
    scene.hooks.start = (token, target) => scene.candidates().start(token, target, bounds);
    const path = scene.path();
    assert.ok(path.advance(scene.token, { x: F(52.9), y: F(64.1) }));
    assert.ok(bitsToFloat(get(body(), 0x3d0)) === 52 && bitsToFloat(get(body(), 0x3d4)) === 64 && get(body(), 0x3d8) === 9);
    scene.setPoint(0x3d0, { x: -10000, y: -10000 });
    scene.field(0x298, 0x107);
    assert.ok(path.advance(scene.token, { x: F(52.9), y: F(64.1) }));
    assert.ok(bitsToFloat(get(body(), 0x3d0)) === -10000 && bitsToFloat(get(body(), 0x3d4)) === -10000 && get(body(), 0x3d8) === 9);
});

test('AI 다리 경로 생성자: 본체 소유자/PATCH 풀/내용물 상태/전역 조회기가 없으면 거부하고 가짜 본체를 거부한다', () => {
    const scene = new BridgePathScene();
    const globals = { now: () => 0, mapLimit: () => 256, bridgeType: () => 82, logKind: () => 0, startMarker: () => 0, endMarker: () => 0, targetMarker: () => 0 };
    assert.ok(throws(() => new RawAiBridgePath(undefined, scene.pool, scene.finder, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiBridgePath(scene.memory, {}, scene.finder, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiBridgePath(scene.memory, scene.pool, undefined, globals, scene.hooks)));
    for (const name of Object.keys(globals)) assert.ok(throws(() => new RawAiBridgePath(scene.memory, scene.pool, scene.finder, { ...globals, [name]: undefined }, scene.hooks)));
    assert.equal(scene.path().pool, scene.pool);
    assert.ok(throws(() => scene.path().advance(scene.memory.allocate(16), { x: 1, y: 1 })));
});
