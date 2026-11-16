// 실제 Session AI 배치/생산·다리 탐색/건설 선택 부모의 조립 계약이다(WB04-E625).
// Load·자연 플레이/포획/희생/승리·공중3종 새 전용 생성/생산/행동을 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RegularProcess } from '../www/core/squid-process.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { get, put } from './raw-support.mjs';

// 같은 실제 생성자 순서를 사용하며 외부 채널/로그만 기록한다.
function owner(extra = {}) {
    const trace = []; const o = campaignOwners({ ...extra, missionOutcome: { demo: () => 0 }, player: { log: (...args) => trace.push(['log', ...args]), ...extra.player },
        ai: { allocationBytes: (bytes) => bytes.fill(0), defaultName: () => 'Production probe', participation: { clearChat: (...args) => trace.push(['clear', ...args]), addChat: (...args) => trace.push(['add', ...args]), sendAlliance: (...args) => trace.push(['send', ...args]) }, ...extra.ai } });
    try { o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }); o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); return { o, trace }; }
    catch (error) { o.dispose(); throw error; }
}
// 원문600BYTE 명시 입력과 실제 Player 기술 부모/초기 갱신을 사용한다.
function brain(o) { o.playerState.playerKnown = 1; const settings = new Uint8Array(600); put(settings, 0x22c, 500); put(settings, 0x240, 1); return o.aiCreate.create(2, settings); }

test('Session AI 생산 조립: 선행·반복·반납·같은 실제 Pool/난수와 생성 시 본체 할당0', () => {
    const p = campaignOwners();
    try { assert.throws(() => p.connectAiProduction(), /초기화 전/); } finally { p.dispose(); }
    const { o } = owner();
    try {
        const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectAiProduction();
        assert.equal(o.aiProductionModules.ready, true); assert.equal(o.aiDeployment.pool, o.core.pool); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.deepEqual(o.aiMemory.allocations, []);
        assert.equal(o.loaded, false); assert.throws(() => o.connectAiProduction(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectAiProduction(), /반납/);
});

test('Session AI Regular: 같은 실제 확보 객체·내용물 부모/Host·payload·현재 시각과 누락 오류', () => {
    const { o } = owner();
    try {
        o.connectAiProduction(); const parent = o.world.create(85); const h = o.aiProductionModules.deploymentHooks;
        assert.throws(() => h.addRegular(parent, 3.5), /예약 누락/); assert.equal(h.reserveRegular(), true); const reserved = o.aiPendingRegular; assert.ok(reserved instanceof RegularProcess);
        o.core.processState.now = 27.25; h.addRegular(parent, 3.5); assert.equal(o.aiPendingRegular, null); assert.equal(o.core.processes.findEvent(parent, 0), reserved); assert.equal(reserved.payload, 3.5); assert.equal(reserved.time, 27.25);
        assert.throws(() => h.addRegular(parent, 3.5), /예약 누락/); assert.equal(h.reserveRegular(), true);
    } finally { o.dispose(); }
    assert.equal(o.aiPendingRegular, null);
});

test('Session AI 비용/역할: 같은 현재 지갑·기술 부모·워크샾/신전 역할 변경과 실제 미연결 관문', () => {
    const { o } = owner();
    try {
        o.connectAiProduction(); const token = brain(o); const raw = o.aiMemory.bytes(token);
        const parent = o.player.technology(2); assert.equal(o.aiProductionModules.technology(2), parent); assert.ok(o.core.types[85].cost > 0);
        o.aiStrategySetup.seedBridges(token, parent, 1); const piece = o.priests.carrier.findContained(parent, 0x40, 82, 0); assert.ok(piece); assert.equal(get(o.core.pool.slot(piece), 18, 2), 82);
        put(raw, 0x2d8, 0); assert.equal(o.aiDeployment.canAfford(token, 85), false); put(raw, 0x2d8, 0x7fffffff); assert.equal(o.aiDeployment.canAfford(token, 85), true);
        assert.equal(o.aiBuildChoice.workshopType(0), 86); o.aiWorkshops[0] = 85; assert.equal(o.aiBuildChoice.workshopType(0), 85);
        o.tutorialTypes.set(0x541248, 125); assert.equal(o.aiBuildChoice.templeType(1), 125);
        assert.throws(() => o.aiProductionModules.deploymentHooks.localPlace({}), /미설치/); assert.throws(() => o.aiProductionModules.collectorHooks.mayPlace({}), /MayPlace/);
        assert.throws(() => o.aiProductionModules.factoryHooks.level(parent), /미지원 실제 표/);
        o.players[3 * 180 + 0x50] = 0x22; assert.equal(o.aiProductionModules.enemyAvailable(3), true); o.priestState.priests.currentOwnerState()[3] = 1; assert.equal(o.aiProductionModules.enemyAvailable(3), false);
    } finally { o.dispose(); }
});

test('Session AI 다리: 실제 현재 FrameCodeTable/보호 패턴으로 모양을 만들고 게임 난수는 바꾸지 않는다', () => {
    const { o } = owner();
    try {
        o.connectAiProduction(); const bytes = new Uint8Array(0xbc); const seed = o.random.state;
        o.aiBridgeSearch.shape(0, 0, bytes); assert.ok(get(bytes, 0xb8) > 0); assert.equal(o.random.state, seed); assert.ok(bytes.subarray(0, 64).some((value) => value !== 0));
    } finally { o.dispose(); }
});

test('Session AI 에너지/로그: 실제 평가 반환과 현재 장기 진단 DWORD·종류/본문을 보존한다', () => {
    const { o, trace } = owner();
    try {
        o.connectAiProduction(); const token = brain(o); trace.length = 0;
        o.energyState.ignore = true; o.playerState.special = true; const box = { text: 'unchanged' }; assert.equal(o.aiProductionModules.dryHooks.energy(box, 0, 0, 85, 2), 1); assert.equal(box.text, 'unchanged');
        o.gameplayGlobals.aiDebug().word(0x52f3d0).value = 17; o.gameplayGlobals.aiDebug().word(0x52f3dc).value = 18; o.gameplayGlobals.aiDebug().word(0x52f3e0).value = 19;
        o.aiProductionModules.collectorHooks.log(token, 85, -3, 7); o.aiProductionModules.dryHooks.log(token, true); o.aiProductionModules.factoryHooks.log(token, 1, 0);
        assert.deepEqual(trace, [['log', 17, `AI2: Lay ${o.core.types[85].name} at (-3,7)\n`], ['log', 18, 'AI2: Killed Fake Dry Spot\n\n'], ['log', 19, 'AI2: Upgrade factory!\n\n']]);
    } finally { o.dispose(); }
});
