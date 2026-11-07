// 대본이 예약하는 설정 일반 명령 세 개(ChangePal·ChangePriority·SetFortType)를 현재 설정·공유 DWORD·State/Notify 부모에 연결하는 입력 분배 모듈이다(cpppj `client/ConfigurationCommandInput.cpp/.h`, 10.78 전용).
// Guide/Gump/뗌 뒤이며 일반 Console/정지 검사보다 앞이다. 수정키를 지운 26비트 명령 전체가 세 명령이 아니면 false, 대상의 뗌은 효과 없이 true 이다(키의 낮은 BYTE 로 대신하지 않는다).
//  - ChangePal(0x02000029): 키를 먼저 고른다(전투 중이면 battlePal, 아니면 도전 중 chalPal, 아니면 fortPal; Set 알림 뒤 모드가 바뀌어도 유지) → text(y)의 첫 NUL 앞 요청으로 set(키, 요청) → get(키)의 현재 값 첫 NUL 앞으로 palette(선택값). Set 입력 대신 현재 설정을 다시 읽는다.
//  - ChangePriority(0x0200008a): 공유 DWORD highPriority 에 y 를 먼저 쓰고(이후 OS 호출이 실패해도 관측 가능) thread(y==1→1, y==2→15, 그 밖→0; 범위 밖을 clamp 하지 않고 NORMAL 요청) → config(`highPriorityValue = "<현재 signed 값>"`) 로 OS 콜백 뒤의 공유값을 다시 읽는다.
//  - SetFortType(0x020000c7): 편집기가 아니면 assertion(2012)(정상 복귀하면 계속) → state(owner)의 28바이트 State 에 y 의 낮은 BYTE 를 +0x11 에 쓰고 → notify(현재 owner 를 다시 조회). 28바이트 미만이면 RangeError.
// planNativeThreadPriority 는 WinMain 국소 부모(00439983)의 비교/요청값(원문 wanted 와 현재 OS 등급)을 계산하며 OS 호출은 소유하지 않는다. 선택된 부모의 누락은 그 위치에서 거부하고 앞선 효과는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E27: 설정 일반 명령 이식으로 추가)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';

// 원문 등록값(54279c/54291c/542a10)이다. (2026-11-08 WB04-E27)
export const CHANGE_PAL_COMMAND = 0x02000029;
export const CHANGE_PRIORITY_COMMAND = 0x0200008a;
export const SET_FORT_TYPE_COMMAND = 0x020000c7;
// 수정키 제거 마스크이다. (2026-11-08 WB04-E27)
const KEY_MASK = 0x03ffffff;
// 원문 이름표(소문자)이며 추측 별칭이나 기본 메뉴 항목은 더하지 않는다. (2026-11-08 WB04-E27)
const COMMAND_NAMES = new Map([
    ['changepal', CHANGE_PAL_COMMAND],
    ['changepriority', CHANGE_PRIORITY_COMMAND],
    ['setforttype', SET_FORT_TYPE_COMMAND],
]);
// State 최소 바이트 수와 fort type BYTE 위치, SetFortType 진단 줄이다. (2026-11-08 WB04-E27)
const STATE_BYTES = 28;
const FORT_TYPE_OFFSET = 0x11;
const FORT_TYPE_ASSERT_LINE = 2012;

// 선택된 부모가 없으면 빈 성공으로 바꾸지 않고 거부한다. (2026-11-08 WB04-E27)
function need(device) {
    if (typeof device !== 'function') throw new Error('현재설정일반명령의 실제 부모 미연결');
    return device;
}

// 첫 NUL 앞만 소유한다(Config 알림이 공유 고리를 바꾸기 전에 요청을 확보). (2026-11-08 WB04-E27)
function untilNul(text) {
    const end = text.indexOf('\0');
    return end < 0 ? text : text.slice(0, end);
}

// 이름을 ASCII 대소문자 무시로 조회하고 미등록이면 0xffffffff 이다. (2026-11-08 WB04-E27)
export function configurationCommandScriptCode(name) {
    return COMMAND_NAMES.get(asciiLower(name)) ?? 0xffffffff;
}

// 상위 사건 플래그를 제외한 26비트 명령 전체가 세 명령 중 하나인지이다. (2026-11-08 WB04-E27)
export function isConfigurationCommand(code) {
    const value = (code & KEY_MASK) >>> 0;
    for (const command of COMMAND_NAMES.values()) if (value === command) return true;
    return false;
}

// 원문 요청(raw wanted)과 현재 OS 우선순위로 실제 설정 요청을 계산한다. 범위 밖 wanted 도 clamp 하지 않고 매 프레임 NORMAL 을 다시 요청하는 원문 효과를 유지한다. (2026-11-08 WB04-E27)
export function planNativeThreadPriority(wanted, currentOs) {
    // 현재 OS 값만 0/1/2 비교 등급으로 바꾼다.
    const currentLevel = (currentOs === 1 || currentOs === 2) ? 1 : currentOs === 15 ? 2 : 0;
    return { wanted, currentLevel, priority: wanted === 1 ? 1 : wanted === 2 ? 15 : 0, change: currentLevel !== wanted };
}

// event = { code, x, y }, reads = { battle(), challenge(), editor(), owner(), highPriority: { value } }(외부 효과 뒤 현재 값을 다시 읽는 전역),
// hooks = { text(token), set(key, value), get(key), palette(name), thread(priority), config(text), state(slot) → 28바이트 이상 Uint8Array, notify(slot), assertion(line) } 이다. 소비하면 true 이다. (2026-11-08 WB04-E27)
export function dispatchConfigurationCommandInput(event, reads, hooks) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (!isConfigurationCommand(code)) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    // 첫 번째 인자를 추측 사용하거나 낮은 WORD 로 자르지 않는다.
    const parameter = event.y >>> 0;
    if (code === CHANGE_PAL_COMMAND) {
        const key = need(reads.battle)() ? 'battlePal' : need(reads.challenge)() ? 'chalPal' : 'fortPal';
        const requested = untilNul(need(hooks.text)(parameter));
        need(hooks.set)(key, requested);
        const selected = untilNul(need(hooks.get)(key));
        need(hooks.palette)(selected);
    } else if (code === CHANGE_PRIORITY_COMMAND) {
        // OS 호출 실패 전에도 원문 공유 DWORD 쓰기는 이미 관측 가능하다.
        reads.highPriority.value = parameter | 0;
        need(hooks.thread)(parameter === 1 ? 1 : parameter === 2 ? 15 : 0);
        need(hooks.config)(`highPriorityValue = "${reads.highPriority.value | 0}"`);
    } else {
        if (!need(reads.editor)()) need(hooks.assertion)(FORT_TYPE_ASSERT_LINE);
        const state = need(hooks.state)(need(reads.owner)());
        if (!state || state.length < STATE_BYTES) throw new RangeError('SetFortType 의 실제 State 28BYTE 범위');
        state[FORT_TYPE_OFFSET] = parameter & 0xff;
        // State 생성/확장/진단에서 변경된 현재 Template owner 를 다시 조회한다.
        need(hooks.notify)(need(reads.owner)());
    }
    return true;
}
