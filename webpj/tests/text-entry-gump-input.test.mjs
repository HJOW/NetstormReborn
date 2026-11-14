// 문자 편집 Gump(TextEntryGump)의 입력 처리(handle)·Console 자동완성/제출(handleConsole)·그리기(draw)를 기준선 소스 계약과 손 계산 값으로 검증한다. (2026-11-14 WB04-E439)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(TextEntryGump.cpp)에서 옮겼다. 글 폭은 바이트당 8·높이 16, 현재 위치는 (10,20)이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { TextEntryGump } from '../www/ui/text-entry-gump.js';
import { rig, built, cstr } from './text-entry-gump-support.mjs';

const SHIFT = 0x20000000; const CONTROL = 0x10000000; const RELEASE = 0x80000000; const COMMAND = 0x02000000;
const names = (log) => log.map((entry) => entry[0]);
// 빈 본문에 text 를 넣고 캐럿을 at(기본 끝)에 두는 틀이다. 구성 뒤 shift 기준은 0 이다.
function typed(text, at = text.length, options = {}, width = 200, height = 60) { const r = built(options, '', width, height); r.buffer().insert(text); r.buffer().select(at, at); r.log.length = 0; return r; }
const selection = (r) => [r.buffer().state().selectionBegin, r.buffer().state().selectionEnd];

test('handle 문자: 일반 문자는 선택을 교체해 넣고 변경 통지→Redraw→입력 전달 순으로 알리며 반환 1 이다', () => {
    const r = typed('ab'); assert.equal(r.gump.handle(1, 0x43, 7, 8), 1); assert.equal(r.buffer().text(), 'abC'.slice(0, 2) + 'C'); assert.deepEqual(r.log, [['redraw', 1, false]]);
    r.root.changeNotice = 1; r.root.forwardInput = 1; r.log.length = 0; assert.equal(r.gump.handle(1, 0x44 | SHIFT, 7, 8), 1);
    assert.equal(r.buffer().text(), 'abCD'); assert.deepEqual(r.log, [['bubble', 1, 0x020000bf, 0, 0], ['redraw', 1, false], ['bubble', 1, 0x44 | SHIFT, 7, 8]]);
    // 선택은 입력 문자로 대체된다.
    r.buffer().select(0, 4); r.gump.handle(1, 0x5a, 0, 0); assert.equal(r.buffer().text(), 'Z');
});

test('handle 문자: 뗌/명령/코드 0x10000·0x20000/읽기 전용은 입력 블록을 건너뛰어 반환 0 이고, 제어 문자·0x100 이상은 반환 0 이며 통지 없이 Redraw 만 한다', () => {
    const r = typed('ab'); for (const event of [0x43 | RELEASE, 0x43 | COMMAND, 0x10000, 0x20000]) { assert.equal(r.gump.handle(1, event >>> 0, 0, 0), 0, event.toString(16)); assert.deepEqual(r.log, []); }
    r.root.readOnly = 1; assert.equal(r.gump.handle(1, 0x43, 0, 0), 0); assert.equal(r.buffer().text(), 'ab'); assert.deepEqual(r.log, []); r.root.readOnly = 0;
    r.root.changeNotice = 1; r.root.forwardInput = 1; for (const code of [0x01, 0x1f, 0x100, 0x120]) { r.log.length = 0; assert.equal(r.gump.handle(1, code, 5, 6), 0, String(code)); assert.deepEqual(r.log, [['redraw', 1, false], ['bubble', 1, code, 5, 6]]); }
    assert.equal(r.buffer().text(), 'ab');
    // 0x20 과 0xff 는 문자다.
    r.log.length = 0; r.root.changeNotice = 0; r.root.forwardInput = 0; assert.equal(r.gump.handle(1, 0x20, 0, 0), 1); assert.equal(r.gump.handle(1, 0xff, 0, 0), 1); assert.equal(r.buffer().text(), 'ab \xff');
});

test('handle 문자: maxLength 가 양수면 현재 length+1 이 엄격히 작을 때만 넣고(선택 길이를 빼지 않음) 못 넣어도 반환 1·변경 통지는 보낸다', () => {
    const r = typed('abc'); r.root.maxLength = 4; r.root.changeNotice = 1;
    assert.equal(r.gump.handle(1, 0x64, 0, 0), 1); assert.equal(r.buffer().text(), 'abc'); assert.deepEqual(r.log, [['bubble', 1, 0x020000bf, 0, 0], ['redraw', 1, false]]);
    // 전체 선택이어도 length+1 로 판정한다.
    r.buffer().select(0, 3); r.gump.handle(1, 0x64, 0, 0); assert.equal(r.buffer().text(), 'abc');
    r.root.maxLength = 5; r.buffer().select(3, 3); r.gump.handle(1, 0x64, 0, 0); assert.equal(r.buffer().text(), 'abcd'); r.gump.handle(1, 0x65, 0, 0); assert.equal(r.buffer().text(), 'abcd');
    r.root.maxLength = -3; r.gump.handle(1, 0x66, 0, 0); assert.equal(r.buffer().text(), 'abcdf'); r.root.maxLength = 0; r.gump.handle(1, 0x67, 0, 0); assert.equal(r.buffer().text(), 'abcdfg');
});

test('handle 문자: 제외문자는 입력하지 않고 반환 0·변경 통지 없이 Redraw 하며 LF 는 제외 목록에서 지워진다', () => {
    const r = typed('ab'); r.gump.setExcluded(1, 'x\ny'); r.root.changeNotice = 1; r.log.length = 0;
    assert.equal(r.gump.handle(1, 0x78, 0, 0), 0); assert.equal(r.gump.handle(1, 0x79, 0, 0), 0); assert.equal(r.buffer().text(), 'ab'); assert.deepEqual(names(r.log), ['redraw', 'redraw']);
    assert.equal(r.gump.handle(1, 0x61, 0, 0), 1); assert.equal(r.buffer().text(), 'aba'); assert.deepEqual(names(r.log).slice(2), ['bubble', 'redraw']);
});

test('handle 이동 키: Left/Right 는 shift 없이 선택이 있어도 begin-1/end+1 한 칸이고 shift 는 첫 키에서만 기준을 잡아 선택을 늘린다', () => {
    const r = typed('hello', 5); r.gump.handle(1, 0x250000, 0, 0); assert.deepEqual(selection(r), [4, 4]);
    r.buffer().select(2, 4); r.gump.handle(1, 0x250000, 0, 0); assert.deepEqual(selection(r), [1, 1]); r.buffer().select(2, 4); r.gump.handle(1, 0x270000, 0, 0); assert.deepEqual(selection(r), [5, 5]);
    const s = typed('hello', 5); s.gump.handle(1, 0x250000 | SHIFT, 0, 0); assert.deepEqual(selection(s), [4, 5]); assert.deepEqual([s.root.shiftAnchor, s.root.shiftEnd], [5, 4]);
    s.gump.handle(1, 0x250000 | SHIFT, 0, 0); assert.deepEqual(selection(s), [3, 5]); assert.deepEqual([s.root.shiftAnchor, s.root.shiftEnd], [5, 3]);
    const t = typed('hello', 2); t.gump.handle(1, 0x270000 | SHIFT, 0, 0); assert.deepEqual(selection(t), [2, 3]); t.gump.handle(1, 0x270000 | SHIFT, 0, 0); assert.deepEqual(selection(t), [2, 4]);
    // 왼쪽 끝/오른쪽 끝은 clamp 한다.
    const edge = typed('hello', 0); edge.gump.handle(1, 0x250000, 0, 0); assert.deepEqual(selection(edge), [0, 0]); const end = typed('hello', 5); end.gump.handle(1, 0x270000, 0, 0); assert.deepEqual(selection(end), [5, 5]);
});

test('handle Home/End: shift 없이 0/length 로 이동하고 열 delta 는 Home 은 -scrollColumn, End 는 shiftEnd 를 쓰며 shift 는 기준을 잡고 선택을 늘린다', () => {
    const home = typed('hello', 3); home.root.scrollColumn = 2; home.gump.handle(1, 0x240000, 0, 0); assert.deepEqual(selection(home), [0, 0]); assert.deepEqual(home.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), ['sliderPosition', 201, 0]);
    const shiftHome = typed('hello', 3); shiftHome.gump.handle(1, 0x240000 | SHIFT, 0, 0); assert.deepEqual(selection(shiftHome), [0, 3]); assert.deepEqual([shiftHome.root.shiftAnchor, shiftHome.root.shiftEnd], [3, 0]);
    // End: shiftEnd(0)만큼 열 delta, shift 가 있으면 shiftEnd=length+1 이 delta 로 쓰인다(열 clamp 로 4).
    const end = typed('hello', 1); end.gump.handle(1, 0x230000, 0, 0); assert.deepEqual(selection(end), [5, 5]); assert.deepEqual(end.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), ['sliderPosition', 201, 0]);
    const shiftEnd = typed('hello', 2); shiftEnd.gump.handle(1, 0x230000 | SHIFT, 0, 0); assert.deepEqual(selection(shiftEnd), [2, 5]); assert.deepEqual([shiftEnd.root.shiftAnchor, shiftEnd.root.shiftEnd], [2, 6]); assert.deepEqual(shiftEnd.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), ['sliderPosition', 201, 4]);
});

test('handle 편집 키: Backspace/Delete 는 선택이 없으면 앞/뒤 한 글자, 있으면 선택을 지우고 Ctrl+Backspace 는 전체 Reset, Shift+Delete 는 Cut 이다', () => {
    const b = typed('hello', 5); b.gump.handle(1, 8, 0, 0); assert.equal(b.buffer().text(), 'hell'); b.buffer().select(1, 3); b.gump.handle(1, 8, 0, 0); assert.equal(b.buffer().text(), 'hl'); b.buffer().select(0, 0); b.gump.handle(1, 8, 0, 0); assert.equal(b.buffer().text(), 'hl');
    b.gump.handle(1, 8 | CONTROL, 0, 0); assert.equal(b.buffer().text(), '');
    const d = typed('hello', 1); d.gump.handle(1, 0x2e0000, 0, 0); assert.equal(d.buffer().text(), 'hllo'); d.buffer().select(0, 2); d.gump.handle(1, 0x2e0000, 0, 0); assert.equal(d.buffer().text(), 'lo'); d.buffer().select(2, 2); d.gump.handle(1, 0x2e0000, 0, 0); assert.equal(d.buffer().text(), 'lo');
    const cut = typed('hello', 5, { clipboard: 'zz' }); cut.buffer().select(1, 3); cut.gump.handle(1, 0x2e0000 | SHIFT, 0, 0); assert.deepEqual(cut.clip.written, ['el']); assert.equal(cut.buffer().text(), 'hlo');
});

test('handle Insert: Ctrl 은 복사, Shift 는 제외문자를 거른 붙여넣기이고 둘 다 없으면 효과 없이 반환 1 이다', () => {
    const r = typed('hello', 2, { clipboard: 'zzq' }); r.gump.handle(1, 0x2d0000 | CONTROL, 0, 0); assert.deepEqual(r.clip.written, ['hello']); assert.equal(r.buffer().text(), 'hello');
    assert.equal(r.gump.handle(1, 0x2d0000 | SHIFT, 0, 0), 1); assert.equal(r.buffer().text(), 'hezzqllo'); const before = r.buffer().text(); assert.equal(r.gump.handle(1, 0x2d0000, 0, 0), 1); assert.equal(r.buffer().text(), before);
    // 제외문자는 clipboard 에서 지운다.
    const x = typed('ab', 2, { clipboard: 'zzq' }); x.gump.setExcluded(1, 'z'); x.gump.handle(1, 0x2d0000 | SHIFT, 0, 0); assert.equal(x.buffer().text(), 'abq');
});

test('handle Enter: multiline 이면 LF 를 넣고(전달 값은 10) 아니어도 Enter 사건을 node.serial 로 올리며, Tab/ESC/위/아래는 반환 0 이다', () => {
    const r = typed('ab'); r.root.forwardInput = 1; r.root.changeNotice = 1; assert.equal(r.gump.handle(1, 13, 3, 4), 1); assert.equal(r.buffer().text(), 'ab\n');
    assert.deepEqual(r.log, [['bubble', 1, 0x02000038, 77, 0], ['bubble', 1, 0x020000bf, 0, 0], ['redraw', 1, false], ['bubble', 1, 10, 3, 4]]);
    const single = typed('ab'); single.root.multiline = 0; single.root.forwardInput = 1; single.gump.handle(1, 13, 3, 4); assert.equal(single.buffer().text(), 'ab'); assert.deepEqual(single.log, [['bubble', 1, 0x02000038, 77, 0], ['redraw', 1, false], ['bubble', 1, 13, 3, 4]]);
    const keys = typed('ab'); keys.root.changeNotice = 1; for (const code of [9, 27, 0x260000, 0x280000]) { keys.log.length = 0; assert.equal(keys.gump.handle(1, code, 0, 0), 0, String(code)); assert.deepEqual(keys.log, [['redraw', 1, false]]); }
});

test('handle Slider 사건: 변경 사건은 해당 Slider 의 현재 position 으로 스크롤하고 모르는 serial 은 assertion(0x246)이며, 단계 사건은 M 폭만큼 가로 이동한다', () => {
    const lines = Array.from({ length: 10 }, (_, index) => `line${index}`).join('\n'); const r = typed(lines, 0, {}, 200, 32);
    r.sliderOf(200).position = 3; assert.equal(r.gump.handle(1, 0x02000036, 200, 0), 0); assert.deepEqual(r.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 200), ['sliderPosition', 200, 3]); assert.equal(r.root.scrollLine, 3);
    // 가로: 긴 줄에서 position 5 로 스크롤한다.
    const wide = typed('x'.repeat(60), 0, {}, 200, 60); wide.sliderOf(201).position = 5; wide.gump.handle(1, 0x02000036, 201, 0); assert.deepEqual(wide.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), ['sliderPosition', 201, 5]); assert.equal(wide.root.scrollColumn, 5);
    r.log.length = 0; r.gump.handle(1, 0x02000036, 999, 0); assert.deepEqual(r.log, [['assertion', 0x246]]);
    // 단계 사건: 가로 Slider 의 1/3 은 +8, 2/4 는 -8, 그 밖의 값/다른 serial 은 무시하되 M 은 먼저 잰다.
    const step = typed('x'.repeat(60), 0, {}, 200, 60); let measured = 0; step.hooks.measure = ((original) => (token, text, length) => { if (text.length === 2 && text[0] === 0x4d) measured += 1; return original(token, text, length); })(step.hooks.measure);
    const gump = new TextEntryGump(step.state, step.hooks);
    step.root.scrollColumn = 10; assert.equal(gump.handle(1, 0x0200002e, 201, 1), 1); assert.deepEqual(step.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), ['sliderPosition', 201, 18]);
    step.log.length = 0; step.root.scrollColumn = 10; assert.equal(gump.handle(1, 0x0200002e, 201, 2), 1); assert.deepEqual(step.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), ['sliderPosition', 201, 2]);
    step.log.length = 0; assert.equal(gump.handle(1, 0x0200002e, 201, 3), 1); assert.equal(gump.handle(1, 0x0200002e, 201, 4), 1);
    // 처리하지 않는 경우(두 번째 값 5, 다른 serial)도 M 은 정확히 한 번(처리기 자신의 측정) 먼저 잰다. 처리된 경우는 setScroll 이 한 번 더 잰다.
    measured = 0; assert.equal(gump.handle(1, 0x0200002e, 201, 5), 0); assert.equal(measured, 1); measured = 0; assert.equal(gump.handle(1, 0x0200002e, 200, 1), 0); assert.equal(measured, 1);
    measured = 0; assert.equal(gump.handle(1, 0x0200002e, 201, 1), 1); assert.equal(measured, 2);
});

test('handleConsole Tab: 새 단어는 문자열을 보내고 반복 Tab 은 null 을 보내며 결과로 단어를 교체하고, 선택이 있거나 autocomplete 가 없으면 호출 없이 반환 1 이다', () => {
    const results = ['world', 'word2', null]; const r = typed('say hello wor', 13, { console: true, complete: () => results.shift() }); r.root.autocomplete = 5; r.root.resetWord = 1;
    assert.equal(r.gump.handleConsole(1, 9, 0, 0), 1); assert.equal(r.buffer().text(), 'say hello world'); assert.deepEqual([r.root.wordStart, r.root.wordLength, r.root.resetWord], [10, 5, 0]); assert.deepEqual(r.log.find((entry) => entry[0] === 'complete'), ['complete', 5, 'wor', 3, false]); assert.deepEqual(selection(r), [15, 15]);
    r.log.length = 0; assert.equal(r.gump.handleConsole(1, 9 | SHIFT, 0, 0), 1); assert.equal(r.buffer().text(), 'say hello word2'); assert.deepEqual(r.log.find((entry) => entry[0] === 'complete'), ['complete', 5, null, 5, true]);
    r.log.length = 0; r.gump.handleConsole(1, 9, 0, 0); assert.equal(r.buffer().text(), 'say hello word2'); assert.deepEqual([r.root.wordLength, r.root.resetWord], [5, 0]);
    r.log.length = 0; r.buffer().select(0, 3); assert.equal(r.gump.handleConsole(1, 9, 0, 0), 1); assert.deepEqual(r.log, []);
    const none = typed('say wor', 7, { console: true, complete: () => 'x' }); assert.equal(none.gump.handleConsole(1, 9, 0, 0), 1); assert.deepEqual(none.log, []);
    assert.throws(() => { const m = typed('a', 1, { console: true }); m.root.autocomplete = 1; new TextEntryGump(m.state, { ...m.hooks, complete: undefined }).handleConsole(1, 9, 0, 0); }, { name: 'TypeError', message: /autocomplete callback/ });
});

test('handleConsole Tab: 단어 시작은 공백/탭/콜론 바로 뒤까지 거슬러 찾고 구분자가 없으면 0 이다', () => {
    for (const [text, word] of [['to:Ann', 'Ann'], ['say\thi', 'hi'], ['one two', 'two'], ['plain', 'plain'], ['a  b', 'b'], ['x:', '']]) {
        const r = typed(text, text.length, { console: true, complete: () => null }); r.root.autocomplete = 9; r.root.resetWord = 1; r.gump.handleConsole(1, 9, 0, 0);
        assert.deepEqual(r.log.find((entry) => entry[0] === 'complete'), ['complete', 9, word, word.length, false], text);
    }
});

test('handleConsole: Enter 는 현재 본문을 제출한 뒤 단어 재설정을 켜고 기본 처리에 위임하며, Shift 키는 재설정하지 않고, 그 밖의 키는 재설정을 켠 채 위임한다', () => {
    const r = typed('send me', 7, { console: true }); r.root.multiline = 0; r.root.submit = 3; r.root.resetWord = 0;
    assert.equal(r.gump.handleConsole(1, 13, 0, 0), 1); assert.deepEqual(r.log.find((entry) => entry[0] === 'submit'), ['submit', 3, 'send me']); assert.equal(r.root.resetWord, 1); assert.deepEqual(r.log.find((entry) => entry[0] === 'bubble').slice(0, 3), ['bubble', 1, 0x02000038]);
    r.root.resetWord = 0; r.log.length = 0; assert.equal(r.gump.handleConsole(1, 0x100000, 0, 0), 0); assert.equal(r.root.resetWord, 0);
    r.log.length = 0; assert.equal(r.gump.handleConsole(1, 0x41, 0, 0), 1); assert.equal(r.root.resetWord, 1); assert.equal(r.buffer().text(), 'send meA');
    // 제출 callback 이 없으면 submit 토큰이 있을 때만 TypeError, 뗌/명령 사건은 재설정 없이 기본 처리다.
    const s = typed('x', 1, { console: true }); s.root.submit = 4; assert.throws(() => new TextEntryGump(s.state, { ...s.hooks, submit: undefined }).handleConsole(1, 13, 0, 0), { name: 'TypeError', message: /제출 callback/ });
    const e = typed('x', 1, { console: true }); e.root.resetWord = 0; e.gump.handleConsole(1, 0x41 | RELEASE >>> 0, 0, 0); e.gump.handleConsole(1, 0x41 | COMMAND, 0, 0); assert.equal(e.root.resetWord, 0); assert.equal(e.buffer().text(), 'x');
    assert.throws(() => rig().gump.handleConsole(1, 9, 0, 0), /Console 공유 파생 노드/);
});

test('draw: 배경 뒤 줄마다 LF 를 공백으로 바꿔 현재 줄 높이만큼 내려 그리고 빈 줄은 공백 한 칸이다', () => {
    const r = typed('ab\ncd\nef', 0); r.gump.draw(1); assert.deepEqual(r.log, [['drawBackground', 1], ['drawText', 'ab ', 0, 0, null], ['drawText', 'cd ', 0, 16, null], ['drawText', 'ef', 0, 32, null]]);
    const empty = typed(''); empty.gump.draw(1); assert.deepEqual(empty.log, [['drawBackground', 1], ['drawText', ' ', 0, 0, null]]);
    // 열 스크롤은 줄 앞을 잘라내고, 줄 길이를 넘으면 자르지 않는다.
    const scroll = typed('hello world', 0); scroll.root.scrollColumn = 6; scroll.gump.draw(1); assert.deepEqual(scroll.log.filter((entry) => entry[0] === 'drawText'), [['drawText', 'world', 0, 0, null]]);
    const over = typed('abc', 0); over.root.scrollColumn = 9; over.gump.draw(1); assert.deepEqual(over.log.filter((entry) => entry[0] === 'drawText'), [['drawText', 'abc', 0, 0, null]]);
    // 표시 시작 줄은 scrollLine 이다.
    const from = typed('a\nb\nc\nd', 0, {}, 200, 32); from.root.scrollLine = 2; from.gump.draw(1); assert.deepEqual(from.log.filter((entry) => entry[0] === 'drawText'), [['drawText', 'c ', 0, 0, null], ['drawText', 'd', 0, 16, null]]);
});

test('draw: 표시 줄 수는 높이/줄높이에 나머지가 있을 때 한 줄 더하는 원본 정수 관문이고 멈춘 줄에서 그리기를 끝낸다', () => {
    const lines = 'a\nb\nc\nd\ne\nf'; const count = (height) => { const r = typed(lines, 0, {}, 200, height); r.gump.draw(1); return r.log.filter((entry) => entry[0] === 'drawText').length; };
    // 32 → 정확히 2줄, 40 → 나머지 8 이라 3줄, 48 → 3줄, 60 → 나머지 12 라 4줄.
    assert.deepEqual([count(32), count(40), count(48), count(60), count(16), count(17)], [2, 3, 3, 4, 1, 2]);
});

test('draw 선택: 선택 앞/선택(반전색)/뒤를 따로 그리고 선택 사각형을 현재 위치에서 채우며, 캐럿은 위상이 홀수이고 읽기 전용이 아닐 때만 1칸 폭으로 그린다', () => {
    const r = typed('hello world', 0); r.buffer().select(2, 7); r.gump.draw(1);
    assert.deepEqual(r.log, [['drawBackground', 1], ['drawText', 'he', 0, 0, null], ['fill', 26, 20, 66, 36, 0x5566], ['drawText', 'llo w', 16, 0, 0x88], ['drawText', 'orld', 56, 0, null]]);
    // 선택이 줄 시작/끝이면 앞/뒤 그림이 없다.
    const edge = typed('hello', 0); edge.buffer().select(0, 5); edge.gump.draw(1); assert.deepEqual(edge.log, [['drawBackground', 1], ['fill', 10, 20, 50, 36, 0x5566], ['drawText', 'hello', 0, 0, 0x88]]);
    // 캐럿: 위치 3 → 폭 24, 선택이 없고 위상이 홀수.
    const caret = typed('hello', 3); caret.root.blinkPhase = 1; caret.gump.draw(1); assert.deepEqual(caret.log, [['drawBackground', 1], ['drawText', 'hello', 0, 0, null], ['fill', 34, 20, 35, 36, 0x5566]]);
    const even = typed('hello', 3); even.root.blinkPhase = 2; even.gump.draw(1); assert.equal(names(even.log).includes('fill'), false);
    const ro = typed('hello', 3); ro.root.blinkPhase = 1; ro.root.readOnly = 1; ro.gump.draw(1); assert.equal(names(ro.log).includes('fill'), false);
    // 여러 줄: 캐럿은 자기 줄의 y 에 그린다(둘째 줄 y=16).
    const multi = typed('ab\ncd', 4); multi.root.blinkPhase = 1; multi.gump.draw(1); assert.deepEqual(multi.log.at(-1), ['fill', 10 + 8, 20 + 16, 10 + 8 + 1, 20 + 16 + 16, 0x5566]);
});

test('draw 수신 대상: Console 강조가 켜져 있고 콜론이 있으면 이름이 유효할 때만 노랑으로 콜론까지 그리고 나머지는 접두 폭에서 이어 그린다', () => {
    const r = typed('Ann: hi', 0, { console: true }); r.root.recipientHighlight = 1; r.gump.draw(1);
    assert.deepEqual(r.log, [['drawBackground', 1], ['recipient', 'Ann'], ['drawText', 'Ann:', 0, 0, 0x33], ['drawText', ' hi', 32, 0, null]]);
    const bad = typed('Bob: hi', 0, { console: true }); bad.root.recipientHighlight = 1; bad.gump.draw(1); assert.deepEqual(bad.log.slice(1), [['recipient', 'Bob'], ['drawText', 'Bob:', 0, 0, null], ['drawText', ' hi', 32, 0, null]]);
    const noColon = typed('Ann hi', 0, { console: true }); noColon.root.recipientHighlight = 1; noColon.gump.draw(1); assert.deepEqual(noColon.log.slice(1), [['drawText', 'Ann hi', 0, 0, null]]);
    const off = typed('Ann: hi', 0, { console: true }); off.root.recipientHighlight = 0; off.gump.draw(1); assert.deepEqual(off.log.slice(1), [['drawText', 'Ann: hi', 0, 0, null]]);
    // 선택이 있는 줄은 수신 대상 색을 적용하지 않는다.
    const selected = typed('Ann: hi', 0, { console: true }); selected.root.recipientHighlight = 1; selected.buffer().select(0, 3); selected.gump.draw(1); assert.equal(names(selected.log).includes('recipient'), false);
    const missing = typed('Ann: hi', 0, { console: true }); missing.root.recipientHighlight = 1; assert.throws(() => new TextEntryGump(missing.state, { ...missing.hooks, recipient: undefined }).draw(1), { name: 'TypeError', message: /수신 대상 검사/ });
});

test('draw 오류: 줄 높이 0 은 Error, 출력 장치 누락은 TypeError, 줄 복사가 거부되면 assertion(0x2ae) 뒤 Error 다', () => {
    const r = typed('abc', 0); r.hooks.measure = () => [8, 0]; assert.throws(() => new TextEntryGump(r.state, r.hooks).draw(1), { name: 'Error', message: /줄높이 나눗셈/ });
    const min = typed('abc', 0); min.hooks.measure = () => [8, -1]; min.root.baseHeight = -2147483648; assert.throws(() => new TextEntryGump(min.state, min.hooks).draw(1), /줄높이 나눗셈/);
    for (const name of ['drawBackground', 'drawText', 'position', 'fill']) assert.throws(() => new TextEntryGump(r.state, { ...r.hooks, [name]: undefined }).draw(1), { name: 'TypeError', message: /픽셀\/글자 출력/ }, name);
    const long = typed('x'.repeat(1100), 0); assert.throws(() => long.gump.draw(1), { name: 'Error', message: /줄 복사 범위 거부/ }); assert.deepEqual(long.log.filter((entry) => entry[0] === 'assertion'), [['assertion', 0x2ae]]);
});
