// 브라우저 버튼 사건을 원본 큐 코드·좌표로 변환하는 경계를 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputQueue } from '../www/core/input-queue.js';
import { browserMouseEvent, queueBrowserMouseEvent } from '../www/platform/browser-mouse-events.js';

// 좌·중·우의 raw 코드와 보조 키·signed 화면 밖 좌표를 보존한다.
test('브라우저 버튼 큐: 세 버튼과 release·보조 키·음수 좌표', () => {
    const queue = new InputQueue(), pointer = { x: -8, y: 1100 };
    assert.equal(queueBrowserMouseEvent(queue, { button: 0, shiftKey: true, ctrlKey: true, altKey: true }, pointer, false), true);
    assert.deepEqual(queue.pop(false), { code: 0x38010000, x: -8, y: 1100 });
    assert.equal(queueBrowserMouseEvent(queue, { button: 1 }, pointer, true), true);
    assert.deepEqual(queue.pop(false), { code: 0x80040000, x: -8, y: 1100 });
    assert.equal(queueBrowserMouseEvent(queue, { button: 2 }, pointer, true), true);
    assert.deepEqual(queue.pop(false), { code: 0x80020000, x: -8, y: 1100 });
    assert.equal(queueBrowserMouseEvent(queue, { button: 3 }, pointer, false), false); assert.equal(queue.empty, true);
    assert.deepEqual(browserMouseEvent({ button: 0, shiftKey: true, ctrlKey: true, altKey: true }, pointer, false), { code: 0x38010000, x: -8, y: 1100 });
    assert.equal(browserMouseEvent({ button: 3 }, pointer, false), null);
    assert.throws(() => queueBrowserMouseEvent(null, {}, pointer, false), /큐/u);
});
