// InstallContents 마지막 Factory 연구/업그레이드/입력의 실제 부모와 현재 상태를 대조한다(WB04-E599).
// 보호 자산·실제 기존 모듈·명시 소비자 계약이다. 미션/실제 생산·공격·포획 관측은 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawFactoryResearch } from '../www/core/raw-factory-research.js';
import { RawFactoryUpgrade } from '../www/core/raw-factory-upgrade.js';
import { RawFactoryInput } from '../www/core/raw-factory-input.js';
import { RawFactoryUpgradeCommand } from '../www/core/raw-factory-upgrade-command.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';

// 미선택 외부 효과를 빈 성공으로 바꾸지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 실제 보호 자산·팔레트와 같은 기반 owner다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 서비스 경계만 명시 입력으로 공급하며 같은 실제 World/하위 몸체를 사용한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 원문 생성 순서로 생산까지 연결한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); }
// Factory 구성까지 연결한다. 전체 World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectFactories(); return o; }
// 실제 Factory 생성자 또는 지정 TYPE의 원문 슬롯을 입력으로 만든다. 세계에 Pop하지 않는다.
function actor(o, type = 85) { return o.world.create(type, 0); }

test('Session Factory: 선행/반복/반납 관문과 실제 네 모듈·미공개를 유지한다', () => {
    const o = owner(); try { assert.throws(() => o.connectFactories(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); o.connectFactories(); assert.equal(o.factoryModules.ready, true); assert.ok(o.factoryResearch instanceof RawFactoryResearch); assert.ok(o.factoryUpgradeCommand instanceof RawFactoryUpgradeCommand); assert.ok(o.factoryUpgrade instanceof RawFactoryUpgrade); assert.ok(o.factoryInput instanceof RawFactoryInput); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.factoryUpgradePending, null); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); assert.throws(() => o.connectFactories(), /반복/); } finally { o.dispose(); }
    assert.throws(() => o.connectFactories(), /반납/); assert.throws(() => o.factoryModules.commandHooks.reserveShared(), /반납/);
});

test('Session Factory: 상태는 현재 로딩/dirty·권한/시각·Outpost/의존 TYPE와 editor를 공유한다', () => {
    const o = scene(); try { const m = o.factoryModules; o.objectState.loadingDepth = 3; o.state.assetState.writeBoss(false); o.core.processState.now = 7.25; o.state.gunState.checking = -2; o.gameplayGlobals.outpostType().value = 125; o.flyerDependType = 2; o.playerState.battleOwner = 6; o.globals.editor = true; o.core.ownerMode.fort = false;
        m.researchState.productionDirty.value = true; assert.equal(o.core.bookkeeping.productionDirty, true); assert.deepEqual([m.researchState.loadingDepth(), m.researchState.boss(), m.researchState.now(), m.researchState.outpostType(), m.commandState.checking(), m.upgradeState.outpost(), m.upgradeState.dependType(), m.upgradeState.localPlayer(), m.inputState.editor(), m.inputState.localOwner()], [3, false, 7.25, 125, 4294967294, 125, 2, 6, true, 6]);
    } finally { o.dispose(); }
});

test('Session Factory: 연구 도움말/슬롯 로그/송신은 현재 서비스와 signed32 채널을 사용한다', () => {
    const calls = []; const o = scene({ factoryHelp: (...args) => calls.push(['help', ...args]), player: { log: (...args) => calls.push(['log', ...args]) }, carrierRequest: (...args) => calls.push(['send', ...args]) });
    try { const sid = actor(o); o.core.pool.physical(sid)[34] = 4; const h = o.factoryModules.researchHooks; const packet = Uint8Array.of(1, 2); h.help('section', 7); h.inputLog(sid, -2); h.send(0xffffffff, packet); o.services.carrierRequest = (...args) => calls.push(['next', ...args]); h.send(3, packet); assert.deepEqual(calls, [['help', 'section', 7], ['log', 5, '4: slotsRemaining/A=-2\n'], ['send', -1, packet], ['next', 3, packet]]); o.services.factoryHelp = null; assert.throws(() => h.help('x', 1), /도움말 미연결/); } finally { o.dispose(); }
});

test('Session Factory: 연구 생성/검색은 같은 내용물 부모이며 슬롯/Refresh는 현재 생산으로 간다', () => {
    const o = scene(); try { const root = actor(o, 75); put(o.core.pool.physical(root), 0, 0x50ee08); const h = o.factoryModules.researchHooks; const form = h.create(root, 94, 3, 8, 0x10, 4); assert.equal(h.find(root, 8, 94, 0), form); const calls = []; o.production.appendFactoryProducts = (...args) => { calls.push(['slots', ...args]); return -7; }; o.production.refresh = (...args) => calls.push(['refresh', ...args]); assert.equal(h.slots(root), -7); assert.equal(h.refresh(6), undefined); assert.deepEqual(calls, [['slots', root, 0], ['refresh', 6]]); } finally { o.dispose(); }
});

test('Session Factory: SharedRegular는 같은 실제 예약 객체를 한 번 부착한 뒤 현재 시각으로 ResetTask한다', () => {
    const o = scene(); try { const root = actor(o); const h = o.factoryModules.commandHooks; assert.throws(() => h.addShared(root, 3, 2.5), /예약 누락/); assert.equal(h.reserveShared(), true); const reserved = o.factoryUpgradePending; assert.ok(reserved instanceof RegularProcess); o.core.processState.now = 19.75; h.addShared(root, 3, 2.5); assert.equal(o.factoryUpgradePending, null); assert.equal(o.core.processes.findEvent(root, 3), reserved); assert.equal(reserved.shared, true); assert.equal(reserved.time, 19.75); assert.equal(reserved.payload, 2.5); assert.throws(() => h.addShared(root, 3, 2.5), /예약 누락/); } finally { o.dispose(); }
});

test('Session Factory: 명령 비용/입력 SP와 실제 AddSp는 같은 현재 HP/SP를 사용한다', () => {
    const calls = []; const o = scene({ refreshSp: (value) => calls.push(value) }); try { const m = o.factoryModules; assert.equal(m.commandHooks.cost(85), o.hp.typeCostUnits(85)); o.sp.set(7, floatToBits(1000)); assert.equal(m.inputHooks.money(7), 1000); m.commandHooks.addSp(7, -250); assert.equal(bitsToFloat(o.sp.get(7)), 750); assert.equal(m.inputHooks.money(7), 750); } finally { o.dispose(); }
});

test('Session Factory: Level 현재VT 관문과 같은 표시/HP 부모를 사용한다', () => {
    const o = scene(); try { const sid = actor(o); const h = o.factoryModules.upgradeHooks; assert.equal(h.level(sid), o.assetModules.virtualCount(sid)); assert.equal(h.maxHp(sid), o.hp.maxHitPoints(sid)); const other = actor(o, 75); assert.throws(() => h.level(other), /level 가상표 미복원/); const display = o.core.display.update; let actual; o.core.display.update = (raw) => { actual = raw; }; h.display(sid); assert.equal(actual.buffer, o.core.pool.physical(sid).buffer); assert.equal(actual.byteOffset, o.core.pool.physical(sid).byteOffset); o.core.display.update = display; } finally { o.dispose(); }
});

test('Session Factory: 후설치 Flare/수신과 건설 준비의 현재 포인터·앞선 효과를 유지한다', () => {
    const o = scene(); try { const m = o.factoryModules; assert.throws(() => m.researchHooks.flare(1, 2, 3), /플레어 미설치/); const calls = []; o.flare = { spawn: (...args) => calls.push(['flare', ...args]) }; m.researchHooks.flare(4, 5, 6); m.upgradeHooks.flare(7, 8, 9); const receive = o.factoryUpgradeCommand; o.factoryUpgradeCommand = null; assert.throws(() => m.researchHooks.upgradeReceive(Uint8Array.of()), /수신 미설치/); o.factoryUpgradeCommand = { receive: (...args) => calls.push(['receive', ...args]) }; const packet = Uint8Array.of(1); m.researchHooks.upgradeReceive(packet); o.factoryUpgradeCommand = receive;
        o.core.processState.now = 10.5; o.playerState.battleOwner = 6; assert.throws(() => m.upgradeHooks.construction(1, 2), /실제 건설 미설치/); assert.equal(o.state.buildingState.now, 10.5); assert.equal(o.state.buildingState.localPlayer, 6); assert.deepEqual(calls, [['flare', 4, 5, 6], ['flare', 7, 8, 9], ['receive', packet]]);
    } finally { o.dispose(); }
});

test('Session Factory: AI는 현재 owner BYTE와 Player+b0 DWORD·도전/부호 관문을 사용한다', () => {
    const o = scene(); try { const sid = actor(o); const h = o.factoryModules.upgradeHooks; const raw = o.core.pool.physical(sid); raw[34] = 0; assert.equal(h.aiPresent(sid), false); raw[34] = 9; assert.throws(() => h.aiPresent(sid), /현재AI Player 범위/); raw[34] = 2; assert.equal(h.aiPresent(sid), false); put(o.players, 2 * 180 + 0xb0, 0xfedcba98); assert.equal(h.aiPresent(sid), true);
        h.notifyAi(0, sid); h.notifyAi(0xffffffff, sid); o.core.ownerMode.challenge = true; h.notifyAi(9, sid); o.core.ownerMode.challenge = false; assert.throws(() => h.notifyAi(9, sid), /완료AI Player 범위/); assert.throws(() => h.notifyAi(2, sid), /실제 AI 생산 미설치/); const calls = []; o.aiFactoryProduction = { upgraded: (...args) => calls.push(args) }; h.notifyAi(2, sid); assert.deepEqual(calls, [[0xfedcba98, sid]]);
    } finally { o.dispose(); }
});

test('Session Factory: 전역/위치 소리는 현재 이름표와 원문6/5인자를 유지한다', () => {
    const calls = []; const o = scene(); try { const h = o.factoryModules.upgradeHooks; o.services.sound.play = (...args) => calls.push(['global', ...args]); o.services.sound.playOnceAt = (...args) => calls.push(['at', ...args]); const sound = o.soundNames.lookup('priestFree.wav'); h.sound('priestFree.wav'); h.soundAt(1.5, 2.5, 'priestFree.wav'); assert.deepEqual(calls, [['global', sound, 0, 0, 0, 1, 0], ['at', 1.5, 2.5, sound, 0, 0]]); } finally { o.dispose(); }
});

test('Session Factory: 입력은 현재 연구/요청/공통 객체로 위임하고 packed 인자는 공통 부모에 넘기지 않는다', () => {
    const calls = []; const o = scene(); try { const h = o.factoryModules.inputHooks; assert.throws(() => h.parent(1, 2, 3), /공통 입력 미설치/); o.objectCommand = { command: (...args) => { calls.push(['parent', ...args]); return 123; } }; assert.equal(o.factoryInput.input(1, 0x00123456, 0xabcdef01), 123); o.factoryResearch.input = (...args) => { calls.push(['research', ...args]); return 456; }; assert.equal(h.research(2, 3, 4), 456); o.factoryUpgradeCommand.request = (...args) => calls.push(['request', ...args]); h.request(5); assert.deepEqual(calls, [['parent', 1, 0x00123456], ['research', 2, 3, 4], ['request', 5]]); } finally { o.dispose(); }
});

test('Session Factory: 미부착 SharedRegular 예약은 부모 반납에서 해제한다', () => {
    const o = scene(); o.factoryModules.commandHooks.reserveShared(); const reserved = o.factoryUpgradePending; o.dispose(); assert.equal(o.factoryUpgradePending, null); assert.equal(reserved.form, 0); assert.equal(reserved.pid, 0);
});
