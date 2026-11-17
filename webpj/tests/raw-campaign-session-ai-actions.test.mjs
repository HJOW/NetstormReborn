// Session 공개 AI 행동이 현재 토큰과 기존 실제 부품으로 전달되는 계약이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawCampaignSessionAiActions } from '../www/core/raw-campaign-session-ai-actions.js';

// 호출된 인수와 순서를 기록하는 현재 owner 계약이다.
function scene() {
    const calls = [], random = {}, scratch = {}, candidate = { x: 5, y: 6 };
    const record = (name, result) => (...args) => { calls.push([name, ...args]); return result; };
    const owner = { loaded: true, participation: { brain: (id) => id === 1 ? 123 : 0 }, random,
        aiProductionModules: { technology: record('technology', 42) }, aiStrategySetup: { refreshBridges: record('refresh') },
        aiBridgePath: (value) => { calls.push(['scratch', value]); return { advance: record('advance', true) }; },
        aiEconomy: { reconcile: record('economy') }, aiDeployment: { place: record('deploy') }, aiCollectors: { ensure: record('collectors') },
        aiFactoryProduction: { manage: record('factories') }, aiDrySpots: { add: record('add', true), maintain: record('maintain', 7), seed: record('seed') },
        aiDryGeometry: { candidate: record('candidate', candidate) }, aiFakeDrySpots: { extend: record('extend') },
    };
    return { owner, calls, scratch, random, candidate, actions: new RawCampaignSessionAiActions(owner) };
}

// 현재 기술·본체·호출자 scratch·공용 난수와 반환값을 그대로 전달한다.
test('Session AI 행동: 다리·생산·부족 좌표의 현재 장치 전달', () => {
    const { actions, calls, scratch, random, candidate } = scene(), point = { x: 2.1, y: 3.1 };
    assert.equal(actions.refreshBridges(1), true); assert.equal(actions.advanceBridge(1, point, scratch), true);
    assert.equal(actions.reconcileEconomy(1), true); assert.equal(actions.deploy(1, 71, 1.1, 2.2, 3, 4), true);
    assert.equal(actions.ensureCollectors(1), true); assert.equal(actions.manageFactories(1), true);
    assert.equal(actions.recordDrySpot(1, 1.1, 2.2, 71), true); assert.equal(actions.recheckDrySpots(1), 7);
    assert.equal(actions.seedDrySpot(1), true); assert.equal(actions.computeDryCandidate(point, candidate), candidate); assert.equal(actions.extendFakeDrySpots(1), true);
    assert.deepEqual(calls, [
        ['technology', 1], ['refresh', 123, 42], ['scratch', scratch], ['advance', 123, { x: Math.fround(point.x), y: Math.fround(point.y) }], ['economy', 123],
        ['deploy', 123, 71, Math.fround(1.1), Math.fround(2.2), 3, 4], ['collectors', 123], ['factories', 123],
        ['add', 123, Math.fround(1.1), Math.fround(2.2), 71], ['maintain', 123], ['seed', 123, random], ['candidate', point, candidate], ['extend', 123],
    ]);
});

// 미설치·잘못된 소유자를 거부하며 본체가 없는 owner는 하위 효과를 호출하지 않는다.
test('Session AI 행동: 설치·소유자·빈 본체 관문', () => {
    const { owner, actions, calls } = scene();
    assert.equal(actions.refreshBridges(2), false); assert.equal(actions.recheckDrySpots(2), 0); assert.deepEqual(calls, []);
    assert.throws(() => actions.deploy(9, 71, 0, 0, 0, 0), /owner1\.\.8/);
    owner.loaded = false; assert.throws(() => actions.seedDrySpot(1), /설치/); assert.throws(() => actions.computeDryCandidate({}, {}), /설치/);
});
