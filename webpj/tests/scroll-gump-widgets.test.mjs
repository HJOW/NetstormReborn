// 전용 PE fixture가 없는 Slider 전체 생성과 화살표/손잡이 Draw(ScrollGumpWidgets)를 기준선 소스 계약과 손으로 계산한 기하로 검증한다. (2026-11-14 WB04-E426)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(ScrollGumpWidgets.cpp)에서 옮겼다. 돌 버튼은 호출 기록 대역, 배경 생성은 실제 GumpBackgroundCreate 다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ScrollGumpWidgets, ScrollGumpArrowNode, ScrollGumpWidgetState, bindScrollGumpWidgetDraw, bindScrollGumpWidgetLifetime } from '../www/ui/scroll-gump-widgets.js';
import { ScrollGump, ScrollGumpNode, ScrollGumpThumbNode, ScrollGumpState } from '../www/ui/scroll-gump.js';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpBackgroundCreate } from '../www/ui/gump-background-create.js';

// 토큰 10 Slider·11/12 화살표·13 손잡이를 기본 생성 장면에 둔다. 새 객체 할당은 11,12,13 순서로 돌려주고 호출을 기록한다.
function scene(options = {}) {
    const create = new GumpCreateState(); create.screenWidth = 640; create.screenHeight = 480;
    const nodes = new Map([[1, new GumpCreateNode()], [10, new ScrollGumpNode()], [11, new ScrollGumpArrowNode()], [12, new ScrollGumpArrowNode()], [13, new ScrollGumpThumbNode()]]); const lookup = (token) => nodes.get(token);
    const factory = new GumpCreate(create, { node: lookup, allocate: () => 1 }); factory.initializeRoot();
    const log = []; const tokens = options.tokens ?? [11, 12, 13]; const state = new ScrollGumpWidgetState();
    const stones = { construct: (...args) => { log.push(['stone', ...args.slice(0, 5), Array.from(args[5])]); }, draw: (token) => log.push(['stoneDraw', token]) };
    const backgrounds = new GumpBackgroundCreate(create, factory, () => 0, { node: lookup });
    const sliders = { setRange: (...args) => log.push(['range', ...args]), setPage: (...args) => log.push(['page', ...args]), setPosition: (...args) => log.push(['position', ...args]) };
    const lifetime = { deleteScalar: (token, flags) => { log.push(['life', token, flags]); return token; } };
    const hooks = { node: lookup, arrow: lookup, thumb: lookup, base: lookup, allocate: (bytes) => { log.push(['allocate', bytes]); return tokens.length ? tokens.shift() : 0; }, attach: (...args) => log.push(['attach', ...args]) };
    for (const key of options.omit ?? []) delete hooks[key];
    const widgets = new ScrollGumpWidgets(create, state, factory, stones, backgrounds, options.sliders ?? sliders, lifetime, hooks);
    return { create, nodes, factory, log, state, stones, backgrounds, sliders, lifetime, hooks, widgets, slider: nodes.get(10), dec: nodes.get(11), inc: nodes.get(12), thumb: nodes.get(13) };
}
const only = (log, name) => log.filter((entry) => entry[0] === name);

test('생성자/노드 검증: 네 조회 경계가 필수이고 기본/파생 주소가 같은 노드만 허용한다', () => {
    const s = scene(); const build = (hooks) => new ScrollGumpWidgets(s.create, s.state, s.factory, s.stones, s.backgrounds, s.sliders, s.lifetime, hooks);
    for (const missing of ['node', 'arrow', 'thumb', 'base']) { const hooks = { ...s.hooks }; delete hooks[missing]; assert.throws(() => build(hooks), /공유 저장소/, missing); }
    assert.throws(() => s.widgets.construct(99, 9, 9, 0, 0), /Slider 생성 공유 노드 없음/); assert.throws(() => s.widgets.constructArrow(0, 1, 1, 0, 0, 0), /화살표 공유 노드 없음/); assert.throws(() => s.widgets.constructThumb(99, 1, 1, 0, 0), /손잡이 공유 노드 없음/);
    assert.throws(() => s.widgets.deleteScalar(0, 0), /기본 노드 없음/);
    const split = build({ ...s.hooks, base: (token) => (token === 10 ? new ScrollGumpNode() : s.nodes.get(token)) }); assert.throws(() => split.construct(10, 9, 9, 0, 0), /공유 노드 없음/);
});

test('constructArrow: 빈 라벨과 {0,0,color,0,0,0,1,1} 문맥으로 돌 버튼을 만든 뒤 가상 표·팝업 유지 비트·signed 정렬·방향을 쓴다', () => {
    const s = scene(); s.dec.buttonFlags = 0x4; assert.equal(s.widgets.constructArrow(11, 9, 12, 2, 7, 0xfe), 11);
    assert.deepEqual(s.log, [['stone', 11, 9, 12, null, 7, [0, 0, 0xfe, 0, 0, 0, 1, 1]]]);
    assert.deepEqual([s.dec.vtable, s.dec.buttonFlags, s.dec.command, s.dec.direction], [0x5116a8, 0x24, 1, 2]);
    // 정렬 BYTE 는 signed 로 확장한다. 색은 DWORD 로 보관한다.
    for (const [value, expected] of [[0, 0], [-1, 0xffffffff], [127, 127], [200, 0xffffffc8]]) { s.state.arrowAlignment = value; s.widgets.constructArrow(11, 1, 1, 0, 0, 0); assert.equal(s.dec.command, expected, String(value)); }
    s.widgets.constructArrow(11, 1, 1, 0, 0, 0xffffffff); assert.deepEqual(s.log.at(-1).at(-1), [0, 0, 0xffffffff, 0, 0, 0, 1, 1]);
});

test('constructThumb: 기본 배경 (0,0,0x3ec0000,0x201)을 전역 기본 type/frame 으로 만든 뒤 파생 표·rect 0·정렬·인자 사본·float32 크기·키보드 비트를 쓴다', () => {
    const s = scene(); s.thumb.handle = { left: 5, top: 6, right: 7, bottom: 8 }; s.thumb.previousY = 77; s.thumb.previousX = 88; s.state.defaultFrame = 3;
    assert.equal(s.widgets.constructThumb(13, 9, 82, 55, 66), 13);
    // 기본 배경 생성은 전역 기본 type/frame 을 쓰고 손잡이 인자 사본과 구별된다. 이전 y/x 는 미작성이라 그대로다.
    assert.deepEqual([s.thumb.backgroundFlags, s.thumb.type, s.thumb.frame, s.thumb.detailType, s.thumb.detailFrame], [0x03ec0000, 0, 3, 55, 66]);
    assert.deepEqual([s.thumb.vtable, s.thumb.command, s.thumb.handle, s.thumb.previousY, s.thumb.previousX], [0x511668, 51, { left: 0, top: 0, right: 0, bottom: 0 }, 77, 88]);
    assert.deepEqual([s.thumb.baseWidth, s.thumb.width, s.thumb.baseHeight, s.thumb.height, s.thumb.scale], [9, 9, 82, 82, 1]); assert.equal(s.thumb.flags, 0x209);
    // 정렬 BYTE signed·큰 정수 float32 반올림. 배율은 저장 전 정수에 곱한다.
    s.state.thumbAlignment = -2; s.widgets.constructThumb(13, 16777217, -5, 0, 0); assert.deepEqual([s.thumb.command, s.thumb.baseWidth, s.thumb.baseHeight], [0xfffffffe, 16777216, -5]);
    // 기본 배경 type 이 0 이 아니면 자산 경계가 필요하다(기본 배경 처리기가 거부).
    s.state.defaultType = 5; assert.throws(() => s.widgets.constructThumb(13, 1, 1, 0, 0), /배경 타입/);
});

test('construct 세로: 40 기본·방향/step/키보드·float32 크기·두께=폭의 절삭·세 자식 할당/부착 순서와 초기 범위 0..100/page 10/position 50', () => {
    const s = scene(); assert.equal(s.widgets.construct(10, 9, 100, 7, 8), 10);
    assert.deepEqual([s.slider.vtable, s.slider.command, s.slider.scrollFlags, s.slider.step, s.slider.flags & 0x200, s.slider.baseWidth, s.slider.width, s.slider.baseHeight, s.slider.height], [0x5116f8, 39, 0, 1, 0x200, 9, 9, 100, 100]);
    assert.deepEqual([s.slider.decrement, s.slider.increment, s.slider.thumb, s.slider.position], [11, 12, 13, 0]);
    // 할당 크기: 화살표 0x178 두 번·손잡이 0xd8. 화살표 방향은 위 0/아래 2 이고 type/frame 은 unused/color 로 전달된다.
    assert.deepEqual(only(s.log, 'allocate'), [['allocate', 0x178], ['allocate', 0x178], ['allocate', 0xd8]]);
    assert.deepEqual(only(s.log, 'stone'), [['stone', 11, 9, 9, null, 7, [0, 0, 8, 0, 0, 0, 1, 1]], ['stone', 12, 9, 9, null, 7, [0, 0, 8, 0, 0, 0, 1, 1]]]); assert.deepEqual([s.dec.direction, s.inc.direction], [0, 2]);
    assert.deepEqual([s.thumb.baseWidth, s.thumb.baseHeight, s.thumb.detailType, s.thumb.detailFrame], [9, 82, 7, 8]);
    // 부착은 현재 자식 토큰 순서로 mode 0, 증가 버튼 y 는 높이-두께, 손잡이 y 는 두께다. 마지막에 값 통지 순서다.
    assert.deepEqual(only(s.log, 'attach'), [['attach', 11, 0, 0, 10, 0], ['attach', 12, 0, 91, 10, 0], ['attach', 13, 0, 9, 10, 0]]);
    assert.deepEqual(s.log.slice(-3), [['range', 10, 0, 100], ['page', 10, 10], ['position', 10, 50]]);
    // 크기 0 은 축마다 40 으로 바뀐다(둘 다 0 이면 정사각 세로).
    const t = scene(); t.widgets.construct(10, 0, 0, 0, 0); assert.deepEqual([t.slider.baseWidth, t.slider.baseHeight, t.slider.scrollFlags], [40, 40, 0]); assert.deepEqual(only(t.log, 'attach')[1], ['attach', 12, 0, 0, 10, 0]); assert.deepEqual([t.thumb.baseWidth, t.thumb.baseHeight], [40, -40]);
    const u = scene(); u.widgets.construct(10, 0, 100, 0, 0); assert.deepEqual([u.slider.baseWidth, u.slider.baseHeight, u.slider.scrollFlags], [40, 100, 0]); const v = scene(); v.widgets.construct(10, 100, 0, 0, 0); assert.deepEqual([v.slider.baseWidth, v.slider.baseHeight, v.slider.scrollFlags], [100, 40, 1]);
});

test('construct 가로: 높이<폭이면 scrollFlags 1·두께=저장한 높이·화살표 방향 왼쪽 3/오른쪽 1·손잡이 폭=폭-2×두께·x 방향 부착', () => {
    const s = scene(); s.widgets.construct(10, 100, 9, 7, 8); assert.equal(s.slider.scrollFlags, 1);
    assert.deepEqual(only(s.log, 'stone').map((entry) => entry.slice(1, 4)), [[11, 9, 9], [12, 9, 9]]); assert.deepEqual([s.dec.direction, s.inc.direction], [3, 1]); assert.deepEqual([s.thumb.baseWidth, s.thumb.baseHeight], [82, 9]);
    assert.deepEqual(only(s.log, 'attach'), [['attach', 11, 0, 0, 10, 0], ['attach', 12, 91, 0, 10, 0], ['attach', 13, 9, 0, 10, 0]]);
    // 폭과 높이가 같으면 세로다. 소수 크기는 저장 전 정수(원본 인자는 int)라 절삭 후의 값이다.
    const t = scene(); t.widgets.construct(10, 20, 20, 0, 0); assert.equal(t.slider.scrollFlags, 0);
    // 배율은 저장한 float32 높이에 곱한다(다시 읽는 패치 규칙): 초기 scale 은 1 이라 같다.
    const u = scene(); u.widgets.construct(10, 16777217, 16777217, 0, 0); assert.deepEqual([u.slider.baseWidth, u.slider.baseHeight, u.slider.height], [16777216, 16777216, 16777216]);
    // 두께는 세로에서 저장 전 폭(정수)의 CRT 절삭이다. 큰 두께는 손잡이 높이가 음수가 된다(원본 그대로).
    const v = scene(); v.widgets.construct(10, 60, 100, 0, 0); assert.deepEqual([v.thumb.baseWidth, v.thumb.baseHeight], [60, -20]);
});

test('construct 경계: 할당 실패는 자식 토큰 0 → 부착 거부·할당/부착 경계 없음 거부·실제 Slider 로 범위/page/position 이 적용된다', () => {
    const s = scene({ tokens: [] }); assert.throws(() => s.widgets.construct(10, 9, 100, 0, 0), /기본 노드 없음/); assert.deepEqual([s.slider.decrement, s.slider.increment, s.slider.thumb], [0, 0, 0]); assert.equal(only(s.log, 'allocate').length, 3);
    const a = scene({ omit: ['allocate'] }); assert.throws(() => a.widgets.construct(10, 9, 100, 0, 0), /할당 경계/);
    const b = scene({ omit: ['attach'] }); assert.throws(() => b.widgets.construct(10, 9, 100, 0, 0), /Attach 경계/); assert.deepEqual(only(b.log, 'allocate').length, 3);
    // 실제 ScrollGump 를 연결하면 범위 0..100·page 10·position 50 이 저장된다(void 이므로 redraw 만).
    const state = Object.assign(new ScrollGumpState(), { clickEvent: 1, repeatEvent: 2, stepEvent: 3, changeEvent: 4 }); const holder = {};
    const real = scene(); const gump = new ScrollGump(real.create, state, { attach: () => {} }, { bounds: () => ({}) }, { bubble: () => 0 }, { capture: () => {} }, { node: (token) => real.nodes.get(token), thumbNode: (token) => real.nodes.get(token), baseNode: (token) => real.nodes.get(token), push: () => {}, attach: () => {}, redraw: (token, erase) => { holder.redraw = [token, erase]; }, poll: () => ({ code: 0 }) });
    const wired = scene({ sliders: gump }); wired.nodes = real.nodes; void wired; const integrated = new ScrollGumpWidgets(real.create, real.state, real.factory, real.stones, real.backgrounds, gump, real.lifetime, real.hooks);
    integrated.construct(10, 9, 100, 0, 0); assert.deepEqual([real.slider.minimum, real.slider.maximum, real.slider.page, real.slider.position], [0, 100, 10, 50]); assert.deepEqual(holder.redraw, [10, false]);
});

test('drawArrow: 돌 버튼 Draw 뒤 현재 Bounds/기본 문맥 색/방향으로 안쪽 여백 2 삼각형 세 선을 그린다(중점은 0 방향 절삭)', () => {
    const s = scene(); s.dec.textContext.set([0, 0, 77, 0, 0, 0, 1, 1]); const drawn = []; const target = { lines: (lines) => drawn.push(lines) }; const collection = { bounds: (token) => { s.log.push(['bounds', token]); return { left: 10, top: 20, right: 19, bottom: 29 }; } };
    const line = (x1, y1, x2, y2) => ({ first: { x: x1, y: y1 }, last: { x: x2, y: y2 }, color: 77 });
    // 안쪽 사각형 (12,22)-(17,27), 중점 (14,24)다.
    const expected = { 0: [line(12, 27, 17, 27), line(12, 27, 14, 22), line(17, 27, 14, 22)], 1: [line(12, 22, 12, 27), line(12, 22, 17, 24), line(12, 27, 17, 24)], 2: [line(12, 22, 17, 22), line(12, 22, 14, 27), line(17, 22, 14, 27)], 3: [line(17, 22, 17, 27), line(17, 22, 12, 24), line(17, 27, 12, 24)], 9: [line(12, 27, 17, 27), line(12, 27, 14, 22), line(17, 27, 14, 22)] };
    for (const direction of [0, 1, 2, 3, 9]) { s.dec.direction = direction; s.log.length = 0; s.widgets.drawArrow(11, collection, {}, target); assert.deepEqual(drawn.at(-1), expected[direction], String(direction)); assert.deepEqual(s.log, [['stoneDraw', 11], ['bounds', 11]], String(direction)); }
    // 음수 좌표의 중점은 0 방향으로 절삭한다(-3 과 0 의 합 -3/2 → -1).
    const negative = { bounds: () => ({ left: -5, top: -7, right: 2, bottom: 2 }) }; s.dec.direction = 1; s.widgets.drawArrow(11, negative, {}, target); assert.deepEqual(drawn.at(-1)[1], line(-3, -5, 0, -2)); s.dec.direction = 2; s.widgets.drawArrow(11, negative, {}, target); assert.deepEqual(drawn.at(-1)[1], line(-3, -5, -1, 0));
    // 좌표 합은 DWORD 로 감긴다.
    const wide = { bounds: () => ({ left: 0x7ffffffe, top: 0, right: 0x7fffffff, bottom: 0 }) }; s.dec.direction = 1; s.widgets.drawArrow(11, wide, {}, target); assert.deepEqual([drawn.at(-1)[1].first.x, drawn.at(-1)[1].last.x], [-2147483648, 0x7ffffffd]);
    assert.throws(() => s.widgets.drawArrow(99, collection, {}, target), /화살표 공유 노드 없음/);
});

test('drawThumb: 배경 Draw 뒤 현재 Bounds+handle(양 끝 포함 그대로)을 기본 배경 type/frame 으로 다시 그린다', () => {
    const s = scene(); s.thumb.handle = { left: 1, top: 14, right: 8, bottom: 39 }; s.thumb.type = 5; s.thumb.frame = 6; const calls = [];
    const backgrounds = { draw: (token) => calls.push(['draw', token]), background: (...args) => calls.push(['background', ...args]) }; const collection = { bounds: (token) => { calls.push(['bounds', token]); return { left: 40, top: 70, right: 49, bottom: 152 }; } };
    s.widgets.drawThumb(13, collection, backgrounds); assert.deepEqual(calls, [['draw', 13], ['bounds', 13], ['background', { left: 41, top: 84, right: 48, bottom: 109 }, 0x03e00000, 5, 6]]);
    // 배경 Draw 가 type/frame 을 갱신하면 갱신된 현재 값을 읽는다.
    const updating = { draw: () => { s.thumb.type = 8; s.thumb.frame = 9; }, background: (...args) => calls.push(['bg', ...args]) }; s.widgets.drawThumb(13, collection, updating); assert.deepEqual(calls.at(-1).slice(2), [0x03e00000, 8, 9]);
    assert.throws(() => s.widgets.drawThumb(99, collection, backgrounds), /손잡이 공유 노드 없음/);
});

test('deleteScalar/분배 연결: 기본 소멸 위임·Slider 는 빈 Draw·화살표/손잡이 분배·이전 처리 보존·경계 없음 거부', () => {
    const s = scene(); assert.equal(s.widgets.deleteScalar(10, 1), 10); assert.deepEqual(s.log.at(-1), ['life', 10, 1]);
    const events = []; const sliderHandler = { name: 'slider' }; const arrowHandler = { drawArrow: (...args) => events.push(['arrow', args[0], args[1]]) }; const thumbHandler = { drawThumb: (...args) => events.push(['thumb', args[0], args[2]]) };
    const hooks = { draw: (token) => events.push(['previous', token]) }; const collection = {}; const backgrounds = { id: 'bg' };
    bindScrollGumpWidgetDraw(hooks, collection, {}, {}, backgrounds, (t) => (t === 10 ? sliderHandler : null), (t) => (t === 11 ? arrowHandler : null), (t) => (t === 13 ? thumbHandler : null));
    hooks.draw(10); hooks.draw(11); hooks.draw(13); hooks.draw(99); assert.deepEqual(events, [['arrow', 11, collection], ['thumb', 13, backgrounds], ['previous', 99]]);
    const bare = {}; bindScrollGumpWidgetDraw(bare, collection, {}, {}, backgrounds, () => null, () => null, () => null); assert.throws(() => bare.draw(1), /Draw 처리기 필요/);
    assert.throws(() => bindScrollGumpWidgetDraw({}, collection, {}, {}, backgrounds, null, () => null, () => null), /분류 경계/); assert.throws(() => bindScrollGumpWidgetDraw({}, collection, {}, {}, backgrounds, () => null, null, () => null), /분류 경계/); assert.throws(() => bindScrollGumpWidgetDraw({}, collection, {}, {}, backgrounds, () => null, () => null, null), /분류 경계/);
    const deleted = []; const life = { deletingDestructor: (token, flags) => deleted.push(['previous', token, flags]) }; bindScrollGumpWidgetLifetime(life, (token) => (token === 11 ? { deleteScalar: (t, f) => deleted.push(['own', t, f]) } : null));
    life.deletingDestructor(11, 1); life.deletingDestructor(12, 2); assert.deepEqual(deleted, [['own', 11, 1], ['previous', 12, 2]]);
    const none = {}; bindScrollGumpWidgetLifetime(none, () => null); assert.throws(() => none.deletingDestructor(1, 0), /scalar 소멸자 필요/); assert.throws(() => bindScrollGumpWidgetLifetime({}, null), /수명 분류/);
});

test('경계 보강: 새 노드/전역 기본값·Slider 필드 초기화는 이전 값을 지우고 +bc 는 보존·기본 생성 플래그 정확값', () => {
    // 새 노드/전역의 기본값이다(초기 종류 39/51/1·기본 type 0/frame 1).
    assert.equal(new ScrollGumpArrowNode().direction, 0); assert.deepEqual(Object.values(new ScrollGumpWidgetState()), [39, 51, 1, 0, 1]);
    // 파생 필드에 이전 값이 남아 있어도 생성자는 값 필드와 자식 토큰을 0 으로 쓰고 step 만 1 로 쓰며 미작성 +bc 는 보존한다.
    const s = scene(); Object.assign(s.slider, { minimum: 7, maximum: 8, position: 9, page: 10, step: 11, reservedB0: 5, reservedBC: 6, scrollFlags: 3, decrement: 99, increment: 98, thumb: 97 }); s.slider.reserved[0] = 3; s.slider.reserved[1] = 4;
    s.widgets.construct(10, 9, 100, 0, 0);
    assert.deepEqual([s.slider.minimum, s.slider.maximum, s.slider.position, s.slider.page, s.slider.step, s.slider.reservedB0, s.slider.reservedBC, s.slider.scrollFlags, s.slider.reserved], [0, 0, 0, 0, 1, 0, 6, 0, [0, 0]]);
    assert.deepEqual([s.slider.decrement, s.slider.increment, s.slider.thumb], [11, 12, 13]);
    // 기본 생성은 (command 0, 크기 0, 플래그 1) 이라 보이는 void 플래그와 키보드 비트만 가진다.
    assert.equal(s.slider.flags, 0x209); assert.equal(s.slider.id, s.create.entries.indexOf(10));
    // 가로 초기화에서도 세로로 남은 방향 비트를 덮어쓴다.
    const t = scene(); t.slider.scrollFlags = 1; t.widgets.construct(10, 9, 100, 0, 0); assert.equal(t.slider.scrollFlags, 0);
});
