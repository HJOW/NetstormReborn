// 브라우저가 보유한 포인터·보조 키 상태를 원본 Client 폴링 사건으로 변환한다.
// 사용: new BrowserInputPoll(inputGetter, layoutGetter, activeGetter).poll(code); 실제 눌림 사건은 recordMouseDown/recordKeyDown으로 기록한다.
import { InputCode } from '../core/input-queue.js';

// DOM 버튼 번호와 원본 Win32 가상 키 번호의 대응이다.
const MOUSE_KEYS = [1, 4, 2];
// 현재 브라우저 상태에서 실제 확인 가능한 좌·우 보조 키다.
const MODIFIERS = new Map([['ShiftLeft', 16], ['ShiftRight', 16], ['ControlLeft', 17], ['ControlRight', 17], ['AltLeft', 18], ['AltRight', 18]]);

// 매 폴링은 현재 입력과 화면을 다시 읽고 눌린 적 있음 비트만 해당 키별로 소비한다.
export class BrowserInputPoll {
    // 앱의 현재 입력·화면·활성 상태를 사본 없이 빌린다.
    #input; #layout; #active; #pressed = new Set();
    // 포인터 소유자를 재시작해도 현재 getter를 사용한다.
    constructor(input, layout, active) {
        if ([input, layout, active].some((value) => typeof value !== 'function')) throw new TypeError('브라우저 입력 폴링 소유자 필요');
        this.#input = input; this.#layout = layout; this.#active = active;
    }
    // DOM의 실제 버튼 누름을 원본의 해당 키별 눌린 적 있음으로 기록한다.
    recordMouseDown(button) { const key = MOUSE_KEYS[button]; if (key) this.#pressed.add(key); }
    // 최초 keydown만 기록하여 브라우저 자동 반복이 새 눌림으로 바뀌지 않게 한다.
    recordKeyDown(code, repeated) { const key = MODIFIERS.get(code); if (key && !repeated) this.#pressed.add(key); }
    // 원본 kept 마스크·release·pressedSince·outside와 화면에 제한된 좌표를 반환한다.
    poll(code) {
        if (!Number.isInteger(code) || code < 0 || code > 0xffffffff) throw new RangeError('브라우저 입력 폴링 코드 오류');
        const input = this.#input(), layout = this.#layout();
        if (!input?.pointer || !(input.keys instanceof Set) || !layout || !Number.isInteger(layout.logicalWidth) || !Number.isInteger(layout.logicalHeight)) throw new Error('브라우저 입력·화면 수명 누락');
        const kept = (code & 0x43ffffff) >>> 0, key = (kept >>> 16) & 0xffff, pointer = input.pointer;
        let down;
        if (key === 0) down = false;
        else if (key === 1) down = !!(pointer.buttons & 1);
        else if (key === 2) down = !!(pointer.buttons & 2);
        else if (key === 4) down = !!(pointer.buttons & 4);
        else if (key === 16) down = input.keys.has('ShiftLeft') || input.keys.has('ShiftRight');
        else if (key === 17) down = input.keys.has('ControlLeft') || input.keys.has('ControlRight');
        else if (key === 18) down = input.keys.has('AltLeft') || input.keys.has('AltRight');
        else throw new RangeError('브라우저에서 연결하지 않은 폴링 키');
        let result = kept;
        if (!down) result |= InputCode.release;
        if (this.#pressed.delete(key)) result |= InputCode.pressedSince;
        if (!pointer.inside || !this.#active()) result |= InputCode.outside;
        const x = Math.max(0, Math.min(layout.logicalWidth, pointer.x | 0));
        const y = Math.max(0, Math.min(layout.logicalHeight, pointer.y | 0));
        return { code: result >>> 0, x, y };
    }
}
