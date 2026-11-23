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
        selectFromPointer: (...args) => calls.push(['select', ...args]),
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
    const event = { code: (InputCode.leftButton | InputCode.control) >>> 0, x: 0, y: 0 };
    input.event(event); assert.deepEqual(calls.splice(0), [['pick', 0, 0, 0], ['select', 77, 0, true, event]]);
    mode = 1; assert.throws(() => input.event(event), /웹 제외 편집\/항해/);
});

// 11.01 방향키 카메라 이동(cpppj 1328): 활성 조각 회전이 먼저이고, 남은 ←↑→↓ 누름만 카메라 장치에 32픽셀씩 보낸다. 뗌·정지는 보내지 않는다. (2026-11-23 WB04-E1291)
test('방향키: 조각 회전이 먼저 소비하고 남은 누름만 32픽셀 카메라 이동이며 뗌·정지는 거부된다', () => {
    const moves = []; let held = false, paused = false;
    const cursor = { snapToGrid: {}, placement: {}, handleInput: (event) => held && (event.code === 0x250000 || event.code === 0x270000) };
    const world = { session: () => ({ cursor: () => cursor }), cursorRotationMode: () => 0 };
    const input = new CampaignUserInput(world, { readInt: () => ({ value: 0 }) }, { paused: () => paused, queue: { pushCommand: () => {} }, cameraKey: (dx, dy) => moves.push([dx, dy]) });
    const ARROWS = { left: 0x250000, up: 0x260000, right: 0x270000, down: 0x280000 };
    for (const code of Object.values(ARROWS)) input.event({ code, x: 0, y: 0 });
    assert.deepEqual(moves.splice(0), [[-32, 0], [0, -32], [32, 0], [0, 32]], '조각이 없으면 네 방향 모두 카메라');
    held = true;
    input.event({ code: ARROWS.left, x: 0, y: 0 }); input.event({ code: ARROWS.right, x: 0, y: 0 });
    assert.deepEqual(moves, [], '조각을 든 ←/→ 는 회전이 소비한다');
    input.event({ code: ARROWS.up, x: 0, y: 0 }); input.event({ code: ARROWS.down, x: 0, y: 0 });
    assert.deepEqual(moves.splice(0), [[0, -32], [0, 32]], '조각을 들어도 ↑/↓ 는 카메라');
    held = false;
    input.event({ code: (ARROWS.right | InputCode.release) >>> 0, x: 0, y: 0 }); assert.deepEqual(moves, [], '키 뗌은 움직이지 않는다');
    paused = true; input.event({ code: ARROWS.right, x: 0, y: 0 }); assert.deepEqual(moves, [], '정지 중에는 움직이지 않는다');
    paused = false;
    // 수정키가 섞인 가상 키 코드(Shift 등)도 키 번호가 같으면 같은 방향이다(cpppj 는 `code & 0x03ffffff` 로 판정).
    input.event({ code: (ARROWS.right | InputCode.shift) >>> 0, x: 0, y: 0 }); assert.deepEqual(moves.splice(0), [[32, 0]]);
    // 장치가 없으면 조용히 건너뛰지 않고 오류이다.
    const bare = new CampaignUserInput(world, { readInt: () => ({ value: 0 }) }, { paused: () => false, queue: { pushCommand: () => {} } });
    assert.throws(() => bare.event({ code: ARROWS.left, x: 0, y: 0 }), /방향키 카메라 장치 미연결/);
});
