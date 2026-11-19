// 원본형 편집기가 확정 한글을 삽입하고 토큰을 깨지 않는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { built } from './text-entry-gump-support.mjs';
import { InputQueue } from '../www/core/input-queue.js';
import { displayTextCodec } from '../www/assets/display-text.js';
import { queueCommittedText } from '../www/core/display-text-input.js';

// 실제 큐 사건을 원본형 편집기로 전달한다.
function enter(queue, gump) { while (!queue.empty) { const event = queue.pop(false); gump.handle(1, event.code, event.x, event.y); } }
// 현재 바이트 본문을 같은 표시 codec으로 읽는다.
function text(r) { return displayTextCodec.decode(Uint8Array.from(r.buffer().text(), (character) => character.charCodeAt(0))); }

// 혼합 본문의 이동·선택·삭제는 한글 토큰을 한 문자로 처리한다.
test('TextEntry 확정 한글: 실제 큐 삽입과 이동·선택·삭제', () => {
    const r = built({}, ''), queue = new InputQueue();
    assert.equal(queueCommittedText(queue, 'A한글B'), true); enter(queue, r.gump);
    assert.equal(text(r), 'A한글B'); assert.equal(r.buffer().state().selectionBegin, 10);
    r.gump.handle(1, 0x250000, 0, 0); assert.equal(r.buffer().state().selectionBegin, 9);
    r.gump.handle(1, 0x250000, 0, 0); assert.equal(r.buffer().state().selectionBegin, 5);
    r.gump.handle(1, 8, 0, 0); assert.equal(text(r), 'A글B'); assert.equal(r.buffer().state().selectionBegin, 1);
    r.gump.handle(1, 0x2e0000, 0, 0); assert.equal(text(r), 'AB');
    assert.equal(queueCommittedText(queue, '한글'), true); enter(queue, r.gump); assert.equal(text(r), 'A한글B');
    r.buffer().select(2, 3); assert.equal(queueCommittedText(queue, '가'), true); enter(queue, r.gump);
    assert.equal(text(r), 'A가글B'); assert.deepEqual([r.buffer().state().selectionBegin, r.buffer().state().selectionEnd], [5, 5]);
});

// 확장 문자는 물리 바이트 제한과 제외 목록을 전체 토큰에 적용한다.
test('TextEntry 확정 한글: 바이트 한도·선택 대체·제외', () => {
    const r = built({}, ''), queue = new InputQueue(); r.root.maxLength = 5;
    assert.equal(queueCommittedText(queue, '한'), true); enter(queue, r.gump); assert.equal(text(r), '한');
    const event = 0x0100ae00; assert.equal(r.gump.handle(1, event, 0, 0), 0); assert.equal(text(r), '한');
    r.buffer().select(0, 4); assert.equal(r.gump.handle(1, event, 0, 0), 1); assert.equal(text(r), '글');
    r.gump.setExcluded(1, String.fromCharCode(31)); r.buffer().select(0, 4);
    assert.equal(r.gump.handle(1, 0x0100d55c, 0, 0), 0); assert.equal(text(r), '글');
});

// Shift 선택과 일반 바이트 대체도 확장 토큰의 물리 경계를 보존한다.
test('TextEntry 확정 한글: Shift 선택과 일반 문자 대체', () => {
    const r = built({}, ''), queue = new InputQueue(); queueCommittedText(queue, 'A한글B'); enter(queue, r.gump);
    r.buffer().select(9, 9); r.gump.handle(1, 0x20250000, 0, 0);
    assert.deepEqual([r.buffer().state().selectionBegin, r.buffer().state().selectionEnd], [5, 9]);
    r.gump.handle(1, 0x20250000, 0, 0); assert.deepEqual([r.buffer().state().selectionBegin, r.buffer().state().selectionEnd], [1, 9]);
    r.gump.handle(1, 0x20270000, 0, 0); assert.deepEqual([r.buffer().state().selectionBegin, r.buffer().state().selectionEnd], [5, 9]);
    r.root.maxLength = 11; assert.equal(r.gump.handle(1, 0x58, 0, 0), 1); assert.equal(text(r), 'A한XB');
});

// 클립보드의 바이트 상한·제외문자를 확장 토큰 전체에 적용한다.
test('TextEntry 확정 한글: 붙여넣기 한도·제외·선택 보존', () => {
    const raw = (value) => Array.from(displayTextCodec.encode(value), (byte) => String.fromCharCode(byte)).join('');
    const limited = built({ clipboard: raw('한글B') }, 'A'); limited.buffer().select(1, 1); limited.root.maxLength = 6;
    limited.gump.handle(1, 0x202d0000, 0, 0); assert.equal(text(limited), 'A한');
    const blocked = built({ clipboard: raw('한글B') }, 'A'); blocked.buffer().select(1, 1); blocked.gump.setExcluded(1, String.fromCharCode(31));
    blocked.gump.handle(1, 0x202d0000, 0, 0); assert.equal(text(blocked), 'AB');
    const existing = built({ clipboard: 'abcd' }, ''); const queue = new InputQueue(); queueCommittedText(queue, '한'); enter(queue, existing.gump);
    existing.root.maxLength = 7; existing.gump.handle(1, 0x202d0000, 0, 0); assert.equal(text(existing), '한ab');
    const none = built({ clipboard: raw('한') }, 'A'); none.buffer().select(0, 1); none.root.maxLength = 4;
    none.gump.handle(1, 0x202d0000, 0, 0); assert.equal(text(none), 'A'); assert.deepEqual([none.buffer().state().selectionBegin, none.buffer().state().selectionEnd], [0, 1]);
});

// 실제 Slider가 확정한 한글 줄의 수평 열이 토큰 내부에 남지 않는다.
test('TextEntry 확정 한글: 수평 스크롤을 현재 줄 문자 경계로 보정', () => {
    const r = built({}, '', 8, 16), queue = new InputQueue(); queueCommittedText(queue, 'A한글B'); enter(queue, r.gump); r.log.length = 0;
    r.gump.setScroll(1, 0, 3); assert.equal(r.root.scrollColumn, 1); assert.equal(r.sliderOf(201).position, 1);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), [['sliderPosition', 201, 3], ['sliderPosition', 201, 1]]);
});

// 실제 줄 복사·스크롤 뒤 선택과 캐럿이 완전한 한글 바이트를 유지한다.
test('TextEntry 확정 한글: 수평 잘라내기 뒤 선택과 화면 밖 선택', () => {
    const r = built({}, ''), queue = new InputQueue(); queueCommittedText(queue, 'A한글B'); enter(queue, r.gump);
    const decode = (value) => displayTextCodec.decode(Uint8Array.from(value, (character) => character.charCodeAt(0)));
    r.buffer().select(5, 9); r.root.scrollColumn = 2; r.log.length = 0; r.gump.draw(1);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'drawText').map((entry) => decode(entry[1])), ['한', '글', 'B']);
    assert.equal(r.log.filter((entry) => entry[0] === 'fill').length, 1);
    r.buffer().select(0, 1); r.root.scrollColumn = 1; r.root.blinkPhase = 1; r.log.length = 0; r.gump.draw(1);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'drawText').map((entry) => decode(entry[1])), ['한글B']);
    assert.equal(r.log.filter((entry) => entry[0] === 'fill').length, 0);
});
