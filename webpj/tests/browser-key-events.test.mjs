// 브라우저 비문자 키를 원본 가상 키 사건으로 예약한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputQueue } from '../www/core/input-queue.js';
import { queueBrowserVirtualKeyEvent } from '../www/platform/browser-key-events.js';

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
