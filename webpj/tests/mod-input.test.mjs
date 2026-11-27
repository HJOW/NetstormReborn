// 실제 원본 InputQueue를 사용해 모드 경계의 원자성·FIFO·미션/모달 재검증을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputQueue, InputCode } from '../www/core/input-queue.js';
import { ModRuntime, ModError, emptyModSnapshot } from '../www/modding/mod-api.js';
import { validateModOutput, queueModOutput } from '../www/modding/mod-input.js';
import { ModGameHost } from '../www/modding/mod-game-host.js';

// 현재 크기와 미션 번호는 명령 생성 이후 호스트에서 바꿀 수 있다.
function snapshot(sessionId = 'session-1', width = 1024) {
    const input = emptyModSnapshot(); Object.assign(input.state, { sessionId, mission: 'TheWarBegins', localOwner: 1, logicalSize: { width, height: 768 }, mapRect: { left: 85, top: 0, right: width, bottom: 768 } }); return input;
}
// 실제 공개 API의 출력을 만들어 호스트 검증까지 통과시킨다.
function output(callback) {
    const runtime = new ModRuntime({ modId: 'test.queue' }); runtime.register((api) => api.on('sessionStart', () => callback(api.commands)));
    return runtime.dispatch({ name: 'sessionStart', sequence: 1, sessionId: 'session-1', mission: 'TheWarBegins', gameSeconds: 0 }, snapshot());
}
// 현재 신뢰 호스트 인수를 만든다.
function devices(queue, extra = {}) { return { eventSequence: 1, sessionId: 'session-1', snapshot: snapshot(), queue, pointer: { x: 8, y: 9 }, ...extra }; }
// 큐 사건을 원래 순서로 소비한다.
function drain(queue) { const events = []; while (!queue.empty) events.push(queue.pop(false)); return events; }
// 임의 개수의 선행 사람 입력을 넣는다.
function fill(queue, count) { for (let index = 0; index < count; index += 1) queue.push(0x61, index, 0); }
// 호스트 데이터 형식 오류만 식별한다.
const invalid = (error) => error instanceof ModError && error.code === 'invalidArgument';

test('사람 입력 뒤에 클릭 누름/뗌과 문자/방향키를 원래 비트로 넣는다', () => {
    const queue = new InputQueue(); queue.push(0x61, 1, 2);
    const batch = output((commands) => { commands.click(200, 300, 'right', { control: true }); commands.selectPriest(); commands.pressKey('ArrowLeft', { shift: true }); });
    const result = queueModOutput(batch, devices(queue)); assert.deepEqual(result.pointer, { x: 200, y: 300 }); assert.ok(result.results.every((item) => item.status === 'queued' && item.eventSequence === 1));
    const events = drain(queue); assert.deepEqual(events[0], { code: 0x61, x: 1, y: 2 });
    assert.deepEqual(events.slice(1).map((item) => item.code), [0x10020000, 0x90020000, 0x70, 0x80000070, 0x20250000, 0xa0250000]);
    assert.ok(events.slice(1).every((item) => item.x === 200 && item.y === 300));
});

test('포화에서 누름만 남기지 않고 수용된 앞 명령은 보존한다', () => {
    const queue = new InputQueue(); fill(queue, 35);
    const batch = output((commands) => { commands.selectPriest(); commands.nextUnit(); commands.rotatePiece(); });
    const result = queueModOutput(batch, devices(queue)); assert.deepEqual(result.results.map((item) => item.code), [null, null, 'queueFull']); assert.equal(queue.size, 39);
    assert.deepEqual(drain(queue).slice(35).map((item) => item.code), [0x70, 0x80000070, 0x6e, 0x8000006e]);
    fill(queue, 38); const full = queueModOutput(output((commands) => commands.click(100, 200)), devices(queue)); assert.equal(full.results[0].code, 'queueFull'); assert.equal(queue.size, 38); assert.equal(full.pointer, null);
});

test('원래 고리의 감김에서도 선행 입력을 추월하지 않는다', () => {
    const queue = new InputQueue(); fill(queue, 38); for (let index = 0; index < 37; index += 1) queue.pop(false);
    queueModOutput(output((commands) => commands.selectPriest()), devices(queue));
    assert.deepEqual(drain(queue), [{ code: 0x61, x: 37, y: 0 }, { code: 0x70, x: 8, y: 9 }, { code: 0x80000070, x: 8, y: 9 }]);
});

test('미션 이탈/교체·모달·정지는 큐/포인터를 바꾸지 않는다', () => {
    const batch = output((commands) => commands.click(100, 200));
    // 현재 상태를 바꿔도 원래 요청 미션 번호는 별도 신뢰 인수로 유지한다.
    for (const [extra, expected] of [[{ snapshot: emptyModSnapshot() }, 'noSession'], [{ snapshot: snapshot('session-2') }, 'staleSession'], [{ blocked: true }, 'inputBlocked'], [{ snapshot: (() => { const input = snapshot(); input.state.paused = true; return input; })() }, 'inputBlocked']]) {
        const queue = new InputQueue(); queue.push(0x61, 1, 2); const result = queueModOutput(batch, devices(queue, extra));
        assert.equal(result.results[0].code, expected); assert.equal(result.pointer, null); assert.deepEqual(drain(queue), [{ code: 0x61, x: 1, y: 2 }]);
    }
});

test('화면 크기 변경 후 범위 밖 클릭만 거절하고 뒤 키는 처리한다', () => {
    const queue = new InputQueue(), batch = output((commands) => { commands.click(1000, 100); commands.selectPriest(); });
    const result = queueModOutput(batch, devices(queue, { snapshot: snapshot('session-1', 800) }));
    assert.deepEqual(result.results.map((item) => item.code), ['invalidArgument', null]); assert.equal(result.pointer, null); assert.equal(queue.size, 2);
});

test('위조 출력의 원본 명령·미션·번호·보조키·로그는 첫 큐 효과 전에 거절한다', () => {
    const original = output((commands) => { commands.selectPriest(); commands.nextUnit(); });
    // 정상 첫 명령 뒤의 위조도 전체 출력 검증 단계에서 잡아 앞 명령을 넣지 않는다.
    for (const change of [(batch) => { batch.commands[1].name = 'pushCommand'; }, (batch) => { batch.commands[1].sessionId = 'session-2'; }, (batch) => { batch.commands[1].commandId = 9; }, (batch) => { batch.commands[1].args.modifiers.alt = true; }, (batch) => { batch.commands[1].args.key = 'Escape'; }, (batch) => { batch.logs.push({ level: 'info', text: 'x'.repeat(1025) }); }, (batch) => { batch.extra = 1; }, (batch) => { batch.eventSequence = 2; }]) {
        const batch = structuredClone(original), queue = new InputQueue(); change(batch); assert.throws(() => queueModOutput(batch, devices(queue)), invalid); assert.equal(queue.size, 0);
    }
});

test('전송 개수/접근자 위조와 등록 중 명령을 다시 거절한다', () => {
    const batch = output((commands) => commands.selectPriest());
    assert.throws(() => validateModOutput(batch, { eventSequence: 0, sessionId: 'session-1' }), invalid);
    const many = structuredClone(batch); many.commands = Array(9).fill(many.commands[0]); assert.throws(() => validateModOutput(many, { eventSequence: 1, sessionId: 'session-1' }), invalid);
    let read = false; const accessor = Object.defineProperty({}, 'commands', { enumerable: true, get() { read = true; return []; } }); assert.throws(() => validateModOutput(accessor, { eventSequence: 1, sessionId: 'session-1' }), invalid); assert.equal(read, false);
});

test('모든 허용 문자는 임의 명령 번호 없이 누름/뗌 쌍이다', () => {
    const queue = new InputQueue(), batch = output((commands) => { commands.produceBridge(0); commands.produceBridge(1); commands.produceBridge(2); commands.produceBridge(3); commands.produceBridge(4); commands.produceBridge(5); commands.repeatProduction(); commands.rotatePiece(); });
    queueModOutput(batch, devices(queue)); const events = drain(queue);
    assert.equal(events.length, 16); assert.deepEqual(events.filter((_, index) => index % 2 === 0).map((item) => String.fromCharCode(item.code)), ['q', 'w', 'a', 's', 'z', 'x', 'd', 'c']);
    assert.ok(events.filter((_, index) => index % 2 === 1).every((item) => (item.code & InputCode.release) !== 0));
});

test('미션 없는 실제 호스트 외곽은 빈 값 사본을 주고 반납 뒤 접근을 거절한다', () => {
    const app = { _campaignDisplayActive: () => false, _clientInputQueue: new InputQueue(), _layout: { logicalWidth: 1024, logicalHeight: 768 } }, host = new ModGameHost(app);
    const state = host.snapshot(); assert.equal(state.state.sessionId, null); assert.equal(host.sessionSeed(), 0); assert.equal(host.inputBlocked(), true);
    state.state.logicalSize.width = 1; assert.equal(host.snapshot().state.logicalSize.width, 1024); host.dispose(); assert.throws(() => host.snapshot(), (error) => error.code === 'disposed');
});
