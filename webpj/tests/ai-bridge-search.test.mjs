// AI 다리 검색 raw-ai-bridge-search.js(RawAiBridgeSearch)를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - 모양: ai-bridge-shape-x86.tsv 의 208행(26개 패턴×4회전, 호출 전 0xbc 채움 BYTE 에서 grid/끝점 여백을 지우지 않는 전체 버퍼). 열 = 패턴, 회전, 채움 BYTE, 전체 0xbc hex.
//  - 점유: ai-bridge-clear-x86.tsv 의 416행(실제 decoder 순서·비대칭 점유 지도·소수 좌표에서 첫 차단 반환). 열 = 패턴, 회전, x/y float 비트, 지도 모드, 기대 반환.
//  - 점수: ai-bridge-score-x86.tsv 의 640행(x87 의 X 보존/Y float 저장·signed 감김·금지/벌점·미갱신 결과). 열 = 패턴, 회전, 배치 x/y 비트, 목표 x/y, 제외 끝점, 임시 XY 채움 BYTE, 기대 점수, 기대 임시 XY.
//  - 검색: ai-bridge-search-x86.tsv 의 72장면×4호출(288)에서 본체 0xb08/공용 난수/임시 XY/완료값을 모두 대조한다. 열 = 장면, 지도 모드, 난수 상태, 임시 XY, 초기 본체, 호출별 `완료,난수,임시XY,본체;…`.
//  - cpppj 계약: 실제 초기화 → 24칸 후보 → 8개 검색 → 대기 → 결과 반환을 E176 후보/E177 경로와 함께 수행하는 조립 시험, 생성자/프레임 범위 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E178: AI 다리 검색 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiBridgeCandidates } from '../www/core/raw-ai-bridge-candidates.js';
import { RawAiBridgePath } from '../www/core/raw-ai-bridge-path.js';
import { RawAiBridgeSearch, SHAPE_BYTES } from '../www/core/raw-ai-bridge-search.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E178)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const shapeRows = readRows(path.join(FIXTURES, 'ai-bridge-shape-x86.tsv')) ?? [];
const clearRows = readRows(path.join(FIXTURES, 'ai-bridge-clear-x86.tsv')) ?? [];
const scoreRows = readRows(path.join(FIXTURES, 'ai-bridge-score-x86.tsv')) ?? [];
const searchRows = readRows(path.join(FIXTURES, 'ai-bridge-search-x86.tsv')) ?? [];

// 보고서의 실제 bridge.type 코드 입력 240 BYTE(60개 × side, variant, number, flags)이며 모양/점수 결과가 아니다. (2026-11-09 WB04-E178)
const BRIDGE_CODES = '4150010041500b2041501440425001004250020042500b2042500c2042501440435001004350020043500b2043500c2043501440445001004450020044500b2044500c2044501440455001004550020045500b2045500c20455014404650010046500b20465014404750010047500b20475014404850010048500b20485014404950010049500b20495014404a5001004a5002004a500b204a500b204a500c204a5014404b5001004b5002004b500b204b500b204b500c204b5014404c5001004c500b204c5014404d5001004d500b204d5014404e5001004e500b204e5014404f5001004f500b204f50144050500000';

// 예외가 나는지 본다. (2026-11-09 WB04-E178)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E178)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 16진 문자열을 BYTE 배열로 되돌린다. (2026-11-09 WB04-E178)
const bytesOf = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// 임시 XY 두 float 의 원문 비트를 NaN 도 바꾸지 않고 보존한 객체로 만든다. (2026-11-09 WB04-E178)
function pointFromBytes(raw) {
    const xBits = get(raw, 0);
    const yBits = get(raw, 4);
    return { x: bitsToFloat(xBits), y: bitsToFloat(yBits), xBits, yBits };
}

// 임시 XY 의 전체 비트를 원문과 비교할 16진 문자열이다(갱신 시 xBits/yBits 는 지워진다). (2026-11-09 WB04-E178)
function pointHex(point) {
    const raw = new Uint8Array(8);
    put(raw, 0, point.xBits ?? floatToBits(point.x));
    put(raw, 4, point.yBits ?? floatToBits(point.y));
    return hex(raw);
}

// 결과를 계산하지 않고 현재 caller 의 모양 프레임/spot 입력만 조립한다. (2026-11-09 WB04-E178)
class SearchScene {
    // 실제 본체/프레임/점유/난수를 같은 장면으로 만든다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x55000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.frames = Array.from({ length: 188 }, () => []);
        this.spots = new Uint8Array(65536);
        this.bridge = 82;
        this.random = new GameRandom();
        const raw = bytesOf(BRIDGE_CODES);
        const codes = [];
        // 실제 코드 순서/중복을 유지한다.
        for (let index = 0; index < raw.length; index += 4) codes.push({ side: raw[index], variant: raw[index + 1], number: raw[index + 2], flags: raw[index + 3] });
        this.frames[this.bridge] = codes;
    }

    // 제품과 검사 모두 같은 현재 저장소를 참조한다. (2026-11-09 WB04-E178)
    search() { return new RawAiBridgeSearch(this.memory, this.frames, this.spots, () => this.bridge, this.random); }

    // 원본 관측에 쓴 지도 입력이며 후보 점수/배치를 만들지 않는다(행 순서를 flat 주소와 같게 한다). (2026-11-09 WB04-E178)
    setSpots(mode, searching) {
        for (let y = 0; y < 256; y += 1) {
            for (let x = 0; x < 256; x += 1) {
                let value = 0;
                if (mode === 1) value = searching ? 0x80 : 1;
                else if (mode === 2) value = (x + 3 * y) % 11 === 0 ? (searching ? 0x40 : 0x80) : 0;
                else if (mode === 3 && x === 50) value = 0x40;
                this.spots[y * 256 + x] = value;
            }
        }
    }
}

test('AI 다리 검색: 실제 PE 모양 208행의 0xbc 전체 버퍼를 대조한다', () => {
    assert.equal(shapeRows.length, 208);
    const scene = new SearchScene();
    // 원본 입력만 제품에 공급하며 끝점 기대를 재계산하지 않는다.
    shapeRows.forEach((row, index) => {
        assert.equal(row.length, 4);
        const raw = new Uint8Array(SHAPE_BYTES).fill(Number(row[2]) & 0xff);
        scene.search().shape(Number(row[0]), Number(row[1]), raw);
        assert.equal(hex(raw), row[3], `다리 모양 행 ${index} 패턴 ${row[0]} 회전 ${row[1]}`);
    });
});

test('AI 다리 검색: 실제 PE 점유 416행의 첫 차단 반환을 대조한다', () => {
    assert.equal(clearRows.length, 416);
    const scene = new SearchScene();
    // 모든 26개 조각/4회전/4지도 입력을 실제로 계산한다.
    clearRows.forEach((row, index) => {
        assert.equal(row.length, 6);
        scene.setSpots(Number(row[4]), false);
        const placement = { point: { x: bitsToFloat(Number(row[2])), y: bitsToFloat(Number(row[3])) }, pattern: Number(row[0]), direction: Number(row[1]) };
        assert.equal(scene.search().clear(placement), Number(row[5]) !== 0, `점유 행 ${index}`);
    });
});

test('AI 다리 검색: 실제 PE 끝점 점수 640행의 점수와 임시 XY 를 대조한다', () => {
    assert.equal(scoreRows.length, 640);
    const scene = new SearchScene();
    // shape 는 검증된 실제 제품 함수로 만들고 독립 점수/출력과 비교한다.
    scoreRows.forEach((row, index) => {
        assert.equal(row.length, 10);
        const shape = new Uint8Array(SHAPE_BYTES);
        scene.search().shape(Number(row[0]), Number(row[1]), shape);
        const placement = { point: { x: bitsToFloat(Number(row[2])), y: bitsToFloat(Number(row[3])) }, pattern: Number(row[0]), direction: Number(row[1]) };
        const scratch = pointFromBytes(new Uint8Array(8).fill(Number(row[7]) & 0xff));
        const score = scene.search().score(placement, Number(row[4]) | 0, Number(row[5]) | 0, shape, Number(row[6]), scratch);
        assert.ok((score >>> 0) === Number(row[8]) && pointHex(scratch) === row[9], `끝점 행 ${index} 점수 ${score}/${row[8]} XY ${pointHex(scratch)}/${row[9]}`);
    });
});

test('AI 다리 검색: 실제 PE 72장면을 4번씩 재개해 본체·난수·임시 XY·완료값을 대조한다', () => {
    assert.equal(searchRows.length, 72);
    let returns = 0;
    // 각 장면은 native 입력의 실제 초기 본체를 그대로 설치한다.
    searchRows.forEach((row, sceneIndex) => {
        assert.equal(row.length, 6);
        const scene = new SearchScene();
        scene.setSpots(Number(row[1]), true);
        scene.random.setState(Number(row[2]));
        const scratch = pointFromBytes(bytesOf(row[3]));
        scene.memory.bytes(scene.token).set(bytesOf(row[4]));
        // 변경된 실제 본체/난수를 다음 호출에 이어 쓰며 결과를 덮어주지 않는다.
        for (const observation of row[5].split(';')) {
            const expected = observation.split(',');
            assert.equal(expected.length, 4);
            const complete = scene.search().search(scene.token, scratch);
            assert.equal(complete, Number(expected[0]) !== 0, `검색 완료 장면 ${sceneIndex} 호출 ${returns}`);
            assert.equal(scene.random.state, Number(expected[1]), `검색 난수 장면 ${sceneIndex} 호출 ${returns}`);
            assert.equal(pointHex(scratch), expected[2], `검색 임시 XY 장면 ${sceneIndex} 호출 ${returns}`);
            assert.equal(hex(scene.memory.bytes(scene.token)), expected[3], `검색 본체 장면 ${sceneIndex} 호출 ${returns}`);
            returns += 1;
        }
    });
    assert.equal(returns, 288);
});

test('AI 다리 검색: 실제 초기화 → 24칸 후보 → 8개 검색 → 대기 → 결과 반환을 원본 Step 과 함께 수행한다', () => {
    const scene = new SearchScene();
    const pool = new SidPool(32768, { server: false });
    // 실제 50번 지면 슬롯을 확보한다.
    for (let number = 5; number <= 50; number += 1) assert.equal(pool.allocate(2), number);
    const raw = pool.allocated(50);
    raw.fill(0);
    raw[10] = 97;
    raw[30] = 7;
    raw[34] = 2;
    const types = Array.from({ length: 188 }, () => ({ flags1: 0 }));
    types[97].flags1 = 0x800;
    // 후보 모듈은 FrameCodeTable 을 읽고 검색 모듈은 number 를 가진 코드 배열을 읽으므로 같은 실제 코드를 두 모양으로 둔다.
    const candidateFrames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    candidateFrames[97] = new FrameCodeTable([{ side: 0x50, variant: 0x41, flags: 0 }]);
    const surfaces = new Uint16Array(65536).fill(50);
    const candidates = new RawAiBridgeCandidates(scene.memory, pool, types, candidateFrames, surfaces, scene.spots, { bridge: () => scene.bridge, fencemark: () => 90, windArcher: () => 140 });
    const finder = new ContainedFinderState();
    const globals = { now: () => 0, mapLimit: () => 256, bridgeType: () => scene.bridge, logKind: () => 0, startMarker: () => 0, endMarker: () => 0, targetMarker: () => 0 };
    const scratch = { x: -10000, y: -10000 };
    const hooks = {
        initialize: (token, source, target) => { assert.ok(new RawAiBridgePath(scene.memory, pool, finder, globals, {}).initialize(token, source, target)); },
        prepare: (token) => candidates.prepare(token),
        search: (token) => scene.search().search(token, scratch)
    };
    const path = new RawAiBridgePath(scene.memory, pool, finder, globals, hooks);
    const body = () => scene.memory.bytes(scene.token);
    put(body(), 0x5f0, 1);
    put(body(), 0x5f8, 7);
    put(body(), 0x5fc, 2);
    put(body(), 0x604, 8);
    put(body(), 0x628, 8);
    // 현재 재고 8개를 설치하며 결과 조각을 미리 고르지 않는다.
    for (let index = 0; index < 8; index += 1) put(body(), 0x608 + index * 4, index);
    const source = { x: 50, y: 63 };
    let complete = false;
    let pending = false;
    // 원본 단계마다 한 번 처리하며 실제 예산 중단을 확인한다.
    for (let call = 0; call < 100 && !complete; call += 1) {
        const stage = get(body(), 0x5f0);
        complete = path.step(scene.token, source, { x: 80, y: 90 });
        if (stage === 3 && get(body(), 0x5f0) === 3) pending = true;
    }
    assert.ok(complete && pending && get(body(), 0x5f0) === 0);
    assert.ok(source.x > 0 && source.y > 0 && (source.x !== 50 || source.y !== 63));
    assert.ok(get(body(), 0x6a8) < 65536 && get(body(), 0x6b4) < 8 && get(body(), 0x6b8) <= 6);
});

test('AI 다리 검색: 현재 TYPE/패턴/모양 범위가 부정확하면 결과를 만들기 전에 거부한다', () => {
    const scene = new SearchScene();
    const search = scene.search();
    // 패턴 26번과 현재 프레임 표 밖 TYPE 은 각 호출 위치에서 거부된다.
    assert.ok(throws(() => search.shape(26, 0, new Uint8Array(SHAPE_BYTES))));
    assert.ok(throws(() => search.shape(0, 8, new Uint8Array(SHAPE_BYTES))));
    assert.ok(throws(() => search.shape(0, 0, new Uint8Array(SHAPE_BYTES - 1))));
    assert.ok(throws(() => search.clear({ point: { x: 1, y: 1 }, pattern: 26, direction: 0 })));
    scene.bridge = 188;
    assert.ok(throws(() => scene.search().clear({ point: { x: 1, y: 1 }, pattern: 0, direction: 0 })));
    scene.bridge = 82;
    // 원본은 y*256+x 평면 주소만 검사하므로 x 가 255 를 넘어도 같은 평면 안이면 다음 행을 읽고, 평면 65536 밖(y 300)만 거부한다.
    assert.equal(scene.search().clear({ point: { x: 300, y: 1 }, pattern: 0, direction: 0 }), true);
    assert.ok(throws(() => scene.search().clear({ point: { x: 1, y: 300 }, pattern: 0, direction: 0 })));
    // 가짜 본체/모양 크기는 주변을 읽지 않고 거부한다.
    assert.ok(throws(() => scene.search().score({ point: { x: 1, y: 1 }, pattern: 0, direction: 0 }, 0, 0, new Uint8Array(SHAPE_BYTES - 1), 0, { x: 0, y: 0 })));
    assert.ok(throws(() => scene.search().search(scene.memory.allocate(16), { x: 0, y: 0 })));
});

test('AI 다리 검색: 연결점 방향 번호가 8 이상이면 표 범위를 거부하고 앞선 예산/본체 쓰기를 유지한다', () => {
    const scene = new SearchScene();
    const body = scene.memory.bytes(scene.token);
    put(body, 0x628, 1);
    put(body, 0x6c8, 0);
    put(body, 0x690, 1);
    put(body, 0x6cc, 0);
    put(body, 0x6c4, 0);
    put(body, 0x648 + 8, 8);
    assert.ok(throws(() => scene.search().search(scene.token, { x: 0, y: 0 })));
    assert.equal(get(body, 0x6cc), 0);
});

test('AI 다리 검색 생성자: 본체 소유자/프레임 표/spot/다리 조회기/난수가 없거나 크기가 틀리면 거부한다', () => {
    const scene = new SearchScene();
    const make = (memory, frames, spots, bridge, random) => () => new RawAiBridgeSearch(memory, frames, spots, bridge, random);
    assert.ok(throws(make(undefined, scene.frames, scene.spots, () => 82, scene.random)));
    assert.ok(throws(make(scene.memory, {}, scene.spots, () => 82, scene.random)));
    assert.ok(throws(make(scene.memory, scene.frames, new Uint8Array(65535), () => 82, scene.random)));
    assert.ok(throws(make(scene.memory, scene.frames, scene.spots, 82, scene.random)));
    assert.ok(throws(make(scene.memory, scene.frames, scene.spots, () => 82, {})));
});
