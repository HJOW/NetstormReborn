// 전용 PE fixture가 없는 생산 실패 안내 Gump(ProductionHintGump: 복제/파싱/측정/4초 만료·두 실패 wrapper·Idle 만료·Draw 여백·소멸 순서)를 기준선 소스 계약과 손으로 계산한 값으로 검증한다. (2026-11-14 WB04-E434)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/조건은 기준선 소스(ProductionHintGump.cpp)에서 옮겼다. 한 줄 TextGump/StyleText/수명/시계/번역은 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ProductionHintGump, ProductionHintNode, ProductionHintState } from '../www/ui/production-hint-gump.js';
import { GumpCreateState } from '../www/ui/gump-create.js';

// 토큰 7 한 개를 가진 장면이다. 복제 문자열은 토큰 표로 소유하며 모든 효과를 기록한다.
function scene(options = {}) {
    const create = new GumpCreateState(); create.collection.root = 3; const state = new ProductionHintState(); const nodes = new Map([[7, new ProductionHintNode()]]); const log = []; const clock = { value: 100 }; const on = {};
    const owned = new Map(); let next = 500;
    const record = (name) => (...args) => { log.push([name, ...args]); return on[name]?.(...args); };
    const hooks = { node: (token) => nodes.get(token), now: () => { log.push(['now']); return clock.value; }, allocate: (bytes) => { log.push(['allocate', bytes]); return options.allocated ?? 7; }, constructText: record('construct'), kill: record('kill'),
        duplicate: (text) => { log.push(['duplicate', text]); owned.set(next, text); return next++; }, string: (token) => { log.push(['string', token]); return owned.get(token); }, free: record('free'),
        translate: (text) => { log.push(['translate', text]); return `T(${text})`; }, style: record('style'), fit: record('fit'), idle: record('idle'),
        bounds: (token) => { log.push(['bounds', token]); return on.bounds?.(token) ?? { left: 1, top: 2, right: 3, bottom: 4 }; }, position: (token) => { log.push(['position', token]); return on.position?.(token) ?? { x: 10, y: 20 }; },
        draw: record('draw'), attach: record('attach'), deleteText: (token, flags) => { log.push(['delete', token, flags]); return 99; } };
    for (const key of options.omit ?? []) delete hooks[key];
    const hint = new ProductionHintGump(create, state, hooks); return { create, state, nodes, log, clock, on, hooks, hint, node: nodes.get(7), owned };
}
const names = (log) => log.map((entry) => entry[0]);
const bits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };

test('노드 기본값: 추가 필드(미작성/만료/소유/기준)는 모두 0 이다', () => {
    const node = new ProductionHintNode(); assert.deepEqual([node.reserved, node.until, node.owned, node.reference, node.head], [0, 0, 0, 0, 0]);
});

test('상태/생성자: 종류 25·흰색 7·검정/마지막 serial 0 이고 필수 경계가 하나라도 없으면 거부한다', () => {
    const state = new ProductionHintState(); assert.deepEqual([state.kind, state.white, state.black, state.lastSerial], [25, 7, 0, 0]);
    const s = scene(); for (const name of ['node', 'now', 'allocate', 'constructText', 'kill', 'duplicate', 'string', 'free', 'translate', 'style', 'fit', 'idle', 'bounds', 'position', 'draw', 'attach', 'deleteText']) { const hooks = { ...s.hooks }; delete hooks[name]; assert.throws(() => new ProductionHintGump(s.create, s.state, hooks), (error) => error instanceof TypeError && /실제 경계 누락/.test(error.message), name); }
});

test('construct: 한 줄 생성→가상 표/만료 0/종류/문맥/기준 0→복제→파싱→측정(6,4)→idle→현재 시각+4초 순서다', () => {
    const s = scene(); Object.assign(s.node, { reference: 9, until: 55 }); s.node.textContext.set([1, 1, 1, 1, 1, 1, 1, 1]); s.state.white = 12; s.state.black = 3; s.clock.value = 100.5;
    assert.equal(s.hint.construct(7, '~0~w~Etext'), 7);
    assert.deepEqual(s.log, [['construct', 7], ['duplicate', '~0~w~Etext'], ['string', 500], ['style', 7, '~0~w~Etext'], ['fit', 7, 6, 4], ['idle', 7, true], ['now']]);
    assert.deepEqual([s.node.vtable, s.node.command, s.node.reference, s.node.owned, s.node.until], [0x50c2b4, 25, 0, 500, 104.5]); assert.deepEqual(Array.from(s.node.textContext), [0, 6, 12, 0, 0, 3, 1, 1]);
    // 종류 BYTE 는 signed 확장이다. 만료는 현재 시각+4 의 double 이다.
    s.state.kind = -3; s.hint.construct(7, 'x'); assert.equal(s.node.command, 0xfffffffd); s.clock.value = 0.1; s.hint.construct(7, 'x'); assert.equal(s.node.until, 0.1 + 4);
    s.state.kind = 200; s.hint.construct(7, 'x'); assert.equal(s.node.command, (200 << 24 >> 24) >>> 0);
    // 생성 중 파싱 콜백은 만료 0·기준 0·소유 토큰을 본다(마지막에 현재 시각+4 를 쓴다).
    const seen = []; s.node.until = 55; s.on.style = () => seen.push([s.node.until, s.node.reference, s.node.owned]); s.hint.construct(7, 'x'); assert.deepEqual(seen, [[0, 0, s.node.owned]]); s.on.style = null;
    // 생성자 콜백이 쓴 값은 덮어쓴다. 없는 노드는 거부한다.
    s.on.construct = () => { s.node.until = 77; s.node.reference = 8; }; s.state.kind = 25; s.hint.construct(7, 'x'); assert.deepEqual([s.node.until, s.node.reference], [s.clock.value + 4, 0]); assert.throws(() => s.hint.construct(9, 'x'), /생산 안내 노드 없음/);
});

test('show: 두 실패 문구를 번역해 생성하고 기준 직사각형 float32 비트/플래그/깊이/기준 토큰을 쓴 뒤 y+20 으로 Attach 하며 마지막 serial 을 기록한다', () => {
    const s = scene(); s.node.serial = 4242; s.node.flags = 8; assert.equal(s.hint.show(false, 100, 200, 55), 7);
    assert.deepEqual(rows(s.log, 'translate'), [['~0~w~ENot enough Storm Power to build this']]); assert.deepEqual(s.log[0], ['allocate', 0xb8]); assert.equal(s.node.owned >= 500, true);
    assert.deepEqual(Array.from(s.node.positioning.slice(2)), [bits(1), bits(2), bits(3), bits(4)]); assert.equal(s.node.flags, (8 | 0x4000 | 0x8100) >>> 0); assert.equal(s.node.depth, 0xffffff9c); assert.equal(s.node.reference, 55);
    assert.deepEqual(s.log.at(-1), ['attach', 7, 100, 220, 3, 0]); assert.equal(s.state.lastSerial, 4242);
    // 빈 인벤토리 문구와 직사각형 float32/signed 좌표.
    const t = scene(); t.on.bounds = () => ({ left: -1, top: 16777217, right: 0.5, bottom: 7 }); t.hint.show(true, 5, 0x7fffffff, 9); assert.deepEqual(rows(t.log, 'translate'), [['~0~w~ENone in inventory']]);
    assert.deepEqual(Array.from(t.node.positioning.slice(2)), [bits(-1), bits(16777217), bits(0.5), bits(7)]); assert.deepEqual(t.log.at(-1), ['attach', 7, 5, Math.fround((0x7fffffff + 20) | 0), 3, 0]);
    // 번역 결과가 그대로 복제되고 파싱된다.
    assert.deepEqual(rows(s.log, 'duplicate'), [['T(~0~w~ENot enough Storm Power to build this)']]); assert.equal(scene({ allocated: 0 }).hint !== undefined, true); assert.throws(() => scene({ allocated: 0 }).hint.show(false, 1, 2, 3), /할당 실패/);
});

test('idle: 만료 시각이 현재 시각 이하이면 같은 지연 삭제를 예약하고 NaN 은 만료시키지 않는다', () => {
    const s = scene(); s.node.until = 100; s.clock.value = 99.9; s.hint.idle(7); assert.deepEqual(names(s.log), ['now']); s.log.length = 0; s.clock.value = 100; s.hint.idle(7); assert.deepEqual(s.log, [['now'], ['kill', 7]]);
    s.log.length = 0; s.clock.value = 101; s.hint.idle(7); assert.equal(names(s.log).includes('kill'), true); s.log.length = 0; s.node.until = NaN; s.hint.idle(7); assert.equal(names(s.log).includes('kill'), false);
    s.node.until = 1; s.clock.value = NaN; s.log.length = 0; s.hint.idle(7); assert.equal(names(s.log).includes('kill'), false); assert.throws(() => s.hint.idle(9), /생산 안내 노드 없음/);
});

test('draw: 경계/위치를 읽고 (3,2) 여백(DWORD 감김)으로 현재 head 를 출력한다', () => {
    const s = scene(); s.node.head = 321; s.hint.draw(7); assert.deepEqual(s.log, [['bounds', 7], ['position', 7], ['draw', 321, { x: 13, y: 22 }]]);
    const wrap = scene(); wrap.on.position = () => ({ x: 0x7fffffff, y: -0x80000000 }); wrap.hint.draw(7); assert.deepEqual(wrap.log.at(-1), ['draw', 0, { x: (0x7fffffff + 3) | 0, y: (-0x80000000 + 2) | 0 }]); assert.throws(() => wrap.hint.draw(9), /생산 안내 노드 없음/);
});

test('deleteScalar: 가상 표를 쓰고 소유 문자열이 있으면 반납한 뒤(필드는 0 으로 지우지 않음) TextGump 소멸 결과를 돌려준다', () => {
    const s = scene(); s.node.vtable = 1; s.node.owned = 500; assert.equal(s.hint.deleteScalar(7, 3), 99); assert.deepEqual(s.log, [['free', 500], ['delete', 7, 3]]); assert.equal(s.node.vtable, 0x50c2b4); assert.equal(s.node.owned, 500);
    const t = scene(); t.node.owned = 0; assert.equal(t.hint.deleteScalar(7, 0), 99); assert.deepEqual(t.log, [['delete', 7, 0]]); assert.throws(() => t.hint.deleteScalar(9, 0), /생산 안내 노드 없음/);
});

// 기록에서 이름의 항목만, 이름 제외한 값으로 돌려준다.
function rows(log, name) { return log.filter((entry) => entry[0] === name).map((entry) => entry.slice(1)); }
