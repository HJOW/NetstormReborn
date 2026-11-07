// C/c 키가 현재 회전 방향 DWORD 를 먼저 반전하고 같은 Config 에 기록하는 원문 입력(004d96b6, 004d97a0..004d97ce)을 옮긴 모듈이다(cpppj `client/RotationKeyInput.cpp/.h`, 10.78 전용).
// 수정키를 제거한(0x03ffffff) 키가 'C'/'c' 이고 뗌이 아니면 소비한다: 정지 판정(paused, 00460de0)이 참이면 효과 없이 소비하고, 아니면 현재 회전 방향 DWORD(rotation.mode)를 0↔1 로 반전해 쓴 뒤
// 같은 반전값으로 할당문 'rotmode=<값>'을 Config(00441580)에 기록한다(Config 가 현재 값을 바꿔도 서식 인자는 앞서 저장한 반전값). 설정 실패 때도 앞선 DWORD 쓰기를 되돌리지 않는다.
// 뗌/다른 키/가상 키/명령은 소비하지 않는다(false). rotation 은 { mode } 속성을 가진 현재 전역 대여이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E20: 회전 방향 키 이식으로 추가)
import { InputCode } from './input-queue.js';

// 수정키 제거 마스크와 'C', 'c' 코드이다. (2026-11-08 WB04-E20)
const KEY_MASK = 0x03ffffff;
const UPPER_C = 0x43;
const LOWER_C = 0x63;

// event = { code, x, y }, rotation = { mode }, hooks = { paused() → bool, configure(문자열) }. (2026-11-08 WB04-E20)
export function dispatchRotationKeyInput(event, rotation, hooks) {
    const key = (event.code & KEY_MASK) >>> 0;
    if ((event.code & InputCode.release) !== 0 || (key !== UPPER_C && key !== LOWER_C)) return false;
    if (typeof hooks?.paused !== 'function') throw new Error('회전 방향 키의 실제 정지 판정 미연결');
    if (hooks.paused()) return true;
    // Config 가 현재값을 바꿔도 서식 인자는 이 앞서 저장한 반전값이다.
    const selected = rotation.mode === 0 ? 1 : 0;
    rotation.mode = selected;
    if (typeof hooks.configure !== 'function') throw new Error('회전 방향 키의 실제 Config 쓰기 미연결');
    hooks.configure(`rotmode=${selected}`);
    return true;
}
