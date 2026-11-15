// RawCampaignSession의 실제 Impl 기반 소유 자료·Player/SP 생성 구간을 대조한다(WB04-E574).
// 보호 GameAssets/World/Player/Core/SP를 직접 사용한다. 전체 Session/Load·미션 화면 검사는 아직 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { TaffArchive } from '../www/assets/taff.js';
import { SoundList } from '../www/audio/sound-list.js';
import { CampaignSessionAssets, RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { ClientChatWindowGlobals } from '../www/core/client-chat-window-globals.js';
import { ClientConnectionActivityGlobals } from '../www/core/client-connection-activity-globals.js';
import { ClientCoreDataGlobals } from '../www/core/client-core-data-globals.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { ClientSpGlobals } from '../www/core/client-sp-globals.js';
import { ClientSubscriberGlobals } from '../www/core/client-subscriber-globals.js';
import { FortTypeConversionStorage } from '../www/core/fort-type-conversion-storage.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { ClientRandomGlobals, CrtRandom, GameRandom, GameRandomSeed } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { readData } from './helpers.mjs';
import { get } from './raw-support.mjs';

// 보호 자료를 한 번 읽되 TYPE/월드/Player는 각 장면에서 새로 소유한다.
const actual = realCaptureAssets(); const game = new GameAssets(actual.table, actual.database); const files = new TaffArchive(readData('netstorm.tarc'));
// 실제 시각 입력과 소리 이름표만 공급한다. 생성자가 소리를 재생하거나 VFS를 읽지 않는다.
function services(extra = {}) { return { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra }; }
// 같은 실제 자산·권한·보조 전역을 빌리는 장기 표면 owner를 만든다.
function storageFor(source = game, authority = new RawAuthorityState(), gameAux = new GameAuxState()) {
    const assets = new CampaignSessionAssets(game, files); const metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
    return new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source, authority, gameAux });
}
// 반납이 실제 세 대여를 풀었는지 새 대여로 확인하고 바로 반납한다.
function returned(gameplay, players, storage) {
    assert.equal(storage?.borrowed ?? false, false); const p = players.acquire(); p.release(); const g = gameplay.acquire(); g.release();
}

test('Session 자산: 보호188TYPE·116프레임과 현재 표/최초 비용표의 독립 소유', () => {
    const a = new CampaignSessionAssets(game, files); const b = new CampaignSessionAssets(game, files);
    assert.equal(a.game, game); assert.equal(a.files, files); assert.equal(a.types.length, 188); assert.equal(a.frames.length, 188);
    // 70 이전 내장 타입은 빈 프레임이며 실제 자산116블록은 원문 코드를 사용한다.
    for (let n = 0; n < 188; n += 1) assert.deepEqual(a.frames[n].codes, n < 70 || n >= 186 ? [] : game.typeTable.definitions[n - 70].clusters.map(({ code }) => code));
    a.types[71].cost = 123; a.types[71].minUsage[0] = 987; assert.notEqual(a.loadedTypes[71].cost, 123); assert.notEqual(a.loadedTypes[71].minUsage[0], 987); assert.notEqual(b.types[71].cost, 123);
    assert.throws(() => new CampaignSessionAssets({}, files), /GameAssets/);
});

test('Session 기반: 실제 단일 World와 같은 Template 깊이를 빌리며 Player/SP·Runtime을 앞당기지 않는다', () => {
    const o = new RawCampaignSessionOwners(game, files, 'thewarbegins', services());
    try {
        assert.equal(o.core.pool, o.world.storage.pool); assert.equal(o.core.types, o.world.storage.types); assert.equal(o.world.storage.borrowed, true);
        assert.equal(o.playerRecords.base(), 0); assert.equal(o.players.length, 0); assert.equal(o.sp.initialized, false); assert.equal(o.random.state, 0); assert.equal(o.particleCrt.state, 0); assert.equal(o.randomGlobals.gameMode, 0);
        assert.equal(o.headers, o.templateStorage.headers()); assert.equal(o.headers.length, 180); assert.equal(o.hp.pool, o.core.pool); assert.equal(o.hp.bookkeeping, o.core.bookkeeping);
        assert.equal(o.chunks, o.world.storage.chunks); assert.equal(o.islandBytes, o.world.storage.islands); assert.equal(o.dudePopulation, o.world.storage.sidLists.dudes); assert.equal(o.stamp, o.gameplayGlobals.terrainStamp());
        o.fullDeck = 0x12345678; o.revision = 19; o.loginEnd = 12.75; o.contentSavedGame = true;
        assert.deepEqual([o.fullDeck, o.revision, o.loginEnd, o.contentSavedGame], [0x12345678, 19, 12.75, true]); assert.equal(o.gameplayGlobals.fullDeck().value, 0x12345678); assert.equal(o.gameplayGlobals.mapRevision().value, 19); assert.equal(o.localStateGlobals.loginEnd().value, 12.75); assert.equal(o.localStateGlobals.savedGame().value, true);
        assert.equal(o.playersReady, false); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
        o.world.setLoadingDepth(7); assert.equal(o.templateStorage.loadingDepth, 7); assert.equal(o.objectState.loadingDepth, 7); o.objectState.scope = 13; assert.equal(o.templateStorage.serializationGlobals.scope, 13);
        assert.equal(o.playerState, o.gameplayGlobals.player); assert.equal(o.globals, o.gameplayGlobals.world); assert.equal(o.cursorDrop, o.gameplayGlobals.cursor); assert.equal(o.energyState, o.gameplayGlobals.energy); assert.equal(o.contentFinder, o.gameplayGlobals.finder);
        assert.equal(o.priestState.movement.priest, 158); assert.equal(o.objectState.mission, false); assert.equal(o.finishState.chaffMode, false);
    } finally { o.dispose(); }
    returned(o.gameplayGlobals, o.playerRecords, o.world.storage); o.dispose(); assert.throws(() => o.initializePlayersAndSp(), /반납/);
});

test('Session 공유 자료: 같은 장기 owner/옵션·금지 표와 멤버 역할은 복사 없이 현재 값을 사용한다', () => {
    const s = storageFor(); const gameplayGlobals = new ClientGameplayGlobals(null, s.authority, s.gameAux);
    const input = services({ gameplayGlobals, surfaceStorage: s, templateStorage: new CampaignTemplateStorage(), fortTypeConversion: new FortTypeConversionStorage(), chatGlobals: new ClientChatWindowGlobals(), localStateGlobals: new ClientLocalStateGlobals(), connectionActivityGlobals: new ClientConnectionActivityGlobals(), missionOptions: new Uint8Array(87), missionRestrictions: new Uint32Array(256) });
    s.gameAux.freeTechnologyType = 181; s.gameAux.templeProductType = 182; s.gameAux.bombFlareType = 183; s.gameAux.fenceMarkerType = 184; s.gameAux.fenceShieldType = 185; s.gameAux.sunAviaryProcessType = 41; s.gameAux.windAviaryProcessType = 42; s.gameAux.rainAviaryProcessType = 43;
    const o = new RawCampaignSessionOwners(game, files, 'same', input);
    try {
        // 같은 참조로 빌리는 입력은 모두 동일 객체다.
        for (const name of ['gameplayGlobals', 'templateStorage', 'fortTypeConversion', 'chatGlobals', 'localStateGlobals', 'connectionActivityGlobals', 'soundNames']) assert.equal(o[name], input[name]);
        assert.notEqual(o.services, input); input.unixSeconds = () => 2; assert.equal(o.services.unixSeconds(), 1700000000);
        assert.equal(o.battleOptions, input.missionOptions); assert.equal(o.restrictions, input.missionRestrictions); o.hiddenPlayers[4] = 9; assert.equal(input.missionOptions[0x42], 9); o.restrictions[71] = 7; assert.equal(input.missionRestrictions[71], 7);
        assert.deepEqual([o.freeTechnologyType, o.templeProduct, o.flareType, o.fenceNodeType, o.fenceShieldType, o.sunAviaryType, o.windAviaryType, o.rainAviaryType], [181, 182, 183, 184, 185, 41, 42, 43]);
        o.freeTechnologyType = 180; o.templeProduct = 179; o.flareType = 178; assert.deepEqual([s.gameAux.freeTechnologyType, s.gameAux.templeProductType, s.gameAux.bombFlareType], [180, 179, 178]);
        s.gameAux.fenceMarkerType = 177; assert.equal(o.fenceNodeType, 177); s.gameAux.priestType = 159; assert.equal(o.priestState.movement.priest, 159);
        assert.deepEqual([o.piece, o.dudeProviderIndex, o.windFlyerType, o.rainFlyerType, o.sunFlyerType], [107, 0, 138, 144, 73]);
    } finally { o.dispose(); }
    assert.equal(s.gameAux.freeTechnologyType, 180); assert.equal(s.gameAux.fenceMarkerType, 177); returned(gameplayGlobals, o.playerRecords, s);
});

test('Session 사전 거부: 제외 모드와 다른 Subscriber·권한·보조 전역은 fresh 전에 실패한다', () => {
    const s = storageFor(); const gameplay = new ClientGameplayGlobals(null, s.authority, s.gameAux); gameplay.player.playerKnown = 77;
    const otherAuthority = storageFor(game, new RawAuthorityState(), s.gameAux); const otherAux = storageFor(game, s.authority, new GameAuxState());
    try {
        // 다른 owner 검사는 같은 실제 게임 전역을 초기화하기 전에 실행된다.
        for (const extra of [{ installMode: 'editor' }, { adoptPreparedWorld: true }, { editorTransport: {} }, { deferExistingEditorFile: true }, { generalBattleStart: {} }, { subscriberGlobals: new ClientSubscriberGlobals() }, { surfaceStorage: otherAuthority }, { surfaceStorage: otherAux }]) {
            assert.throws(() => new RawCampaignSessionOwners(game, files, 'x', services({ gameplayGlobals: gameplay, surfaceStorage: s, ...extra })), /불일치|지원하지/); assert.equal(gameplay.player.playerKnown, 77); assert.equal(s.borrowed, false);
        }
    } finally { s.dispose(); otherAuthority.dispose(); otherAux.dispose(); }
});

test('Session 자산 불일치: 게임 대여 뒤의 원문 위치에서 거부하고 앞선 대여를 반납한다', () => {
    const s = storageFor({}); const g = new ClientGameplayGlobals(null, s.authority, s.gameAux); const p = new ClientPlayerRecords(); g.player.playerKnown = 77;
    assert.throws(() => new RawCampaignSessionOwners(game, files, 'x', services({ gameplayGlobals: g, playerRecords: p, surfaceStorage: s })), /자산 불일치/);
    assert.equal(g.player.playerKnown, 0); assert.equal(s.generation, 0); assert.equal(p.base(), 0); returned(g, p, s); s.dispose();
});

test('Session 중복 대여: 살아 있는 실제 owner의 World/Player/게임 상태를 새 Session으로 덮지 않는다', () => {
    const first = new RawCampaignSessionOwners(game, files, 'first', services()); const raw = first.core.pool.slot(1).slice();
    try {
        first.gameplayGlobals.player.playerKnown = 31;
        assert.throws(() => new RawCampaignSessionOwners(game, files, 'second', services({ gameplayGlobals: first.gameplayGlobals })), /중복 대여/); assert.equal(first.playerState.playerKnown, 31); assert.deepEqual(first.core.pool.slot(1), raw);
        const other = new ClientGameplayGlobals(); assert.throws(() => new RawCampaignSessionOwners(game, files, 'second', services({ gameplayGlobals: other, playerRecords: first.playerRecords })), /이전 세션/); const use = other.acquire(); use.release();
        assert.throws(() => first.playerRecords.acquire(), /이전 세션/);
    } finally { first.dispose(); }
});

test('Session 생성 중 실패: 옵션·시각·소리·금지 표 단계까지 확보한 실제 대여를 역순 반납한다', () => {
    const s = storageFor(); const g = new ClientGameplayGlobals(null, s.authority, s.gameAux); const p = new ClientPlayerRecords();
    try {
        // 모든 실패는 실제 World가 구성된 뒤이며 다음 시도는 같은 물리 owner를 다시 빌린다.
        for (const extra of [{ missionOptions: new Uint8Array(86) }, { unixSeconds: null }, { soundNames: null }, { missionRestrictions: new Uint32Array(255) }]) {
            assert.throws(() => new RawCampaignSessionOwners(game, files, 'x', services({ gameplayGlobals: g, playerRecords: p, surfaceStorage: s, ...extra })), /87BYTE|Unix 초|SoundList|256DWORD/); returned(g, p, s);
        }
        assert.equal(s.generation, 4); assert.equal(p.base(), 0);
    } finally { s.dispose(); }
});

test('Session Player/SP: 실제 초기화9기록·방향 동맹81·같은 Core뷰·독립 난수/사제 추적을 연결한다', () => {
    let clockCalls = 0; const o = new RawCampaignSessionOwners(game, files, 'x', services({ unixSeconds: () => { clockCalls += 1; return 1700000000; } }));
    try {
        assert.equal(clockCalls, 0); o.initializePlayersAndSp(); assert.equal(clockCalls, 1); assert.equal(o.playersReady, true); assert.equal(o.players, o.playerRecords.records()); assert.equal(o.coreDataGlobals.players, o.players); assert.equal(o.players.length, 1620);
        // 원문 Player initialize의 각 행은 자기 방향 동맹만 켜며 Core 10BYTE는 별도 기본값이다.
        for (let owner = 0; owner < 9; owner += 1) { assert.equal(get(o.players, owner * 180 + 0x54), 1 << owner); for (let target = 0; target < 9; target += 1) assert.equal(o.alliances[owner * 9 + target], Number(owner === target)); }
        assert.equal(o.sp.trackingOwner, o.priestState.regen.patch.tracking); assert.equal(o.sp.trackingOffset, o.sp.offset);
        assert.equal(o.playerState.playerKnown, 1); assert.equal(o.level.pool, o.core.pool); assert.equal(o.objectState.mission, true); assert.equal(o.objectState.chaffMode, true); assert.equal(o.finishState.chaffMode, true);
        const expectedRandom = { crt: new CrtRandom(0), game: new GameRandom(0), gameMode: 0 }; const seed = new GameRandomSeed(expectedRandom, () => 1700000000); const expected = new ScrambledSpStore(expectedRandom.game, () => seed.reseed()); expected.bindTrackingOffset({ value: 0 }); expected.initialize();
        assert.equal(o.random.state, expectedRandom.game.state); assert.equal(o.particleCrt.state, expectedRandom.crt.state); assert.deepEqual(o.sp.pools(), expected.pools()); assert.deepEqual(o.sp.permutation(), expected.permutation()); assert.equal(o.sp.offset, expected.offset); assert.equal(o.randomGlobals.gameMode, 0);
        assert.throws(() => o.initializePlayersAndSp(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
});

test('Session 공통 Player/SP: 초기화한 같은 Client 저장소는 재할당·재시드하지 않고 장기 값으로 빌린다', () => {
    const random = new ClientRandomGlobals(); let clock = 0; const sp = new ClientSpGlobals(random, () => { clock += 1; return 1700000000; }); sp.store().initialize(); sp.store().set(1, 1234);
    const p = new ClientPlayerRecords(); const core = new ClientCoreDataGlobals(); p.bindCoreData(core); p.initialize(() => {}); p.records()[37] = 0x91; p.alliances()[8] = 0x76543210;
    const before = p.records().slice(); const gameBefore = random.game.state; const crtBefore = random.crt.state;
    const o = new RawCampaignSessionOwners(game, files, 'x', services({ randomGlobals: random, spGlobals: sp, playerRecords: p, coreDataGlobals: core, adoptInitializedPlayers: true }));
    try {
        o.initializePlayersAndSp(); assert.equal(o.sp, sp.store()); assert.equal(o.random, random.game); assert.equal(o.particleCrt, random.crt); assert.equal(o.sp.get(1), 1234); assert.equal(clock, 1); assert.equal(random.game.state, gameBefore); assert.equal(random.crt.state, crtBefore); assert.deepEqual(p.records(), before); assert.equal(o.coreDataGlobals, core); assert.equal(p.alliances()[8], 0x76543210);
    } finally { o.dispose(); }
    assert.equal(sp.store().get(1), 1234); assert.deepEqual(p.records(), before);
});

test('Session 공통 계약: SP 초기화·난수 owner·Player/Core 정체성 실패는 Player 쓰기 전에 멈춘다', () => {
    const random = new ClientRandomGlobals(); const sp = new ClientSpGlobals(random, () => 1700000000);
    const scenarios = [{ randomGlobals: random, spGlobals: sp }, { randomGlobals: new ClientRandomGlobals(), spGlobals: sp }, { spGlobals: sp }, { playerRecords: new ClientPlayerRecords(), coreDataGlobals: new ClientCoreDataGlobals() }];
    // 이 검사는 각 실패의 실제 고정뷰 owner를 독립적으로 빌리고 반납한다.
    for (const extra of scenarios) {
        const o = new RawCampaignSessionOwners(game, files, 'x', services(extra));
        try { assert.throws(() => o.initializePlayersAndSp(), /SP 초기화|소유자 불일치/); assert.equal(o.playerRecords.base(), 0); assert.equal(o.playersReady, false); assert.throws(() => o.initializePlayersAndSp(), /반복/); } finally { o.dispose(); }
    }
});

test('Session 현재 Core: 이전 Player를 가리키는 Core는 실제0선택 뒤에만 새 Player로 바뀐다', () => {
    const p = new ClientPlayerRecords(); const core = new ClientCoreDataGlobals(); p.bindCoreData(core); p.initialize(() => {}); const old = p.records(); core.current = old.subarray(0x6c, 0x76);
    const o = new RawCampaignSessionOwners(game, files, 'x', services({ playerRecords: p, coreDataGlobals: core }));
    try { o.initializePlayersAndSp(); assert.notEqual(o.players, old); assert.equal(core.players, o.players); assert.equal(core.current, null); assert.equal(o.playerState.templateOwner, 1); assert.equal(o.playerState.playerKnown, 1); } finally { o.dispose(); }
});

test('Session 부분 실패: Player 뒤 시각 예외는 실제 앞선 기록·추적 연결을 유지하고 반복을 거부한다', () => {
    const o = new RawCampaignSessionOwners(game, files, 'x', services({ unixSeconds: () => { throw new Error('시각 중단'); } }));
    try {
        assert.throws(() => o.initializePlayersAndSp(), /시각 중단/); assert.equal(o.playerRecords.initialized(), true); assert.equal(o.players, o.playerRecords.records()); assert.equal(o.coreDataGlobals.players, o.players); assert.equal(o.sp.initialized, false); assert.equal(o.sp.trackingOwner, o.priestState.regen.patch.tracking); assert.equal(o.playersReady, false); assert.equal(o.objectState.mission, false); assert.equal(o.playerState.playerKnown, 0); assert.equal(o.randomGlobals.gameMode, 0); assert.throws(() => o.initializePlayersAndSp(), /반복/);
    } finally { o.dispose(); }
    assert.equal(o.playerRecords.records().length, 1620); returned(o.gameplayGlobals, o.playerRecords, o.world.storage);
});

test('Session AI 할당: 생성 때 호출하지 않고 실제 할당의 같은 바이트에만 공급한다', () => {
    let calls = 0; const o = new RawCampaignSessionOwners(game, files, 'x', services({ ai: { allocationBytes: (bytes) => { calls += 1; bytes.fill(0x91); } } }));
    try { assert.equal(calls, 0); const token = o.aiMemory.allocate(0xb08); assert.equal(token, 0x20000000); assert.equal(calls, 1); assert.equal(o.aiMemory.bytes(token)[0xb07], 0x91); o.aiMemory.free(token); assert.throws(() => o.aiMemory.bytes(token), /누락/); } finally { o.dispose(); }
    const missing = new RawCampaignSessionOwners(game, files, 'x', services()); try { assert.throws(() => missing.aiMemory.allocate(0xb08), /미연결/); } finally { missing.dispose(); }
});

test('Session 채택 거부: 초기화하지 않은 Player를 완료 기록으로 만들지 않는다', () => {
    const o = new RawCampaignSessionOwners(game, files, 'x', services({ adoptInitializedPlayers: true }));
    try { assert.throws(() => o.initializePlayersAndSp(), /초기화 완료/); assert.equal(o.playersReady, false); assert.equal(o.players.length, 0); assert.equal(o.playerRecords.base(), 0); assert.equal(o.sp.initialized, false); } finally { o.dispose(); }
});

test('Session 레벨 진단: 현재 실제 모듈은 같은 원문 식을 실제 장치에 전달한다', () => {
    const events = []; const o = new RawCampaignSessionOwners(game, files, 'x', services({ ai: { assertion: (expression) => events.push(expression) } }));
    try { o.initializePlayersAndSp(); const sid = o.core.pool.allocate(2); o.core.pool.slot(sid)[10] = 71; o.level.set(sid, 9); assert.equal(o.level.get(sid), 1); assert.deepEqual(events, ['0 <= n && n < 8']); } finally { o.dispose(); }
    const missing = new RawCampaignSessionOwners(game, files, 'x', services());
    try { missing.initializePlayersAndSp(); const sid = missing.core.pool.allocate(2); missing.core.pool.slot(sid)[10] = 71; assert.throws(() => missing.level.set(sid, 9), /미연결/); } finally { missing.dispose(); }
});
