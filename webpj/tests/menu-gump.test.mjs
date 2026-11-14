// 전용 PE fixture가 없는 MenuGump(메뉴 크기/장식 프레임/확장 idle/controlled 스택)를 기준선 소스 계약과 손으로 계산한 값으로 검증한다. (2026-11-14 WB04-E428)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(MenuGump.cpp)에서 옮겼다. 기본 생성/수명 장치는 실제이고 가상 경계는 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { MenuGump, MenuGumpNode, MenuGumpState, MenuGumpItem, bindMenuGumpFrame, MENU_ITEM_CAPACITY, CONTROLLED_MENU_CAPACITY } from '../www/ui/menu-gump.js';
import { menuScene, only } from './menu-gump-support.mjs';

const FANCY = 0x800000; const HORIZONTAL = 0x40000; const EXPAND_WIDTH = 0x80000;

test('기본 저장소: 항목 40칸·표 20칸·선택 -1·float32 배열과 항목 0x3c바이트 필드를 가진다', () => {
    const node = new MenuGumpNode(); const state = new MenuGumpState();
    assert.equal(MENU_ITEM_CAPACITY, 40); assert.equal(CONTROLLED_MENU_CAPACITY, 20);
    assert.equal(node.items.length, 40); assert.ok(node.items.every((item) => item instanceof MenuGumpItem && item.label.length === 40));
    assert.equal(node.offsets.length, 40); assert.equal(node.heights.length, 40); assert.equal(node.margins.length, 4);
    assert.ok(node.offsets instanceof Float32Array && node.heights instanceof Float32Array && node.margins instanceof Float32Array);
    assert.equal(node.selection, -1); assert.equal(node.selectionCallback, 0); assert.equal(node.itemCount, 0); assert.equal(node.expanding, 0);
    assert.equal(state.entries.length, 20); assert.equal(state.index, 0); assert.equal(state.instances, 0);
    // 나머지 기본값은 모두 0이며 float32 배열과 항목 라벨도 0으로 시작한다.
    assert.deepEqual([node.menuFlags, node.fancy, node.title, node.itemWidth, node.targetWidth, node.targetHeight], [0, 0, 0, 0, 0, 0]);
    assert.ok(node.offsets.every((value) => value === 0) && node.heights.every((value) => value === 0) && node.margins.every((value) => value === 0) && node.items.every((item) => item.label.every((byte) => byte === 0)));
    const item = new MenuGumpItem(); assert.deepEqual([item.childId, item.command, item.parameter, item.styled, item.flags], [0, 0, 0, 0, 0]);
    // 항목마다 독립 저장소다.
    node.items[0].command = 7; assert.equal(node.items[1].command, 0);
});

test('생성자/노드 검증: 두 조회 경계가 필수이고 기본/파생 주소가 같은 노드만 허용한다', () => {
    const s = menuScene();
    for (const missing of ['node', 'baseNode']) { const hooks = { ...s.hooks }; delete hooks[missing]; assert.throws(() => new MenuGump(s.create, s.state, s.lifetime, hooks), /저장소 필요/, missing); }
    assert.throws(() => s.menu.setWidth(99, 1), /메뉴 공유 저장소 없음/); assert.throws(() => s.menu.setWidth(0, 1), /메뉴 공유 저장소 없음/);
    // 복사본을 돌려주는 기본 조회는 거부한다.
    const split = new MenuGump(s.create, s.state, s.lifetime, { ...s.hooks, baseNode: () => ({}) });
    assert.throws(() => split.setWidth(2, 1), /메뉴 공유 저장소 없음/);
    // 변경 전에 거부하므로 원래 값이 유지된다.
    assert.equal(s.menuNode.baseWidth, 50);
});

test('setWidth/setHeight: 비장식 메뉴는 float32 인자와 배율 곱만 쓰고 콜백을 부르지 않는다', () => {
    const s = menuScene(); s.menuNode.scale = Math.fround(1.1);
    s.menu.setWidth(2, 100.1); assert.equal(s.menuNode.baseWidth, Math.fround(100.1)); assert.equal(s.menuNode.width, 110.11000061035156);
    s.menu.setHeight(2, 77.7); assert.equal(s.menuNode.baseHeight, Math.fround(77.7)); assert.equal(s.menuNode.height, Math.fround(Math.fround(77.7) * Math.fround(1.1)));
    assert.deepEqual(s.log, []);
    // 장식 비트가 아닌 다른 비트는 장식 경로로 들어가지 않는다.
    s.menuNode.menuFlags = HORIZONTAL | 0x7fffff; s.menu.setWidth(2, 3); assert.equal(s.menuNode.width, Math.fround(3 * Math.fround(1.1))); assert.deepEqual(s.log, []);
});

test('setWidth(장식): Push→기본 폭+32 의 double 합계를 한 번만 float32로 좁힘→배율→Attach(-16, -(제목 높이)-32 절삭)', () => {
    const s = menuScene(); Object.assign(s.menuNode, { menuFlags: FANCY, fancy: 3, title: 4 }); s.fancy.scale = Math.fround(1.3); s.title.baseHeight = Math.fround(5.1);
    s.on.push = () => { assert.equal(s.fancy.baseWidth, 10); };
    s.menu.setWidth(2, 10.1); s.on.push = null;
    assert.equal(s.menuNode.baseWidth, Math.fround(10.1));
    // 좁힌 값(54.72999572753906)이 아니라 double 합계의 곱(54.72999954223633)이다.
    assert.equal(s.fancy.baseWidth, Math.fround(Math.fround(10.1) + 32)); assert.equal(s.fancy.width, 54.72999954223633);
    assert.deepEqual(s.log, [['push', 3], ['attach', 3, -16, -37, 2, 0]]);
    // 제목 기본 높이가 0이면 -32, 소수 제목 높이는 0쪽으로 절삭한다.
    s.log.length = 0; s.title.baseHeight = 0; s.menu.setWidth(2, 1); assert.deepEqual(s.log[1], ['attach', 3, -16, -32, 2, 0]);
    s.log.length = 0; s.title.baseHeight = Math.fround(0.9); s.menu.setWidth(2, 1); assert.deepEqual(s.log[1], ['attach', 3, -16, -32, 2, 0]);
    s.log.length = 0; s.title.baseHeight = Math.fround(100.9); s.menu.setWidth(2, 1); assert.deepEqual(s.log[1], ['attach', 3, -16, -132, 2, 0]);
});

test('setWidth(장식): Attach y 는 CRT 절삭의 하위 DWORD 이며 NaN/범위 밖 제목 높이는 0, 32비트를 넘으면 감긴다', () => {
    const s = menuScene(); Object.assign(s.menuNode, { menuFlags: FANCY, fancy: 3, title: 4 });
    // NaN·무한대·±2^63 이상은 무효 변환이라 0 이다.
    for (const height of [NaN, Infinity, -Infinity, 1e19, -1e19]) { s.log.length = 0; s.title.baseHeight = height; s.menu.setWidth(2, 1); assert.deepEqual(s.log[1], ['attach', 3, -16, 0, 2, 0], String(height)); }
    // 3e9+32 의 절삭은 하위 32비트만 남아 signed 로 읽는다(33비트로 읽으면 -3000000032).
    s.log.length = 0; s.title.baseHeight = 3e9; s.menu.setWidth(2, 1); assert.deepEqual(s.log[1], ['attach', 3, -16, Math.fround(4294967296 - 3000000032), 2, 0]);
    s.log.length = 0; s.title.baseHeight = -3e9; s.menu.setWidth(2, 1); assert.deepEqual(s.log[1], ['attach', 3, -16, Math.fround(2999999968 - 4294967296), 2, 0]);
});

test('setHeight(장식): 제목 높이+현재 메뉴 높이+48 의 중간 합계를 좁히지 않고 장식 높이/Attach를 갱신한다', () => {
    const s = menuScene(); Object.assign(s.menuNode, { menuFlags: FANCY, fancy: 3, title: 4 }); s.fancy.scale = Math.fround(1.3); s.title.baseHeight = Math.fround(5.1);
    s.menu.setHeight(2, 58.5);
    assert.equal(s.menuNode.baseHeight, 58.5); assert.equal(s.fancy.baseHeight, 111.5999984741211); assert.equal(s.fancy.height, 145.0800018310547);
    assert.deepEqual(s.log, [['push', 3], ['attach', 3, -16, -37, 2, 0]]);
    // 제목 높이가 같은 합계의 일부이므로 float32 중간 합산이면 달라지는 입력도 double 합계를 쓴다.
    s.menu.setHeight(2, 40.3); assert.equal(s.fancy.baseHeight, 93.4000015258789);
    // 장식 높이는 제목 기본 높이를 읽는다(제목 0 은 거부).
    s.menuNode.title = 0; assert.throws(() => s.menu.setHeight(2, 1), /메뉴 장식\/제목 저장소 없음/);
});

test('장식 경계 결함은 도달한 지점에서만 거부하고 이미 쓴 필드는 유지한다', () => {
    const s = menuScene({ omit: ['push'] }); Object.assign(s.menuNode, { menuFlags: FANCY, fancy: 3, title: 4 });
    assert.throws(() => s.menu.setWidth(2, 9), /메뉴 장식 Push 경계 필요/); assert.equal(s.menuNode.baseWidth, 9);
    assert.throws(() => s.menu.setHeight(2, 8), /메뉴 장식 Push 경계 필요/); assert.equal(s.menuNode.baseHeight, 8);
    // 비장식 메뉴는 Push 가 없어도 정상이다.
    s.menuNode.menuFlags = 0; assert.doesNotThrow(() => s.menu.setWidth(2, 5));
    const t = menuScene({ omit: ['attach'] }); Object.assign(t.menuNode, { menuFlags: FANCY, fancy: 3, title: 4 });
    assert.throws(() => t.menu.setWidth(2, 9), /메뉴 장식 Attach 경계 필요/); assert.equal(t.fancy.baseWidth, 41);
    // 장식/제목 저장소가 없으면 Push 뒤에 거부한다.
    const u = menuScene(); Object.assign(u.menuNode, { menuFlags: FANCY, fancy: 99, title: 4 });
    assert.throws(() => u.menu.setWidth(2, 9), /메뉴 장식\/제목 저장소 없음/); assert.deepEqual(u.log, [['push', 99]]);
});

test('redraw: 장식이 있으면 장식 먼저·기본 요청은 항상 호출하며 menuFlags 와 무관하다', () => {
    const s = menuScene(); s.menu.redraw(2, true); assert.deepEqual(s.log, [['baseRedraw', 2, true]]);
    s.log.length = 0; s.menuNode.fancy = 3; s.menu.redraw(2, false); assert.deepEqual(s.log, [['redraw', 3, false], ['baseRedraw', 2, false]]);
    // 장식 콜백 뒤에도 기본 요청이 이어진다.
    const t = menuScene({ omit: ['redraw'] }); t.menuNode.fancy = 3; assert.throws(() => t.menu.redraw(2, false), /메뉴 장식 Redraw 경계 필요/); assert.deepEqual(t.log, []);
    const u = menuScene({ omit: ['baseRedraw'] }); assert.throws(() => u.menu.redraw(2, false), /메뉴 기본 Redraw 경계 필요/);
    const v = menuScene({ omit: ['redraw'] }); assert.doesNotThrow(() => v.menu.redraw(2, false));
});

test('itemPosition: 가로는 index×항목 폭/현재 높이, 세로는 현재 폭/저장 offset, 범위 밖 offset 은 거부한다', () => {
    const s = menuScene(); Object.assign(s.menuNode, { menuFlags: HORIZONTAL, itemWidth: Math.fround(12.7), height: Math.fround(9.3), width: 77 });
    assert.deepEqual(s.menu.itemPosition(2, 3), [Math.fround(3 * Math.fround(12.7)), Math.fround(9.3)]);
    // 가로는 음수/큰 index 도 offset 배열을 읽지 않고 계산한다.
    assert.deepEqual(s.menu.itemPosition(2, -2), [Math.fround(-2 * Math.fround(12.7)), Math.fround(9.3)]);
    assert.deepEqual(s.menu.itemPosition(2, 1000), [Math.fround(1000 * Math.fround(12.7)), Math.fround(9.3)]);
    s.menuNode.menuFlags = 0x80000; s.menuNode.offsets[3] = 21.5; s.menuNode.offsets[39] = 4;
    assert.deepEqual(s.menu.itemPosition(2, 3), [77, 21.5]); assert.deepEqual(s.menu.itemPosition(2, 39), [77, 4]);
    // 첫 칸(index 0)은 정상이며, 가로 비트가 없고 bit0 만 있는 세로 메뉴는 세로로 본다.
    s.menuNode.menuFlags = 0x80001; s.menuNode.offsets[0] = 3; assert.deepEqual(s.menu.itemPosition(2, 0), [77, 3]); s.menuNode.menuFlags = 0x80000;
    assert.throws(() => s.menu.itemPosition(2, 40), /offset 범위 오류/); assert.throws(() => s.menu.itemPosition(2, -1), /offset 범위 오류/);
    // signed 로 읽는 DWORD index 의 상위 비트도 음수다.
    assert.throws(() => s.menu.itemPosition(2, 0xffffffff), /offset 범위 오류/);
});

test('startExpand: 폭 방향은 폭 목표→높이 0, 그 외는 폭 0→높이 목표 순서이고 idle 등록 뒤 expanding 을 쓴다', () => {
    const s = menuScene(); s.menuNode.targetWidth = 90; s.menuNode.targetHeight = 60; s.menuNode.scale = 1;
    s.menu.startExpand(2, EXPAND_WIDTH);
    assert.deepEqual([s.menuNode.baseWidth, s.menuNode.baseHeight], [90, 0]); assert.equal(s.menuNode.expanding, 1);
    assert.equal(s.lifeState.idle[0], 2); assert.equal(s.menuNode.flags & 0x80, 0x80);
    s.lifeState.idle.fill(0); s.menuNode.flags &= ~0x80;
    s.menu.startExpand(2, HORIZONTAL);
    assert.deepEqual([s.menuNode.baseWidth, s.menuNode.baseHeight], [0, 60]);
    // 다른 비트만 있으면 폭 방향이 아니다.
    s.lifeState.idle.fill(0); s.menuNode.flags &= ~0x80; s.menu.startExpand(2, 0x7ff7ffff); assert.deepEqual([s.menuNode.baseWidth, s.menuNode.baseHeight], [0, 60]);
});

test('startExpand: 첫 크기 변경 콜백이 바꾼 목표 높이를 다시 읽는다', () => {
    const s = menuScene(); Object.assign(s.menuNode, { menuFlags: FANCY, fancy: 3, title: 4, targetWidth: 20, targetHeight: 30 });
    s.on.push = () => { s.menuNode.targetHeight = 44; };
    s.menu.startExpand(2, 0); assert.equal(s.menuNode.baseHeight, 44); assert.equal(s.menuNode.baseWidth, 0);
    // idle 표가 가득 차면 expanding 을 쓰기 전에 거부한다.
    const t = menuScene(); t.lifeState.idle.fill(9); assert.throws(() => t.menu.startExpand(2, 0), /idle/); assert.equal(t.menuNode.expanding, 0);
});

test('idle: 확장 중이 아니면 아무것도 하지 않고, 목표에 닿으면 expanding 만 지우며, 부족한 축만 Push→높이→폭→Pop 한다', () => {
    const s = menuScene(); Object.assign(s.menuNode, { targetWidth: 80, targetHeight: 70 });
    s.menu.idle(2); assert.deepEqual(s.log, []); assert.deepEqual([s.menuNode.baseWidth, s.menuNode.baseHeight], [50, 40]);
    s.menuNode.expanding = 1; s.menuNode.width = 80; s.menuNode.height = 70; s.menu.idle(2); assert.equal(s.menuNode.expanding, 0); assert.deepEqual(s.log, []);
    // 폭만 모자라면 폭만 쓴다(높이는 같으므로 SetHeight 생략).
    s.menuNode.expanding = 1; s.menuNode.width = 10; s.menuNode.height = 70; s.menu.idle(2);
    assert.equal(s.menuNode.expanding, 1); assert.deepEqual(s.log, [['push', 2], ['pop', 2]]); assert.equal(s.menuNode.baseWidth, 80); assert.equal(s.menuNode.baseHeight, 40);
    // 높이만 모자란 경우도 같다.
    s.log.length = 0; Object.assign(s.menuNode, { width: 80, height: 5, baseWidth: 1, baseHeight: 1 }); s.menu.idle(2); assert.equal(s.menuNode.baseHeight, 70); assert.equal(s.menuNode.baseWidth, 1);
    // 둘 다 모자라면 높이 먼저다.
    s.log.length = 0; Object.assign(s.menuNode, { menuFlags: FANCY, fancy: 3, title: 4, width: 1, height: 1, baseWidth: 5, baseHeight: 5 }); const seen = [];
    s.on.attach = (token) => seen.push([token, s.fancy.baseWidth, s.fancy.baseHeight]); s.menu.idle(2);
    assert.deepEqual(s.log.map((entry) => entry[0] + entry[1]), ['push2', 'push3', 'attach3', 'push3', 'attach3', 'pop2']);
    // 첫 장식 갱신은 높이만 바뀐 상태이고 둘째는 폭까지 바뀐 상태다(높이가 먼저).
    assert.deepEqual(seen, [[3, 10, 20 + 70 + 48], [3, 80 + 32, 20 + 70 + 48]]); assert.deepEqual([s.menuNode.baseHeight, s.menuNode.baseWidth], [70, 80]);
});

test('idle: NaN 비교는 확장 분기로 들어가고 Push/Pop 경계가 없으면 Push 전에 거부한다', () => {
    const s = menuScene(); Object.assign(s.menuNode, { expanding: 1, targetWidth: NaN, targetHeight: 70, width: 100, height: 100 });
    s.menu.idle(2); assert.equal(s.menuNode.expanding, 1); assert.deepEqual(s.log, [['push', 2], ['pop', 2]]);
    // plain float 비교이므로 NaN 폭 목표는 폭 설정을 생략하고, NaN 현재 높이는 높이 설정을 생략한다.
    assert.equal(s.menuNode.baseWidth, 50); assert.equal(s.menuNode.baseHeight, 40);
    for (const omit of ['push', 'pop']) { const t = menuScene({ omit: [omit] }); Object.assign(t.menuNode, { expanding: 1, targetWidth: 90, targetHeight: 90 }); assert.throws(() => t.menu.idle(2), /Push\/Pop 경계 필요/, omit); assert.deepEqual(t.log, []); }
});

test('controlled 스택: current/itemCount/takeCurrent 는 현재 칸을 읽고 예약 칸·범위 밖을 변경 전에 거부한다', () => {
    const s = menuScene(); s.state.entries[0] = 0; s.state.entries[1] = 2; s.state.entries[2] = 5; s.state.index = 2; s.childA.itemCount = 7; s.menuNode.itemCount = 3;
    assert.equal(s.menu.current(), 5); assert.equal(s.menu.itemCount(), 7);
    s.state.index = 1; assert.equal(s.menu.itemCount(), 3);
    // 메뉴가 없으면 current 는 0 이고 itemCount 는 거부한다.
    s.state.index = 0; assert.equal(s.menu.current(), 0); assert.throws(() => s.menu.itemCount(), /메뉴 공유 저장소 없음/);
    // take: 토큰을 읽은 뒤 칸을 지우고 index 를 내린다.
    s.state.index = 2; assert.equal(s.menu.takeCurrent(), 5); assert.equal(s.state.entries[2], 0); assert.equal(s.state.index, 1); assert.equal(s.state.entries[1], 2);
    // 예약 칸은 꺼낼 수 없다(변경 없음).
    s.state.index = 0; s.state.entries[0] = 9; assert.throws(() => s.menu.takeCurrent(), /예약 칸/); assert.equal(s.state.entries[0], 9); assert.equal(s.state.index, 0);
    // 범위 밖 index(19 는 정상, 20/큰 DWORD 는 거부).
    s.state.index = 19; assert.equal(s.menu.current(), 0); s.state.index = 20; assert.throws(() => s.menu.current(), /index 범위/);
    s.state.index = 0xffffffff; assert.throws(() => s.menu.current(), /index 범위/);
});

test('attachCurrent/attachObjectCurrent: 현재 메뉴를 float32 좌표로 가상 부착한 뒤 콜백이 바꾼 현재 칸을 지운다', () => {
    const s = menuScene(); s.state.entries[1] = 2; s.state.index = 1;
    s.menu.attachCurrent(-5, 7, 1); assert.deepEqual(s.log, [['attach', 2, -5, 7, 1, 0]]); assert.equal(s.state.entries[1], 0); assert.equal(s.state.index, 0);
    // 짝수 좌표는 비트 단위로 보존된다(정수 변환이 최하위 비트를 바꾸지 않는다).
    s.log.length = 0; s.state.entries[1] = 2; s.state.index = 1; s.menu.attachCurrent(-6, 8, 4); assert.deepEqual(s.log, [['attach', 2, -6, 8, 4, 0]]);
    // 큰 정수는 float32 로 좁혀진다.
    s.log.length = 0; s.state.entries[1] = 2; s.state.index = 1; s.menu.attachCurrent(16777217, -16777217, 0); assert.deepEqual(s.log, [['attach', 2, 16777216, -16777216, 0, 0]]);
    // 콜백이 index 를 바꾸면 바뀐 칸을 지운다.
    s.log.length = 0; s.state.entries[1] = 2; s.state.entries[2] = 5; s.state.index = 1; s.on.attach = () => { s.state.index = 2; }; s.menu.attachCurrent(1, 1, 0);
    assert.equal(s.state.entries[2], 0); assert.equal(s.state.entries[1], 2); assert.equal(s.state.index, 1);
    // 객체 부착은 객체 ID/등록표 루트(entries[0])를 함께 전달한다.
    s.on.attach = null; s.log.length = 0; s.state.index = 1; s.state.entries[1] = 2; s.menu.attachObjectCurrent(0xdead, 3, -4);
    assert.deepEqual(s.log, [['attachObject', 2, 0xdead, 3, -4, s.create.entries[0], 0]]); assert.equal(s.state.index, 0); assert.equal(s.state.entries[1], 0);
    s.log.length = 0; s.state.index = 1; s.state.entries[1] = 2; s.menu.attachObjectCurrent(1, 4, -6); assert.deepEqual(s.log, [['attachObject', 2, 1, 4, -6, s.create.entries[0], 0]]);
    // 현재 메뉴가 없거나 경계가 없으면 칸을 바꾸기 전에 거부한다.
    s.state.index = 0; assert.throws(() => s.menu.attachCurrent(1, 1, 0), /메뉴 공유 저장소 없음/);
    for (const omit of ['attach', 'attachObject']) { const t = menuScene({ omit: [omit] }); t.state.entries[1] = 2; t.state.index = 1; assert.throws(() => (omit === 'attach' ? t.menu.attachCurrent(1, 1, 0) : t.menu.attachObjectCurrent(1, 1, 1)), /Attach 경계 필요/, omit); assert.equal(t.state.index, 1); assert.equal(t.state.entries[1], 2); }
});

test('bindMenuGumpFrame: 메뉴만 idle/redraw 를 새 몸체로 보내고 다른 토큰은 이전 처리기를 보존한다', () => {
    const s = menuScene(); const calls = []; const hooks = { idle: (token) => calls.push(['idle', token]), redraw: (token, erase) => calls.push(['redraw', token, erase]) };
    const previous = { ...hooks }; bindMenuGumpFrame(hooks, (token) => (token === 2 ? s.menu : null));
    s.menuNode.expanding = 1; Object.assign(s.menuNode, { width: 5, height: 5, targetWidth: 5, targetHeight: 5 }); hooks.idle(2); assert.equal(s.menuNode.expanding, 0); assert.deepEqual(calls, []);
    hooks.idle(7); hooks.redraw(7, true); assert.deepEqual(calls, [['idle', 7], ['redraw', 7, true]]);
    hooks.redraw(2, false); assert.deepEqual(s.log, [['baseRedraw', 2, false]]);
    assert.notEqual(hooks.idle, previous.idle);
    // 분배 조회가 없거나 이전 처리기가 없으면 거부한다.
    assert.throws(() => bindMenuGumpFrame({}, null), /메뉴 프레임 분배 조회 필요/);
    const bare = {}; bindMenuGumpFrame(bare, () => null); assert.throws(() => bare.idle(1), /메뉴 외 idle 경계 필요/); assert.throws(() => bare.redraw(1, false), /메뉴 외 Redraw 경계 필요/);
});

test('기본값: 확장 비트/장식 비트/가로 비트와 오류 종류(저장소 Error, 경계 TypeError, 범위 RangeError)', () => {
    const s = menuScene(); assert.throws(() => s.menu.setWidth(99, 1), (error) => error.constructor === Error);
    const t = menuScene({ omit: ['attach'] }); t.state.entries[1] = 2; t.state.index = 1; assert.throws(() => t.menu.attachCurrent(1, 1, 0), (error) => error instanceof TypeError);
    assert.throws(() => s.menu.itemPosition(2, 99), (error) => error instanceof RangeError); s.state.index = 99; assert.throws(() => s.menu.current(), (error) => error instanceof RangeError);
    assert.equal(only(s.log, 'push').length, 0);
});
