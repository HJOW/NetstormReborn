// 미션 Guide 입력(mission-guide-input.js, cpppj `client/MissionGuideInput`)을 원본 10.78 관측과 cpppj 시험, 웹 계약으로 확인한다.
//  - 원본 관측: mission-first-x86.tsv 278행(First 함수 정상 반환 264 + F8 내부 분기 14: 순번/flags/demo/단계/Tell 결과 × 사건 코드 × 큐 예약)의 전체 208BYTE·반환·호출 순서, mission-common-x86.tsv 108행(Init demo 문자열과 Reload 캡처),
//    mission-abort-x86.tsv 1002행(Abort/미처리 사건 960행의 반환·큐·Tell과 제어 이름 조회/토큰/큐 42행)을 전부 대조한다.
//  - cpppj 시험 이식: MissionGuideInputTests 3건(Native278 · QueueOrderCapacityAndRelease · CommonNative108), MissionControlTests 의 Native1002. 같은 파일의 AuthoredReplayAndAbortConfirmation 은 실제 DialogPage 파서(DialogScript, PORT_MAP todo)가
//    필요해 그 이식 때 옮긴다(보호 tell 스크립트의 TryAgain/ABORT 본문이 있는지만 여기서 확인).
//  - 웹 계약: 제어 이름의 대소문자 무시와 미지원 이름의 무변경 거부, Abort 의 Tell 연결 누락 거부, Reload 캡처의 이름 소유 복사, demo 문자열 규칙, F8 예약 조건(release·Ctrl/Shift·비활성·다른 키).
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E343)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { put, readRows } from './raw-support.mjs';
import { InputCode, InputQueue } from '../www/core/input-queue.js';
import { createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { createMissionStartState } from '../www/core/raw-mission-start.js';
import { ScriptStringRing } from '../www/ui/script-command.js';
import {
    LEAVE_BATTLE_COMMAND, MISSION_ABORT_COMMAND, MISSION_BEGIN_COMMAND, MISSION_END_COMMAND, MISSION_FIRST_COMMAND, MISSION_FORK_COMMAND, MISSION_RELOAD_COMMAND,
    applyMissionDemo, captureMissionReload, dispatchMissionAbort, dispatchMissionFirst, missionControlScriptCode, missionFirst, queueMissionControlAction, queueMissionFirstKey, tellMissionStage,
} from '../www/core/mission-guide-input.js';

// 원본 관측 행이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-12 WB04-E343)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const FIRST_ROWS = fixture('mission-first-x86.tsv');
const COMMON_ROWS = fixture('mission-common-x86.tsv');
const ABORT_ROWS = fixture('mission-abort-x86.tsv');
// 격리 PE 입력의 가상 표 식별값이며 호스트 포인터가 아니다. (2026-11-12 WB04-E343)
const NATIVE_MISSION_TABLE = 0x12002000;

// 소문자 16진 문자열로 바꾼다. (2026-11-12 WB04-E343)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
// 문자열의 각 글자를 한 바이트로 보는 ASCII 16진이다. (2026-11-12 WB04-E343)
const textHex = (text) => hex(Array.from(text, (ch) => ch.charCodeAt(0)));
// 16진 문자열을 한 글자 한 바이트의 원문으로 복원한다(빈 값은 빈 문자열). (2026-11-12 WB04-E343)
function controlBytes(encoded) {
    let result = '';
    // 한 바이트씩 읽어 UTF-8 치환으로 원문을 바꾸지 않는다.
    for (let index = 0; index < encoded.length; index += 2) result += String.fromCharCode(Number.parseInt(encoded.slice(index, index + 2), 16));
    return result;
}
// 예외가 나는지 본다. (2026-11-12 WB04-E343)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

// 한 행의 10개 입력과 상태/관문을 만들고 외부 효과를 기록하는 First 장면이다. cpppj FirstScene 에 대응한다. (2026-11-12 WB04-E343)
class FirstScene {
    // input = op / flags / sequence / demo / stage / shown / Tell / current / code / 하위 입력 반환이다.
    constructor(values) {
        this.input = values.split(',').map(Number);
        assert.equal(this.input.length, 10);
        this.raw = new Uint8Array(0xd0);
        // 원본 관측 준비와 같은 무관 바이트 표식이다.
        for (let index = 0; index < this.raw.length; index += 1) this.raw[index] = (index * 37 + 41) & 0xff;
        put(this.raw, 0, NATIVE_MISSION_TABLE);
        put(this.raw, 0x54, this.input[1]);
        put(this.raw, 0x58, this.input[2]);
        put(this.raw, 0x68, this.input[3]);
        put(this.raw, 0xb4, this.input[5]);
        this.state = { ...createMissionStartState(), flags: this.input[1], sequence: this.input[2] };
        this.gate = { ...createMissionFrameGateState(), demo: this.input[3], stage: this.input[4] | 0 };
        this.shown = { value: this.input[5] };
        this.trace = '';
    }

    // 경계 한 번의 전체 trace 를 순서대로 추가한다.
    add(value) {
        if (this.trace) this.trace += ';';
        this.trace += value;
    }

    // Tell 의 반환값/팝업/정지는 명시 원본 외부 입력이며 단계/결과 계산을 대신하지 않는다.
    show() {
        tellMissionStage(this.gate, this.shown, {
            tell: (section) => { this.add(`tell:${textHex(section)}`); return this.input[6]; },
            closeTransient: () => this.add('close'),
            pause: () => this.add('pause'),
        });
    }

    // 같은 상태에 실제 제품 First 와 사건 분기/입력 큐를 실행한다.
    run() {
        let result = 'void'; // First 자체에는 의미 있는 반환값이 없다.
        if (this.input[0] === 0) missionFirst(this.state, () => this.show());
        else if (this.input[0] === 1) {
            const event = { code: this.input[8], x: 0x12345678, y: 0x87654321 | 0 }; // native 의 세 DWORD 입력이다.
            const first = this.input[7] ? () => missionFirst(this.state, () => this.show()) : undefined;
            if (dispatchMissionFirst(event, first)) result = '1';
            else if (this.input[7]) { this.add(`input:${this.input[8]}:305419896:2271560481`); result = String(this.input[9]); } else result = '0';
        } else {
            const queue = new InputQueue();
            queueMissionFirstKey({ code: this.input[8], x: 0, y: 0 }, queue, this.input[7] !== 0);
            if (!queue.empty) {
                const event = queue.pop(false);
                this.add(`queue:${event.code}:${event.x}:${event.y}`);
            }
            result = 'slice';
        }
        put(this.raw, 0x54, this.state.flags);
        put(this.raw, 0x58, this.state.sequence);
        put(this.raw, 0xb4, this.shown.value);
        return `${result}|${hex(this.raw)}|${this.trace === '' ? '-' : this.trace}`;
    }
}

test('원본 관측 278행: First 함수 정상 반환 264개와 F8 내부 분기 14개의 전체 바이트·반환·호출 순서', { skip: FIRST_ROWS === null }, () => {
    assert.equal(FIRST_ROWS.length, 278);
    // 제품 실행 결과를 동결된 원문과 직접 비교한다. 기대값을 다시 계산하지 않는다.
    for (const row of FIRST_ROWS) {
        assert.equal(row.length, 2);
        assert.equal(new FirstScene(row[0]).run(), row[1]);
    }
});

test('cpppj MissionGuideInput_QueueOrderCapacityAndRelease: 40칸 큐의 순서/초과 폐기와 release·Ctrl/Shift·비활성 관문', () => {
    const queue = new InputQueue();
    queue.push(17, 12, 13);
    assert.equal(queueMissionFirstKey({ code: 0x770000, x: 1, y: 2 }, queue, true), true);
    assert.equal(queue.pop(false).code, 17);
    const first = queue.pop(false);
    assert.deepEqual(first, { code: MISSION_FIRST_COMMAND, x: 0, y: 0 });
    assert.equal(queue.empty, true);
    assert.equal(queueMissionFirstKey({ code: InputCode.release | 0x770000, x: 0, y: 0 }, queue, true), false);
    assert.equal(queueMissionFirstKey({ code: 0x770000, x: 0, y: 0 }, queue, false), false);
    assert.equal(queueMissionFirstKey({ code: 0x780000, x: 0, y: 0 }, queue, true), false);
    assert.equal(queueMissionFirstKey({ code: (0x770000 | InputCode.control) >>> 0, x: 0, y: 0 }, queue, true), false);
    assert.equal(queueMissionFirstKey({ code: (0x770000 | InputCode.shift) >>> 0, x: 0, y: 0 }, queue, true), false);
    assert.equal(queue.empty, true);
    // DEL(127)의 원본 Ctrl+Backspace 변환과 용량 검사를 분리하려고 일반 문자 1000 부터 채운다.
    for (let index = 0; index < 39; index += 1) queue.push(index + 1000, 0, 0);
    assert.equal(queueMissionFirstKey({ code: 0x770000, x: 0, y: 0 }, queue, true), true);
    // 기존 사건 39개의 순서는 F8 예약 시도 뒤에도 그대로다.
    for (let index = 0; index < 39; index += 1) assert.equal(queue.pop(false).code, index + 1000);
    assert.equal(queue.empty, true);
    const state = createMissionStartState();
    missionFirst(state, undefined);
    assert.equal(state.flags, 1);
    assert.equal(state.sequence, 0);
    state.sequence = 1;
    assert.throws(() => missionFirst(state, undefined), /안내 연결 누락/u);
});

test('원본 관측 108행: Init demo 문자열과 Reload 캡처의 선택 필드', { skip: COMMON_ROWS === null }, () => {
    assert.equal(COMMON_ROWS.length, 108);
    // 빈 문자열·signed/감김·bool 정규화와 소유 이름 복사를 원문 출력으로 대조한다.
    for (const row of COMMON_ROWS) {
        assert.equal(row.length, 5);
        let text = controlBytes(row[3]);
        if (row[0] === 'I') {
            const demo = { value: Number(row[1]) };
            applyMissionDemo(text, demo);
            assert.equal(String(demo.value), row[4]);
        } else {
            const request = captureMissionReload(text, Number(row[1]), Number(row[2]));
            text = '해제된 이전 이름'; // 원래 입력 수명이 끝나도 요청이 원본 이름을 소유해야 한다.
            assert.equal(`${request.mode},${request.demo},${textHex(request.name)}`, row[4]);
        }
    }
});

// 원본 전체 활성 큐와 비교할 입력 큐를 순서대로 직렬화한다(실제 pop 을 쓰며 이 입력은 DEL 변환값과 겹치지 않는다). (2026-11-12 WB04-E343)
function controlQueue(queue) {
    const parts = [];
    // 큐의 끝까지 꺼낸다.
    while (!queue.empty) {
        const event = queue.pop(false);
        parts.push(`${event.code}:${event.x >>> 0}:${event.y >>> 0}`);
    }
    return parts.length === 0 ? '-' : parts.join(';');
}

test('원본 관측 1002행: Guide Abort/미처리 사건 960개와 제어 이름 조회/토큰/큐 42개', { skip: ABORT_ROWS === null }, () => {
    assert.equal(ABORT_ROWS.length, 1002);
    let abortRows = 0;
    let nameRows = 0;
    // 각 행은 새 큐/고리를 가지며 앞선 관측의 결과를 재사용하지 않는다.
    for (const row of ABORT_ROWS) {
        if (row[0] === 'A') {
            assert.equal(row.length, 3);
            const input = row[1].split(',').map(Number);
            assert.equal(input.length, 6);
            const queue = new InputQueue();
            let trace = '';
            // 독립 명시 입력의 기존 활성 큐만 채운다.
            for (let index = 0; index < input[4]; index += 1) queue.push(0x300 + index, index + 10, index + 20);
            const event = { code: input[1], x: input[2] | 0, y: input[3] | 0 }; // 원본 세 DWORD 의 비트를 보존한다.
            let result = 0;
            if (dispatchMissionAbort(event, queue, (section) => { trace = `tell:${textHex(section)}`; })) result = 1;
            else if (input[0]) { trace = `input:${input[1]}:${input[2]}:${input[3]}`; result = input[5]; } // 미처리 사건의 명시 가상 반환 경계다.
            assert.equal(`${result}|${controlQueue(queue)}|${trace === '' ? '-' : trace}`, row[2]);
            abortRows += 1;
        } else {
            assert.equal(row[0], 'S');
            assert.equal(row.length, 5);
            const source = controlBytes(row[1]);
            const comma = source.indexOf(',');
            assert.ok(comma >= 0);
            const queue = new InputQueue();
            const strings = new ScriptStringRing();
            assert.equal(queueMissionControlAction(source.slice(0, comma), source.slice(comma + 1), queue, strings), true);
            assert.equal(queue.empty, false);
            const event = queue.pop(false);
            assert.equal(queue.empty, true);
            assert.equal(event.x, event.y);
            assert.equal(strings.index, Number(row[3]));
            const expected = row[2].split(',');
            assert.equal(expected.length, 2);
            assert.equal(event.code, Number(expected[0]));
            if (expected[1][0] === 's') assert.equal(strings.resolve(event.x >>> 0), controlBytes(expected[1].slice(1)));
            else assert.equal(event.x >>> 0, Number(expected[1].slice(1)));
            nameRows += 1;
        }
    }
    assert.equal(abortRows, 960);
    assert.equal(nameRows, 42);
});

test('제어 이름: 여섯 이름은 대소문자를 무시하고 그 밖은 0xffffffff 이며 미지원 이름은 상태를 바꾸지 않고 거부한다', () => {
    assert.equal(missionControlScriptCode('MissionRestart'), MISSION_RELOAD_COMMAND);
    assert.equal(missionControlScriptCode('MISSIONABORT'), MISSION_ABORT_COMMAND);
    assert.equal(missionControlScriptCode('leavebattle'), LEAVE_BATTLE_COMMAND);
    assert.equal(missionControlScriptCode('MissionBegin'), MISSION_BEGIN_COMMAND);
    assert.equal(missionControlScriptCode('MissionFork'), MISSION_FORK_COMMAND);
    assert.equal(missionControlScriptCode('MissionEnd'), MISSION_END_COMMAND);
    for (const unknown of ['', 'Unimplemented', 'MissionRestart2', ' MissionAbort']) assert.equal(missionControlScriptCode(unknown), 0xffffffff);
    // 코드 값은 원본 전역 명령 표의 값이다.
    assert.deepEqual([MISSION_FIRST_COMMAND, MISSION_RELOAD_COMMAND, MISSION_ABORT_COMMAND, LEAVE_BATTLE_COMMAND, MISSION_BEGIN_COMMAND, MISSION_FORK_COMMAND, MISSION_END_COMMAND],
        [0x02000091, 0x0200008d, 0x0200008c, 0x020000c5, 0x0200008b, 0x0200008e, 0x0200008f]);
    const queue = new InputQueue();
    const strings = new ScriptStringRing();
    assert.equal(queueMissionControlAction('Unimplemented', '0', queue, strings), false);
    assert.equal(queue.empty, true);
    assert.equal(strings.index, 0);
    // 지원 이름은 숫자 인자를 그대로, 문자 인자는 고리 토큰으로 예약한다.
    assert.equal(queueMissionControlAction('MissionRestart', '0', queue, strings), true);
    assert.deepEqual(queue.pop(false), { code: MISSION_RELOAD_COMMAND, x: 0, y: 0 });
    assert.equal(queueMissionControlAction('MissionBegin', 'Tutorial1', queue, strings), true);
    const begin = queue.pop(false);
    assert.equal(begin.code, MISSION_BEGIN_COMMAND);
    assert.equal(strings.resolve(begin.x >>> 0), 'Tutorial1');
});

test('Abort 분기: y 가 0 이면 ABORT Tell, 아니면 LeaveBattle/2/2 예약이고 항상 소비하며 Tell 연결이 없으면 거부한다', () => {
    const queue = new InputQueue();
    const tells = [];
    assert.equal(dispatchMissionAbort({ code: MISSION_ABORT_COMMAND, x: 99, y: 0 }, queue, (section) => tells.push(section)), true);
    assert.deepEqual(tells, ['ABORT']);
    assert.equal(queue.empty, true);
    assert.equal(dispatchMissionAbort({ code: MISSION_ABORT_COMMAND, x: 0, y: 1 }, queue, (section) => tells.push(section)), true);
    assert.deepEqual(queue.pop(false), { code: LEAVE_BATTLE_COMMAND, x: 2, y: 2 });
    assert.equal(tells.length, 1);
    // 상위 플래그 비트(release 등)는 무시하고 다른 사건은 소비하지 않는다.
    assert.equal(dispatchMissionAbort({ code: (MISSION_ABORT_COMMAND | InputCode.release) >>> 0, x: 0, y: -1 }, queue, () => {}), true);
    assert.equal(queue.pop(false).code, LEAVE_BATTLE_COMMAND);
    assert.equal(dispatchMissionAbort({ code: MISSION_FIRST_COMMAND, x: 0, y: 0 }, queue, () => {}), false);
    assert.ok(throws(() => dispatchMissionAbort({ code: MISSION_ABORT_COMMAND, x: 0, y: 0 }, queue, undefined)));
    // First 도 상위 플래그 비트를 무시하며 현재 미션이 없어도 소비한다.
    let firsts = 0;
    assert.equal(dispatchMissionFirst({ code: (MISSION_FIRST_COMMAND | InputCode.release) >>> 0, x: 0, y: 0 }, () => { firsts += 1; }), true);
    assert.equal(dispatchMissionFirst({ code: MISSION_FIRST_COMMAND, x: 0, y: 0 }, undefined), true);
    assert.equal(dispatchMissionFirst({ code: MISSION_ABORT_COMMAND, x: 0, y: 0 }, () => { firsts += 1; }), false);
    assert.equal(firsts, 1);
});

test('안내 표시: demo 는 Demo 섹션, 단계 글자 0 은 빈 섹션, 반환값은 후속 효과와 무관하게 마지막에 쓰인다', () => {
    const gate = createMissionFrameGateState();
    const shown = { value: 7 };
    const sections = [];
    const effects = (value) => ({ tell: (section) => { sections.push(section); return value; }, closeTransient: () => sections.push('K'), pause: () => sections.push('P') });
    gate.demo = 1;
    tellMissionStage(gate, shown, effects(0));
    assert.equal(shown.value, 0);
    gate.demo = 0;
    gate.stage = 0x141;
    tellMissionStage(gate, shown, effects(5));
    gate.stage = 0;
    tellMissionStage(gate, shown, effects(0xffffffff));
    gate.stage = -191; // 하위 바이트 0x41 ('A')
    tellMissionStage(gate, shown, effects(0));
    assert.deepEqual(sections, ['Demo', 'A.', 'K', 'P', '', 'K', 'P', 'A.']);
    assert.equal(shown.value, 0);
    tellMissionStage(gate, shown, effects(0xffffffff));
    assert.equal(shown.value, 0xffffffff);
    // 연결 누락: Tell 이 없으면 거부, 비영 반환이면 팝업과 정지가 모두 있어야 한다.
    assert.throws(() => tellMissionStage(gate, shown, {}), /Tell 연결 누락/u);
    // 부호 있는 음수 반환도 DWORD 로 쓴다.
    tellMissionStage(gate, shown, { tell: () => -1, closeTransient: () => {}, pause: () => {} });
    assert.equal(shown.value, 0xffffffff);
    assert.ok(throws(() => tellMissionStage(gate, shown, { tell: () => 1, closeTransient: () => {} })));
    assert.ok(throws(() => tellMissionStage(gate, shown, { tell: () => 1, pause: () => {} })));
    tellMissionStage(gate, shown, { tell: () => 0 });
    assert.equal(shown.value, 0);
});

test('Reload 캡처와 demo 문자열 규칙', () => {
    const request = captureMissionReload('Mission', 7, 0);
    assert.deepEqual(request, { name: 'Mission', mode: 1, demo: 0 });
    assert.deepEqual(captureMissionReload('', 0, -1), { name: '', mode: 0, demo: 1 });
    const demo = { value: 123 };
    applyMissionDemo('', demo);
    assert.equal(demo.value, 123);
    applyMissionDemo('  -7x', demo);
    assert.equal(demo.value, 0xfffffff9);
    applyMissionDemo('abc', demo);
    assert.equal(demo.value, 0);
    applyMissionDemo('4294967297', demo);
    assert.equal(demo.value, 1);
    // 정밀도를 잃지 않고 32비트로 감긴다(2^64+1 → 1).
    applyMissionDemo('18446744073709551617', demo);
    assert.equal(demo.value, 1);
});
