// 원문 Pause 가상 키(004d8dd3..004d8df1)의 일반 Pause 명령 예약을 정지 조회와 분리하는 입력 분배 모듈이다(cpppj `client/PauseKeyInput.cpp/.h`, 10.78 전용).
// Guide/Gump/Console 을 지난 현재 일반 입력만 받는다. 키 누름 사건(뗌 비트 없음, 수정키 제거 후 가상 키 0x130000)이면 현재 Pause 명령 DWORD(00542a30)를 예약 직전에 조회해 일반 FIFO 에 (명령, 0, 0)으로 예약하고(00452fe0) true.
// 뗌이거나 다른 키/글자/명령은 소비하지 않는다(false, 장치를 요구하지 않음). Shift/Control/Alt/outside/pressed 비트는 거부하지 않으며 현재 정지는 읽지 않는다(다음 일반 Pause 명령이 소비 시점의 정지를 따른다).
// 조회 장치가 실행한 효과는 이후 예약 장치 누락 때도 보존한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E20: Pause 키 입력 이식으로 추가)
import { InputCode } from './input-queue.js';

// Pause 가상 키(0x13)를 16비트 왼쪽으로 민 값이다. (2026-11-08 WB04-E20)
const PAUSE_KEY = 0x130000;
const KEY_MASK = 0x03ffffff;

// event = { code, x, y }, hooks = { command() → 현재 Pause 명령, queue(command, 0, 0) }. 소비하면 true 이다. (2026-11-08 WB04-E20)
export function dispatchPauseKeyInput(event, hooks) {
    if ((event.code & InputCode.release) !== 0 || (event.code & KEY_MASK) !== PAUSE_KEY) return false;
    if (typeof hooks?.command !== 'function') throw new Error('Pause 키의 현재 명령 조회 미연결');
    const command = hooks.command();
    if (typeof hooks.queue !== 'function') throw new Error('Pause 키의 일반 입력 큐 미연결');
    hooks.queue(command, 0, 0);
    return true;
}
