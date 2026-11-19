// 실제 Gump 생성·수명·입력 부모를 앱의 같은 Client 전역에 연결한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';

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
