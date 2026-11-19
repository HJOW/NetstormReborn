// 브라우저 포인터 버튼 사건을 원본 입력 큐의 raw 코드와 signed 좌표로 변환한다.
// 사용: queueBrowserMouseEvent(queue, event, pointer, released). 이동·취소와 미지원 버튼은 이 함수의 대상이 아니다.
import { InputCode } from '../core/input-queue.js';

// DOM 좌·중·우 버튼 번호에 대응하는 원본 입력 코드다.
const BUTTON_CODES = [InputCode.leftButton, InputCode.middleButton, InputCode.rightButton];

// 브라우저 사건의 보조 키와 논리 좌표를 보존해 같은 40칸 큐에 넣는다.
export function queueBrowserMouseEvent(queue, event, pointer, released) {
    if (typeof queue?.push !== 'function' || !event || !pointer) throw new TypeError('브라우저 버튼 사건의 큐·포인터 필요');
    const button = BUTTON_CODES[event.button]; if (!button) return false;
    let code = button;
    if (event.shiftKey) code |= InputCode.shift;
    if (event.ctrlKey) code |= InputCode.control;
    if (event.altKey) code |= InputCode.alt;
    if (released) code |= InputCode.release;
    queue.push(code >>> 0, pointer.x | 0, pointer.y | 0); return true;
}
