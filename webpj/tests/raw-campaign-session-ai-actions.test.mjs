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

// 이동·수집과 건설 선택은 현재 본체를 공유하며 검색 시작에 원래 좌표 비트를 넘긴다.
test('Session AI 행동: 이동과 건설 탐색 전달', () => {
    const { owner, actions, calls } = scene(), raw = new Uint8Array(0xb08), view = new DataView(raw.buffer);
    view.setUint32(0x260, 3, true); view.setFloat32(0x270, 1, true); view.setFloat32(0x274, 2, true);
    view.setFloat32(0x278, 10, true); view.setFloat32(0x27c, 20, true); view.setUint32(0x2b0, 0x7fa12345, true); view.setFloat32(0x2b4, 6.5, true);
    owner.aiMemory = { bytes: (token) => { assert.equal(token, 123); return raw; } };
    // 효과를 소비하는 실제 부품의 호출 순서와 인자만 관찰한다.
    const record = (name, result) => (...args) => { calls.push([name, ...args]); return result; };
    owner.aiMovement = { movementForm: record('form', 27), stop: record('stop'), toTarget: record('target'), toPoint: record('point'), home: record('home') };
    owner.aiResourceChoice = { collectOrHome: record('collect'), reclassifyDisconnected: record('geysers') };
    owner.aiCarrierActions = { act: record('carriers') }; owner.aiBuildSearch = { initialize: record('initialize'), scan: record('scan') };
    owner.aiBuildChoice = { select: record('select', 88), enemyLowFunds: record('low', true) };
    assert.equal(actions.movementForm(8), 27); actions.stopMovement(8); assert.equal(actions.moveToObject(1, 8, 9, 0x100000000), true);
    assert.equal(actions.moveToPoint(1, 8, { x: 1.1, y: 2.2 }, 0), true); assert.equal(actions.returnHome(1, 8), true);
    assert.equal(actions.collectOrHome(1, 8), true); assert.equal(actions.recheckGeysers(1), true); assert.equal(actions.manageCarriers(1), true);
    assert.equal(actions.startBuildSearch(1, 71), true); assert.equal(actions.advanceBuildSearch(1, 1), true);
    assert.equal(actions.selectBuildType(1), 88); assert.equal(actions.enemyLowFunds(1), true);
    const init = calls.find((call) => call[0] === 'initialize');
    assert.deepEqual(init.slice(1, 4), [123, 71, { left: 1, top: 2, right: 10, bottom: 20 }]);
    assert.equal(init[4].xBits, 0x7fa12345); assert.ok(Number.isNaN(init[4].x)); assert.equal(init[4].y, 6.5); assert.equal(init[5], 3);
    assert.deepEqual(calls.find((call) => call[0] === 'point'), ['point', 123, 8, { x: Math.fround(1.1), y: Math.fround(2.2) }, 0]);
    assert.deepEqual(calls.find((call) => call[0] === 'target'), ['target', 123, 8, 9, 0]);
    assert.equal(actions.startBuildSearch(2, 71), false); assert.equal(actions.selectBuildType(2), 0);
});

// 표적·배치 평가의 결과 상자를 실제 소비자가 직접 수정한다.
test('Session AI 행동: 사제·배치 점수와 현재 출력 상자', () => {
    const { owner, actions, calls } = scene(), target = { value: 4 }, bestPoint = { x: 0, y: 0 }, bestScore = { value: -1 };
    owner.aiPriestActions = { act: (token, priest) => calls.push(['priest', token, priest]) };
    owner.aiDropTargets = {
        score: (token, sid) => { calls.push(['score', token, sid]); return 17; },
        gun: (token, type, point, direction, box) => { calls.push(['gun', token, type, point, direction]); box.value = 9; return 19; },
        blockers: (token, type, point, direction, box) => { calls.push(['blockers', token, type, point, direction]); box.value = 8; return 23; },
    };
    owner.aiDropEvaluation = {
        evaluate: (token, type, point, direction, box) => { calls.push(['evaluate', token, type, point, direction]); box.value = 7; return 29; },
        consider: (token, type, point, direction, best, score, box) => { calls.push(['consider', token, type, point, direction]); best.x = point.x; score.value = 31; box.value = 6; return true; },
    };
    owner.aiDropScan = { scan: () => true, remove: (token, index) => calls.push(['remove', token, index]) };
    owner.aiDropSelection = { select: (token) => calls.push(['select', token]) }; owner.aiDropActions = { run: (token) => calls.push(['drops', token]) };
    assert.equal(actions.managePriest(1, 0), true); assert.equal(actions.scoreTarget(1, 11), 17);
    assert.equal(actions.scoreGun(1, 71, 1.1, 2.2, -1, target), 19); assert.equal(target.value, 9);
    assert.equal(actions.scoreBlockers(1, 71, 1, 2, 2, target), 23); assert.equal(target.value, 8);
    assert.equal(actions.scoreDrop(1, 71, 1, 2, 2, target), 29); assert.equal(target.value, 7);
    assert.equal(actions.considerDrop(1, 71, 1.1, 2, 2, bestPoint, bestScore, target), true);
    assert.equal(bestPoint.x, Math.fround(1.1)); assert.equal(bestScore.value, 31); assert.equal(target.value, 6);
    assert.equal(actions.advanceDropScan(1, 71, 2), true); assert.equal(actions.removeDropChoice(1, 0xffffffff), true);
    assert.equal(actions.selectDropType(1), true); assert.equal(actions.manageDrops(1), true);
    assert.deepEqual(calls.find((call) => call[0] === 'remove'), ['remove', 123, -1]);
    target.value = 42; assert.equal(actions.scoreGun(2, 71, 0, 0, 0, target), 0); assert.equal(target.value, 42);
    assert.deepEqual(calls[0], ['priest', 123, 0]);
});

// 기본 이름과 본체 반납은 미션 설치보다 이름·참여 장치의 현재 수명을 따른다.
test('Session AI 행동: 이름·본체 소유권 관문', () => {
    const { owner, actions, calls } = scene(); owner.loaded = false;
    assert.throws(() => actions.objectName(1), /설치/); assert.throws(() => actions.objectBaseName(1), /미설치/);
    owner.objectNames = { name: () => 'name', baseName: (sid) => `base${sid}` };
    owner.participation.removeBrain = (id) => calls.push(['removeBrain', id]); owner.aiMemory = {};
    assert.equal(actions.objectBaseName(4), 'base4'); actions.remove(2); assert.deepEqual(calls, [['removeBrain', 2]]);
    assert.equal(actions.memory(), owner.aiMemory); owner.loaded = true; assert.equal(actions.objectName(4), 'name');
});
