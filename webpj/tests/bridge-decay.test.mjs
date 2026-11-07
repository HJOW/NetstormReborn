// 다리 붕괴 순수 논리 bridge-decay.js(열린 방향·방문 목록·수명 감소·프레임 전환·destroy 재정의·한 칸 처리·스캔 커서/주기)를 세 원본 PE의 x86 관측 bridgedecay-x86.tsv 약 1만 4천 행과 대조한다.
// 행 종류: Cell 4241·Life 3264·Destroy 1152·Crack 348·Normal 174·Weaken 696·Restore 696·Init 12·ScanScene/Scan 3941. 이 논리는 판본에 무관해 세 PE의 행을 모두 재생한다
// (스캔 초기화만 판본 이름으로 10.78/CD 범위를 고른다). 기대 결과는 다시 계산하지 않고 입력만 구성한다. (2026-11-08 WB04-E44: Bridge 붕괴 논리 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import {
    BRIDGE_DECAY_OUTCOME, BRIDGE_LIFE_MASK, DEFAULT_BRIDGE_DECAY_MODE, bridgeApplyLife, bridgeCrackedFrame, bridgeDecayCell, bridgeDecayScanAdvance, bridgeDecayScanEligible, bridgeDecayScanReset,
    bridgeDestroyProceeds, bridgeNormalFrame, bridgeRestore, bridgeWeaken,
} from '../www/core/bridge-decay.js';
import { SurfaceFinder } from '../www/core/surface-finder.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, readRows } from './raw-support.mjs';

// 기대값 파일의 행 종류별 개수(cpppj 검사와 같다)와 수명이 7을 넘어 새 코드가 거부하는 Life 입력 수이다. 파일이 잘리면 검사가 실패한다. (2026-11-08 WB04-E44)
const COUNTS = { cell: 4241, life: 3264, destroy: 1152, crack: 348, normal: 174, weaken: 696, init: 12, scan: 3941 };
const LIFE_GUARDED = 400;
const SURFACE = 0x800;
const BRIDGE = 4;

// fixture 파일 경로와 전체 행이다. (2026-11-08 WB04-E44)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bridgedecay-x86.tsv'));

// 기대값의 Frames 행(표 번호별 실제/변형 bridge 프레임 코드)을 코드 배열로 읽는다. (2026-11-08 WB04-E44)
function framesOf(table) {
    const row = rows.find((item) => item[0] === 'Frames' && item[1] === table);
    assert.ok(row, `Frames ${table}`);
    const codes = [];
    // 네 바이트 코드마다 원본 순서를 유지한다.
    for (let index = 0; index + 8 <= row[2].length; index += 8) {
        const byte = (offset) => parseInt(row[2].slice(index + offset * 2, index + offset * 2 + 2), 16);
        codes.push({ side: byte(0), variant: byte(1), number: byte(2), flags: byte(3) });
    }
    return codes;
}

// "a,b,c;d,e,f" 형식의 정수 묶음을 읽는다. '-'는 빈 목록이다. (2026-11-08 WB04-E44)
const tuples = (text) => (text === '-' || text === '' ? [] : text.split(';').map((entry) => entry.split(',').map(Number)));

// 입력 객체 한 개: 번호, 기준점, 발자국, 타입 플래그, 종류(0 다리/1 섬), 프레임, state, extra, 그래프 번호, 수명 단어이다. (2026-11-08 WB04-E44)
const inputsOf = (text) => tuples(text).map((v) => ({ id: v[0], x: v[1], y: v[2], width: v[3], height: v[4], flags1: v[5] >>> 0, flags2: v[6] >>> 0, kind: v[7], frame: v[8], state: v[9], extra: v[10], graph: v[11], word: v[12] }));

// 그래프 번호 → 표면 수 표이다. (2026-11-08 WB04-E44)
const recordsOf = (text) => new Map(tuples(text).map((v) => [v[0], (v[1] << 16) >> 16]));

// 다리 객체의 가변 상태를 만든다. 섬은 방문 목록에 들어가지 않으므로 상태가 없다. (2026-11-08 WB04-E44)
function statesOf(inputs, records) {
    return inputs.filter((input) => input.kind === 0).map((input) => ({
        id: input.id, word: input.word, frame: input.frame, graphSurfaces: records.get(input.graph) ?? 0, graphValid: records.has(input.graph), extra: input.extra, dead: (input.state & 2) !== 0,
    }));
}

// 원본 사건 표기로 바꾼다: D:번호:플래그, R:번호, C:칸x:칸y, S:x비트:y비트. (2026-11-08 WB04-E44)
function eventText(events, inputs) {
    const parts = events.map((event) => {
        const input = inputs.find((item) => item.id === event.id);
        if (event.kind === 'Destroy') return `D:${event.id}:${event.flags}`;
        if (event.kind === 'Redraw') return `R:${event.id}`;
        if (event.kind === 'Carriers') return `C:${input.x}:${input.y}`;
        return `S:${floatToBits(input.x)}:${floatToBits(input.y)}`;
    });
    return parts.length ? parts.join(' ') : '-';
}

// 결과 상태를 기대값의 after 열 표기(번호,단어,프레임,state)로 바꾼다. 섬은 입력 값 그대로이다. (2026-11-08 WB04-E44)
function afterText(inputs, states) {
    return inputs.map((input) => {
        let { word, frame, state } = input;
        const found = states.find((entry) => entry.id === input.id);
        if (found) {
            word = found.word;
            frame = found.frame;
            if (found.dead) state |= 2;
        }
        return `${input.id},${word},${frame},${state}`;
    }).join(';');
}

// 표면 탐색기를 만든다: 객체 목록, 번호 지도, spot 지도. 다리는 실제 프레임 코드, 섬은 사방 연결 코드 하나를 쓴다. 결과는 계산하지 않는다. (2026-11-08 WB04-E44)
function sceneOf(inputs, codes, spotText) {
    const map = new Uint16Array(256 * 256);
    const spots = new Uint8Array(256 * 256);
    const objects = inputs.map((input) => ({
        id: input.id, x: input.x, y: input.y, width: input.width, height: input.height, flags1: input.flags1, flags2: input.flags2,
        frame: input.kind === 0 ? codes[input.frame] : { side: 0x41, variant: 0x41, number: 1, flags: 0 }, dead: (input.state & 2) !== 0, buried: (input.extra & 8) !== 0,
    }));
    // 오른쪽 아래 기준점의 발자국 전체에 번호를 적는다(원본에 넣은 입력 규약과 같다).
    for (const input of inputs) {
        for (let y = input.y - input.height + 1; y <= input.y; y += 1) for (let x = input.x - input.width + 1; x <= input.x; x += 1) map[y * 256 + x] = input.id;
    }
    // 지정한 spot 비트만 넣는다.
    for (const [x, y, value] of tuples(spotText)) spots[y * 256 + x] = value;
    return new SurfaceFinder(objects, map, spots);
}

// 기대값의 정수 열에 저장한 배정도 비트를 복원하고, 배정도를 비트로 되돌린다. (2026-11-08 WB04-E44)
const view = new DataView(new ArrayBuffer(8));
function bits(text) {
    view.setBigUint64(0, BigInt(text));
    return view.getFloat64(0);
}
function doubleBits(value) {
    view.setFloat64(0, value);
    return view.getBigUint64(0);
}

const throwsOf = (operation) => {
    try { operation(); } catch { return true; }
    return false;
};

test('bridge-decay: 한 칸 처리(Cell 4241행)의 사건 순서와 결과 상태가 세 PE 관측과 일치한다', () => {
    const codes = framesOf('0');
    const outcomes = new Map();
    let count = 0;
    // 고정 장면과 무작위 장면의 모든 시작 칸이다.
    for (const row of rows.filter((item) => item[0] === 'Cell')) {
        const inputs = inputsOf(row[1]);
        const finder = sceneOf(inputs, codes, row[3]);
        const states = statesOf(inputs, recordsOf(row[2]));
        const mode = { server: row[4] === '1', authority: true, editor: false, debugRedraw: row[5] === '1', debugKeep: row[6] === '1' };
        const result = bridgeDecayCell(finder, codes, states, Number(row[7]), mode);
        assert.ok(result.outcome !== BRIDGE_DECAY_OUTCOME.incomplete && result.outcome !== BRIDGE_DECAY_OUTCOME.faulted);
        assert.equal(eventText(result.events, inputs), row[8], `Cell ${count} 사건`);
        assert.equal(afterText(inputs, result.states), row[9], `Cell ${count} 상태`);
        outcomes.set(result.outcome, (outcomes.get(result.outcome) ?? 0) + 1);
        count += 1;
    }
    assert.equal(count, COUNTS.cell);
    // 기대값이 주요 분기를 모두 지났는지 확인한다.
    for (const outcome of ['smallGraph', 'hard', 'isolated', 'closed', 'blocked', 'decayed']) assert.ok(outcomes.get(BRIDGE_DECAY_OUTCOME[outcome]) > 0, outcome);
});

test('bridge-decay: 수명 감소 전체(Life 3264행: 제거·비트 갱신·금 간 프레임·디버그 갱신·이동체 확인)', () => {
    const codes = framesOf('0');
    let count = 0;
    let guarded = 0;
    // 방향 글자·프레임 종류·이전 수명·감소량·서버 여부의 조합이다.
    for (const row of rows.filter((item) => item[0] === 'Life')) {
        const inputs = inputsOf(row[1]);
        const states = statesOf(inputs, recordsOf(row[2]));
        const mode = { server: row[3] === '1', authority: true, editor: false, debugRedraw: row[4] === '1', debugKeep: row[5] === '1' };
        const reduction = Number(row[6]);
        const events = [];
        count += 1;
        // 원본은 수명을 4비트(0~15)로 그대로 쓴다. 새 코드는 정상 범위 0~7을 넘는 결과를 쓰기 전에 거부한다.
        if (((inputs[0].word & BRIDGE_LIFE_MASK) >> 3) - reduction > 7) {
            const before = { ...states[0] };
            assert.ok(throwsOf(() => bridgeApplyLife(codes, states[0], reduction, mode, events)));
            assert.deepEqual(states[0], before);
            assert.equal(events.length, 0);
            guarded += 1;
            continue;
        }
        assert.ok(bridgeApplyLife(codes, states[0], reduction, mode, events));
        assert.equal(eventText(events, inputs), row[7], `Life ${count} 사건`);
        assert.equal(afterText(inputs, states), row[8], `Life ${count} 상태`);
    }
    assert.equal(count, COUNTS.life);
    assert.equal(guarded, LIFE_GUARDED);
});

test('bridge-decay: 다리 destroy 재정의(Destroy 1152행)가 기본 destroy 로 넘어가는 조건을 대조한다', () => {
    const codes = framesOf('0');
    let count = 0;
    let blocked = 0;
    // 편집기·권한·extra·표면 수·프레임·디버그 유지·인자의 조합이다.
    for (const row of rows.filter((item) => item[0] === 'Destroy')) {
        const inputs = inputsOf(row[1]);
        const states = statesOf(inputs, recordsOf(row[2]));
        const mode = { server: true, authority: row[4] === '1', editor: row[3] === '1', debugRedraw: false, debugKeep: row[5] === '1' };
        const proceeds = bridgeDestroyProceeds(mode, states[0].extra, states[0].graphSurfaces, codes[states[0].frame].flags);
        assert.equal(proceeds, row[7] !== '-');
        if (proceeds) assert.equal(row[7], `D:${inputs[0].id}:${row[6]}`);
        if (!proceeds) blocked += 1;
        count += 1;
    }
    assert.equal(count, COUNTS.destroy);
    assert.ok(blocked > 0);
});

test('bridge-decay: 금 간/보통/약화/복구 프레임 전환(1,914행)을 실제 표와 프레임이 빠진 변형 표에서 대조한다', () => {
    const counts = new Map();
    // 표 0은 실제 bridge.type, 1·2는 일부 프레임을 뺀 표이다.
    for (const row of rows) {
        if (!['Crack', 'Normal', 'Weaken', 'Restore'].includes(row[0])) continue;
        const codes = framesOf(row[1]);
        const frame = Number(row[2]);
        const redraw = 'R:6100';
        if (row[0] === 'Crack') {
            const found = bridgeCrackedFrame(codes, frame, row[3] === '1');
            assert.equal(found !== -1, row[4] === '1');
            assert.equal(found === -1 ? frame : found, Number(row[5]));
            assert.equal(row[6], found === -1 ? '-' : redraw);
        } else if (row[0] === 'Normal') {
            const found = bridgeNormalFrame(codes, frame);
            assert.equal(found === -1 ? frame : found, Number(row[5]));
            assert.equal(row[6], found === -1 ? '-' : redraw);
        } else {
            const word = Number(row[3]);
            const change = row[0] === 'Weaken' ? bridgeWeaken(codes, word, frame) : bridgeRestore(codes, word, frame);
            if (row[0] === 'Weaken') assert.equal(change.changed, row[4] === '1');
            assert.equal(change.word, Number(row[5]));
            assert.equal(change.frame, Number(row[6]));
            assert.equal(row[7], change.changed ? redraw : '-');
        }
        counts.set(row[0], (counts.get(row[0]) ?? 0) + 1);
    }
    assert.equal(counts.get('Crack'), COUNTS.crack);
    assert.equal(counts.get('Normal'), COUNTS.normal);
    assert.equal(counts.get('Weaken'), COUNTS.weaken);
    assert.equal(counts.get('Restore'), COUNTS.weaken);
});

test('bridge-decay: 스캔 초기화(Init 12행)와 프레임별 커서 전진·주기 되돌림·처리 대상 번호(Scan 3941행)를 두 판본 관측과 대조한다', () => {
    let inits = 0;
    let scans = 0;
    let visitedTotal = 0;
    let state = null;
    let objects = [];
    // 시퀀스는 Init → ScanScene → Scan 행들의 순서로 저장돼 있다.
    for (const row of rows) {
        if (row[0] === 'Init') {
            state = bridgeDecayScanReset(row[1] === 'originals', bits(row[3]));
            assert.equal(state.first, Number(row[4]));
            assert.equal(state.last, Number(row[5]));
            assert.equal(state.cursor, Number(row[6]));
            assert.equal(doubleBits(state.next), BigInt(row[7]));
            inits += 1;
        } else if (row[0] === 'ScanScene') objects = inputsOf(row[3]);
        else if (row[0] === 'Scan') {
            const mode = { server: true, authority: row[7] === '1', editor: row[6] === '1', debugRedraw: false, debugKeep: false };
            // 각 행은 실행 전 커서·다음 시각을 가진 독립 입력이다.
            state.cursor = Number(row[8]);
            state.next = bits(row[9]);
            const step = bridgeDecayScanAdvance(state, bits(row[3]), bits(row[4]), Number(row[5]) > 0, mode);
            const visited = [];
            // 구간 안의 번호를 오름차순으로 보며 처리 대상만 모은다.
            for (let sid = step.begin; sid < step.end; sid += 1) {
                const found = objects.find((object) => object.id === sid);
                if (!found || !bridgeDecayScanEligible(found.state, found.flags2, found.extra)) continue;
                visited.push(sid);
                visitedTotal += 1;
            }
            assert.equal(visited.length ? visited.join(',') : '-', row[10], `Scan ${scans} 대상`);
            assert.equal(state.cursor, Number(row[11]), `Scan ${scans} 커서`);
            assert.equal(doubleBits(state.next), BigInt(row[12]), `Scan ${scans} 다음 시각`);
            scans += 1;
        }
    }
    assert.equal(inits, COUNTS.init);
    assert.equal(scans, COUNTS.scan);
    assert.ok(visitedTotal > 0);
});

test('bridge-decay: 원본 기본 프레임 간격에서 스캔은 프레임마다 11개 번호를 전진하고 주기 끝 프레임에 나머지를 모두 훑는다', () => {
    const state = bridgeDecayScanReset(true, 0);
    assert.deepEqual([state.first, state.last, state.next], [15000, 23001, 10]);
    let now = 0;
    let covered = 0;
    // 14ms 프레임으로 한 주기를 지난다.
    while (now < 10) {
        now += 0.014;
        const step = bridgeDecayScanAdvance(state, now, 0.014, false);
        if (now < 10) assert.equal(step.end - step.begin, 11);
        covered += step.end - step.begin;
    }
    assert.equal(covered, 8002);
    assert.equal(state.cursor, 15000);
    assert.equal(state.next, 10 + now);
    // 정지·편집기·권한 없음은 커서와 다음 시각을 건드리지 않는다.
    const before = { ...state };
    bridgeDecayScanAdvance(state, now + 50, 0.014, true);
    bridgeDecayScanAdvance(state, now + 50, 0.014, false, { editor: true });
    bridgeDecayScanAdvance(state, now + 50, 0.014, false, { authority: false });
    assert.deepEqual(state, before);
    // CD 판은 범위가 6000..14000 이다.
    const cd = bridgeDecayScanReset(false, 2.5);
    assert.deepEqual([cd.first, cd.last, cd.cursor, cd.next], [6000, 14000, 6000, 12.5]);
    assert.ok(bridgeDecayScanEligible(0, BRIDGE, 8) && !bridgeDecayScanEligible(0, BRIDGE, 1));
    assert.ok(!bridgeDecayScanEligible(4, BRIDGE, 0) && !bridgeDecayScanEligible(0, 2, 0));
});

test('bridge-decay: 원본이 정상 반환하지 않는 입력(그래프 254·접합 고리)은 상태를 바꾸지 않고 구분해 알린다', () => {
    const codes = framesOf('0');
    // 방향 글자의 번호 1 프레임을 찾는다(입력 구성용).
    const frameOf = (side) => {
        const found = codes.findIndex((code) => code.side === side.charCodeAt(0) && code.number === 1);
        assert.ok(found >= 0, side);
        return found;
    };
    // 2×2 접합 고리: F(동·남) G(남·서) / I(북·동) H(북·서)와 고리에 붙은 열린 끝 판자(204)를 시작 칸으로 삼는다.
    const ring = [['F', 100, 100], ['G', 101, 100], ['I', 100, 101], ['H', 101, 101]];
    const inputs = ring.map(([side, x, y], index) => ({ id: 200 + index, x, y, width: 1, height: 1, flags1: SURFACE, flags2: BRIDGE, kind: 0, frame: frameOf(side), state: 0, extra: 0, graph: 1, word: 3 << 3 }));
    inputs.push({ id: 204, x: 99, y: 100, width: 1, height: 1, flags1: SURFACE, flags2: BRIDGE, kind: 0, frame: frameOf('K'), state: 0, extra: 0, graph: 1, word: 3 << 3 });
    inputs[0].frame = frameOf('C'); // 동·남·서로 바꿔 판자와 잇는다.
    const finder = sceneOf(inputs, codes, '-');
    const states = statesOf(inputs, new Map([[1, 9]]));
    const ringResult = bridgeDecayCell(finder, codes, states, 204);
    assert.equal(ringResult.outcome, BRIDGE_DECAY_OUTCOME.incomplete);
    assert.equal(ringResult.events.length, 0);
    assert.equal(afterText(inputs, ringResult.states), afterText(inputs, states));
    // 시작 칸의 그래프가 254 면 아무것도 하지 않는다.
    states[states.length - 1].graphValid = false;
    const invalid = bridgeDecayCell(finder, codes, states, 204);
    assert.equal(invalid.outcome, BRIDGE_DECAY_OUTCOME.invalidGraph);
    assert.equal(invalid.events.length, 0);
    // 제거할 칸의 그래프가 254 면 그 지점에서 멈추고 dead 로 표시하지 않는다.
    const events = [];
    const lost = { id: 300, word: 1 << 3, frame: frameOf('K'), graphSurfaces: 0, graphValid: false, extra: 0, dead: false };
    assert.equal(bridgeApplyLife(codes, lost, 1, DEFAULT_BRIDGE_DECAY_MODE, events), false);
    assert.ok(!lost.dead && events.length === 0);
    // 서버가 아니면 수명 0 에서도 destroy 재정의에 닿지 않는다.
    assert.equal(bridgeApplyLife(codes, lost, 1, { ...DEFAULT_BRIDGE_DECAY_MODE, server: false }, events), true);
    assert.equal(lost.word, 0);
    // 입력 누락·다리가 아닌 시작 칸·범위 밖 프레임은 예외이다.
    assert.throws(() => bridgeDecayCell(finder, codes, [], 204), RangeError);
    assert.throws(() => bridgeCrackedFrame(codes, codes.length, false), RangeError);
    assert.throws(() => bridgeNormalFrame(codes, -1), RangeError);
});
