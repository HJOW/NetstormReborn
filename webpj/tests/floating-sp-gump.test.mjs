// 전용 PE fixture가 없는 비용 표시 Gump(FloatingSpGump: 생성 순서·서식 문자열·가속도 -2·객체 참조 Attach·프레임 이동/범위 이탈·소멸 위임)를 기준선 소스 계약과 손으로 계산한 값으로 검증한다. (2026-11-14 WB04-E434)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/조건은 기준선 소스(FloatingSpGump.cpp)에서 옮겼다. 한 줄 TextGump/StyleText/수명/시계는 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { FloatingSpGump, FloatingSpGumpNode } from '../www/ui/floating-sp-gump.js';
import { GumpCreateState } from '../www/ui/gump-create.js';

// 토큰 7 한 개를 가진 장면이다. 시계는 clock 값을 돌려주고 모든 가상 효과를 기록한다.
function scene(options = {}) {
    const create = new GumpCreateState(); create.screenWidth = 640; create.screenHeight = 480; create.collection.root = 3;
    const nodes = new Map([[7, new FloatingSpGumpNode()]]); const log = []; const clock = { value: 10 }; const on = {};
    const record = (name) => (...args) => { log.push([name, ...args]); return on[name]?.(...args); };
    const hooks = { node: (token) => nodes.get(token), now: () => { log.push(['now']); return clock.value; }, allocate: (bytes) => { log.push(['allocate', bytes]); return options.allocated ?? 7; },
        constructText: record('construct'), clearText: record('clear'), fitText: record('fit'), kill: record('kill'), styledText: record('styled'), idle: record('idle'), redraw: record('redraw'), attach: record('attach'),
        reference: record('reference'), deleteText: (token, flags) => { log.push(['delete', token, flags]); return 1234; } };
    for (const key of options.omit ?? []) delete hooks[key];
    const sp = new FloatingSpGump(create, hooks); return { create, nodes, log, clock, on, hooks, sp, node: nodes.get(7) };
}
const names = (log) => log.map((entry) => entry[0]);

test('노드 기본값: 추가 필드(속도/가속도/패딩/직전 시각)는 모두 0 이다', () => {
    const node = new FloatingSpGumpNode(); assert.deepEqual([node.velocity, node.acceleration, node.padding, node.stamp, node.head], [0, 0, 0, 0, 0]);
});

test('생성자: 필수 연결이 하나라도 없으면 거부하고 reference 만 선택이다', () => {
    const s = scene(); for (const name of ['node', 'now', 'allocate', 'constructText', 'clearText', 'fitText', 'styledText', 'idle', 'redraw', 'kill', 'attach', 'deleteText']) { const hooks = { ...s.hooks }; delete hooks[name]; assert.throws(() => new FloatingSpGump(s.create, hooks), (error) => error instanceof TypeError && /실제 Gump\/시계 연결 필요/.test(error.message), name); }
    const hooks = { ...s.hooks }; delete hooks.reference; assert.doesNotThrow(() => new FloatingSpGump(s.create, hooks));
});

test('construct: 한 줄 생성→가상 표/시각 0/가속도/속도 0→Clear→원본 서식→측정→idle 등록→현재 시각/깊이 순서다', () => {
    const s = scene(); s.node.stamp = 99; s.node.velocity = 5; s.node.acceleration = 6; s.node.depth = 1; s.clock.value = 12.75;
    assert.equal(s.sp.construct(7, 50, -2), 7);
    assert.deepEqual(s.log, [['construct', 7], ['clear', 7], ['styled', 7, '~3~E~y50~[IfortGump.3]'], ['fit', 7], ['idle', 7, true], ['now']]);
    assert.deepEqual([s.node.vtable, s.node.acceleration, s.node.velocity, s.node.stamp, s.node.depth], [0x0050688c, -2, 0, 12.75, 0xffffffd8]);
    // 가속도는 float32 로 저장하고 음수/큰 비용도 부호 있는 십진 문자열이다. 비용은 DWORD signed 로 해석한다.
    s.log.length = 0; s.sp.construct(7, -5, 0.1); assert.equal(s.node.acceleration, Math.fround(0.1)); assert.equal(s.log[2][2], '~3~E~y-5~[IfortGump.3]');
    s.log.length = 0; s.sp.construct(7, 0xffffffff, 1); assert.equal(s.log[2][2], '~3~E~y-1~[IfortGump.3]');
    // 생성 중 각 콜백은 시각 0/속도 0 을 본다(생성 직후 0 으로 쓰고 마지막에 현재 시각을 쓴다).
    const seen = []; s.node.stamp = 99; s.node.velocity = 7; for (const name of ['clear', 'styled', 'fit', 'idle']) s.on[name] = () => seen.push([name, s.node.stamp, s.node.velocity]); s.sp.construct(7, 1, 1);
    assert.deepEqual(seen, [['clear', 0, 0], ['styled', 0, 0], ['fit', 0, 0], ['idle', 0, 0]]); for (const name of ['clear', 'styled', 'fit', 'idle']) s.on[name] = null;
    // 생성 중 stamp 는 한 줄 생성 직후 0 이다(생성자 콜백이 쓴 값은 덮어쓴다).
    s.on.construct = () => { s.node.stamp = 55; s.node.velocity = 9; }; s.sp.construct(7, 1, 1); assert.deepEqual([s.node.velocity, s.node.stamp], [0, 12.75]);
    // 없는 노드는 거부한다.
    assert.throws(() => s.sp.construct(0, 1, 1), /비용 표시 실제 노드 없음/); assert.throws(() => s.sp.construct(9, 1, 1), /비용 표시 실제 노드 없음/);
});

test('show: new(0xb8)→가속도 -2 생성→실제 루트에 float32 좌표로 Attach(mode 0)하고 참조가 있으면 위치 (0,0)에 참조 뒤 부착한다', () => {
    const s = scene(); assert.equal(s.sp.show(50, 100, 200), 7);
    assert.deepEqual(names(s.log), ['allocate', 'construct', 'clear', 'styled', 'fit', 'idle', 'now', 'attach']); assert.deepEqual(s.log[0], ['allocate', 0xb8]); assert.equal(s.node.acceleration, -2); assert.deepEqual(s.log.at(-1), ['attach', 7, 100, 200, 3, 0]);
    s.log.length = 0; s.sp.show(5, -16777217, 16777217); assert.deepEqual(s.log.at(-1), ['attach', 7, -16777216, 16777216, 3, 0]);
    s.log.length = 0; s.sp.show(5, 100, 200, 77); assert.deepEqual(names(s.log).slice(-2), ['reference', 'attach']); assert.deepEqual(s.log.at(-2), ['reference', 7, 77]); assert.deepEqual(s.log.at(-1), ['attach', 7, 0, 0, 3, 0]);
    // 참조 0 이거나 DWORD 로 0 이 되면 참조 경계를 부르지 않는다. 참조 경계가 없으면 거부한다.
    s.log.length = 0; s.sp.show(5, 1, 2, 0x100000000); assert.deepEqual(names(s.log).includes('reference'), false);
    // 참조 1(최하위 비트만 켜진 값)도 참조다.
    s.log.length = 0; s.sp.show(5, 9, 9, 1); assert.deepEqual(s.log.at(-2), ['reference', 7, 1]); assert.deepEqual(s.log.at(-1), ['attach', 7, 0, 0, 3, 0]);
    const bare = scene({ omit: ['reference'] }); assert.doesNotThrow(() => bare.sp.show(5, 1, 2)); assert.throws(() => bare.sp.show(5, 1, 2, 9), /실제 객체 참조 미연결/); assert.equal(bare.log.filter((entry) => entry[0] === 'attach').length, 1);
    // 할당 실패(토큰 0)는 거부한다.
    assert.throws(() => scene({ allocated: 0 }).sp.show(5, 1, 2), /할당 실패/);
});

test('idle: 시각 차이(double)를 보관하고 redraw→y=속도+y(float32)→범위 안이면 redraw 와 속도 갱신, 밖이면 idle 해제 뒤 Kill 한다', () => {
    const s = scene(); Object.assign(s.node, { stamp: 10, velocity: 2.5, acceleration: -2, x: 50, y: 100 }); s.clock.value = 10.5; s.sp.idle(7);
    assert.deepEqual(names(s.log), ['now', 'redraw', 'redraw']); assert.deepEqual(s.log[1], ['redraw', 7, false]);
    assert.equal(s.node.stamp, 10.5); assert.equal(s.node.y, 102.5); assert.equal(s.node.velocity, Math.fround(-2 * 0.5 + 2.5));
    // float32 저장: y 는 속도+y 를 float32 로 좁히고 속도는 가속도×시간+속도를 좁힌다.
    const t = scene(); const v0 = Math.fround(0.1); const y0 = Math.fround(100.1); const a0 = Math.fround(0.3); Object.assign(t.node, { stamp: 0, velocity: v0, acceleration: a0, x: 1, y: y0 }); t.clock.value = 0.7; t.sp.idle(7); assert.equal(t.node.y, Math.fround(v0 + y0)); assert.notEqual(t.node.y, v0 + y0); assert.equal(t.node.velocity, Math.fround(a0 * 0.7 + v0)); assert.notEqual(t.node.velocity, a0 * 0.7 + v0);
    // 범위 이탈: y<0, x<0, 폭<x, 높이<y 각각이며 경계값(x=폭, y=높이)은 안이다.
    const out = (fields) => { const u = scene(); Object.assign(u.node, { stamp: 0, velocity: 0, acceleration: 0, x: 10, y: 10, ...fields }); u.sp.idle(7); return names(u.log); };
    assert.deepEqual(out({ y: -1 }), ['now', 'redraw', 'idle', 'kill']); assert.deepEqual(out({ x: -1 }), ['now', 'redraw', 'idle', 'kill']); assert.deepEqual(out({ x: 640.5 }), ['now', 'redraw', 'idle', 'kill']); assert.deepEqual(out({ y: 481 }), ['now', 'redraw', 'idle', 'kill']);
    assert.deepEqual(out({ x: 640 }), ['now', 'redraw', 'redraw']); assert.deepEqual(out({ y: 480 }), ['now', 'redraw', 'redraw']); assert.deepEqual(out({ x: 0, y: 0 }), ['now', 'redraw', 'redraw']);
    // 범위 이탈이면 속도/위치만 갱신하고 두 번째 redraw 와 속도 갱신은 하지 않는다.
    const gone = scene(); Object.assign(gone.node, { stamp: 0, velocity: 3, acceleration: 9, x: 10, y: -100 }); gone.clock.value = 1; gone.sp.idle(7); assert.equal(gone.node.velocity, 3); assert.deepEqual(gone.log.slice(-2), [['idle', 7, false], ['kill', 7]]);
    // unordered(NaN) 는 범위 이탈이 아니다(x87 C0/C2 parity).
    assert.deepEqual(out({ y: NaN }), ['now', 'redraw', 'redraw']); assert.deepEqual(out({ x: NaN }), ['now', 'redraw', 'redraw']);
});

test('idle: 두 번째 redraw 뒤 가속도×경과시간을 double 로 곱하고 콜백이 바꾼 현재 필드를 재조회하며 노드가 없으면 거부한다', () => {
    const s = scene(); Object.assign(s.node, { stamp: 1, velocity: 1, acceleration: 4, x: 5, y: 5 }); s.clock.value = 1.25; let seen = 0; s.on.redraw = () => { seen += 1; if (seen === 2) s.node.velocity = 10; };
    s.sp.idle(7); assert.equal(s.node.velocity, Math.fround(4 * 0.25 + 10)); assert.throws(() => s.sp.idle(0), /비용 표시 실제 노드 없음/); assert.throws(() => s.sp.idle(9), /비용 표시 실제 노드 없음/);
    // 첫 redraw 콜백이 위치를 바꿔도 y 는 그 뒤 현재 노드 값으로 계산한다.
    const t = scene(); Object.assign(t.node, { stamp: 0, velocity: 1, acceleration: 0, x: 5, y: 5 }); let first = true; t.on.redraw = () => { if (first) { first = false; t.node.y = 50; } }; t.sp.idle(7); assert.equal(t.node.y, 51);
});

test('deleteScalar: 기존 TextGump 소멸/반납에 토큰과 flags 를 그대로 위임하고 결과를 돌려준다', () => {
    const s = scene(); assert.equal(s.sp.deleteScalar(7, 3), 1234); assert.deepEqual(s.log, [['delete', 7, 3]]);
});
