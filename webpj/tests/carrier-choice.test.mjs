// 운반 후보 선택 raw-carrier-choice.js(RawCarrierChoice)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-choice-x86.tsv 721행(입력 19 DWORD — 연산 0 평가/1 귀환 선택/2 간헐천 선택·원본 목록 구멍/중복/역순 모양·count/capacity·Player·AI·병기·속도·경로 비교·검사 전역·변이 11종·집 신전·유효 마스크·경로 모양·주소 기준·baseline·큰 좌표·mover)
//    → 외부 효과 순서(R 발자국/B 임시 BFS/V·G 유효성/Q Graph/! 진단), 상태 11 DWORD(선택·currentMetric·baseline·comparePaths·두 목록 count·집·발자국/탐색/유효성 호출 수·전체 풀 Adler-32),
//    정렬된 후보 배열 hex(평가 입력일 때). 원문 안의 `relocate` 주소는 독립 관측용 배열/풀 기준 주소를 정렬 비교기 입력(sortAddresses)으로 공급한다.
//  - 계약: 빈 목록도 baseline 을 currentMetric 에 복사하고 선택되지 않은 큰 하위를 요구하지 않으며, 판본/Player 표 크기를 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E102: 운반 후보 선택 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { BoardTargetState, RawBoardTarget } from '../www/core/raw-board-target.js';
import { CarrierChoiceState, RawCarrierChoice } from '../www/core/raw-carrier-choice.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E102)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-choice-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E102)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const bits = (value) => floatToBits(value);

// 원래 source 목록의 구멍/중복/역순 입력이다. (2026-11-08 WB04-E102)
function choiceValue(row, index) {
    if (row[1] === 1 && index % 3 === 0) return 0;
    if (row[1] === 2) return 60 + (index % 2);
    if (row[1] === 3) return 67 - (index % 8);
    return 60 + index;
}

// 경로 선택 결과를 계산하지 않는 BFS 경계의 명시 꼭짓점/방향 입력이다. (2026-11-08 WB04-E102)
function choiceRoute(output, pattern, candidate) {
    let points = [];
    let directions = [];
    if (pattern !== 0 && !(pattern === 2 && candidate % 2 === 0)) {
        if (pattern === 4) { points = [{ x: 0, y: 0 }]; directions = [7]; }
        else if (pattern === 2) { points = [{ x: 0, y: 0 }, { x: 30, y: 40 }, { x: 31, y: 41 }]; directions = [0, 2, 4]; }
        else if (pattern === 3) { points = [{ x: 0, y: 0 }, { x: 3 + candidate % 3, y: 4 + candidate % 5 }]; directions = [0, 2]; }
        else if (pattern === 5) { points = [{ x: 0, y: 0 }, { x: bitsToFloat(0x7fc00000), y: 4 }]; directions = [0, 2]; }
        else { points = [{ x: 0, y: 0 }, { x: 3, y: 4 }]; directions = [0, 2]; }
    }
    output.setPath(points, directions);
}

// 같은 물리 입력과 큰 하위 경계만 공급하며 선택·정렬·거리·Prepare 는 실제 구현인 장면이다. (2026-11-08 WB04-E102)
function makeScene() {
    const scene = {
        pool: new SidPool(24000),
        types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0 })),
        movement: Array.from({ length: 188 }, () => [0, 0]),
        players: new Uint8Array(9 * 0xb4),
        returns: new SquidPostPopList(),
        geysers: new SquidPostPopList(),
        candidates: new SquidPostPopList(),
        state: new CarrierChoiceState(),
        boardState: new BoardTargetState(),
        live: { now: 10.5, checking: false },
        row: [],
        footprints: 0,
        searches: 0,
        validations: 0,
        candidate: 60,
        trace: []
    };
    const raw = (sid) => scene.pool.physical(sid);
    const event = (name, values) => scene.trace.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
    const point = (sid) => ({ x: bitsToFloat(get(raw(sid), 14)), y: bitsToFloat(get(raw(sid), 18)) });
    // PE 에 제공한 초기 입력을 설치하며 예상 선택이나 거리를 계산하지 않는다.
    scene.prepare = (input) => {
        const row = input;
        scene.row = row;
        assert.equal(row.length, 19);
        scene.trace = [];
        scene.footprints = 0;
        scene.searches = 0;
        scene.validations = 0;
        scene.candidate = 60;
        scene.live.now = 10.5;
        scene.live.checking = row[10] !== 0;
        scene.state = new CarrierChoiceState();
        scene.state.comparePaths = row[9] !== 0;
        scene.state.baselineBits = row[16];
        scene.state.currentMetricBits = 0x12345678;
        for (const type of scene.types) { type.flags1 = 0; type.flags2 = 0; }
        for (const pair of scene.movement) { pair[0] = 0; pair[1] = 0; }
        scene.movement[133] = [bitsToFloat(row[7]), bitsToFloat(row[8])];
        scene.movement[137] = [2, 180];
        scene.movement[156] = [4, 45];
        scene.types[133].flags2 = row[6];
        scene.types[121].flags1 = 0x800;
        scene.players.fill(0);
        put(scene.players, row[4] * 0xb4 + 0xb0, row[5]);
        put(scene.players, row[4] * 0xb4 + 0x7c, row[12]);
        // 전체 풀을 0x2a 로 채운다(같은 ArrayBuffer 를 빌려 한 번에 채우는 시험 전용 접근).
        new Uint8Array(raw(0).buffer).fill(0x2a);
        // 슬롯 0/부모 50 과 512개 원본 후보의 바이트 입력을 그대로 넣는다.
        for (let sid = 0; sid < 572; sid += 1) {
            if (sid !== 0 && sid !== 50 && sid < 60) continue;
            const slot = raw(sid);
            slot[10] = sid === 0 || sid === 50 ? 133 : sid === 60 && row[1] === 5 ? 121 : 122;
            slot[11] = row[1] === 2 ? 3 : 0;
            slot[34] = sid === 0 || sid === 50 ? row[4] : 2;
            let x = sid === 0 || sid === 50 ? 20.25 : 25 + ((sid - 60) % 8) * 3;
            let y = sid === 0 || sid === 50 ? 21.5 : 20 + Math.floor((sid - 60) / 8) * 3;
            if (row[17]) { x *= 65536; y *= 65536; }
            put(slot, 14, bits(x));
            put(slot, 18, bits(y));
            put(slot, 8, (sid * 31 + 111) & 65535, 2);
        }
        for (const list of [scene.returns, scene.geysers, scene.candidates]) {
            list.entries = Array.from({ length: row[3] }, (_, index) => choiceValue(row, index));
            list.count = row[2];
        }
    };
    // 실제 새 임시 경로의 시각 배열은 BFS 호출 전 5000개 모두 0 이어야 한다.
    scene.hooks = {
        movement: (sid) => scene.movement[raw(sid)[10]],
        footprint: (sid) => {
            event('R', [sid, 0]);
            scene.footprints += 1;
            if (sid !== 50) scene.candidate = sid;
            const p = point(sid);
            const rectangle = { left: Math.fround(p.x - 0.5), top: Math.fround(p.y - 0.5), right: Math.fround(p.x + 0.5), bottom: Math.fround(p.y + 0.5) };
            if (scene.footprints === 1) {
                const row = scene.row;
                if (row[11] === 1) {
                    put(raw(50), 14, bits(42.25)); put(raw(50), 18, bits(43.5));
                    put(raw(60), 14, bits(50.25)); put(raw(60), 18, bits(51.5));
                }
                if (row[11] === 2) raw(50)[10] = 137;
                if (row[11] === 7) scene.candidates.count = 2;
            }
            return rectangle;
        },
        search: (output, sid, start, destination, flags) => {
            scene.searches += 1;
            event('B', [bits(start.x), bits(start.y), bits(destination.x), bits(destination.y), flags]);
            assert.equal(output.count, 0);
            const times = output.bytes.subarray(0x9c58, 0x9c58 + 5000 * 8);
            assert.ok(times.every((byte) => byte === 0));
            choiceRoute(output, scene.row[14], scene.candidate);
            if (scene.searches === 1 && scene.row[11] === 3) { scene.live.now = 13.75; scene.state.comparePaths = false; scene.state.baselineBits = 0x7fa12345; }
            if (scene.searches === 1 && scene.row[11] === 9) raw(50)[10] = 156;
        },
        validReturn: (parent, target) => {
            event('V', [parent, target]);
            scene.validations += 1;
            if (scene.validations === 1 && scene.row[11] === 5) { scene.returns.count = 2; put(scene.players, scene.row[4] * 0xb4 + 0x7c, 61); }
            return (scene.row[13] & (1 << (target % 3))) !== 0;
        },
        validGeyser: (parent, target) => {
            event('G', [parent, target]);
            scene.validations += 1;
            if (scene.row[11] === 6) scene.geysers.count = 2;
            return (scene.row[13] & (1 << (target % 3))) !== 0;
        },
        graphFor: (sid) => { event('Q', [sid]); return 7; },
        assertion: (line) => {
            event('!', [line]);
            assert.ok(line === 0xace || line === 0xb03);
            if (scene.row[11] === 4) scene.types[121].flags1 = 0;
            if (scene.row[11] === 4 || scene.row[11] === 8) scene.state.baselineBits = 0x7fa12345;
        },
        // 독립 원문의 배열/풀 기준 주소를 정렬 비교기 입력으로 공급한다.
        sortAddresses: () => ({ poolBase: 0x14000000, arrayBase: scene.row[15] })
    };
    // 선택/현재 전역/실제 호출 수와 전체 물리 raw Adler 를 원문 순서로 기록한다.
    scene.status = (selected) => [selected, scene.state.currentMetricBits, scene.state.baselineBits, scene.state.comparePaths ? 1 : 0, scene.returns.count, scene.geysers.count,
        get(scene.players, scene.row[4] * 0xb4 + 0x7c), scene.footprints, scene.searches, scene.validations, adler32(new Uint8Array(raw(0).buffer))].map((value) => String(value >>> 0)).join(',');
    return scene;
}

test('CarrierChoice: 원본 10.78 721입력의 후보 수집·정렬·경로 평가의 효과 순서·선택·currentMetric·전체 풀 Adler·정렬 배열이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 원본 53비트와 대조하고 64비트 결과도 별도 원문으로 보존한다.
    for (const fields of rows) {
        assert.equal(fields.length, 7);
        scene.prepare(fields[0].split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value)))));
        const row = scene.row;
        const board = new RawBoardTarget(scene.boardState);
        const choice = new RawCarrierChoice(scene.pool, scene.types, scene.players, scene.returns, scene.geysers, () => scene.live.now, () => scene.live.checking, board, scene.state, scene.hooks);
        let selected = 0;
        if (row[0] === 0) selected = choice.evaluate(scene.candidates, 50, row[18]);
        else if (row[0] === 1) selected = choice.chooseReturn(50);
        else selected = choice.chooseGeyser(50);
        const trace = scene.trace.length === 0 ? '-' : scene.trace.join(';');
        let array = '-';
        if (row[0] === 0 && row[2]) {
            const words = Uint32Array.from(scene.candidates.entries.slice(0, row[2]));
            array = Buffer.from(words.buffer, words.byteOffset, words.byteLength).toString('hex');
        }
        const label = `행 ${count}: 입력 ${fields[0]}`;
        assert.equal(trace, fields[1], label);
        assert.equal(scene.status(selected), fields[2], label);
        assert.equal(array, fields[3], label);
        count += 1;
    }
    assert.equal(count, 721);
});

test('CarrierChoice: 빈 목록도 baseline 을 currentMetric 에 복사하고 선택되지 않은 큰 하위를 요구하지 않으며 판본/Player 표 크기를 거부한다', () => {
    const scene = makeScene();
    scene.state.baselineBits = 0x7fa12345;
    const board = new RawBoardTarget(scene.boardState);
    const choice = new RawCarrierChoice(scene.pool, scene.types, scene.players, new SquidPostPopList(), new SquidPostPopList(), () => scene.live.now, () => false, board, scene.state, {});
    assert.equal(choice.evaluate(new SquidPostPopList(), 50, 0), 0);
    assert.equal(scene.state.currentMetricBits, 0x7fa12345);
    assert.equal(choice.pool, scene.pool);
    assert.ok(throws(() => new RawCarrierChoice(scene.pool, scene.types, new Uint8Array(0), new SquidPostPopList(), new SquidPostPopList(), () => 0, () => false, board, scene.state, {})));
    assert.ok(throws(() => new RawCarrierChoice({ edition: 'cd1072' }, scene.types, scene.players, new SquidPostPopList(), new SquidPostPopList(), () => 0, () => false, board, scene.state, {})));
});
