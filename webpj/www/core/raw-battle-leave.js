// 전투에서 State 3 로 돌아갈 때의 원본 52h 통지(0040aa60 헤더·004a71a0 수신)와 다음 상태 예약(004b3540·004b6350·004d803e·004d8035)을 같은 Player/상태 저장소에 적용하는 모듈이다(cpppj `client/RawBattleLeave.cpp/.h`).
// 사용:
//   initializeBattleLeavePacket(packet) → 앞 4바이트(버전 1·타입 0x52·길이 5·0)만 쓰고 나머지 입력 물리는 보존한다.   makeBattleLeavePacket() → State 3 가 생성자 뒤 다섯째 바이트를 0 으로 쓴 5바이트 통지이다.
//   receiveBattleLeave(sender, players) → 패킷 본문을 읽지 않고 현재 sender(0..8)의 Player(180바이트 기록)+80 DWORD 에 비트 2 를 합친다(나머지 세 바이트 보존).   registerBattleLeavePacket(dispatch, players) → 52h 를 빈 observer/버전 1·정책 0/실제 수신기로 등록한다.
//   requestMainMenuAfterLeave(state) → next 의 첫 DWORD 만 3 으로 예약한다(즉시 정리하지 않음).
//   prepareBattleAbortState(globals, deleteSavedState) → 004b6350: abortPending=1, 미션 후/편집/메타맵이 아니고 전투면 (결과 0 이면 3) next=6, 그 밖은 결과 0·next=3, 세 블록의 +84 를 0 으로 지운 뒤 저장 파일 삭제 장치로 넘긴다.
//     globals = { state: {next, immediate, current}, abortPending: {value}, outcome: {value}, missionPost, editor, metamap, battle } 이며 읽기 전용 다섯은 { value } 상자·현재 값을 돌려주는 함수·단순 값 중 하나이다.
//   prepareBattleLeaveState(reason, globals, deleteSavedState, remoteLeave) → 004d803e 의 양수 이유(0 이하는 거부): 원격 전투 조건이면 remoteLeave 를 필수로 먼저 부르고, 이유 2 는 직접 next=3, 이유 3/9 는 선행 삭제 뒤 sourceType=0, 그 뒤 공통 중단을 실행한다.
//     globals = { abort: 위 prepareBattleAbortState 의 globals, scriptBreak: {value}, sourceType: {value}, playerKnown }.
//   dispatchBattleRetreatConfirmation(event, tell) → 004d8035: 떠나기 명령(0x020000c5)의 뗌은 소비하고 signed 이유(y)가 0 이하이면 "ConfirmRetreat" 안내만 열며 양수/다른 명령은 false 이다.
//   sendBattleLeaveForState(session) → 실제 로컬 9 에 52h 통지를 예약한다(session.sendLocalPacket(9, packet), 강제 수신/flush 없음).
// 선택된 장치가 없으면 앞선 쓰기를 되돌리지 않고 명시 오류로 알린다. 원격(MP) 경로의 실제 처리는 호출자가 remoteLeave 로 공급하며 빈 성공으로 대체하지 않는다.
// 원본 관측: cpppj `battle-leave-x86.tsv` 16행(header 3·receive 7·next 3·parent 3)·`battle-abort-state-x86.tsv` 8행·`battle-leave-reason-x86.tsv` 22행·`retreat-confirmation-x86.tsv` 18행을 tests/raw-battle-leave.test.mjs 가 대조한다.
// 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E557: 전투 이탈 통지와 상태 예약 RawBattleLeave 이식)
import { LEAVE_BATTLE_COMMAND } from './mission-guide-input.js';
import { InputCode } from './input-queue.js';

// 52h 통지의 길이(타입 52·버전 1·길이 5 고정 규격)와 Player 기록 폭·수신이 합치는 필드 위치와 비트, 플레이어 수이다. (2026-11-15 WB04-E557)
export const BATTLE_LEAVE_PACKET_BYTES = 5;
const PACKET_TYPE = 0x52;
const PLAYER_STRIDE = 180;
const LEAVE_FIELD_OFFSET = 80;
const LEAVE_FLAG = 4;
const PLAYER_LIMIT = 9;
// 실제 State 번호(전투 복귀 6·메인 복귀 3), +84 부가 인자 칸(DWORD 33), 확인 안내 구역 이름이다. (2026-11-15 WB04-E557)
const BATTLE_STATE = 6;
const MAIN_STATE = 3;
const EXTRA_WORD = 33;
const CONFIRM_SECTION = 'ConfirmRetreat';
// 읽기 전용 현재 전역은 { value } 상자·현재 값을 돌려주는 함수·독립 입력의 단순 값 중 하나로 받아 호출 위치마다 다시 읽는다. (2026-11-15 WB04-E557)
const read = (source) => (typeof source === 'function' ? source() : (source !== null && typeof source === 'object' ? source.value : source));

// 0040aa60 처럼 앞 4바이트만 쓰고 나머지 입력 물리는 보존한다. (2026-11-15 WB04-E557)
export function initializeBattleLeavePacket(packet) {
    if (packet.length < 4) throw new RangeError('전투 종료 패킷 헤더 물리 부족');
    packet[0] = 1;
    packet[1] = PACKET_TYPE;
    packet[2] = BATTLE_LEAVE_PACKET_BYTES;
    packet[3] = 0;
}

// State 3 가 생성자 뒤 다섯째 바이트를 0 으로 쓴 실제 통지이며 지정하지 않은 추가 필드를 만들지 않는다. (2026-11-15 WB04-E557)
export function makeBattleLeavePacket() {
    const packet = new Uint8Array(BATTLE_LEAVE_PACKET_BYTES);
    initializeBattleLeavePacket(packet);
    packet[4] = 0;
    return packet;
}

// 004a71a0: 원본은 패킷 본문을 읽지 않고 분배기의 현재 sender 만 사용한다. 리틀엔디언 DWORD 의 비트 2 만 바꾸므로 나머지 세 바이트도 보존한다. (2026-11-15 WB04-E557)
export function receiveBattleLeave(sender, players) {
    const at = sender * PLAYER_STRIDE + LEAVE_FIELD_OFFSET;
    if (sender >= PLAYER_LIMIT || at + 4 > players.length) throw new RangeError('전투 종료 수신 Player 물리 범위');
    players[at] |= LEAVE_FLAG;
}

// 52h 를 같은 분배 표에 등록한다. 등록된 Player 배열의 수명은 해당 Session 의 수신 등록 해제보다 길어야 한다. (2026-11-15 WB04-E557)
export function registerBattleLeavePacket(dispatch, players) {
    const receiver = dispatch.registration(PACKET_TYPE).canonicalReceive;
    // 실제 Native 연결 수신(004a71a0)이 있으면 같은 값만 대여하고 독립 typed 의 기존 등록을 유지한다.
    if (receiver && receiver.nativeEntry === 0x004a71a0) {
        const row = dispatch.registration(PACKET_TYPE);
        if (row.version !== 1 || row.mismatch !== 0 || row.length !== BATTLE_LEAVE_PACKET_BYTES || !row.observer || row.packetObserver) throw new Error('Native 연결 수신의 같은 metadata 불일치');
        if (typeof dispatch.registerCanonical !== 'function') throw new Error('Native 연결 수신 등록 장치 미연결');
        dispatch.registerCanonical(PACKET_TYPE, receiver);
        return;
    }
    dispatch.observe(PACKET_TYPE, () => {});
    dispatch.version(PACKET_TYPE, 1, 0);
    dispatch.register(PACKET_TYPE, (sender) => receiveBattleLeave(sender, players));
}

// 004b3540: next 의 첫 DWORD 만 3 으로 예약하며 다른 부가 인자/블록은 지우지 않는다. (2026-11-15 WB04-E557)
export function requestMainMenuAfterLeave(state) { state.next[0] = MAIN_STATE; }

// 004b6350 의 다음 3/6 선택과 세 +84 store 뒤에만 저장 파일 장치로 넘어간다. 선택된 장치 중단도 앞선 쓰기를 보존한다. (2026-11-15 WB04-E557)
export function prepareBattleAbortState(globals, deleteSavedState) {
    globals.abortPending.value = 1;
    if (!read(globals.missionPost) && !read(globals.editor) && !read(globals.metamap)) {
        // 원격 복귀 선택은 원문 정적 복원만 했으며 새 멀티플레이 검사는 하지 않는다.
        if (read(globals.battle)) {
            if (!(globals.outcome.value >>> 0)) globals.outcome.value = 3;
            globals.state.next[0] = BATTLE_STATE;
        }
    } else {
        globals.outcome.value = 0;
        globals.state.next[0] = MAIN_STATE;
    }
    globals.state.next[EXTRA_WORD] = 0;
    globals.state.immediate[EXTRA_WORD] = 0;
    globals.state.current[EXTRA_WORD] = 0;
    if (typeof deleteSavedState !== 'function') throw new Error('전투 종료의 실제 저장 경로/삭제 장치 미연결');
    deleteSavedState();
}

// 004d803e 의 양수 이유 부모이다. 이유 2 는 직접 next=3(공통 중단/저장 삭제/+84 지우기 없음), 3/9 는 선행 삭제 뒤 sourceType 0 과 공통 중단이다. (2026-11-15 WB04-E557)
export function prepareBattleLeaveState(reason, globals, deleteSavedState, remoteLeave) {
    if ((reason | 0) < 1) throw new RangeError('전투 이탈 양수 이유 계약 오류');
    // 원격 전투 조건이면 원격 페널티/재접속 처리를 필수로 먼저 부른다(MP 실행 검사는 제외되며 빈 성공으로 대체하지 않음).
    if (read(globals.playerKnown) && read(globals.abort.battle) && !read(globals.abort.missionPost) && reason !== 2 && reason !== 3) {
        if (typeof remoteLeave !== 'function') throw new Error('LeaveBattle의 원격 전투 페널티/재접속 처리 미연결');
        remoteLeave();
    }
    globals.scriptBreak.value = 1;
    if (reason === 2) { requestMainMenuAfterLeave(globals.abort.state); return; }
    if (reason === 3 || reason === 9) {
        if (typeof deleteSavedState !== 'function') throw new Error('전투 이탈의 선행 저장 삭제 장치 미연결');
        deleteSavedState();
        globals.sourceType.value = 0;
    }
    // 같은 장기 전역을 다시 읽으며 두 번째 삭제의 부분 효과도 보존한다.
    prepareBattleAbortState(globals.abort, deleteSavedState);
}

// 004d8035 의 signed JL 을 보존한다: 이유 0 이하는 이탈 실행이 아니라 확인창 요청이다. event = { code, x, y }. (2026-11-15 WB04-E557)
export function dispatchBattleRetreatConfirmation(event, tell) {
    if (((event.code & 0x03ffffff) >>> 0) !== LEAVE_BATTLE_COMMAND) return false;
    if (event.code & InputCode.release) return true;
    if ((event.y | 0) >= 1) return false;
    if (typeof tell !== 'function') throw new Error('전투 이탈 확인의 실제 Tell 소비자 미연결');
    tell(CONFIRM_SECTION);
    return true;
}

// State 1/3 의 cleanup 전 52h/본문 0 을 같은 실제 로컬 9 에 예약한다(강제 수신/flush 는 하지 않음). session = { sendLocalPacket(destination, packet) }. (2026-11-15 WB04-E557)
export function sendBattleLeaveForState(session) {
    session.sendLocalPacket(9, makeBattleLeavePacket());
}
