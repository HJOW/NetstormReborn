// Tab 최근 건설 위치·U 최근 손실 위치·P/R 사제 선택·N 다음 이동 유닛 선택 키를 원본 10.78 관측으로 대조한다(WB04-E22,
// cpppj ConstructionHistoryKeyTests·LostPositionKeyTests·PriestSelectionShortcutTests·NextUnitSelectionTests 이식).
//  - construction-history-key-x86.tsv 511행(초기화 3 + Tab 508): 전체 40 BYTE 이력·순번·호출. lost-position-key-x86.tsv 1022행: 모달/정지/두 축 경계/수정키/다른 사건과 부분 중단.
//  - priest-selection-shortcut-x86.tsv 220행: 조회 전 캡처·장기 카운터·일반 큐. next-unit-selection-x86.tsv 410행: 첫 선택/가운데 이동/정지/후속 선택과 순환·free·dead·전체 owner 비교.
// cpppj 의 실제 Client 전역(ClientUserSelectionGlobals 등)·실제 Select/ToggleMultiple 연결 검사는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E22: 선택 관련 키 입력 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { InputCode } from '../www/core/input-queue.js';
import { SidPool } from '../www/core/sid-pool.js';
import { createConstructionHistory, initializeConstructionHistory, dispatchConstructionHistoryKey, dispatchLostPositionKey } from '../www/core/construction-history-key.js';
import { selectPriestByKey, selectNextMovingUnit } from '../www/core/selection-shortcuts.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E22)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const historyRows = fixture('construction-history-key-x86.tsv');
const lostRows = fixture('lost-position-key-x86.tsv');
const priestRows = fixture('priest-selection-shortcut-x86.tsv');
const nextRows = fixture('next-unit-selection-x86.tsv');
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = (value) => floatToBits(value);

// 원문 40 BYTE hex 로 다섯 좌표를 원시 비트 그대로 준비하고 되돌린다. (2026-11-08 WB04-E22)
function readHistoryPoints(history, text) {
    assert.equal(text.length, 80);
    const bytes = Uint8Array.from(Buffer.from(text, 'hex'));
    for (let index = 0; index < 5; index += 1) history.points[index] = { x: bitsToFloat(get(bytes, index * 8)), y: bitsToFloat(get(bytes, index * 8 + 4)) };
}
function historyPoints(history) {
    const bytes = new Uint8Array(40);
    for (let index = 0; index < 5; index += 1) { put(bytes, index * 8, bits(history.points[index].x)); put(bytes, index * 8 + 4, bits(history.points[index].y)); }
    return hex(bytes);
}

// 같은 형식의 전체 입력을 방문하며 제품 함수 안에 기대 결과를 복제하지 않는다.
test('건설 위치 Tab 키: 원본10.78 511행(초기화 3 + Tab 508, 전체 40 BYTE 이력·순번·호출)', { skip: !historyRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of historyRows) {
        assert.equal(row.length, 16);
        const history = createConstructionHistory();
        readHistoryPoints(history, row[2]);
        history.index = number(row[3]);
        let trace = '';
        let completed = true;
        const cells = number(row[4]) | 0;
        const hooks = {
            paused: () => { trace = `P:${row[5]}`; return number(row[5]) !== 0; },
            center: (x, y) => {
                trace += `;C:${bits(x)}:${bits(y)}`;
                if (number(row[6])) { history.index = number(row[7]); history.points[0] = { x: -10000, y: 250.25 }; }
                if (number(row[8])) throw new Error('원문 Center 명시 중단 입력');
            },
        };
        try {
            if (row[0] === 'Init') initializeConstructionHistory(history);
            else assert.ok(dispatchConstructionHistoryKey({ code: number(row[1]), x: 0, y: 0 }, history, () => cells, hooks));
        } catch (error) { if (error.message === '원문 Center 명시 중단 입력') completed = false; else throw error; }
        const label = `행 ${count} ${row.slice(0, 5).join(',')}`;
        assert.equal(historyPoints(history), row[9], `${label} 이력`);
        assert.equal(history.index, number(row[10]), `${label} 순번`);
        assert.equal(trace || '-', row[11], `${label} 호출`);
        assert.equal(completed, number(row[12]) !== 0, `${label} 완료`);
        count += 1;
    }
    assert.equal(count, 511);
});

// 다른 사건/뗌/정지는 장부나 카메라를 요구하지 않고 물리 범위는 임의 modulo 로 보정하지 않는다.
test('건설 위치 Tab 키: 지연 관문과 첫 칸 복귀의 부분 효과', () => {
    const history = createConstructionHistory();
    history.index = 0xffffffff;
    const cells = () => 256;
    for (const code of [(InputCode.release | 9) >>> 0, (InputCode.command | 9) >>> 0, 0x90000, 0x43]) assert.equal(dispatchConstructionHistoryKey({ code, x: 0, y: 0 }, history, cells, {}), false);
    assert.equal(dispatchConstructionHistoryKey({ code: 9, x: 0, y: 0 }, history, cells, { paused: () => true }), true);
    assert.equal(history.index, 0xffffffff);
    assert.throws(() => dispatchConstructionHistoryKey({ code: 9, x: 0, y: 0 }, history, cells, { paused: () => false }), RangeError);
    initializeConstructionHistory(history);
    history.index = 3;
    history.points[0] = { x: 20, y: 30 };
    // 카메라 미연결 이전의 첫 칸 복귀 쓰기는 보존한다.
    assert.throws(() => dispatchConstructionHistoryKey({ code: 9, x: 0, y: 0 }, history, cells, { paused: () => false }), /즉시 카메라 미연결/);
    assert.equal(history.index, 0);
});

// 원문 부모의 모든 행을 읽고 U/u 는 호출 순서·raw 좌표·부분 중단까지 같은 제품 함수로 확인한다.
test('손실 위치 U 키: 원본10.78 1022행(모달·정지·두 축 경계·수정키·다른 사건·부분 중단)', { skip: !lostRows && '원본 fixture 없음' }, () => {
    let count = 0;
    let modalRows = 0;
    let otherRows = 0;
    let outsideRows = 0;
    for (const row of lostRows) {
        assert.equal(row.length, 18);
        const event = number(row[0]);
        const key = (event & 0x03ffffff) >>> 0;
        // 신호 NaN 비트가 JS 수 변환에서 바뀌지 않도록 원시 비트를 보관하고 모듈 호출 때만 수로 바꾼다.
        let xBits = number(row[1]);
        let yBits = number(row[2]);
        const cells = number(row[3]) | 0;
        let trace = '';
        let completed = true;
        const hooks = {
            paused: () => { trace = `P:${row[4]}`; if (number(row[6])) { xBits = number(row[7]); yBits = number(row[8]); } return number(row[4]) !== 0; },
            center: (first, second) => { trace += `;C:${bits(first)}:${bits(second)}`; if (number(row[9])) throw new Error('원문 Center 명시 중단 입력'); },
        };
        const release = (event & InputCode.release) !== 0;
        if (!release && number(row[5])) {
            // 모달 거부는 Uber 의 외부 선행 관문이며 U helper 의 기능으로 합산하지 않는다.
            assert.ok(row[17] === 'modal' && row[12] === '-' && number(row[14]) === 0);
            modalRows += 1;
        } else if (!release && key !== 0x55 && key !== 0x75) {
            // 전체 키 부모는 다른 키도 정지/strchr 를 거치지만 U 전용 helper 는 실제 장치를 조회하지 않는다.
            assert.equal(dispatchLostPositionKey({ code: event, x: 0, y: 0 }, () => bitsToFloat(xBits), () => bitsToFloat(yBits), () => cells, {}), false);
            assert.ok(row[17] === 'key' && number(row[14]) === 0 && !row[12].includes('C:'));
            otherRows += 1;
        } else {
            let accepted = false;
            try { accepted = dispatchLostPositionKey({ code: event, x: 0, y: 0 }, () => bitsToFloat(xBits), () => bitsToFloat(yBits), () => cells, hooks); } catch (error) { if (error.message === '원문 Center 명시 중단 입력') completed = false; else throw error; }
            if (completed) assert.equal(accepted, !release, `행 ${count} 소비`);
            assert.equal(trace || '-', row[12], `행 ${count} 호출`);
        }
        const label = `행 ${count} ${row.slice(0, 5).join(',')}`;
        assert.deepEqual([xBits, yBits], [number(row[10]), number(row[11])], `${label} 좌표`);
        assert.equal(completed, number(row[13]) !== 0, `${label} 완료`);
        if (event & InputCode.outside) outsideRows += 1;
        count += 1;
    }
    assert.ok(count === 1022 && count === lostRows.length && modalRows && otherRows && outsideRows);
});

// 정지 callback 이 바꾼 같은 현재 좌표/세계 크기를 읽으며 무효점에서는 Center 장치를 요구하지 않는다.
test('손실 위치 U 키: 현재 참조와 지연 경계', () => {
    let x = -10000;
    let y = -10000;
    let cells = 1;
    for (const event of [(InputCode.release | 0x55) >>> 0, (InputCode.command | 0x55) >>> 0, 0x550000, 0x56]) assert.equal(dispatchLostPositionKey({ code: event, x: 0, y: 0 }, () => x, () => y, () => cells, {}), false);
    assert.throws(() => dispatchLostPositionKey({ code: 0x55, x: 0, y: 0 }, () => x, () => y, () => cells, {}), /정지 판정 미연결/);
    let calls = 0;
    const hooks = { paused: () => { x = 111.25; y = 121.5; cells = 256; return false; }, center: (first, second) => { assert.deepEqual([first, second], [111.25, 121.5]); calls += 1; } };
    // outside 는 키 마스크로 제거되며 수정키는 새 거부 조건이 아니다.
    const modifiers = (InputCode.outside | InputCode.shift | InputCode.control | InputCode.alt | 0x75) >>> 0;
    assert.equal(dispatchLostPositionKey({ code: modifiers, x: 0, y: 0 }, () => x, () => y, () => cells, hooks), true);
    assert.equal(calls, 1);
});

// 조회 전 기본 선택 캡처·후속 현재 카운터/owner 읽기를 포함해 원문 220개 입력을 대조한다.
test('사제 선택 P/R 키: 원본10.78 220행(조회 전 캡처·장기 카운터·일반 큐)', { skip: !priestRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const fields of priestRows) {
        assert.equal(fields.length, 3);
        const row = fields[0].split(',').map(number);
        assert.equal(row.length, 7);
        const scene = { selected: row[0], repeated: row[1], owner: row[2] };
        const trace = [];
        const event = (name, args) => trace.push(name + args.map((value) => `:${value >>> 0}`).join(''));
        const hooks = {
            localOwner: () => scene.owner,
            viewCommand: () => row[6],
            find: (player) => { event('F', [player]); if (row[4] === 1) scene.selected = 51; if (row[4] === 2) scene.owner = 8; if (row[4] === 3) scene.repeated = 0xffffffff; if (row[4] === 4) scene.repeated = 0; return row[3]; },
            select: (sid, force, activate) => { event('S', [sid, force, activate]); if (row[5] === 1) scene.selected = sid; if (row[5] === 2) scene.repeated = 0xffffffff; if (row[5] === 3) scene.repeated = 77; },
            queue: (code, first, second) => event('Q', [code, first, second]),
        };
        selectPriestByKey(scene, hooks);
        const expected = fields[2].split(',').map(number);
        assert.equal(trace.join(';'), fields[1], `행 ${count} 호출 ${fields[0]}`);
        assert.deepEqual([scene.selected >>> 0, scene.repeated >>> 0, scene.owner >>> 0], expected, `행 ${count} 현재 값`);
        count += 1;
    }
    assert.equal(count, priestRows.length);
    assert.equal(count, 220);
});

// 선택 거부/누락이 있어도 이미 기록한 변경 분기의 count 0 과 콜백의 현재 DWORD 감김은 보존한다.
test('사제 선택 P/R 키: 부분 실패와 현재 DWORD 감김', () => {
    const state = { selected: 50, repeated: 7 };
    const trace = [];
    const hooks = { localOwner: () => 1, viewCommand: () => 0x020000b8, find: (player) => { trace.push(`F:${player}`); return 51; }, queue: () => {} };
    assert.throws(() => selectPriestByKey(state, hooks), /장치 미연결/);
    assert.deepEqual([state.selected, state.repeated, trace.join(';')], [50, 0, 'F:1']);
    // select 가 카운터를 감김 값으로 바꿔도 마지막 증가는 현재 값에서 한다.
    const wrap = { selected: 50, repeated: 0 };
    selectPriestByKey(wrap, { localOwner: () => 1, viewCommand: () => 0, find: () => 50, select: () => { wrap.repeated = 0xffffffff; }, queue: () => {} });
    assert.equal(wrap.repeated, 0);
});

// 첫 선택/가운데/정지/후속 선택의 순서와 순환/free/dead/전체 owner 비교를 실제 원문 기대값에 대조한다.
test('다음 유닛 N 키: 원본10.78 410행(첫 선택·가운데·정지·후속 선택·순환·free·dead·owner)', { skip: !nextRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    let count = 0;
    for (const fields of nextRows) {
        assert.equal(fields.length, 3);
        const row = fields[0].split(',').map(number);
        assert.equal(row.length, 30);
        const scene = { selected: row[0], local: row[2] };
        const trace = [];
        const event = (name, args) => trace.push(name + args.map((value) => `:${value >>> 0}`).join(''));
        // 기대값을 계산하지 않고 독립 초기 상태만 설치한다.
        for (let sid = 0; sid < 8; sid += 1) {
            const raw = pool.physical(sid);
            raw.fill(0);
            raw[10] = 100 + sid;
            raw[11] = row[6 + sid * 3] & 0xff;
            raw[34] = row[7 + sid * 3] & 0xff;
            types[100 + sid].flags2 = row[8 + sid * 3];
        }
        const hooks = {
            localOwner: () => scene.local,
            paused: () => { event('P', [row[3]]); return row[3]; },
            select: (sid, force, activate) => { event('S', [sid, force, activate]); if (row[4]) scene.selected = [sid, 0, 5, 4][row[4] - 1]; },
            center: (sid) => {
                event('C', [sid]);
                if (row[5] === 1) scene.selected = 0;
                if (row[5] === 2) pool.physical(sid)[11] = 1;
                if (row[5] === 3) types[100 + sid].flags2 = 0;
                if (row[5] === 4) scene.selected = 6;
                if (row[5] === 5) scene.local = 2;
            },
            multiple: (sid, on) => event('T', [sid, on]),
        };
        selectNextMovingUnit(pool, types, scene, row[1], hooks);
        const current = [scene.selected, scene.local];
        for (let sid = 0; sid < 8; sid += 1) { current.push(pool.slot(sid)[11]); current.push(types[100 + sid].flags2); }
        const expected = fields[2].split(',').map(number);
        assert.equal(trace.join(';') || '-', fields[1], `행 ${count} 호출 ${fields[0]}`);
        assert.deepEqual(current.map((value) => value >>> 0), expected, `행 ${count} 현재 상태`);
        count += 1;
    }
    assert.equal(count, nextRows.length);
    assert.equal(count, 410);
});
