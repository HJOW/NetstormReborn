// 실제 Native 참가 처리기로 로그인/AI의 서로 다른 기록과 반납 뒤 잔류 비트를 만든다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawPlayerParticipation, PlayerParticipationState } from '../www/core/raw-player-participation.js';
import { InputQueue } from '../www/core/input-queue.js';
import { ModGameHost } from '../www/modding/mod-game-host.js';

// 조회 어댑터가 빌리는 Client 외곽만 제공한다. 참가자 기록은 원래 처리기가 직접 만든다.
function makeHost() {
    const records = new Uint8Array(9 * 180), alliances = new Uint32Array(81), moneyReads = [], state = new PlayerParticipationState();
    const participation = new RawPlayerParticipation(records, alliances, new Uint8Array(9), state, { assignConnection() {}, destructBrain() {}, freeBrain() {}, assertion() { throw new Error('예상하지 않은 Native 단정'); } });
    participation.checkLogin(1, 1); participation.attachBrain(2, 123);
    const spState = { initialized: true }, session = { localConnectionReady: () => true, spStorage: () => spState, playerRecords: () => records, alliances: () => alliances, sp: (owner) => { moneyReads.push(owner); return owner * 1000; }, localOwner: () => 1, core: () => ({ pool: { capacity: 5 } }), participation: () => participation };
    const world = { connecting: false, session: () => session, mapView: () => ({ left: 85, top: 0, right: 1024, bottom: 768 }), selected: () => 0 };
    const app = { _campaignDisplayActive: () => true, _clientInputQueue: new InputQueue(), _campaignWorld: world, _clientRandom: { fortCoreSeed: 1 }, _layout: { logicalWidth: 1024, logicalHeight: 768 }, _clientGuide: { current: () => ({ script: { name: 'TheWarBegins' } }) }, _gameClock: { time: () => ({ game: 0 }), paused: () => false } };
    return { host: new ModGameHost(app), app, records, alliances, participation, moneyReads, spState };
}

test('모드 조회: 로그인 없는 실제 AI도 포함하고 반납 뒤 남은 AI bit는 제외한다', () => {
    const { host, records, participation, moneyReads } = makeHost(), flags = new DataView(records.buffer);
    assert.equal(flags.getUint32(2 * 180 + 0x50, true), 0x20); assert.equal(participation.brain(2), 123);
    const snapshot = host.snapshot(); assert.deepEqual(snapshot.players.map((player) => player.owner), [1, 2]); assert.deepEqual(snapshot.players.map((player) => player.stormPower), [1000, 2000]); assert.deepEqual(moneyReads, [1, 2]);
    participation.removeBrain(2); assert.equal(flags.getUint32(2 * 180 + 0x50, true), 0x20); assert.equal(participation.brain(2), 0);
    moneyReads.length = 0; assert.deepEqual(host.snapshot().players.map((player) => player.owner), [1]); assert.deepEqual(moneyReads, [1]); host.dispose();
});

test('모드 조회: 방향 동맹·로그인 참가자는 유지하고 사본 수정이 Native 기록을 바꾸지 않는다', () => {
    const { host, records, alliances, participation } = makeHost(); participation.checkLogin(3, 3); participation.setAlliance(1, 1 << 2);
    const before = records.slice(), beforeAllies = alliances.slice(), snapshot = host.snapshot();
    assert.deepEqual(snapshot.players.map((player) => player.owner), [1, 2, 3]); assert.deepEqual(snapshot.players[0].allies, [2]); assert.deepEqual(snapshot.players[1].allies, []);
    snapshot.players[0].allies.length = 0; snapshot.players[1].stormPower = 0;
    assert.deepEqual(host.snapshot().players[0].allies, [2]); assert.equal(host.snapshot().players[1].stormPower, 2000); assert.deepEqual(records, before); assert.deepEqual(alliances, beforeAllies); host.dispose();
});

test('모드 조회: SP 초기화 전에는 참가자 금액을 읽어서 초기화하지 않는다', () => {
    const { host, spState, moneyReads } = makeHost(); spState.initialized = false;
    assert.equal(host.snapshot().state.sessionId, null); assert.deepEqual(host.snapshot().players, []); assert.deepEqual(moneyReads, []);
    spState.initialized = true; assert.deepEqual(host.snapshot().players.map((player) => player.owner), [1, 2]); host.dispose();
});

test('Guide 반납 뒤 World가 잠시 남아도 종료된 미션을 노출하지 않는다', () => {
    const { host, app } = makeHost(); assert.equal(host.snapshot().state.sessionId, 'session-1'); app._clientGuide.current = () => null;
    assert.equal(host.snapshot().state.sessionId, null); assert.deepEqual(host.snapshot().players, []); host.dispose();
});
