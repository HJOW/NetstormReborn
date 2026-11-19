// 확정 문자 사건은 원본 표시 바이트와 같은 Client 큐에 원자적으로 들어간다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputQueue } from '../www/core/input-queue.js';
import { displayTextCodec } from '../www/assets/display-text.js';
import { isCommittedTextInput, committedTextBytes, makeCommittedTextEvents, queueCommittedText, hasExtendedDisplayText, displayTextFloor, displayTextCeil, previousDisplayText, nextDisplayText } from '../www/core/display-text-input.js';

// ASCII/1252는 바이트 하나, 확장 문자는 한 태그이며 해당 바이트로 복원한다.
test('확정 문자: 영어·1252·한국어 태그와 표시 바이트', () => {
    const events = makeCommittedTextEvents('Aé€한', -3, 5);
    assert.deepEqual(events, [0x41, 0xe9, 0x80, 0x0100d55c].map((code) => ({ code, x: -3, y: 5 })));
    assert.equal(isCommittedTextInput(events[3].code), true); assert.equal(isCommittedTextInput(events[0].code), false);
    assert.deepEqual(committedTextBytes(events[3].code), displayTextCodec.encode('한'));
    assert.throws(() => committedTextBytes(0x01000041), /미지원/u); assert.throws(() => committedTextBytes(65), /태그/u);
    assert.throws(() => makeCommittedTextEvents('a\nb'), /제어문자/u); assert.throws(() => makeCommittedTextEvents('😀'), /BMP/u);
    assert.throws(() => makeCommittedTextEvents('\ud800'), /BMP/u);
});

// 잘못된 본문과 공간 부족은 이전 큐를 그대로 보존한다.
test('확정 문자: 전체 검증과 용량 부족의 원자성', () => {
    const queue = new InputQueue(); queue.push(7, 0, 0);
    assert.throws(() => queueCommittedText(queue, 'ok\0', 1, 2), /제어문자/u); assert.equal(queue.size, 1);
    assert.equal(queueCommittedText(queue, '한글'.repeat(20)), false); assert.equal(queue.size, 1);
    assert.equal(queue.pop(false).code, 7); assert.equal(queueCommittedText(queue, '한A', 1, 2), true);
    assert.deepEqual(queue.pop(false), { code: 0x0100d55c, x: 1, y: 2 }); assert.deepEqual(queue.pop(false), { code: 65, x: 1, y: 2 });
    assert.equal(queue.empty, true); assert.throws(() => queueCommittedText(null, 'a'), /큐/u);
});

// 확장 바이트 내부의 커서와 선택을 한글 한 문자 경계로 유지한다.
test('확정 문자: 혼합 바이트 선택·이전·다음 경계', () => {
    const bytes = displayTextCodec.encode('A한글B'); assert.equal(bytes.length, 10); assert.equal(hasExtendedDisplayText(bytes), true);
    assert.deepEqual([0, 1, 2, 4, 5, 8, 9, 10, 11].map((at) => displayTextFloor(bytes, at)), [0, 1, 1, 1, 5, 5, 9, 10, 10]);
    assert.deepEqual([0, 1, 2, 4, 5, 8, 9, 10, 11].map((at) => displayTextCeil(bytes, at)), [0, 1, 5, 5, 5, 9, 9, 10, 10]);
    assert.deepEqual([previousDisplayText(bytes, 9), previousDisplayText(bytes, 5), nextDisplayText(bytes, 1), nextDisplayText(bytes, 5)], [5, 1, 5, 9]);
    const raw = Array.from(bytes, (byte) => String.fromCharCode(byte)).join(''); assert.equal(nextDisplayText(raw, 1), 5);
    assert.equal(hasExtendedDisplayText(Uint8Array.of(31, 255, 255, 255)), false); assert.equal(displayTextFloor(Uint8Array.of(31, 255, 255, 255), 2), 2);
    assert.throws(() => displayTextFloor(bytes, -1), /위치/u);
});
