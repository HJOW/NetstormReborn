// ConnectConstruction의 실제 World/목록/기본 wire와 현재 후설치 소비자 계약이다(WB04-E603).
// 실제 미션 Load/시작·원본 전용 관측·사제 포획/희생/공중3종 행동 검사는 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawCampaignConstructionBindings } from '../www/core/raw-campaign-construction-bindings.js';
import { ClientBuildReplayContext, ClientBuildReplayGlobals } from '../www/core/client-build-replay.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put } from './raw-support.mjs';

// 선택하지 않은 외부 서비스는 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 같은 보호 자산과 팔레트의 실제 게임 자료다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 실제 하위 모듈을 구성하고 외부 장치만 명시 입력으로 공급한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 원문 순서로 Flare까지 구성한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); }
// 전체 World를 게시하지 않은 실제 typed 건설 장면이다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectConstruction(); return o; }

test('Session Construction: 선행/반복/반납 관문과 같은 실제 Pool/무효과 생성/미공개', () => {
    const o = owner(); try { assert.throws(() => o.connectConstruction(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectConstruction(); assert.equal(o.constructionModules.ready, true); assert.ok(o.construction instanceof RawCampaignConstructionBindings); assert.equal(o.construction.pool, o.core.pool); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectConstruction(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectConstruction(), /반납/); assert.throws(() => o.constructionModules.gameplay.notice.findPriest(1), /반납/);
});

test('Session Construction: 최근5칸/방향9DWORD는 장기 물리이고 결선/준비 대여/반납이 꼬리를 지우지 않는다', () => {
    const o = owner(); before(o); const storage = o.world.storage; const recent = storage.recentGeysers; const entries = recent.entries; const directions = storage.geyserPlayerDirections;
    assert.deepEqual([...directions], [16448, 4, 0, 6, 2, 7, 3, 5, 1]); assert.equal(recent.count, 0); assert.deepEqual(entries, [0, 0, 0, 0, 0]); assert.notEqual(recent, o.state.assetState.currentGeysers());
    recent.count = 2; entries[4] = 0xdeadbeef; directions[8] = 0x80000001; o.connectConstruction(); assert.equal(recent.count, 2); assert.equal(entries[4], 0xdeadbeef); o.dispose();
    const prepared = storage.acquirePrepared(); assert.equal(storage.recentGeysers, recent); assert.equal(storage.recentGeysers.entries, entries); assert.equal(recent.count, 2); assert.equal(entries[4], 0xdeadbeef); prepared.release();
    const fresh = storage.acquire(); assert.equal(storage.recentGeysers.entries, entries); assert.equal(recent.count, 0); assert.deepEqual(entries, [0, 0, 0, 0, 0]); assert.equal(storage.geyserPlayerDirections, directions); assert.equal(directions[8], 0x80000001); fresh.release();
});

test('Session Construction: PlayerBounds는 같은 현재 옵션BYTE/미션/패턴TYPE/방향표를 조회한다', () => {
    const o = scene(); try { const state = o.constructionModules.boundsState; o.battleOptions[6] = 3; o.piece = 111; o.objectState.mission = false; o.core.ownerMode.challenge = true; assert.equal(state.mapSize, 3); assert.equal(state.puzzlePieceType, 111); assert.equal(state.mission, false); assert.equal(state.metamap, true); assert.equal(state.playerDirections, o.world.storage.geyserPlayerDirections); assert.equal(state.decoderDiagnostics, o.metadata.geometry.decoderDiagnostics); o.battleOptions[6] = 1; assert.equal(state.mapSize, 1); } finally { o.dispose(); }
});

test('Session Construction: 서비스 값 사본과 실제 섬 조회/미선택 외부 장치 오류를 보존한다', () => {
    const callback = () => 123; const presentation = { sendMoney: callback }; const construction = { configInt: callback, presentation }; const o = owner({ construction }); construction.configInt = forbidden; presentation.sendMoney = forbidden;
    try { before(o); o.connectConstruction(); const effects = o.constructionModules.effects; assert.equal(effects.configInt, callback); assert.equal(effects.presentation.sendMoney, callback); assert.throws(() => effects.wallSeconds(), /실제 외부 장치 미연결/); const exists = o.islands.exists; o.islands.exists = (region) => region + 9; assert.equal(effects.islandExists(4), true); o.islands.exists = exists; } finally { o.dispose(); }
});

test('Session Construction: 환불/취소는 설치 관문 뒤 현재 Login에 실제 wire를 한 번씩 보낸다', () => {
    const o = scene(); try { const p = o.constructionModules.effects.presentation; const money = { player: 2, source: 3, kind: 1, amount: -17 }; assert.throws(() => p.sendMoney(2, money), /설치 전/); o.loaded = true; assert.throws(() => p.sendMoney(2, money), /로컬 전송 없는/); const calls = []; o.login = { sendMutable: (target, packet) => calls.push([target, packet]) }; p.sendMoney(0xffffffff, money); p.sendReject(4, { x: -0, y: 2.5, type: 100, argument: 7, direction: 2 }); assert.equal(calls.length, 2); assert.equal(calls[0][0], -1); assert.deepEqual([...calls[0][1]], [1, 0x6c, 11, 0, 2, 3, 1, 239, 255, 255, 255]); assert.equal(calls[1][0], 4); assert.equal(new DataView(calls[1][1].buffer).getUint32(4, true), 0x80000000); assert.equal(calls[1][1][1], 0x2f); } finally { o.dispose(); }
});

test('Session Construction: RemoteTell은 Player 순서/현재 Login/같은 패킷의 앞선 observer 변경을 보존한다', () => {
    const o = scene(); try { const p = o.constructionModules.effects.presentation; const targets = []; const packets = []; o.loaded = true;
        // 첫 송신이 현재 Login을 교체하고 다음 송신은 새 실제 장치를 조회한다.
        o.login = { sendMutable: (target, packet) => { targets.push(target); packets.push(packet); packet[0] = 9; o.login = { sendMutable: (next, same) => { assert.equal(same[0], 9); targets.push(next); packets.push(same); } }; } };
        p.tellOthers(3, 'Section'); assert.deepEqual(targets, [1, 2, 4, 5, 6, 7, 8]); assert.ok(packets.every((packet) => packet === packets[0])); assert.equal(packets[0][1], 0x70); assert.equal(packets[0].length, 264); assert.equal(new DataView(packets[0].buffer).getUint32(4, true), 8); targets.length = 0; o.login = { sendMutable: (target) => targets.push(target) }; p.tellPlayer(0xffffffff, ''); assert.deepEqual(targets, [-1]);
    } finally { o.dispose(); }
});

test('Session Construction: 기본 비용 서식/로컬 Tell은 웹 표시 해독을 마지막 경계에서 사용한다', () => {
    const calls = []; const o = scene({ factoryTell: (text) => calls.push(text) }); try { const p = o.constructionModules.effects.presentation; assert.equal(p.format('Player %s!', 'Name'), 'Player Name!'); p.tell('Cost \x80'); assert.deepEqual(calls, ['Cost €']); o.services.factoryTell = null; assert.throws(() => p.tell(''), /현재Tell 미연결/); } finally { o.dispose(); }
});

test('Session Construction: 기록 관문은 같은 playing DWORD와 매 호출 현재 weak context를 읽는다', () => {
    const replay = new ClientBuildReplayGlobals(); const first = new ClientBuildReplayContext(); const second = new ClientBuildReplayContext(); const calls = []; first.state.events[0].x = 10; first.state.events[0].y = 11; first.state.events[0].name.set([76, 58, 65, 0]); second.state.events[0].x = 20; second.state.events[0].y = 21; second.state.events[0].name.set([76, 58, 66, 0]);
    first.hooks = { validPoint: () => false, label: (name, sid) => calls.push([name, sid]) }; second.hooks = { validPoint: () => true, label: (name, sid) => calls.push([name, sid]) }; replay.current = new WeakRef(first); const o = scene({ buildReplay: replay });
    try { const s = o.state.constructionState.confirm; replay.playing = 0x80000000; assert.equal(s.recorderEnabledNow(), true); const h = o.constructionModules.gameplay.confirm; assert.equal(h.recorderActive(), true); h.record(50); replay.current = new WeakRef(second); assert.equal(h.recorderActive(), false); second.hooks.validPoint = () => false; h.record(51); assert.deepEqual(calls, [['A', 50], ['B', 51]]); second.dispose(); assert.throws(() => h.recorderActive(), /호스트 수명 종료/); } finally { o.dispose(); }
});

test('Session Construction: 건물 효과/Builder/시작은 후설치한 현재 부모와 효과 직전 현재 시각을 사용한다', () => {
    const o = scene(); try { const h = o.constructionModules.gameplay; assert.throws(() => h.request.displace(1, 2, 3, 4), /건설 효과 미설치/); assert.throws(() => h.confirm.findBuilder(1, 2, 3, 4), /기준점 미설치/); const calls = []; o.core.processState.now = 17; o.playerState.battleOwner = 3;
        o.buildingEffects = { displace: (...args) => { assert.equal(o.state.buildingState.now, 17); calls.push(['displace', ...args]); return false; }, dropIsland: (...args) => calls.push(['drop', ...args]) }; o.buildingMover = { move: (...args) => calls.push(['move', ...args]) }; o.building = { start: (...args) => calls.push(['start', ...args]) }; o.anchor = { locate: (...args) => { calls.push(['locate', ...args]); return o.core.pool.capacity; } };
        assert.equal(h.request.displace(1, 2, 3, 4), false); assert.equal(o.state.buildingState.localPlayer, 3); assert.equal(o.state.buildingEffectsState.alliances, o.alliances); h.request.cancel(2, 3, 8, 4); h.notice.moveBuilder(1, 50, 0x1122, 0x3344); h.notice.start(51, 0x3f800000); assert.throws(() => h.confirm.findBuilder(1, 2, 3, 4), /SID 범위/); o.anchor.locate = () => 0; assert.equal(h.confirm.findBuilder(1, 2, 3, 4), 0); assert.deepEqual(calls.slice(0, 4), [['displace', 1, 2, 3, 4], ['drop', 2, 3, 8, 4], ['move', 1, 50, 0x1122, 0x3344], ['start', 51, 1]]);
    } finally { o.dispose(); }
});

test('Session Construction: 접근점은 현재 Footprint/Graph와 checking을 쓰고 이동은 원문8인자를 전달한다', () => {
    const o = scene(); try { const h = o.constructionModules.gameplay.notice; assert.throws(() => h.approach(50, 10, 11, 0), /Graph 미설치/); const original = o.gunTarget.footprint; const calls = []; o.gunTarget.footprint = (sid, extended) => { calls.push(['footprint', sid, extended]); return { left: 20, top: 20, right: 22, bottom: 22 }; }; o.anchor = { graphAt: (x, y) => { calls.push(['graph', x, y]); return 7; } }; const point = h.approach(50, 10, 21, 0); assert.deepEqual(point, { x: 20, y: 21 }); assert.deepEqual(calls[0], ['footprint', 50, false]); assert.throws(() => h.tryMove(50, 10, 11, 60), /실제54/); o.motion = { paths: { startPriest: (value) => { calls.push(value); return null; } } }; assert.equal(h.tryMove(50, 10, 11, 60), false); assert.deepEqual(calls.at(-1), { parent: 50, destination: { x: 10, y: 11 }, replaceStart: 0, helper: 60, tailWord: 0, tailByte: 0, reserved: 0 }); o.gunTarget.footprint = original; } finally { o.dispose(); }
});

test('Session Construction: MayPlace는 같은 현재 Player/동맹/Graph/섬100기록을 전체 부모에 전달한다', () => {
    const o = scene(); try { const h = o.constructionModules.gameplay.request, anchor = o.state.gameplayPlacement.anchor; assert.throws(() => h.mayPlace({}), /전체 MayPlace 미설치/); const query = { type: 100, owner: 1 }; o.playerState.battleOwner = 0xffffffff; o.state.anchorState.graphReady = true; o.globals.editor = true; o.state.buildingState.pushGenus = 0x100; put(o.islandBytes, 4, 0x80000001); put(o.islandBytes, 8, 3); let seen = 0;
        o.gameplayPipeline = { mayPlace: (current) => { seen += 1; assert.equal(current, query); assert.equal(o.state.gameplayPlacement.placement.localPlayer, -1); assert.equal(o.state.gameplayPlacement.permission.alliances, o.alliances); assert.equal(o.state.gameplayPlacement.anchor, anchor); assert.notEqual(anchor, o.state.anchorState); assert.equal(anchor.graphReady, o.state.anchorState.graphReady); assert.equal(o.state.gameplayPlacement.collision.ignoredGenus, 0x100); assert.deepEqual(o.gameplayRegions[0], { restricted: 0x80000001, exists: 3 }); return false; } }; assert.equal(h.mayPlace(query), false); assert.equal(seen, 1);
    } finally { o.dispose(); }
});
