// 전용 PE fixture가 없는 TextGump(한 줄)·ScrollingTextGump(다중 줄 표시/스크롤)를 기준선 소스 계약과 실제 StyleTextBuffer/GumpCreate/Lifetime/Collection/Clip 연결로 검증한다. (2026-11-14 WB04-E423)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(TextGump.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { TextGump, TextGumpNode, TextGumpState, ScrollingTextGump, ScrollingTextGumpNode, bindTextGumpDraw, bindTextGumpLifetime } from '../www/ui/text-gump.js';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpDrawState, GumpDraw, setGumpDrawClip, GUMP_DRAW_CLIP } from '../www/render/gump-draw.js';
import { StyleText, StyleTextState, StyleTextFragmentKind as K } from '../www/ui/style-text-fragments.js';
import { StyleTextBuffer, StyleTextBufferState } from '../www/ui/style-text-buffer.js';
import { Markup, MarkupState, bindMarkup } from '../www/ui/markup-picture.js';
import { bindStyleTextBufferMarkup } from '../www/ui/style-text-buffer.js';

// 문자열은 바이트 하나가 문자 하나이며 NUL 로 끝낸다. 8DWORD 문맥은 글꼴 번호만 context[1] 에 둔다.
const bytes = (text) => Uint8Array.from(Buffer.from(text + '\0', 'latin1'));
const words = (font = 0, color = 0) => Uint32Array.from([0, font, color, 0, 0, 0, 0, 0]);
const textOf = (node) => Buffer.from(node.text.subarray(0, node.text.indexOf(0))).toString('latin1');

// 글자 폭 1·높이 10+글꼴·상승 8+글꼴 인 독립 글꼴 장치로 한 줄/다중 줄 처리기를 같은 안정 노드에 묶은 장면이다.
function scene(options = {}) {
    const create = new GumpCreateState(); create.screenWidth = 640; create.screenHeight = 480;
    const nodes = new Map([[1, new GumpCreateNode()], [2, new TextGumpNode()], [3, new ScrollingTextGumpNode()], [4, new GumpCreateNode()]]); const lookup = (token) => nodes.get(token);
    const factory = new GumpCreate(create, { node: lookup, allocate: () => 1 }); factory.initializeRoot();
    const fragments = new StyleTextState(); const events = []; const draws = []; const scaleOnMeasure = { value: null };
    const measure = (text, context, length) => { const nul = text.indexOf(0); const limit = nul < 0 ? text.length : nul; return [length < 0 ? limit : Math.min(length, limit), 10 + context[1]]; };
    const styleHooks = { textMeasure: (text, context, output) => { const size = measure(text, context, -1); output.width = size[0]; output.height = size[1]; if (scaleOnMeasure.value !== null) nodes.get(2).scale = scaleOnMeasure.value; }, textMetrics: (context) => [10 + context[1], 8 + context[1]], textDraw: (text, position, context) => draws.push([Buffer.from(text.subarray(0, text.indexOf(0))).toString('latin1'), { ...position }, context[1]]), release: (token) => events.push(['release', token]) };
    const pictureState = new MarkupState(); pictureState.typeCount = 200; const pictures = new Markup(fragments, pictureState, { type: () => ({ token: 50, shape: 500, frameCount: 100, hotspotX: 0, hotspotY: 0 }), frame: () => ({ width: 30, height: 12, hotspotX: 0, hotspotY: 0, extraX: 0, extraY: 0 }) }); bindMarkup(styleHooks, pictures);
    const style = new StyleText(fragments, styleHooks);
    const lifeState = new GumpLifetimeState(); const released = []; const lifeHooks = { node: lookup, release: (token) => released.push(token) };
    const lifetime = new GumpLifetime(create, lifeState, lifeHooks); lifeHooks.deletingDestructor = (token, flags) => lifetime.deleteScalar(token, flags);
    const collection = new GumpCollection(create.collection, { node: lookup }); const clip = new GumpDrawState(); const textState = new TextGumpState();
    const single = new TextGump(create, textState, factory, lifetime, collection, clip, style, { node: lookup, baseNode: lookup });
    // 연결 버퍼는 실제 StyleTextBuffer 이며 호출 기록을 남기는 얇은 대리 객체로 감싼다.
    const bufferState = new StyleTextBufferState(); const bufferHooks = { measureText: (text, context, length) => measure(text, context, length) }; bindStyleTextBufferMarkup(bufferHooks, pictures);
    const real = new StyleTextBuffer(bufferState, fragments, style, options.maxLines ?? 16, bufferHooks); const calls = [];
    const handler = { layout: (width) => { calls.push(['layout', width]); real.layout(width); }, totalHeight: () => real.totalHeight(), maximumWidth: () => real.maximumWidth(), appendText: (...args) => { calls.push(['append', args[2], args[3]]); real.appendText(...args); }, appendPicture: (...args) => { calls.push(['picture', args[3], args[4], args[5]]); real.appendPicture(...args); }, removeLine: (index) => { calls.push(['remove', index]); real.removeLine(index); } };
    const slider = []; const sliderState = { position: 0, override: null };
    const scrollHooks = { node: lookup, baseNode: lookup, buffer: (token) => { calls.push(['buffer', token]); return { state: bufferState, handler }; },
        sliderRange: (token, minimum, maximum) => slider.push(['range', token, minimum, maximum]), sliderPage: (token, page) => slider.push(['page', token, page]), sliderSetPosition: (token, position) => { slider.push(['set', token, position]); sliderState.position = position; }, sliderPosition: (token) => { slider.push(['get', token]); return sliderState.override ?? sliderState.position; }, redraw: (token, erase) => slider.push(['redraw', token, erase]) }; for (const key of options.omit ?? []) delete scrollHooks[key]; const scrolling = new ScrollingTextGump(create, collection, clip, fragments, style, scrollHooks);
    bindTextGumpLifetime(lifeHooks, (token) => token === 2 ? single : null);
    // 노드 2(한 줄)·3(다중 줄)·4(slider)를 실제 기본 생성으로 등록하고 루트(1) 아래 둔다.
    const root = nodes.get(1); void root; for (const [token, width, height] of [[3, 40, 25], [4, 9, 25]]) { factory.construct(token, 0, width, height, 1); nodes.get(token).parent = 1; }
    const multi = nodes.get(3); multi.x = 5; multi.y = 7; multi.slider = 4; multi.buffer = 9;
    return { create, nodes, factory, fragments, events, draws, style, styleHooks, lifetime, lifeHooks, released, collection, clip, textState, single, scrolling, bufferState, real, handler, calls, slider, sliderState, pictures, pictureState, scaleOnMeasure, multi, slide: nodes.get(4), text: nodes.get(2) };
}
// 한 줄 노드(2)를 만든다.
function made(s, width = 33, height = 17, font = 3, color = 9) { s.single.construct(2, width, height, font, color); s.text.parent = 1; s.text.x = 10; s.text.y = 20; return s.text; }
// 소유 체인의 문자열 목록이다.
function chain(s, head) { const out = []; for (let token = head; token; token = s.fragments.fragments.get(token).next) { const node = s.fragments.fragments.get(token); out.push(node.kind === K.TEXT ? textOf(node) : '[pic]'); } return out; }
// 표시 줄 요약(센티널 순서)이다.
function soft(s) { const out = []; for (let token = s.bufferState.sentinel.next; token; token = s.bufferState.softLines.get(token).next) { const line = s.bufferState.softLines.get(token); out.push({ token, y: line.y, height: line.height, width: line.width, pieces: chain(s, line.head), head: line.head }); } return out; }

test('TextGump.construct: 기본 생성 뒤 가상 표·head 0·signed 종류 BYTE·float32 크기·문맥·클립 플래그를 쓴다', () => {
    const s = scene(); s.text.head = 77; s.text.vtable = 1;
    assert.equal(s.single.construct(2, 33, 17, 3, 9), 2);
    assert.deepEqual([s.text.vtable, s.text.head, s.text.command, s.text.scale, s.text.baseWidth, s.text.width, s.text.baseHeight, s.text.height], [0x5144d4, 0, 45, 1, 33, 33, 17, 17]);
    assert.deepEqual(Array.from(s.text.textContext), [0, 3, 9, 0, 0, 0, 1, 1]); assert.equal(s.text.flags, (1 | 8 | GUMP_DRAW_CLIP) >>> 0);
    assert.equal(s.create.entries.includes(2), true); assert.equal(s.create.count, 4);
    // 종류 BYTE 는 signed 로 확장한다(128 이상은 음수 DWORD).
    for (const [alignment, expected] of [[0, 0], [127, 127], [-3, 0xfffffffd], [200, 0xffffffc8], [255, 0xffffffff]]) { s.textState.alignment = alignment; s.single.construct(2, 1, 1, 0, 0); assert.equal(s.text.command, expected, String(alignment)); }
    // 크기는 float32 로 저장하고 큰 정수는 반올림한다(배율 크기도 같은 값에서 계산).
    s.single.construct(2, 0x7fffffff, -5, 0, 0); assert.deepEqual([s.text.baseWidth, s.text.width, s.text.baseHeight, s.text.height], [2147483648, 2147483648, -5, -5]);
    s.single.construct(2, 16777217, 0, 0xffffffff, 0xfffffffe); assert.deepEqual([s.text.baseWidth, s.text.textContext[1], s.text.textContext[2]], [16777216, 0xffffffff, 0xfffffffe]);
    // 기본 생성은 기존 head 를 반납하지 않는다(소유 체인이 있어도 0 으로 쓴다).
    s.single.construct(2, 5, 5, 0, 0); s.single.appendText(2, bytes('ab'), words()); const kept = s.text.head; s.single.construct(2, 5, 5, 0, 0); assert.equal(s.text.head, 0); assert.equal(s.fragments.fragments.has(kept), true);
    // 공유 노드가 아니거나 없으면 거부한다.
    assert.throws(() => s.single.construct(99, 1, 1, 0, 0), /공유 노드/); assert.throws(() => s.single.construct(0, 1, 1, 0, 0), /공유 노드/);
    assert.throws(() => new TextGump(s.create, s.textState, s.factory, s.lifetime, s.collection, s.clip, s.style, { node: () => null }), /공유 저장소/);
    const split = new TextGump(s.create, s.textState, s.factory, s.lifetime, s.collection, s.clip, s.style, { node: (t) => s.nodes.get(t), baseNode: (t) => (t === 2 ? new TextGumpNode() : s.nodes.get(t)) }); assert.throws(() => split.construct(2, 1, 1, 0, 0), /공유 노드/);
});

test('TextGump.appendText: -1 만 NUL 탐색·길이 0 은 조각 없음·명시 길이는 그대로·잘못된 음수와 범위 거부·태그 0', () => {
    const s = scene(); made(s);
    s.single.appendText(2, bytes('abc'), words(2, 5)); s.single.appendText(2, bytes('xyz'), words(1), 2); s.single.appendText(2, bytes('q'), words(), 0);
    assert.deepEqual(chain(s, s.text.head), ['abc', 'xy']); const first = s.fragments.fragments.get(s.text.head); assert.deepEqual([first.metadata, Array.from(first.context)], [0, [0, 2, 5, 0, 0, 0, 0, 0]]);
    // 문맥은 복사본이다.
    const context = words(7); s.single.appendText(2, bytes('z'), context); context[1] = 9; assert.equal(s.fragments.fragments.get(s.fragments.fragments.get(s.fragments.fragments.get(s.text.head).next).next).context[1], 7);
    // 그 밖의 음수·NUL 없는 입력·범위 밖 명시 길이는 거부하며, 조각을 먼저 만든 뒤 노드를 조회하는 원본 순서를 유지한다.
    assert.throws(() => s.single.appendText(2, bytes('a'), words(), -2), /음수/); assert.throws(() => s.single.appendText(2, Uint8Array.from([97]), words()), /NUL 범위/); assert.throws(() => s.single.appendText(2, bytes('a'), words(), 5), /범위/);
    const before = s.fragments.fragments.size; assert.throws(() => s.single.appendText(99, bytes('a'), words()), /공유 노드/); assert.equal(s.fragments.fragments.size, before + 1);
    // 명시 길이는 중간 NUL 도 복사하며 저장 문자열은 첫 NUL 앞까지가 본문이다.
    const t = scene(); made(t); t.single.appendText(2, Uint8Array.from([97, 0, 98, 0]), words(), 3); assert.deepEqual(chain(t, t.text.head), ['a']);
});

test('TextGump.clear/fit: 소유 체인 scalar 삭제와 측정 뒤 현재 scale 로 float32 배율 크기를 저장한다', () => {
    const s = scene(); made(s); s.single.appendText(2, bytes('abcd'), words(2)); s.single.appendText(2, bytes('ef'), words(0));
    const tokens = [s.text.head, s.fragments.fragments.get(s.text.head).next];
    s.single.fit(2); assert.deepEqual([s.text.baseWidth, s.text.baseHeight, s.text.width, s.text.height], [6, 12, 6, 12]);
    // 여백은 DWORD 합이며 기본 크기를 float32 로 저장한다. 배율 크기는 측정 콜백이 바꾼 현재 scale 에 저장 전 정수 합을 곱한다.
    s.scaleOnMeasure.value = 0.5; s.single.fit(2, 3, -2); assert.deepEqual([s.text.baseWidth, s.text.width, s.text.baseHeight, s.text.height], [9, 4.5, 10, 5]);
    s.scaleOnMeasure.value = null; s.single.fit(2, 0x7fffffff, 0x7fffffff); assert.deepEqual([s.text.baseWidth, s.text.baseHeight], [Math.fround(6 + 0x7fffffff | 0), Math.fround(12 + 0x7fffffff | 0)]);
    s.single.fit(2, 16777211, 0); assert.equal(s.text.baseWidth, 16777216 + 1 - 1 === 16777216 ? Math.fround(16777217) : 0); assert.equal(s.text.width, Math.fround(16777217 * s.text.scale));
    // 빈 체인은 0×0 이다.
    const e = scene(); made(e); e.single.fit(2, 4, 5); assert.deepEqual([e.text.baseWidth, e.text.baseHeight], [4, 5]);
    s.events.length = 0; s.single.clear(2); assert.deepEqual(s.events, [['release', tokens[0]], ['release', tokens[1]]]); assert.equal(s.text.head, 0); assert.equal(s.fragments.fragments.has(tokens[0]), false);
    assert.throws(() => s.single.clear(99), /공유 노드/); assert.throws(() => s.single.fit(99), /공유 노드/);
});

test('TextGump.draw: 빈 head 는 아무것도 하지 않고, 내부 클립은 현재 bounds 와 교차하며 현재 head 를 위치에 그리고 예외에도 복구한다', () => {
    const s = scene(); made(s); setGumpDrawClip(s.clip, { left: 0, top: 0, right: 640, bottom: 480 });
    s.single.draw(2); assert.deepEqual(s.draws, []);
    s.single.appendText(2, bytes('abc'), words(1, 4)); s.single.appendText(2, bytes('de'), words(2, 5)); s.text.width = 12; s.text.height = 9;
    const inside = []; s.styleHooks.textDraw = (text, position, context) => { inside.push([textOf({ text }), { ...position }, context[1], { ...s.clip.clip }, { ...s.clip.vfxClip }]); };
    s.single.draw(2);
    // 위치 (10,20) 에 글자를 이어 그린다(글자 폭 1 칸씩 3, 2). 최대 상승 10(글꼴 2)에 맞춰 글꼴 1 조각은 한 칸 아래다.
    assert.deepEqual(inside.map((x) => [x[0], x[1], x[2]]), [['abc', { x: 10, y: 21 }, 1], ['de', { x: 13, y: 20 }, 2]]);
    assert.deepEqual(inside[0][3], { left: 10, top: 20, right: 22, bottom: 29 }); assert.deepEqual(inside[0][4], { left: 10, top: 20, right: 21, bottom: 28 });
    assert.deepEqual(s.clip.clip, { left: 0, top: 0, right: 640, bottom: 480 });
    // 출력 중 예외가 나도 클립을 되돌린다.
    s.styleHooks.textDraw = () => { throw new Error('출력 실패'); }; assert.throws(() => s.single.draw(2), /출력 실패/); assert.deepEqual(s.clip.clip, { left: 0, top: 0, right: 640, bottom: 480 });
    // 공통 Draw 의 바깥 클립 플래그가 없어도 한 줄 Draw 자체가 내부 클립을 적용한다(플래그를 지워도 같은 결과).
    s.text.flags = (s.text.flags & ~GUMP_DRAW_CLIP) >>> 0; inside.length = 0; s.styleHooks.textDraw = (text, position) => inside.push([{ ...s.clip.clip }, { ...position }]); s.single.draw(2); assert.deepEqual(inside[0][0], { left: 10, top: 20, right: 22, bottom: 29 });
});

test('TextGump.deleteScalar: 가상 표 복귀·clear 두 번·기본 소멸 위임과 반납 플래그, 파생 분배 연결', () => {
    const s = scene(); made(s); s.single.appendText(2, bytes('ab'), words()); s.single.appendText(2, bytes('cd'), words()); const tokens = [s.text.head, s.fragments.fragments.get(s.text.head).next];
    s.text.vtable = 0; s.events.length = 0; const during = []; s.styleHooks.release = (token) => { during.push(s.text.vtable); s.events.push(['release', token]); };
    assert.equal(s.single.deleteScalar(2, 0), 2); assert.deepEqual(during, [0x5144d4, 0x5144d4]); assert.equal(s.text.vtable, 0x50ac70); assert.deepEqual(s.events, [['release', tokens[0]], ['release', tokens[1]]]); assert.deepEqual(s.released, []);
    // 두 번째 clear 는 이미 비어 추가 반납이 없고, 하위 BYTE 의 bit0 일 때만 객체를 해제한다.
    assert.equal(s.single.deleteScalar(2, 0x100), 2); assert.deepEqual(s.released, []); assert.equal(s.single.deleteScalar(2, 1), 2); assert.deepEqual(s.released, [2]);
    // 연결한 수명 분배는 한 줄 처리기에는 deleteScalar, 다른 토큰에는 이전 분배를 부른다.
    const t = scene(); made(t); t.single.appendText(2, bytes('xy'), words()); const head = t.text.head; t.events.length = 0; t.lifeHooks.deletingDestructor(2, 1); assert.deepEqual(t.events, [['release', head]]); assert.deepEqual(t.released, [2]);
    const other = []; const hooks = { deletingDestructor: (token, flags) => other.push([token, flags]) }; bindTextGumpLifetime(hooks, (token) => (token === 2 ? t.single : null)); hooks.deletingDestructor(3, 5); assert.deepEqual(other, [[3, 5]]);
    assert.throws(() => bindTextGumpLifetime({}, () => null) || ({}).deletingDestructor(3, 0), TypeError); const bare = {}; bindTextGumpLifetime(bare, () => null); assert.throws(() => bare.deletingDestructor(3, 0), /소멸자 처리기/); assert.throws(() => bindTextGumpLifetime({}, null), /수명 분류/);
});

test('bindTextGumpDraw: 한 줄 → 다중 줄 → 이전 분배 순서이며 마지막은 처리기 없음 거부', () => {
    const calls = []; const single = { draw: (t) => calls.push(['single', t]) }; const multi = { draw: (t) => calls.push(['multi', t]) };
    const hooks = { draw: (t) => calls.push(['previous', t]) }; bindTextGumpDraw(hooks, (t) => (t === 1 ? single : null), (t) => (t === 1 || t === 2 ? multi : null));
    hooks.draw(1); hooks.draw(2); hooks.draw(3); assert.deepEqual(calls, [['single', 1], ['multi', 2], ['previous', 3]]);
    const bare = {}; bindTextGumpDraw(bare, () => null, () => null); assert.throws(() => bare.draw(1), /Draw 처리기 없음/); assert.throws(() => bindTextGumpDraw({}, null, () => null), /분류 경계/); assert.throws(() => bindTextGumpDraw({}, () => null, null), /분류 경계/);
    // 실제 GumpDraw 의 kind2 분배에 연결해도 같은 결과다.
    const s = scene(); made(s); s.single.appendText(2, bytes('ab'), words()); const pass = {}; bindTextGumpDraw(pass, (token) => (token === 2 ? s.single : null), (token) => (token === 3 ? s.scrolling : null)); const draw = new GumpDraw(s.collection, s.clip, pass); draw.draw(2); assert.deepEqual(s.draws.map((d) => d[0]), ['ab']);
});

// 표시 줄이 만들어진 다중 줄 장면이다(글자 폭 1·줄 높이 10: aaa/bbbb/ccccc 가 y 0/10/20, 전체 높이 30, 보기 40×25, 위치 5,7).
function filled(s, text = 'aaa\nbbbb\nccccc\n') { s.scrolling.appendTextDefault(3, bytes(text)); s.scrolling.ensureLayout(3); s.calls.length = 0; s.slider.length = 0; return s; }
// 기록에서 한 종류만 거른다.
const only = (log, name) => log.filter((entry) => entry[0] === name);

test('ScrollingTextGump.ensureLayout: 노드/버퍼 dirty 중 하나일 때만 두 플래그를 먼저 비우고 slider 폭을 뺀 현재 폭으로 layout', () => {
    const s = scene(); const seen = []; const layout = s.handler.layout; s.handler.layout = (width) => { seen.push([width, s.bufferState.dirty, s.multi.textFlags & 1]); layout(width); };
    s.scrolling.ensureLayout(3); assert.deepEqual(only(s.calls, 'layout'), []);
    // 문자열 추가는 노드 dirty 를 세우고 redraw(false)를 통지한다. 배치는 폭 40 이다.
    s.scrolling.appendTextDefault(3, bytes('aaa\nbbbb\nccccc\n')); assert.equal(s.multi.textFlags & 1, 1); assert.deepEqual(only(s.slider, 'redraw'), [['redraw', 3, false]]); assert.equal(s.bufferState.dirty, 1);
    s.scrolling.ensureLayout(3); assert.deepEqual(seen, [[40, 0, 0]]); assert.deepEqual([s.bufferState.dirty, s.multi.textFlags & 1], [0, 0]); assert.deepEqual(soft(s).map((l) => [l.y, l.height, l.width, l.pieces]), [[0, 10, 3, ['aaa']], [10, 10, 4, ['bbbb']], [20, 10, 5, ['ccccc']]]);
    // 두 플래그가 모두 꺼져 있으면 다시 배치하지 않는다. 하나만 켜져 있어도 배치한다.
    s.scrolling.ensureLayout(3); assert.equal(seen.length, 1); s.multi.textFlags = 1; s.scrolling.ensureLayout(3); assert.equal(seen.length, 2); s.bufferState.dirty = 1; s.scrolling.ensureLayout(3); assert.equal(seen.length, 3); assert.equal(s.bufferState.dirty, 0);
    // slider 사용 비트(2)이면 현재 slider 의 기본 폭을 빼고 CRT 절삭한다. 소수 폭도 마찬가지다.
    s.multi.textFlags = 3; s.scrolling.ensureLayout(3); assert.equal(seen.at(-1)[0], 31); assert.equal(s.multi.textFlags, 2);
    s.multi.baseWidth = 40.75; s.slide.baseWidth = 9.5; s.multi.textFlags = 3; s.scrolling.ensureLayout(3); assert.equal(seen.at(-1)[0], 31); s.multi.textFlags = 1; s.scrolling.ensureLayout(3); assert.equal(seen.at(-1)[0], 40);
    // 폭이 0 이하이면 이전 표시 목록을 그대로 두고 두 변경 플래그만 비운다.
    const before = soft(s).map((l) => l.token); s.multi.baseWidth = 5; s.slide.baseWidth = 9; s.multi.textFlags = 3; s.scrolling.ensureLayout(3); assert.equal(seen.at(-1)[0], -4); assert.deepEqual(soft(s).map((l) => l.token), before); assert.equal(s.multi.textFlags, 2);
    // 연결 버퍼가 소멸했거나 없으면 거부한다.
    s.real.destroy(); assert.throws(() => s.scrolling.ensureLayout(3), /연결 버퍼 없음/);
});

test('ScrollingTextGump.fitHeight/fitContent: 강제 배치 뒤 높이 +1·임시 폭 10000 배치 뒤 최대 폭+여백+slider 폭+1·배율 크기는 저장 전 합계', () => {
    const s = scene(); s.scrolling.fitHeight(3); assert.deepEqual([s.multi.baseHeight, s.multi.height], [1, 1]);
    filled(s); s.multi.scale = 0.5; s.multi.textFlags = 0; const seen = []; const layout = s.handler.layout; s.handler.layout = (width) => { seen.push([width, s.multi.baseWidth, s.multi.width]); layout(width); };
    s.scrolling.fitHeight(3); assert.deepEqual([s.multi.baseHeight, s.multi.height], [31, 15.5]); assert.deepEqual(seen, [[40, 40, 40]]);
    // 임시 폭 10000 으로 배치하고(배율 폭은 10000×scale), 이후 최대 폭 5 + 여백 8 + 1 = 14 / 전체 높이 30 + 4 = 34 를 저장한다.
    seen.length = 0; s.scrolling.fitContent(3, 8, 4); assert.deepEqual(seen, [[10000, 10000, 5000]]); assert.deepEqual([s.multi.baseWidth, s.multi.width, s.multi.baseHeight, s.multi.height], [14, 7, 34, 17]);
    // slider 사용 비트이면 임시 배치 폭에서 slider 폭을 빼고 결과 폭에는 slider 폭을 더한다. 플래그는 이미 비운 뒤다.
    seen.length = 0; s.multi.scale = 1; s.multi.textFlags = 3; s.scrolling.fitContent(3, 2, 0); assert.deepEqual(seen, [[9991, 10000, 10000]]); assert.deepEqual([s.multi.baseWidth, s.multi.width, s.multi.baseHeight, s.multi.height, s.multi.textFlags], [17, 17, 30, 30, 2]);
    // 크기 합은 저장 전 float 가 아니라 정수/실수 합계에 배율을 곱한다(소수 slider 폭).
    s.slide.baseWidth = 0.1; s.multi.scale = 3; s.multi.textFlags = 3; s.scrolling.fitContent(3, 0, 0); assert.equal(s.multi.baseWidth, Math.fround(5 + Math.fround(0.1) + 1)); assert.equal(s.multi.width, Math.fround((5 + Math.fround(0.1) + 1) * 3));
    assert.throws(() => s.scrolling.fitHeight(99), /공유 노드/); assert.throws(() => s.scrolling.fitContent(0), /공유 노드/);
});

test('ScrollingTextGump 추가/교체: 버퍼 추가 → 현재 dirty 비트 → redraw(false) 순서·현재 문맥 복사·그림은 문자열 통지 없음·교체는 완료 줄만 제거', () => {
    const s = scene(); const order = []; s.slider.push = (entry) => { order.push(entry[0]); return s.slider.length; }; const append = s.handler.appendText; s.handler.appendText = (...args) => { order.push('append'); append(...args); };
    s.scrolling.appendText(3, bytes('x\n'), words(1), -1, 7); assert.deepEqual(order, ['append', 'redraw']); assert.equal(s.multi.textFlags & 1, 1); assert.equal(s.fragments.fragments.get(s.bufferState.hardLines[0]).metadata, 7);
    // 문자열 통지 안에서 노드를 바꿔도 현재 textFlags 에 dirty 를 더한다(추가 전에 읽은 값을 쓰지 않는다).
    s.multi.textFlags = 0; s.bufferState.onAppend = () => { s.multi.textFlags = 2; }; s.scrolling.appendText(3, bytes('y\n'), words(), -1, 0); assert.equal(s.multi.textFlags, 3);
    // 현재 기본 문맥은 추가 전에 복사한다(통지에서 원래 문맥을 바꿔도 이번 입력은 복사본).
    s.multi.textContext.set([0, 2, 6, 0, 0, 0, 1, 1]); s.bufferState.onAppend = () => { s.multi.textContext[1] = 9; }; s.scrolling.appendTextDefault(3, bytes('z'), -1, 4);
    const tail = s.fragments.fragments.get(s.bufferState.hardLines[s.bufferState.lineCount]); assert.deepEqual([Array.from(tail.context), tail.metadata], [[0, 2, 6, 0, 0, 0, 1, 1], 4]); s.bufferState.onAppend = null;
    // 그림은 버퍼 문자열 통지를 호출하지 않고 dirty/redraw 만 갱신한다.
    let notices = 0; s.bufferState.onAppend = () => { notices += 1; }; s.multi.textFlags = 0; order.length = 0; s.scrolling.appendPicture(3, s.pictures, 5, 6, 8, 20, 6); assert.equal(notices, 0); assert.equal(s.multi.textFlags & 1, 1); assert.deepEqual(order, ['redraw']);
    // 교체: 문맥 복사 → 완료 줄 제거 → 일반 문자열 추가(스타일 명령 미해석) → dirty/redraw. 미완료 끝 줄과 표시 복제는 남는다.
    const t = scene(); filled(t, 'aa\nbb\n'); t.scrolling.appendTextDefault(3, bytes('tail')); const displays = soft(t).map((l) => l.token); t.calls.length = 0; t.slider.length = 0; t.multi.textFlags = 0;
    t.scrolling.replaceText(3, bytes('~[T9]xy\n'), 5); assert.deepEqual(only(t.calls, 'remove'), [['remove', 0], ['remove', 0]]); assert.deepEqual(only(t.calls, 'append'), [['append', -1, 5]]);
    assert.equal(t.bufferState.lineCount, 1); assert.deepEqual(chain(t, t.bufferState.hardLines[0]), ['tail', '~[T9]xy']); assert.deepEqual(soft(t).map((l) => l.token), displays); assert.equal(t.multi.textFlags & 1, 1); assert.deepEqual(only(t.slider, 'redraw'), [['redraw', 3, false]]);
    // 필수 redraw 경계가 없으면 추가는 끝난 뒤 거부한다.
    const u = scene({ omit: ['redraw'] }); assert.throws(() => u.scrolling.appendTextDefault(3, bytes('a\n')), /Redraw 경계/); assert.equal(u.multi.textFlags & 1, 1); assert.equal(u.bufferState.lineCount, 1);
});

test('ScrollingTextGump.setScroll/scrollBottom: 현재 높이/뷰포트로 제한한 뒤 slider range→page→position→현재 값 재조회와 노드 재조회 순서', () => {
    const s = filled(scene());
    s.scrolling.setScroll(3, 12); assert.deepEqual(s.slider, [['range', 4, 0, 30], ['page', 4, 25], ['set', 4, 5], ['get', 4]]); assert.equal(s.multi.scroll, 5);
    // 범위 안 위치는 그대로, 음수는 0, slider 가 돌려준 현재 값이 최종 scroll 이다.
    for (const [input, expected] of [[3, 3], [-3, 0], [5, 5], [6, 5], [30, 5]]) { s.slider.length = 0; s.scrolling.setScroll(3, input); assert.equal(s.multi.scroll, expected, String(input)); assert.deepEqual(s.slider[2], ['set', 4, expected]); }
    s.sliderState.override = 2; s.scrolling.setScroll(3, 4); assert.equal(s.multi.scroll, 2); s.sliderState.override = null;
    // 뷰포트(기본 높이)는 CRT 절삭이고 음수는 0 이다. 내용이 더 낮으면 위치는 내용 - 페이지(음수는 0)다.
    s.multi.baseHeight = 25.9; s.slider.length = 0; s.scrolling.setScroll(3, 100); assert.deepEqual([s.slider[1], s.multi.scroll], [['page', 4, 25], 5]);
    s.multi.baseHeight = -3; s.slider.length = 0; s.scrolling.setScroll(3, 100); assert.deepEqual([s.slider[1], s.multi.scroll], [['page', 4, 0], 30]);
    s.multi.baseHeight = 50; s.slider.length = 0; s.scrolling.setScroll(3, 7); assert.deepEqual([s.slider[1], s.multi.scroll], [['page', 4, 50], 0]);
    // 각 호출 직전에 노드/slider 토큰을 다시 읽는다(range 콜백이 slider 를 바꾸면 이후 호출은 새 토큰).
    s.multi.baseHeight = 25; const push = s.slider.push.bind(s.slider); s.slider.length = 0; s.slider.push = (entry) => { push(entry); if (entry[0] === 'range') s.multi.slider = 99; return s.slider.length; }; s.scrolling.setScroll(3, 2);
    assert.deepEqual(s.slider.map((e) => e[1]), [4, 99, 99, 99]); s.multi.slider = 4; delete s.slider.push;
    // 맨 아래: 내용 높이 - 기본 높이(실수)를 절삭한다. 인자는 읽지 않는다.
    s.multi.baseHeight = 25.5; s.scrolling.scrollBottom(3, 1234); assert.equal(s.multi.scroll, 4); s.multi.baseHeight = 25; s.scrolling.scrollBottom(3); assert.equal(s.multi.scroll, 5);
    // 필요한 slider 경계는 도달할 때만 거부하며 앞선 호출은 이미 일어난 뒤다.
    for (const [omit, pattern, expected] of [['sliderRange', /range 경계/, []], ['sliderPage', /page 경계/, ['range']], ['sliderSetPosition', /position 쓰기 경계/, ['range', 'page']], ['sliderPosition', /position 조회 경계/, ['range', 'page', 'set']]]) {
        const t = filled(scene({ omit: [omit] })); assert.throws(() => t.scrolling.setScroll(3, 1), pattern); assert.deepEqual(t.slider.map((e) => e[0]), expected, omit);
    }
});

test('ScrollingTextGump 스크롤 요청: 저장+redraw(false)·상대 요청은 현재 scroll 에 DWORD 합·맨 아래 요청·processRequest 적용과 -1 복귀·atBottom 비교', () => {
    const s = filled(scene());
    s.scrolling.requestScroll(3, 7); assert.equal(s.multi.scrollRequest, 7); assert.deepEqual(s.slider, [['redraw', 3, false]]);
    s.scrolling.requestScroll(3, -5); assert.equal(s.multi.scrollRequest, -5);
    s.multi.scroll = 5; s.scrolling.requestRelativeScroll(3, 4); assert.equal(s.multi.scrollRequest, 9); s.scrolling.requestRelativeScroll(3, -9); assert.equal(s.multi.scrollRequest, 0); s.scrolling.requestRelativeScroll(3, 0x7fffffff); assert.equal(s.multi.scrollRequest, 0);
    s.slider.length = 0; s.scrolling.requestBottom(3, 6); assert.deepEqual([s.multi.scrollRequest, s.multi.bottomPadding], [-2, 6]); assert.deepEqual(s.slider, [['redraw', 3, false]]); s.scrolling.requestBottom(3); assert.equal(s.multi.bottomPadding, 0);
    // -1 은 무동작, -2 는 맨 아래(여백은 읽지 않음), 나머지는 setScroll 뒤 -1 로 쓴다.
    s.slider.length = 0; s.multi.scrollRequest = -1; s.scrolling.processRequest(3); assert.deepEqual(s.slider, []);
    s.multi.scrollRequest = -2; s.scrolling.processRequest(3); assert.deepEqual([s.multi.scroll, s.multi.scrollRequest], [5, -1]); assert.equal(s.slider.length, 4);
    s.multi.scrollRequest = 3; s.slider.length = 0; s.scrolling.processRequest(3); assert.deepEqual([s.multi.scroll, s.multi.scrollRequest, s.slider[2]], [3, -1, ['set', 4, 3]]);
    // 다른 음수 요청은 거부하고 요청을 지우지 않는다. 콜백이 새 요청을 써도 정상 반환 뒤 -1 로 덮고, 예외로 끝나면 덮지 않는다.
    s.multi.scrollRequest = -5; assert.throws(() => s.scrolling.processRequest(3), /음수/); assert.equal(s.multi.scrollRequest, -5);
    const push = s.slider.push.bind(s.slider); s.slider.push = (entry) => { push(entry); if (entry[0] === 'range') s.multi.scrollRequest = 8; return s.slider.length; }; s.multi.scrollRequest = 4; s.scrolling.processRequest(3); assert.equal(s.multi.scrollRequest, -1);
    s.sliderState.override = 6; delete s.slider.push; const u = filled(scene({ omit: ['sliderPage'] })); u.multi.scrollRequest = 2; assert.throws(() => u.scrolling.processRequest(3), /page 경계/); assert.equal(u.multi.scrollRequest, 2);
    // 맨 아래 판정: 내용 - 기본 높이 - 10 <= scroll (동률 포함, NaN 은 거짓).
    const t = filled(scene()); t.multi.baseHeight = 5; for (const [scroll, expected] of [[14, false], [15, true], [16, true]]) { t.multi.scroll = scroll; assert.equal(t.scrolling.atBottom(3), expected, String(scroll)); }
    t.multi.baseHeight = NaN; t.multi.scroll = 1e9; assert.equal(t.scrolling.atBottom(3), false); t.multi.baseHeight = 25; t.multi.scroll = -5; assert.equal(t.scrolling.atBottom(3), true); t.multi.scroll = -6; assert.equal(t.scrolling.atBottom(3), false);
});

test('ScrollingTextGump.hitTest: 현재 half-open 경계·표시 줄/조각 폭·정확한 위 경계 제외·scroll 의 첫 줄·세로 범위는 줄 높이-조각 높이', () => {
    const s = filled(scene()); const lines = soft(s); const clone = (index) => lines[index].head;
    // 보기 경계는 left/top 포함, right/bottom 제외다(5,7)~(45,32).
    for (const [x, y] of [[4, 8], [45, 8], [6, 6], [6, 32]]) assert.equal(s.scrolling.hitTest(3, { x, y }), 0, `${x},${y}`);
    assert.equal(s.scrolling.hitTest(3, { x: 7, y: 9 }), clone(0)); assert.equal(s.scrolling.hitTest(3, { x: 5, y: 8 }), clone(0));
    // 조각의 위쪽 경계(localY 0)는 제외하고 줄 폭 이상 x 는 선택하지 않는다.
    assert.equal(s.scrolling.hitTest(3, { x: 5, y: 7 }), 0); assert.equal(s.scrolling.hitTest(3, { x: 8, y: 9 }), 0); assert.equal(s.scrolling.hitTest(3, { x: 7, y: 9 }), clone(0));
    assert.equal(s.scrolling.hitTest(3, { x: 6, y: 18 }), clone(1)); assert.equal(s.scrolling.hitTest(3, { x: 9, y: 28 }), clone(2)); assert.equal(s.scrolling.hitTest(3, { x: 10, y: 28 }), 0);
    // 뷰포트 높이 25 를 넘는 y 는 보기 밖이고, 보이는 마지막 줄(y 20 < 25)의 아래쪽은 bottom 경계 전까지 선택한다.
    assert.equal(s.scrolling.hitTest(3, { x: 6, y: 31 }), clone(2));
    // scroll 5: 첫 줄이 위로 밀려 y = -5 에서 시작한다.
    s.multi.scroll = 5; assert.equal(s.scrolling.hitTest(3, { x: 6, y: 9 }), clone(0)); assert.equal(s.scrolling.hitTest(3, { x: 6, y: 13 }), clone(1)); assert.equal(s.scrolling.hitTest(3, { x: 6, y: 11 }), clone(0)); assert.equal(s.scrolling.hitTest(3, { x: 6, y: 12 }), 0);
    // scroll 15 는 첫 줄과 둘째 줄의 y+높이가 모두 15 이하/이상인 경계를 확인한다(둘째 줄 끝 20 이상이라 둘째 줄에서 시작).
    s.multi.scroll = 15; assert.equal(s.scrolling.hitTest(3, { x: 6, y: 8 }), clone(1)); s.multi.scroll = 10; assert.equal(s.scrolling.hitTest(3, { x: 6, y: 8 }), clone(1));
    // 높이 조각이 줄보다 낮으면(그림 12) 위쪽 빈 부분은 선택하지 않는다: 세로 유효 범위는 줄 높이 - 조각 높이부터다.
    const p = scene(); p.scrolling.appendTextDefault(3, bytes('ab')); p.scrolling.appendPicture(3, p.pictures, 5, 6); p.scrolling.appendTextDefault(3, bytes('\n')); p.scrolling.ensureLayout(3);
    const row = soft(p)[0]; assert.deepEqual([row.height, row.width], [12, 32]); const [text, picture] = [row.head, p.fragments.fragments.get(row.head).next];
    assert.equal(p.scrolling.hitTest(3, { x: 6, y: 10 }), text); assert.equal(p.scrolling.hitTest(3, { x: 8, y: 8 }), picture); assert.equal(p.scrolling.hitTest(3, { x: 8, y: 7 }), 0);
    // 글자(높이 10)의 위쪽 2 칸은 줄 높이 12 와의 차이라 선택되지 않고, 누적 폭 검사 때문에 그 칸은 뒤의 그림에 돌아간다(원본 알고리즘 그대로).
    assert.equal(p.scrolling.hitTest(3, { x: 6, y: 9 }), picture); assert.equal(p.scrolling.hitTest(3, { x: 6, y: 8 }), picture);
    // 높이가 12 인 그림은 줄 높이 12 와 같아 위쪽 경계(localY 0)만 제외한다.
    assert.equal(p.scrolling.hitTest(3, { x: 6, y: 7 }), 0);
    // scroll 이 마지막 줄을 넘으면 센티널을 넘었다고 거부한다.
    s.multi.scroll = 31; assert.throws(() => s.scrolling.hitTest(3, { x: 6, y: 8 }), /센티널을 넘음/);
    assert.throws(() => s.scrolling.hitTest(99, { x: 6, y: 8 }), /저장소 없음/);
});

test('ScrollingTextGump.draw: layout/스크롤 요청 적용 뒤 내부 클립 안에서 줄마다 최대 상승 기준선으로 조각을 이어 그리고 viewport 끝 동률은 그리지 않는다', () => {
    const s = filled(scene()); setGumpDrawClip(s.clip, { left: 0, top: 0, right: 640, bottom: 480 }); const clips = []; const textDraw = s.styleHooks.textDraw; s.styleHooks.textDraw = (text, position, context) => { clips.push({ ...s.clip.clip }); textDraw(text, position, context); };
    s.scrolling.draw(3); assert.deepEqual(s.draws.map((d) => [d[0], d[1]]), [['aaa', { x: 5, y: 7 }], ['bbbb', { x: 5, y: 17 }], ['ccccc', { x: 5, y: 27 }]]); assert.ok(clips.every((c) => c.left === 5 && c.top === 7 && c.right === 45 && c.bottom === 32));
    assert.deepEqual(s.clip.clip, { left: 0, top: 0, right: 640, bottom: 480 });
    // scroll 5: 첫 줄이 -5 에서 시작해 마지막 줄은 y 15 에서 그려진다.
    s.draws.length = 0; s.multi.scroll = 5; s.scrolling.draw(3); assert.deepEqual(s.draws.map((d) => d[1].y), [2, 12, 22]);
    // viewport 높이 10: 둘째 줄 y 가 10 으로 같아지면 그리지 않는다. 11 이면 그린다.
    s.draws.length = 0; s.multi.scroll = 0; s.multi.baseHeight = 10; s.scrolling.draw(3); assert.equal(s.draws.length, 1); s.draws.length = 0; s.multi.baseHeight = 10.5; s.scrolling.draw(3); assert.equal(s.draws.length, 2); s.multi.baseHeight = 25;
    // 기준선: 한 줄의 조각마다 자신의 상승을 빼고 줄의 최대 상승을 더한다(글꼴 3 의 상승 11, 글꼴 0 의 상승 8).
    const m = scene(); m.scrolling.appendText(3, bytes('ab'), words(0)); m.scrolling.appendText(3, bytes('cd\n'), words(3)); m.scrolling.draw(3);
    assert.deepEqual(m.draws.map((d) => [d[0], d[1], d[2]]), [['ab', { x: 5, y: 10 }, 0], ['cd', { x: 7, y: 7 }, 3]]);
    // 요청한 스크롤은 그리기 직전에 적용한다(slider 호출 뒤 요청은 -1).
    const q = filled(scene()); q.multi.scrollRequest = 4; q.scrolling.draw(3); assert.deepEqual([q.multi.scroll, q.multi.scrollRequest], [4, -1]); assert.deepEqual(q.draws.map((d) => d[1].y), [3, 13, 23]);
    // dirty 이면 그리기 전에 다시 배치한다. 출력 도중 예외가 나도 클립을 복구한다.
    const r = scene(); r.scrolling.appendTextDefault(3, bytes('aa\n')); r.scrolling.draw(3); assert.equal(r.draws.length, 1); r.styleHooks.textDraw = () => { throw new Error('출력 실패'); }; setGumpDrawClip(r.clip, { left: 1, top: 2, right: 300, bottom: 200 }); assert.throws(() => r.scrolling.draw(3), /출력 실패/); assert.deepEqual(r.clip.clip, { left: 1, top: 2, right: 300, bottom: 200 });
    // 빈 줄도 높이만큼 이동하고(빈 문자열 줄) 소멸한 조각을 가리키면 거부한다.
    const t = scene(); t.scrolling.appendTextDefault(3, bytes('\nxy\n')); t.scrolling.draw(3); assert.deepEqual(t.draws.map((d) => [d[0], d[1].y]), [['', 7], ['xy', 17]]);
    const broken = filled(scene()); broken.fragments.fragments.delete(soft(broken)[0].head); assert.throws(() => broken.scrolling.draw(3), /조각 저장소 없음/);
    // 줄 끝점이 scroll 과 같은 첫 줄은 유지해 화면 위 y 로 그리고(-3), 하나 더 내리면 건너뛴다.
    const k = filled(scene()); k.multi.scroll = 10; k.scrolling.draw(3); assert.deepEqual(k.draws.map((d) => [d[0], d[1].y]), [['aaa', -3], ['bbbb', 7], ['ccccc', 17]]); k.draws.length = 0; k.multi.scroll = 11; k.scrolling.draw(3); assert.deepEqual(k.draws.map((d) => [d[0], d[1].y]), [['bbbb', 6], ['ccccc', 16]]);
});

test('경계 보강: 새 노드 기본값·생성 인자 검증·짝수 크기·빈 문자열·기본 인자 전달·0 요청 허용', () => {
    // 새 노드의 기본값(원본 소유 필드)이다.
    assert.equal(new TextGumpNode().head, 0); assert.equal(new TextGumpState().alignment, 45);
    const fresh = new ScrollingTextGumpNode(); assert.deepEqual([fresh.slider, fresh.buffer, fresh.scroll, fresh.scrollRequest, fresh.bottomPadding, fresh.textFlags, fresh.ownsBuffer, fresh.unwritten, fresh.reserved, fresh.layoutWidth, fresh.dragY, fresh.metadata, fresh.callback], [0, 0, 0, -1, 0, 0, 0, 0, [0, 0], 0, 0, 0, 0]);
    // 다중 줄 처리기는 세 조회 경계를 모두 요구하고, 연결 버퍼는 세 조건을 각각 거부한다.
    const s = scene(); const base = { node: (t) => s.nodes.get(t), baseNode: (t) => s.nodes.get(t), buffer: () => ({ state: s.bufferState, handler: s.handler }) };
    for (const missing of ['node', 'baseNode', 'buffer']) { const hooks = { ...base }; delete hooks[missing]; assert.throws(() => new ScrollingTextGump(s.create, s.collection, s.clip, s.fragments, s.style, hooks), /공유 저장소/, missing); }
    for (const bad of [{ state: null, handler: s.handler }, { state: s.bufferState, handler: null }, { state: new StyleTextBufferState(), handler: s.handler }, null]) { const t = new ScrollingTextGump(s.create, s.collection, s.clip, s.fragments, s.style, { ...base, buffer: () => bad }); assert.throws(() => t.ensureLayout(3), /연결 버퍼 없음/); }
    // 한 줄 크기는 짝수/홀수 모두 그대로 저장한다.
    const t = scene(); t.single.construct(2, 34, 18, 4, 6); assert.deepEqual([t.text.baseWidth, t.text.baseHeight], [34, 18]); t.single.construct(2, 2, 4, 0, 0); assert.deepEqual([t.text.baseWidth, t.text.baseHeight, t.text.textContext[1]], [2, 4, 0]);
    // 빈 문자열(NUL 만)은 거부하지 않고 조각도 만들지 않는다. 길이 0 도 같다.
    t.single.appendText(2, bytes(''), words()); t.single.appendText(2, bytes('abc'), words(), 0); assert.equal(t.text.head, 0);
    // 기본 인자: 길이 -1·태그 0·맞춤 0 이 버퍼에 그대로 전달된다.
    const u = scene(); u.scrolling.appendText(3, bytes('a\n'), words()); u.scrolling.appendTextDefault(3, bytes('b\n')); u.scrolling.appendPicture(3, u.pictures, 5, 6); u.scrolling.replaceText(3, bytes('c\n'));
    assert.deepEqual(only(u.calls, 'append'), [['append', -1, 0], ['append', -1, 0], ['append', -1, 0]]); assert.deepEqual(only(u.calls, 'picture'), [['picture', 0, 0, 0]]);
    // fitContent 기본 여백은 0 이다(최대 폭 5 + 1).
    const w = filled(scene()); w.scrolling.fitContent(3); assert.deepEqual([w.multi.baseWidth, w.multi.baseHeight], [6, 30]);
    // 요청 0 은 유효한 위치다(음수만 거부한다).
    const v = filled(scene()); v.multi.scroll = 4; v.multi.scrollRequest = 0; v.scrolling.processRequest(3); assert.deepEqual([v.multi.scroll, v.multi.scrollRequest], [0, -1]);
});

test('ftol 경계: 무효/범위 밖/음수/32비트 초과 폭은 CRT 절삭의 하위 DWORD 로 layout 에 전달된다', () => {
    const s = filled(scene()); s.multi.textFlags = 0; const seen = []; const layout = s.handler.layout; s.handler.layout = (width) => { seen.push(width); layout(width); };
    // 폭 = 기본 폭(float32). 무효(NaN/∞)와 ±2^63 이상은 0, 그 안은 64비트 절삭의 하위 32비트(signed)다.
    for (const [input, expected] of [[NaN, 0], [Infinity, 0], [-Infinity, 0], [3.4e38, 0], [-3.4e38, 0], [2 ** 63, 0], [-(2 ** 63), 0], [2 ** 40 + 2 ** 17, 131072], [-(2 ** 40 + 2 ** 17), -131072], [2 ** 32 + 2 ** 20, 1048576], [-(2 ** 32 + 2 ** 20), -1048576], [2 ** 31, -2147483648], [40.75, 40], [-0.5, 0], [-1.5, -1]]) {
        s.multi.baseWidth = input; s.multi.textFlags = 1; s.scrolling.ensureLayout(3); assert.equal(seen.at(-1), expected, String(input));
    }
    // 같은 절삭을 페이지/스크롤에도 쓴다(페이지는 큰 뷰포트의 하위 DWORD).
    s.multi.baseHeight = 2 ** 32 + 2 ** 20; s.slider.length = 0; s.scrolling.setScroll(3, 0); assert.deepEqual(s.slider[1], ['page', 4, 1048576]); s.multi.baseHeight = NaN; s.slider.length = 0; s.scrolling.setScroll(3, 0); assert.deepEqual(s.slider[1], ['page', 4, 0]);
});

test('hitTest/draw 경계 보강: 좁은 보기 오른쪽 가장자리·스크롤한 위쪽 가장자리·내용이 보기보다 짧을 때 센티널에서 멈춤·조각 경계 누적 폭·상승 0 글꼴', () => {
    const s = filled(scene()); const lines = soft(s); const clone = (index) => lines[index].head;
    // 보기 폭을 3 으로 줄이면 오른쪽 가장자리(x=8)는 보기 밖이다. 줄 폭 4 인 둘째 줄이 있어도 선택하지 않는다.
    s.multi.width = 3; assert.equal(s.scrolling.hitTest(3, { x: 8, y: 18 }), 0); assert.equal(s.scrolling.hitTest(3, { x: 7, y: 18 }), clone(1)); s.multi.width = 40;
    // scroll 5 에서 보기 위쪽(y=7)의 한 칸 위(y=6)는 보기 밖이고 위쪽 경계 자체는 포함한다.
    s.multi.scroll = 5; assert.equal(s.scrolling.hitTest(3, { x: 6, y: 6 }), 0); assert.equal(s.scrolling.hitTest(3, { x: 6, y: 7 }), clone(0)); s.multi.scroll = 0;
    // 내용(두 줄, 높이 20)이 보기(25)보다 짧으면 마지막 줄 뒤 센티널에서 멈추고 아래쪽 빈 곳은 0 이다.
    const t = filled(scene(), 'aaa\nbbbb\n'); assert.equal(t.scrolling.hitTest(3, { x: 6, y: 29 }), 0); assert.equal(t.scrolling.hitTest(3, { x: 6, y: 18 }), soft(t)[1].head);
    // 조각 경계: 글자(폭 2)와 그림(폭 30)이 이어진 줄에서 경계 x(localX 2)는 그림에, 그 앞은 글자에 속한다(누적 폭의 반열린 비교).
    const p = scene(); p.scrolling.appendTextDefault(3, bytes('ab')); p.scrolling.appendPicture(3, p.pictures, 5, 6); p.scrolling.appendTextDefault(3, bytes('\n')); p.scrolling.ensureLayout(3);
    const row = soft(p)[0]; const picture = p.fragments.fragments.get(row.head).next;
    assert.equal(p.scrolling.hitTest(3, { x: 6, y: 11 }), row.head); assert.equal(p.scrolling.hitTest(3, { x: 7, y: 11 }), picture); assert.equal(p.scrolling.hitTest(3, { x: 7, y: 10 }), picture); assert.equal(p.scrolling.hitTest(3, { x: 36, y: 11 }), picture); assert.equal(p.scrolling.hitTest(3, { x: 37, y: 11 }), 0);
    // 상승이 0 인 글꼴(99)만 있는 줄은 초기 상승 0 을 쓴다: 조각 y = 위치 - 0 + (0 + 줄 y).
    const q = scene(); q.styleHooks.textMetrics = (context) => [10 + context[1], context[1] === 99 ? 0 : 8 + context[1]]; q.scrolling.appendText(3, bytes('zz\n'), words(0)); q.scrolling.draw(3); q.draws.length = 0;
    const r = scene(); r.styleHooks.textMetrics = (context) => [10, context[1] === 99 ? 0 : 8]; r.scrolling.appendText(3, bytes('zz\n'), words(99)); r.scrolling.draw(3); assert.deepEqual(r.draws.map((d) => d[1]), [{ x: 5, y: 7 }]);
});
