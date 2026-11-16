// Session1538..1610의 실제 자산 결선과 원본83VT 정적 자료를 대조한다(WB04-E590).
// 전체 미션/자연 완주·실제 포획/희생·공중3종 전용 생성/행동 검사는 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { readRows, get, put, floatToBits } from './raw-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { typeString } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawCampaignAssetBindings } from '../www/core/raw-campaign-asset-bindings.js';
import { COMMON_DAMAGEABLE_REGULAR_TABLES as COMMON, GENERAL_DAMAGEABLE_HIT_TABLES as GENERAL, BASE_DAMAGEABLE_HIT_TABLES as BASE } from '../www/core/campaign-damageable-regular-tables.js';

// 같은 실제 보호 자산의 TYPE/SHP/COL을 빌린다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 선택하지 않은 외부 효과를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택되지 않은 외부 효과'); }
// 필수 시각/음향과 실제 함수 사본의 외부 입력만 공급한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 같은 실제 owner의 앞 생성자 구간을 원문 순서로 조립한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); }
// 이번 실제 자산 등록까지 조립하며 외곽 Session/Load는 공개하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectAssets(); return o; }
// 현재 생성자의 실제 슬롯에 고정 입력 HP/좌표/owner만 설치한다. 미션 Pop/교전 결과를 쓰지 않는다.
function actor(o, type = 75, owner = 2, hp = 10000) { const sid = o.world.create(type, 2); const raw = o.core.pool.allocated(sid); raw[34] = owner; put(raw, 26, hp); put(raw, 14, floatToBits(21)); put(raw, 18, floatToBits(21)); return sid; }

test('Session 자산 표: 원본83VT의 +5c/+98 정적 데이터와 공통13·일반31·기본45 범위가 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/damageable-regular-tables-x86.tsv')); assert.ok(rows); assert.equal(rows.length, 83); assert.deepEqual([COMMON.length, GENERAL.length, BASE.length], [13, 31, 45]);
    // 기대 주소는 원본 PE 추출 자료에서만 읽는다. 함수 실행 수를 늘리지 않는다.
    for (const row of rows) { const [table, , hit, common] = row.map(Number); assert.equal(COMMON.includes(table), common === 1); assert.equal(GENERAL.includes(table), hit === 0x44c8e0); assert.equal(BASE.includes(table), hit === 0x4add10); }
    assert.ok([COMMON, GENERAL, BASE].every(Object.isFrozen)); assert.equal(GENERAL.includes(0x50ea08), false); assert.equal(BASE.includes(0x50ea08), false);
});

test('Session 자산: 선행/한 번 결선·같은 상태/난수·실제 PostPop 등록과 반납을 보존한다', () => {
    const o = owner(); const calls = [];
    try { assert.throws(() => o.connectAssets(), /초기화 전/); before(o); const seed = o.random.state; const set = o.core.postPop.setBody.bind(o.core.postPop); o.core.postPop.setBody = (kind, body) => { calls.push([kind, body]); set(kind, body); }; o.connectAssets(); assert.ok(o.assetBindings instanceof RawCampaignAssetBindings); assert.equal(o.assetBindings.postPop.pool, o.core.pool); assert.equal(o.assetModules.ready, true); assert.equal(o.random.state, seed); assert.deepEqual(calls.map(([kind, body]) => [kind, !!body]), [['asset', true], ['factory', true]]); assert.throws(() => o.connectAssets(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.deepEqual(calls.filter(([kind]) => kind === 'asset' || kind === 'factory').slice(-2), [['asset', null], ['factory', null]]); assert.equal(o.core.kernel.size, 0); assert.throws(() => o.connectAssets(), /반납/); assert.throws(() => o.assetModules.virtualCount(1), /반납/);
});

test('Session 자산: 네 서비스 표를 값으로 복사하고 사용자 지정 실제 피해/지면 효과를 보존한다', () => {
    const hit = () => 17; const ground = () => 23; const warn = () => 31; const assets = { postPop: { reserveRegular: () => false }, regular: { hitPoints: hit, damageGround: ground }, factory: { warnDuplicate: warn }, gun: { marker: 7 } }; const o = owner({ assets });
    try { assets.regular.hitPoints = forbidden; assets.factory.warnDuplicate = forbidden; assets.gun.marker = 9; before(o); o.connectAssets(); const e = o.assetModules.effects; assert.equal(e.regular.hitPoints, hit); assert.equal(o.developerDamage, hit); assert.equal(o.decorationHit, hit); assert.equal(e.regular.damageGround, ground); assert.equal(e.factory.warnDuplicate, warn); assert.equal(e.gun.marker, 7); assert.equal(e.postPop.reserveRegular(), true); assert.notEqual(e.regular, o.services.assets.regular); assert.equal(o.services.assets.postPop.reserveRegular(), false); } finally { o.dispose(); }
});

test('Session 자산 실패: 효과/공유 피해 사본 뒤 다른 Carrier Pool은 실제 등록 전에 거부한다', () => {
    const o = owner(); const other = owner();
    try { before(o); before(other); o.carrier = other.carrier; assert.throws(() => o.connectAssets(), /Core\/TYPE\/Player 저장소 불일치/); assert.ok(o.assetModules.effects); assert.equal(o.decorationHit, o.developerDamage); assert.equal(o.assetBindings, null); assert.equal(o.assetModules.ready, false); assert.throws(() => o.connectAssets(), /반복/); } finally { o.dispose(); other.dispose(); }
});

test('Session 자산 가상80: 현재 VT/TYPE·레벨과 의식 설치/재귀·예외 뒤 활성 사슬 정리를 보존한다', () => {
    const o = scene();
    try { const sid = actor(o); o.core.pool.allocated(sid)[11] = 3 << 5; assert.equal(o.assetModules.virtualCount(sid), 3); assert.equal(o.rewardCount(sid), 3); const altar = actor(o, 164); assert.throws(() => o.assetModules.virtualCount(altar), /제단.*설치 전/); o.altarSacrifice = { count: (value) => o.assetModules.virtualCount(value) }; assert.throws(() => o.assetModules.virtualCount(altar), /재귀 순환/); o.altarSacrifice = { count: () => 0xffffffff }; assert.equal(o.assetModules.virtualCount(altar), 0xffffffff); assert.equal(o.rewardCount(altar), -1); const dais = actor(o, 165); assert.throws(() => o.assetModules.virtualCount(dais), /다이스.*설치 전/); put(o.core.pool.allocated(dais), 0, 0x501dd0); o.core.pool.allocated(dais)[11] = 4 << 5; assert.equal(o.assetModules.virtualCount(dais), 4); assert.throws(() => o.assetModules.virtualCount(0), /SID 범위/); put(o.core.pool.allocated(sid), 0, 0); assert.throws(() => o.assetModules.virtualCount(sid), /VT 미연결/); o.core.pool.allocated(sid)[10] = 188; assert.throws(() => o.assetModules.virtualCount(sid), /TYPE 범위/); } finally { o.dispose(); }
});

test('Session 자산 피해: 같은 실제 일반/기본 HP와 불워크 공격자0 관문을 사용한다', () => {
    const o = scene();
    try { const general = actor(o); assert.equal(o.developerDamage(general, 7, 3, 0), 1); assert.equal(get(o.core.pool.slot(general), 26), 9993); const base = actor(o); put(o.core.pool.allocated(base), 0, 0x501dd0); assert.equal(o.developerDamage(base, 9, 3, 0), 1); assert.equal(get(o.core.pool.slot(base), 26), 9991); const bulwark = actor(o, 79); assert.equal(get(o.core.pool.slot(bulwark), 0), 0x50ea08); assert.equal(o.developerDamage(bulwark, 151, 1, 0), 1); assert.equal(get(o.core.pool.slot(bulwark), 26), 9849); } finally { o.dispose(); }
});

test('Session 자산 피해: 공격자 폭을 먼저 거부하고 미복원 가상98을 넓은 다른 피격기로 우회하지 않는다', () => {
    const o = scene();
    try { assert.throws(() => o.developerDamage(999999, 1, 1, 65536), /공격자 SID 범위/); const altar = actor(o, 164); assert.throws(() => o.developerDamage(altar, 1, 1, 0), /미복원 TYPE164/); assert.equal(get(o.core.pool.slot(altar), 26), 10000); const priest = actor(o, 158); o.globals.missionPost = 1; assert.equal(o.developerDamage(priest, 3, 3, 0), 1); assert.equal(get(o.core.pool.slot(priest), 26), 9997); } finally { o.dispose(); }
});

test('Session 자산 신전 피해: 현재 건설 Form 없음은 자연 반환하고 발견 후에만 실제 건설 장치를 요구한다', () => {
    const o = scene();
    try { const temple = actor(o, 123); o.core.pool.allocated(temple)[40] |= 1; assert.equal(o.developerDamage(temple, 7, 3, 0), 1); assert.equal(get(o.core.pool.slot(temple), 26), 10000); const process = o.core.processes.addRegular(temple, 3, 0); o.gameplayGlobals.gameAux().constructionProcessType = o.core.pool.slot(process.form)[10]; assert.throws(() => o.developerDamage(temple, 7, 3, 0), /건설 연결 누락/); assert.equal(get(o.core.pool.slot(temple), 26), 10000); } finally { o.dispose(); }
});

test('Session 자산 거주 구역: 현재 Fort/Dude/공급원 수 관문 뒤 실제 입구와 누락 생성기/작업기를 선택한다', () => {
    const o = scene();
    try { const e = o.assetModules.effects; o.core.ownerMode.fort = false; assert.equal(e.regular.residenceEvent2(0), undefined); o.core.ownerMode.fort = true; o.dudePopulation.setCount(0); o.core.bookkeeping.currentProviders().setCount(0); assert.equal(e.regular.residenceEvent2(0), undefined); o.core.bookkeeping.currentProviders().setCount(1); const residence = actor(o, 89); assert.deepEqual(o.assetModules.residence.entrance(residence), o.hits.dudes.residencePoint(residence)); assert.throws(() => e.regular.residenceEvent2(residence), /Dude 생성기 미설치/); assert.throws(() => o.assetModules.residence.task(residence, 0x5102, 0), /Dude 작업기 미설치/); } finally { o.dispose(); }
});

test('Session 자산 신전 Regular: missionPost 대신 현재 mission·섬 순회·이탈 로그/이름/예약 WORD를 연결한다', () => {
    const calls = []; const o = scene({ player: { log: (...args) => calls.push(args) }, priestTakenName: (owner, name) => { calls.push([owner, name]); return 'Player2'; } });
    try { const e = o.assetModules.effects.regular; o.core.bridgeState.mission = false; o.globals.missionPost = 1; assert.equal(e.suppressLeaveRequest(), false); o.core.bridgeState.mission = true; assert.equal(e.suppressLeaveRequest(), true); o.islands.reset(); o.islands.add(2, 0); o.islands.add(3, 1); assert.equal(e.countForeignOwnedIslands(1), 2); e.playerLeavingNotice(2); assert.deepEqual(calls, [[2, ''], [29, 'Player Player2 is leaving!\n']]); const sid = actor(o); e.state(sid, 0x12345); assert.equal(get(o.core.pool.slot(sid), 12, 2), 0x2345); assert.equal(o.assetModules.vortexRuntime.leave, e.requestLeave); assert.equal(o.assetModules.vortexRuntime.descend, e.descend); } finally { o.dispose(); }
});

test('Session 자산 후처리: 현재 impactSound/Graph/청크 BYTE와 선택되지 않은 실제 장치 오류를 유지한다', () => {
    const calls = []; const o = scene({ ai: { assertion: (expression) => calls.push(expression) } });
    try { const h = o.assetModules.effects.postPop; const sid = actor(o, 75); const name = typeString(game.types()[5].definition, 'impactSound'); assert.equal(h.typeDword(sid, 0x74), name ? o.soundNames.lookup(name) : 0); assert.throws(() => h.typeDword(sid, 0x70), /필드 미지원/); assert.equal(h.gunFrame(65), 3); o.chunks.at(1, 1).island = 255; assert.equal(h.regionNumber(31.9, 16.5), 255); assert.throws(() => h.nuggetPostPop(sid, 1), /너겟 PostPop 미설치/); assert.throws(() => h.growingIcePostPop(sid, 1), /아이스.*설치 전/); assert.throws(() => h.createSlot(sid, 70), /내용물 생성기 미설치/);  } finally { o.dispose(); }
});

test('Session 자산 Factory: 현재 두 타이머/제외 TYPE와 늦은 건설/업그레이드·플레어 경계를 공유한다', () => {
    const o = scene();
    try { const e = o.assetModules.effects.regular; const aux = o.gameplayGlobals.gameAux(); aux.firstTimerFactoryType = 0x12345678; aux.secondTimerFactoryType = 44; aux.outpostType = 46; assert.deepEqual([o.assetModules.factoryTypes.readFirstTimer(), o.assetModules.factoryTypes.readSecondTimer(), o.assetModules.factoryTypes.readFixedFrame()], [0x12345678, 44, 46]); assert.throws(() => e.startConstruction(1, 3), /건설 시작.*미설치/); assert.throws(() => e.beginFactoryUpgrade(1, 3), /업그레이드 그림 미설치/); assert.throws(() => e.finishFactoryUpgrade(1), /업그레이드 완료 미설치/); assert.throws(() => e.factoryFlare(1, 0, 0), /플레어 미설치/); assert.equal(o.assetModules.factoryRuntime.begin, e.beginFactoryUpgrade); assert.throws(() => e.themeMode(), /themeMode/); } finally { o.dispose(); }
});

test('Session 자산 준비: 같은 현재 건설 시각/로컬 owner/동맹·패턴 주소를 실제 상태에 기록한다', () => {
    const o = scene();
    try { o.playerState.battleOwner = 7; o.playerState.playerKnown = 1; o.globals.missionPost = 1; o.core.ownerMode.fort = true; o.core.bridgeState.bridgeType = 81; o.core.bridgeState.noIslandType = 156; o.assetModules.prepareBuilding(31.75); assert.equal(o.state.buildingState.now, 31.75); assert.equal(o.state.buildingState.localPlayer, 7); assert.equal(o.state.buildingEffectsState.alliances, o.alliances); o.assetModules.prepareConstruction(); const s = o.state.constructionState; assert.deepEqual([s.place.localPlayer, s.notice.localPlayer, s.request.localPlayer, s.command.localPlayer], [7, 7, 7, 7]); assert.equal(s.place.battleShown, true); assert.equal(s.request.editor, true); assert.equal(s.place.chargeBlocked, true); assert.equal(s.command.chargeBlocked, true); assert.deepEqual([...s.confirm.patternTypes], [107, 81, 94, 156, 131, 129, 140, 142]); assert.equal(s.request.patternTypes, s.confirm.patternTypes); assert.equal(s.clear.patternTypes, s.confirm.patternTypes); assert.equal(s.place.patternTypes, s.confirm.patternTypes); } finally { o.dispose(); }
});

test('Session 자산 실제 분배: 등록된 자산 후처리가 같은 공통 장부 깊이로 복귀한다', () => {
    const o = scene();
    try { const sid = actor(o); const calls = []; const body = o.assetBindings.postPop; const post = body.postPop.bind(body); body.postPop = (parent, flags) => { calls.push([parent, flags]); post(parent, flags); }; ++o.core.bookkeeping.depth; o.core.postPop.postPopVirtual(sid, 0); assert.deepEqual(calls, [[sid, 0]]); assert.equal(o.core.bookkeeping.depth, 0); } finally { o.dispose(); }
});
