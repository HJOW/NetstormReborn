import test from 'node:test';
import assert from 'node:assert/strict';
import { QuickHelpPopupState } from '../www/ui/quick-help-popup-state.js';

// 표식은 NULL과 빈 문자열을 구분하고 길이 실패에서는 기존 물리를 보존한다.
test('Quick Help 팝업 표식은 명령과 현재 80바이트를 공유한다', () => {
    const popup = new QuickHelpPopupState(); popup.argument.fill(0x41);
    popup.mark(0x80000001, Uint8Array.of(66, 67, 0, 68));
    assert.equal(popup.command, 0x80000001); assert.deepEqual([...popup.argumentBytes()], [66, 67]); assert.equal(popup.argument[3], 0x41);
    popup.mark(2, new Uint8Array(0)); assert.equal(popup.argument[0], 0); assert.equal(popup.argument[1], 67);
    const before = popup.argument.slice(); assert.throws(() => popup.mark(3, new Uint8Array(80).fill(1)), /80바이트 초과/);
    assert.equal(popup.command, 2); assert.deepEqual(popup.argument, before);
    popup.mark(4, null); assert.ok(popup.argument.every((value) => value === 0));
    popup.argument.fill(1); assert.throws(() => popup.argumentBytes(), /NUL 누락/);
});
