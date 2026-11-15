// 다음 루프 상태 전환 큐 client-state.js(ClientState)를 cpppj `State` 계약으로 대조한다(WB04-E525).
//  - cpppj 시험(MissionLifecycleTests 등) 대응: 빈 큐에서만 Post 를 받고(대기 중이면 두 번째 요청은 무시), Take 는 문자열 자원까지 소유한 맨 앞을 꺼내며, 빈 큐는 전환을 만들지 않고, Pending 은 조회만으로 꺼내지 않는다.
//  - Cancel: 대기 큐 맨 앞의 정확한 명령(`LoadMissionFort`·`LoadMissionWorld` 등)만 취소하고 다른 요청은 보존한다.
//  - TakeAfterPoll: Poll 진입 전의 같은 대기 요청만 꺼내고, Poll 이 취소/교체한 요청(일련번호가 바뀐 항목)은 같은 Tick 에서 꺼내지 않으며 Poll 장치가 없으면 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E525)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientPhase, ClientState } from '../www/core/client-state.js';

test('ClientState: 대기 중이면 새 전환을 받지 않고 Take 는 첫 요청만 꺼내며 빈 큐는 전환을 만들지 않는다', () => {
    const state = new ClientState();
    assert.equal(state.phase, ClientPhase.MainMenu);
    assert.equal(state.pending(), false);
    assert.equal(state.take(), null);
    state.post({ command: 'Tell', argument: 'ABORT' });
    assert.equal(state.pending(), true);
    // 한 번의 입력 묶음이 여러 화면을 건너뛰지 않는다.
    state.post({ command: 'Tell', argument: 'TryAgain' });
    // 조회만으로 꺼내지 않는다.
    assert.equal(state.pending(), true);
    const action = state.take();
    assert.deepEqual(action, { command: 'Tell', argument: 'ABORT' });
    assert.equal(state.pending(), false);
    assert.equal(state.take(), null);
    // 꺼낸 뒤에는 다시 새 요청을 받는다.
    state.post({ command: 'LoadMissionFort', argument: 'TheWarBegins' });
    assert.equal(state.take().argument, 'TheWarBegins');
});

test('ClientState: Cancel 은 맨 앞의 정확한 명령만 취소하고 다른 요청은 보존한다', () => {
    const state = new ClientState();
    assert.equal(state.cancel('LoadMissionFort'), false);
    state.post({ command: 'LoadMissionWorld', argument: '' });
    // 다른 명령은 취소하지 않고 보존한다.
    assert.equal(state.cancel('LoadMissionFort'), false);
    assert.equal(state.pending(), true);
    assert.equal(state.cancel('LoadMissionWorld'), true);
    assert.equal(state.pending(), false);
    // 명령은 대소문자까지 정확히 같아야 한다.
    state.post({ command: 'LoadMissionFort', argument: 'x' });
    assert.equal(state.cancel('loadmissionfort'), false);
    assert.equal(state.cancel('LoadMissionFort'), true);
});

test('ClientState: TakeAfterPoll 은 Poll 진입 전의 같은 요청만 꺼내고 Poll 이 바꾼 요청은 다음 Tick 까지 보존한다', () => {
    const state = new ClientState();
    assert.throws(() => state.takeAfterPoll(null), TypeError);
    // 빈 큐에서 Poll 이 새 요청을 예약하면 같은 Tick 에서 꺼내지 않는다.
    let polled = 0;
    assert.equal(state.takeAfterPoll(() => { polled += 1; state.post({ command: 'A', argument: '' }); }), null);
    assert.equal(polled, 1);
    assert.equal(state.pending(), true);
    // 대기 중인 같은 요청은 Poll 이 건드리지 않으면 꺼낸다.
    assert.equal(state.takeAfterPoll(() => { polled += 1; }).command, 'A');
    assert.equal(state.pending(), false);
    // Poll 이 대기 요청을 취소하고 교체 요청을 예약하면(일련번호가 바뀜) 교체 요청은 다음 Tick 까지 보존한다.
    state.post({ command: 'B', argument: '' });
    assert.equal(state.takeAfterPoll(() => { state.cancel('B'); state.post({ command: 'C', argument: '' }); }), null);
    assert.equal(state.pending(), true);
    assert.equal(state.take().command, 'C');
    // Poll 이 취소만 하면 아무것도 꺼내지 않는다.
    state.post({ command: 'D', argument: '' });
    assert.equal(state.takeAfterPoll(() => { state.cancel('D'); }), null);
    assert.equal(state.pending(), false);
    // Poll 이 예외를 던지면 대기 요청은 보존된다.
    state.post({ command: 'E', argument: '' });
    assert.throws(() => state.takeAfterPoll(() => { throw new Error('Poll 실패'); }), /Poll 실패/);
    assert.equal(state.pending(), true);
});
