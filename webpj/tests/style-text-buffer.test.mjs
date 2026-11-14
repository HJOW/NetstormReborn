// 전용 PE fixture가 없는 StyleTextBuffer(다중 줄 입력 저장소·표시 줄 공백 줄바꿈)를 기준선 소스 계약·독립 단어 배치 모형·실제 글꼴/그림 연결로 검증한다. (2026-11-14 WB04-E422)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(StyleTextBuffer.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { StyleTextBuffer, StyleTextBufferState, StyleTextBufferLine, bindStyleTextBufferFonts, bindStyleTextBufferMarkup } from '../www/ui/style-text-buffer.js';
import { StyleText, StyleTextState, StyleTextFragmentKind as K, bindStyleTextFonts, registerStyleTextFragment, StyleTextFragment } from '../www/ui/style-text-fragments.js';
import { Markup, MarkupState, bindMarkup } from '../www/ui/markup-picture.js';
import { GumpText } from '../www/render/gump-text.js';
import { stoneFonts } from './stone-button-support.mjs';

// 문자열은 바이트 하나가 문자 하나이며 NUL 로 끝낸다.
const bytes = (text) => Uint8Array.from(Buffer.from(text + '\0', 'latin1'));
// 8DWORD 문맥(글꼴 번호만 context[1] 에 둔다)이다.
const words = (font = 0, color = 0) => Uint32Array.from([0, font, color, 0, 0, 0, 0, 0]);
// 조각의 문자열(첫 NUL 앞)이다.
const textOf = (node) => Buffer.from(node.text.subarray(0, node.text.indexOf(0))).toString('latin1');

// 한 글자 폭 1·높이 10+글꼴 번호인 독립 글꼴 장치와 release 기록을 가진 장면이다. 그림은 130×120 헤더 한 종류다.
function rig(options = {}) {
    const fragments = new StyleTextState(); const events = []; const measureCalls = [];
    const measure = (text, context, length) => { const nul = text.indexOf(0); const limit = nul < 0 ? text.length : nul; const count = length < 0 ? limit : Math.min(length, limit); return [count, 10 + context[1]]; };
    const styleHooks = { textMeasure: (text, context, output) => { const size = measure(text, context, -1); output.width = size[0]; output.height = size[1]; }, textMetrics: () => [0, 0], release: (token) => events.push(['release', token]) };
    const pictureState = new MarkupState(); pictureState.typeCount = 200; const header = { width: 130, height: 120, hotspotX: 0, hotspotY: 0, extraX: 0, extraY: 0 };
    const pictures = new Markup(fragments, pictureState, { type: () => ({ token: 50, shape: 500, frameCount: 100, hotspotX: 0, hotspotY: 0 }), frame: () => header });
    bindMarkup(styleHooks, pictures);
    const style = new StyleText(fragments, styleHooks);
    const hooks = { measureText: (text, context, length) => { measureCalls.push([textOf({ text }), length]); return measure(text, context, length); } };
    bindStyleTextBufferMarkup(hooks, pictures);
    const state = new StyleTextBufferState(); const buffer = new StyleTextBuffer(state, fragments, style, options.maxLines ?? 8, hooks);
    return { fragments, events, measureCalls, style, styleHooks, hooks, pictures, pictureState, state, buffer, header };
}
// 입력 줄 i 의 조각 문자열 목록이다.
function hard(s, index) { const out = []; for (let token = s.state.hardLines[index]; token; token = s.fragments.fragments.get(token).next) { const node = s.fragments.fragments.get(token); out.push(node.kind === K.TEXT ? textOf(node) : `[${node.kind}]`); } return out; }
// 표시 줄 목록이다(센티널 next 순서).
function soft(s) { const out = []; for (let token = s.state.sentinel.next; token; token = s.state.softLines.get(token).next) { const line = s.state.softLines.get(token); const pieces = []; for (let item = line.head; item; item = s.fragments.fragments.get(item).next) { const node = s.fragments.fragments.get(item); pieces.push(node.kind === K.TEXT ? textOf(node) : `[${node.kind}]`); } out.push({ token, width: line.width, height: line.height, y: line.y, pieces }); } return out; }

test('생성자: 양수 최대 줄 수와 새 저장소만 허용하고 원본 초기값으로 채운다', () => {
    const fragments = new StyleTextState(); const style = new StyleText(fragments, {}); const state = new StyleTextBufferState();
    for (const bad of [0, -1, 1.5]) assert.throws(() => new StyleTextBuffer(new StyleTextBufferState(), fragments, style, bad), /양수 최대 줄 수/);
    state.dirty = 9; state.noWrap = 9; state.reserved = 9; state.nextSoftToken = 77; state.onAppend = () => {}; state.softCount = 5; state.lineCount = 4;
    new StyleTextBuffer(state, fragments, style, 4);
    assert.deepEqual([state.maxLines, state.lineCount, state.softCount, state.reserved, state.dirty, state.noWrap, state.nextSoftToken, state.initialized, state.onAppend], [4, 0, 0, 0, 0, 0, 1, true, null]);
    assert.equal(state.hardLines.length, 4); assert.deepEqual(Array.from(state.hardLines), [0, 0, 0, 0]); assert.ok(state.sentinel instanceof StyleTextBufferLine);
    // 이미 초기화한 저장소·입력 칸이 있는 저장소·표시 줄이 있는 저장소·센티널 head 가 있는 저장소를 거부한다.
    assert.throws(() => new StyleTextBuffer(state, fragments, style, 4), /새 저장소/);
    for (const dirty of [(x) => { x.hardLines = new Uint32Array(1); }, (x) => x.softLines.set(1, new StyleTextBufferLine()), (x) => { x.sentinel.head = 3; }]) { const used = new StyleTextBufferState(); dirty(used); assert.throws(() => new StyleTextBuffer(used, fragments, style, 4), /새 저장소/); }
});

test('appendText: 통지 → 길이/NUL → LF 분할 순서·빈 구간 조각·CR/탭 유지·끝 칸 보관·태그/문맥·dirty', () => {
    const s = rig(); const notices = []; s.state.onAppend = (input) => notices.push([input.length, s.state.dirty]);
    s.buffer.appendText(bytes('ab\n\ncd\tef\rg\nhi'), words(2, 5), -1, 0x1234);
    assert.deepEqual(notices, [[15, 0]]); assert.equal(s.state.dirty, 1); assert.equal(s.state.lineCount, 3);
    // 빈 구간도 실제 빈 문자열 조각이고 CR/탭은 개행이 아니다. 마지막 구간은 끝 칸에 보관한다.
    assert.deepEqual([hard(s, 0), hard(s, 1), hard(s, 2), hard(s, 3)], [['ab'], [''], ['cd\tef\rg'], ['hi']]);
    const node = s.fragments.fragments.get(s.state.hardLines[0]); assert.deepEqual([node.metadata, Array.from(node.context)], [0x1234, [0, 2, 5, 0, 0, 0, 0, 0]]);
    assert.equal(s.fragments.fragments.get(s.state.hardLines[3]).metadata, 0x1234);
    // 문맥은 복사본이라 이후 변경이 조각에 영향을 주지 않는다.
    const context = words(1); s.buffer.appendText(bytes('x\n'), context, -1, 0); context[1] = 9; assert.equal(s.fragments.fragments.get(s.fragments.fragments.get(s.state.hardLines[3]).next).context[1], 1);
});

test('appendText: 명시 길이·NUL 필수·범위 오류와 통지 시점·dirty 갱신 시점', () => {
    const s = rig();
    // 음수 길이는 NUL 까지, 비음수 길이는 중간 NUL/LF 와 무관하게 지정 범위다.
    s.buffer.appendText(bytes('abc\nde'), words(), -1); assert.deepEqual([hard(s, 0), hard(s, 1)], [['abc'], ['de']]);
    const t = rig(); t.buffer.appendText(Uint8Array.from([97, 10, 98, 10, 99]), words(), 4); assert.deepEqual([t.state.lineCount, hard(t, 0), hard(t, 1)], [2, ['a'], ['b']]);
    t.buffer.appendText(Uint8Array.from([120, 121]), words(), 0); assert.deepEqual(hard(t, 2), []); assert.equal(t.state.dirty, 1);
    // 오류 입력에서도 통지는 먼저 호출되지만 dirty 는 오류 경로에서 켜지지 않는다.
    const u = rig(); let seen = 0; u.state.onAppend = () => { seen += 1; };
    assert.throws(() => u.buffer.appendText(Uint8Array.from([97, 98]), words(), -1), /NUL 필요/); assert.equal(seen, 1); assert.equal(u.state.dirty, 0);
    assert.throws(() => u.buffer.appendText(bytes('abc'), words(), 5), /범위 초과/); assert.equal(seen, 2); assert.equal(u.state.lineCount, 0);
    // 길이가 정확히 입력 길이면 허용한다.
    u.buffer.appendText(bytes('abc'), words(), 4); assert.equal(hard(u, 0)[0], 'abc'); assert.equal(seen, 3);
    // 파괴 뒤에는 저장소가 없다.
    u.buffer.destroy(); assert.throws(() => u.buffer.appendText(bytes('a'), words()), /입력 저장소 없음/); assert.equal(seen, 3);
});

test('appendText: 끝 칸이 차면 완료된 첫 줄을 삭제하고 미완료 끝 칸까지 앞으로 이동하며 release 순서를 지킨다', () => {
    const s = rig({ maxLines: 4 });
    s.buffer.appendText(bytes('a\nb\nc\nx'), words()); const [a, b, c, x] = Array.from(s.state.hardLines);
    assert.deepEqual([s.state.lineCount, hard(s, 3)], [3, ['x']]);
    // 다음 LF 에서 lineCount === maxLines-1 이므로 첫 줄 a 를 삭제하고 이동한 뒤 y 를 끝 칸(x 의 체인)에 붙인다.
    s.events.length = 0; s.buffer.appendText(bytes('y\n'), words());
    assert.deepEqual(s.events, [['release', a]]); assert.equal(s.fragments.fragments.has(a), false);
    assert.deepEqual(Array.from(s.state.hardLines).slice(0, 2), [b, c]); assert.deepEqual([hard(s, 2), s.state.lineCount, s.state.hardLines[3]], [['x', 'y'], 3, 0]);
    // 연속 LF 는 같은 규칙을 반복한다.
    s.buffer.appendText(bytes('\n\n'), words()); assert.equal(s.state.lineCount, 3); assert.equal(s.state.dirty, 1);
    // 최대 줄 수 1 에서는 첫 줄을 버릴 수 없다(삭제 가능한 완료 줄 없음).
    const one = rig({ maxLines: 1 }); one.buffer.appendText(bytes('ab'), words()); assert.deepEqual(hard(one, 0), ['ab']); assert.throws(() => one.buffer.appendText(bytes('\n'), words()), /삭제 완료 줄 범위/);
});

test('removeLine: 완료 줄만 삭제하고 전체 배열을 이동하며 범위·dirty·release', () => {
    const s = rig({ maxLines: 4 }); s.buffer.appendText(bytes('a\nb\nc'), words()); s.state.dirty = 0;
    assert.throws(() => s.buffer.removeLine(-1), /범위 오류/); assert.throws(() => s.buffer.removeLine(2), /범위 오류/); assert.throws(() => s.buffer.removeLine(3), /범위 오류/);
    const [a, b, c] = Array.from(s.state.hardLines); s.events.length = 0; s.buffer.removeLine(0);
    assert.deepEqual(s.events, [['release', a]]); assert.deepEqual(Array.from(s.state.hardLines), [b, c, 0, 0]); assert.deepEqual([s.state.lineCount, s.state.dirty], [1, 1]);
    // 가운데 줄 삭제는 앞 칸을 유지한다.
    const t = rig({ maxLines: 5 }); t.buffer.appendText(bytes('a\nb\nc\nd'), words()); const tokens = Array.from(t.state.hardLines); t.buffer.removeLine(1); assert.deepEqual(Array.from(t.state.hardLines), [tokens[0], tokens[2], tokens[3], 0, 0]); assert.equal(t.state.lineCount, 2);
    t.buffer.destroy(); assert.throws(() => t.buffer.removeLine(0), /저장소 없음/);
});

test('appendPicture: 끝 칸에 그림을 더하고 줄 수는 늘리지 않으며 맞춤 값이 하나라도 비0 이면 Fit', () => {
    const s = rig(); s.buffer.appendText(bytes('ab'), words()); s.state.dirty = 0;
    s.buffer.appendPicture(s.pictures, 5, 6, 0x77); assert.equal(s.state.lineCount, 0); assert.equal(s.state.dirty, 1);
    const token = s.fragments.fragments.get(s.state.hardLines[0]).next; const picture = s.pictureState.pictures.get(token);
    assert.deepEqual([picture.type, picture.frame, picture.scale, picture.mapped, s.fragments.fragments.get(token).metadata], [5, 6, 1, 0, 0x77]);
    // 맞춤 값은 한쪽만 비0 이어도 Fit 을 부른다(130×120 헤더에서 65×60 → 0.5, 폭만 65 이면 높이 0 → 0 비율).
    const scaleAt = (index) => { let item = s.state.hardLines[0]; for (let step = 0; step < index; step += 1) item = s.fragments.fragments.get(item).next; return s.pictureState.pictures.get(item).scale; };
    s.buffer.appendPicture(s.pictures, 5, 6, 0, 65, 60); assert.equal(scaleAt(2), 0.5);
    // 한쪽 값만 비0 이어도 Fit 을 호출한다(0 폭 비율은 0 이라 배율 0, 0 높이 비율도 0).
    s.buffer.appendPicture(s.pictures, 5, 6, 0, 130, 30); assert.equal(scaleAt(3), 0.25);
    s.buffer.appendPicture(s.pictures, 5, 6, 0, 0, 30); assert.equal(scaleAt(4), 0);
    s.buffer.appendPicture(s.pictures, 5, 6, 0, 65, 0); assert.equal(scaleAt(5), 0);
    s.buffer.appendPicture(s.pictures, 5, 6, 0, 0, 0); assert.equal(scaleAt(6), 1);
    // 줄이 완료되지 않은 끝 칸에만 붙는다.
    s.buffer.appendText(bytes('\n'), words()); s.buffer.appendPicture(s.pictures, 5, 7); assert.deepEqual(hard(s, 1), ['[1]']); assert.equal(hard(s, 0).length, 8);
    s.buffer.destroy(); assert.throws(() => s.buffer.appendPicture(s.pictures, 5, 6), /저장소 없음/);
});

test('layout: 폭 0 이하는 기존 표시 목록을 유지하고 양수 폭은 이전 표시 조각을 반납하며 미완료 칸은 순회하지 않는다', () => {
    const s = rig(); s.buffer.appendText(bytes('ab\ncd\nxyz'), words());
    s.buffer.layout(0); s.buffer.layout(-5); assert.deepEqual(soft(s), []); assert.equal(s.state.softCount, 0);
    s.buffer.layout(100);
    assert.deepEqual(soft(s).map((line) => line.pieces), [['ab'], ['cd']]); assert.equal(s.state.softCount, 2); assert.equal(s.state.dirty, 1);
    const firstTokens = soft(s).map((line) => line.token); const clones = soft(s).map((line) => line.pieces.length); void clones;
    // 입력 조각과 표시 복제는 서로 다른 토큰이다. 같은 입력을 다시 배치하면 이전 표시 조각만 반납한다.
    const inputs = Array.from(s.state.hardLines).slice(0, 2); const displayHeads = [...s.state.softLines.values()].map((line) => line.head); assert.ok(displayHeads.every((head) => !inputs.includes(head)));
    s.events.length = 0; s.buffer.layout(0); assert.deepEqual(s.events, []); s.buffer.layout(50);
    assert.deepEqual(s.events.map((event) => event[1]), displayHeads); assert.ok(soft(s).every((line) => !firstTokens.includes(line.token))); assert.equal(inputs.every((token) => s.fragments.fragments.has(token)), true);
    // 표시 줄 토큰은 반납 뒤에도 재사용하지 않고 계속 증가한다.
    assert.deepEqual(soft(s).map((line) => line.token), [3, 4]); assert.equal(s.state.nextSoftToken, 5);
    // 빈 입력 줄도 빈 표시 줄을 만든다.
    const e = rig(); e.buffer.appendText(bytes('\n\nabc\n'), words()); e.buffer.layout(10); assert.deepEqual(soft(e).map((line) => line.pieces), [[''], [''], ['abc']]);
    assert.deepEqual(soft(e).map((line) => [line.y, line.height, line.width]), [[0, 10, 0], [10, 10, 0], [20, 10, 3]]);
});

test('layout 단어 배치: 정확히 폭과 같으면 분할 경로·공백 포함 복사·첫 단어 강제 배치와 빈 표시 줄·현재 줄 폭이 비0 이면 먼저 이동', () => {
    // 여러 조각(appendText 연속 호출)을 한 입력 줄로 만든다. 마지막 조각에만 LF 를 붙여 뒤에 빈 조각이 생기지 않게 한다(빈 마지막 조각은 '' 로 표현).
    const run = (parts, width, noWrap = 0) => { const s = rig(); s.state.noWrap = noWrap; parts.forEach((part, index) => s.buffer.appendText(bytes(index === parts.length - 1 ? `${part}\n` : part), words())); s.buffer.layout(width); return soft(s); };
    const view = (lines) => lines.map((line) => [line.pieces, line.width]);
    // 전체 폭이 제한 폭 미만이면 한 조각이다. 정확히 같으면 분할 경로로 단어 단위(뒤 공백 포함)로 나뉜다.
    assert.deepEqual(view(run(['abc def'], 8)), [[['abc def'], 7]]);
    assert.deepEqual(view(run(['abc def'], 7)), [[['abc '], 4], [['def'], 3]]);
    assert.deepEqual(view(run(['abc def ghi'], 8)), [[['abc def '], 8], [['ghi'], 3]]);
    // 연속 공백은 하나씩 단어 경계가 되고 앞 공백 뒤까지 한 조각에 포함된다.
    assert.deepEqual(view(run(['a  b'], 3)), [[['a  '], 3], [['b'], 1]]);
    // 한 단어가 폭과 같거나 넘으면 빈 줄에서 강제로 배치하고 그 뒤에 빈 표시 줄을 만든다(글자 단위로 쪼개지 않는다).
    assert.deepEqual(view(run(['abcde'], 5)), [[['abcde'], 5], [[], 0]]);
    assert.deepEqual(view(run(['abcde'], 6)), [[['abcde'], 5]]);
    assert.deepEqual(view(run(['abcdefgh ij'], 5)), [[['abcdefgh '], 9], [['ij'], 2]]);
    // 현재 줄 폭이 비0 이면 첫 단어가 들어가지 않을 때 먼저 새 줄로 옮긴 뒤 강제 배치한다.
    assert.deepEqual(view(run(['ab', 'abcdefgh'], 5)), [[['ab'], 2], [['abcdefgh'], 8], [[], 0]]);
    // 앞 조각 뒤에 이어서 들어가는 단어는 같은 줄에 붙고 나머지가 다음 줄로 간다(전체 접두 폭에 현재 줄 폭을 더해 비교).
    assert.deepEqual(view(run(['ab', 'cd ef gh'], 7)), [[['ab', 'cd '], 5], [['ef gh'], 5]]);
    // 줄바꿈 금지(noWrap)는 폭과 무관하게 한 조각으로 둔다.
    assert.deepEqual(view(run(['abcdefgh ij'], 5, 1)), [[['abcdefgh ij'], 11]]);
    // 강제 배치 뒤 새 빈 줄에는 빈 문자열 조각이 그대로 들어간다(0+0 < 폭 이므로 전체 복사).
    assert.deepEqual(view(run(['abcde', ''], 5)), [[['abcde'], 5], [[''], 0]]);
});

test('layout 그림: 비문자 조각은 이동 전 현재 줄 높이만 갱신·빈 줄 강제 배치·noWrap·복제는 배율 1/치환 0', () => {
    const s = rig(); s.buffer.appendText(bytes('abc'), words()); s.buffer.appendPicture(s.pictures, 5, 6, 0x99, 65, 60); s.buffer.appendText(bytes('\n'), words());
    const input = s.fragments.fragments.get(s.state.hardLines[0]).next; s.pictureState.pictures.get(input).mapped = 1;
    // 폭 100: 글자 3 + 그림 65(배율 0.5) = 68 < 100 이라 같은 줄이고 높이는 그림 60 으로 갱신된다(끝 빈 조각도 같은 줄).
    s.buffer.layout(100); let lines = soft(s); assert.deepEqual(lines.map((l) => [l.pieces, l.width, l.height]), [[['abc', '[1]', ''], 68, 60]]);
    const clone = s.state.softLines.get(lines[0].token).head; const picture = s.fragments.fragments.get(clone).next;
    assert.notEqual(picture, input); assert.deepEqual([s.pictureState.pictures.get(picture).scale, s.pictureState.pictures.get(picture).mapped, s.fragments.fragments.get(picture).metadata], [1, 0, 0x99]);
    assert.deepEqual([s.pictureState.pictures.get(input).scale, s.pictureState.pictures.get(input).mapped], [0.5, 1]);
    // 폭 60: 글자 뒤 그림이 넘으면(head 있음) 새 줄로 가며, 이전 줄만 그림 높이를 받고 새 줄 높이는 0 으로 남는다. 그림 뒤 폭이 찬 줄에는 빈 조각이 들어가지 않는다.
    s.buffer.layout(60); lines = soft(s); assert.deepEqual(lines.map((l) => [l.pieces, l.width, l.height, l.y]), [[['abc'], 3, 60, 0], [['[1]'], 65, 0, 60]]);
    // 빈 줄에서는 폭보다 큰 그림도 강제 배치하고 새 줄을 만들지 않는다. noWrap 이면 head 가 있어도 새 줄을 만들지 않는다.
    const t = rig(); t.buffer.appendPicture(t.pictures, 5, 6); t.buffer.appendText(bytes('\n'), words()); t.buffer.layout(100); assert.deepEqual(soft(t).map((l) => [l.pieces, l.width, l.height]), [[['[1]'], 130, 120]]);
    const u = rig(); u.state.noWrap = 1; u.buffer.appendText(bytes('abc'), words()); u.buffer.appendPicture(u.pictures, 5, 6); u.buffer.appendText(bytes('\n'), words()); u.buffer.layout(10); assert.deepEqual(soft(u).map((l) => [l.pieces, l.width, l.height]), [[['abc', '[1]', ''], 133, 120]]);
    // 그림 외 다른 외부 조각은 이전 복제 경계가 없으면 거부하고, 있으면 그 경계로 위임한다.
    const v = rig(); const other = registerStyleTextFragment(v.fragments, Object.assign(new StyleTextFragment(), { kind: K.EXTERNAL })); v.styleHooks.externalMeasure = () => [1, 1];
    v.state.hardLines[0] = other; v.state.lineCount = 1; assert.throws(() => v.buffer.layout(10), /그림 외 파생 복제 경계/);
    const chained = { cloneExternal: () => 4242 }; bindStyleTextBufferMarkup(chained, v.pictures); assert.equal(chained.cloneExternal(other), 4242);
    // 측정/복제 장치가 없으면 도달할 때만 거부한다.
    const w = rig(); const bare = new StyleTextBuffer(new StyleTextBufferState(), w.fragments, w.style, 3, {}); bare.appendText(bytes('abc\n'), words());
    assert.throws(() => bare.layout(10), /문자열 측정 경계/);
    const b2 = new StyleTextBuffer(new StyleTextBufferState(), w.fragments, w.style, 3, { measureText: () => [1, 1] }); b2.appendPicture(w.pictures, 5, 6); b2.appendText(bytes('\n'), words()); assert.throws(() => b2.layout(10), /비문자 가상 복제 경계/);
});

test('maximumWidth/totalHeight: 표시 줄 수만큼 순회한 최대 폭·마지막 줄 y+높이·링크 불일치 거부·y 는 삽입 시점 값', () => {
    const s = rig(); assert.deepEqual([s.buffer.maximumWidth(), s.buffer.totalHeight()], [0, 0]);
    s.buffer.appendText(bytes('ab\nabcdef\nx\n'), words(3)); s.buffer.layout(100);
    assert.deepEqual(soft(s).map((l) => [l.width, l.height, l.y]), [[2, 13, 0], [6, 13, 13], [1, 13, 26]]); assert.deepEqual([s.buffer.maximumWidth(), s.buffer.totalHeight()], [6, 39]);
    // y 는 삽입 시점의 전임 y+높이로만 정해지고 이후 높이가 바뀌어도 다시 계산하지 않는다.
    s.state.softLines.get(soft(s)[0].token).height = 50; assert.deepEqual([soft(s)[1].y, s.buffer.totalHeight()], [13, 39]);
    // 카운터가 링크보다 크면 센티널을 만나 거부하고 작으면 앞쪽만 순회한다.
    s.state.softCount = 4; assert.throws(() => s.buffer.maximumWidth(), /링크 불일치/); s.state.softCount = 1; assert.equal(s.buffer.maximumWidth(), 2); s.state.softCount = 0; assert.deepEqual([s.buffer.maximumWidth(), s.buffer.totalHeight()], [0, 0]);
    // 최대값의 초기값은 0 이라 음수 폭은 결과가 되지 못한다.
    s.state.softCount = 3; s.state.softLines.get(soft(s)[0].token).width = -5; assert.equal(s.buffer.maximumWidth(), 6);
    s.buffer.destroy(); assert.throws(() => s.buffer.maximumWidth(), /저장소 없음/); assert.throws(() => s.buffer.totalHeight(), /저장소 없음/); assert.throws(() => s.buffer.layout(10), /저장소 없음/);
});

test('표시 줄 토큰: 외부에서 먼저 넣은 토큰을 건너뛰고 DWORD 끝 뒤(0)는 소진으로 거부한다', () => {
    const s = rig(); s.buffer.appendText(bytes('a\nb\nc\n'), words()); s.state.softLines.set(1, new StyleTextBufferLine()); s.state.softLines.set(2, new StyleTextBufferLine());
    s.buffer.layout(50); assert.deepEqual(soft(s).map((l) => l.token), [3, 4, 5]); assert.equal(s.state.nextSoftToken, 6);
    // 호스트가 넣은 줄 1/2 는 센티널 연결 밖이라 반납되지 않는다.
    assert.ok(s.state.softLines.has(1) && s.state.softLines.has(2));
    // 마지막 DWORD 토큰을 쓰면 다음 토큰은 0(소진)이다.
    const t = rig(); t.buffer.appendText(bytes('a\nb\n'), words()); t.state.nextSoftToken = 0xffffffff; assert.throws(() => t.buffer.layout(50), /토큰 소진/); assert.deepEqual(soft(t).map((l) => l.token), [0xffffffff]); assert.equal(t.state.nextSoftToken, 0);
    const u = rig(); u.buffer.appendText(bytes('a\n'), words()); u.state.nextSoftToken = 0; assert.throws(() => u.buffer.layout(50), /토큰 소진/);
    // 마지막 토큰이 이미 쓰이고 있으면 0 으로 넘어가 소진이다(앞쪽 빈 토큰으로 돌아가지 않는다).
    const v = rig(); v.buffer.appendText(bytes('a\n'), words()); v.state.nextSoftToken = 0xffffffff; v.state.softLines.set(0xffffffff, new StyleTextBufferLine()); assert.throws(() => v.buffer.layout(50), /토큰 소진/);
});

test('destroy: 완료 입력 줄 → 입력 배열 → 표시 줄 → 센티널 조각 순서 반납·미완료 끝 칸은 삭제하지 않는다', () => {
    const s = rig({ maxLines: 6 }); s.buffer.appendText(bytes('a\nb\ncd'), words()); s.buffer.layout(50);
    const inputs = Array.from(s.state.hardLines).slice(0, 3); const display = [...s.state.softLines.values()].map((line) => line.head);
    const sentinelHead = registerStyleTextFragment(s.fragments, Object.assign(new StyleTextFragment(), { text: bytes('s'), context: words() })); s.state.sentinel.head = sentinelHead;
    s.events.length = 0; s.buffer.destroy();
    assert.deepEqual(s.events.map((e) => e[1]), [inputs[0], inputs[1], ...display, sentinelHead]);
    // 미완료 끝 칸(cd)의 조각은 공통 저장소에 그대로 남고 배열만 비워진다.
    assert.equal(s.fragments.fragments.has(inputs[2]), true); assert.deepEqual([s.state.hardLines.length, s.state.initialized, s.state.sentinel.head, s.state.sentinel.next, s.state.sentinel.previous], [0, false, 0, 0, 0]);
    // 카운터는 재초기화하지 않는다(원본 소멸자와 같다).
    assert.deepEqual([s.state.lineCount, s.state.softCount, s.state.softLines.size], [2, 2, 0]); assert.throws(() => s.buffer.destroy(), /저장소 없음/);
    // 완료 줄 수가 최대 칸을 넘는 호스트 오류는 거부한다.
    const t = rig({ maxLines: 3 }); t.buffer.appendText(bytes('a\n'), words()); t.state.lineCount = 5; assert.throws(() => t.buffer.destroy(), /소멸 완료 줄 수/); assert.throws(() => t.buffer.layout(10), /완료 줄 수 범위/);
});

test('앱 연결: 실제 FontStore/GumpText 폭으로 줄바꿈하고 표시 줄 폭이 실제 StyleText 측정과 같으며 폭 미만이다', () => {
    const fonts = stoneFonts(true); const text = new GumpText(fonts); const fragments = new StyleTextState(); const styleHooks = {}; bindStyleTextFonts(styleHooks, text); const style = new StyleText(fragments, styleHooks);
    const hooks = {}; bindStyleTextBufferFonts(hooks, text); const state = new StyleTextBufferState(); const buffer = new StyleTextBuffer(state, fragments, style, 16, hooks);
    const paragraph = 'AAAA AAAAAA AA AAAAAAAA A AAAAA AAA AAAAAAA AAAA AA AAAAAAAAA';
    buffer.appendText(bytes(`${paragraph}\nAA\n${paragraph}\n`), words(0, 17));
    const width = 20; buffer.layout(width);
    const view = []; for (let token = state.sentinel.next; token; token = state.softLines.get(token).next) view.push(state.softLines.get(token));
    assert.ok(view.length > 8, `표시 줄 ${view.length}개`);
    let longest = 0; let lastBottom = 0; let wrapped = 0;
    for (const line of view) {
        // 줄 폭은 같은 실제 StyleText 가 조각 체인을 측정한 폭과 같다.
        assert.equal(line.width, style.measure({ value: line.head }).width);
        let pieces = 0; for (let item = line.head; item; item = fragments.fragments.get(item).next) { pieces += 1; assert.deepEqual(Array.from(fragments.fragments.get(item).context), [0, 0, 17, 0, 0, 0, 0, 0]); }
        // 한 조각 줄은 강제 배치일 수 있지만 여러 조각 줄은 반드시 폭 미만이다.
        if (pieces > 1) assert.ok(line.width < width, `${line.width}`);
        if (line.width) wrapped += 1; longest = Math.max(longest, line.width); lastBottom = line.y + line.height;
    }
    assert.ok(wrapped >= 8); assert.equal(buffer.maximumWidth(), longest); assert.equal(buffer.totalHeight(), lastBottom);
    // 모든 표시 줄은 측정 높이가 같다(같은 글꼴 슬롯)이므로 y 는 줄 수에 비례한다.
    assert.deepEqual(view.map((line) => line.y), view.map((_, index) => index * view[0].height));
    buffer.destroy(); assert.throws(() => buffer.maximumWidth(), /저장소 없음/);
});

test('layout 단어 배치 독립 모형: 무작위 단락·폭·줄바꿈 금지 에서 표시 줄 문자열/폭/높이/y/수가 같다', () => {
    // 한 글자 폭 1 인 글꼴에서 원본 규칙을 문자열 인덱스로 다시 쓴 독립 모형이다(코드 구조를 옮기지 않고 규칙 문장으로 구현).
    function model(inputLines, width, noWrap) {
        const lines = []; const newLine = () => { const y = lines.length ? lines.at(-1).y + lines.at(-1).height : 0; const line = { pieces: [], width: 0, height: 0, y }; lines.push(line); return line; };
        let line = null;
        for (const input of inputLines) {
            line = newLine();
            for (const { text, font } of input) {
                const high = 10 + font;
                if (line.width + text.length < width || noWrap) { line.pieces.push(text); line.height = Math.max(line.height, high); line.width += text.length; continue; }
                let position = 0;
                // 폭에 닿는 단어 앞까지의 접두(뒤 공백 포함)를 한 조각으로 받아들이고, 못 받으면 현재 줄이 비었을 때 첫 단어를 강제 배치한다.
                while (position < text.length) {
                    let accepted = 0; let wrap = false; let copy = true;
                    while (position + accepted < text.length) {
                        const space = text.indexOf(' ', position + accepted); const stop = space < 0 ? text.length : space; const prefix = stop - position;
                        if (line.width + prefix >= width) { wrap = true; if (!accepted) { if (line.width) copy = false; else accepted = prefix + 1; } break; }
                        accepted = prefix + 1;
                    }
                    if (copy) { const piece = text.slice(position, position + accepted); line.pieces.push(piece); line.height = Math.max(line.height, high); line.width += piece.length; position += accepted; }
                    if (wrap) line = newLine();
                }
            }
        }
        return lines;
    }
    let seed = 20261114; const random = () => { seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; return seed / 4294967296; };
    const pick = (count) => Math.floor(random() * count); let compared = 0; let wrapped = 0; let forced = 0;
    // 서로 다른 폭/글꼴/공백 개수의 단락 수백 개를 같은 입력으로 비교한다.
    for (let round = 0; round < 600; round += 1) {
        const width = 3 + pick(24); const noWrap = random() < 0.12 ? 1 : 0; const inputs = [];
        for (let lineIndex = 0, total = 1 + pick(4); lineIndex < total; lineIndex += 1) {
            const fragmentsOfLine = [];
            for (let part = 0, count = 1 + pick(3); part < count; part += 1) {
                let text = ''; for (let word = 0, n = pick(6); word < n; word += 1) { text += ' '.repeat(word && random() < 0.8 ? 1 + pick(3) : (random() < 0.15 ? 1 : 0)) + 'abcdefghijklmnop'.slice(0, 1 + pick(11)); }
                if (random() < 0.25) text += ' '.repeat(1 + pick(2));
                fragmentsOfLine.push({ text, font: pick(3) });
            }
            inputs.push(fragmentsOfLine);
        }
        const s = rig({ maxLines: 16 }); s.state.noWrap = noWrap;
        // 한 입력 줄에 여러 조각이 생기도록 문맥을 바꿔 연속 추가한 뒤 LF 로 닫는다.
        for (const input of inputs) { for (const { text, font } of input) s.buffer.appendText(bytes(text), words(font)); s.buffer.appendText(bytes('\n'), words()); }
        s.buffer.layout(width);
        const expected = model(inputs.map((input) => [...input.filter((part) => part.text.length > 0), { text: '', font: 0 }]), width, noWrap);
        const actual = soft(s);
        assert.deepEqual(actual.map((l) => ({ pieces: l.pieces, width: l.width, height: l.height, y: l.y })), expected.map((l) => ({ pieces: l.pieces, width: l.width, height: l.height, y: l.y })), `라운드 ${round} 폭 ${width} noWrap ${noWrap} ${JSON.stringify(inputs)}`);
        assert.equal(s.state.softCount, expected.length); assert.equal(s.buffer.maximumWidth(), Math.max(0, ...expected.map((l) => l.width))); assert.equal(s.buffer.totalHeight(), expected.length ? expected.at(-1).y + expected.at(-1).height : 0);
        compared += 1; if (actual.length > inputs.length) wrapped += 1; if (actual.some((l) => l.pieces.some((p) => p.trimEnd().length >= width))) forced += 1;
    }
    assert.equal(compared, 600); assert.ok(wrapped > 150, `줄바꿈 ${wrapped}건`); assert.ok(forced > 20, `강제 배치 ${forced}건`);
});

test('경계 보강: 새 상태 기본값·호스트가 깨뜨린 상태 거부·기본 태그 0·명시 길이 밖 LF 무시·removeLine 마지막 허용 칸', () => {
    // 새 저장소 값은 모두 0(다음 표시 줄 토큰만 1)이고 아직 초기화 전이다.
    const fresh = new StyleTextBufferState();
    assert.deepEqual([fresh.maxLines, fresh.lineCount, fresh.softCount, fresh.reserved, fresh.dirty, fresh.noWrap, fresh.nextSoftToken, fresh.initialized, fresh.onAppend, fresh.hardLines.length, fresh.softLines.size], [0, 0, 0, 0, 0, 0, 1, false, null, 0, 0]);
    assert.deepEqual(Object.values(new StyleTextBufferLine()), [0, 0, 0, 0, 0, 0]); assert.deepEqual(Object.values(fresh.sentinel), [0, 0, 0, 0, 0, 0]);
    // 초기화된 저장소의 센티널 필드도 모두 0 이다.
    const s = rig(); assert.deepEqual(Object.values(s.state.sentinel), [0, 0, 0, 0, 0, 0]);
    // 저장소 검사는 세 조건을 각각 독립으로 거부한다(호스트가 상태를 깨뜨린 경우).
    const broken = (patch) => { const t = rig(); patch(t.state); return t; };
    assert.throws(() => broken((x) => { x.initialized = false; }).buffer.appendText(bytes('a'), words()), /저장소 없음/);
    assert.throws(() => broken((x) => { x.maxLines = 0; x.hardLines = new Uint32Array(0); }).buffer.appendText(bytes('a'), words()), /저장소 없음/);
    assert.throws(() => broken((x) => { x.hardLines = new Uint32Array(3); }).buffer.appendText(bytes('a'), words()), /저장소 없음/);
    // 끝 칸 인덱스가 범위 밖이면 어느 쪽이든 거부한다(원본 순서상 조각 생성이 먼저라 조각은 남을 수 있다).
    for (const lineCount of [-1, 8]) { const t = broken((x) => { x.lineCount = lineCount; }); assert.throws(() => t.buffer.appendText(bytes('abc'), words()), /끝 줄 범위/); assert.throws(() => t.buffer.appendPicture(t.pictures, 5, 6), /끝 줄 범위/); }
    // 기본 태그는 0 이다(글자/그림).
    const d = rig(); d.buffer.appendText(bytes('x\ny'), words()); d.buffer.appendPicture(d.pictures, 5, 6);
    assert.deepEqual([d.fragments.fragments.get(d.state.hardLines[0]).metadata, d.fragments.fragments.get(d.state.hardLines[1]).metadata, d.fragments.fragments.get(d.fragments.fragments.get(d.state.hardLines[1]).next).metadata], [0, 0, 0]);
    // 명시 길이 밖의 LF 는 읽지 않는다(길이 2 이면 'ab' 만 끝 칸에 남는다).
    const e = rig(); e.buffer.appendText(bytes('ab\ncd'), words(), 2); assert.deepEqual([e.state.lineCount, hard(e, 0)], [0, ['ab']]);
    // removeLine: 완료 줄과 끝 칸 사이의 마지막 허용 칸(maxLines-2)도 삭제할 수 있다.
    // 호스트가 완료 줄 수를 최대 칸까지 키운 경우 마지막 칸 삭제를 거부한다(삭제 가능한 마지막 칸은 maxLines-2).
    const g = rig({ maxLines: 4 }); g.state.lineCount = 4; assert.throws(() => g.buffer.removeLine(3), /삭제 완료 줄 범위/);
    const f = rig({ maxLines: 4 }); f.buffer.appendText(bytes('a\nb\nc\nd'), words()); const tokens = Array.from(f.state.hardLines); f.buffer.removeLine(2); assert.deepEqual(Array.from(f.state.hardLines), [tokens[0], tokens[1], tokens[3], 0]); assert.equal(f.state.lineCount, 2);
});

test('layout 경계 보강: 폭 1·표시 줄 없는 재배치 뒤 센티널 0·반납한 토큰 건너뛰기·호스트가 키운 완료 줄 수 거부·그림 폭 비교 경계', () => {
    const s = rig(); s.buffer.appendText(bytes('ab\n'), words()); s.buffer.layout(1);
    assert.deepEqual(soft(s).map((l) => [l.pieces, l.width]), [[['ab'], 2], [[], 0]]);
    // 완료 줄이 없어지면 재배치 결과는 비어 있고 센티널 링크도 0 이다.
    s.buffer.removeLine(0); s.buffer.layout(5); assert.deepEqual(soft(s), []); assert.deepEqual([s.state.sentinel.next, s.state.sentinel.previous, s.state.softCount], [0, 0, 0]);
    // 외부 토큰 1 만 있으면 첫 줄은 2 를 받는다(한 칸씩 건너뛴다).
    const t = rig(); t.buffer.appendText(bytes('a\nb\n'), words()); t.state.softLines.set(1, new StyleTextBufferLine()); t.buffer.layout(50); assert.deepEqual(soft(t).map((l) => l.token), [2, 3]);
    // 호스트가 완료 줄 수를 최대 칸보다 크게 만들면 배치/소멸 모두 같은 지점에서 거부한다.
    const u = rig({ maxLines: 3 }); u.buffer.appendText(bytes('a\n'), words()); u.state.lineCount = 4; assert.throws(() => u.buffer.layout(10), /완료 줄 수 범위/);
    const v = rig({ maxLines: 3 }); v.buffer.appendText(bytes('a\n'), words()); v.state.lineCount = 4; assert.throws(() => v.buffer.destroy(), /소멸 완료 줄 수/);
    // 그림 폭 비교: 현재 줄 폭과 그림 폭의 합이 제한 폭과 같으면 새 줄로 가고(>=), 높이(120)가 아닌 폭(130)으로 비교한다.
    const p = (width) => { const x = rig(); x.buffer.appendText(bytes('abc'), words()); x.buffer.appendPicture(x.pictures, 5, 6); x.buffer.appendText(bytes('\n'), words()); x.buffer.layout(width); return soft(x).map((l) => l.pieces); };
    assert.deepEqual(p(134), [['abc', '[1]', '']]); assert.deepEqual(p(133), [['abc'], ['[1]', '']]); assert.deepEqual(p(125), [['abc'], ['[1]']]);
    // 빈 줄의 그림은 폭과 같아도(130) 폭보다 커도(129) 새 줄을 만들지 않고 폭이 130 보다 클 때도 마찬가지다. 높이(120)보다 작은 폭 125 에서도 강제 배치한다.
    const q = (width) => { const x = rig(); x.buffer.appendPicture(x.pictures, 5, 6); x.buffer.appendText(bytes('\n'), words()); x.buffer.layout(width); return soft(x).map((l) => [l.pieces, l.width]); };
    for (const width of [125, 129, 130, 131]) assert.deepEqual(q(width), width > 130 ? [[['[1]', ''], 130]] : [[['[1]'], 130]], String(width));
});

test('분할 경계 보강: 끝 공백 뒤 문자열은 빈 줄을 더 만들지 않고 명시 길이의 내장 NUL 은 첫 NUL 에서 끊는다', () => {
    const run = (text, width) => { const s = rig(); s.buffer.appendText(bytes(`${text}\n`), words()); s.buffer.layout(width); return soft(s).map((l) => [l.pieces, l.width]); };
    // 끝 공백까지 폭 안에 들어가면(8 < 9 아님: 8 == 8 이라 분할 경로) 조각 하나로 끝나고 빈 줄은 생기지 않는다.
    assert.deepEqual(run('abc def ', 8), [[['abc def '], 8]]);
    assert.deepEqual(run('abc def  ', 9), [[['abc def  '], 9]]);
    // 명시 길이로 받은 문자열에 NUL 이 들어 있어도 저장 문자열은 첫 NUL 앞까지가 본문이다.
    const s = rig(); s.buffer.appendText(Uint8Array.from([97, 98, 0, 99, 100, 10]), words(), 6); s.buffer.layout(2);
    assert.deepEqual(soft(s).map((l) => [l.pieces, l.width]), [[['ab'], 2], [[], 0]]);
    const t = rig(); t.buffer.appendText(Uint8Array.from([97, 98, 0, 99, 100, 10]), words(), 6); t.buffer.layout(10); assert.deepEqual(soft(t).map((l) => [l.pieces, l.width]), [[['ab'], 2]]);
});

test('bindStyleTextBufferFonts: 명시 길이/NUL 측정은 실제 GumpText 첫 폭 표와 논리 슬롯 높이를 그대로 쓴다', () => {
    const text = new GumpText(stoneFonts(true)); const hooks = {}; bindStyleTextBufferFonts(hooks, text); const context = words(0, 17);
    const input = bytes('AAAA');
    assert.deepEqual([0, 1, 2, 3, 4, -1].map((length) => hooks.measureText(input, context, length)), [[0, 14], [2, 14], [4, 14], [6, 14], [8, 14], [8, 14]]);
    // 폭/높이를 기본 0 에서 시작하지 않고 측정 결과로 채운다(빈 문자열도 높이 14).
    assert.deepEqual(hooks.measureText(bytes(''), context, -1), [0, 14]);
});
