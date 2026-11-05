// 세션 장기 상태 3단계(WB04-C3)를 검사한다: 원본 40칸 입력 큐와 메인 루프 단계 순서.
//  - InputQueue: cpppj tests/ScreenTests.cpp 의 InputQueue_* 세 검사(순서·가득 참·뗌 거르기·DEL→Ctrl+Backspace·명령 코드 다듬기)를 옮겼다(원본 관측 fixture 는 없다).
//  - MainLoop: docs/exe/main-loop.md 2절의 패치판 순서, 영상 갱신 정지 때 00464170 만 실행, 입력 종료 절차, 처리기 없는 단계의 보고,
//    그리고 GameClock·InputQueue·Kernel 을 함께 돌린 프레임 순서(입력→Kernel, 갱신 단계 예약 명령은 다음 프레임 입력).
// (2026-11-05 WB04-C3: 입력/예약 큐와 프레임 순서 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { INPUT_QUEUE_CAPACITY, InputCode, InputQueue } from '../www/core/input-queue.js';
import { MAIN_LOOP_STEPS, MainLoop } from '../www/core/main-loop.js';
import { GameClock } from '../www/core/game-clock.js';
import { BaseProcess, Kernel } from '../www/core/kernel.js';

// 넣은 순서로 나오고 빈 큐는 0 사건을 준다(원본 00452f90·00452f00).
test('InputQueue: 넣은 순서로 꺼내고 빈 큐는 0 사건', () => {
    const queue = new InputQueue();
    assert.equal(queue.empty, true);
    assert.deepEqual(queue.pop(false), { code: 0, x: 0, y: 0 });
    queue.push(InputCode.leftButton, 10, 20);
    queue.push(InputCode.rightButton | InputCode.shift, 30, 40);
    assert.equal(queue.empty, false);
    assert.equal(queue.size, 2);
    assert.deepEqual(queue.pop(false), { code: InputCode.leftButton, x: 10, y: 20 });
    assert.deepEqual(queue.pop(false), { code: InputCode.rightButton | InputCode.shift, x: 30, y: 40 });
    assert.equal(queue.empty, true);
    assert.throws(() => queue.push(-1, 0, 0), RangeError);
    assert.throws(() => queue.push(0, 2 ** 31, 0), RangeError);
});

// 40칸 고리에는 39개까지 담기고, 가득 차면 새로 넣은 것을 버린다. 한 바퀴 뒤에도 순서가 유지된다.
test('InputQueue: 39개까지 담고 넘치면 새 사건을 버린다', () => {
    const queue = new InputQueue();
    // 칸 수보다 많이 넣는다.
    for (let index = 1; index <= 50; index += 1) queue.push(index, index, 0);
    assert.equal(queue.size, INPUT_QUEUE_CAPACITY - 1);
    // 앞의 39개가 순서대로 남는다.
    for (let index = 1; index <= INPUT_QUEUE_CAPACITY - 1; index += 1) assert.equal(queue.pop(false).code, index);
    assert.equal(queue.empty, true);
    queue.push(100, 0, 0);
    queue.push(101, 0, 0);
    assert.deepEqual([queue.pop(false).code, queue.pop(false).code, queue.empty], [100, 101, true]);
});

// 뗌 거르기(꺼낸 뒤 판단해 큐에서는 빠짐), DEL → Ctrl+Backspace(가상 키 0x7f 는 그대로), 명령 사건 코드 다듬기.
test('InputQueue: 뗌 거르기·DEL 변환·명령 예약 코드', () => {
    const queue = new InputQueue();
    queue.push((InputCode.leftButton | InputCode.release) >>> 0, 1, 2);
    queue.push(InputCode.leftButton, 3, 4);
    assert.deepEqual(queue.pop(true), { code: 0, x: 0, y: 0 });
    assert.deepEqual(queue.pop(true), { code: InputCode.leftButton, x: 3, y: 4 });
    queue.push((InputCode.rightButton | InputCode.release) >>> 0, 5, 6);
    assert.equal(queue.pop(false).code, (InputCode.rightButton | InputCode.release) >>> 0);
    queue.push(0x7f, 0, 0);
    assert.equal(queue.pop(false).code, InputCode.control | 8);
    queue.push((InputCode.release | 0x7f) >>> 0, 0, 0);
    assert.equal(queue.pop(false).code, (InputCode.release | InputCode.control | 8) >>> 0);
    queue.push(0x7f << InputCode.virtualKeyShift, 0, 0);
    assert.equal(queue.pop(false).code, 0x7f << InputCode.virtualKeyShift);
    queue.pushCommand(0xffffffff, 7, 8);
    assert.deepEqual(queue.pop(false), { code: (0x0100ffff | InputCode.command) >>> 0, x: 7, y: 8 });
    // 꺼낸 사건을 바꿔도 큐 저장소는 바뀌지 않는다(다시 넣은 사건으로 확인).
    queue.push(5, 1, 1);
    const event = queue.pop(false);
    event.code = 9;
    queue.push(6, 1, 1);
    assert.equal(queue.pop(false).code, 6);
});

// 단계 표: 원본 순서 번호가 줄지 않고, 갱신 목록이 문서의 패치판 순서(Kernel 은 AI 해제 뒤·간헐천 앞)이며, 입력(9)이 Kernel 보다 앞이다.
test('MainLoop: 단계 표가 원본 메인 루프 순서와 같다', () => {
    const orders = MAIN_LOOP_STEPS.map((step) => step.order);
    assert.deepEqual(orders, [...orders].sort((a, b) => a - b));
    assert.deepEqual(MAIN_LOOP_STEPS.filter((step) => step.phase === 'update').map((step) => step.address), [
        0x00435060, 0x00441de0, 0x004b9010, 0x004b67c0, 0x004180f0, 0x004dd2b0, 0x004182c0, 0x00422bc0, 0x00469f60, 0x00446c50,
        0x00481320, 0x004662b0, 0x00464170, 0x004ab9a0, 0x00417de0, 0x00471a30, 0x00486bc0, 0x004b6730, 0x0044ac30, 0x004d3090,
    ]);
    const index = (id) => MAIN_LOOP_STEPS.findIndex((step) => step.id === id);
    assert.ok(index('captureTime') < index('input') && index('input') < index('kernel') && index('kernel') < index('draw'));
    assert.ok(index('cleanupGumps') < index('idleGumps') && index('idleGumps') < index('kernel'));
    // 문서 표의 주소가 모두 원본 문서에 실제로 적혀 있다(옮겨 적기 오류 확인).
    const documentPath = path.join(REPO_ROOT, 'docs', 'exe', 'main-loop.md');
    if (fs.existsSync(documentPath)) {
        const document = fs.readFileSync(documentPath, 'utf8');
        for (const step of MAIN_LOOP_STEPS.filter((entry) => entry.address !== null)) {
            assert.ok(document.includes(`FUN_${step.address.toString(16).padStart(8, '0')}`), `${step.id} 주소가 main-loop.md 에 없음`);
        }
    }
});

// 실행: 처리기가 있는 단계만 표 순서로 실행하고 나머지는 missing 으로 보고한다. 제외 단계·없는 단계의 처리기는 거부한다.
test('MainLoop: 처리기 순서·미연결 보고·제외 단계 거부', () => {
    const calls = [];
    // 반환값이 없는 기록 처리기이다(입력의 참 반환은 종료 절차이므로 push 결과를 돌려주지 않는다).
    const record = (id) => () => { calls.push(id); };
    const loop = new MainLoop({ handlers: { draw: record('draw'), kernel: record('kernel'), input: record('input'), captureTime: record('captureTime') } });
    const result = loop.runFrame();
    assert.deepEqual(calls, ['captureTime', 'input', 'kernel', 'draw']);
    assert.deepEqual(result.ran, calls);
    assert.equal(result.quit, false);
    assert.equal(result.missing.includes('stateTick') && result.missing.includes('geysers') && result.missing.includes('transportPoll'), true);
    assert.equal(result.missing.includes('priority') || result.missing.includes('timeSync') || result.missing.includes('cdCheck'), false, '제외 단계는 미연결로 세지 않는다');
    assert.throws(() => new MainLoop({ handlers: { timeSync: () => {} } }), /제외/);
    assert.throws(() => new MainLoop({ handlers: { nothing: () => {} } }), /없는 단계/);
    assert.throws(() => new MainLoop({ handlers: { kernel: 1 } }), TypeError);
});

// 영상 갱신 정지(깊이 ≥1): 갱신 목록 중 00464170 만 실행하고 그리기·알림은 계속한다. 입력의 참 반환은 이 프레임의 나머지를 하지 않는다.
test('MainLoop: 영상 갱신 정지와 입력 종료 절차', () => {
    let depth = 1;
    const calls = [];
    const handlers = Object.fromEntries(['captureTime', 'input', 'configSave', 'idleGumps', 'kernel', 'draw', 'playerNotices'].map((id) => [id, () => { calls.push(id); }]));
    const loop = new MainLoop({ handlers, movieUpdatePauseDepth: () => depth });
    assert.equal(loop.runFrame().updatesPaused, true);
    assert.deepEqual(calls, ['captureTime', 'input', 'idleGumps', 'draw', 'playerNotices']);
    calls.length = 0;
    depth = 0;
    loop.runFrame();
    assert.deepEqual(calls, ['captureTime', 'input', 'configSave', 'idleGumps', 'kernel', 'draw', 'playerNotices']);
    // 음수 깊이도 정지가 아니다(원본 signed < 1).
    calls.length = 0;
    depth = -3;
    assert.equal(loop.runFrame().updatesPaused, false);
    // 입력이 참을 돌려주면 갱신·그리기·알림을 하지 않는다.
    const quitting = new MainLoop({ handlers: { ...handlers, input: () => { calls.push('input'); return true; } } });
    calls.length = 0;
    const result = quitting.runFrame();
    assert.deepEqual([calls, result.quit], [['captureTime', 'input'], true]);
    // 원본 int 반환: 0 이 아닌 정수도 종료, 0 은 계속이다. 그 밖의 반환값은 처리기 오류로 알린다.
    assert.equal(new MainLoop({ handlers: { input: () => -1 } }).runFrame().quit, true);
    assert.equal(new MainLoop({ handlers: { input: () => 0 } }).runFrame().quit, false);
    assert.throws(() => new MainLoop({ handlers: { input: () => [] } }).runFrame(), TypeError);
});

// 프레임 순서 통합: 시각 고정 → 입력(큐 비우기) → Kernel. 갱신 단계(cleanupGumps)에서 예약한 명령은 다음 프레임 입력에서 소비되고,
// 입력에서 내린 명령은 같은 프레임 Kernel 이 본다. 프로세스는 고정된 프레임 시각을 읽는다.
test('MainLoop: 시계·입력 큐·Kernel 의 프레임 순서', () => {
    let now = 1000;
    const clock = new GameClock(0);
    const queue = new InputQueue();
    const kernel = new Kernel(8);
    const log = [];
    let frame = null;
    let pendingOrder = null;
    // 입력에서 받은 명령을 같은 프레임에 처리하는 프로세스이다.
    class Worker extends BaseProcess {
        runFrame() {
            log.push(`kernel:${frame.number}:${frame.game}:${pendingOrder ?? '-'}`);
            pendingOrder = null;
        }
    }
    kernel.add(new Worker());
    let reserveNext = false;
    const loop = new MainLoop({
        handlers: {
            captureTime: () => { frame = clock.capture(now); },
            input: () => {
                // 원본 004d62b0 처럼 큐가 빌 때까지 꺼낸다.
                while (!queue.empty) {
                    const event = queue.pop(false);
                    log.push(`input:${frame.number}:${event.code.toString(16)}`);
                    if (event.code & InputCode.command) pendingOrder = event.code & 0xffff;
                }
                return false;
            },
            cleanupGumps: () => {
                if (reserveNext) {
                    queue.pushCommand(0x1f, 0, 0);
                    log.push(`reserve:${frame.number}`);
                    reserveNext = false;
                }
            },
            kernel: () => kernel.runFrame(),
        },
    });
    // 1프레임: 브라우저 처리기가 프레임 사이에 넣은 클릭을 입력에서 소비한다.
    queue.push(InputCode.leftButton, 5, 5);
    reserveNext = true;
    loop.runFrame();
    now = 2500;
    loop.runFrame();
    assert.deepEqual(log, [
        'input:1:10000', 'reserve:1', 'kernel:1:1:-',
        'input:2:200001f', 'kernel:2:2.5:31',
    ]);
});
