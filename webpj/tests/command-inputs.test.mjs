// 일반 입력 분배 다섯 개(설정 명령·송금·AI 진단·시나리오 객체·저장 카메라 숫자 키)를 원본 10.78 관측으로 대조한다(WB04-E27, cpppj ConfigurationCommandInputTests·SendMoneyInputTests·AiDebugMenuTests(입력)·ScenarioObjectInputTests·SavedCameraKeyTests 이식).
//  - configuration-commands-x86.tsv 238행(정지/Console 독립·Guide/Gump 선행 소비 제외·현재 전역 재조회·부분 효과 순서), configuration-command-names-x86.tsv 30행(이름→번호만).
//  - send-money-input-x86.tsv 888행(고정 세 금액·AI/연결 Player 분기·차감·지갑·패킷 순서)과 그중 AI 312행의 위치별 진단 줄.
//  - ai-debug-input-x86.tsv 200행, scenario-object-input-x86.tsv 480행, saved-camera-key-x86.tsv 1750행(언어/수정키/유효성/현재 Renderer 중심/부분 쓰기 전체 장부).
// 대본 큐(QueueScriptCommand)·실제 Client 설정/State 저장 manager·카메라 소유자 연결은 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E27: 입력 분배 다섯 개 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, readRows } from './raw-support.mjs';
import { InputCode } from '../www/core/input-queue.js';
import {
    CHANGE_PAL_COMMAND, CHANGE_PRIORITY_COMMAND, SET_FORT_TYPE_COMMAND, configurationCommandScriptCode,
    dispatchConfigurationCommandInput, isConfigurationCommand, planNativeThreadPriority,
} from '../www/core/configuration-command-input.js';
import {
    SEND_300_COMMAND, SEND_700_COMMAND, SEND_1000_COMMAND, SEND_ALL_ASK_COMMAND, SEND_ALL_COMMAND, SEND_X_COMMAND,
    confirmSendMoney, dispatchSendMoneyInput, isSendMoneyCommand, sendMoneyScriptCode,
} from '../www/core/send-money-input.js';
import {
    AI_REPORTING_COMMAND, DEBUG_SWITCH_COMMAND, aiDebugCommandScriptCode, applyAiDebugCommandState,
    dispatchAiDebugCommandInput, isAiDebugCommand,
} from '../www/core/ai-debug-command-input.js';
import { dispatchScenarioObjectInput, isScenarioObjectCommand, scenarioObjectScriptCode } from '../www/core/scenario-object-input.js';
import {
    SavedCameraPositions, captureSavedCameraCenter, dispatchSavedCameraKey, initializeSavedCameraPositions, isSavedCameraPositionValid,
} from '../www/core/saved-camera-key.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E27)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const configRows = fixture('configuration-commands-x86.tsv');
const configNameRows = fixture('configuration-command-names-x86.tsv');
const moneyRows = fixture('send-money-input-x86.tsv');
const aiRows = fixture('ai-debug-input-x86.tsv');
const objectRows = fixture('scenario-object-input-x86.tsv');
const cameraRows = fixture('saved-camera-key-x86.tsv');

// 원문 DWORD 를 읽고(감김) 부호 있는 값/비트 열로 되돌린다. (2026-11-08 WB04-E27)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const signed = (value) => value | 0;
const bits = (value) => value >>> 0;
// 원본 바이트 16진과 문자열(글자 하나당 바이트 하나) 사이 변환이다. '-' 는 빈 문자열이다. (2026-11-08 WB04-E27)
const hexToText = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));
const textToHex = (text) => Buffer.from(text, 'latin1').toString('hex');
// 명시한 하위 중단만 잡기 위한 오류이다. 모듈 자체의 미연결 오류와 구별한다. (2026-11-08 WB04-E27)
class Stop extends Error {}
const failed = (action) => { try { action(); } catch { return true; } return false; };

// 독립 PE 가 결정한 공통 관문 뒤 순서와 현재 전역 재조회이다. MP challenge 경로는 관측에 없다.
test('설정 명령: 원본10.78 238행(Console/정지 독립·현재 전역 재조회·부분 효과)', { skip: !configRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of configRows) {
        if (row[0] === 'event') continue;
        assert.equal(row.length, 21);
        count += 1;
        let battle = number(row[7]);
        const editor = number(row[8]);
        let owner = number(row[9]);
        const priority = { value: signed(number(row[10])) };
        const source = hexToText(row[11]);
        const state = new Uint8Array(28).fill(0xa5);
        const trace = [];
        const calls = new Map();
        // 하위 효과를 기록한 뒤 입력에 지정된 현재 메모리 변화만 적용한다.
        const add = (name, argument = '') => {
            trace.push(`${name}${argument}`);
            const trigger = `${name}${(calls.get(name) ?? 0) + 1}`;
            calls.set(name, (calls.get(name) ?? 0) + 1);
            if (row[14] === '-') return;
            for (const effect of row[14].split(';')) {
                const fields = effect.split('/');
                assert.equal(fields.length, 3);
                if (fields[0] !== trigger) continue;
                const address = number(fields[1]);
                const value = number(fields[2]);
                if (address === 0x54db2c) priority.value = signed(value);
                else if (address === 0x540cac) owner = value;
                else if (address === 0x594fbc) battle = value;
                else assert.fail(`알 수 없는 변경 주소 ${address}`);
            }
        };
        const hooks = {
            text: () => source,
            set: (key, value) => add('set', `:${textToHex(key)}:${textToHex(value)}`),
            get: (key) => { add('get', `:${textToHex(key)}:0:0:0`); return hexToText(row[12]); },
            palette: (name) => add('palette', `:${textToHex(name)}`),
            thread: (value) => { add('current-thread'); add('thread', `:4294967294:${value}`); },
            config: (assignment) => add('config', `:${assignment}`),
            state: (slot) => { add('state', `:${slot}`); return state; },
            notify: (slot) => add('notify', `:${slot}`),
            assertion: (line) => add('assert', `:${line}`),
        };
        const reads = { battle: () => battle, challenge: () => 0, editor: () => editor, owner: () => owner, highPriority: priority };
        // Guide/Gump 가 소비한 행은 분배 전에 제외한다. Console/정지는 이 명령의 추가 거부 조건이 아니다.
        if (!number(row[5]) && !number(row[6])) {
            assert.equal(dispatchConfigurationCommandInput({ code: number(row[0]), x: signed(number(row[1])), y: signed(number(row[2])) }, reads, hooks), true);
        }
        const label = `행 ${count} ${row[0]}`;
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[15], `${label} 장치 순서`);
        assert.ok(row[16] === '1' && row[17] === '0');
        assert.deepEqual([bits(priority.value), owner, Buffer.from(state).toString('hex')], [number(row[18]), number(row[19]), row[20]], `${label} 최종 상태`);
    }
    assert.equal(count, 238);
});

// 이름→번호 대응만 읽는다(인자·문자열 고리·큐 예약은 대본 큐 단위에서).
test('설정 명령: 이름표 30행의 이름→번호와 번호 판별', { skip: !configNameRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of configNameRows) {
        assert.equal(row.length, 9);
        count += 1;
        const name = hexToText(row[1]).split(',')[0];
        assert.equal(configurationCommandScriptCode(name), row[6] === '-' ? 0xffffffff : number(row[6].split(',')[0]), `${name} 행 ${count}`);
    }
    assert.equal(count, 30);
    assert.ok(isConfigurationCommand(CHANGE_PAL_COMMAND | InputCode.shift) && !isConfigurationCommand(0x29));
});

// 새 쓰기·설정 알림·메인 루프가 빌리는 priority DWORD 를 분리 복제하지 않는다.
test('설정 명령: 우선순위 공유 DWORD·부분 효과·뗌/다른 키', () => {
    const priority = { value: 73 };
    const trace = [];
    const reads = { highPriority: priority };
    const hooks = {
        thread: (value) => { assert.equal(value, 15); assert.equal(priority.value, 2); priority.value = -7; trace.push('T'); },
        config: (assignment) => { assert.equal(assignment, 'highPriorityValue = "-7"'); trace.push('C'); },
    };
    assert.equal(dispatchConfigurationCommandInput({ code: CHANGE_PRIORITY_COMMAND, x: 123, y: 2 }, reads, hooks), true);
    assert.deepEqual([priority.value, trace.join('')], [-7, 'TC']);
    // OS 경계가 중단되면 공유 DWORD 쓰기는 남고 설정 알림은 실행하지 않는다. 범위 밖 값은 NORMAL(0)을 요청한다.
    trace.length = 0;
    hooks.thread = (value) => { assert.equal(value, 0); assert.equal(priority.value, -1); trace.push('F'); throw new Stop('명시 OS 경계 중단'); };
    assert.ok(failed(() => dispatchConfigurationCommandInput({ code: CHANGE_PRIORITY_COMMAND, x: 0, y: -1 }, reads, hooks)));
    assert.deepEqual([priority.value, trace.join('')], [-1, 'F']);
    hooks.thread = (value) => { assert.equal(value, 1); trace.push('T'); };
    hooks.config = () => { trace.push('X'); throw new Stop('명시 Config 경계 중단'); };
    trace.length = 0;
    assert.ok(failed(() => dispatchConfigurationCommandInput({ code: CHANGE_PRIORITY_COMMAND, x: 0, y: 1 }, reads, hooks)));
    assert.deepEqual([priority.value, trace.join('')], [1, 'TX']);
    // 뗌은 장치 없이 소비하고 다른 키는 소비하지 않는다.
    assert.equal(dispatchConfigurationCommandInput({ code: (CHANGE_PRIORITY_COMMAND | InputCode.release) >>> 0, x: 0, y: 2 }, reads, {}), true);
    assert.equal(dispatchConfigurationCommandInput({ code: 0x58, x: 0, y: 0 }, reads, {}), false);
    assert.equal(priority.value, 1);
});

// OS 등급 비교와 요청 값이다.
test('설정 명령: 스레드 우선순위 계획과 SetFortType State 경계', () => {
    assert.deepEqual(planNativeThreadPriority(1, 1), { wanted: 1, currentLevel: 1, priority: 1, change: false });
    assert.deepEqual(planNativeThreadPriority(2, 0), { wanted: 2, currentLevel: 0, priority: 15, change: true });
    // 범위 밖 wanted 는 clamp 하지 않고 매 프레임 NORMAL 을 다시 요청한다.
    assert.deepEqual(planNativeThreadPriority(7, 0), { wanted: 7, currentLevel: 0, priority: 0, change: true });
    assert.equal(planNativeThreadPriority(0, 2).currentLevel, 1);
    assert.equal(planNativeThreadPriority(2, 15).change, false);
    // SetFortType: getter 뒤 owner 를 다시 읽어 Notify 에 넘기고, 28바이트 미만 State 는 거부한다.
    let owner = 1;
    const state = new Uint8Array(28);
    const seen = [];
    const hooks = { state: (slot) => { seen.push(`S${slot}`); owner = 2; return state; }, notify: (slot) => seen.push(`N${slot}`), assertion: (line) => seen.push(`A${line}`) };
    const reads = { editor: () => 0, owner: () => owner };
    assert.equal(dispatchConfigurationCommandInput({ code: SET_FORT_TYPE_COMMAND, x: 0, y: 0x1234 }, reads, hooks), true);
    assert.deepEqual([state[0x11], seen.join(',')], [0x34, 'A2012,S1,N2']);
    hooks.state = () => new Uint8Array(27);
    assert.throws(() => dispatchConfigurationCommandInput({ code: SET_FORT_TYPE_COMMAND, x: 0, y: 1 }, reads, hooks), RangeError);
});

// 입력 열은 장치 응답으로만 쓰고 기대 trace 는 독립 원문 관측이다.
test('송금: 원본10.78 888행(고정 세 금액·AI/연결 Player 분기·차감·지갑·패킷)', { skip: !moneyRows && '원본 fixture 없음' }, () => {
    const hex = (bytes) => Buffer.from(bytes).toString('hex');
    let count = 0;
    for (const row of moneyRows) {
        if (row[0] === 'name') continue;
        assert.equal(row.length, 16);
        count += 1;
        const trace = [];
        const hooks = {
            local: () => number(row[3]),
            flags: (owner) => { assert.equal(owner, number(row[2])); return number(row[4]); },
            checking: () => number(row[11]),
            mission: () => number(row[10]),
            canAfford: (amount) => { trace.push(`afford:${floatToBits(amount)}`); return number(row[5]) !== 0; },
            addOwner: (owner, amount) => trace.push(`addOwner:${owner}:${floatToBits(amount)}`),
            addLocal: (amount) => trace.push(`addLocal:${floatToBits(amount)}`),
            wallet: (owner, amount) => trace.push(`wallet:${owner}:${amount}`),
            send: (packet) => trace.push(`send:9:${hex(packet)}`),
            assertion: () => trace.push('assert'),
        };
        // Guide/Gump 가 소비한 사건은 일반 명령을 실행하지 않는다.
        if (!number(row[6]) && !number(row[7])) dispatchSendMoneyInput({ code: number(row[1]), x: 71, y: signed(number(row[2])) }, hooks);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[13], `행 ${count} ${row[0]}`);
        assert.ok(row[14] === '1' && row[15] === '0');
    }
    assert.equal(count, 888);
});

// AI(flags 0x20/0x22) 312행만 위치별 진단 줄을 받고 패킷 장치는 선택하면 실패한다.
test('송금: AI 312행의 위치별 진단 줄(inSingle, Userinput.cpp)', { skip: !moneyRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of moneyRows) {
        if (row[0] === 'name') continue;
        const flags = number(row[4]);
        if (flags !== 0x20 && flags !== 0x22) continue;
        count += 1;
        const trace = [];
        const hooks = {
            local: () => number(row[3]),
            flags: (owner) => { assert.equal(owner, number(row[2])); return flags; },
            checking: () => number(row[11]),
            mission: () => number(row[10]),
            canAfford: (amount) => { trace.push(`afford:${floatToBits(amount)}`); return number(row[5]) !== 0; },
            addLocal: (amount) => trace.push(`addLocal:${floatToBits(amount)}`),
            wallet: (owner, amount) => trace.push(`wallet:${owner}:${amount}`),
            assertionAt: (expression, file, line) => {
                const code = (number(row[1]) & 0x03ffffff) >>> 0;
                assert.deepEqual([expression, file, line], ['inSingle', '.\\Userinput.cpp', code === 0x02000120 ? 0x913 : code === 0x02000108 ? 0x8e9 : 0x8c2]);
                trace.push('assert');
            },
            addOwner: () => { throw new Error('제외한 연결 Player 차감'); },
            send: () => { throw new Error('제외한 패킷 송금'); },
        };
        if (!number(row[6]) && !number(row[7])) assert.equal(dispatchSendMoneyInput({ code: number(row[1]), x: 71, y: signed(number(row[2])) }, hooks), true);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[13], `행 ${count}`);
    }
    assert.equal(count, 312);
});

// All 은 확인 순간의 SP 를 읽고 정수 부분만 송금한다.
test('송금: All 은 현재 금액을 읽고 X 입력·확인 창은 대상만 전달한다', () => {
    let sp = 1234.75;
    let wallet = 0;
    const counters = { reads: 0, checks: 0, dialogs: 0, asks: 0 };
    const hooks = {
        money: () => { counters.reads += 1; return sp; },
        known: () => 1,
        canAfford: (amount) => { counters.checks += 1; return amount <= sp; },
        flags: (owner) => { assert.equal(owner, 2); return 0x22; },
        checking: () => 1,
        mission: () => 1,
        addLocal: (amount) => { sp += amount; },
        wallet: (owner, amount) => { assert.equal(owner, 2); wallet += amount; },
        numberDialog: (owner) => { assert.equal(owner, 2); counters.dialogs += 1; },
        askAll: (owner) => { assert.equal(owner, 2); counters.asks += 1; },
    };
    assert.equal(dispatchSendMoneyInput({ code: SEND_X_COMMAND, x: 0, y: 2 }, hooks), true);
    assert.deepEqual([counters.dialogs, sp, wallet], [1, 1234.75, 0]);
    assert.equal(dispatchSendMoneyInput({ code: SEND_ALL_ASK_COMMAND, x: 0, y: 2 }, hooks), true);
    assert.deepEqual([counters.asks, sp, wallet], [1, 1234.75, 0]);
    assert.equal(dispatchSendMoneyInput({ code: SEND_ALL_COMMAND, x: 0, y: 2 }, hooks), true);
    assert.deepEqual([counters.reads, counters.checks, sp, wallet], [2, 1, 0.75, 1234]);
    // 확인 버튼: 너무 큰 금액·0 은 이체 없이 1 로 닫고, 첫 LF 앞만 읽는다.
    assert.equal(confirmSendMoney(2, '9999999999', hooks), 1);
    assert.deepEqual([sp, wallet], [0.75, 1234]);
    assert.equal(confirmSendMoney(2, '0', hooks), 1);
    assert.equal(sp, 0.75);
    sp = 1000;
    assert.equal(confirmSendMoney(2, '700\n999', hooks), 1);
    assert.deepEqual([sp, wallet], [300, 1934]);
    assert.equal(confirmSendMoney(2, '301', hooks), 1);
    assert.equal(sp, 300);
});

// 뗌과 다른 입력은 월드/돈 장치를 요구하지 않는다.
test('송금: 여섯 이름표·뗌·다른 입력은 장치 불필요', () => {
    const names = [['Send300Bucks', SEND_300_COMMAND], ['Send700Bucks', SEND_700_COMMAND], ['SendBucks', SEND_1000_COMMAND], ['sEnDxBuCkS', SEND_X_COMMAND], ['SendAllBucks2', SEND_ALL_ASK_COMMAND], ['SendAllBucks', SEND_ALL_COMMAND]];
    const seen = new Set();
    for (const [name, code] of names) {
        assert.equal(sendMoneyScriptCode(name), code);
        assert.ok(isSendMoneyCommand(code));
        assert.ok(!seen.has(code), '번호 중복');
        seen.add(code);
        assert.equal(dispatchSendMoneyInput({ code: (code | 0x80000000) >>> 0, x: 0, y: 0 }, {}), true);
    }
    assert.equal(dispatchSendMoneyInput({ code: 123, x: 0, y: 0 }, {}), false);
    assert.equal(sendMoneyScriptCode('Unknown'), 0xffffffff);
    // 선택된 장치의 누락은 거부한다.
    assert.throws(() => dispatchSendMoneyInput({ code: SEND_X_COMMAND, x: 0, y: 1 }, {}), /미연결/);
});

// 신규 MP 양성은 고정 관측에 포함하지 않는다. 입력 열은 장치 물리로만 쓴다.
test('AI 진단 입력: 원본10.78 200행(reporting 토글·debug 29/99)과 공통 뗌 관문', { skip: !aiRows && '원본 fixture 없음' }, () => {
    let count = 0;
    const put = (bytes, offset, value) => { for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff; };
    for (const row of aiRows) {
        assert.equal(row.length, 6);
        count += 1;
        const player = new Uint8Array(180);
        const body = new Uint8Array(0xb08).fill(0xa5);
        put(player, 0xb0, number(row[2]) ? 1 : 0);
        put(body, 0xb00, number(row[3]));
        let debug = number(row[3]);
        const state = {
            metamap: () => 0,
            player: (owner) => { assert.equal(owner, number(row[1])); return player; },
            body: (token) => { assert.equal(token, 1); return body; },
            readDebug: (address) => { assert.equal(address, 0x52f3cc); return debug; },
            writeDebug: (address, value) => { assert.equal(address, 0x52f3cc); debug = value; },
        };
        const savedPlayer = Uint8Array.from(player);
        const code = number(row[0]);
        applyAiDebugCommandState({ code, x: 123, y: signed(((code & 0x3ffffff) >>> 0) === DEBUG_SWITCH_COMMAND ? 0x52f3cc : number(row[1])) }, state);
        const expected = new Uint8Array(0xb08).fill(0xa5);
        put(expected, 0xb00, number(row[4]));
        assert.deepEqual([Buffer.from(body).equals(Buffer.from(expected)), Buffer.from(player).equals(Buffer.from(savedPlayer)), debug], [true, true, number(row[5])], `행 ${count}`);
    }
    assert.equal(count, 200);
    assert.ok(isAiDebugCommand(DEBUG_SWITCH_COMMAND | InputCode.shift));
    assert.equal(dispatchAiDebugCommandInput({ code: (AI_REPORTING_COMMAND | InputCode.release) >>> 0, x: 0, y: 1 }, {}), true);
    assert.equal(dispatchAiDebugCommandInput({ code: (DEBUG_SWITCH_COMMAND | InputCode.release) >>> 0, x: 0, y: 0x52f3cc }, {}), true);
    assert.equal(dispatchAiDebugCommandInput({ code: 0x58, x: 0, y: 0 }, {}), false);
});

// 선택된 물리의 범위와 미선택 경로의 장치 비요구이다.
test('AI 진단 입력: 이름표·Player 범위·brain 토큰 없음·메타맵', () => {
    assert.equal(aiDebugCommandScriptCode('ToGgLeReporting'), AI_REPORTING_COMMAND);
    assert.equal(aiDebugCommandScriptCode('toggledebug\0tail'), DEBUG_SWITCH_COMMAND);
    assert.equal(aiDebugCommandScriptCode('AIReporting'), 0xffffffff);
    // y 가 0 이하이거나 메타맵이면 Player 를 읽지 않는다.
    assert.equal(dispatchAiDebugCommandInput({ code: AI_REPORTING_COMMAND, x: 0, y: 0 }, { metamap: () => 0 }), true);
    assert.equal(dispatchAiDebugCommandInput({ code: AI_REPORTING_COMMAND, x: 0, y: 3 }, { metamap: () => 1 }), true);
    // Player 번호 9 이상은 물리 범위 밖이다.
    assert.throws(() => dispatchAiDebugCommandInput({ code: AI_REPORTING_COMMAND, x: 0, y: 9 }, { metamap: () => 0 }), RangeError);
    // brain 토큰이 0 이면 본문을 읽지 않는다.
    assert.equal(dispatchAiDebugCommandInput({ code: AI_REPORTING_COMMAND, x: 0, y: 2 }, { metamap: () => 0, player: () => new Uint8Array(180) }), true);
    // 짧은 Player 행은 물리 범위 오류이다.
    assert.throws(() => dispatchAiDebugCommandInput({ code: AI_REPORTING_COMMAND, x: 0, y: 2 }, { metamap: () => 0, player: () => new Uint8Array(0xb2) }), RangeError);
    // 미선택 장치는 요구하지 않고 선택된 debug 경로의 누락은 거부한다.
    assert.throws(() => dispatchAiDebugCommandInput({ code: DEBUG_SWITCH_COMMAND, x: 0, y: 1 }, {}), /미연결/);
});

// 가상 몸체의 효과를 실행하지 않고 원문 일반 입력의 전체 호출 기록을 재생한다.
test('시나리오 객체 입력: 원본10.78 480행(메뉴 수명 없이 대상·뗌·반환 무시)', { skip: !objectRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    for (const fields of objectRows) {
        assert.equal(fields.length, 9);
        const code = number(fields[1]);
        const first = number(fields[2]);
        const second = number(fields[3]);
        const result = number(fields[4]);
        let trace = '';
        let calls = 0;
        assert.equal(dispatchScenarioObjectInput({ code, x: signed(first), y: signed(second) }, (sid, command) => {
            calls += 1;
            trace = `${sid}:${command}:${first}:${second}:${result}`;
            return result;
        }), true);
        assert.equal(trace === '' ? '-' : trace, fields[6], `행 ${rows + 1} ${fields[0]}`);
        // 팝업 조회 수는 원문 전체 입력의 근거로 보존하며 객체 분배 함수의 효과로 주장하지 않는다.
        assert.ok(calls <= 1 && fields[8] === '0');
        rows += 1;
    }
    assert.equal(rows, 480);
});

// 다섯 이름은 대소문자 무시이며 없는 별칭은 만들지 않는다. 낮은 SID 0 은 장치를 요구하지 않는다.
test('시나리오 객체 입력: 이름표와 대상 SID 0·장치 요구', () => {
    for (const name of ['MakeMine', 'InflictDamage', 'DestroyPiece', 'TurnToObelisk', 'mElTdOwN']) {
        const code = scenarioObjectScriptCode(name);
        assert.ok(code !== 0xffffffff && isScenarioObjectCommand(code), name);
    }
    assert.equal(scenarioObjectScriptCode('Destroy'), 0xffffffff);
    assert.equal(scenarioObjectScriptCode('MakeMin'), 0xffffffff);
    const code = scenarioObjectScriptCode('MakeMine');
    // 높은 WORD 만 있는 y 는 낮은 SID 0 이라 소비만 하고, 뗌과 다른 키도 장치가 필요 없다.
    assert.equal(dispatchScenarioObjectInput({ code, x: 0, y: 0x10000 }, undefined), true);
    assert.equal(dispatchScenarioObjectInput({ code: (code | InputCode.release) >>> 0, x: 0, y: 6008 }, undefined), true);
    assert.equal(dispatchScenarioObjectInput({ code: 0x58, x: 0, y: 6008 }, undefined), false);
    assert.throws(() => dispatchScenarioObjectInput({ code, x: 0, y: 6008 }, undefined), /미연결/);
});

// 언어/수정키/0 포함 유효성/현재 Renderer 중심과 실패 앞의 전체 저장 장부를 최초 원문에 대조한다.
test('저장 카메라 키: 원본10.78 1750행(언어·수정키·유효성·현재 중심·부분 쓰기)', { skip: !cameraRows && '원본 fixture 없음' }, () => {
    // 지정한 폭의 16진을 바이트열로 읽는다.
    const image = (text, size) => { assert.equal(text.length, size * 2); return Uint8Array.from(Buffer.from(text, 'hex')); };
    let count = 0;
    for (const row of cameraRows) {
        assert.equal(row.length, 16);
        const saved = new SavedCameraPositions();
        const savedBytes = image(row[3], 80);
        // 무효 비트/NaN 도 그대로 80바이트 이미지에 옮긴다.
        new Uint8Array(saved.bits.buffer).set(savedBytes);
        const viewBytes = new DataView(image(row[4], 64).buffer);
        const view = { pixels: [viewBytes.getInt32(0x1c, true), viewBytes.getInt32(0x20, true)], cellFloatBits: [viewBytes.getUint32(0x24, true), viewBytes.getUint32(0x28, true)] };
        const viewportBytes = new DataView(image(row[5], 16).buffer);
        let viewport = [0, 1, 2, 3].map((index) => viewportBytes.getInt32(index * 4, true));
        let language = signed(number(row[2]));
        let trace = '';
        let completed = true;
        const hooks = {
            paused: () => { trace = `P:${row[6]}`; if (number(row[7]) !== 0xfffffffe) language = signed(number(row[7])); return number(row[6]) !== 0; },
            language: () => language,
            sound: () => {
                trace += ';S:select.wav:0:0:0:0:0';
                if (number(row[8])) { view.pixels = [-87, -61]; view.cellFloatBits = [floatToBits(8), floatToBits(5.5)]; viewport = [82, 19, 1000, 747]; saved.set(0, 150.5, 151.5); }
                if (row[9] === 'Sound') throw new Stop('원문 선택음 명시 중단');
            },
            capture: () => { trace += ';V'; if (row[9] === 'Capture') throw new Stop('원문 Renderer 중심 명시 중단'); return captureSavedCameraCenter(view, viewport); },
            center: (x, y) => { trace += `;C:${floatToBits(x)}:${floatToBits(y)}`; if (row[9] === 'Center') throw new Stop('원문 즉시 중심 명시 중단'); },
        };
        try {
            if (row[0] === 'Init') initializeSavedCameraPositions(saved);
            else assert.equal(typeof dispatchSavedCameraKey({ code: number(row[1]), x: 0, y: 0 }, saved, hooks), 'boolean');
        } catch (error) {
            if (!(error instanceof Stop)) throw error;
            completed = false;
        }
        if (trace === '') trace = '-';
        const label = `행 ${count} event ${row[1]} language ${row[2]}`;
        assert.equal(Buffer.from(saved.bits.buffer).toString('hex'), row[10], `${label} 저장 이미지`);
        assert.equal(bits(language), number(row[11]), `${label} 언어`);
        assert.equal(trace, row[12], `${label} 장치 순서`);
        assert.equal(completed, number(row[13]) !== 0, `${label} 완료`);
        count += 1;
    }
    assert.equal(count, 1750);
});

// 선택하지 않은 장치는 요구하지 않고 소리 뒤 캡처 실패의 선행 변경도 보존한다.
test('저장 카메라 키: 장치 지연 요구와 부분 쓰기', () => {
    const saved = new SavedCameraPositions();
    initializeSavedCameraPositions(saved);
    const initial = Buffer.from(saved.bits.buffer).toString('hex');
    const hooks = {};
    // 다른 종류의 사건에 정지/언어/장치를 요구하지 않는다.
    for (const code of [(InputCode.release | 0x31) >>> 0, (InputCode.command | 0x31) >>> 0, 0x310000, 0x43]) assert.equal(dispatchSavedCameraKey({ code, x: 0, y: 0 }, saved, hooks), false);
    hooks.paused = () => true;
    assert.equal(dispatchSavedCameraKey({ code: 0x31, x: 0, y: 0 }, saved, hooks), true);
    assert.equal(Buffer.from(saved.bits.buffer).toString('hex'), initial);
    hooks.paused = () => false;
    hooks.language = () => 0;
    // 무효 저장 위치는 카메라가 없어도 자연 생략한다.
    assert.equal(dispatchSavedCameraKey({ code: 0x31, x: 0, y: 0 }, saved, hooks), true);
    assert.throws(() => dispatchSavedCameraKey({ code: (InputCode.shift | 0x21) >>> 0, x: 0, y: 0 }, saved, hooks), /미연결/);
    assert.equal(Buffer.from(saved.bits.buffer).toString('hex'), initial);
    hooks.sound = () => saved.set(4, 12, 34);
    assert.throws(() => dispatchSavedCameraKey({ code: (InputCode.shift | 0x21) >>> 0, x: 0, y: 0 }, saved, hooks), /미연결/);
    assert.deepEqual([saved.get(4).x, saved.get(0).x], [12, -10000]);
    // 유효한 0 은 중심 장치를 실제 요구한다.
    saved.set(0, 0, -0);
    assert.throws(() => dispatchSavedCameraKey({ code: 0x31, x: 0, y: 0 }, saved, hooks), /미연결/);
    // 유효성과 중심 계산의 경계이다.
    assert.ok(isSavedCameraPositionValid(0, 255.99) && !isSavedCameraPositionValid(256, 0) && !isSavedCameraPositionValid(-0.5, 0) && !isSavedCameraPositionValid(Number.NaN, 0));
    // (82+1000)/2 + (-87) = 454, (19+747)/2 + (-61) = 322.
    assert.deepEqual(captureSavedCameraCenter({ pixels: [-87, -61], cellFloatBits: [floatToBits(8), floatToBits(5.5)] }, [82, 19, 1000, 747]), { x: Math.fround(454 / 8), y: Math.fround(322 / 5.5) });
});
