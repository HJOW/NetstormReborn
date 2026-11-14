// 전용 PE fixture가 없는 Slider(ScrollGump)의 값/범위/페이지·자식 부착·손잡이 선택/드래그/통지를 기준선 소스 계약과 손으로 계산한 기하로 검증한다. (2026-11-14 WB04-E425)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(ScrollGump.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ScrollGump, ScrollGumpNode, ScrollGumpThumbNode, ScrollGumpState, bindScrollGump, bindScrollingTextSlider } from '../www/ui/scroll-gump.js';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { InputCode } from '../www/core/input-queue.js';

// 사건 ID 네 개(임의 값)와 실제 기본 생성으로 등록한 Slider(10)·화살표(11,12)·손잡이(13) 장면이다. 트리/사건/입력/수집기는 호출을 기록하는 대역이다.
const EVENTS = { clickEvent: 0x1000, repeatEvent: 0x1001, stepEvent: 0x1002, changeEvent: 0x1003 };
function scene(options = {}) {
    const create = new GumpCreateState(); create.screenWidth = 640; create.screenHeight = 480;
    const nodes = new Map([[1, new GumpCreateNode()], [10, new ScrollGumpNode()], [11, new GumpCreateNode()], [12, new GumpCreateNode()], [13, new ScrollGumpThumbNode()]]); const lookup = (token) => nodes.get(token);
    const factory = new GumpCreate(create, { node: lookup, allocate: () => 1 }); factory.initializeRoot();
    // 세로 Slider(폭 9·높이 100)와 손잡이(폭 9·높이 82)를 기본 생성한다. 화살표는 폭/높이 9 다.
    factory.construct(10, 0, 9, 100, 1); factory.construct(11, 0, 9, 9, 1); factory.construct(12, 0, 9, 9, 1); factory.construct(13, 0, 9, 82, 1);
    const slider = nodes.get(10); Object.assign(slider, { minimum: 0, maximum: 30, position: 5, page: 10, step: 3, decrement: 11, increment: 12, thumb: 13 }); nodes.get(13).parent = 10; nodes.get(11).parent = 10; nodes.get(12).parent = 10;
    const log = []; const state = Object.assign(new ScrollGumpState(), EVENTS); const bubbleResults = []; const poll = { code: 0 }; const bounds = { left: 40, top: 70 };
    const tree = { attach: (...args) => log.push(['tree', ...args]) }; const events = { bubble: (...args) => { log.push(['bubble', ...args]); return bubbleResults.length ? bubbleResults.shift() : 0; } };
    const input = { capture: (token) => log.push(['capture', token]) }; const collection = { bounds: (token) => { log.push(['bounds', token]); return { ...bounds, right: bounds.left + 9, bottom: bounds.top + 82 }; } };
    const hooks = { node: lookup, thumbNode: lookup, baseNode: lookup, push: (token) => log.push(['push', token]), attach: (...args) => log.push(['attach', ...args]), redraw: (token, erase) => log.push(['redraw', token, erase]), poll: (code) => { log.push(['poll', code]); return { code: poll.code, x: 0, y: 0 }; } };
    for (const key of options.omit ?? []) delete hooks[key];
    const gump = new ScrollGump(create, state, tree, collection, events, input, hooks);
    return { create, nodes, factory, slider, thumb: nodes.get(13), dec: nodes.get(11), inc: nodes.get(12), log, state, bubbleResults, poll, bounds, tree, events, input, collection, hooks, gump };
}
const only = (log, name) => log.filter((entry) => entry[0] === name);

test('생성자/노드 검증: 세 조회 경계가 필수이고 기본/파생 주소가 같은 노드만 허용한다', () => {
    const s = scene(); const build = (hooks) => new ScrollGump(s.create, s.state, s.tree, s.collection, s.events, s.input, hooks);
    for (const missing of ['node', 'thumbNode', 'baseNode']) { const hooks = { ...s.hooks }; delete hooks[missing]; assert.throws(() => build(hooks), /공유 저장소/, missing); }
    assert.throws(() => s.gump.setPosition(99, 0), /공유 노드 없음/); assert.throws(() => s.gump.position(0), /공유 노드 없음/);
    const split = build({ ...s.hooks, baseNode: (token) => (token === 10 ? new ScrollGumpNode() : s.nodes.get(token)) }); assert.throws(() => split.setPosition(10, 0), /공유 노드 없음/);
    const thumbSplit = build({ ...s.hooks, baseNode: (token) => (token === 13 ? new ScrollGumpThumbNode() : s.nodes.get(token)) }); assert.throws(() => thumbSplit.thumbHandle(13, InputCode.leftButton, 0, 0), /손잡이 공유 노드 없음/);
});

test('setPosition: page 계약 → 최솟값/최댓값-page 제한 → 저장 → void 가 아니면 현재 Push 뒤 현재 x/y 로 Attach, void 이면 redraw(false)', () => {
    const s = scene(); s.slider.flags = (s.slider.flags & ~8) >>> 0; s.slider.x = 12.5; s.slider.y = 7.25;
    s.gump.setPosition(10, 12); assert.equal(s.slider.position, 12); assert.deepEqual(s.log, [['push', 10], ['attach', 10, 12.5, 7.25, 0, 0]]);
    // 제한: 최솟값 미만은 최솟값, (최댓값-page)=20 초과는 20 이다. 경계 값은 그대로다.
    for (const [input, expected] of [[-5, 0], [0, 0], [20, 20], [21, 20], [0x7ffffff5, 20], [0x7fffffff, 0x7fffffff], [-0x80000000, 0]]) { s.gump.setPosition(10, input); assert.equal(s.slider.position, expected, String(input)); }
    // page + position 의 DWORD 합이 넘어가는 큰 위치도 원본처럼 감겨 비교한다(0x7ffffff0 + 10 은 음수로 감김 → 제한하지 않는다).
    s.gump.setPosition(10, 0x7ffffff6); assert.equal(s.slider.position, 0x7ffffff6);
    // Push 가 현재 x/y 를 바꾸면 그 값을 Attach 에 쓴다. void 이면 redraw 만 한다.
    const t = scene(); t.slider.flags = (t.slider.flags & ~8) >>> 0; t.hooks.push = (token) => { t.log.push(['push', token]); t.slider.x = 3; t.slider.y = 4; }; const u = new ScrollGump(t.create, t.state, t.tree, t.collection, t.events, t.input, t.hooks); u.setPosition(10, 1); assert.deepEqual(only(t.log, 'attach'), [['attach', 10, 3, 4, 0, 0]]);
    const v = scene(); assert.ok(v.slider.flags & 8); v.gump.setPosition(10, 4); assert.deepEqual(v.log, [['redraw', 10, false]]);
    // page 가 범위보다 크면 거부하고 위치를 바꾸지 않는다.
    const w = scene(); w.slider.page = 31; assert.throws(() => w.gump.setPosition(10, 4), /page가 범위보다 큼/); assert.equal(w.slider.position, 5); w.slider.page = 30; w.gump.setPosition(10, 9); assert.equal(w.slider.position, 0);
    // 필요한 가상 경계는 저장 뒤 도달할 때 거부한다.
    const x = scene({ omit: ['redraw'] }); assert.throws(() => x.gump.setPosition(10, 4), /Redraw 경계/); assert.equal(x.slider.position, 4);
    const y = scene({ omit: ['push'] }); y.slider.flags = (y.slider.flags & ~8) >>> 0; assert.throws(() => y.gump.setPosition(10, 4), /Push 경계/); assert.equal(y.slider.position, 4);
    const z = scene({ omit: ['attach'] }); z.slider.flags = (z.slider.flags & ~8) >>> 0; assert.throws(() => z.gump.setPosition(10, 4), /Attach 경계/); assert.deepEqual(only(z.log, 'push'), [['push', 10]]);
});

test('setPage/setRange/position: 범위 길이 이하 page·두 끝 정렬·동률 최대+1·위치 두 번 재적용·조회는 그대로', () => {
    const s = scene(); s.gump.setPage(10, 8); assert.equal(s.slider.page, 8); assert.deepEqual(s.log, [['redraw', 10, false]]);
    s.gump.setPage(10, 99); assert.equal(s.slider.page, 30); s.gump.setPage(10, -4); assert.equal(s.slider.page, -4);
    // 범위 안 page 를 줄이면 위치가 새 상한으로 다시 제한된다.
    const t = scene(); t.slider.position = 5; t.slider.page = 0; t.gump.setPage(10, 28); assert.equal(t.slider.position, 2);
    // setRange: 끝점 정렬·동률은 최소+1·저장 뒤 page 를 길이로 제한·현재 position 을 두 번 적용한다(redraw 두 번).
    const r = scene(); r.slider.page = 25; r.log.length = 0; r.gump.setRange(10, 20, 12); assert.deepEqual([r.slider.minimum, r.slider.maximum, r.slider.page, r.slider.position], [12, 20, 8, 12]); assert.equal(only(r.log, 'redraw').length, 2);
    const q = scene(); q.slider.page = 0; q.gump.setRange(10, 7, 7); assert.deepEqual([q.slider.minimum, q.slider.maximum, q.slider.position], [7, 8, 7]);
    const p = scene(); p.gump.setRange(10, 0, 100); assert.deepEqual([p.slider.minimum, p.slider.maximum, p.slider.page, p.slider.position], [0, 100, 10, 5]);
    const o = scene(); o.slider.page = 0; o.gump.setRange(10, 0x7fffffff, 0x7fffffff); assert.deepEqual([o.slider.minimum, o.slider.maximum], [0x7fffffff, -0x80000000]);
    // 조회는 보정/변환 없이 현재 값이다.
    s.slider.position = -77; assert.equal(s.gump.position(10), -77);
});

test('attach 세로: 기본 부착 → Push 세 자식 → 크기/위치 → 현재 값/페이지의 손잡이 사각형(양 끝 포함)', () => {
    const s = scene(); s.slider.scrollFlags = 0; s.gump.attach(10, 11.5, 22.25, 7, 1);
    assert.deepEqual(s.log, [['tree', 10, 11.5, 22.25, 7, 1], ['push', 11], ['push', 12], ['push', 13], ['attach', 11, 0, 0, 10, 0], ['attach', 12, 0, 91, 10, 0], ['attach', 13, 0, 9, 10, 0]]);
    // 두께 9: 위/아래 화살표는 9×9, 손잡이는 폭 9·높이 100-18=82 다.
    assert.deepEqual([s.dec.baseHeight, s.dec.baseWidth, s.dec.height, s.dec.width, s.inc.baseHeight, s.inc.baseWidth, s.thumb.baseHeight, s.thumb.baseWidth], [9, 9, 9, 9, 9, 9, 82, 9]);
    // 범위 30·page 10·position 5·가용 82 → page 픽셀 27, position 픽셀 13, 가로(교차) 8 이다.
    assert.deepEqual(s.thumb.handle, { right: 8, left: 1, top: 14, bottom: 39 });
    // 위치 분자는 position-minimum 이 아닌 position 이다(최소 4 로 올려도 같다).
    s.slider.minimum = 4; s.gump.attach(10, 0, 0, 0, 0); assert.deepEqual(s.thumb.handle, { right: 8, left: 1, top: 1 + Math.trunc(5 * 82 / 26), bottom: Math.trunc(5 * 82 / 26) + Math.trunc(10 * 82 / 26) - 1 });
    // 배율은 저장 전 크기에 곱한다(0.5 배율 화살표 높이 9 → 4.5).
    const t = scene(); t.dec.scale = 0.5; t.gump.attach(10, 0, 0, 0, 0); assert.deepEqual([t.dec.baseHeight, t.dec.height, t.dec.baseWidth, t.dec.width], [9, 4.5, 9, 4.5]);
    // 두께는 기본 폭의 CRT 절삭이다(9.75 → 9). 큰 두께는 손잡이 높이가 음수가 될 수 있다.
    const u = scene(); u.slider.baseWidth = 9.75; u.gump.attach(10, 0, 0, 0, 0); assert.deepEqual([u.dec.baseHeight, u.inc.baseHeight, u.thumb.baseHeight, u.thumb.baseWidth, u.thumb.handle.right], [9, 9, 82, 9.75, 8]);
    const v = scene(); v.slider.baseWidth = 60; v.gump.attach(10, 0, 0, 0, 0); assert.deepEqual([v.dec.baseHeight, v.thumb.baseHeight, v.thumb.baseWidth], [60, -20, 60]);
    // 범위 0 이면 나눗셈이 무효(NaN/∞)라 손잡이 크기는 0 이다.
    const w = scene(); w.slider.maximum = 0; w.slider.page = 0; w.gump.attach(10, 0, 0, 0, 0); assert.deepEqual([w.thumb.handle.top, w.thumb.handle.bottom], [1, -1]);
    // 필요한 가상 경계는 도달할 때만 거부한다(기본 부착은 이미 일어난 뒤).
    const x = scene({ omit: ['push'] }); assert.throws(() => x.gump.attach(10, 0, 0, 0, 0), /Push 경계/); assert.deepEqual(only(x.log, 'tree').length, 1);
    const y = scene({ omit: ['attach'] }); assert.throws(() => y.gump.attach(10, 0, 0, 0, 0), /자식 Attach 경계/);
});

test('attach 가로: 두께는 기본 높이·화살표는 두께 폭·손잡이 폭은 기본 폭-2×두께·위치는 x 방향', () => {
    const s = scene(); s.slider.scrollFlags = 1; s.slider.baseWidth = 100; s.slider.baseHeight = 9; s.gump.attach(10, 0, 0, 0, 0);
    assert.deepEqual(only(s.log, 'attach'), [['attach', 11, 0, 0, 10, 0], ['attach', 12, 91, 0, 10, 0], ['attach', 13, 9, 0, 10, 0]]);
    assert.deepEqual([s.dec.baseHeight, s.dec.baseWidth, s.inc.baseHeight, s.inc.baseWidth, s.thumb.baseHeight, s.thumb.baseWidth], [9, 9, 9, 9, 9, 82]);
    assert.deepEqual(s.thumb.handle, { bottom: 8, left: 14, top: 1, right: 39 });
    // 범위 1 에 page 0 이면 손잡이 폭은 0 이며 position 이 0 이면 left 1·right 0 이다.
    const t = scene(); t.slider.scrollFlags = 1; t.slider.baseWidth = 100; t.slider.baseHeight = 9; t.slider.maximum = 1; t.slider.page = 0; t.slider.position = 0; t.gump.attach(10, 0, 0, 0, 0); assert.deepEqual(t.thumb.handle, { bottom: 8, left: 1, top: 1, right: -1 });
});

test('dragVertical: 가로는 무동작·세로는 DWORD 곱/CRT 절삭으로 y 변화량을 position 으로 바꾼다', () => {
    const s = scene(); s.thumb.handle.top = 14; s.gump.dragVertical(10, 6); assert.equal(s.slider.position, 7);
    // 위치는 setPosition 의 제한을 거친다: 아래로 크게 끌면 최댓값-page, 위로 크게 끌면 최솟값.
    s.gump.dragVertical(10, 1000); assert.equal(s.slider.position, 20); s.gump.dragVertical(10, -1000); assert.equal(s.slider.position, 0);
    // 분모는 높이-2×폭이다. 같거나 작으면 ∞/NaN → 0 으로 처리한다(CRT 무효 변환).
    s.slider.baseHeight = 18; s.gump.dragVertical(10, 6); assert.equal(s.slider.position, 0);
    // 곱은 DWORD 로 감긴다(range 0x10000 × (delta+top)).
    const t = scene(); t.slider.maximum = 0x10000; t.slider.page = 0; t.thumb.handle.top = 0; t.slider.baseHeight = 100; t.slider.baseWidth = 9; t.gump.dragVertical(10, 0x10000); assert.equal(t.slider.position, Math.trunc(Math.imul(0x10000, 0x10000) / 82));
    // 가로 Slider 는 아무 것도 하지 않는다.
    const u = scene(); u.slider.scrollFlags = 1; u.gump.dragVertical(10, 6); assert.deepEqual([u.slider.position, u.log], [5, []]);
});

test('handle: 화살표 click/repeat 는 부모가 step 을 처리하지 않을 때만 step 만큼 이동하고 값이 바뀌면 변경 통지·손잡이 step 은 부모 전달 뒤 변경만 통지', () => {
    const s = scene(); const decSerial = s.dec.serial; const incSerial = s.inc.serial; const thumbSerial = s.thumb.serial;
    // 감소: 부모 Bubble 이 0 이면 position-step, 변경되면 변경 통지(값 0).
    assert.equal(s.gump.handle(10, EVENTS.clickEvent, decSerial, 0), 0); assert.equal(s.slider.position, 2); assert.deepEqual(only(s.log, 'bubble'), [['bubble', 10, EVENTS.stepEvent, s.slider.serial, 2], ['bubble', 10, EVENTS.changeEvent, s.slider.serial, 0]]);
    // 증가(repeat 사건 + 상위 비트가 섞인 코드): 사건 마스크로 하위 분류만 비교한다.
    s.log.length = 0; assert.equal(s.gump.handle(10, (EVENTS.repeatEvent | 0x80000000 | 0x04000000) >>> 0, incSerial, 0), 0); assert.equal(s.slider.position, 5); assert.deepEqual(only(s.log, 'bubble')[0], ['bubble', 10, EVENTS.stepEvent, s.slider.serial, 1]);
    // 부모가 처리하면(Bubble 이 비0) position 을 직접 바꾸지 않고 통지도 하지 않는다.
    s.log.length = 0; s.bubbleResults.push(10); s.gump.handle(10, EVENTS.clickEvent, decSerial, 0); assert.equal(s.slider.position, 5); assert.equal(only(s.log, 'bubble').length, 1);
    // 이동해도 값이 같으면(제한) 변경 통지가 없다.
    const t = scene(); t.slider.position = 0; t.gump.handle(10, EVENTS.clickEvent, t.dec.serial, 0); assert.equal(t.slider.position, 0); assert.equal(only(t.log, 'bubble').length, 1);
    // 손잡이 serial 의 click 이나 모르는 serial 은 무시한다.
    const u = scene(); u.gump.handle(10, EVENTS.clickEvent, u.thumb.serial, 0); u.gump.handle(10, EVENTS.clickEvent, 12345, 0); u.gump.handle(10, 0x7777, u.dec.serial, 0); assert.deepEqual(u.log, []);
    // 손잡이 step: 부모에 현재 slider serial 과 second 를 전달하고, 그 사이 값이 바뀌었으면(부모가 setPosition) 변경을 통지한다.
    const v = scene(); v.events.bubble = (...args) => { v.log.push(['bubble', ...args]); if (args[1] === EVENTS.stepEvent) v.slider.position = 9; return 0; }; v.gump = new ScrollGump(v.create, v.state, v.tree, v.collection, v.events, v.input, v.hooks); assert.equal(v.gump.handle(10, EVENTS.stepEvent, v.thumb.serial, 3), 0);
    assert.deepEqual(only(v.log, 'bubble'), [['bubble', 10, EVENTS.stepEvent, v.slider.serial, 3], ['bubble', 10, EVENTS.changeEvent, v.slider.serial, 0]]);
    const w = scene(); w.gump.handle(10, EVENTS.stepEvent, w.thumb.serial, 4); assert.deepEqual(only(w.log, 'bubble'), [['bubble', 10, EVENTS.stepEvent, w.slider.serial, 4]]);
    // step 사건이라도 손잡이 serial 이 아니면 무시한다.
    const x = scene(); x.gump.handle(10, EVENTS.stepEvent, x.dec.serial, 4); assert.deepEqual(x.log, []);
    assert.equal(thumbSerial, s.thumb.serial);
});

test('thumbHandle: 왼쪽 버튼만 처리·세로 release 만 플래그 해제·손잡이 밖은 page 방향 3/4·안은 선택과 캡처', () => {
    const s = scene(); s.thumb.handle = { left: 1, top: 14, right: 8, bottom: 39 }; s.bounds.top = 70; s.bounds.left = 40;
    // 왼쪽 버튼이 아닌 사건은 처리하지 않는다(사건 마스크 후 비교).
    assert.equal(s.gump.thumbHandle(13, InputCode.rightButton, 0, 0), 0); assert.equal(s.gump.thumbHandle(13, 0x1234, 0, 0), 0); assert.deepEqual(s.log, []);
    // 세로 release 는 플래그를 해제하고 항상 1, 가로 release 는 플래그를 건드리지 않는다.
    s.thumb.flags = (s.thumb.flags | 0x100) >>> 0; assert.equal(s.gump.thumbHandle(13, (InputCode.leftButton | InputCode.release) >>> 0, 0, 0), 1); assert.equal(s.thumb.flags & 0x100, 0);
    const h = scene(); h.thumb.baseWidth = 82; h.thumb.baseHeight = 9; h.thumb.flags = (h.thumb.flags | 0x100) >>> 0; assert.equal(h.gump.thumbHandle(13, (InputCode.leftButton | InputCode.release) >>> 0, 0, 0), 1); assert.equal(h.thumb.flags & 0x100, 0x100);
    // 누름(세로): 로컬 y = second - bounds.top. 손잡이 위(13)는 page 위(4), 아래(40)는 page 아래(3), 안은 선택/캡처다.
    s.log.length = 0; assert.equal(s.gump.thumbHandle(13, InputCode.leftButton, 45, 70 + 13), 1); assert.deepEqual(only(s.log, 'bubble'), [['bubble', 13, EVENTS.stepEvent, s.thumb.serial, 4]]);
    s.log.length = 0; s.gump.thumbHandle(13, InputCode.leftButton, 45, 70 + 40); assert.deepEqual(only(s.log, 'bubble'), [['bubble', 13, EVENTS.stepEvent, s.thumb.serial, 3]]);
    for (const local of [14, 39]) { s.log.length = 0; s.thumb.flags = 0; s.gump.thumbHandle(13, InputCode.leftButton, 45, 70 + local); assert.deepEqual(only(s.log, 'capture'), [['capture', 13]], String(local)); assert.equal(s.thumb.previousY, 70 + local); assert.equal(s.thumb.flags & 0x100, 0x100); assert.equal(only(s.log, 'bubble').length, 0); }
    // 가로: x 와 left/right 를 쓰고 이전 x 를 저장한다(패치는 가로도 캡처한다).
    h.thumb.handle = { left: 14, top: 1, right: 39, bottom: 8 }; h.log.length = 0; h.gump.thumbHandle(13, InputCode.leftButton, 40 + 13, 0); assert.deepEqual(only(h.log, 'bubble'), [['bubble', 13, EVENTS.stepEvent, h.thumb.serial, 4]]);
    h.log.length = 0; h.gump.thumbHandle(13, InputCode.leftButton, 40 + 40, 0); assert.deepEqual(only(h.log, 'bubble'), [['bubble', 13, EVENTS.stepEvent, h.thumb.serial, 3]]);
    h.log.length = 0; h.thumb.flags = 0; h.gump.thumbHandle(13, InputCode.leftButton, 40 + 20, 0); assert.deepEqual([h.thumb.previousX, h.thumb.previousY, only(h.log, 'capture').length], [60, 0, 1]);
    // 부호 있는 좌표: 음수 y 도 signed 로 비교한다.
    const n = scene(); n.thumb.handle = { left: 1, top: -20, right: 8, bottom: -5 }; n.bounds.top = -30; n.gump.thumbHandle(13, InputCode.leftButton, 0, (-30 + -10) >>> 0); assert.equal(n.thumb.previousY, -40); assert.equal(only(n.log, 'capture').length, 1);
});

test('thumbBroadcast: 폴링 release 는 플래그만 해제·그 밖에는 이전 y 와의 차이로 부모 Slider 를 갱신하고 부모 serial 로 변경 통지', () => {
    const s = scene(); s.thumb.handle.top = 14; s.thumb.previousY = 100; s.thumb.flags = (s.thumb.flags | 0x100) >>> 0; s.poll.code = InputCode.release;
    s.gump.thumbBroadcast(13, 0, 0, 120); assert.deepEqual(only(s.log, 'poll'), [['poll', InputCode.leftButton]]); assert.equal(s.thumb.flags & 0x100, 0); assert.equal(s.slider.position, 5); assert.equal(s.thumb.previousY, 100);
    // 누르고 있는 동안: delta 6 → position 7·이전 y 갱신·부모(slider) serial 로 변경 통지(값 0).
    s.poll.code = 0; s.log.length = 0; s.gump.thumbBroadcast(13, 0, 0, 106); assert.equal(s.thumb.previousY, 106); assert.equal(s.slider.position, 7); assert.deepEqual(only(s.log, 'bubble'), [['bubble', 13, EVENTS.changeEvent, s.slider.serial, 0]]);
    // delta 0 이면 아무 것도 하지 않는다. 음수 delta 도 처리한다.
    s.log.length = 0; s.gump.thumbBroadcast(13, 0, 0, 106); assert.equal(only(s.log, 'bubble').length, 0); s.gump.thumbBroadcast(13, 0, 0, 104); assert.equal(s.thumb.previousY, 104); assert.equal(s.slider.position, 4);
    // 폴링 경계가 없으면 거부한다(플래그/위치는 바꾸지 않는다).
    const t = scene({ omit: ['poll'] }); t.thumb.flags = (t.thumb.flags | 0x100) >>> 0; assert.throws(() => t.gump.thumbBroadcast(13, 0, 0, 5), /폴링 경계/); assert.equal(t.thumb.flags & 0x100, 0x100);
});

test('bindScrollGump: Slider/손잡이 토큰만 파생 처리기로 분배하고 나머지는 이전 분배를 보존한다', () => {
    const s = scene(); const previous = []; const tree = { attach: (...args) => previous.push(['attach', ...args]) }; const events = { handle: (...args) => { previous.push(['events', ...args]); return 77; } }; const input = { handle: (...args) => { previous.push(['input', ...args]); return 88; } }; const frame = { broadcast: (...args) => previous.push(['frame', ...args]) };
    bindScrollGump(tree, events, input, frame, (token) => (token === 10 ? s.gump : null), (token) => (token === 13 ? s.gump : null));
    tree.attach(10, 1, 2, 0, 0, { xBits: 1 }); assert.deepEqual(only(s.log, 'tree'), [['tree', 10, 1, 2, 0, 0]]); tree.attach(11, 3, 4, 10, 1, { xBits: 5 }); assert.deepEqual(previous.shift(), ['attach', 11, 3, 4, 10, 1, { xBits: 5 }]);
    // 사건/입력: Slider 토큰은 handle, 손잡이 토큰은 thumbHandle, 다른 토큰은 이전 분배의 반환값을 그대로 쓴다.
    assert.equal(events.handle(10, EVENTS.clickEvent, s.dec.serial, 0), 0); assert.equal(s.slider.position, 2); assert.equal(events.handle(13, 0x1234, 0, 0), 0); assert.equal(events.handle(11, 5, 6, 7), 77); assert.deepEqual(previous.shift(), ['events', 11, 5, 6, 7]);
    assert.equal(input.handle(13, InputCode.leftButton | InputCode.release, 0, 0) >>> 0, 1); assert.equal(input.handle(11, 5, 6, 7), 88); assert.deepEqual(previous.shift(), ['input', 11, 5, 6, 7]);
    // 프레임 통지는 손잡이 토큰만 가로챈다.
    s.poll.code = InputCode.release; s.thumb.flags = (s.thumb.flags | 0x100) >>> 0; frame.broadcast(13, 0, 0, 0); assert.equal(s.thumb.flags & 0x100, 0); frame.broadcast(11, 1, 2, 3); assert.deepEqual(previous.shift(), ['frame', 11, 1, 2, 3]);
    // 이전 분배가 없으면 거부한다. resolve 가 함수가 아니면 연결을 거부한다.
    const bare = { attach: undefined, handle: undefined }; const bareInput = { handle: undefined }; const bareFrame = { broadcast: undefined }; const bareEvents = { handle: undefined }; bindScrollGump(bare, bareEvents, bareInput, bareFrame, () => null, () => null);
    assert.throws(() => bare.attach(1, 0, 0, 0, 0), /Attach 처리기/); assert.throws(() => bareEvents.handle(1, 0, 0, 0), /입력 처리기/); assert.throws(() => bareInput.handle(1, 0, 0, 0), /입력 처리기/); assert.throws(() => bareFrame.broadcast(1, 0, 0, 0), /Broadcast 처리기/);
    assert.throws(() => bindScrollGump({}, {}, {}, {}, null, () => null), /분류 경계/); assert.throws(() => bindScrollGump({}, {}, {}, {}, () => null, null), /분류 경계/);
});

test('bindScrollingTextSlider: 다중 줄 TextGump 의 네 Slider 경계가 실제 범위/페이지/위치/현재 값 구현을 읽고 쓴다', () => {
    const s = scene(); const hooks = {}; bindScrollingTextSlider(hooks, s.gump);
    // E423 setScroll 과 같은 호출 순서: range(0,30) → page(25) → position(12→제한 5) → 현재 값.
    s.slider.minimum = 0; s.slider.maximum = 999; s.slider.page = 0; s.slider.position = 0;
    hooks.sliderRange(10, 0, 30); assert.deepEqual([s.slider.minimum, s.slider.maximum], [0, 30]); hooks.sliderPage(10, 25); assert.equal(s.slider.page, 25); hooks.sliderSetPosition(10, 12); assert.equal(hooks.sliderPosition(10), 5);
    // 범위가 1 이고 page 가 0 이면 위치는 0 이다.
    hooks.sliderRange(10, 5, 5); assert.deepEqual([s.slider.minimum, s.slider.maximum, s.slider.page], [5, 6, 1]); hooks.sliderPage(10, 0); hooks.sliderSetPosition(10, 100); assert.equal(hooks.sliderPosition(10), 6);
});

test('경계 보강: 새 노드/전역 기본값·손잡이 플래그의 정확한 값·정사각 손잡이는 세로·CRT 절삭의 32비트 감김과 무효 변환', () => {
    // 새 노드/전역의 기본값(원본 소유 필드)이다.
    const node = new ScrollGumpNode(); assert.deepEqual([node.minimum, node.maximum, node.position, node.page, node.step, node.scrollFlags, node.decrement, node.increment, node.thumb, node.reservedB0, node.reservedBC, node.reserved], [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, [0, 0]]);
    const thumbNode = new ScrollGumpThumbNode(); assert.deepEqual([thumbNode.detailType, thumbNode.detailFrame, thumbNode.handle, thumbNode.previousY, thumbNode.previousX], [0, 0, { left: 0, top: 0, right: 0, bottom: 0 }, 0, 0]);
    assert.deepEqual(Object.values(new ScrollGumpState()), [0, 0, 0, 0]);
    // 누름은 플래그 0x100 만 더하고 release 는 0x100 만 지운다(다른 비트는 그대로).
    const s = scene(); s.thumb.handle = { left: 1, top: 14, right: 8, bottom: 39 }; const before = s.thumb.flags; assert.equal(before, 9);
    s.gump.thumbHandle(13, InputCode.leftButton, 45, 70 + 20); assert.equal(s.thumb.flags, 0x109); s.gump.thumbHandle(13, (InputCode.leftButton | InputCode.release) >>> 0, 45, 70 + 20); assert.equal(s.thumb.flags, 9);
    s.poll.code = 0; s.thumb.flags = 0x109; s.poll.code = InputCode.release; s.gump.thumbBroadcast(13, 0, 0, 5); assert.equal(s.thumb.flags, 9);
    // 가로/세로 판정은 폭 > 높이 일 때만 가로다(정사각은 세로로 처리한다).
    const q = scene(); q.thumb.baseWidth = 9; q.thumb.baseHeight = 9; q.thumb.flags = 0x109; q.gump.thumbHandle(13, (InputCode.leftButton | InputCode.release) >>> 0, 0, 0); assert.equal(q.thumb.flags, 9);
    q.thumb.handle = { left: 1, top: 14, right: 8, bottom: 39 }; q.log.length = 0; q.gump.thumbHandle(13, InputCode.leftButton, 1000, 70 + 20); assert.equal(q.thumb.previousY, 90); assert.equal(q.thumb.previousX, 0);
    // CRT 절삭은 signed 64비트 정수의 하위 32비트다: 2^32 는 0 이라 위치 0, 33비트로 읽으면 2^32 라 최댓값-page 로 제한된다.
    const t = scene(); Object.assign(t.slider, { minimum: 0, maximum: 1 << 20, page: 0, position: 5, baseWidth: 9, baseHeight: 18.25 }); t.thumb.handle.top = 0; t.gump.dragVertical(10, 1024); assert.equal(t.slider.position, 0);
    // 무효 변환(0/0 → NaN)은 0 이고 예외가 아니다.
    const u = scene(); u.slider.baseHeight = 18; u.thumb.handle.top = 0; u.gump.dragVertical(10, 0); assert.equal(u.slider.position, 0);
    // 하위 32비트가 음수가 되는 값(2^31)은 최솟값으로 제한된다.
    const v = scene(); Object.assign(v.slider, { minimum: 0, maximum: 1 << 20, page: 0, position: 5, baseWidth: 9, baseHeight: 18.5 }); v.thumb.handle.top = 0; v.gump.dragVertical(10, 1024); assert.equal(v.slider.position, 0);
});
