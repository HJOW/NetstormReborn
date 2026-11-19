// 실제 공통 Client에서 편집기·버퍼·두 Slider를 함께 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { ClientTextEntryControls } from '../www/ui/client-text-entry-controls.js';
import { TextEntryGumpState } from '../www/ui/text-entry-gump.js';
import { ButtonGumpState } from '../www/ui/button-gump.js';
import { StoneButtonGumpState } from '../www/ui/stone-button-gump.js';
import { ScrollGumpState } from '../www/ui/scroll-gump.js';
import { ScrollGumpWidgetState } from '../www/ui/scroll-gump-widgets.js';
import { makeCommittedTextEvents } from '../www/core/display-text-input.js';
import { displayTextCodec } from '../www/assets/display-text.js';

// 실제 처리기를 같은 소유자에 두고 검사에 필요한 장치를 공급한다.
function fixture() {
    let time = 10; const owner = new ClientGumpRoot(300, 200, () => 0, () => {});
    const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState(), scrollState = new ScrollGumpState();
    Object.assign(buttonState, { valueEvent: 0x02000070, clickEvent: 0x0200002c, rightEvent: 0x02000012, repeatEvent: 0x0200002f, baseColor: 56 });
    Object.assign(scrollState, { clickEvent: 0x0200002c, repeatEvent: 0x0200002f, stepEvent: 0x0200002e, changeEvent: 0x02000036 });
    stoneState.color = 49; stoneState.palette = { black: 0, white: 1, edge: 2 };
    const controls = new ClientTextEntryControls(owner, { entryState: new TextEntryGumpState(), buttonState, stoneState, scrollState, widgetState: new ScrollGumpWidgetState(),
        backgroundColor: () => 35, now: () => time, measure: () => [8, 16], poll: () => ({ code: 0 }), sound: () => {}, baseEvent: () => 0 });
    return { owner, controls, stoneState, tick: (value) => { time = value; } };
}

// 실제 공통 키 분배와 확정 한글 입력, 점멸 및 자식 우선 삭제를 검증한다.
test('Client TextEntry: 실제 버퍼·두 Slider·입력과 수명', () => {
    const { owner, controls, tick } = fixture(), token = controls.allocate();
    controls.construct(token, 150, 60, 'hello', 1, 0, 3); owner.attach(token, 10, 10);
    const node = owner.node(token), buffer = controls.storage().find(node.buffer), sliders = [node.verticalSlider, node.horizontalSlider];
    assert.ok(sliders.every((slider) => owner.node(slider))); assert.equal(owner.createState().count, 14);
    assert.equal(buffer.text(), 'hello'); owner.input().capture(token);
    assert.equal(owner.input().route(0x41, 0, 0), token); assert.equal(buffer.text(), 'A');
    owner.input().capture(token); assert.equal(owner.input().route(makeCommittedTextEvents('한')[0].code, 0, 0), token);
    assert.equal(displayTextCodec.decode(Uint8Array.from(buffer.text(), (character) => character.charCodeAt(0))), 'A한');
    tick(11); owner.idle(); assert.equal(node.blinkPhase, 1);
    owner.remove(token); assert.equal(owner.node(token), null); assert.ok(sliders.every((slider) => owner.node(slider) === null));
    controls.close(); assert.equal(controls.storage().bufferCount(), 0); assert.equal(controls.storage().allocationCount(), 0);
    assert.equal(owner.createState().count, 1); owner.dispose();
});

// Console도 같은 버퍼와 자식을 쓰고 부분 생성 실패 뒤 전체 수명을 정리한다.
test('Client TextEntry: Console과 부분 생성 실패 정리', () => {
    const { owner, controls, stoneState } = fixture(), bare = controls.allocate(true);
    controls.close(); assert.equal(owner.node(bare), null);
    const token = controls.allocate(true); controls.construct(token, 150, 60, null, 1, 0, 3); owner.attach(token, 5, 5);
    assert.equal(owner.node(token).command, 10); controls.close(); assert.equal(controls.storage().bufferCount(), 0); assert.equal(owner.createState().count, 1);
    stoneState.backgroundType = 123; const failed = controls.allocate();
    assert.throws(() => controls.construct(failed, 150, 60, null, 1, 0, 3), /타입\/SHP/u);
    controls.close(); assert.equal(controls.storage().bufferCount(), 0); assert.equal(controls.storage().allocationCount(), 0); assert.equal(owner.createState().count, 1); owner.dispose();
});
