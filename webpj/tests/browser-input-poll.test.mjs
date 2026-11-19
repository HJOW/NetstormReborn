// 브라우저 포인터와 보조 키를 원본 Client 폴링 형식으로 읽는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { BrowserInputPoll } from '../www/platform/browser-input-poll.js';
import { InputCode } from '../www/core/input-queue.js';
import { Netstorm } from '../www/netstorm.js';

// 실제 브라우저 버튼 순서와 키별 누름 기록의 독립 소비를 확인한다.
test('브라우저 입력 폴링: 버튼·보조 키·눌림 기록과 좌표', () => {
    const input = { pointer: { x: 12, y: 20, inside: true, buttons: 7 }, keys: new Set(['AltLeft', 'ControlRight']) };
    const layout = { logicalWidth: 100, logicalHeight: 80 }; let active = true;
    const poll = new BrowserInputPoll(() => input, () => layout, () => active);
    poll.recordMouseDown(0); poll.recordMouseDown(1); poll.recordMouseDown(2); poll.recordKeyDown('AltLeft', false); poll.recordKeyDown('ControlRight', false);
    assert.deepEqual(poll.poll(InputCode.leftButton), { code: InputCode.leftButton | InputCode.pressedSince, x: 12, y: 20 });
    assert.equal(poll.poll(InputCode.leftButton).code, InputCode.leftButton);
    assert.equal(poll.poll(InputCode.rightButton).code, InputCode.rightButton | InputCode.pressedSince);
    assert.equal(poll.poll(InputCode.middleButton).code, InputCode.middleButton | InputCode.pressedSince);
    assert.equal(poll.poll(0x00120000).code, 0x40120000); assert.equal(poll.poll(0x00110000).code, 0x40110000);
    poll.recordKeyDown('AltLeft', true); assert.equal(poll.poll(0x00120000).code, 0x00120000);
    input.pointer.buttons = 0; input.keys.clear(); input.pointer.inside = false; input.pointer.x = -2; input.pointer.y = 100;
    assert.deepEqual(poll.poll(InputCode.leftButton), { code: 0x84010000, x: 0, y: 80 });
    input.pointer.inside = true; active = false; assert.equal(poll.poll(0x00110000).code, 0x84110000);
    assert.throws(() => poll.poll(0x005a0000), /연결하지 않은/u); assert.throws(() => poll.poll(-1), /코드/u);
});

// 실제 앱의 입력 소유자는 재시작 시 새 폴링 기록을 갖고 현재 상태를 읽는다.
test('브라우저 입력 폴링: 앱의 현재 입력과 키 사건을 사용한다', () => {
    const app = new Netstorm(); app._layout = { logicalWidth: 100, logicalHeight: 80 }; app._retryAudioOnGesture = () => {};
    app._input.pointer = { x: 14, y: 17, inside: true, buttons: 1, type: 'mouse' };
    app._onKeyDown({ code: 'AltLeft', repeat: false, preventDefault() {} });
    assert.deepEqual(app._clientInputPoll.poll(0x00120000), { code: 0x40120000, x: 14, y: 17 });
    assert.equal(app._clientInputPoll.poll(InputCode.leftButton).code, InputCode.leftButton);
    app._onKeyDown({ code: 'AltLeft', repeat: true, preventDefault() {} }); assert.equal(app._clientInputPoll.poll(0x00120000).code, 0x00120000);
});
