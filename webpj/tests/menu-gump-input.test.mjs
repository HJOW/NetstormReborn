// 전용 PE fixture가 없는 MenuGumpInput(메뉴 선택 통지/클릭 처리/하위 메뉴 열기·닫기/명령/팝업 닫기/행 갱신 영역)을 기준선 소스 계약과 손으로 계산한 값으로 검증한다. (2026-11-14 WB04-E431)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(MenuGumpInput.cpp)에서 옮겼다. 위치 수집기·메뉴 크기/확장 처리기·기본 생성 저장소는 실제이고 수명/사건/트리/가상 통지/명령 큐/소리는 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { MenuGumpInput, MenuGumpInputState, bindMenuGumpInput, bindMenuGumpInputSound } from '../www/ui/menu-gump-input.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { menuScene, only } from './menu-gump-support.mjs';

const TEXT = 0x10000; const COMMAND = 1; const KEEP = 2; const SELECTED = 8; const CHILD = 0x20; const PARAMETER = 0x80; const DISABLED = 0x200; const DYNAMIC = 0x800;
const HORIZONTAL = 0x40000; const VERTICAL = 0x80000; const ROOT_MENU = 0x100000; const DELETE_COMMAND = 0x200000; const COMPACT = 0x1000000; const DELETE_OUTSIDE = 0x4000000;
const VISIBLE = 1; const POPUP = 0x4000; const DEAD = 0x10000; const LEFT = 0x10000; const RIGHT = 0x20000; const RELEASE = 0x80000000; const OUTSIDE = 0x4000000;

// 루트(1) 아래 메뉴(2)를 (100,50) 80×60 세로 3항목(offset 0/20/40, 높이 20)으로 두고 자식 메뉴 5/6, 팝업 후보 3/4 를 만든다. 메뉴 식별은 26 이다.
function inputScene(options = {}) {
    const s = menuScene(); const { nodes } = s; const log = s.log; const on = s.on;
    Object.assign(nodes.get(1), { child: 2 }); Object.assign(nodes.get(2), { parent: 1, x: 100, y: 50, width: 80, height: 60, command: 26, menuFlags: VERTICAL | 1, itemCount: 3, itemWidth: 80, flags: nodes.get(2).flags | VISIBLE });
    nodes.get(2).offsets.set([0, 20, 40]); nodes.get(2).heights.set([20, 20, 20]); for (let i = 0; i < 3; i += 1) nodes.get(2).items[i].flags = TEXT;
    for (const token of [5, 6]) Object.assign(nodes.get(token), { parent: 2, command: 26, menuFlags: VERTICAL | 1, targetWidth: 30, targetHeight: 40, width: 30, height: 40 });
    const inputState = new MenuGumpInputState(); Object.assign(inputState, { requestEvent: 7, closedEvent: 9 });
    const collection = new GumpCollection(s.create.collection, { node: s.hooks.node });
    const lifetime = { kill: (token) => log.push(['kill', token]) }; const tree = { addDepth: (token, delta) => log.push(['depth', token, delta]) }; const events = { bubble: (...args) => { log.push(['bubble', ...args]); return 0; } };
    const record = (name) => (...args) => { log.push([name, ...args]); return on[name]?.(...args); };
    const hooks = { node: s.hooks.node, base: s.hooks.baseNode, broadcast: record('vbroadcast'), handle: (...args) => { log.push(['vhandle', ...args]); return on.vhandle?.(...args) ?? 0; }, push: record('vpush'), attach: record('vattach'), redraw: record('vredraw'), command: record('command'), sound: record('sound'), invalidate: (rect) => { log.push(['invalidate', { ...rect }]); } };
    for (const key of options.omit ?? []) delete hooks[key];
    const input = new MenuGumpInput(s.create, inputState, s.menu, collection, events, lifetime, tree, hooks);
    return { ...s, input, inputState, collection, lifetime, tree, events, inputHooks: hooks, menuNode: nodes.get(2) };
}
const select = (s, index) => { s.menuNode.selection = index; s.menuNode.items[index].flags |= SELECTED; };
const rows = (log, name) => only(log, name).map((entry) => entry.slice(1));

test('생성자/노드 검증: 두 조회 경계가 필수이고 기본/파생 주소가 같은 노드만 허용한다', () => {
    const s = inputScene();
    for (const missing of ['node', 'base']) { const hooks = { ...s.inputHooks }; delete hooks[missing]; assert.throws(() => new MenuGumpInput(s.create, s.inputState, s.menu, s.collection, s.events, s.lifetime, s.tree, hooks), /메뉴 입력 공유 저장소 필요/, missing); }
    const split = new MenuGumpInput(s.create, s.inputState, s.menu, s.collection, s.events, s.lifetime, s.tree, { ...s.inputHooks, base: () => ({}) }); assert.throws(() => split.close(2, true), /메뉴 입력 공유 노드 없음/);
    assert.throws(() => s.input.redrawItem(0, 0), /메뉴 입력 기본 노드 없음/); assert.equal(new MenuGumpInputState().alignment, 26); const fresh = new MenuGumpInputState(); assert.deepEqual([fresh.requestEvent, fresh.closedEvent], [0, 0]);
});

test('handle 거절: 확장 중·왼쪽/오른쪽 누름이 아닌 사건·release 는 0 을 돌려주고 가상 통지를 부르지 않는다', () => {
    const s = inputScene(); s.menuNode.expanding = 1; assert.equal(s.input.handle(2, LEFT, 150, 80), 0); s.menuNode.expanding = 0;
    for (const event of [0, 0x30000, 0x40000, LEFT | RELEASE, RIGHT | RELEASE]) assert.equal(s.input.handle(2, event >>> 0, 150, 80), 0, String(event));
    assert.deepEqual(s.log, []);
    // 종류 마스크(0x03ffffff) 밖의 높은 비트(0x04000000, 0x40000000)는 종류에 영향이 없고 마스크 안의 다른 비트(0x100000)는 종류를 바꾼다.
    s.on.vbroadcast = () => { s.menuNode.selection = -1; }; assert.equal(s.input.handle(2, (LEFT | 0x40000000) >>> 0, 1, 2), 1); assert.equal(only(s.log, 'vbroadcast').length, 1);
    assert.equal(s.input.handle(2, (LEFT | 0x04000000) >>> 0, 1, 2), 1); assert.equal(s.input.handle(2, (LEFT | 0x100000) >>> 0, 1, 2), 0);
});

test('handle(선택 없음): 가상 통지 뒤 선택이 없으면 1 을 반환하고 외부 누름 삭제 비트가 있으면 메뉴를 Kill 한다', () => {
    const s = inputScene(); assert.equal(s.input.handle(2, RIGHT, 150, 80), 1); assert.deepEqual(s.log, [['vbroadcast', 2, RIGHT, 150, 80]]);
    s.log.length = 0; s.menuNode.menuFlags |= DELETE_OUTSIDE; assert.equal(s.input.handle(2, LEFT, 1, 2), 1); assert.deepEqual(s.log, [['vbroadcast', 2, LEFT, 1, 2], ['kill', 2]]);
    const t = inputScene({ omit: ['broadcast'] }); assert.throws(() => t.input.handle(2, LEFT, 1, 2), /메뉴 가상 통지 경계 필요/);
});

test('handle(명령): 인자 비트가 없으면 flags 전체를 인자로 명령 큐에 넣고 소리→Close→부모 가지 닫기→팝업 닫기 순서다', () => {
    const s = inputScene(); s.menuNode.items[1].flags = TEXT | COMMAND; s.menuNode.items[1].command = 55; s.menuNode.items[0].flags = TEXT | SELECTED; s.on.vbroadcast = () => { select(s, 1); };
    // 팝업 후보: 루트 자식 체인 3(POPUP)→4(POPUP|DEAD)→2. 명령 뒤에 live 팝업만 Kill 한다.
    Object.assign(s.nodes.get(1), { child: 3 }); Object.assign(s.nodes.get(3), { flags: POPUP | 8, next: 4, parent: 1 }); Object.assign(s.nodes.get(4), { flags: POPUP | DEAD | 8, next: 2, parent: 1 });
    assert.equal(s.input.handle(2, LEFT, 150, 80), 1);
    assert.deepEqual(s.log.map((entry) => entry[0]), ['vbroadcast', 'command', 'sound', 'kill']); assert.deepEqual(rows(s.log, 'command'), [[55, 1, TEXT | COMMAND | SELECTED]]); assert.deepEqual(rows(s.log, 'sound'), [['openSubGump.wav']]); assert.deepEqual(rows(s.log, 'kill'), [[3]]);
    // Close(clear=true) 가 모든 강조를 지운다(선택 인덱스는 유지).
    assert.ok(s.menuNode.items.every((item) => !(item.flags & SELECTED))); assert.equal(s.menuNode.selection, 1);
    // 명시 인자 비트가 있으면 항목 인자를 쓴다.
    s.log.length = 0; s.menuNode.items[1].flags = TEXT | COMMAND | PARAMETER; s.menuNode.items[1].parameter = 99; s.input.handle(2, RIGHT, 150, 80); assert.deepEqual(rows(s.log, 'command'), [[55, 1, 99]]);
    for (const omit of ['command', 'sound']) { const t = inputScene({ omit: [omit] }); t.menuNode.items[1].flags = TEXT | COMMAND; t.on.vbroadcast = () => select(t, 1); assert.throws(() => t.input.handle(2, LEFT, 1, 2), omit === 'command' ? /메뉴 명령 큐 경계 필요/ : /메뉴 음향 경계 필요/, omit); }
});

test('handle(유지/닫기): 유지 비트가 있으면 부모 가지를 닫지 않고, 없으면 루트 메뉴 경계까지 올라가며 강조를 지운다', () => {
    const s = inputScene(); s.menuNode.items[0].flags = TEXT | KEEP | COMMAND; s.on.vbroadcast = () => select(s, 0); s.input.handle(2, LEFT, 1, 2);
    // KEEP 이므로 Close(clearSelection=false): 강조가 남는다.
    assert.equal(s.menuNode.items[0].flags & SELECTED, SELECTED);
    // 자식 메뉴(5)에서 클릭: 부모 2 는 메뉴 식별이므로 올라가고, 루트(식별 아님)에서 멈춰 2 를 Close(true) 한다.
    const t = inputScene(); const child = t.nodes.get(5); child.items[0].flags = TEXT; child.itemCount = 1; t.menuNode.items[2].flags = TEXT | CHILD; t.menuNode.items[2].childId = 4; t.menuNode.items[1].flags = TEXT | SELECTED; child.items[0].flags = TEXT | SELECTED;
    t.on.vbroadcast = () => { child.selection = 0; }; t.input.handle(5, LEFT, 1, 2);
    assert.equal(t.menuNode.items[1].flags & SELECTED, 0); assert.equal(child.items[0].flags & SELECTED, 0);
    // 루트 메뉴 비트가 있으면 그 메뉴에서 멈춘다(부모 2 는 닫지 않는다).
    const u = inputScene(); const kid = u.nodes.get(5); kid.menuFlags |= ROOT_MENU; kid.itemCount = 1; kid.items[0].flags = TEXT | SELECTED; u.menuNode.items[1].flags = TEXT | SELECTED; u.on.vbroadcast = () => { kid.selection = 0; }; u.input.handle(5, LEFT, 1, 2);
    assert.equal(kid.items[0].flags & SELECTED, 0); assert.equal(u.menuNode.items[1].flags & SELECTED, SELECTED);
    // 부모 체인이 루트에 닿으면(부모 0) 기본 노드 조회가 실패한다.
    const v = inputScene(); v.nodes.get(1).command = 26; v.menuNode.items[0].flags = TEXT; v.on.vbroadcast = () => select(v, 0); assert.throws(() => v.input.handle(2, LEFT, 1, 2), /메뉴 입력 기본 노드 없음/);
});

test('handle(하위 메뉴): 자식 항목은 Push→visible→항목 위치 Attach→깊이 -1→확장 시작→열기 소리→Redraw 순서이고 방향은 부모 반대다', () => {
    const s = inputScene(); const child = s.nodes.get(5); child.flags = 8; s.menuNode.items[1].flags = TEXT | CHILD; s.menuNode.items[1].childId = 4; s.on.vbroadcast = () => select(s, 1);
    assert.equal(s.input.handle(2, LEFT, 1, 2), 1);
    assert.deepEqual(s.log.map((entry) => entry[0]), ['vbroadcast', 'vpush', 'vattach', 'depth', 'sound', 'vredraw']);
    assert.deepEqual(rows(s.log, 'vpush'), [[5]]); assert.equal(child.flags & VISIBLE, VISIBLE);
    // 세로 부모: 항목 위치 = (현재 폭 80, offset 20), 자식은 가로 확장(폭 0, 높이 목표).
    assert.deepEqual(rows(s.log, 'vattach'), [[5, 80, 20, 0, 0]]); assert.deepEqual(rows(s.log, 'depth'), [[5, 0xffffffff]]); assert.deepEqual(rows(s.log, 'sound'), [['openGump.wav']]); assert.deepEqual(rows(s.log, 'vredraw'), [[2, false]]);
    assert.deepEqual([child.baseWidth, child.baseHeight, child.expanding], [0, 40, 1]); assert.equal(s.lifeState.idle[0], 5);
    // 유지 비트는 닫기에서 강조를 지우지 않는다: CHILD 항목은 Close(clearSelection=false) 라 강조가 남는다.
    assert.equal(s.menuNode.items[1].flags & SELECTED, SELECTED);
    // 가로 부모: 자식은 세로 확장(폭 목표, 높이 0), 위치 = (index×항목 폭, 현재 높이).
    const t = inputScene(); const kid = t.nodes.get(6); kid.flags = 8; Object.assign(t.menuNode, { menuFlags: HORIZONTAL | 1, itemWidth: 25 }); t.menuNode.items[2].flags = TEXT | CHILD; t.menuNode.items[2].childId = 5; t.on.vbroadcast = () => select(t, 2); t.input.handle(2, LEFT, 1, 2);
    assert.deepEqual(rows(t.log, 'vattach'), [[6, 50, 60, 0, 0]]); assert.deepEqual([kid.baseWidth, kid.baseHeight], [30, 0]);
    // 자식 등록 ID 가 비었으면 변경 전에 거부한다.
    const u = inputScene(); u.menuNode.items[1].flags = TEXT | CHILD; u.menuNode.items[1].childId = 77; u.on.vbroadcast = () => select(u, 1); assert.throws(() => u.input.handle(2, LEFT, 1, 2), /메뉴 자식 등록 ID 없음/);
    for (const omit of ['push', 'attach']) { const w = inputScene({ omit: [omit] }); w.nodes.get(5).flags = 8; w.menuNode.items[1].flags = TEXT | CHILD; w.menuNode.items[1].childId = 4; w.on.vbroadcast = () => select(w, 1); assert.throws(() => w.input.handle(2, LEFT, 1, 2), omit === 'push' ? /메뉴 자식 Push 경계 필요/ : /메뉴 자식 Attach 경계 필요/, omit); }
});

test('handle(동적 자식): 자식이 없는 동적 항목은 요청 사건을 가상 입력으로 보내 반환 토큰을 숨긴 채 부착하고 자식 비트를 세운 뒤 연다', () => {
    const s = inputScene(); const child = s.nodes.get(5); child.flags = 8 | VISIBLE; s.menuNode.items[0].flags = TEXT | DYNAMIC; s.menuNode.items[0].parameter = 3; s.on.vbroadcast = () => select(s, 0); s.on.vhandle = () => 5;
    assert.equal(s.input.handle(2, LEFT, 1, 2), 1);
    // 가상 입력 인자: (메뉴, 요청 사건 7, 메뉴 serial, 항목 flags 전체(인자 비트 없음)).
    assert.deepEqual(rows(s.log, 'vhandle'), [[2, 7, s.menuNode.serial, TEXT | DYNAMIC | 0]]);
    assert.equal(s.menuNode.items[0].childId, child.id); assert.equal(s.menuNode.items[0].flags & CHILD, CHILD); assert.deepEqual(rows(s.log, 'vattach')[0], [5, 0, 0, 2, 0]);
    // 이어서 일반 자식 열기 경로(Push→visible→Attach)가 실행된다.
    assert.deepEqual(s.log.map((entry) => entry[0]).slice(0, 6), ['vbroadcast', 'vhandle', 'vattach', 'vpush', 'vattach', 'depth']); assert.equal(child.flags & VISIBLE, VISIBLE);
    // 반환 0 이면 자식이 없고 일반 닫기로 이어진다.
    const t = inputScene(); t.menuNode.items[0].flags = TEXT | DYNAMIC | PARAMETER; t.menuNode.items[0].parameter = 12; t.menuNode.items[0].childId = 0; t.on.vbroadcast = () => select(t, 0); t.input.handle(2, LEFT, 1, 2);
    assert.deepEqual(rows(t.log, 'vhandle'), [[2, 7, t.menuNode.serial, 12]]); assert.equal(t.menuNode.items[0].flags & CHILD, 0); assert.deepEqual(rows(t.log, 'vpush'), []);
    for (const omit of ['handle', 'attach']) { const u = inputScene({ omit: [omit] }); u.nodes.get(5).flags = 8; u.menuNode.items[0].flags = TEXT | DYNAMIC; u.on.vbroadcast = () => select(u, 0); u.on.vhandle = () => 5; assert.throws(() => u.input.handle(2, LEFT, 1, 2), omit === 'handle' ? /가상 입력 경계 필요/ : /동적 자식 Attach 경계 필요/, omit); }
});

test('handle(명령 뒤 삭제): 삭제 비트가 있으면 명령이 없을 때 1 만 반환하고, 명령이면 Kill 뒤 부모 체인으로 닫힘 통지를 올린 다음 팝업을 닫는다', () => {
    const s = inputScene(); s.menuNode.menuFlags |= DELETE_COMMAND; s.menuNode.items[0].flags = TEXT; s.on.vbroadcast = () => select(s, 0); assert.equal(s.input.handle(2, LEFT, 1, 2), 1); assert.deepEqual(rows(s.log, 'kill'), []); assert.deepEqual(rows(s.log, 'bubble'), []);
    const t = inputScene(); t.menuNode.menuFlags |= DELETE_COMMAND; t.menuNode.items[0].flags = TEXT | COMMAND; t.menuNode.items[0].command = 8; t.on.vbroadcast = () => select(t, 0); Object.assign(t.nodes.get(1), { child: 3 }); Object.assign(t.nodes.get(3), { flags: POPUP | 8, next: 0, parent: 1 });
    assert.equal(t.input.handle(2, LEFT, 1, 2), 1); assert.deepEqual(t.log.map((entry) => entry[0]), ['vbroadcast', 'command', 'sound', 'kill', 'bubble', 'kill']);
    assert.deepEqual(rows(t.log, 'bubble'), [[2, 9, t.menuNode.serial, t.menuNode.serial]]); assert.deepEqual(rows(t.log, 'kill'), [[2], [3]]);
    const u = inputScene({ omit: [] }); u.menuNode.items[0].flags = TEXT | COMMAND; u.on.vbroadcast = () => select(u, 0); u.input.handle(2, LEFT, 1, 2); assert.deepEqual(rows(u.log, 'kill'), []);
});

test('close: 표시 중 자식은 재귀로 닫고 redraw 뒤 숨기며, 동적 자식은 숨김 여부와 무관하게 Kill 하고 항목을 비운다', () => {
    const s = inputScene(); const a = s.nodes.get(5); const b = s.nodes.get(6); a.flags |= VISIBLE; a.itemCount = 1; a.items[0].flags = TEXT | CHILD | SELECTED; a.items[0].childId = 5; b.flags |= VISIBLE;
    s.menuNode.items[0].flags = TEXT | CHILD | SELECTED; s.menuNode.items[0].childId = 4; s.menuNode.items[1].flags = TEXT | CHILD | DYNAMIC | SELECTED; s.menuNode.items[1].childId = 5; s.menuNode.items[2].flags = TEXT | SELECTED;
    a.items[0].childId = 5; a.items[0].flags = TEXT | SELECTED; // 자기 순환을 피하려 자식 없는 항목으로 둔다.
    s.input.close(2, true);
    // 항목0: 자식 5(visible) 닫기 → redraw(5)→숨김. 항목1: 자식 6(visible) 닫기 → redraw(6)→숨김→동적 Kill.
    assert.deepEqual(s.log.map((entry) => entry.slice(0, 2)), [['vredraw', 5], ['vredraw', 6], ['kill', 6]]); assert.equal(a.flags, 8); assert.equal(b.flags, 8);
    assert.deepEqual([s.menuNode.items[1].childId, s.menuNode.items[1].flags], [0, TEXT | DYNAMIC]); assert.ok(s.menuNode.items.every((item) => !(item.flags & SELECTED))); assert.equal(a.items[0].flags & SELECTED, 0);
    // clearSelection=false 는 강조를 지우지 않는다. 숨겨진 동적 자식도 Kill 한다.
    const t = inputScene(); t.nodes.get(5).flags = 8; t.menuNode.itemCount = 2; t.menuNode.items[0].flags = TEXT | CHILD | DYNAMIC | SELECTED; t.menuNode.items[0].childId = 4; t.menuNode.items[1].flags = TEXT | SELECTED; t.input.close(2, false);
    assert.deepEqual(rows(t.log, 'kill'), [[5]]); assert.equal(t.menuNode.items[1].flags & SELECTED, SELECTED); assert.deepEqual(rows(t.log, 'vredraw'), []);
    // 항목 수 바로 뒤 칸은 사용 중이 아니므로 자식 항목이어도 닫지 않는다(항목 수 2, 칸 2 에 표시 중 자식).
    const w = inputScene(); w.nodes.get(5).flags |= VISIBLE; w.menuNode.itemCount = 2; w.menuNode.items[2].flags = TEXT | CHILD; w.menuNode.items[2].childId = 4; w.input.close(2, true); assert.deepEqual(w.log, []); assert.equal(w.nodes.get(5).flags & VISIBLE, VISIBLE);
    // signed 음수 항목 수는 반복하지 않는다. 항목 수 41 이상은 칸 범위로 거부한다.
    const u = inputScene(); u.menuNode.itemCount = 0xffffffff; u.input.close(2, true); assert.deepEqual(u.log, []); u.menuNode.itemCount = 41; assert.throws(() => u.input.close(2, true), /메뉴 입력 항목 범위 오류/);
    // 재귀 순환은 거부한다.
    const v = inputScene(); const loop = v.nodes.get(5); loop.flags |= VISIBLE; loop.itemCount = 1; loop.items[0].flags = TEXT | CHILD; loop.items[0].childId = 4; v.menuNode.items[0].flags = TEXT | CHILD; v.menuNode.items[0].childId = 4; assert.throws(() => v.input.close(2, true), /메뉴 재귀 링크 순환/);
});

test('closePopups: 실제 루트의 sticky 팝업 중 dead 아닌 직계 자식만 Kill 하며 next 는 Kill 전에 읽는다', () => {
    const s = inputScene(); Object.assign(s.nodes.get(1), { child: 3 }); Object.assign(s.nodes.get(3), { flags: POPUP | 8, next: 4 }); Object.assign(s.nodes.get(4), { flags: POPUP | DEAD | 8, next: 5 }); Object.assign(s.nodes.get(5), { flags: 8, next: 6 }); Object.assign(s.nodes.get(6), { flags: POPUP | 8, next: 0 });
    s.lifetime.kill = (token) => { s.log.push(['kill', token]); s.nodes.get(token).next = 0; }; s.input.closePopups(); assert.deepEqual(rows(s.log, 'kill'), [[3], [6]]);
    // 순환 링크는 거부한다.
    const t = inputScene(); Object.assign(t.nodes.get(1), { child: 3 }); Object.assign(t.nodes.get(3), { flags: 8, next: 4 }); Object.assign(t.nodes.get(4), { flags: 8, next: 3 }); assert.throws(() => t.input.closePopups(), /메뉴 팝업 형제 순환/);
});

test('redrawItem: visible 부모 체인에서만 compact 전체/행 영역을 양수 크기로 요청한다', () => {
    const s = inputScene(); s.input.redrawItem(2, 1); assert.deepEqual(rows(s.log, 'invalidate'), [[{ left: 100, top: 70, right: 180, bottom: 90 }]]);
    // compact 는 메뉴 전체 높이(60), 폭은 현재 폭(80)이다.
    s.log.length = 0; s.menuNode.menuFlags = VERTICAL | COMPACT | 1; s.input.redrawItem(2, 40); assert.deepEqual(rows(s.log, 'invalidate'), [[{ left: 100, top: 50, right: 180, bottom: 110 }]]);
    // 범위 밖 행/숨은 메뉴/숨은 부모/0 이하 크기는 생략하거나 거부한다.
    s.log.length = 0; s.menuNode.menuFlags = VERTICAL | 1; assert.throws(() => s.input.redrawItem(2, 40), /메뉴 입력 항목 범위 오류/); assert.throws(() => s.input.redrawItem(2, -1), /메뉴 입력 항목 범위 오류/);
    s.menuNode.flags &= ~VISIBLE; s.input.redrawItem(2, 1); s.menuNode.flags |= VISIBLE; s.nodes.get(1).flags &= ~VISIBLE; s.input.redrawItem(2, 1); s.nodes.get(1).flags |= VISIBLE; assert.deepEqual(s.log, []);
    s.menuNode.heights[1] = 0; s.input.redrawItem(2, 1); s.menuNode.heights[1] = 20; s.menuNode.width = 0; s.input.redrawItem(2, 1); s.menuNode.width = 80; assert.deepEqual(s.log, []);
    const t = inputScene({ omit: ['invalidate'] }); assert.throws(() => t.input.redrawItem(2, 1), /메뉴 행 변경 영역 경계 필요/);
    // 부모 순환은 거부한다.
    const u = inputScene(); u.nodes.get(1).parent = 2; assert.throws(() => u.input.redrawItem(2, 1), /메뉴 행 부모 순환/);
});

test('broadcast(세로): 로컬 좌표의 행을 고르고 old/new 행 갱신 사이에 선택/강조를 바꾸며 비활성은 고르지 않는다', () => {
    const s = inputScene(); s.input.broadcast(2, 0, 150, 80);
    // 로컬 (50,30) → 행 1(offset 20, 높이 20).
    assert.equal(s.menuNode.selection, 1); assert.equal(s.menuNode.items[1].flags & SELECTED, SELECTED); assert.deepEqual(rows(s.log, 'invalidate'), [[{ left: 100, top: 70, right: 180, bottom: 90 }]]); assert.deepEqual(rows(s.log, 'vredraw'), []);
    // 같은 선택이면 아무것도 하지 않는다.
    s.log.length = 0; s.input.broadcast(2, 0, 150, 80); assert.deepEqual(s.log, []);
    // 행 0 으로 이동: old 행 갱신→강조 해제→새 행 갱신→선택.
    s.input.broadcast(2, 0, 150, 55); assert.deepEqual(s.log.map((entry) => entry[0]), ['invalidate', 'invalidate']); assert.equal(s.menuNode.selection, 0); assert.equal(s.menuNode.items[1].flags & SELECTED, 0); assert.equal(s.menuNode.items[0].flags & SELECTED, SELECTED);
    assert.deepEqual(rows(s.log, 'invalidate').map((row) => row[0].top), [70, 50]);
    // 비활성 행은 선택하지 않고 기존 선택만 해제한다. 영역 밖 좌표는 선택을 해제한다.
    s.log.length = 0; s.menuNode.items[2].flags = TEXT | DISABLED; s.input.broadcast(2, 0, 150, 100); assert.equal(s.menuNode.selection, -1); assert.equal(s.menuNode.items[0].flags & SELECTED, 0); assert.equal(only(s.log, 'invalidate').length, 1);
    s.input.broadcast(2, 0, 150, 80); s.input.broadcast(2, 0, 500, 500); assert.equal(s.menuNode.selection, -1);
    // 화면 밖 선택 해제 비트는 안쪽 좌표여도 해제한다.
    s.input.broadcast(2, 0, 150, 80); assert.equal(s.menuNode.selection, 1); s.input.broadcast(2, OUTSIDE, 150, 80); assert.equal(s.menuNode.selection, -1);
});

test('broadcast(경계): 로컬 경계(0 포함, 폭/높이 미포함)·마지막 행 기본 선택·compact 는 0 번 행·음수 signed 좌표', () => {
    const s = inputScene(); s.input.broadcast(2, 0, 100, 50); assert.equal(s.menuNode.selection, 0);
    s.input.broadcast(2, 0, 179, 109); assert.equal(s.menuNode.selection, 2);
    // 폭 80/높이 60 끝(로컬 80/60)은 밖이다.
    s.input.broadcast(2, 0, 180, 80); assert.equal(s.menuNode.selection, -1); s.input.broadcast(2, 0, 150, 110); assert.equal(s.menuNode.selection, -1); s.input.broadcast(2, 0, 99, 80); assert.equal(s.menuNode.selection, -1); s.input.broadcast(2, 0, 150, 49); assert.equal(s.menuNode.selection, -1);
    // 행 사이 빈틈이면 마지막 행이 선택된다(높이 10 으로 줄여 offset 20.. 사이를 만든다).
    s.menuNode.heights.set([5, 5, 5]); s.input.broadcast(2, 0, 150, 108); assert.equal(s.menuNode.selection, 2);
    // compact 는 0 번 행만 고른다. 음수 좌표는 signed 로 해석한다(밖).
    const t = inputScene(); t.menuNode.menuFlags = VERTICAL | COMPACT | 1; t.input.broadcast(2, 0, 150, 100); assert.equal(t.menuNode.selection, 0); t.input.broadcast(2, 0, 0xffffff9c >>> 0, 80); assert.equal(t.menuNode.selection, -1);
    // 항목이 없는 메뉴도 0 번 행(마지막 인덱스 규칙)이며 0 항목 수는 첫 행을 고른다.
    const u = inputScene(); u.menuNode.itemCount = 1; u.input.broadcast(2, 0, 150, 100); assert.equal(u.menuNode.selection, 0);
});

test('broadcast(가로): 항목 폭으로 나눈 절삭 인덱스를 고르고 선택 변경마다 전체 Redraw 를 요청하며 세로 행 갱신은 없다', () => {
    const s = inputScene(); Object.assign(s.menuNode, { menuFlags: HORIZONTAL | 1, itemWidth: 25 }); s.input.broadcast(2, 0, 160, 80);
    // 로컬 x 60 / 25 = 2.4 → 2. 가로는 행 영역 갱신이 없고 전체 재그리기(erase=false)다.
    assert.equal(s.menuNode.selection, 2); assert.deepEqual(rows(s.log, 'invalidate'), []); assert.deepEqual(rows(s.log, 'vredraw'), [[2, false]]);
    s.log.length = 0; s.input.broadcast(2, 0, 110, 80); assert.equal(s.menuNode.selection, 0); assert.deepEqual(rows(s.log, 'vredraw'), [[2, false]]);
    // 항목 폭 0 은 0 으로 절삭된다(나눗셈 무한대/NaN).
    s.menuNode.itemWidth = 0; s.log.length = 0; s.menuNode.selection = -1; s.menuNode.items.forEach((item) => { item.flags = TEXT; }); s.input.broadcast(2, 0, 110, 80); assert.equal(s.menuNode.selection, 0);
    // 선택 변경이 없으면 Redraw 도 없다. Redraw 경계가 없으면 거부한다.
    s.log.length = 0; s.input.broadcast(2, 0, 110, 80); assert.deepEqual(s.log, []);
    const t = inputScene({ omit: ['redraw'] }); Object.assign(t.menuNode, { menuFlags: HORIZONTAL | 1, itemWidth: 25 }); assert.throws(() => t.input.broadcast(2, 0, 160, 80), /메뉴 입력 Redraw 경계 필요/);
});

test('broadcast(가림): 메뉴 식별인 자식이 그 점을 덮으면(표시 중 경계 안, 또는 하위 재귀) 선택하지 않는다', () => {
    const s = inputScene(); const kid = s.nodes.get(5); Object.assign(kid, { flags: 8 | VISIBLE, x: 40, y: 10, width: 40, height: 40 }); s.nodes.get(2).child = 5;
    s.input.broadcast(2, 0, 150, 80); assert.equal(s.menuNode.selection, -1);
    // 자식 밖의 점은 선택한다. 숨긴 자식은 자신의 경계로는 가리지 않는다.
    s.input.broadcast(2, 0, 110, 100); assert.equal(s.menuNode.selection, 2); kid.flags = 8; s.menuNode.selection = -1; s.input.broadcast(2, 0, 150, 80); assert.equal(s.menuNode.selection, 1);
    // 식별이 다른 자식은 하위 탐색도 하지 않는다. 숨긴 자식의 표시 중 손자는 가린다.
    kid.command = 3; kid.flags = 8 | VISIBLE; s.menuNode.selection = -1; s.input.broadcast(2, 0, 150, 80); assert.equal(s.menuNode.selection, 1); kid.command = 26; kid.flags = 8; const grand = s.nodes.get(6); Object.assign(grand, { flags: 8 | VISIBLE, parent: 5, x: 0, y: 0, width: 40, height: 40, command: 26 }); kid.child = 6; s.menuNode.selection = -1; s.input.broadcast(2, 0, 150, 80); assert.equal(s.menuNode.selection, -1);
    // 형제 순환은 거부한다.
    const t = inputScene(); t.nodes.get(2).child = 5; t.nodes.get(5).next = 6; t.nodes.get(6).next = 5; t.nodes.get(5).command = 3; t.nodes.get(6).command = 3; assert.throws(() => t.input.broadcast(2, 0, 150, 80), /메뉴 가림 형제 순환/);
});

test('broadcast: 사건의 화면 좌표는 signed DWORD 로 수집기에 전달하고 이벤트 bit0·DISABLED 아닌 명령 항목·정확한 경계를 올바르게 처리한다', () => {
    const s = inputScene(); const points = []; const real = s.collection; const spy = { screenToLocal: (token, point) => { points.push({ ...point }); return real.screenToLocal(token, point); }, position: (token) => real.position(token) };
    const input = new MenuGumpInput(s.create, s.inputState, s.menu, spy, s.events, s.lifetime, s.tree, s.inputHooks); input.broadcast(2, 0, 150, 80); input.broadcast(2, 0, 0xfffffff0, 0xfffffff6);
    assert.deepEqual(points, [{ x: 150, y: 80 }, { x: -16, y: -10 }]);
    // 이벤트 bit0 는 선택 해제 비트(0x4000000)가 아니다. 명령 비트(bit0)만 있는 항목은 비활성이 아니다.
    s.menuNode.selection = -1; s.menuNode.items[1].flags = TEXT | COMMAND; s.input.broadcast(2, 1, 150, 80); assert.equal(s.menuNode.selection, 1);
    // 행 경계: 로컬 y 가 정확히 행 끝(20, 40)이면 다음 행이다.
    s.input.broadcast(2, 0, 150, 70); assert.equal(s.menuNode.selection, 1); s.input.broadcast(2, 0, 150, 69); assert.equal(s.menuNode.selection, 0); s.input.broadcast(2, 0, 150, 90); assert.equal(s.menuNode.selection, 2);
    // 항목 수 2(짝수)에서 틈새는 마지막 인덱스 1 을 고른다.
    const t = inputScene(); t.menuNode.itemCount = 2; t.menuNode.heights.set([5, 5, 5]); t.input.broadcast(2, 0, 150, 108); assert.equal(t.menuNode.selection, 1);
    // 가로: 바깥에서 이미 선택이 없으면 Redraw 도 없다(초기 선택 -1 과 같다).
    const u = inputScene(); Object.assign(u.menuNode, { menuFlags: HORIZONTAL | 1, itemWidth: 25 }); u.input.broadcast(2, OUTSIDE, 150, 80); assert.deepEqual(rows(u.log, 'vredraw'), []);
});

test('broadcast(가로 절삭): 항목 폭이 0/아주 작아 몫이 무효·범위 밖·32비트를 넘어도 CRT 절삭의 하위 DWORD 로 고른다', () => {
    const s = inputScene(); Object.assign(s.menuNode, { menuFlags: HORIZONTAL | 1, itemWidth: 0 }); s.input.broadcast(2, 0, 100, 80); assert.equal(s.menuNode.selection, 0);
    const t = inputScene(); Object.assign(t.menuNode, { menuFlags: HORIZONTAL | 1, itemWidth: 1e-18 }); t.input.broadcast(2, 0, 110, 80); assert.equal(t.menuNode.selection, 0);
    // 4294967301 은 하위 32비트가 5 이고 33비트로 읽으면 음수가 된다(몫의 반올림에 따라 4 또는 5).
    const u = inputScene(); Object.assign(u.menuNode, { menuFlags: HORIZONTAL | 1, itemWidth: 5 / 4294967301 }); u.input.broadcast(2, 0, 105, 80); assert.ok([4, 5].includes(u.menuNode.selection), String(u.menuNode.selection));
});

test('redrawItem: 폭/높이가 정확히 1 이면 요청하고 0 이하이면 생략하며 handle 의 숨김은 자식 flags 전체 값을 보존한다', () => {
    const s = inputScene(); s.menuNode.width = 1; s.input.redrawItem(2, 1); assert.deepEqual(rows(s.log, 'invalidate'), [[{ left: 100, top: 70, right: 101, bottom: 90 }]]);
    s.log.length = 0; s.menuNode.width = 80; s.menuNode.heights[1] = 1; s.input.redrawItem(2, 1); assert.deepEqual(rows(s.log, 'invalidate'), [[{ left: 100, top: 70, right: 180, bottom: 71 }]]);
    // 동적 자식의 숨김 처리 뒤 열기에서 visible 만 다시 켜진다(다른 비트는 그대로).
    const t = inputScene(); const child = t.nodes.get(5); child.flags = 8 | VISIBLE | 0x100; t.menuNode.items[0].flags = TEXT | DYNAMIC; t.on.vbroadcast = () => select(t, 0); t.on.vhandle = () => 5; t.input.handle(2, LEFT, 1, 2); assert.equal(child.flags, 8 | VISIBLE | 0x100 | 0x80); // 0x80 은 확장 시작의 idle 등록 비트다.
    // 선택 해제 시 강조 비트만 지운다(다른 비트는 그대로).
    const u = inputScene(); u.menuNode.items[1].flags = TEXT | 0x100000 | SELECTED; u.menuNode.selection = 1; u.input.broadcast(2, OUTSIDE, 150, 80); assert.equal(u.menuNode.items[1].flags, TEXT | 0x100000);
});

test('closePopups: 표시 비트(bit0)가 켜진 팝업도 dead 가 아니면 Kill 한다', () => {
    const s = inputScene(); Object.assign(s.nodes.get(1), { child: 3 }); Object.assign(s.nodes.get(3), { flags: POPUP | VISIBLE | 8, next: 0 }); s.input.closePopups(); assert.deepEqual(rows(s.log, 'kill'), [[3]]);
});

test('bindMenuGumpInput/bindMenuGumpInputSound: 메뉴만 새 몸체로 보내고 다른 토큰은 이전 처리기를 보존하며 음향은 이름만 재생한다', () => {
    const s = inputScene(); const calls = []; const events = { handle: (...args) => { calls.push(['events', ...args]); return 4; } }; const input = { handle: (...args) => { calls.push(['input', ...args]); return 5; } }; const frame = { broadcast: (...args) => calls.push(['frame', ...args]) };
    bindMenuGumpInput(events, input, frame, (token) => (token === 2 ? s.input : null));
    assert.equal(events.handle(7, 1, 2, 3), 4); assert.equal(input.handle(7, 1, 2, 3), 5); frame.broadcast(7, 1, 2, 3); assert.deepEqual(calls, [['events', 7, 1, 2, 3], ['input', 7, 1, 2, 3], ['frame', 7, 1, 2, 3]]);
    assert.equal(events.handle(2, 0, 150, 80), 0); assert.equal(input.handle(2, 0, 150, 80), 0); frame.broadcast(2, 0, 150, 80); assert.equal(s.menuNode.selection, 1); assert.equal(calls.length, 3);
    assert.throws(() => bindMenuGumpInput({}, {}, {}, null), /메뉴 입력 분배 조회 필요/); const bare = { }; const inp = {}; const fr = {}; bindMenuGumpInput(bare, inp, fr, () => null);
    assert.throws(() => bare.handle(1, 0, 0, 0), /메뉴 외 입력 경계 필요/); assert.throws(() => inp.handle(1, 0, 0, 0), /메뉴 외 입력 경계 필요/); assert.throws(() => fr.broadcast(1, 0, 0, 0), /메뉴 외 통지 경계 필요/);
    const played = []; const hooks = {}; bindMenuGumpInputSound(hooks, { playByName: (...args) => played.push(args) }); hooks.sound('openGump.wav'); assert.deepEqual(played, [['openGump.wav', 0, 0, 0, 0, 0]]);
});
