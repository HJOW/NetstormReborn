// 전용 PE fixture가 없는 다중 줄 스타일 파서(ScrollingTextGumpMarkup)를 기준선 소스 계약과 실제 TextGump/StyleText/StyleTextMarkup/StyleTextBuffer 연결로 검증한다. (2026-11-14 WB04-E424)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(ScrollingTextGumpMarkup.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ScrollingTextGumpMarkup } from '../www/ui/scrolling-text-gump-markup.js';
import { StyleTextMarkup, StyleTextMarkupState, emptyStyleTextMarkupType } from '../www/ui/style-text-markup.js';
import { StyleTextFragmentKind as K } from '../www/ui/style-text-fragments.js';
import { scene, bytes, textOf, only } from './text-gump-support.mjs';

// 파서와 대괄호 처리기를 같은 장면(노드 3, 글꼴 1·색 7 기본 문맥)에 묶는다. 타입 5 는 프레임 20개이고 그림 SHP 높이는 12 다.
function parserScene() {
    const s = scene(); s.multi.textContext.set([0, 1, 7, 0, 0, 0, 1, 1]);
    const types = new Map([[5, { ...emptyStyleTextMarkupType(), frameCount: 20, defaultFrame: 4, gumpFrame: 8, letterKinds: 3, first: [10, 14, 17, ...new Array(13).fill(-1)] }]]);
    const sounds = []; const markupState = new StyleTextMarkupState(); const hooks = { type: (type) => types.get(type) ?? emptyStyleTextMarkupType(), findType: () => -1, typeCount: () => 200, frameHeight: () => 12, black: () => 0x21, sound: (name) => sounds.push(name) };
    const markup = new StyleTextMarkup(s.fragments, s.pictures, markupState, hooks);
    const parser = new ScrollingTextGumpMarkup(s.scrolling, s.style, s.pictures, markup, { node: (token) => s.nodes.get(token), base: (token) => s.nodes.get(token) });
    return { ...s, markup, markupState, markupHooks: hooks, sounds, parser };
}
// 끝 칸(아직 LF 를 받지 않은 줄)의 조각 목록이다(문자열/그림, 문맥, 태그).
function tail(s) { const out = []; for (let token = s.bufferState.hardLines[s.bufferState.lineCount]; token; token = s.fragments.fragments.get(token).next) { const node = s.fragments.fragments.get(token); out.push({ text: node.kind === K.TEXT ? textOf(node) : '[pic]', context: Array.from(node.context), metadata: node.metadata }); } return out; }

test('생성자/노드 검증: 두 조회 경계가 필수이고 기본/파생 주소가 같은 노드만 허용한다', () => {
    const s = parserScene(); const make = (hooks) => new ScrollingTextGumpMarkup(s.scrolling, s.style, s.pictures, s.markup, hooks);
    assert.throws(() => make({ node: () => null }), /공유 노드 경계/); assert.throws(() => make({ base: () => null }), /공유 노드 경계/); assert.throws(() => make(undefined), /공유 노드 경계/);
    assert.throws(() => s.parser.parse(99, 'a'), /공유 노드 없음/); assert.throws(() => s.parser.parse(0, 'a'), /공유 노드 없음/);
    const split = make({ node: (token) => s.nodes.get(token), base: () => new s.multi.constructor() }); assert.throws(() => split.parse(3, 'a'), /공유 노드 없음/);
    assert.equal(s.fragments.fragments.size, 0);
});

test('Parse: 구간마다 appendText → dirty → redraw(false) 이고 길이 -1/명시 길이·현재 태그·복사한 문맥으로 이어 붙인다', () => {
    const s = parserScene(); s.parser.parse(3, 'abc');
    assert.deepEqual(only(s.calls, 'append'), [['append', -1, 0]]); assert.deepEqual(tail(s), [{ text: 'abc', context: [0, 1, 7, 0, 0, 0, 1, 1], metadata: 0 }]); assert.equal(s.multi.textFlags & 1, 1); assert.deepEqual(only(s.slider, 'redraw'), [['redraw', 3, false]]);
    // 짧은 명령으로 문맥만 바꾸고 노드의 기본 문맥은 바꾸지 않는다(복사본을 쓴다).
    const t = parserScene(); t.parser.parse(3, 'a~Bb~Ic~Nd');
    assert.deepEqual(tail(t).map((x) => [x.text, x.context[3]]), [['a', 0], ['b', 2], ['c', 1], ['d', 0]]); assert.deepEqual(only(t.calls, 'append'), [['append', 1, 0], ['append', 1, 0], ['append', 1, 0], ['append', -1, 0]]); assert.equal(only(t.slider, 'redraw').length, 4); assert.deepEqual(Array.from(t.multi.textContext), [0, 1, 7, 0, 0, 0, 1, 1]);
    // 첫 NUL 까지만 처리하고 Uint8Array 입력도 같다.
    const u = parserScene(); u.parser.parse(3, 'ab\0cd'); assert.deepEqual(tail(u).map((x) => x.text), ['ab']); const v = parserScene(); v.parser.parse(3, Uint8Array.from([97, 126, 66, 98, 0, 99])); assert.deepEqual(tail(v).map((x) => [x.text, x.context[3]]), [['a', 0], ['b', 2]]);
    // 빈 문자열도 마지막 빈 입력(문자열 통지/dirty/redraw)을 수행한다.
    // 첫 글자가 NUL 이면 아무것도 읽지 않고, 첫 글자가 `~` 명령이면 앞 구간(길이 0)은 만들지 않는다(문자열 통지/redraw 는 명령 뒤 구간만).
    const n = parserScene(); let seen = 0; n.bufferState.onAppend = () => { seen += 1; }; n.parser.parse(3, '\0~Bx'); assert.deepEqual(tail(n), []); assert.equal(seen, 1); const m = parserScene(); seen = 0; m.bufferState.onAppend = () => { seen += 1; }; m.parser.parse(3, '~Bx'); assert.deepEqual(tail(m).map((y) => [y.text, y.context[3]]), [['x', 2]]); assert.equal(seen, 1); assert.equal(only(m.slider, 'redraw').length, 1);
    const w = parserScene(); let notices = 0; w.bufferState.onAppend = () => { notices += 1; }; w.parser.parse(3, ''); assert.equal(notices, 1); assert.deepEqual(only(w.slider, 'redraw'), [['redraw', 3, false]]); assert.deepEqual(tail(w), []);
});

// 명시 UTF-16은 한글과 보조 평면 코드 단위·리터럴·태그를 같은 조각에 보존한다.
test('Parse UTF-16: 한글·서로게이트·스타일 명령의 코드 단위를 보존한다', () => {
    const s = parserScene(), source = '한~B글~~끝~[T7]🙂\0';
    const input = Uint16Array.from({ length: source.length }, (_value, index) => source.charCodeAt(index)); s.parser.parse(3, input);
    const nodes = []; let token = s.bufferState.hardLines[0];
    // 같은 버퍼 체인의 실제 UTF-16 조각을 순서대로 읽는다.
    while (token) { const node = s.fragments.fragments.get(token); nodes.push(node); token = node.next; }
    assert.ok(nodes.every((node) => node.text instanceof Uint16Array));
    assert.deepEqual(nodes.map((node) => String.fromCharCode(...node.text.subarray(0, node.text.indexOf(0)))), ['한', '글', '~', '끝', '🙂']);
    assert.deepEqual(nodes.map((node) => node.context[3]), [0, 2, 2, 2, 2]); assert.equal(nodes.at(-1).metadata, 7);
});

test('Parse 짧은 명령: 색은 StyleText 팔레트 장치·`~.` `~n` 은 현재 기본 문맥 재조회·제한(callback)이면 글꼴 0/1/3/4 무시·모르는 명령은 `~` 만 버린다', () => {
    const s = parserScene(); s.parser.parse(3, 'a~kb~.c~nd');
    assert.deepEqual(tail(s).map((x) => [x.text, x.context[2]]), [['a', 7], ['b', 107], ['c', 7], ['d', 7]]);
    // 문자열 통지가 노드의 기본 문맥을 바꾸면 이후 `~.`/`~n` 은 현재 값을 읽는다(처음 복사한 값이 아니다).
    const t = parserScene(); t.bufferState.onAppend = () => { t.multi.textContext[2] = 55; t.multi.textContext[1] = 6; }; t.parser.parse(3, 'a~kb~.c~kd~ne');
    assert.deepEqual(tail(t).map((x) => [x.text, x.context[1], x.context[2]]), [['a', 1, 7], ['b', 1, 107], ['c', 6, 55], ['d', 6, 107], ['e', 6, 55]]);
    // 제한 모드(+d4 콜백 비0)에서는 글꼴 0/1/3/4 를 무시하고 5/6 은 허용한다.
    const u = parserScene(); u.multi.callback = 1; u.parser.parse(3, '~3a~5b'); assert.deepEqual(tail(u).map((x) => [x.text, x.context[1]]), [['a', 1], ['b', 5]]);
    const v = parserScene(); v.parser.parse(3, '~3a~5b'); assert.deepEqual(tail(v).map((x) => [x.text, x.context[1]]), [['a', 3], ['b', 5]]);
    // 모르는 명령은 `~` 한 글자만 버리고 뒤 글자는 본문으로 이어진다. 끝 `~` 는 마지막 빈 입력을 남긴다.
    const w = parserScene(); let notices = 0; w.bufferState.onAppend = () => { notices += 1; }; w.parser.parse(3, 'a~?b'); assert.deepEqual(tail(w).map((x) => x.text), ['a', '?b']); assert.equal(notices, 2);
    const x = parserScene(); notices = 0; x.bufferState.onAppend = () => { notices += 1; }; x.parser.parse(3, 'ab~'); assert.deepEqual(tail(x).map((y) => y.text), ['ab']); assert.equal(notices, 2); assert.equal(only(x.slider, 'redraw').length, 2);
    // `~~` 리터럴도 길이 1 의 버퍼 입력(통지와 redraw 포함)이다.
    const y = parserScene(); notices = 0; y.bufferState.onAppend = () => { notices += 1; }; y.parser.parse(3, 'x~~y'); assert.deepEqual(tail(y).map((z) => z.text), ['x', '~', 'y']); assert.equal(notices, 3); assert.equal(only(y.slider, 'redraw').length, 3); assert.deepEqual(only(y.calls, 'append'), [['append', 1, 0], ['append', 1, 0], ['append', -1, 0]]);
});

test('Parse 대괄호: 닫는 `]` 가 있을 때만 대괄호 처리기를 부르고 태그/소리/그림을 현재 노드에 연결한다(미닫힘은 `~[` 만 소비)', () => {
    const s = parserScene(); s.parser.parse(3, '~[T7]ab'); assert.equal(s.multi.metadata, 7); assert.deepEqual(tail(s).map((x) => [x.text, x.metadata]), [['ab', 7]]);
    // 태그는 호출 간 유지된다(+c8).
    s.parser.parse(3, 'cd'); assert.deepEqual(tail(s).map((x) => [x.text, x.metadata]), [['ab', 7], ['cd', 7]]);
    // 그림: 현재 태그로 추가하고 `#` 맞춤은 65×60 을 전달한다. 제한 모드가 아니면 0×0 이다.
    const t = parserScene(); t.parser.parse(3, '~[T9]x~[I5.3]y~[I5.*#]'); assert.deepEqual(only(t.calls, 'picture'), [['picture', 9, 0, 0], ['picture', 9, 65, 60]]);
    assert.deepEqual(tail(t).map((p) => [p.text, p.metadata]), [['x', 9], ['[pic]', 9], ['y', 9], ['[pic]', 9]]); assert.equal(t.pictureState.pictures.size, 2);
    // 제한 모드(+d4 콜백 비0): 높이 12 ≤ 16 인 그림은 99999×12 로 맞춘다. 콜백은 그림 검사 시점의 현재 값을 읽는다.
    const u = parserScene(); u.multi.callback = 1; u.parser.parse(3, '~[I5.3]'); assert.deepEqual(only(u.calls, 'picture'), [['picture', 0, 99999, 12]]);
    const v = parserScene(); v.markupHooks.frameHeight = () => 17; v.multi.callback = 1; v.parser.parse(3, 'a~[I5.3]b'); assert.deepEqual(tail(v).map((p) => p.text), ['a', 'b']); assert.deepEqual(only(v.calls, 'picture'), []);
    // 소리는 이름 재생 장치로, 모르는 대괄호는 아무것도 하지 않고 `]` 까지 소비한다.
    const w = parserScene(); w.parser.parse(3, 'a~[S.click]b~[Q]c'); assert.deepEqual(w.sounds, ['click']); assert.deepEqual(tail(w).map((p) => p.text), ['a', 'b', 'c']);
    // 닫히지 않은 대괄호는 `~[` 두 글자만 버리고 나머지를 본문으로 추가한다.
    const x = parserScene(); x.parser.parse(3, 'a~[Ib'); assert.deepEqual(tail(x).map((p) => p.text), ['a', 'Ib']);
    // 대괄호 안의 그림 오류(80바이트 프레임 식)는 그대로 전파되고 이미 추가한 구간은 남는다.
    const y = parserScene(); assert.throws(() => y.parser.parse(3, `a~[I5.1${'x'.repeat(79)}]b`), RangeError); assert.deepEqual(tail(y).map((p) => p.text), ['a']);
    // E 명령은 이 파서의 복사한 문맥에만 쓴다: 이후 구간이 그림자 비트/좌표를 가진다.
    const z = parserScene(); z.parser.parse(3, 'a~[E1.2]b'); assert.deepEqual(tail(z).map((p) => [p.text, p.context[0], p.context[6], p.context[7]]), [['a', 0, 1, 1], ['b', 2, 1, 2]]);
});
