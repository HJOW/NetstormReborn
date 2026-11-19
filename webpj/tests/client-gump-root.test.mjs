// 실제 Gump 생성·수명·입력 부모를 앱의 같은 Client 전역에 연결한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { GumpBackgroundCreate, GumpBackgroundNode } from '../www/ui/gump-background-create.js';

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
