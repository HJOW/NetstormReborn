// 브라우저 비문자 키 사건을 원본 가상 키 입력 큐 형식으로 옮긴다.
// 사용: queueBrowserVirtualKeyEvent(queue, keyboardEvent, pointer, released). 문자·IME 결과는 별도 경계가 처리한다.
import { InputCode } from '../core/input-queue.js';

// 문자로 변환되지 않는 탐색 키와 좌우를 공유하는 보조 키의 Win32 가상 키다.
const VIRTUAL_KEYS = new Map([
    ['PageUp', 0x21], ['PageDown', 0x22], ['End', 0x23], ['Home', 0x24],
    ['ArrowLeft', 0x25], ['ArrowUp', 0x26], ['ArrowRight', 0x27], ['ArrowDown', 0x28], ['Insert', 0x2d], ['Delete', 0x2e],
    ['ShiftLeft', 0x10], ['ShiftRight', 0x10], ['ControlLeft', 0x11], ['ControlRight', 0x11], ['AltLeft', 0x12], ['AltRight', 0x12],
]);

// 입력된 비문자 키와 수정자, 같은 현재 포인터 좌표를 원본 큐에 넣는다.
export function queueBrowserVirtualKeyEvent(queue, event, pointer, released) {
    if (typeof queue?.push !== 'function' || !event || !pointer) throw new TypeError('브라우저 가상 키의 큐·포인터 필요');
    if (event.isComposing || event.key === 'Dead') return false;
    let key = VIRTUAL_KEYS.get(event.code);
    if (!key && /^F(?:[1-9]|1[0-2])$/u.test(event.code ?? '')) key = 0x6f + Number(event.code.slice(1));
    if (!key) return false;
    let code = key << InputCode.virtualKeyShift;
    if (event.shiftKey) code |= InputCode.shift;
    if (event.ctrlKey) code |= InputCode.control;
    if (event.altKey) code |= InputCode.alt;
    if (released) code |= InputCode.release;
    queue.push(code >>> 0, pointer.x | 0, pointer.y | 0); return true;
}
