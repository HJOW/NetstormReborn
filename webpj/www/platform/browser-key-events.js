// 브라우저 비문자 키 사건을 원본 가상 키 입력 큐 형식으로 옮긴다.
// 사용: queueBrowserVirtualKeyEvent(queue, keyboardEvent, pointer, released). 문자·IME 결과는 별도 경계가 처리한다.
import { InputCode } from '../core/input-queue.js';
import { encodeOriginalText } from '../assets/text-encoding.js';

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

// 브라우저 제어 키 중 원본 WM_CHAR에 들어가는 단일 바이트 값이다.
const CHARACTER_CONTROLS = new Map([['Backspace', 8], ['Tab', 9], ['Enter', 13], ['Escape', 27]]);

// 영문·Windows-1252 문자와 기본 제어 키를 문자 사건 하나로 예약한다.
export function queueBrowserCharacterEvent(queue, event, pointer) {
    if (typeof queue?.push !== 'function' || !event || !pointer) throw new TypeError('브라우저 문자 사건의 큐·포인터 필요');
    if (event.isComposing || event.key === 'Dead' || event.metaKey) return false;
    let value = CHARACTER_CONTROLS.get(event.key);
    if (value === undefined && typeof event.key === 'string' && [...event.key].length === 1) {
        if (event.ctrlKey && !event.altKey && /^Key[A-Z]$/u.test(event.code ?? '')) value = event.code.charCodeAt(3) - 64;
        else {
            let bytes;
            try { bytes = encodeOriginalText(event.key); } catch (error) { if (error instanceof RangeError) return false; throw error; }
            if (bytes.length !== 1) return false; value = bytes[0];
        }
    }
    if (value === undefined) return false;
    let code = value;
    if (event.shiftKey) code |= InputCode.shift;
    if (event.ctrlKey) code |= InputCode.control;
    if (event.altKey) code |= InputCode.alt;
    queue.push(code >>> 0, pointer.x | 0, pointer.y | 0); return true;
}
