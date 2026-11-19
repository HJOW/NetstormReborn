// 실제 Gump 생성·수명·입력 부모를 앱의 같은 Client 전역에 연결한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { GumpBackgroundCreate, GumpBackgroundNode } from '../www/ui/gump-background-create.js';
import { UberRootNode } from '../www/ui/uber-root-gump.js';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from '../www/render/gump-draw.js';

// 등록 루트와 현재 입력 전역은 크기 변경에도 같은 객체이며 원본 삭제 큐로 반납한다.
test('Client Gump 루트: 실제 등록표·크기·입력·삭제 수명', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false); app._layout = { logicalWidth: 1024, logicalHeight: 768 };
    app._initializeClientGump(); const owner = app._clientGump, root = owner.root(), state = owner.createState(), life = owner.lifetimeState();
    assert.equal(state.entries[0], 1); assert.equal(state.collection.root, 1); assert.equal(state.count, 1); assert.equal(root.serial, 1);
    assert.deepEqual([root.width, root.height, root.baseWidth, root.baseHeight], [1024, 768, 1024, 768]);
    assert.equal(owner.input().route(0x02000003, 2, 0, true), 0);
    owner.resize(1366, 768); assert.equal(owner.root(), root); assert.equal(state.entries[0], 1);
    assert.deepEqual([state.screenWidth, state.screenHeight, root.width, root.height], [1366, 768, 1366, 768]);
    assert.throws(() => app._initializeClientGump(), /중복/u); assert.throws(() => owner.resize(0, 768), /크기/u);
    owner.dispose(); assert.equal(state.entries[0], 0); assert.equal(state.count, 0); assert.equal(life.killCount, 0);
    assert.throws(() => owner.root(), /반납/u); assert.doesNotThrow(() => owner.dispose());
});

// 진단 getter와 정상 화면 크기는 Client를 만들 때 반드시 제공해야 한다.
test('Client Gump 루트: 잘못된 Client 수명을 거부한다', () => {
    const app = new Netstorm(); assert.throws(() => app._initializeClientGump(), /수명 누락/u);
    assert.throws(() => new ClientGumpRoot(10, 10, null), /진단 전역 누락/u);
    assert.throws(() => new ClientGumpRoot(0, 10, () => 0), /크기/u);
});

// 실제 파생 노드의 가상 출력은 공통 클립과 픽셀 장치를 공유하며 반납 뒤 제거된다.
test('Client Gump 루트: 파생 출력과 클립·픽셀·삭제 수명', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), node = new GumpBackgroundNode();
    const token = owner.allocateDerived(node, () => 0, (value, flags) => owner.lifetime().deleteScalar(value, flags));
    owner.factory().construct(token, 0, 20, 10, 0x400001); owner.attach(token, 10, 20);
    const state = new GumpDrawState(); setGumpDrawClip(state, { left: 0, top: 0, right: 50, bottom: 50 });
    const pixels = new Uint8Array(100 * 80), target = new GumpPixelTarget(pixels, 100, 80, state);
    assert.throws(() => owner.draw(token, state, target), /출력기 미연결/);
    owner.bindDraw(token, (_current, actual) => actual.fill({ left: 0, top: 0, right: 100, bottom: 80 }, 77));
    owner.draw(token, state, target); assert.equal(pixels[20 * 100 + 10], 77); assert.equal(pixels[29 * 100 + 29], 77);
    assert.equal(pixels[19 * 100 + 10], 0); assert.equal(pixels[20 * 100 + 30], 0);
    assert.deepEqual(state.clip, { left: 0, top: 0, right: 50, bottom: 50 });
    owner.bindDraw(token, () => { throw new Error('출력 실패'); }); assert.throws(() => owner.draw(token, state, target), /출력 실패/);
    assert.deepEqual(state.clip, { left: 0, top: 0, right: 50, bottom: 50 });
    owner.bindDraw(token, null); assert.throws(() => owner.draw(token, state, target), /출력기 미연결/);
    owner.remove(token); assert.throws(() => owner.bindDraw(token, () => {}), /토큰/); owner.dispose();
});

// 같은 실제 트리에 부착한 기본 노드가 입력을 받고 지연 삭제 뒤에는 받지 않는다.
test('Client Gump 루트: 자식 부착·입력·지연 삭제가 같은 등록표를 쓴다', () => {
    const redraw = [], events = [], owner = new ClientGumpRoot(100, 80, () => 0, (token, erase) => redraw.push([token, erase]));
    const token = owner.createBase(1, 20, 10, 0x401, (event, first, second) => { events.push([event, first, second]); return 1; });
    owner.attach(token, 10, 20); assert.equal(owner.root().child, token); assert.equal(owner.createState().count, 2);
    assert.equal(owner.input().route(42, 3, 4), token); assert.deepEqual(events, [[42, 3, 4]]);
    assert.ok(redraw.length > 0); owner.remove(token);
    assert.equal(owner.root().child, 0); assert.equal(owner.createState().count, 1); assert.equal(owner.lifetimeState().killCount, 0);
    assert.equal(owner.input().route(42, 3, 4), 0); assert.throws(() => owner.remove(token), /토큰/u);
    owner.dispose();
});

// 카메라 적용의 루트 재그리기가 실제 트리의 표시·자식 전파 관문을 사용한다.
test('Client Gump 루트: 현재 표시 자식과 하위 재그리기', () => {
    const redraw = [], owner = new ClientGumpRoot(100, 80, () => 0, (token, erase) => redraw.push([token, erase]));
    const visible = owner.createBase(1, 20, 20, 0x421, () => 0), hidden = owner.createBase(2, 20, 20, 0x401, () => 0);
    const child = owner.createBase(3, 10, 10, 0x401, () => 0);
    owner.attach(visible, 0, 0); owner.attach(hidden, 30, 0); owner.attach(child, 0, 0, visible);
    owner.node(hidden).flags &= ~1; redraw.length = 0;
    owner.redrawRoots(); assert.deepEqual(redraw, [[visible, false], [child, false]]);
    owner.dispose(); assert.throws(() => owner.redrawRoots(), /반납한 Client Gump/);
});

// 원본 차단 조회는 직계 0x4000만 검사하고 닫기는 비삭제 직계만 Kill한다.
test('Client Gump 루트: 직계 팝업 차단과 지연 닫기', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {});
    const parent = owner.createBase(1, 20, 20, 1, () => 0), nested = owner.createBase(2, 5, 5, 1, () => 0);
    const first = owner.createBase(3, 10, 10, 1, () => 0), second = owner.createBase(4, 10, 10, 1, () => 0);
    owner.attach(parent, 0, 0); owner.attach(nested, 0, 0, parent); owner.node(nested).flags |= 0x4000;
    assert.equal(owner.blockingPopups(), false);
    owner.attach(first, 30, 0); owner.attach(second, 50, 0); owner.node(first).flags |= 0x4000; owner.node(second).flags = (owner.node(second).flags | 0x4000) & ~1;
    assert.equal(owner.blockingPopups(), true); owner.closePopups();
    assert.equal(owner.node(first).flags & 0x10000, 0x10000); assert.equal(owner.node(second).flags & 0x10000, 0x10000);
    assert.equal(owner.node(nested).flags & 0x10000, 0); assert.equal(owner.lifetimeState().killCount, 2); assert.equal(owner.blockingPopups(), false);
    owner.closePopups(); assert.equal(owner.lifetimeState().killCount, 2); owner.dispose();
    assert.throws(() => owner.blockingPopups(), /반납/);
});

// 실제 Client 진단이 켜져 있으면 부착의 상세 문자열을 같은 전역에 남기고 계속한다.
test('Client Gump 루트: 부착 진단이 현재 Client 전역에 기록된다', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false); app._layout = { logicalWidth: 100, logicalHeight: 80 }; app._initializeClientGump();
    app._clientGameplay.aiDebug().checking().value = 1;
    const owner = app._clientGump, token = owner.createBase(0, 10, 10, 1, () => 0);
    owner.attach(token, 0, 0); assert.equal(app._clientGameplay.aiDebug().assertionDetail().value, 'gt = 0');
    owner.dispose();
});

// 배경 파생 생성기와 입력·소멸자가 같은 실제 확장 노드를 사용한다.
test('Client Gump 루트: 실제 파생 배경 노드의 생성·입력·소멸', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), node = new GumpBackgroundNode(), events = [];
    const token = owner.allocateDerived(node, (event) => { events.push(event); return 1; }, (current, flags) => { events.push('destroy'); return owner.lifetime().deleteScalar(current, flags); });
    assert.throws(() => owner.allocateDerived(node, () => 0, () => {}), /중복/u);
    const background = new GumpBackgroundCreate(owner.createState(), owner.factory(), () => 55, { node: (current) => owner.node(current) });
    background.construct(token, 20, 10, 0, 0x401, 0, 0); assert.equal(owner.node(token), node);
    assert.equal(node.command, 55); owner.attach(token, 10, 20);
    assert.equal(owner.input().route(42, 0, 0), token); owner.remove(token);
    assert.deepEqual(events, [42, 'destroy']); assert.equal(owner.node(token), null); assert.equal(owner.createState().count, 1);
    assert.throws(() => owner.allocateDerived(new GumpCreateNode(), () => 0, () => {}), /파생 노드/u);
    assert.throws(() => owner.allocateDerived(new GumpBackgroundNode(), () => 0, null), /소멸자/u);
    owner.dispose();
});

// 원본 임시 등록 해제 뒤 파생 루트를 공개하고 그 실제 처리기에 입력한다.
test('Client Gump 루트: 실제 파생 루트 교체와 입력 분배', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), previous = owner.root(), root = new UberRootNode(), events = [];
    const token = owner.allocateDerived(root, (event) => { events.push(event); return 1; }, (value, flags) => owner.lifetime().deleteScalar(value, flags));
    owner.factory().construct(token, 1, 100, 80, 0x401);
    const state = owner.createState(); state.entries[root.id] = 0; root.id = 0;
    owner.replaceRoot(token);
    assert.equal(owner.root(), root); assert.notEqual(owner.root(), previous); assert.equal(state.entries[0], token); assert.equal(state.collection.root, token);
    assert.equal(owner.input().route(42, 0, 0), token); assert.deepEqual(events, [42]);
    owner.resize(120, 80); assert.equal(root.width, 120); owner.dispose(); assert.equal(state.entries[0], 0);
});

// 직접 부착과 공통 Move가 같은 실제 파생 부착 본체를 호출하고 기본 본체로 위임한다.
test('Client Gump 루트: 파생 가상 부착과 공통 Move 분배', () => {
    const redraw = [], calls = [], owner = new ClientGumpRoot(100, 80, () => 0, (token, erase) => redraw.push([token, erase]));
    const node = new GumpBackgroundNode();
    const token = owner.allocateDerived(node, () => 0, (value, flags) => owner.lifetime().deleteScalar(value, flags),
        (value, x, y, parent, mode, bits) => { calls.push([value, x, y, parent, mode]); owner.attachBase(value, x, y, parent, mode, bits); });
    const background = new GumpBackgroundCreate(owner.createState(), owner.factory(), () => 55, { node: (value) => owner.node(value) });
    background.construct(token, 20, 10, 0, 1, 0, 0); owner.attach(token, 10, 20);
    assert.deepEqual(calls[0], [token, 10, 20, owner.createState().entries[0], 0]);
    assert.equal(owner.root().child, token); assert.equal(node.x, 10); assert.equal(node.y, 20);
    owner.move(token, 30, 40); assert.deepEqual(calls[1], [token, 30, 40, 0, 0]);
    assert.equal(node.x, 30); assert.equal(node.y, 40); assert.ok(redraw.length > 0);
    assert.throws(() => owner.allocateDerived(new GumpBackgroundNode(), () => 0, () => {}, 3), /부착 계약/u);
    owner.remove(token); assert.equal(owner.node(token), null); owner.dispose();
});

// 공통 프레임은 실제 등록표의 visible·notify·dead에 따라 파생 방송을 분배한다.
test('Client Gump 루트: 실제 프레임 방송의 등록·표시·삭제 필터', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), events = [], node = new GumpBackgroundNode();
    const token = owner.allocateDerived(node, () => 0, (value, flags) => owner.lifetime().deleteScalar(value, flags), null,
        (value, event, first, second) => events.push([value, event, first, second]));
    owner.factory().construct(token, 0, 10, 10, 0x101); owner.attach(token, 0, 0);
    owner.broadcast(3, 4, 5); assert.deepEqual(events, [[token, 3, 4, 5]]);
    node.flags &= ~1; owner.broadcast(3, 4, 5); node.flags |= 1; node.flags |= 0x10000; owner.broadcast(3, 4, 5);
    node.flags &= ~0x10000; node.flags &= ~0x100; owner.broadcast(3, 4, 5); assert.equal(events.length, 1);
    assert.throws(() => owner.allocateDerived(new GumpBackgroundNode(), () => 0, () => {}, null, 3), /방송 계약/u);
    owner.remove(token); owner.broadcast(3, 4, 5); assert.equal(events.length, 1); owner.dispose();
    assert.throws(() => owner.broadcast(0, 0, 0), /반납/u);
});

// 원본 포인터 관문이 같은 입력 overlay와 공통 방송을 순서대로 호출한다.
test('Client Gump 루트: 포인터 overlay·방송의 대화창·포커스·창 안팎 관문', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), events = [], node = new GumpBackgroundNode();
    const token = owner.allocateDerived(node, (event, first, second) => { events.push(['overlay', event, first, second]); return 1; },
        (value, flags) => owner.lifetime().deleteScalar(value, flags), null,
        (value, event, first, second) => events.push(['broadcast', value, event, first, second]));
    owner.factory().construct(token, 0, 20, 20, 0x101); owner.attach(token, 10, 10); owner.lifetime().setOverlay(token, true);
    const point = { code: 42, x: 15, y: 15 };
    assert.deepEqual(owner.notifyPointer(point, 0, false, true), { overlay: 1 });
    assert.deepEqual(events.splice(0), [['overlay', 0x0200003b, 15, 15], ['broadcast', token, 42, 15, 15]]);
    // 창 밖은 overlay와 방송을 모두 건너뛴다.
    assert.deepEqual(owner.notifyPointer({ ...point, code: 0x0400002a }, 0, false, true), { overlay: null }); assert.deepEqual(events, []);
    // 일반 상태의 각각의 차단 조건은 overlay만 생략하고 방송은 보존한다.
    for (const [mode, cursor, allowed] of [[1, false, true], [0, true, true], [0, false, false]]) {
        assert.deepEqual(owner.notifyPointer(point, mode, cursor, allowed), { overlay: null });
        assert.deepEqual(events.splice(0), [['broadcast', token, 42, 15, 15]]);
    }
    owner.input().capture(token); assert.deepEqual(owner.notifyPointer(point, 0, false, true), { overlay: null }); events.length = 0;
    // 대화창은 focus·커서·모드·허용 여부보다 우선한다.
    owner.input().addDialog(token); assert.deepEqual(owner.notifyPointer(point, 1, true, false), { overlay: 1 });
    assert.deepEqual(events.splice(0), [['overlay', 0x0200003b, 15, 15], ['broadcast', token, 42, 15, 15]]);
    assert.deepEqual(owner.notifyPointer({ code: 42, x: 99, y: 79 }, 0, false, true), { overlay: 0 });
    assert.throws(() => owner.notifyPointer({ code: 1, x: 0 }, 0, false, true), /사건 계약/u);
    owner.dispose(); assert.throws(() => owner.notifyPointer(point, 0, false, true), /반납/u);
});

// 공통 idle 표는 같은 실제 파생 노드를 현재 슬롯 순서로 호출하고 삭제 시 해제한다.
test('Client Gump 루트: 파생 idle 분배와 현재 표 재조회', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), calls = [], nodes = [new GumpBackgroundNode(), new GumpBackgroundNode()];
    let second = 0, firstRun = true;
    const first = owner.allocateDerived(nodes[0], () => 0, (value, flags) => owner.lifetime().deleteScalar(value, flags), null, null,
        (value) => { calls.push(value); if (firstRun) { firstRun = false; owner.lifetime().setIdle(second, false); } });
    second = owner.allocateDerived(nodes[1], () => 0, (value, flags) => owner.lifetime().deleteScalar(value, flags), null, null, (value) => calls.push(value));
    // 실제 공통 등록·트리·idle 수명을 사용한다.
    for (const token of [first, second]) { owner.factory().construct(token, 0, 10, 10, 1); owner.attach(token, 0, 0); owner.lifetime().setIdle(token, true); }
    owner.idle(); assert.deepEqual(calls, [first]);
    owner.lifetime().setIdle(second, true); owner.remove(first); owner.idle(); assert.deepEqual(calls, [first, second]);
    assert.throws(() => owner.allocateDerived(new GumpBackgroundNode(), () => 0, () => {}, null, null, 3), /idle 계약/u);
    owner.dispose(); assert.throws(() => owner.idle(), /반납/u);
});

// 직접 가상 호출은 입력 필터나 자식 분배 없이 실제 반환 DWORD와 현재 수명을 보존한다.
test('Client Gump 루트: 커서의 직접 가상 명령', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), calls = [];
    const token = owner.createBase(0, 10, 10, 1, (...args) => { calls.push(args); return 0xfedcba98; });
    owner.attach(token, 0, 0);
    assert.equal(owner.handle(token, 0x0200005f, 3, 4), 0xfedcba98);
    assert.deepEqual(calls, [[0x0200005f, 3, 4]]); assert.equal(owner.handle(1, 0, 0, 0), 0);
    owner.remove(token); assert.throws(() => owner.handle(token, 0, 0, 0), /토큰 누락/u); owner.dispose();
    assert.throws(() => owner.handle(1, 0, 0, 0), /반납/u);
});
