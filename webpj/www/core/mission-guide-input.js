// 원본 활성 미션의 F8 예약·Guide First 사건·Abort/Reload 제어 명령과 현재 안내 재표시를 옮긴 모듈이다(cpppj `client/MissionGuideInput.cpp/.h`, 10.78).
//  - missionFirst(state, tellStage)(004c2a70): 순번이 0 이면 Tell 을 부르지 않고 flags 값 1(비트 0)만 요청한다(다른 flags 와 상태 필드는 보존). 그 밖에는 현재 안내를 다시 표시하는 `tellStage()` 를 부르며 연결이 없으면 거부한다.
//  - tellMissionStage(gate, shownBox, effects)(004c2a90): demo 이면 섹션 `Demo`, 아니면 단계 글자(gate.stage 의 하위 바이트, 0 이면 빈 섹션)에 `.` 를 붙인 섹션을 `effects.tell` 로 Tell 하고, 반환이 비영이면 `closeTransient()` → `pause()` 를 부른 뒤
//    **그 호출의 반환값**을 `shownBox.value`(현재+b4)에 쓴다(후속 효과가 단계/결과를 바꿔도 마지막 쓰기는 처음 반환값).
//  - dispatchMissionFirst(event, currentFirst)·queueMissionFirstKey(event, queue, activeMission): First 명령 사건(0x02000091, 플래그 비트 제외 후 일치하면 현재 미션이 없어도 소비)과 활성 미션의 F8 key-down(Ctrl/Shift·뗌 제외)을
//    같은 40칸 큐 끝의 First/0/0 예약으로 연결한다(즉시 Tell 하지 않음).
//  - dispatchMissionAbort(event, queue, tell)(00463e40): Abort 사건의 세 번째 DWORD(y)가 0 이면 실제 ABORT Tell, 아니면 LeaveBattle/2/2 를 예약하고 항상 소비한다.
//  - missionControlScriptCode(name)·queueMissionControlAction(name, argument, queue, strings): 복원한 여섯 제어 이름(MissionRestart·MissionAbort·LeaveBattle·MissionBegin·MissionFork·MissionEnd, 대소문자 무시)의 원본 코드를 조회하고
//    (그 밖은 0xffffffff) 같은 토큰 규칙으로 문자열 고리/입력 큐에 예약한다. 지원하지 않는 이름은 상태를 바꾸지 않고 false.
//  - captureMissionReload(name, mode, demo)(00463cf0)·applyMissionDemo(text, demoBox)(004c3290 의 demo 문자열 분기): 해제 전에 이름을 소유 복사하고 mode/demo 를 0/1 로 정규화하는 요청, 빈 문자열이 아니면 atol DWORD 로 덮어쓰는 demo.
//  state 는 `createMissionStartState()` 모양, gate 는 `createMissionFrameGateState()` 모양이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E341: MissionLifecycle 의 선행으로 missionFirst 만 먼저 이식, WB04-E343: 나머지 이식)
import { configParseLong, asciiLower } from '../assets/config.js';
import { InputCode } from './input-queue.js';
import { queueScriptCommand } from '../ui/script-command.js';

// 미션 제어 명령 코드 일곱 개이다. 각각 원본 전역 명령 표의 값이며 호스트 함수 주소가 아니다. (2026-11-12 WB04-E343)
export const MISSION_FIRST_COMMAND = 0x02000091; // 00542938: 현재 미션 첫 단계/안내 재표시.
export const MISSION_RELOAD_COMMAND = 0x0200008d; // 00542928: 현재 미션 재생성(데모 타이머가 일반 큐에 넣음).
export const MISSION_ABORT_COMMAND = 0x0200008c; // 00542924: 확인 창/종료 예약. 분기는 사건의 세 번째 DWORD 를 읽음.
export const LEAVE_BATTLE_COMMAND = 0x020000c5; // 00542a08: 실제 전투 떠나기. Abort 비영 값에서 인자 2/2 를 예약함.
export const MISSION_BEGIN_COMMAND = 0x0200008b; // 00542920: 문자열 인자 미션 시작.
export const MISSION_FORK_COMMAND = 0x0200008e; // 0054292c: fork%d 분기 시작.
export const MISSION_END_COMMAND = 0x0200008f; // 00542930: 현재 미션 가상 종료/해제.

// 명령 사건 코드에서 플래그 비트를 뺀 하위 26비트 마스크와 F8 가상 키(0x77) 사건 값이다. (2026-11-12 WB04-E343)
const COMMAND_BITS = 0x03ffffff;
const F8_KEY_EVENT = 0x77 << InputCode.virtualKeyShift;

// 원본 이름 표에서 독립 관측한 실제 코드만 공급한다(stricmp 의 ASCII 대소문자 비교). 다른 명령을 임의 별칭으로 바꾸지 않는다. (2026-11-12 WB04-E343)
export function missionControlScriptCode(name) {
    const lower = asciiLower(String(name));
    if (lower === 'missionrestart') return MISSION_RELOAD_COMMAND;
    if (lower === 'missionabort') return MISSION_ABORT_COMMAND;
    if (lower === 'leavebattle') return LEAVE_BATTLE_COMMAND;
    if (lower === 'missionbegin') return MISSION_BEGIN_COMMAND;
    if (lower === 'missionfork') return MISSION_FORK_COMMAND;
    if (lower === 'missionend') return MISSION_END_COMMAND;
    return 0xffffffff;
}

// 페이지 버튼/명령의 여섯 제어 이름만 같은 원본 토큰/80칸 문자열/40칸 입력 큐에 예약한다. 지원하지 않는 이름은 상태와 고리를 바꾸지 않고 false 이며 인자는 기존 queueScriptCommand 규칙을 따른다. (2026-11-12 WB04-E343)
export function queueMissionControlAction(name, argument, queue, strings) {
    if (missionControlScriptCode(name) === 0xffffffff) return false;
    // 실제 명령 parser 가 받을 원본 두 필드이다.
    queueScriptCommand(`${name},${argument}`, queue, strings, {
        lookup: missionControlScriptCode,
        unknown: () => { throw new Error('확인한 미션 제어 이름의 조회 실패'); },
    });
    return true;
}

// 00463e40 의 Abort 분기이다. 확인(y) 0 은 실제 ABORT Tell, 그 외는 LeaveBattle/2/2 예약이며 항상 소비한다. 상위 입력 플래그를 먼저 제거하므로 release 표시는 제외 조건이 아니다. (2026-11-12 WB04-E343)
export function dispatchMissionAbort(event, queue, tell) {
    if (((event.code & COMMAND_BITS) >>> 0) !== MISSION_ABORT_COMMAND) return false;
    if (event.y === 0) {
        if (typeof tell !== 'function') throw new Error('미션 중단 확인의 실제 ABORT Tell 연결 누락');
        tell('ABORT');
    } else queue.pushCommand(LEAVE_BATTLE_COMMAND, 2, 2);
    return true;
}

// 원본 Reload 가 해제 전에 캡처하는 미션 이름과 bool 로 정규화한 두 필드이다. 이름은 소유 복사본이라 기존 Script/월드 해제 뒤에도 유효하다. (2026-11-12 WB04-E343)
export function captureMissionReload(name, mode, demo) {
    return { name: String(name), mode: mode ? 1 : 0, demo: demo ? 1 : 0 };
}

// 004c3290 의 demo 문자열 분기이다. 빈 문자열은 기존 직접 지정값을 유지하고 그 외에는 실제 atol DWORD 로 덮어쓴다(공백/잘못된 숫자 문자열은 0). demoBox 는 `{ value }` 상자(uint32)이다. (2026-11-12 WB04-E343)
export function applyMissionDemo(text, demoBox) {
    if (text !== '') demoBox.value = configParseLong(text) >>> 0;
}

// 004c2a90: 현재 demo/단계로 섹션을 고르고 Tell→Kill→정지 후 마지막 반환 DWORD 를 쓴다. demo 안내는 단계 글자와 무관하고, 단계 글자 0 은 원본 NUL 때문에 빈 섹션이다.
//  effects = { tell(섹션) → 정수(00482de0 의 실제 반환 DWORD), closeTransient()(00466710), pause()(현재 미션의 가상+28) }. (2026-11-12 WB04-E343)
export function tellMissionStage(gate, shownBox, effects) {
    if (typeof effects?.tell !== 'function') throw new Error('현재 미션 안내의 실제 Tell 연결 누락');
    let section = 'Demo';
    if (!gate.demo) {
        const stage = gate.stage & 0xff; // 원본은 signed DWORD 의 낮은 char 만 쓴다.
        section = '';
        if (stage) section = `${String.fromCharCode(stage)}.`;
    }
    const shown = effects.tell(section) >>> 0; // 후속 효과가 현재 단계/결과를 바꾸어도 이 호출의 반환값을 마지막에 쓴다.
    if (shown) {
        if (typeof effects.closeTransient !== 'function' || typeof effects.pause !== 'function') throw new Error('현재 미션 안내의 실제 팝업/정지 연결 누락');
        effects.closeTransient();
        effects.pause();
    }
    shownBox.value = shown;
}

// 004c2a70: 순번 0 에서는 flags 값 1(비트 0)만 요청하고 그 외에는 현재 안내를 다시 표시한다. (2026-11-12 WB04-E341)
export function missionFirst(state, tellStage) {
    if (state.sequence === 0) {
        state.flags = (state.flags | 1) >>> 0;
        return;
    }
    if (typeof tellStage !== 'function') throw new Error('현재 미션 First의 실제 안내 연결 누락');
    tellStage();
}

// 00463e40 의 First 분기이다. Guide 가 먼저 소비하므로 release 표시가 있는 명시 First 명령도 같은 분기로 들어가고, 상위 플래그 제거 후 일치하면 현재 미션이 없어도 true 를 반환한다. (2026-11-12 WB04-E343)
export function dispatchMissionFirst(event, currentFirst) {
    if (((event.code & COMMAND_BITS) >>> 0) !== MISSION_FIRST_COMMAND) return false;
    if (typeof currentFirst === 'function') currentFirst();
    return true;
}

// 원본 key-down 활성 미션의 F8 분기를 같은 40칸 큐에 연결한다. 실제 F8 은 즉시 Tell 을 하지 않고 큐 끝에 First/0/0 을 예약한다. Ctrl/Shift·뗌은 예약하지 않는다.
// 도전/편집/디버그 모드와 다른 키의 전체 분배는 이 함수의 범위 밖이다. (2026-11-12 WB04-E343)
export function queueMissionFirstKey(event, queue, activeMission) {
    if (!activeMission || (event.code & InputCode.release) !== 0 || ((event.code & COMMAND_BITS) >>> 0) !== F8_KEY_EVENT
        || (event.code & (InputCode.control | InputCode.shift)) !== 0) return false;
    queue.pushCommand(MISSION_FIRST_COMMAND, 0, 0);
    return true;
}
