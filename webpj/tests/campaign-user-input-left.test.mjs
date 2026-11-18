import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignUserInput } from '../www/core/campaign-user-input.js';
import { InputCode } from '../www/core/input-queue.js';

// 이동 모드의 후보 조회는 Shift 여부와 관계없이 먼저 일어나며 Ctrl 권한은 선택에 전달된다.
test('이동 모드 좌클릭은 후보 조회 뒤 Shift 선택 또는 이동을 분배한다', () => {
    const calls = [], cursor = { snapToGrid: {}, placement: {}, handleInput: () => false };
    let mode = 2, special = 1;
    const world = {
        session: () => ({ cursor: () => cursor }), cursorRotationMode: () => 0,
        inputMode: () => mode, specialSelection: () => special,
        pick: (...args) => { calls.push(['pick', ...args]); return 77; },
        extendSelection: (...args) => calls.push(['extend', ...args]),
        moveSelected: (x, y, destination) => { calls.push(['move', x, y]); destination(-2, 3); },
    };
    const input = new CampaignUserInput(world, { readInt: () => ({ value: 0 }) }, {
        paused: () => false, queue: { pushCommand: (...args) => calls.push(['queue', ...args]) },
    });
    input.event({ code: (InputCode.leftButton | InputCode.control | InputCode.shift) >>> 0, x: 12, y: 34 });
    assert.deepEqual(calls.splice(0), [['pick', 12, 34, 1], ['extend', 77, 1]]);
    mode = 5; special = 0;
    input.event({ code: (InputCode.leftButton | InputCode.control) >>> 0, x: 56, y: 78 });
    assert.deepEqual(calls.splice(0), [['pick', 56, 78, 0], ['move', 56, 78], ['queue', 0x02000044, -2, 3]]);
    mode = 0;
    assert.throws(() => input.event({ code: InputCode.leftButton, x: 0, y: 0 }), /좌클릭 사건 분배 미연결/);
});
