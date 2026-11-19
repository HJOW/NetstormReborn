// 실제 Client 트리에서 HUD 구분선의 생성·입력·이동·수명을 함께 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { ClientHudDivider } from '../www/ui/client-hud-divider.js';
import { HudDividerNode } from '../www/ui/hud-dock.js';

// 구분선 이동이 실제 serial 검색과 공통 트리를 통해 같은 자식 크기를 갱신한다.
test('Client HUD 구분선: 실제 생성·부착·입력·연결 자식 이동', () => {
    const redraw = [], calls = [], owner = new ClientGumpRoot(100, 80, () => 0, (token, erase) => redraw.push([token, erase]));
    const controls = new ClientHudDivider(owner, { backgroundColor: () => 22, dividerColor: () => 55,
        closePopups: () => calls.push('close'), baseHandle: () => 0, menuInstances: () => 0, poll: () => ({ code: 0 }) });
    const slave = owner.createBase(1, 10, 10, 1, () => 0); owner.attach(slave, 0, 0);
    const token = controls.allocate(), node = owner.node(token); assert.ok(node instanceof HudDividerNode);
    controls.construct(token, 1, 5, 80, 0, 0, 0); assert.equal(node.command, 55); assert.ok(owner.lifetimeState().overlay.includes(token));
    controls.registerSlave(token, slave, 1); owner.attach(token, 20, 0);
    assert.equal(node.x, 20); assert.equal(owner.node(slave).baseWidth, 20); assert.deepEqual(calls, ['close']);
    assert.equal(owner.input().route(0x10000, 21, 20), token); assert.equal(owner.lifetimeState().focus, token);
    controls.broadcast(token, 0, 42, 20); assert.equal(node.x, 39); assert.equal(owner.node(slave).baseWidth, 39);
    assert.deepEqual(calls, ['close', 'close']); assert.ok(redraw.length > 0);
    assert.equal(owner.input().route(0x80010000, 42, 20), token); assert.equal(owner.lifetimeState().focus, 0);
    const state = owner.createState(); owner.dispose(); assert.equal(state.count, 0);
});

// 도달한 실제 외부 타입과 popup 장치가 없으면 원본 분기를 성공 처리하지 않는다.
test('Client HUD 구분선: 누락된 실제 하위 장치를 감춘 성공으로 처리하지 않는다', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {});
    assert.throws(() => new ClientHudDivider(owner, {}), /색 전역/u);
    const controls = new ClientHudDivider(owner, { backgroundColor: () => 1, dividerColor: () => 2 });
    const token = controls.allocate(); controls.construct(token, 1, 5, 80, 0, 0, 0);
    assert.throws(() => owner.attach(token, 10, 0), /popup/u); owner.remove(token); owner.dispose();
});

// 0이 아닌 실제 타입과 프레임 경계는 생성 순서와 shape를 그대로 사용한다.
test('Client HUD 구분선: 배경 타입과 프레임 장치로 실제 shape를 생성한다', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), calls = [];
    const controls = new ClientHudDivider(owner, { backgroundColor: () => 1, dividerColor: () => 2,
        type: (type) => { calls.push(['type', type]); return { token: 9, shape: 11 }; },
        frame: (asset, group, frame) => { calls.push(['frame', asset, group, frame]); return { width: 7, height: 8 }; } });
    const token = controls.allocate(); controls.construct(token, 1, 5, 80, 0, 12, 3);
    assert.deepEqual(calls, [['type', 12], ['frame', 9, 0, 3]]);
    assert.deepEqual([owner.node(token).shape, owner.node(token).tileWidth, owner.node(token).tileHeight], [11, 7, 8]);
    owner.remove(token); owner.dispose();
});
