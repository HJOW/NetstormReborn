// 문자 편집 Gump(TextEntryGump)의 경계 값(플래그 비트·NUL 절단·스크롤 delta·줄 길이/열 경계·선택 그림 경계)을 손 계산 값으로 고정한다. (2026-11-14 WB04-E439)
// 변형 확인에서 처음 살아남은 항목을 직접 입력으로 드러내는 시험이며 순서/경계는 기준선 소스(TextEntryGump.cpp)에서 옮겼다. 글 폭은 바이트당 8·높이 16, 현재 위치는 (10,20)이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { TextEntryGump, ConsoleGumpNode } from '../www/ui/text-entry-gump.js';
import { ScrollGumpNode } from '../www/ui/scroll-gump.js';
import { rig, built, cstr } from './text-entry-gump-support.mjs';

const SHIFT = 0x20000000;
const names = (log) => log.map((entry) => entry[0]);
const calls = (r, name, token) => r.log.filter((entry) => entry[0] === name && (token === undefined || entry[1] === token));
// 빈 본문에 text 를 넣고 캐럿을 at(기본 끝)에 두는 틀이다.
function typed(text, at = text.length, options = {}, width = 200, height = 60) { const r = built(options, '', width, height); r.buffer().insert(text); r.buffer().select(at, at); r.log.length = 0; return r; }
const selection = (r) => [r.buffer().state().selectionBegin, r.buffer().state().selectionEnd];

test('construct: 초기 본문은 첫 NUL 앞만이고 shift 기준을 0 으로 지우며 긴/여러 줄 본문에서도 시작 스크롤은 0,0 이다', () => {
    const nul = rig(); nul.root.shiftAnchor = 9; nul.gump.construct(1, 100, 40, '\0abc', 1, 2, 3); assert.equal(nul.buffer().text(), ''); assert.deepEqual([nul.root.shiftAnchor, nul.root.shiftEnd], [0, 0]);
    const none = rig(); none.root.shiftAnchor = 9; none.gump.construct(1, 100, 40, undefined, 1, 2, 3); assert.equal(none.root.shiftAnchor, 0);
    const tall = rig(); tall.gump.construct(1, 200, 32, `${'x'.repeat(60)}\n1\n2\n3\n4\n5`, 1, 2, 3);
    assert.deepEqual(calls(tall, 'sliderPosition').map((entry) => entry.slice(1)), [[201, 0], [200, 0]]); assert.deepEqual([tall.root.scrollLine, tall.root.scrollColumn], [0, 0]);
});

test('Console/Slider 조회: 훅이 없거나 같은 객체가 아니면 TypeError 가 아니라 Error 로 거부한다', () => {
    const r = rig({ console: true });
    assert.throws(() => new TextEntryGump(r.state, { ...r.hooks, console: undefined }).constructConsole(1, 100, 40, null, 1, 2, 3), { name: 'Error', message: /Console 공유 파생 노드/ });
    assert.throws(() => new TextEntryGump(r.state, { ...r.hooks, console: () => new ConsoleGumpNode() }).constructConsole(1, 100, 40, null, 1, 2, 3), { name: 'Error', message: /Console 공유 파생 노드/ });
    const b = built({}, 'abc');
    assert.throws(() => new TextEntryGump(b.state, { ...b.hooks, slider: undefined }).setScroll(1, 0, 0), { name: 'Error', message: /실제 Slider 없음/ });
    assert.throws(() => new TextEntryGump(b.state, { ...b.hooks, slider: () => new ScrollGumpNode() }).setScroll(1, 0, 0), { name: 'Error', message: /실제 Slider 없음/ });
});

test('measure: 줄/열 크기 측정은 길이 -1 로, caret 위치 측정은 명시 선택 길이로 부른다', () => {
    const r = typed('hello', 3); const lengths = []; r.hooks.measure = ((original) => (token, text, length) => { lengths.push([text.length, length]); return original(token, text, length); })(r.hooks.measure);
    const gump = new TextEntryGump(r.state, r.hooks); gump.setScroll(1, 0, 0); assert.deepEqual(lengths, [[2, -1]]);
    lengths.length = 0; r.root.blinkPhase = 1; gump.draw(1); assert.deepEqual(lengths, [[2, -1], [1024, -1], [1024, 3]]);
});

test('relativeScroll: 짝수 delta 도 그대로 더하고(줄/열 각각) 줄 수/열 clamp 이전 값을 Slider 에 전달한다', () => {
    const r = built({}, `${'x'.repeat(60)}\n1\n2\n3\n4\n5`, 200, 32); r.gump.relativeScroll(1, 2, 4);
    assert.deepEqual(calls(r, 'sliderPosition').map((entry) => entry.slice(1)), [[201, 4], [200, 2]]); assert.deepEqual([r.root.scrollLine, r.root.scrollColumn], [2, 0]);
    // 새 scrollLine 2 의 길이가 아니라 쓰기 전 scrollLine 0 의 길이(61)로 열을 clamp 했으므로 4 가 그대로 전달됐다.
});

test('setScroll: 줄이 줄 수와 같으면 마지막 줄(줄 수-1)로 올려 Slider 에 전달한다', () => {
    const r = built({}, 'a\nb\nc'); r.gump.setScroll(1, 3, 0); assert.deepEqual(calls(r, 'sliderPosition', 200)[0], ['sliderPosition', 200, 2]);
    const over = built({}, 'a\nb\nc'); over.gump.setScroll(1, 99, 0); assert.deepEqual(calls(over, 'sliderPosition', 200)[0], ['sliderPosition', 200, 2]);
    const under = built({}, 'a\nb\nc'); under.gump.setScroll(1, 2, 0); assert.deepEqual(calls(under, 'sliderPosition', 200)[0], ['sliderPosition', 200, 2]);
});

test('setExcluded: 성공하면 assertion 을 부르지 않고 선두/단독 LF 도 지우며 LF 사이 글자를 잃지 않는다', () => {
    const r = built({}); r.gump.setExcluded(1, '\nabc'); assert.equal(cstr(r.storage.bytes(r.root.excludedChars)), 'abc'); assert.equal(calls(r, 'assertion').length, 0);
    r.gump.setExcluded(1, 'a\nb\nc'); assert.equal(cstr(r.storage.bytes(r.root.excludedChars)), 'abc'); r.gump.setExcluded(1, '\n\n'); assert.equal(cstr(r.storage.bytes(r.root.excludedChars)), '');
});

test('setHorizontal/idle: void 비트(8)만 void 로 보고 표시 비트(1)는 void 가 아니며, 표시 비트만 있는 노드도 점멸한다', () => {
    const r = built({}); r.root.x = 3; r.root.y = 4; r.root.flags = 1; r.gump.setHorizontal(1, 1); assert.deepEqual(r.log, [['push', 1], ['attach', 1, 3, 4, 0, 0]]);
    const idle = built({}); idle.root.flags |= 1; idle.root.blinkAt = 0; idle.gump.idle(1); assert.equal(idle.root.blinkPhase, 1); assert.deepEqual(idle.log, [['redraw', 1, false]]);
    const dead = built({}); dead.root.flags |= 0x10000; dead.root.blinkAt = 0; dead.gump.idle(1); assert.equal(dead.root.blinkPhase, 0);
});

test('handle 키 경계: 위 Right 는 한 칸, maxLength 1 은 입력 불가, 0x10001/0x20001 은 특수 코드가 아니라 일반 입력 처리(redraw)다', () => {
    const r = typed('hello', 1); r.gump.handle(1, 0x270000, 0, 0); assert.deepEqual(selection(r), [2, 2]);
    const m = typed('', 0); m.root.maxLength = 1; m.gump.handle(1, 0x61, 0, 0); assert.equal(m.buffer().text(), ''); m.root.maxLength = 2; m.gump.handle(1, 0x61, 0, 0); assert.equal(m.buffer().text(), 'a');
    for (const code of [0x10001, 0x20001]) { const k = typed('ab'); assert.equal(k.gump.handle(1, code, 0, 0), 0, code.toString(16)); assert.deepEqual(k.log, [['redraw', 1, false]]); }
});

test('handle 스크롤 delta: End/Home/단계 사건의 줄 delta 는 0 이고 열 delta 만 쓰며 단계 4 는 -M 폭이다', () => {
    const text = `${'x'.repeat(60)}\n1\n2\n3\n4\n5`;
    const end = typed(text, 0, {}, 200, 32); end.root.scrollLine = 2; end.gump.handle(1, 0x230000, 0, 0); assert.deepEqual(calls(end, 'sliderPosition', 200)[0], ['sliderPosition', 200, 2]);
    const shiftEnd = typed(text, 0, {}, 200, 32); shiftEnd.root.scrollLine = 2; shiftEnd.gump.handle(1, 0x230000 | SHIFT, 0, 0); assert.deepEqual(calls(shiftEnd, 'sliderPosition', 200)[0], ['sliderPosition', 200, 2]);
    const home = typed(text, 5, {}, 200, 32); home.root.scrollLine = 2; home.root.scrollColumn = 7; home.gump.handle(1, 0x240000, 0, 0); assert.deepEqual(calls(home, 'sliderPosition', 200)[0], ['sliderPosition', 200, 2]);
    const step = typed(text, 0, {}, 200, 32); step.root.scrollLine = 2; step.gump.handle(1, 0x0200002e, 201, 1); assert.deepEqual(calls(step, 'sliderPosition', 200)[0], ['sliderPosition', 200, 2]);
    const left = typed('x'.repeat(60), 0, {}, 200, 60); left.root.scrollColumn = 10; left.gump.handle(1, 0x0200002e, 201, 4); assert.deepEqual(calls(left, 'sliderPosition', 201)[0], ['sliderPosition', 201, 2]);
    const right = typed('x'.repeat(60), 0, {}, 200, 60); right.root.scrollColumn = 10; right.gump.handle(1, 0x0200002e, 201, 3); assert.deepEqual(calls(right, 'sliderPosition', 201)[0], ['sliderPosition', 201, 18]);
});

test('handleConsole Tab: 교체 범위는 단어 시작..시작+길이(짝수 끝 포함, 뒤 글자는 보존)이고 단어 구분은 ASCII 공백(9~13)만이다', () => {
    const r = typed('a wo xyz', 4, { console: true, complete: () => 'world' }); r.root.autocomplete = 5; r.root.resetWord = 1; r.gump.handleConsole(1, 9, 0, 0); assert.equal(r.buffer().text(), 'a world xyz'); assert.deepEqual([r.root.wordStart, r.root.wordLength], [2, 5]);
    for (const [text, word] of [['a\rb', 'b'], ['a\x0eb', 'a\x0eb'], ['a\x08b', 'a\x08b'], ['a\x0bb', 'b'], ['a\x0cb', 'b']]) {
        const x = typed(text, text.length, { console: true, complete: () => null }); x.root.autocomplete = 9; x.root.resetWord = 1; x.gump.handleConsole(1, 9, 0, 0); assert.deepEqual(calls(x, 'complete')[0], ['complete', 9, word, word.length, false], JSON.stringify(text));
    }
});

test('draw 높이 관문: 정수 최솟값 높이나 줄높이 -1 은 한쪽만이면 오류가 아니다', () => {
    const min = typed('abc', 0); min.root.baseHeight = -2147483648; new TextEntryGump(min.state, min.hooks).draw(1); assert.deepEqual(min.log, [['drawBackground', 1]]);
    const negative = typed('abc', 0); negative.hooks.measure = () => [8, -1]; new TextEntryGump(negative.state, negative.hooks).draw(1); assert.deepEqual(negative.log, [['drawBackground', 1]]);
});

test('draw 줄 저장소: 1023바이트 줄은 그리고 1024바이트 줄은 거부하며, 앞 줄의 글자가 빈 줄에 남지 않는다', () => {
    const ok = typed('x'.repeat(1023), 0); ok.gump.draw(1); assert.equal(calls(ok, 'drawText')[0][1].length, 1023);
    const long = typed('x'.repeat(1024), 0); assert.throws(() => long.gump.draw(1), { name: 'Error', message: /줄 복사 범위 거부/ });
    const stale = typed('abc\n', 0); stale.gump.draw(1); assert.deepEqual(calls(stale, 'drawText'), [['drawText', 'abc ', 0, 0, null], ['drawText', ' ', 0, 16, null]]);
});

test('draw 열 스크롤: 1 칸도 자르고 길이와 같으면 빈 줄이 되며 길이를 넘으면 자르지 않는다', () => {
    const one = typed('hello', 0); one.root.scrollColumn = 1; one.gump.draw(1); assert.deepEqual(calls(one, 'drawText'), [['drawText', 'ello', 0, 0, null]]);
    const same = typed('abc', 0); same.root.scrollColumn = 3; same.gump.draw(1); assert.deepEqual(calls(same, 'drawText'), [['drawText', '', 0, 0, null]]);
    const beyond = typed('abc', 0); beyond.root.scrollColumn = 4; beyond.gump.draw(1); assert.deepEqual(calls(beyond, 'drawText'), [['drawText', 'abc', 0, 0, null]]);
});

test('draw 수신 대상: 줄 맨 앞의 콜론도 강조 경로를 타고(빈 이름 검사) 선택은 1칸 앞/짝수 길이 끝까지의 경계도 맞는다', () => {
    const colon = typed(':hi', 0, { console: true }); colon.root.recipientHighlight = 1; colon.gump.draw(1);
    assert.deepEqual(colon.log, [['drawBackground', 1], ['recipient', ''], ['drawText', ':', 0, 0, null], ['drawText', 'hi', 8, 0, null]]);
    const early = typed('hello world', 0); early.buffer().select(1, 3); early.gump.draw(1); assert.deepEqual(early.log, [['drawBackground', 1], ['drawText', 'h', 0, 0, null], ['fill', 18, 20, 34, 36, 0x5566], ['drawText', 'el', 8, 0, 0x88], ['drawText', 'lo world', 24, 0, null]]);
    const end = typed('hello!', 0); end.buffer().select(2, 6); end.gump.draw(1); assert.deepEqual(end.log, [['drawBackground', 1], ['drawText', 'he', 0, 0, null], ['fill', 26, 20, 58, 36, 0x5566], ['drawText', 'llo!', 16, 0, 0x88]]);
});

test('draw caret: 선택이 없는 줄(-1,-1)에는 그리지 않아 여러 줄에서도 caret 한 줄만 그린다', () => {
    const r = typed('ab\ncd\nef', 4); r.root.blinkPhase = 1; r.gump.draw(1); assert.equal(names(r.log).filter((name) => name === 'fill').length, 1);
});

test('handleConsole: 특수 코드 0x10000/0x20000 은 입력 블록을 건너뛰어 단어 재설정을 켜지 않는다', () => {
    for (const code of [0x10000, 0x20000]) { const r = typed('x', 1, { console: true }); r.root.resetWord = 0; assert.equal(r.gump.handleConsole(1, code, 0, 0), 0, code.toString(16)); assert.equal(r.root.resetWord, 0, code.toString(16)); assert.deepEqual(r.log, []); }
});

test('draw caret: 선택 구간이 있는 줄(시작≠끝)에는 위상이 홀수여도 caret 을 더 그리지 않는다', () => {
    const r = typed('hello world', 0); r.buffer().select(2, 7); r.root.blinkPhase = 1; r.gump.draw(1); assert.equal(names(r.log).filter((name) => name === 'fill').length, 1);
});
