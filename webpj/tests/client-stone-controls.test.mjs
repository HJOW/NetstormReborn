// 실제 Client Gump 수명에서 돌 버튼과 배경·입력을 함께 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { ClientStoneControls } from '../www/ui/client-stone-controls.js';
import { ButtonGumpState } from '../www/ui/button-gump.js';
import { StoneButtonGumpState } from '../www/ui/stone-button-gump.js';

// 같은 등록표에 실제 돌 버튼·배경을 생성하고 입력·방송·idle·삭제한다.
test('Client 돌 버튼: 실제 배경·입력·공통 수명', () => {
    const dirty = [], sounds = [], owner = new ClientGumpRoot(100, 80, () => 0, (token, erase) => dirty.push([token, erase]));
    const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState();
    Object.assign(buttonState, { valueEvent: 0x02000070, clickEvent: 0x0200002c, rightEvent: 0x02000012, repeatEvent: 0x0200002f, baseColor: 56 });
    stoneState.color = 49; stoneState.palette = { black: 0, white: 1, edge: 2 };
    const controls = new ClientStoneControls(owner, { buttonState, stoneState, backgroundColor: () => 35,
        now: () => 10, poll: () => ({ code: 0 }), sound: () => sounds.push('sound'), baseEvent: () => 0 });
    const token = controls.allocate(); controls.construct(token, 20, 10); owner.attach(token, 10, 20);
    const node = owner.node(token), background = owner.node(node.background); assert.ok(background); assert.equal(owner.createState().count, 3);
    assert.equal(owner.input().route(0x10000, 12, 22), token); assert.equal(node.pressed, 1); assert.deepEqual(sounds, ['sound']);
    owner.broadcast(0, 12, 22); owner.idle(); assert.equal(node.pressed, 1);
    assert.equal(owner.input().route(0x80010000, 12, 22), token); assert.equal(node.pressed, 0);
    assert.ok(dirty.length > 0); owner.remove(token); assert.equal(owner.node(token), null); assert.equal(owner.node(node.background), null);
    assert.equal(owner.createState().count, 1); owner.dispose();
});

// 기본 생성 전 할당과 배경 자산 실패 뒤 미부착 노드도 같은 소유자가 정리한다.
test('Client 돌 버튼: 부분 생성과 미등록 할당 정리', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState();
    const controls = new ClientStoneControls(owner, { buttonState, stoneState, backgroundColor: () => 35, baseEvent: () => 0 });
    const bare = controls.allocate(); controls.close(); assert.equal(owner.node(bare), null); assert.equal(owner.createState().count, 1);
    stoneState.backgroundType = 123; const token = controls.allocate();
    assert.throws(() => controls.construct(token, 20, 10), /타입\/SHP/u); assert.equal(owner.createState().count, 3);
    controls.close(); assert.equal(owner.node(token), null); assert.equal(owner.createState().count, 1); owner.dispose();
});
