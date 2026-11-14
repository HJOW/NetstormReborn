// 전용 PE fixture가 없는 MenuGumpWidgets(메뉴 정상 생성·장식/제목·controlled 시작·가상 Attach 화면 보정·소멸·객체 연결)를 기준선 소스 계약과 손으로 계산한 값으로 검증한다. (2026-11-14 WB04-E432)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(MenuGumpWidgets.cpp, 기준선 뒤 4ee69f4f 포함)에서 옮겼다. 기본 생성/배경 생성/수집기/수명/트리/레이아웃/메뉴 크기 처리기는 실제이고 장식/제목 생성·할당·가상 Attach·Form 연결은 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { MenuGumpWidgets, MenuGumpWidgetState, MenuGumpConstructionEntry, bindMenuTitleLayout, bindMenuGumpWidgets, bindMenuGumpObjectAttach } from '../www/ui/menu-gump-widgets.js';
import { GumpBackgroundCreate } from '../www/ui/gump-background-create.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { StyleText, StyleTextState } from '../www/ui/style-text-fragments.js';
import { layoutScene, only } from './menu-gump-support.mjs';

const TEXT = 0x10000; const MENU = 0x20000; const HORIZONTAL = 0x40000; const VERTICAL = 0x80000; const FANCY = 0x800000; const BACKGROUND = 0x03e00000;

// 토큰 7 은 새 메뉴(생성 대상), 5/6 은 자식 메뉴, 3/4 는 장식/제목 후보이다. 할당은 allocate 큐에서 공급하고 장식/제목 생성은 호출을 기록한다.
function widgetScene(options = {}) {
    const s = layoutScene({ sizes: options.sizes ?? {}, menu: options.menu }); const { nodes, log, on } = s;
    // 새 메뉴 노드를 추가 등록한다(생성 장치가 id 를 부여하므로 construct 전에 등록하지 않는다).
    nodes.set(7, new (nodes.get(2).constructor)()); s.factory.construct(7, 0, 10, 10, 1);
    const color = { value: 0 }; const background = new GumpBackgroundCreate(s.create, s.factory, () => color.value, { node: s.hooks.node });
    const decoration = { construct: (token, width, height) => { log.push(['decoration', token, width, height]); s.factory.construct(token, 0, 0, 0, 1); return token; } };
    const title = { construct: (token, width, height, font, foreground) => { log.push(['title', token, width, height, font, foreground]); s.factory.construct(token, 0, 0, 0, 1); return token; } };
    const stylesState = new StyleTextState(); const style = new StyleText(stylesState, {}); const cleared = []; const realClear = style.clear.bind(style); style.clear = (head) => { cleared.push(head.value); realClear(head); };
    const collection = new GumpCollection(s.create.collection, { node: s.hooks.node }); const treeState = new GumpTreeState(); treeState.checking = () => 0;
    const tree = new GumpTree(s.create, treeState, { node: s.hooks.node, redraw: () => {}, push: () => {}, attach: () => {}, kill: () => {} });
    const state = new MenuGumpWidgetState(); Object.assign(state, { backgroundType: 0, backgroundFrame: 0, foreground: 21, shadow: 23 });
    const queue = [...(options.tokens ?? [7, 3, 4])]; const record = (name) => (...args) => { log.push([name, ...args]); return on[name]?.(...args); };
    const lifetime = { setIdle: (token, enabled) => { log.push(['idle', token, enabled]); }, deleteScalar: (token, flags) => { log.push(['scalar', token, flags]); return token; } };
    const hooks = { node: s.hooks.node, base: s.hooks.baseNode, allocate: (bytes) => { log.push(['allocate', bytes]); return queue.length ? queue.shift() : 0; }, attach: record('vattach'), selectionNotice: record('notice'),
        objectForm: (type, mode) => { log.push(['form', type, mode]); return on.form?.(type, mode) ?? { token: 77, index: 8 }; }, formOwner: record('owner'), formObject: record('object') };
    for (const key of options.omit ?? []) delete hooks[key];
    const widgets = new MenuGumpWidgets(s.create, s.state, state, background, decoration, title, s.layout, style, lifetime, collection, tree, hooks);
    return { ...s, widgets, widgetState: state, background, color, decoration, title, style, cleared, collection, tree, treeState, lifetime, widgetHooks: hooks, queue, newMenu: nodes.get(7) };
}
const rows = (log, name) => only(log, name).map((entry) => entry.slice(1));
const entry = (source, flags, command = 0, parameter = 0, condition = 1) => Object.assign(new MenuGumpConstructionEntry(), { source, flags, command, parameter, condition });

test('상태/기본값: 식별 BYTE 26·현재 Form 타입 조회가 있으면 사본 대신 호출하고 항목 기본값은 문자열 비어 있음/조건 1 이다', () => {
    const state = new MenuGumpWidgetState(); assert.equal(state.alignment, 26); assert.deepEqual([state.backgroundType, state.backgroundFrame, state.foreground, state.shadow, state.objectFormType], [0, 0, 0, 0, 0]);
    state.objectFormType = 12; assert.equal(state.objectFormTypeNow(), 12); let current = 33; state.currentObjectFormType = () => current; assert.equal(state.objectFormTypeNow(), 33); current = 0x1ffffffff; assert.equal(state.objectFormTypeNow(), 0xffffffff);
    const item = new MenuGumpConstructionEntry(); assert.deepEqual([item.source, item.flags, item.command, item.parameter, item.condition], ['', 0, 0, 0, 1]);
});

test('생성자/노드 검증: 두 조회 경계가 필수이고 기본/파생 주소가 같은 노드만 허용한다', () => {
    const s = widgetScene();
    for (const missing of ['node', 'base']) { const hooks = { ...s.widgetHooks }; delete hooks[missing]; assert.throws(() => new MenuGumpWidgets(s.create, s.state, s.widgetState, s.background, s.decoration, s.title, s.layout, s.style, s.lifetime, s.collection, s.tree, hooks), /메뉴 생성 공유 노드 필요/, missing); }
    assert.throws(() => s.widgets.construct(99, [], 0), /메뉴 생성 공유 노드 없음/); assert.throws(() => s.widgets.construct(0, [], 0), /메뉴 생성 공유 노드 없음/);
    const split = new MenuGumpWidgets(s.create, s.state, s.widgetState, s.background, s.decoration, s.title, s.layout, s.style, s.lifetime, s.collection, s.tree, { ...s.widgetHooks, base: () => ({}) }); assert.throws(() => split.construct(7, [], 0), /메뉴 생성 공유 노드 없음/);
});

test('construct: 배경 생성 뒤 부분 초기화(가상 표·식별 BYTE·문맥 일부 DWORD·카운터·플래그)만 쓰고 미작성 offsets/목표 크기/문맥은 보존한다', () => {
    const s = widgetScene(); const node = s.newMenu; node.offsets.fill(9); node.targetWidth = 4; node.targetHeight = 5; node.margins.fill(6); node.textContext.set([1, 2, 3, 4, 5, 6, 7, 8]); node.items[39].flags = 0x55; node.items[0].label.fill(1); node.heights.fill(3); Object.assign(node.items[5], { childId: 4, command: 5, parameter: 6, styled: 7, flags: 8 });
    s.state.instances = 5; assert.equal(s.widgets.construct(7, [], 0x280000), 7);
    assert.equal(node.vtable, 0x0050c6f0); assert.equal(node.backgroundFlags, BACKGROUND); assert.equal(node.command, 26); assert.deepEqual([node.type, node.frame], [0, 0]);
    // 기본 생성 flags 0x201 에서 void 가 켜진 값에 0x20(자식 전파)과 마지막 0x100(통지)이 더해진다.
    assert.equal(node.flags, 0x201 | 8 | 0x20 | 0x100); assert.equal(node.depth, 0xffffffc4); assert.equal(node.selection, -1); assert.equal(s.state.instances, 6);
    assert.deepEqual([node.expanding, node.itemWidth, node.menuFlags, node.itemCount, node.fancy, node.title, node.selectionCallback], [0, 0, 0x280000, 0, 0, 0, 0]);
    // 문맥: 기본 생성이 0 으로 초기화한 뒤 [0]|=2, [1]=0, [2]=현재 흰색, [5]=그림자, [6]=1, [7]=0 만 쓴다.
    assert.deepEqual(Array.from(node.textContext), [2, 0, 21, 0, 0, 23, 1, 0]);
    // 보존: offsets(기본 생성이 건드리지 않는 파생 필드), 항목 저장은 값 초기화, 높이는 0. 세로 메뉴 Reflow 가 항목 0개로 목표를 0 으로 쓰고 여백을 다시 쓴다.
    assert.ok(node.items.every((item) => item.flags === 0 && item.childId === 0 && item.command === 0 && item.parameter === 0 && item.styled === 0 && item.label.every((byte) => byte === 0))); assert.ok(node.heights.every((height) => height === 0)); assert.ok(node.offsets.every((offset) => offset === 9));
    assert.deepEqual(Array.from(node.margins), [15, 15, 1, 1]); assert.deepEqual([node.targetWidth, node.targetHeight], [0, 0]);
});

test('construct(크기/bit0): 방향 비트가 없으면 기본 생성의 0×0 이 남고 bit0 만 있는 flags 는 가로/장식이 아니다', () => {
    const s = widgetScene(); s.widgets.construct(7, [], 0); const node = s.newMenu; assert.deepEqual([node.baseWidth, node.baseHeight, node.width, node.height, node.menuFlags], [0, 0, 0, 0, 0]);
    // 장식 비트에 bit0 가 더해져도 가로가 아니므로 정상 생성이다(가로 비트를 bit0 와 혼동하지 않는다).
    const t = widgetScene({ tokens: [3, 4] }); assert.doesNotThrow(() => t.widgets.construct(7, [], FANCY | 1)); assert.equal(t.newMenu.fancy, 3);
    // 제목 할당이 실패하면 제목 토큰은 0 이고 제목 Attach 에서 거부한다.
    const u = widgetScene({ tokens: [3, 0] }); assert.throws(() => u.widgets.construct(7, [], FANCY), /메뉴 생성 자식\/부모 노드 없음/); assert.equal(u.newMenu.title, 0); assert.equal(u.newMenu.fancy, 3);
});

test('construct(색 byte): 배경 생성의 +20 은 현재 색 BYTE 이고 곧바로 식별 BYTE 로 덮인다. 식별 BYTE 는 signed 확장이다', () => {
    const s = widgetScene(); s.color.value = 0x1ff; s.widgetState.alignment = -3; s.widgets.construct(7, [], 0); assert.equal(s.newMenu.command, 0xfffffffd);
    // 배경 타입/프레임 전역은 배경 생성 인자로 간다(타입 0 이면 SHP 장치가 필요 없다).
    s.widgetState.backgroundType = 0; s.widgetState.backgroundFrame = 4; s.widgets.construct(7, [], 0); assert.equal(s.newMenu.frame, 4);
});

test('construct(항목): 문자열은 flags 를 쓴 뒤 문자열 설정으로 항목을 늘리고 명령/인자/조건은 해당 비트일 때만 소비한다', () => {
    const s = widgetScene(); s.widgets.construct(7, [entry('Open', TEXT | 1, 5), entry('Skip', TEXT | 0x100 | 1, 6, 0, 0), entry('Param', TEXT | 0x80, 0, 9), entry('Third', TEXT | 0x100 | 0x80, 0, 3, 1)], 0x280000);
    const items = s.newMenu.items; assert.equal(s.newMenu.itemCount, 3);
    assert.deepEqual([items[0].flags, items[0].command], [TEXT | 1, 5]); assert.equal(Buffer.from(items[0].label.subarray(0, 4)).toString('latin1'), 'Open');
    // 조건 0 이어도 명령 쓰기는 일어나지만(항목 1 자리) 항목이 늘지 않으므로 다음 항목이 같은 칸을 덮는다.
    assert.deepEqual([items[1].flags, items[1].command, items[1].parameter], [TEXT | 0x80, 6, 9]); assert.equal(Buffer.from(items[1].label.subarray(0, 5)).toString('latin1'), 'Param');
    assert.deepEqual([items[2].flags, items[2].parameter], [TEXT | 0x100 | 0x80, 3]); assert.equal(Buffer.from(items[2].label.subarray(0, 5)).toString('latin1'), 'Third');
    // 생성자가 끝에 Reflow 를 호출해 목표 크기와 높이가 갱신된다.
    assert.equal(s.newMenu.targetHeight, 3 * (4 + 2)); assert.ok(only(s.log, 'plain').length >= 3);
});

test('construct(항목 bit0): 명령 비트만 있는 항목은 문자열/명시 인자/조건 비트가 아니다', () => {
    const s = widgetScene(); s.widgets.construct(7, [entry('A', TEXT | 1, 5, 9, 0), entry(5, MENU | 1, 7)], 0);
    // 조건 비트(0x100)가 없으니 조건 0 이어도 항목이 추가되고 명시 인자 비트(0x80)가 없으니 인자는 쓰지 않는다.
    assert.equal(s.newMenu.itemCount, 1); assert.deepEqual([s.newMenu.items[0].command, s.newMenu.items[0].parameter], [5, 0]); assert.equal(Buffer.from(s.newMenu.items[0].label.subarray(0, 1)).toString('latin1'), 'A');
    // 자식 항목(MENU|bit0)은 문자열이 아니라 자식 경로이며 직전 항목에 자식 비트가 쓰인다.
    assert.equal(s.newMenu.items[0].flags, (TEXT | 1) | 0x20);
    // 항목 세 개 뒤의 자식은 마지막(2번) 항목에 연결된다.
    const t = widgetScene(); t.childA.id = 9; t.widgets.construct(7, [entry('A', TEXT), entry('B', TEXT), entry('C', TEXT), entry(5, MENU)], 0); assert.deepEqual([t.newMenu.items[0].flags, t.newMenu.items[1].flags, t.newMenu.items[2].flags], [TEXT, TEXT, TEXT | 0x20]); assert.equal(t.newMenu.items[2].childId, 9);
    // 자식 숨김은 bit0 만 지운다(다른 비트는 그대로).
    const u = widgetScene(); u.childA.flags = 0x80000009; u.widgets.construct(7, [entry(5, MENU)], 0); assert.equal(u.childA.flags, 0x80000008);
});

test('construct(항목 오류): 문자열/자식 동시 비트·실제 문자열/자식 없음·40개 한도 위반은 도달 지점에서 거부한다', () => {
    const s = widgetScene(); assert.throws(() => s.widgets.construct(7, [entry('x', TEXT | MENU)], 0), /문자열\/자식 플래그 중복/); assert.throws(() => s.widgets.construct(7, [entry(5, TEXT)], 0), /실제 문자열 필요/);
    assert.throws(() => s.widgets.construct(7, [entry('x', 0)], 0), /문자열 또는 자식 플래그 필요/); assert.throws(() => s.widgets.construct(7, [entry('x', MENU)], 0), /실제 자식 토큰 필요/);
    // 조건 false 인 문자열은 실제 문자열이 없어도 검사하지 않는다. 항목이 40개가 되면 거부한다.
    assert.doesNotThrow(() => s.widgets.construct(7, [entry(5, TEXT | 0x100, 0, 0, 0)], 0));
    const full = Array.from({ length: 40 }, (_, index) => entry(`i${index}`, TEXT)); assert.throws(() => s.widgets.construct(7, full, 0), /numItems < 40/); assert.equal(s.newMenu.itemCount, 40);
    assert.doesNotThrow(() => s.widgets.construct(7, full.slice(0, 39), 0)); assert.throws(() => s.widgets.construct(7, [entry('x', TEXT | 1, 1)].concat(Array.from({ length: 39 }, () => entry('y', TEXT))), 0), /numItems < 40/);
});

test('construct(자식 항목): 자식은 visible 해제→가상 Attach(자식,0,0,메뉴,0)→직전 항목에 자식 비트와 ID 를 쓰며 첫 항목이면 연결하지 않는다', () => {
    const s = widgetScene(); s.childA.id = 6; s.childA.flags |= 1; s.widgets.construct(7, [entry('A', TEXT), entry(5, MENU)], 0);
    assert.equal(s.childA.flags & 1, 0); assert.deepEqual(rows(s.log, 'vattach'), [[5, 0, 0, 7, 0]]); assert.deepEqual([s.newMenu.items[0].flags, s.newMenu.items[0].childId], [TEXT | 0x20, 6]);
    // 첫 항목(항목 수 0)이 자식이면 Attach 만 하고 항목은 건드리지 않는다. 조건 false 면 Attach 도 없다.
    const t = widgetScene(); t.childA.flags |= 1; t.widgets.construct(7, [entry(5, MENU)], 0); assert.deepEqual(rows(t.log, 'vattach'), [[5, 0, 0, 7, 0]]); assert.equal(t.newMenu.items[0].flags, 0); assert.equal(t.newMenu.itemCount, 0);
    const u = widgetScene(); u.widgets.construct(7, [entry(5, MENU | 0x100, 0, 0, 0)], 0); assert.deepEqual(rows(u.log, 'vattach'), []);
    // 자식 저장소가 없거나 Attach 경계가 없으면 거부한다.
    const v = widgetScene({ omit: ['attach'] }); assert.throws(() => v.widgets.construct(7, [entry(5, MENU)], 0), /자식 Attach 경계 필요/); assert.throws(() => widgetScene().widgets.construct(7, [entry(99, MENU)], 0), /메뉴 생성 자식\/부모 노드 없음/);
});

test('construct(장식): 장식(0xc4)→Attach→깊이 1→제목(0xd8)→Attach(16,16,장식)→깊이 -1→배경 flags 0x40000 순서이고 가로 비트는 거부한다', () => {
    const s = widgetScene({ tokens: [3, 4] }); s.widgets.construct(7, [], 0xa80000);
    assert.deepEqual(s.log.filter((entry) => ['allocate', 'decoration', 'vattach', 'title'].includes(entry[0])).map((entry) => entry.join(',')), ['allocate,196', 'decoration,3,1,1', 'vattach,3,0,0,7,0', 'allocate,216', 'title,4,0,0,0,21', 'vattach,4,16,16,3,0']);
    assert.deepEqual([s.newMenu.fancy, s.newMenu.title], [3, 4]); assert.equal(s.nodes.get(3).secondaryDepth, 1); assert.equal(s.nodes.get(4).secondaryDepth, 0xffffffff); assert.equal(s.newMenu.backgroundFlags, BACKGROUND | 0x40000);
    // 장식 메뉴는 가로 비트를 쓸 수 없다(생성 일부는 이미 반영된다). 할당 경계가 없으면 거부한다.
    assert.throws(() => widgetScene({ tokens: [3, 4] }).widgets.construct(7, [], FANCY | HORIZONTAL), /장식 메뉴는 가로 플래그를 사용할 수 없음/); assert.throws(() => widgetScene({ omit: ['allocate'] }).widgets.construct(7, [], FANCY), /장식\/제목 할당 경계 필요/);
    // 할당 실패(0)는 토큰 0 이 되어 Attach 에서 거부한다. 장식 비트 하나만 보고 판단한다.
    const t = widgetScene({ tokens: [0] }); assert.throws(() => t.widgets.construct(7, [], FANCY), /메뉴 생성 자식\/부모 노드 없음/); assert.equal(t.newMenu.fancy, 0);
    const u = widgetScene(); u.widgets.construct(7, [], 1); assert.equal(only(u.log, 'allocate').length, 0);
});

test('begin*: 칸 증가→빈 칸 확인→0xb90 할당→생성→표 기록→(idle)→(추가 배경 flags) 순서이고 각 변형의 생성 flags 가 다르다', () => {
    const s = widgetScene(); s.state.index = 0; const token = s.widgets.beginVertical(0x1000); assert.equal(token, 7);
    assert.equal(s.state.index, 1); assert.equal(s.state.entries[1], 7); assert.deepEqual(rows(s.log, 'allocate')[0], [0xb90]); assert.equal(s.newMenu.menuFlags, 0x280000); assert.deepEqual(rows(s.log, 'idle'), [[7, true]]); assert.equal(s.newMenu.backgroundFlags, BACKGROUND | 0x1000);
    const t = widgetScene(); t.state.index = 3; assert.equal(t.widgets.beginHorizontal(), 7); assert.equal(t.newMenu.menuFlags, 0x140000); assert.deepEqual(rows(t.log, 'idle'), [[7, true]]); assert.equal(t.newMenu.backgroundFlags, BACKGROUND); assert.equal(t.state.entries[4], 7);
    const u = widgetScene(); assert.equal(u.widgets.beginFancy(), 7); assert.equal(u.newMenu.menuFlags, 0xa80000); assert.deepEqual(rows(u.log, 'idle'), []); assert.equal(u.newMenu.backgroundFlags, BACKGROUND | 0x40000);
    const v = widgetScene(); v.widgets.beginVertical(); assert.equal(v.newMenu.backgroundFlags, BACKGROUND);
});

test('begin*: 예약 칸·점유 칸·표 범위·할당/생성 실패는 도달 지점에서 거부하고 증가한 index 는 되돌리지 않는다', () => {
    const s = widgetScene(); s.state.index = 0xffffffff; assert.throws(() => s.widgets.beginVertical(), /예약 칸 금지/); assert.equal(s.state.index, 0);
    const t = widgetScene(); t.state.index = 1; t.state.entries[2] = 9; assert.throws(() => t.widgets.beginHorizontal(), /시작 칸이 비어 있어야 함/); assert.equal(t.state.index, 2);
    const u = widgetScene(); u.state.index = 19; assert.throws(() => u.widgets.beginFancy(), /생성 칸 범위 초과/); assert.equal(u.state.index, 20);
    assert.throws(() => widgetScene({ omit: ['allocate'] }).widgets.beginVertical(), /할당 경계 필요/);
    // 할당 실패는 0 을 표에 기록하고 idle 등록은 0 토큰으로 이어진다(수명 처리기가 거부한다).
    const v = widgetScene({ tokens: [0] }); v.state.index = 0; assert.equal(v.widgets.beginFancy(), 0); assert.equal(v.state.entries[1], 0);
    // idle 콜백이 index 를 바꿔도 이후 읽기는 현재 index 를 쓴다.
    // idle 콜백이 index 를 바꾸면 이후 읽기는 현재 칸을 쓴다: 가로는 그 칸의 값(0)을 반환하고 세로는 배경 flags 쓰기에서 0 토큰을 거부한다.
    const w = widgetScene(); w.state.index = 0; w.lifetime.setIdle = () => { w.state.index = 2; }; assert.equal(w.widgets.beginHorizontal(), 0);
    const x = widgetScene(); x.state.index = 0; x.lifetime.setIdle = () => { x.state.index = 2; }; assert.throws(() => x.widgets.beginVertical(0x10), /메뉴 생성 공유 노드 없음/);
});

test('attach(일반): root 부모면 0x4000 을 켜고 화면 안으로 보정한 정수 좌표를 부모 기준 float32 로 바꿔 기본 Attach 에 넘긴다', () => {
    const s = widgetScene(); const calls = []; s.tree.attach = (...args) => calls.push(args); const node = s.newMenu; Object.assign(node, { parent: 1, width: 100, height: 50 });
    // 화면 640×480: 요청 (700,-5) → x 는 640-100=540 로, y 는 -5 → 부모 원점 (0,0) 기준 (540,-5).
    s.widgets.attach(7, 700, -5, 0, 0); assert.deepEqual(calls, [[7, 540, -5, 1, 0]]); assert.equal(node.flags & 0x4000, 0x4000);
    // 화면 아래/오른쪽 한도: (600, 470) → (540, 430). 정수 절삭: 소수 좌표는 float32 로 좁힌 요청을 절삭한다.
    calls.length = 0; s.widgets.attach(7, 10.9, 20.9, 1, 0); assert.deepEqual(calls, [[7, 10, 20, 1, 0]]);
    // 부모 지정이 있으면 노드의 현재 부모를 쓰지 않는다. 부모 0 은 현재 부모를 쓴다.
    calls.length = 0; node.parent = 1; s.widgets.attach(7, 1, 2, 0, 0); assert.equal(calls[0][3], 1); assert.deepEqual(rows(s.log, 'vattach'), []);
});

test('attach: root 부모 비트는 정확히 0x4000 만 더하고 장식 보정 좌표는 NaN/범위 밖은 0, 32비트를 넘으면 하위 DWORD 로 절삭한다', () => {
    const s = widgetScene(); const calls = []; s.tree.attach = (...args) => calls.push(args); Object.assign(s.newMenu, { parent: 1, menuFlags: FANCY | VERTICAL, fancy: 3, width: 80, height: 40, flags: 8 }); Object.assign(s.nodes.get(3), { x: -16, y: -52, width: 112, height: 138 });
    s.widgets.attach(7, 10, 20, 0, 0); assert.equal(s.newMenu.flags, 8 | 0x4000);
    // NaN/무한대/±2^63 이상은 무효 절삭이라 0 이고 보정 뒤 (16, 52) 가 된다.
    for (const bad of [NaN, Infinity, 1e19]) { calls.length = 0; s.widgets.attach(7, bad, 20, 0, 0); assert.deepEqual(calls[0].slice(1, 3), [bad === Infinity ? 16 : 16, 52], String(bad)); }
    // 4294967808(2^32+512)은 하위 32비트 496 으로 절삭되어 한도 안이고 장식 x 만큼 되돌리면 512 다(33비트로 읽으면 음수라 0 이 되어 16).
    calls.length = 0; s.widgets.attach(7, 4294967808, 20, 0, 0); assert.deepEqual(calls[0].slice(1, 3), [512, 52]);
});

test('attach(보정 생략): mode bit0 이거나 노드가 0x20000(제한 없음)이면 좌표를 float32 로만 넘기고 그 밖의 부모는 root 비트를 켜지 않는다', () => {
    const s = widgetScene(); const calls = []; s.tree.attach = (...args) => calls.push(args); Object.assign(s.newMenu, { parent: 2, width: 100, height: 50 });
    s.widgets.attach(7, 700.5, -5.25, 0, 1); assert.deepEqual(calls[0], [7, 700.5, -5.25, 2, 1]); assert.equal(s.newMenu.flags & 0x4000, 0);
    s.newMenu.flags |= 0x20000; s.widgets.attach(7, 700.5, -5.25, 0, 0); assert.deepEqual(calls[1], [7, 700.5, -5.25, 2, 0]);
    // mode bit1 등 다른 비트는 보정을 생략하지 않는다.
    s.newMenu.flags &= ~0x20000; Object.assign(s.nodes.get(2), { x: 0, y: 0, parent: 1 }); s.widgets.attach(7, 900, 0, 2, 2); assert.deepEqual(calls[2], [7, 540, 0, 2, 2]);
});

test('attach(원점/한도): 부모의 화면 위치를 더해 전체 화면 한도를 적용하고 float32 로 좁히며 요청이 NaN 이면 요청 값을 그대로 절삭한다', () => {
    const s = widgetScene(); const calls = []; s.tree.attach = (...args) => calls.push(args); Object.assign(s.nodes.get(2), { parent: 1, x: 100, y: 50 }); Object.assign(s.newMenu, { parent: 2, width: 100, height: 50 });
    // 부모 원점 (100,50): 요청 (30,20) → 화면 (130,70) 한도 안 → 로컬 (30,20).
    s.widgets.attach(7, 30, 20, 0, 0); assert.deepEqual(calls[0], [7, 30, 20, 2, 0]);
    // 요청 (600,500) → 화면 (700,550) → 한도 (540,430) → 로컬 (440,380).
    s.widgets.attach(7, 600, 500, 0, 0); assert.deepEqual(calls[1], [7, 440, 380, 2, 0]);
    // 요청 NaN: min 은 요청값을 택하고 절삭은 0 → 로컬 (-100,-50).
    s.widgets.attach(7, NaN, NaN, 0, 0); assert.deepEqual(calls[2], [7, -100, -50, 2, 0]);
    // 소수 요청의 float32 좁힘은 절삭 전에 일어난다: 16777217.5 는 float32 로 16777218 이다(화면 한도 밖이므로 한도 값 540).
    s.widgets.attach(7, 1e7 + 0.75, 0, 0, 0); assert.equal(calls[3][1], 540 - 100);
});

test('attach(장식 메뉴): 장식 위치/크기로 화면 보정하고 장식 x/y 만큼 되돌려 로컬 좌표를 만든다', () => {
    const s = widgetScene(); const calls = []; s.tree.attach = (...args) => calls.push(args); Object.assign(s.newMenu, { parent: 1, menuFlags: FANCY | VERTICAL, fancy: 3, width: 80, height: 40 }); Object.assign(s.nodes.get(3), { x: -16, y: -52, width: 112, height: 138 });
    // 요청 (10,20): 화면 x = ftol((0+(-16))+10) = -6 → 0 으로, y = ftol((0+20)+(-52)) = -32 → 0 으로. 한도: 640-112=528 / 480-138=342 안. 로컬 = 0-(-16)=16, 0-(-52)=52.
    s.widgets.attach(7, 10, 20, 0, 0); assert.deepEqual(calls[0], [7, 16, 52, 1, 0]);
    // 요청 (700,500): 화면 (684,448) → 한도 (528,342) → 로컬 (528+16, 342+52).
    s.widgets.attach(7, 700, 500, 0, 0); assert.deepEqual(calls[1], [7, 544, 394, 1, 0]);
    // 장식 토큰이 없으면 보정에서 거부한다(부착은 하지 않는다).
    s.newMenu.fancy = 0; assert.throws(() => s.widgets.attach(7, 1, 1, 0, 0), /메뉴 생성 자식\/부모 노드 없음/); assert.equal(calls.length, 2);
});

test('deleteScalar: 가상 표→개수 감소→선택 통지→40개 조각 역순 Clear→기본 scalar 소멸 순서이고 항목 수/스타일 플래그로 줄이지 않는다', () => {
    const s = widgetScene(); s.widgets.construct(7, [], 0); const tokens = []; for (const index of [39, 7, 0]) { const token = s.style.newText(Uint8Array.of(0x41, 0), new Uint32Array(8)); s.newMenu.items[index].styled = token; tokens.push(token); }
    s.state.instances = 1; s.log.length = 0; assert.equal(s.widgets.deleteScalar(7, 1), 7);
    // 역순: 39 → 7 → 0 의 토큰이 먼저 비워진다(나머지 칸은 0 토큰이라 비움 대상 아님). 소유 필드는 모두 0 으로 쓴다.
    assert.deepEqual(s.cleared.filter(Boolean), tokens); assert.ok(s.newMenu.items.every((item) => item.styled === 0)); assert.equal(s.cleared.length, 40);
    assert.deepEqual(rows(s.log, 'scalar'), [[7, 1]]); assert.equal(s.state.instances, 0);
    // 선택 통지 토큰이 0 이면 통지 경계가 없어도 된다. 있으면 경계 필요.
    assert.deepEqual(rows(s.log, 'notice'), []); s.newMenu.selectionCallback = 5; const t = widgetScene({ omit: ['selectionNotice'] }); t.widgets.construct(7, [], 0); t.newMenu.selectionCallback = 5; assert.throws(() => t.widgets.deleteScalar(7, 0), /선택 통지 경계 필요/); assert.equal(t.newMenu.vtable, 0x0050c6f0);
    // 개수 DWORD 는 0 에서 감소하면 감긴다.
    const u = widgetScene(); u.widgets.construct(7, [], 0); u.state.instances = 0; u.widgets.deleteScalar(7, 0); assert.equal(u.state.instances, 0xffffffff);
});

test('reference/attachObject: Form 생성(현재 TYPE, mode 2)→소유자 등록→객체 연결 뒤 같은 노드의 연결 필드만 쓰고 object 0 은 아무것도 하지 않는다', () => {
    const s = widgetScene(); s.widgetState.objectFormType = 11; s.widgetState.currentObjectFormType = () => 14; const node = s.newMenu; node.id = 33; node.form = 5; node.object = 6;
    s.widgets.reference(7, 0); assert.deepEqual(s.log, []); assert.deepEqual([node.object, node.form], [6, 5]);
    node.flags = 8; const before = node.flags; s.widgets.reference(7, 0x1234); assert.deepEqual(s.log, [['form', 14, 2], ['owner', 77, 33], ['object', 77, 0x1234, 0]]); assert.equal(node.flags, (before | 0x40) >>> 0);
    s.log.length = 0; node.object = 0; s.widgets.reference(7, 1); assert.deepEqual(rows(s.log, 'object'), [[77, 1, 0]]); assert.equal(node.object, 1); node.flags = before; s.log.length = 0; s.widgets.reference(7, 0x1234); assert.deepEqual([node.object, node.form], [0x1234, 8]);
    // 현재 TYPE 조회가 없으면 독립 값 사본을 쓴다.
    s.log.length = 0; s.widgetState.currentObjectFormType = null; s.widgets.reference(7, 9); assert.deepEqual(s.log[0], ['form', 11, 2]);
    // 실패/경계 부재.
    s.on.form = () => ({ token: 0, index: 0 }); assert.throws(() => s.widgets.reference(7, 9), /Form 생성 실패/);
    for (const omit of ['objectForm', 'formOwner', 'formObject']) assert.throws(() => widgetScene({ omit: [omit] }).widgets.reference(7, 9), /객체 부착 Form 경계 필요/, omit);
    // attachObject 는 연결 뒤 현재 가상 Attach 를 부른다. object 0 도 부착은 한다.
    const t = widgetScene(); t.widgets.attachObject(7, 0, 1, 2, 3, 4); assert.deepEqual(rows(t.log, 'vattach'), [[7, 1, 2, 3, 4]]); assert.deepEqual(rows(t.log, 'form'), []);
    assert.throws(() => widgetScene({ omit: ['attach'] }).widgets.attachObject(7, 0, 1, 2, 3, 4), /객체 가상 Attach 경계 필요/); t.widgets.setIdle(7, true); assert.deepEqual(rows(t.log, 'idle'), [[7, true]]);
});

test('bindMenuTitleLayout: 제목 Reflow 경계를 RequestScroll/FitContent 에 signed 값으로 연결한다', () => {
    const calls = []; const text = { requestScroll: (...args) => calls.push(['scroll', ...args]), fitContent: (...args) => calls.push(['fit', ...args]) }; const hooks = {}; bindMenuTitleLayout(hooks, text);
    hooks.titleFit(4, 0xffffffff); hooks.titleLayout(4, 0xfffffffe, 3); assert.deepEqual(calls, [['scroll', 4, -1], ['fit', 4, -2, 3]]);
    calls.length = 0; hooks.titleFit(4, 6); hooks.titleLayout(4, 8, 10); assert.deepEqual(calls, [['scroll', 4, 6], ['fit', 4, 8, 10]]);
});

test('bindMenuGumpWidgets/bindMenuGumpObjectAttach: 메뉴만 새 몸체로 보내고 다른 토큰은 이전 처리기를 보존하며 기본 처리기 부재는 거부한다', () => {
    const s = widgetScene(); const calls = []; const tree = { attach: (...args) => calls.push(['tree', ...args]) }; const life = { deletingDestructor: (...args) => { calls.push(['life', ...args]); } };
    bindMenuGumpWidgets(tree, life, (token) => (token === 7 ? s.widgets : null)); tree.attach(9, 1, 2, 3, 4, { xBits: 1 }); life.deletingDestructor(9, 1); assert.deepEqual(calls, [['tree', 9, 1, 2, 3, 4, { xBits: 1 }], ['life', 9, 1]]);
    Object.assign(s.newMenu, { parent: 1, width: 10, height: 10 }); s.tree.attach = (...args) => calls.push(['real', ...args]); tree.attach(7, 5, 6, 0, 0); assert.deepEqual(calls[2], ['real', 7, 5, 6, 1, 0]); life.deletingDestructor(7, 1); assert.deepEqual(rows(s.log, 'scalar'), [[7, 1]]);
    assert.throws(() => bindMenuGumpWidgets({}, {}, null), /메뉴 생성\/수명 분배 조회 필요/); const bt = {}; const bl = {}; bindMenuGumpWidgets(bt, bl, () => null); assert.throws(() => bt.attach(1, 0, 0, 0, 0), /메뉴 외 Attach 경계 필요/); assert.throws(() => bl.deletingDestructor(1, 0), /메뉴 외 소멸 경계 필요/);
    const hooks = { attachObject: (...args) => calls.push(['object', ...args]) }; bindMenuGumpObjectAttach(hooks, (token) => (token === 7 ? s.widgets : null)); hooks.attachObject(9, 1, 2, 3, 4, 5); assert.deepEqual(calls.at(-1), ['object', 9, 1, 2, 3, 4, 5]);
    hooks.attachObject(7, 0, 1, 2, 3, 4); assert.deepEqual(rows(s.log, 'vattach').at(-1), [7, 1, 2, 3, 4]); assert.throws(() => bindMenuGumpObjectAttach({}, null), /메뉴 객체 부착 분배 조회 필요/); const bo = {}; bindMenuGumpObjectAttach(bo, () => null); assert.throws(() => bo.attachObject(1, 0, 0, 0, 0, 0), /메뉴 외 객체 부착 경계 필요/);
});
