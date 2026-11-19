// 브라우저 비문자 키를 원본 가상 키 사건으로 예약한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputQueue } from '../www/core/input-queue.js';
import { queueBrowserVirtualKeyEvent, queueBrowserCharacterEvent } from '../www/platform/browser-key-events.js';

// 방향·기능·보조 키, 누름/뗌과 문자·IME 제외를 실제 큐에서 확인한다.
test('브라우저 가상 키: 원본 코드·수정자·좌표와 문자 분리', () => {
    const queue = new InputQueue(), pointer = { x: -4, y: 90 };
    assert.equal(queueBrowserVirtualKeyEvent(queue, { code: 'ArrowRight', shiftKey: true, ctrlKey: true }, pointer, false), true);
    assert.deepEqual(queue.pop(false), { code: 0x30270000, x: -4, y: 90 });
    assert.equal(queueBrowserVirtualKeyEvent(queue, { code: 'F12', altKey: true }, pointer, true), true);
    assert.deepEqual(queue.pop(false), { code: 0x887b0000, x: -4, y: 90 });
    assert.equal(queueBrowserVirtualKeyEvent(queue, { code: 'ControlRight', ctrlKey: true }, pointer, false), true);
    assert.deepEqual(queue.pop(false), { code: 0x10110000, x: -4, y: 90 });
    assert.equal(queueBrowserVirtualKeyEvent(queue, { code: 'KeyA', key: 'a' }, pointer, false), false);
    assert.equal(queueBrowserVirtualKeyEvent(queue, { code: 'ArrowLeft', isComposing: true }, pointer, false), false);
    assert.equal(queueBrowserVirtualKeyEvent(queue, { code: 'F13' }, pointer, false), false);
    assert.equal(queue.empty, true); assert.throws(() => queueBrowserVirtualKeyEvent(null, {}, pointer, false), /큐/u);
});

// 문자 사건과 가상 키 사건을 분리하고 수정자·제어 키를 보존한다.
test('브라우저 문자 키: 영문·1252·제어 문자와 조합 제외', () => {
    const queue = new InputQueue(), pointer = { x: -1, y: 7 };
    assert.equal(queueBrowserCharacterEvent(queue, { code: 'KeyA', key: 'A', shiftKey: true }, pointer), true);
    assert.deepEqual(queue.pop(false), { code: 0x20000041, x: -1, y: 7 });
    assert.equal(queueBrowserCharacterEvent(queue, { code: 'KeyA', key: 'a', ctrlKey: true }, pointer), true);
    assert.equal(queue.pop(false).code, 0x10000001);
    assert.equal(queueBrowserCharacterEvent(queue, { code: 'KeyE', key: '€' }, pointer), true); assert.equal(queue.pop(false).code, 0x80);
    for (const [key, value] of [['Backspace', 8], ['Tab', 9], ['Enter', 13], ['Escape', 27]]) {
        assert.equal(queueBrowserCharacterEvent(queue, { key }, pointer), true); assert.equal(queue.pop(false).code, value);
    }
    assert.equal(queueBrowserCharacterEvent(queue, { key: '한', isComposing: true }, pointer), false);
    assert.equal(queueBrowserCharacterEvent(queue, { key: '한' }, pointer), false);
    assert.equal(queueBrowserCharacterEvent(queue, { key: 'ArrowRight' }, pointer), false);
    assert.equal(queue.empty, true);
});

// 앱 조합 상태는 중간 키를 삼키고 확정 한글을 원본 큐에 한 번만 넣는다.
test('브라우저 문자 키: 조합 시작·중간 키·확정 사건', async () => {
    const { Netstorm } = await import('../www/netstorm.js'); const app = new Netstorm();
    app._clientNativeInputEnabled = true; app._input.pointer.x = 12; app._input.pointer.y = 34;
    let activated = 0; app._activate = () => { activated += 1; };
    app._onCompositionStart(); app._onKeyDown({ code: 'Enter', key: 'Enter', isComposing: true });
    assert.equal(app._clientInputQueue.empty, true); assert.equal(activated, 0);
    app._onCompositionEnd({ data: '한글' });
    assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x0100d55c, x: 12, y: 34 });
    assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x0100ae00, x: 12, y: 34 });
    app._onCompositionEnd({ data: '한' }); assert.equal(app._clientInputQueue.empty, true);
    app._onCompositionStart(); app._clearInput(); app._onCompositionEnd({ data: '글' }); assert.equal(app._clientInputQueue.empty, true);
});

// 조합 확정 뒤 같은 beforeinput은 중복하지 않고 다음 같은 텍스트는 받는다.
test('브라우저 문자 키: 조합 후속과 독립 beforeinput 구별', async () => {
    const { Netstorm } = await import('../www/netstorm.js'); const app = new Netstorm(); app._clientNativeInputEnabled = true;
    app._onCompositionStart(); app._onCompositionEnd({ data: '한' }); let prevented = 0;
    const event = { data: '한', inputType: 'insertText', isComposing: false, preventDefault() { prevented += 1; } };
    app._onClientBeforeInput(event); app._onClientBeforeInput(event);
    assert.equal(prevented, 2); assert.equal(app._clientInputQueue.pop(false).code, 0x0100d55c);
    assert.equal(app._clientInputQueue.pop(false).code, 0x0100d55c); assert.equal(app._clientInputQueue.empty, true);
});

// 이전 조합과 다른 이벤트 턴에 같은 문자를 다시 넣으면 누락하지 않는다.
test('브라우저 문자 키: 조합 후 다음 턴의 같은 입력은 보존', async () => {
    const { Netstorm } = await import('../www/netstorm.js'); const app = new Netstorm(); app._clientNativeInputEnabled = true;
    app._onCompositionStart(); app._onCompositionEnd({ data: '한' });
    await new Promise((resolve) => setTimeout(resolve, 10));
    app._onClientBeforeInput({ data: '한', inputType: 'insertText', preventDefault() {} });
    assert.equal(app._clientInputQueue.pop(false).code, 0x0100d55c); assert.equal(app._clientInputQueue.pop(false).code, 0x0100d55c);
});
