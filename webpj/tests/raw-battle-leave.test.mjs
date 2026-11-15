// 전투 이탈 통지와 상태 예약 raw-battle-leave.js(RawBattleLeave)를 원본 10.78 관측 64행과 cpppj `BattleLeaveTests`·`BattleLeaveReasonTests`·`RetreatConfirmationTests` 계약으로 대조한다(WB04-E557).
//  - battle-leave-x86.tsv 16행(header 3·receive 7·next 3·parent 3), battle-abort-state-x86.tsv 8행, battle-leave-reason-x86.tsv 22행, retreat-confirmation-x86.tsv 18행.
//  - 계약: 52h 등록(빈 observer·버전 1/정책 0·실제 수신기)과 수신의 Player 비트, 양수 이유 계약, 확인 안내의 이벤트 분배와 필수 Tell.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E557)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { LocalMissionParentResult } from '../www/core/raw-local-mission-state.js';
import { installNativeMainMenuState } from '../www/core/native-main-menu-state.js';
import { RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { LEAVE_BATTLE_COMMAND } from '../www/core/mission-guide-input.js';
import { InputCode } from '../www/core/input-queue.js';
import {
    BATTLE_LEAVE_PACKET_BYTES, initializeBattleLeavePacket, makeBattleLeavePacket, receiveBattleLeave, registerBattleLeavePacket, requestMainMenuAfterLeave,
    prepareBattleAbortState, prepareBattleLeaveState, dispatchBattleRetreatConfirmation, sendBattleLeaveForState
} from '../www/core/raw-battle-leave.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E557)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const leaveRows = fixture('battle-leave-x86.tsv');
const abortRows = fixture('battle-abort-state-x86.tsv');
const reasonRows = fixture('battle-leave-reason-x86.tsv');
const retreatRows = fixture('retreat-confirmation-x86.tsv');
// 바이트 배열을 소문자 16진 문자열로 만든다(원문 Hex). (2026-11-15 WB04-E557)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
// 다섯 144바이트 State 블록을 원문과 같은 순서의 720바이트 16진 문자열로 직렬화한다(첫 DWORD 이외의 보존 물리도 검사). (2026-11-15 WB04-E557)
const stateBytes = (local) => local.blocks().map((block) => hex(new Uint8Array(block.buffer, block.byteOffset, 144))).join(':');
// 모든 블록을 한 바이트 값으로 채운다(원문과 같은 입력 흔적). (2026-11-15 WB04-E557)
function fillBlocks(local, byte) {
    for (const block of local.blocks()) new Uint8Array(block.buffer, block.byteOffset, 144).fill(byte);
}
// 선택 경계 도달 때 원본과 같은 지점에서 관측을 끝내는 검사 전용 신호이다. (2026-11-15 WB04-E557)
class Stop extends Error {
    constructor(cause) { super(cause); this.cause = cause; }
}

test('RawBattleLeave: 원본 10.78 통지 헤더·수신·메인 복귀 예약·State3 복귀 16행의 바이트·송신이 같다', { skip: !leaveRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    // 제품 작성 전에 고정한 원본 출력만 기대값으로 사용한다.
    for (const row of leaveRows) {
        assert.equal(row.length, 4);
        if (row[0] === 'header') {
            const packet = new Uint8Array(16).fill(Number(row[1]));
            initializeBattleLeavePacket(packet);
            assert.equal(hex(packet), row[2]);
        } else if (row[0] === 'receive') {
            const players = new Uint8Array(1620);
            for (let index = 0; index < players.length; index += 1) players[index] = (index * 19 + 7) & 0xff;
            new DataView(players.buffer).setUint32(180 + 80, Number(row[1]) >>> 0, true);
            receiveBattleLeave(1, players);
            assert.equal(hex(players), row[2]);
        } else if (row[0] === 'next') {
            const local = new ClientLocalStateGlobals();
            fillBlocks(local, Number(row[1]));
            requestMainMenuAfterLeave(local.state());
            assert.equal(stateBytes(local), row[2]);
        } else if (row[0] === 'parent') {
            const local = new ClientLocalStateGlobals();
            const state = local.state();
            state.current[0] = 3;
            state.phase[0] = row[1] === 'battle-fourteen' ? 14 : row[1] === 'battle-other-phase' ? 12 : 4;
            let battle = row[1] !== 'default';
            let sent = '';
            let prefixed = false;
            const globals = {
                state,
                sourceType: { value: 0 },
                outcome: { value: 0 },
                firstMenu: { value: 0 },
                editor: { value: 0 },
                battle: () => battle,
                background: new Uint8Array(80),
                currentName: new Uint8Array(256),
                pendingName: new Uint8Array(256)
            };
            const hooks = {
                configInt: () => 1,
                leaveBattle: () => { assert.equal(prefixed, false); sent = `9:${hex(makeBattleLeavePacket())}`; },
                prefix: () => { prefixed = true; battle = false; },
                hudPrefix: () => {}, title: () => {}, closeGuide: () => {}, tell: () => {}, startup: () => 0, tail: () => {}
            };
            assert.equal(installNativeMainMenuState(globals, hooks), LocalMissionParentResult.Commit);
            assert.ok(prefixed);
            assert.equal(sent, row[2]);
        } else assert.fail(`알 수 없는 행 ${row[0]}`);
        rows += 1;
    }
    assert.equal(rows, 16);
});

test('RawBattleLeave: 원본 10.78 공통 중단 상태 8행(단독/편집 직접 쓰기와 삭제 경계 중단의 부분 효과)이 같다', { skip: !abortRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    // 원문이 관측한 전체 상태와 두 DWORD 를 함께 확인한다.
    for (const row of abortRows) {
        assert.equal(row.length, 9);
        const local = new ClientLocalStateGlobals();
        fillBlocks(local, Number(row[1]));
        const abortPending = { value: 0xaabbccdd };
        const outcome = { value: Number(row[2]) };
        const editor = row[0] === 'editor' ? 1 : 0;
        let trace = '';
        let cause = 'normal';
        // 실제 파일은 지우지 않고 선택된 경계 중단만 관측한다.
        const remove = () => {
            trace = 'path';
            if (row[3] === 'path') throw new Stop('path');
            trace += ';delete';
            if (row[3] === 'delete') throw new Stop('delete');
        };
        try { prepareBattleAbortState({ state: local.state(), abortPending, outcome, missionPost: true, editor, metamap: false, battle: !editor }, remove); } catch (stopped) { if (!(stopped instanceof Stop)) throw stopped; cause = stopped.cause; }
        assert.equal(cause, row[4]);
        assert.equal(abortPending.value, Number(row[5]));
        assert.equal(outcome.value, Number(row[6]));
        assert.equal(stateBytes(local), row[7]);
        assert.equal(trace, row[8]);
        rows += 1;
    }
    assert.equal(rows, 8);
});

test('RawBattleLeave: 원본 10.78 양수 이유 22행의 선행/공통 삭제 부분 효과와 상태 예약이 같다', { skip: !reasonRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 선행 삭제/공통 삭제의 실패도 source/abort/State 를 롤백하지 않는 원문 순서를 확인한다.
    for (const row of reasonRows) {
        assert.equal(row.length, 10);
        const local = new ClientLocalStateGlobals();
        fillBlocks(local, 0xa5);
        const script = { value: 17 };
        const source = { value: 133 };
        const abort = { value: 19 };
        const outcome = { value: 2 };
        const editor = row[0] === 'editor' ? 1 : 0;
        const known = row[0] === 'mission' ? 1 : 0;
        const battle = row[0] === 'mission';
        let paths = 0;
        let cause = 'normal';
        let trace = '';
        const state = local.state();
        // 실제 외부 장치 진입 순간의 전역도 기록해 쓰기 순서 차이를 검출한다(PE 실행기의 같은 8 DWORD 순서).
        const boundary = (event) => {
            trace += `${trace ? ';' : ''}${event}`;
            for (const value of [script.value, source.value, abort.value, outcome.value, state.next[0], state.current[33], state.immediate[33], state.next[33]]) trace += `,${value >>> 0}`;
            if (event === row[2]) throw new Stop(event);
        };
        // 파일을 실제 삭제하지 않고 path/CRT remove 두 선택 장치를 원래 순서로 관측한다.
        const remove = () => { paths += 1; boundary(`path${paths}`); boundary(`delete${paths}`); };
        try {
            prepareBattleLeaveState(Number(row[1]) | 0, { abort: { state, abortPending: abort, outcome, missionPost: true, editor, metamap: false, battle }, scriptBreak: script, sourceType: source, playerKnown: known }, remove);
        } catch (stopped) { if (!(stopped instanceof Stop)) throw stopped; cause = stopped.cause; }
        assert.equal(cause, row[3], `행 ${count}`);
        assert.deepEqual([script.value, source.value, abort.value, outcome.value], [Number(row[4]), Number(row[5]), Number(row[6]), Number(row[7])], `행 ${count}`);
        assert.equal(stateBytes(local), row[8], `행 ${count} 상태`);
        assert.equal(trace || '-', row[9], `행 ${count} 경계`);
        count += 1;
    }
    assert.equal(count, 22);
});

test('RawBattleLeave: 원본 10.78 signed 이유 확인 관문 18행(Tell 구역·중단·양수 본문 진행)이 같다', { skip: !retreatRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 최초 CPU 출력만 기대값으로 읽는다.
    for (const row of retreatRows) {
        assert.equal(row.length, 5);
        const reason = Number(BigInt.asIntN(32, BigInt(row[0])));
        let section = '';
        let stopped = false;
        let handled = false;
        try {
            handled = dispatchBattleRetreatConfirmation({ code: LEAVE_BATTLE_COMMAND, x: 77, y: reason }, (name) => { section = name; if (Number(row[1])) throw new Stop('tell'); });
        } catch (error) { if (!(error instanceof Stop)) throw error; stopped = true; }
        assert.equal(stopped, Number(row[2]) === 0);
        assert.ok(stopped || handled === (Number(row[3]) === 0));
        assert.equal(section || '-', row[4]);
        count += 1;
    }
    assert.equal(count, 18);
});

test('RawBattleLeave: 52h 등록(빈 observer·버전 1/정책 0·실제 수신기)과 로컬 9 송신 계약', () => {
    const players = new Uint8Array(1620);
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    registerBattleLeavePacket(dispatch, players);
    const registration = dispatch.registration(0x52);
    assert.deepEqual([registration.version, registration.mismatch, typeof registration.observer, registration.packetObserver, typeof registration.receive], [1, 0, 'function', null, 'function']);
    // 수신기는 패킷 본문을 읽지 않고 현재 sender 의 Player+80 비트 2 만 합친다.
    registration.receive(1, {}, new Uint8Array(5));
    assert.equal(players[180 + 80], 4);
    registration.receive(1, {}, new Uint8Array(5));
    assert.equal(players[180 + 80], 4);
    assert.throws(() => receiveBattleLeave(9, players), RangeError);
    assert.throws(() => receiveBattleLeave(8, new Uint8Array(100)), RangeError);
    // 실제 로컬 9 로 5바이트 통지를 보낸다(강제 수신 없음).
    const sent = [];
    sendBattleLeaveForState({ sendLocalPacket: (destination, packet) => sent.push([destination, hex(packet)]) });
    assert.deepEqual(sent, [[9, '0152050000']]);
    assert.equal(BATTLE_LEAVE_PACKET_BYTES, 5);
    assert.throws(() => initializeBattleLeavePacket(new Uint8Array(3)), RangeError);
});

test('RawBattleLeave: 양수 이유 계약(0 이하 거부·이유 2 직접 예약·누락 삭제 장치)과 확인 이벤트 분배', () => {
    const local = new ClientLocalStateGlobals();
    const script = { value: 17 };
    const source = { value: 133 };
    const abort = { value: 19 };
    const outcome = { value: 2 };
    const globals = { abort: { state: local.state(), abortPending: abort, outcome, missionPost: true, editor: 0, metamap: false, battle: true }, scriptBreak: script, sourceType: source, playerKnown: 1 };
    // 음수와 0 은 확인 안내의 영역이며 전역을 바꾸지 않는다.
    for (const reason of [-2147483648, -1, 0]) {
        assert.throws(() => prepareBattleLeaveState(reason, globals, undefined), RangeError);
        assert.deepEqual([script.value, source.value, abort.value, outcome.value, local.state().next[0]], [17, 133, 19, 2, 0]);
    }
    // 이유 2 는 공통 중단/저장 삭제/+84 지우기 없이 직접 next 를 3 으로 예약한다.
    prepareBattleLeaveState(2, globals, undefined);
    assert.deepEqual([script.value, source.value, abort.value, outcome.value, local.state().next[0]], [1, 133, 19, 2, 3]);
    // 이유 3 의 선행 삭제 장치가 없으면 앞선 쓰기(script)만 남기고 명시 오류이다.
    local.state().next[0] = 0;
    script.value = 17;
    assert.throws(() => prepareBattleLeaveState(3, globals, undefined), /선행 저장 삭제 장치 미연결/);
    assert.deepEqual([script.value, source.value, abort.value, outcome.value, local.state().next[0]], [1, 133, 19, 2, 0]);
    // 원격 전투 조건(번호 확정·전투·미션 후 아님·이유 2/3 아님)에서는 원격 처리를 필수로 먼저 부른다.
    const remote = { abort: { ...globals.abort, missionPost: false }, scriptBreak: script, sourceType: source, playerKnown: 1 };
    assert.throws(() => prepareBattleLeaveState(1, remote, () => {}), /원격 전투 페널티\/재접속 처리 미연결/);
    const order = [];
    prepareBattleLeaveState(9, remote, () => order.push('delete'), () => order.push('remote'));
    assert.deepEqual(order, ['remote', 'delete', 'delete']);
    assert.equal(source.value, 0);
    // 확인 이벤트: 뗌은 Tell 없이 소비하고 다른 명령과 양수 이유는 소비하지 않으며 Tell 누락은 명시 오류이다.
    for (const reason of [-2147483648, -1, 0, 1, 2147483647]) assert.equal(dispatchBattleRetreatConfirmation({ code: LEAVE_BATTLE_COMMAND | InputCode.release, x: 91, y: reason }, undefined), true);
    assert.equal(dispatchBattleRetreatConfirmation({ code: 0x0200008c, x: 0, y: 0 }, undefined), false);
    assert.equal(dispatchBattleRetreatConfirmation({ code: LEAVE_BATTLE_COMMAND, x: 0, y: 1 }, undefined), false);
    assert.throws(() => dispatchBattleRetreatConfirmation({ code: LEAVE_BATTLE_COMMAND, x: 1, y: 0 }, undefined), /Tell 소비자 미연결/);
    const calls = [];
    assert.equal(dispatchBattleRetreatConfirmation({ code: LEAVE_BATTLE_COMMAND | InputCode.control | InputCode.shift, x: 2147483647, y: -1 }, (section) => calls.push(section)), true);
    assert.deepEqual(calls, ['ConfirmRetreat']);
});

test('RawBattleLeave: 공통 중단의 전투 복귀 분기(결과 보존·next 6)와 원격 처리 조건·수신 Player 상한', () => {
    // 미션 후/편집/메타맵이 모두 아니면 전투일 때만 결과(0 이면 3)·next=6 을 쓰고, 전투가 아니면 두 값을 건드리지 않는다. 세 +84 는 어느 쪽이든 지운다.
    const run = (battle, outcomeStart) => {
        const local = new ClientLocalStateGlobals();
        fillBlocks(local, 0xa5);
        const state = local.state();
        state.next[0] = 9;
        const abortPending = { value: 0 };
        const outcome = { value: outcomeStart };
        let deleted = 0;
        prepareBattleAbortState({ state, abortPending, outcome, missionPost: false, editor: 0, metamap: false, battle }, () => { deleted += 1; });
        return [abortPending.value, outcome.value, state.next[0], state.next[33], state.immediate[33], state.current[33], deleted];
    };
    assert.deepEqual(run(true, 0), [1, 3, 6, 0, 0, 0, 1]);
    assert.deepEqual(run(true, 5), [1, 5, 6, 0, 0, 0, 1]);
    assert.deepEqual(run(false, 0), [1, 0, 9, 0, 0, 0, 1]);
    // 저장 삭제 장치가 없으면 앞선 쓰기를 남기고 명시 오류이다.
    const local = new ClientLocalStateGlobals();
    assert.throws(() => prepareBattleAbortState({ state: local.state(), abortPending: { value: 0 }, outcome: { value: 0 }, missionPost: false, editor: 0, metamap: false, battle: true }, undefined), /삭제 장치 미연결/);
    assert.equal(local.state().next[0], 6);
    // 원격 전투 처리는 번호 확정·전투·미션 후 아님·이유 2/3 아님이 모두 맞을 때만 필수로 부른다(하나라도 아니면 부르지 않는다).
    const calls = [];
    const attempt = (reason, patch) => {
        const l = new ClientLocalStateGlobals();
        const globals = { abort: { state: l.state(), abortPending: { value: 0 }, outcome: { value: 0 }, missionPost: false, editor: 0, metamap: false, battle: true }, scriptBreak: { value: 0 }, sourceType: { value: 5 }, playerKnown: 1 };
        Object.assign(globals, patch.top ?? {});
        Object.assign(globals.abort, patch.abort ?? {});
        calls.length = 0;
        prepareBattleLeaveState(reason, globals, () => {}, () => calls.push('remote'));
        return calls.length;
    };
    assert.equal(attempt(1, {}), 1);
    assert.equal(attempt(1, { top: { playerKnown: 0 } }), 0);
    assert.equal(attempt(1, { abort: { battle: false } }), 0);
    assert.equal(attempt(1, { abort: { missionPost: true } }), 0);
    assert.equal(attempt(2, {}), 0);
    assert.equal(attempt(3, {}), 0);
    assert.equal(attempt(9, {}), 1);
    // 수신은 Player 9 개까지이며 물리가 충분해도 sender 9 이상은 거부한다.
    assert.throws(() => receiveBattleLeave(9, new Uint8Array(2000)), RangeError);
    const players = new Uint8Array(2000);
    receiveBattleLeave(8, players);
    assert.equal(players[8 * 180 + 80], 4);
});
