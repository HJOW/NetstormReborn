// 실제 Client 공통 등록표에서 Slider 자식과 입력 및 수명을 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { ClientScrollControls } from '../www/ui/client-scroll-controls.js';
import { ButtonGumpState } from '../www/ui/button-gump.js';
import { StoneButtonGumpState } from '../www/ui/stone-button-gump.js';
import { ScrollGumpState } from '../www/ui/scroll-gump.js';
import { ScrollGumpWidgetState } from '../www/ui/scroll-gump-widgets.js';

// 원본 이벤트 번호와 같은 공통 소유자에 필요한 장치를 제공한다.
function fixture() {
    const owner = new ClientGumpRoot(200, 200, () => 0, () => {});
    const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState(), scrollState = new ScrollGumpState();
    Object.assign(buttonState, { valueEvent: 0x02000070, clickEvent: 0x0200002c, rightEvent: 0x02000012, repeatEvent: 0x0200002f, baseColor: 56 });
    Object.assign(scrollState, { clickEvent: 0x0200002c, repeatEvent: 0x0200002f, stepEvent: 0x0200002e, changeEvent: 0x02000036 });
    stoneState.color = 49; stoneState.palette = { black: 0, white: 1, edge: 2 };
    const controls = new ClientScrollControls(owner, { buttonState, stoneState, scrollState, widgetState: new ScrollGumpWidgetState(),
        backgroundColor: () => 35, now: () => 10, poll: () => ({ code: 0 }), sound: () => {}, baseEvent: () => 0 });
    return { owner, controls, stoneState };
}

// 실제 화살표 클릭을 같은 트리에서 Slider 값 변경까지 전달하고 자식을 반납한다.
test('Client Slider: 화살표·손잡이·입력과 공통 수명', () => {
    const { owner, controls } = fixture(), token = controls.allocate();
    controls.construct(token, 10, 100, 0, 0); owner.attach(token, 10, 10);
    const node = owner.node(token), children = [node.decrement, node.increment, node.thumb];
    assert.equal(owner.createState().count, 7); assert.equal(node.position, 50);
    assert.ok(children.every((child) => owner.node(child)));
    controls.backend().setPosition(token, 40); assert.equal(node.position, 40);
    const rect = owner.collection().bounds(node.decrement), x = rect.left + 2, y = rect.top + 2;
    assert.equal(owner.input().route(0x10000, x, y), node.decrement);
    owner.input().route(0x80010000, x, y); assert.equal(node.position, 39);
    owner.remove(token); assert.equal(owner.node(token), null);
    assert.ok(children.every((child) => owner.node(child) === null));
    controls.close(); assert.equal(owner.createState().count, 1); owner.dispose();
});

// 미등록 부모와 배경 자산 실패 중 생성한 노드도 원래 소유자가 정리한다.
test('Client Slider: 부분 생성 실패 정리', () => {
    const { owner, controls, stoneState } = fixture(), bare = controls.allocate();
    controls.close(); assert.equal(owner.node(bare), null); assert.equal(owner.createState().count, 1);
    stoneState.backgroundType = 123; const token = controls.allocate();
    assert.throws(() => controls.construct(token, 10, 100, 0, 0), /타입\/SHP/u);
    controls.close(); assert.equal(owner.node(token), null); assert.equal(owner.createState().count, 1); owner.dispose();
});
