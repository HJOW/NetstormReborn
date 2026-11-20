// 동맹 요청 입력 처리기(원본 HandleAllianceRequestInput)의 네 명령·뗌·요청 제한 창·안내·packet 모양을 확인한다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1126)
import test from 'node:test';
import assert from 'node:assert/strict';
import { AllianceRequestState, handleAllianceRequestInput } from '../www/core/raw-player-participation.js';

const REQUEST = 0x020000fb, BREAK = 0x020000fa, CONFIRM = 0x0200007e, DRAW = 0x020000db, RELEASE = 0x80000000;
// 9×180 Player 기록에 이름을 쓴다(이름은 +12, 32바이트).
function players(names = {}) { const bytes = new Uint8Array(9 * 180); for (const [owner, name] of Object.entries(names)) bytes.set([...name].map((c) => c.charCodeAt(0)), Number(owner) * 180 + 12); return bytes; }
// 시계와 송신·안내를 기록하는 장치다. clock 은 호출 때마다 시계 값을 읽는다.
function devices(clock) { const log = { sent: [], told: [], clocks: 0 }; return { log, hooks: { clock: () => { log.clocks += 1; return clock.now; }, localOwner: () => 1, send: (packet) => log.sent.push([...packet]), tell: (text) => log.told.push(text) } }; }

test('네 명령이 아니면 거부하고 뗌은 효과 없이 소비하며 수정 비트는 지운다', () => {
    const state = new AllianceRequestState(), { log, hooks } = devices({ now: 0 });
    assert.equal(handleAllianceRequestInput(0x02000120, 2, state, players(), hooks), false); assert.equal(handleAllianceRequestInput(REQUEST | RELEASE, 2, state, players(), hooks) , true);
    assert.deepEqual(log.sent, []); assert.equal(state.initialized, 0); assert.equal(log.clocks, 0);
    // 상위 입력 플래그(0xfc000000)는 지우고 명령만 본다.
    assert.equal(handleAllianceRequestInput((BREAK | 0x04000000) >>> 0, 3, state, players(), hooks), true); assert.deepEqual(log.sent, [[1, 0x79, 6, 0, 1, 3]]);
});

test('해제·확인·무승부는 제한 없이 packet 을 보내고 무승부 대상은 항상 1 이다', () => {
    const state = new AllianceRequestState(), { log, hooks } = devices({ now: 0 });
    for (let index = 0; index < 5; index += 1) handleAllianceRequestInput(BREAK, 2, state, players(), hooks);
    handleAllianceRequestInput(CONFIRM, 4, state, players(), hooks); handleAllianceRequestInput(DRAW, 7, state, players(), hooks);
    assert.equal(log.sent.length, 7); assert.deepEqual(log.sent[0], [1, 0x79, 6, 0, 1, 2]); assert.deepEqual(log.sent[5], [1, 0x78, 6, 0, 1, 4]); assert.deepEqual(log.sent[6], [1, 0x6f, 6, 0, 1, 1]);
    assert.equal(state.initialized, 0, '요청이 아니면 제한 상태를 건드리지 않는다'); assert.deepEqual(log.told, []);
});

test('요청: 최초는 아홉 시계를 저장하고 60초 창 안에서 세 번까지 보내며 넷째는 AllianceSpam 안내만 낸다', () => {
    const state = new AllianceRequestState(), clock = { now: 1000 }, { log, hooks } = devices(clock), list = players({ 2: 'Marquis', 3: 'Duke' });
    handleAllianceRequestInput(REQUEST, 2, state, list, hooks); assert.equal(state.initialized, 1); assert.equal(log.clocks, 9); assert.equal(state.clocks[2][1], 1000); assert.equal(state.counters[2], 1);
    clock.now = 20000; handleAllianceRequestInput(REQUEST, 2, state, list, hooks); handleAllianceRequestInput(REQUEST, 2, state, list, hooks);
    assert.equal(log.sent.length, 3); assert.deepEqual(log.sent[0], [1, 0x77, 6, 0, 1, 2]);
    handleAllianceRequestInput(REQUEST, 2, state, list, hooks);
    assert.equal(log.sent.length, 3); assert.deepEqual(log.told, ['AllianceSpam(3,~r,Marquis)']);
    // 다른 대상은 따로 센다.
    handleAllianceRequestInput(REQUEST, 3, state, list, hooks); assert.equal(log.sent.length, 4); assert.equal(state.counters[3], 1);
    // 61초가 지난 뒤 요청은 창이 새로 잡혀 다시 보낼 수 있다.
    clock.now = 1000 + 61000; handleAllianceRequestInput(REQUEST, 2, state, list, hooks); assert.equal(log.sent.length, 5); assert.equal(state.counters[2], 1);
});

test('요청: 대상 범위 밖이면 제한만 갱신하고 보내지 않으며 제한 변경과 이름 NUL 오류를 따른다', () => {
    const state = new AllianceRequestState(), { log, hooks } = devices({ now: 5 });
    assert.equal(handleAllianceRequestInput(REQUEST, 0, state, players(), hooks), true); assert.equal(handleAllianceRequestInput(REQUEST, 9, state, players(), hooks), true); assert.equal(handleAllianceRequestInput(REQUEST, -1, state, players(), hooks), true);
    assert.deepEqual(log.sent, []); assert.equal(state.initialized, 1);
    state.limit = 1; handleAllianceRequestInput(REQUEST, 4, state, players({ 4: 'Ann' }), hooks); handleAllianceRequestInput(REQUEST, 4, state, players({ 4: 'Ann' }), hooks);
    assert.equal(log.sent.length, 1); assert.deepEqual(log.told, ['AllianceSpam(1,~u,Ann)']);
    // 이름이 32바이트 모두 차 있으면 NUL 이 없어 호스트 오류이다.
    const full = players(); full.fill(65, 5 * 180 + 12, 5 * 180 + 44); state.limit = 0;
    assert.throws(() => handleAllianceRequestInput(REQUEST, 5, state, full, hooks), /NUL 부족/);
});

test('필수 장치가 없으면 해당 위치에서 거부한다', () => {
    const state = new AllianceRequestState();
    assert.throws(() => handleAllianceRequestInput(BREAK, 2, state, players(), { clock: () => 0, send: () => {}, tell: () => {} }), /미연결/);
    assert.throws(() => handleAllianceRequestInput(BREAK, 2, state, players(), { clock: () => 0, localOwner: () => 1, tell: () => {} }), /미연결/);
    assert.throws(() => handleAllianceRequestInput(REQUEST, 2, state, players(), { localOwner: () => 1, send: () => {}, tell: () => {} }), /미연결/);
});
