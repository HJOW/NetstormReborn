// 확정 본문의 일괄 예약이 실제 원본 40칸 고리 큐에서 원자적으로 동작한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputQueue } from '../www/core/input-queue.js';

// 고리 감김과 가득 찬 경우에도 이전 사건과 전체 순서를 보존한다.
test('입력 큐 일괄 예약: 감김·남은 칸·공간 부족과 검증 원자성', () => {
    const queue = new InputQueue(); assert.equal(queue.freeSlots, 39); assert.equal(queue.tryPushBatch([]), true);
    // 읽기와 쓰기를 움직여 다음 일괄 입력이 고리 끝에서 감기도록 한다.
    for (let i = 0; i < 35; i++) queue.push(i + 1, i, -i);
    for (let i = 0; i < 33; i++) queue.pop(false);
    const batch = Array.from({ length: 37 }, (_, index) => ({ code: index + 1000, x: index, y: -index }));
    assert.equal(queue.freeSlots, 37); assert.equal(queue.tryPushBatch(batch), true); assert.equal(queue.size, 39); assert.equal(queue.freeSlots, 0);
    assert.equal(queue.tryPushBatch([{ code: 5, x: 0, y: 0 }]), false); assert.equal(queue.size, 39);
    assert.equal(queue.pop(false).code, 34); assert.equal(queue.pop(false).code, 35);
    // 큐가 감겨도 일괄 입력의 순서는 최초 작성 순서와 같다.
    for (const event of batch) assert.deepEqual(queue.pop(false), event);
    assert.equal(queue.empty, true); queue.push(7, 0, 0);
    assert.throws(() => queue.tryPushBatch([{ code: 8, x: 0, y: 0 }, { code: -1, x: 0, y: 0 }]), RangeError);
    assert.equal(queue.size, 1); assert.equal(queue.pop(false).code, 7); assert.equal(queue.freeSlots, 39);
    assert.throws(() => queue.tryPushBatch(null), TypeError);
});
