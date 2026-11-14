// 전용 PE fixture가 없는 StyleTextBufferStore·ScrollingTextGumpWidgets(다중 줄 생성/소유/입력/프레임 드래그)를 기준선 소스 계약과 실제 TextGump/StyleTextBuffer/GumpLifetime 연결로 검증한다. (2026-11-14 WB04-E427)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(ScrollingTextGumpWidgets.cpp)에서 옮겼다. 트리/사건/입력/Slider 생성자는 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { StyleTextBufferStore, ScrollingTextGumpWidgets, ScrollingTextGumpWidgetState, bindScrollingTextBuffers, bindScrollingTextGumpWidgets } from '../www/ui/scrolling-text-gump-widgets.js';
import { ScrollingTextGump, ScrollingTextGumpNode } from '../www/ui/text-gump.js';
import { ScrollGumpNode } from '../www/ui/scroll-gump.js';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpDrawState } from '../www/render/gump-draw.js';
import { StyleText, StyleTextState } from '../www/ui/style-text-fragments.js';
import { StyleTextBufferState } from '../www/ui/style-text-buffer.js';
import { InputCode } from '../www/core/input-queue.js';

// 사건 ID 여섯 개(임의 값)와 토큰: 1 루트·3 다중 줄 노드·4 Slider. 새 객체 할당은 기본으로 [21(버퍼), 4(Slider)] 순서다. 글자 폭 1·줄 높이 10 인 독립 글꼴이다.
const EVENTS = { changeEvent: 0x2001, stepEvent: 0x2002, refreshEvent: 0x2003, cursorEvent: 0x2004, leftTagEvent: 0x2005, rightTagEvent: 0x2006 };
function scene(options = {}) {
    const create = new GumpCreateState(); create.screenWidth = 640; create.screenHeight = 480;
    const nodes = new Map([[1, new GumpCreateNode()], [3, new ScrollingTextGumpNode()], [4, new ScrollGumpNode()]]); const lookup = (token) => nodes.get(token);
    const factory = new GumpCreate(create, { node: lookup, allocate: () => 1 }); factory.initializeRoot(); factory.construct(4, 0, 9, 100, 1);
    const fragments = new StyleTextState(); const log = []; const released = [];
    const measure = (text, context, length) => { const nul = text.indexOf(0); const limit = nul < 0 ? text.length : nul; return [length < 0 ? limit : Math.min(length, limit), 10 + context[1]]; };
    const styleHooks = { textMeasure: (text, context, output) => { const size = measure(text, context, -1); output.width = size[0]; output.height = size[1]; }, textMetrics: (context) => [10 + context[1], 8 + context[1]], release: (token) => log.push(['release', token]) };
    const style = new StyleText(fragments, styleHooks);
    const store = new StyleTextBufferStore(fragments, style, { measureText: (text, context, length) => measure(text, context, length) }, (token) => { released.push(token); log.push(['free', token]); });
    const lifeState = new GumpLifetimeState(); const lifeHooks = { node: lookup, release: (token) => log.push(['lifeRelease', token]) }; const lifetime = new GumpLifetime(create, lifeState, lifeHooks);
    const collection = new GumpCollection(create.collection, { node: lookup }); const clip = new GumpDrawState(); const slider = []; const sliderState = { position: 0 };
    const scrollHooks = { node: lookup, baseNode: lookup, buffer: (token) => store.binding(token), sliderRange: (...a) => slider.push(['range', ...a]), sliderPage: (...a) => slider.push(['page', ...a]), sliderSetPosition: (token, position) => { slider.push(['set', token, position]); sliderState.position = position; }, sliderPosition: () => sliderState.position, redraw: (token, erase) => log.push(['redraw', token, erase]) };
    const text = new ScrollingTextGump(create, collection, clip, fragments, style, scrollHooks);
    const tokens = options.tokens ?? [21, 4]; const state = Object.assign(new ScrollingTextGumpWidgetState(), EVENTS, { sliderType: 7, sliderFrame: 8 });
    const tree = { attach: (...a) => log.push(['tree', ...a]) }; const events = { bubble: (...a) => { log.push(['bubble', ...a]); return 0; } }; const input = { capture: (token) => log.push(['capture', token]) };
    const sliderWidgets = { construct: (token, width, height, type, frame) => { log.push(['slider', token, width, height, type, frame]); const node = nodes.get(token); node.baseWidth = width; node.width = width; node.baseHeight = height; node.height = height; node.serial = 900; return token; } };
    const hooks = { node: lookup, slider: lookup, base: lookup, allocate: (bytes) => { log.push(['allocate', bytes]); return tokens.length ? tokens.shift() : 0; }, push: (token) => log.push(['push', token]), attach: (...a) => log.push(['attach', ...a]), redraw: (token, erase) => log.push(['wredraw', token, erase]), cursor: (number) => log.push(['cursor', number]) };
    for (const key of options.omit ?? []) delete hooks[key];
    const widgets = new ScrollingTextGumpWidgets(create, state, factory, tree, lifetime, events, input, fragments, store, text, sliderWidgets, hooks);
    return { create, nodes, factory, fragments, style, styleHooks, store, lifeState, lifeHooks, lifetime, collection, text, state, tree, events, input, sliderWidgets, hooks, widgets, log, released, slider, sliderState, node: nodes.get(3), sliderNode: nodes.get(4) };
}
const only = (log, name) => log.filter((entry) => entry[0] === name);
const bytes = (value) => Uint8Array.from(Buffer.from(value + '\0', 'latin1'));
const words = (font = 0) => Uint32Array.from([0, font, 0, 0, 0, 0, 0, 0]);

test('StyleTextBufferStore: 새 토큰만 등록·반납 경계 필수·외부 상태 빌림/중복 거부·binding 은 같은 상태와 처리기·destroy 순서', () => {
    assert.throws(() => new StyleTextBufferStore(new StyleTextState(), {}, {}, null), /반납 경계/);
    const s = scene(); const store = s.store;
    assert.equal(store.construct(21, 16), 21); const binding = store.binding(21); assert.ok(binding.state instanceof StyleTextBufferState); assert.equal(binding.state.maxLines, 16); assert.equal(store.binding(21).state, binding.state); assert.equal(store.binding(21).handler, binding.handler);
    assert.throws(() => store.construct(21, 16), /새 토큰/); assert.throws(() => store.construct(0, 16), /새 토큰/); assert.throws(() => store.construct(22, 0), /양수 최대 줄 수/); assert.throws(() => store.binding(22), /등록 토큰 없음/); assert.throws(() => store.binding(0), /등록 토큰 없음/);
    // 외부 상태 빌림: 같은 상태 객체를 공유하고(복사 없음) 중복 생성은 거부한다.
    const borrowed = new StyleTextBufferState(); assert.equal(store.constructBorrowed(30, borrowed, 8), 30); assert.equal(store.binding(30).state, borrowed); assert.equal(borrowed.maxLines, 8);
    assert.throws(() => store.constructBorrowed(31, borrowed, 8), /외부 상태 중복/); assert.throws(() => store.constructBorrowed(30, new StyleTextBufferState(), 8), /새 외부 토큰/); assert.throws(() => store.constructBorrowed(0, new StyleTextBufferState(), 8), /새 외부 토큰/);
    // 소멸: 완료 줄/표시 조각 반납 → 호스트 free → 기록 제거(미완료 끝 칸 조각은 남는다).
    binding.handler.appendText(bytes('a\nb\ncd'), words(), -1, 0); binding.handler.layout(50); const before = s.fragments.fragments.size; s.log.length = 0; store.destroy(21);
    assert.deepEqual(only(s.log, 'free'), [['free', 21]]); assert.ok(s.log.findIndex((e) => e[0] === 'release') < s.log.findIndex((e) => e[0] === 'free')); assert.equal(binding.state.initialized, false); assert.throws(() => store.binding(21), /등록 토큰 없음/); assert.ok(s.fragments.fragments.size < before && s.fragments.fragments.size > 0);
    assert.throws(() => store.destroy(21), /등록 토큰 없음/);
});

test('construct: 기본 등록 → 가상 표/signed 종류 → 128칸 버퍼(0x38·소유 1) → float32 크기/문맥/overlay/요청 → 숨긴 Slider(9×100) → 우측 mode 1 부착 → 맨 아래 요청/기본 플래그', () => {
    const s = scene(); s.node.textFlags = 0x41; s.node.dragY = 77; s.node.unwritten = 55; s.node.reserved[0] = 3; s.node.reserved[1] = 4; s.node.callback = 9; s.node.scroll = 5; s.node.metadata = 9;
    assert.equal(s.widgets.construct(3, 120, 80.5 | 0, 2, 9), 3);
    assert.deepEqual(only(s.log, 'allocate'), [['allocate', 0x38], ['allocate', 0xd8]]); assert.deepEqual([s.node.buffer, s.node.ownsBuffer, s.store.binding(21).state.maxLines], [21, 1, 128]);
    assert.deepEqual([s.node.vtable, s.node.command, s.node.reserved, s.node.baseWidth, s.node.width, s.node.baseHeight, s.node.height], [0x51454c, 46, [0, 0], 120, 120, 80, 80]);
    assert.deepEqual(Array.from(s.node.textContext), [0, 2, 9, 0, 0, 0, 1, 1]); assert.deepEqual([s.node.layoutWidth, s.node.scroll, s.node.callback, s.node.metadata], [0, 0, 0, 0]);
    // 미작성 +ac/dragY 와 기존 textFlags 의 다른 비트는 보존하고 slider/scroll 비트만 끄고 두 태그 비트를 켠다.
    assert.deepEqual([s.node.unwritten, s.node.dragY, s.node.textFlags], [55, 77, (0x41 & ~6) | 24]);
    assert.equal(s.node.flags & 0x400a00, 0x400a00); assert.deepEqual([s.node.flags & 0x80000, s.lifeState.overlay.includes(3)], [0x80000, true]);
    // Slider: 9×100 생성(생성 인자 type/frame 은 전역 색 번호)·표시 비트 해제·우측 mode 1 부착(x = 저장한 폭 - Slider 기본 폭, y 0)·맨 아래 요청.
    assert.deepEqual(only(s.log, 'slider'), [['slider', 4, 9, 100, 7, 8]]); assert.equal(s.node.slider, 4); assert.equal(s.sliderNode.flags & 1, 0);
    assert.deepEqual(only(s.log, 'attach'), [['attach', 4, 111, 0, 3, 1]]); assert.deepEqual([s.node.scrollRequest, s.node.bottomPadding], [-2, 0]); assert.deepEqual(only(s.log, 'wredraw'), []); assert.deepEqual(only(s.log, 'redraw'), [['redraw', 3, false]]);
    // 큰 정수/음수 폭은 float32 로 저장하고 배율은 저장 전 정수에 곱한다.
    const t = scene(); t.widgets.construct(3, 16777217, -4, 0, 0); assert.deepEqual([t.node.baseWidth, t.node.baseHeight], [16777216, -4]); assert.deepEqual(only(t.log, 'attach')[0].slice(2, 3), [Math.fround(16777216 - 9)]);
    // signed 종류 BYTE.
    const u = scene(); u.state.alignment = -3; u.widgets.construct(3, 1, 1, 0, 0); assert.equal(u.node.command, 0xfffffffd);
});

test('construct 경계: 할당/Slider 실패와 경계 없음·소유 버퍼는 Gump 등록 수를 늘리지 않는다', () => {
    const noBuffer = scene({ tokens: [] }); assert.throws(() => noBuffer.widgets.construct(3, 10, 10, 0, 0), /버퍼 할당 실패/); assert.deepEqual([noBuffer.node.buffer, noBuffer.node.ownsBuffer], [0, 1]); assert.equal(only(noBuffer.log, 'allocate').length, 1);
    const noSlider = scene({ tokens: [21] }); assert.throws(() => noSlider.widgets.construct(3, 10, 10, 0, 0), /Slider 노드 없음/); assert.equal(noSlider.node.slider, 0); assert.equal(only(noSlider.log, 'slider').length, 0);
    assert.throws(() => scene({ omit: ['allocate'] }).widgets.construct(3, 10, 10, 0, 0), /할당 경계/); assert.throws(() => scene({ omit: ['attach'] }).widgets.construct(3, 10, 10, 0, 0), /Attach 경계/); { const bare = scene({ omit: ['redraw'] }); bare.widgets.construct(3, 10, 10, 0, 0); assert.throws(() => bare.widgets.handle(3, EVENTS.refreshEvent, 0, 0), /Redraw 경계/); assert.equal(bare.node.textFlags & 1, 1); }
    const s = scene(); const count = s.create.count; s.widgets.construct(3, 10, 10, 0, 0); assert.equal(s.create.count, count + 1);
    const hooks = (omit) => { const base = { node: (t) => s.nodes.get(t), slider: (t) => s.nodes.get(t), base: (t) => s.nodes.get(t) }; delete base[omit]; return base; };
    for (const missing of ['node', 'slider', 'base']) assert.throws(() => new ScrollingTextGumpWidgets(s.create, s.state, s.factory, s.tree, s.lifetime, s.events, s.input, s.fragments, s.store, s.text, s.sliderWidgets, hooks(missing)), /공유 저장소/, missing);
    assert.throws(() => s.widgets.construct(99, 1, 1, 0, 0), /공유 노드 없음/);
});

test('attach: 기본 부착 → Slider Push → 높이/표시 비트(slider+scroll 모두일 때만 visible) → 우측 부착 → 대기 요청이 없을 때만 현재 scroll 요청 → 폭 변경 dirty', () => {
    const s = scene(); s.widgets.construct(3, 120, 80, 0, 0); s.log.length = 0; s.node.scrollRequest = -1; s.node.scroll = 6; s.node.textFlags = 0; s.node.layoutWidth = 120; s.sliderNode.scale = 0.5; s.sliderNode.flags = 0;
    s.widgets.attach(3, 11.5, 22.5, 1, 1);
    assert.deepEqual(s.log.slice(0, 3), [['tree', 3, 11.5, 22.5, 1, 1], ['push', 4], ['attach', 4, 111, 0, 3, 1]]); assert.deepEqual([s.sliderNode.baseHeight, s.sliderNode.height, s.sliderNode.flags & 1], [80, 40, 0]);
    assert.equal(s.node.scrollRequest, 6); assert.equal(s.node.textFlags & 1, 0); assert.equal(s.node.layoutWidth, 120);
    // slider(2)와 scroll(4)가 모두 켜지면 visible, 하나만이면 숨김이다. 대기 요청이 있으면 scroll 요청을 새로 만들지 않는다.
    for (const [flags, visible] of [[6, 1], [2, 0], [4, 0], [0, 0]]) { s.node.textFlags = flags; s.sliderNode.flags = 1 - visible; s.node.scrollRequest = 3; s.widgets.attach(3, 0, 0, 1, 0); assert.equal(s.sliderNode.flags & 1, visible, String(flags)); assert.equal(s.node.scrollRequest, 3); }
    // 폭이 바뀌면 dirty 를 켜고 저장하며, 이전 폭이 NaN 이어도 다른 값으로 취급한다(패치).
    s.node.layoutWidth = 100; s.node.textFlags = 0; s.widgets.attach(3, 0, 0, 1, 0); assert.deepEqual([s.node.textFlags & 1, s.node.layoutWidth], [1, 120]);
    s.node.layoutWidth = NaN; s.node.textFlags = 0; s.widgets.attach(3, 0, 0, 1, 0); assert.deepEqual([s.node.textFlags & 1, s.node.layoutWidth], [1, 120]);
    s.node.baseWidth = NaN; s.node.layoutWidth = NaN; s.node.textFlags = 0; s.widgets.attach(3, 0, 0, 1, 0); assert.equal(s.node.textFlags & 1, 1);
    { const bare = scene({ omit: ['push'] }); bare.widgets.construct(3, 120, 80, 0, 0); assert.throws(() => bare.widgets.attach(3, 0, 0, 1, 0), /Push 경계/); assert.equal(only(bare.log, 'tree').length, 1); }
});

test('handle Slider/갱신/커서: 변화 사건은 Slider 위치로 스크롤 요청·step 사건 ±5/±높이·갱신 사건은 dirty+redraw·커서 사건은 태그 16/스크롤 15/기본 1', () => {
    const s = scene(); s.widgets.construct(3, 40, 25, 0, 0); s.log.length = 0; const serial = s.sliderNode.serial;
    // 변화 사건: 발신 Slider serial 이 맞아야 하고 현재 Slider position 으로 스크롤을 요청한다.
    s.sliderNode.position = 13; assert.equal(s.widgets.handle(3, EVENTS.changeEvent, serial, 0), 0); assert.equal(s.node.scrollRequest, 13); assert.throws(() => s.widgets.handle(3, EVENTS.changeEvent, serial + 1, 0), /변화 사건 발신 Slider 불일치/);
    // step 사건: 1 위(+5) 2 아래(-5) 3 페이지(+기본 높이) 4 페이지(-기본 높이) 그 밖은 무시. 상대 요청은 현재 scroll 에 더하고 음수는 0.
    s.node.scroll = 20; for (const [second, expected, result] of [[1, 25, 1], [2, 15, 1], [3, 45, 1], [4, 0, 1], [5, 25, 0], [0, 25, 0]]) { s.node.scrollRequest = 25; assert.equal(s.widgets.handle(3, EVENTS.stepEvent, serial, second), result, String(second)); if (result) assert.equal(s.node.scrollRequest, expected, String(second)); else assert.equal(s.node.scrollRequest, 25); }
    s.node.baseHeight = 25.75; s.widgets.handle(3, EVENTS.stepEvent, serial, 3); assert.equal(s.node.scrollRequest, 45); s.widgets.handle(3, EVENTS.stepEvent, serial, 4); assert.equal(s.node.scrollRequest, 0); s.node.baseHeight = 25;
    s.node.scrollRequest = -1; s.widgets.handle(3, EVENTS.stepEvent, serial + 5, 1); assert.equal(s.node.scrollRequest, -1);
    // 갱신 사건: dirty + redraw(false). 반환 0.
    s.node.textFlags = 0; s.log.length = 0; assert.equal(s.widgets.handle(3, EVENTS.refreshEvent, 0, 0), 0); assert.equal(s.node.textFlags & 1, 1); assert.deepEqual(only(s.log, 'wredraw'), [['wredraw', 3, false]]);
    // 커서 사건: 기본 1·scroll 가능하고 page<범위이면 15·태그 조각 위이면 16 을 선택 경계로 보내고 1 을 반환한다.
    s.log.length = 0; s.node.textFlags = 0; assert.equal(s.widgets.handle(3, EVENTS.cursorEvent, 6, 9), 1); assert.deepEqual(only(s.log, 'cursor'), [['cursor', 1]]);
    Object.assign(s.sliderNode, { minimum: 0, maximum: 30, page: 10 }); s.node.textFlags = 4; s.log.length = 0; s.widgets.handle(3, EVENTS.cursorEvent, 6, 9); assert.deepEqual(only(s.log, 'cursor'), [['cursor', 15]]);
    s.sliderNode.page = 30; s.log.length = 0; s.widgets.handle(3, EVENTS.cursorEvent, 6, 9); assert.deepEqual(only(s.log, 'cursor'), [['cursor', 1]]); assert.throws(() => scene({ omit: ['cursor'] }).widgets.handle(3, EVENTS.cursorEvent, 0, 0), /커서 선택 경계/);
});

test('handle 태그 선택: 줄 위 조각의 T 태그로 커서 16·왼쪽 태그 사건·오른쪽 태그 사건과 태그 없는 클릭은 캡처/드래그 시작', () => {
    const s = scene(); s.widgets.construct(3, 40, 25, 0, 0); s.node.x = 5; s.node.y = 7; s.node.parent = 1; s.node.textFlags = 0; s.sliderNode.serial = 900;
    // 태그 7 인 'ab' 와 태그 0 인 'cdef' 두 줄이다. 보기는 (5,7)~(45,32)이고 줄 높이 10 이다.
    s.text.appendText(3, bytes('ab\n'), words(), -1, 7); s.text.appendText(3, bytes('cdef\n'), words(), -1, 0); s.text.ensureLayout(3); s.node.scrollRequest = -1; s.node.textFlags = 8 | 16; s.log.length = 0;
    const tagged = s.store.binding(21).state.sentinel.next; const taggedFragment = s.store.binding(21).state.softLines.get(tagged).head;
    // 커서: 태그 위는 16, 태그 없는 줄은 1.
    s.widgets.handle(3, EVENTS.cursorEvent, 6, 9); s.widgets.handle(3, EVENTS.cursorEvent, 6, 19); assert.deepEqual(only(s.log, 'cursor'), [['cursor', 16], ['cursor', 1]]);
    // 왼쪽 버튼: 태그 위의 누름은 왼쪽 태그 사건(serial, 태그 값)을 부모에 보내고 1, release 는 아무 것도 보내지 않는다. 캡처/드래그는 시작하지 않는다.
    s.log.length = 0; assert.equal(s.widgets.handle(3, InputCode.leftButton, 6, 9), 1); assert.deepEqual(only(s.log, 'bubble'), [['bubble', 3, EVENTS.leftTagEvent, s.node.serial, 7]]); assert.equal(only(s.log, 'capture').length, 0);
    s.log.length = 0; assert.equal(s.widgets.handle(3, (InputCode.leftButton | InputCode.release) >>> 0, 6, 9), 1); assert.deepEqual(s.log, []); assert.equal(s.fragments.fragments.has(taggedFragment), true);
    // 태그 없는 줄(또는 왼쪽 태그 비트 없음)의 누름은 y 를 저장하고 통지 비트를 켜고 캡처한다. release 는 통지 비트를 끈다.
    s.log.length = 0; s.widgets.handle(3, InputCode.leftButton, 6, 19); assert.deepEqual([s.node.dragY, s.node.flags & 0x100], [19, 0x100]); assert.deepEqual(only(s.log, 'capture'), [['capture', 3]]); s.widgets.handle(3, (InputCode.leftButton | InputCode.release) >>> 0, 6, 19); assert.equal(s.node.flags & 0x100, 0);
    s.node.textFlags = 16; s.log.length = 0; s.widgets.handle(3, InputCode.leftButton, 6, 9); assert.equal(only(s.log, 'capture').length, 1); assert.equal(only(s.log, 'bubble').length, 0);
    // 오른쪽 버튼: 오른쪽 태그 비트가 있어야 하고 태그 위의 누름만 사건을 보내 1 을 반환한다. 그 밖은 0 이다(태그 값 0 도 조각이 있으면 사건을 보낸다).
    s.node.textFlags = 16; s.log.length = 0; assert.equal(s.widgets.handle(3, InputCode.rightButton, 6, 9), 1); assert.deepEqual(only(s.log, 'bubble'), [['bubble', 3, EVENTS.rightTagEvent, s.node.serial, 7]]);
    s.log.length = 0; assert.equal(s.widgets.handle(3, InputCode.rightButton, 6, 19), 1); assert.deepEqual(only(s.log, 'bubble'), [['bubble', 3, EVENTS.rightTagEvent, s.node.serial, 0]]);
    s.log.length = 0; assert.equal(s.widgets.handle(3, InputCode.rightButton, 6, 40), 0); assert.equal(s.widgets.handle(3, (InputCode.rightButton | InputCode.release) >>> 0, 6, 9), 0); s.node.textFlags = 0; assert.equal(s.widgets.handle(3, InputCode.rightButton, 6, 9), 0); assert.deepEqual(s.log, []);
    // 모르는 사건은 0 이다.
    assert.equal(s.widgets.handle(3, 0x777777, 6, 9), 0);
});

test('broadcast/설정 함수: 프레임 드래그는 scroll 비트가 있을 때만 y 차이의 반대 방향 상대 요청·선택/slider/wrap/scrollable/교체/소멸', () => {
    const s = scene(); s.widgets.construct(3, 40, 25, 0, 0); s.node.scrollRequest = -1; s.node.scroll = 10; s.node.dragY = 100;
    // scroll 비트가 없으면 이전 y 도 갱신하지 않는다. 있으면 차이의 반대 방향으로 현재 scroll 에 더한다(음수는 0).
    s.node.textFlags = 0; s.widgets.broadcast(3, 0, 0, 90); assert.deepEqual([s.node.dragY, s.node.scrollRequest], [100, -1]);
    s.node.textFlags = 4; s.widgets.broadcast(3, 0, 0, 94); assert.deepEqual([s.node.dragY, s.node.scrollRequest], [94, 16]); s.widgets.broadcast(3, 0, 0, 94); assert.equal(s.node.scrollRequest, 16); s.widgets.broadcast(3, 0, 0, 120); assert.deepEqual([s.node.dragY, s.node.scrollRequest], [120, 0]);
    // 입력 제외 비트: 비0 이면 해제, 0 이면 설치한다.
    s.node.flags = 0x200; s.widgets.setSelectable(3, 1); assert.equal(s.node.flags & 0x200, 0); s.widgets.setSelectable(3, 0); assert.equal(s.node.flags & 0x200, 0x200); s.widgets.setSelectable(3, -2); assert.equal(s.node.flags & 0x200, 0);
    // slider 비트는 값의 최하위 비트다. slider 만 켜지면 scroll 도 켠다. void 가 아닌 노드는 현재 Push 뒤 현재 x/y 로 자신을 Attach 한다.
    s.node.textFlags = 0; s.node.flags = 8; s.log.length = 0; s.widgets.setSlider(3, 3); assert.equal(s.node.textFlags, 6); assert.equal(s.log.length, 0);
    s.node.flags = 0; s.node.x = 3.5; s.node.y = 4.5; s.log.length = 0; s.widgets.setSlider(3, 0); assert.equal(s.node.textFlags, 4); assert.deepEqual(s.log, [['push', 3], ['attach', 3, 3.5, 4.5, 0, 0]]);
    s.node.textFlags = 4; s.widgets.setSlider(3, 2); assert.equal(s.node.textFlags, 4); s.widgets.setSlider(3, 1); assert.equal(s.node.textFlags, 6); s.node.textFlags = 0x41; s.widgets.setSlider(3, 1); assert.equal(s.node.textFlags, 0x47);
    // scroll 비트를 끄면서 slider 가 켜져 있으면 slider 도 끈다.
    s.node.textFlags = 6; s.log.length = 0; s.widgets.setScrollable(3, 0); assert.equal(s.node.textFlags, 0); assert.equal(only(s.log, 'attach').length, 1); s.widgets.setScrollable(3, 5); assert.equal(s.node.textFlags, 4); s.node.textFlags = 2; s.widgets.setScrollable(3, 1); assert.equal(s.node.textFlags, 6);
    // 줄바꿈 설정은 현재 버퍼의 noWrap 만 쓴다(변경 플래그/redraw 호출 없음).
    s.log.length = 0; s.widgets.setWrap(3, 0); assert.equal(s.store.binding(21).state.noWrap, 1); s.widgets.setWrap(3, 5); assert.equal(s.store.binding(21).state.noWrap, 0); assert.deepEqual(s.log, []);
    // 교체: null 거부·소유한 현재 버퍼는 소멸/free·소유 DWORD 는 그대로 쓴다.
    assert.throws(() => s.widgets.replaceBuffer(3, 0, 1), /null일 수 없음/); s.store.construct(22, 8); s.log.length = 0; s.widgets.replaceBuffer(3, 22, 0); assert.deepEqual([s.node.buffer, s.node.ownsBuffer], [22, 0]); assert.deepEqual(only(s.log, 'free'), [['free', 21]]);
    s.store.construct(23, 8); s.log.length = 0; s.widgets.replaceBuffer(3, 23, 0xffffffff); assert.deepEqual([s.node.buffer, s.node.ownsBuffer], [23, 0xffffffff]); assert.deepEqual(only(s.log, 'free'), []);
    // 소멸: 소유한 버퍼만 소멸/free·포인터 0 이고 빌린 버퍼는 보존한다. 기본 scalar 소멸에 위임한다.
    const t = scene(); t.widgets.construct(3, 40, 25, 0, 0); t.node.vtable = 0; t.log.length = 0; assert.equal(t.widgets.deleteScalar(3, 1), 3); assert.deepEqual([t.node.vtable === 0x50ac70, t.node.buffer], [true, 0]); assert.deepEqual(t.released, [21]); assert.deepEqual(only(t.log, 'lifeRelease'), [['lifeRelease', 3]]);
    const u = scene(); u.widgets.construct(3, 40, 25, 0, 0); u.node.ownsBuffer = 0; u.widgets.deleteScalar(3, 0); assert.deepEqual([u.node.buffer, u.released], [21, []]);
});

test('연결 함수: 버퍼 훅은 저장소 binding·Attach/사건/입력/프레임/소멸은 해당 토큰만 파생 처리기로 이전 처리 보존', () => {
    const s = scene(); const hooks = {}; bindScrollingTextBuffers(hooks, s.store); s.store.construct(21, 4); assert.deepEqual(hooks.buffer(21), s.store.binding(21)); assert.throws(() => hooks.buffer(2), /등록 토큰 없음/);
    const previous = []; const tree = { attach: (...a) => previous.push(['attach', ...a]) }; const events = { handle: (...a) => { previous.push(['events', ...a]); return 77; } }; const input = { handle: (...a) => { previous.push(['input', ...a]); return 88; } }; const frame = { broadcast: (...a) => previous.push(['frame', ...a]) }; const life = { deletingDestructor: (...a) => previous.push(['life', ...a]) };
    const calls = []; const handler = { attach: (...a) => calls.push(['attach', ...a]), handle: (...a) => { calls.push(['handle', ...a]); return 5; }, broadcast: (...a) => calls.push(['broadcast', ...a]), deleteScalar: (...a) => calls.push(['delete', ...a]) };
    bindScrollingTextGumpWidgets(tree, events, input, frame, life, (token) => (token === 3 ? handler : null));
    tree.attach(3, 1, 2, 0, 0, { xBits: 9 }); tree.attach(8, 1, 2, 3, 1, { xBits: 9 }); assert.deepEqual(calls.shift(), ['attach', 3, 1, 2, 0, 0]); assert.deepEqual(previous.shift(), ['attach', 8, 1, 2, 3, 1, { xBits: 9 }]);
    assert.equal(events.handle(3, 4, 5, 6), 5); assert.equal(events.handle(8, 4, 5, 6), 77); assert.equal(input.handle(3, 4, 5, 6), 5); assert.equal(input.handle(8, 4, 5, 6), 88);
    frame.broadcast(3, 1, 2, 3); frame.broadcast(8, 1, 2, 3); life.deletingDestructor(3, 1); life.deletingDestructor(8, 2);
    assert.deepEqual(calls.map((c) => c[0]), ['handle', 'handle', 'broadcast', 'delete']); assert.deepEqual(previous.map((c) => c[0]), ['events', 'input', 'frame', 'life']);
    const bare = [{ attach: undefined }, { handle: undefined }, { handle: undefined }, { broadcast: undefined }, { deletingDestructor: undefined }]; bindScrollingTextGumpWidgets(...bare, () => null);
    assert.throws(() => bare[0].attach(1, 0, 0, 0, 0), /Attach 필요/); assert.throws(() => bare[1].handle(1, 0, 0, 0), /입력 필요/); assert.throws(() => bare[2].handle(1, 0, 0, 0), /입력 필요/); assert.throws(() => bare[3].broadcast(1, 0, 0, 0), /Broadcast 필요/); assert.throws(() => bare[4].deletingDestructor(1, 0), /scalar 필요/);
    assert.throws(() => bindScrollingTextGumpWidgets({}, {}, {}, {}, {}, null), /분류 경계/);
});

test('경계 보강: 전역 기본값·정확한 플래그 값·Slider 노드 불일치·홀짝 좌표·무효 높이의 페이지 요청·dirty 만 있는 노드의 태그 비트', () => {
    // 새 전역의 기본값이다(종류 46·나머지 0).
    assert.deepEqual(Object.values(new ScrollingTextGumpWidgetState()), [46, 0, 0, 0, 0, 0, 0, 0, 0]);
    // 생성 직후 플래그는 기본 (void|보이는) + 0x400a00 + overlay(0x80000) 이고 Slider 는 표시 비트만 꺼져 다른 비트가 그대로다.
    const s = scene(); s.widgets.construct(3, 40, 25, 0, 0); assert.equal(s.node.flags, 0x480a09); assert.equal(s.sliderNode.flags, 8);
    // 입력 제외 비트만 바꾸고 다른 비트는 건드리지 않는다.
    s.node.flags = 0x209; s.widgets.setSelectable(3, 7); assert.equal(s.node.flags, 9); s.widgets.setSelectable(3, 0); assert.equal(s.node.flags, 0x209);
    // slider 가 다른 노드를 돌려주면 거부한다(복사본 금지).
    const t = new ScrollingTextGumpWidgets(s.create, s.state, s.factory, s.tree, s.lifetime, s.events, s.input, s.fragments, s.store, s.text, s.sliderWidgets, { ...s.hooks, slider: (token) => (token === 4 ? new ScrollGumpNode() : s.nodes.get(token)) }); assert.throws(() => t.handle(3, EVENTS.changeEvent, 900, 0), /Slider 노드 없음/);
    // void 가 없고 다른 하위 비트만 있는 노드도 자기 Attach 를 한다(void 판정은 비트 8 만).
    s.node.flags = 1; s.node.textFlags = 0; s.log.length = 0; s.widgets.setSlider(3, 0); assert.deepEqual(only(s.log, 'push'), [['push', 3]]);
    // 방향/태그 비트: dirty(1)만 켜진 노드는 왼쪽/오른쪽 태그를 쓰지 않는다. 오른쪽 버튼은 0, 왼쪽 버튼은 태그 없이 캡처한다.
    const u = scene(); u.widgets.construct(3, 40, 25, 0, 0); u.node.x = 5; u.node.y = 7; u.node.parent = 1; u.text.appendText(3, bytes('ab\n'), words(), -1, 7); u.text.ensureLayout(3); u.node.scrollRequest = -1; u.node.textFlags = 1; u.log.length = 0;
    assert.equal(u.widgets.handle(3, InputCode.rightButton, 6, 9), 0); assert.equal(only(u.log, 'bubble').length, 0);
    assert.equal(u.widgets.handle(3, InputCode.leftButton, 6, 9), 1); assert.equal(only(u.log, 'bubble').length, 0); assert.equal(only(u.log, 'capture').length, 1);
    // 좌표는 정확히 signed 32비트로 전달한다: 짝수 y(16)는 첫 줄(0..9)의 마지막 칸이고 y|1 이면 둘째 줄이다.
    u.node.textFlags = 8; u.log.length = 0; u.widgets.handle(3, EVENTS.cursorEvent, 6, 16); u.widgets.handle(3, EVENTS.cursorEvent, 6, 17); assert.deepEqual(only(u.log, 'cursor'), [['cursor', 16], ['cursor', 1]]);
    // 누름의 이전 y 는 정확한 값(짝수)이다.
    u.node.textFlags = 0; u.widgets.handle(3, InputCode.leftButton, 6, 20); assert.equal(u.node.dragY, 20);
    // 높이가 NaN/무효이면 페이지 이동량은 0 이라 요청은 현재 scroll 이다(1 이 아님).
    u.node.baseHeight = NaN; u.node.scroll = 10; u.widgets.handle(3, EVENTS.stepEvent, u.sliderNode.serial, 3); assert.equal(u.node.scrollRequest, 10); u.widgets.handle(3, EVENTS.stepEvent, u.sliderNode.serial, 4); assert.equal(u.node.scrollRequest, 10);
    // 교체 직후 새 버퍼 포인터가 항상 기록되고(소멸한 옛 토큰이 남지 않음) 소유 DWORD 가 그대로다.
    const v = scene(); v.widgets.construct(3, 40, 25, 0, 0); v.store.construct(22, 4); v.widgets.replaceBuffer(3, 22, 1); assert.deepEqual([v.node.buffer, v.node.ownsBuffer], [22, 1]);
});

test('경계 보강 2: 통지 비트와 Slider 표시 비트는 다른 플래그 비트를 건드리지 않는다', () => {
    const s = scene(); s.widgets.construct(3, 40, 25, 0, 0); s.node.x = 5; s.node.y = 7; s.node.parent = 1; s.node.scrollRequest = -1; s.node.textFlags = 0; const before = s.node.flags;
    s.widgets.handle(3, InputCode.leftButton, 6, 20); assert.equal(s.node.flags, (before | 0x100) >>> 0); s.widgets.handle(3, (InputCode.leftButton | InputCode.release) >>> 0, 6, 20); assert.equal(s.node.flags, before);
    // Slider 표시 비트: 켤 때와 끌 때 다른 비트(0x8)를 그대로 둔다.
    s.sliderNode.flags = 9; s.node.textFlags = 0; s.widgets.attach(3, 0, 0, 1, 0); assert.equal(s.sliderNode.flags, 8);
    s.sliderNode.flags = 8; s.node.textFlags = 6; s.widgets.attach(3, 0, 0, 1, 0); assert.equal(s.sliderNode.flags, 9);
});
