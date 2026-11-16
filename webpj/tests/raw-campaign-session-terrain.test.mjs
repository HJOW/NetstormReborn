// Session1825..1865를 실제 선행 모듈/World로 조립하는 소스 계약 검사다(WB04-E619).
// 공중3종 전용 생성/행동·미션 진행/승리·Native 장기 설치 관측은 포함하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { CanonDecoder } from '../www/assets/territory.js';
import { TERRITORY_PATTERNS } from '../www/assets/territory-patterns.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 선택하지 않은 외부 효과는 명시 오류다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 복사/가공하지 않은 보호 자산과 원본 팔레트다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 같은 선행 owner를 만들고 원문 생성 순서의 연결만 실행한다. 미션 객체는 적재하지 않는다.
function owner(extra = {}) {
    return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra,
        sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden, ...extra.sound } });
}
// 생성자1825 전에 완성되는 실제 조립 단계다. 각 메서드는 한 번만 호출한다.
function before(o) {
    o.initializePlayersAndSp();
    // 표의 순서는 원문 생성자의 의존 순서다. 새 가짜 월드/처리기를 넣지 않는다.
    for (const name of ['PlayerTablesAndPackets', 'IslandsAndPriests', 'PlayersAndOwnership', 'BodiesAndTypeTables', 'GunAndCursorViews', 'HitBodies', 'GunBodies', 'Particles', 'Teleport', 'Combat', 'Vortex', 'Assets', 'WorldRuntimeHooks', 'ContentNotices', 'ContentLifetime', 'Production', 'Factories', 'Flare', 'Construction', 'Damageable', 'IceTower', 'CarrierSteal', 'BombPostPop', 'AcidZap', 'AcidFence', 'CarrierRegular', 'SunAviary', 'WindAviary', 'RainAviary', 'ArcSpire', 'Flyer', 'Fence', 'GemReward', 'WorldDeletion', 'WorldRegular', 'WorldTransmission']) o[`connect${name}`]();
}
// 실제 World Runtime은 게시하지만 외곽 Session/Load는 만들지 않는다.
function scene(extra) { const o = owner(extra); before(o); o.connectTerrain(); return o; }

test('Session 지형: 선행·한 번 게시·동일 풀/난수·깊이와 외곽 Session 미공개', () => {
    const o = owner();
    try {
        assert.throws(() => o.connectTerrain(), /초기화 전/); before(o);
        const bytes = o.core.pool.bytes(); const seed = o.random.state; const crt = o.particleCrt.state; // 조립 전 실제 상태다.
        o.connectTerrain(); assert.equal(o.terrainModules.ready, true);
        assert.equal(o.normal.pool, o.core.pool); assert.equal(o.pipeline.pool, o.core.pool); assert.equal(o.finish.pool, o.core.pool); assert.equal(o.finishPlacement.pool, o.core.pool);
        assert.equal(o.objectState.loadingDepth, o.templateStorage.serializationGlobals.loadingDepth); assert.equal(o.state.carrierState.loadingDepth, o.objectState.loadingDepth);
        assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.equal(o.particleCrt.state, crt);
        assert.throws(() => o.world.bindRuntime(o.worldHooks), /한 번/); assert.throws(() => o.connectTerrain(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectTerrain(), /반납/); assert.throws(() => o.terrainModules.generation.layout(), /반납/);
});

test('Session World 게시: 실제 공통 삭제 writer가 Session 송신 소비자까지 전달', () => {
    const sent = []; const o = scene({ formPacket: (packet) => sent.push(packet.slice()) }); // 실제 바깥 전송 경계다.
    try {
        const sid = o.world.create(75); // Pop/공격/행동 없는 일반 자산이다.
        o.world.transmitDeletion(sid, 0x00000009); assert.equal(sent.length, 1); assert.equal(sent[0][1], 0x2e); assert.equal(sent[0][4] | (sent[0][5] << 8), sid);
        o.world.setLoadingDepth(3); assert.equal(o.objectState.loadingDepth, 3); assert.equal(o.state.carrierState.loadingDepth, 3);
    } finally { o.dispose(); }
});

test('Session 지형 root: 같은 현재 군도/확장 상자·옵션·권한·Checking과 Player base 재선택', () => {
    const o = scene(); const g = o.terrainModules.generation; const aux = o.gameplayGlobals.gameAux(); // 실제 장기 전역이다.
    try {
        aux.archipelago = 7; aux.worldGeyserMargin = 4; assert.equal(g.archipelago.value, 7); assert.equal(g.span.value, 4); assert.equal(o.state.farm.currentSpan(), 4);
        g.archipelago.value = 0; g.span.value = 6; assert.equal(aux.archipelago, 0); assert.equal(aux.worldGeyserMargin, 6); assert.equal(o.state.farm.currentSpan(), 6);
        aux.archipelagoMinimum = 21; aux.archipelagoMaximum = 37; aux.archipelagoPercent = 44;
        assert.deepEqual([g.minimum.value, g.maximum.value, g.percent.value], [21, 37, 44]);
        o.battleOptions[5] = 2; o.battleOptions[6] = 3; assert.equal(g.layout(), 2); assert.equal(g.spacing(), 6);
        aux.terrainEnabled = 0; o.core.ownerMode.battle = false; o.core.bridgeState.mission = false; assert.deepEqual([g.enabled(), g.battle(), g.mission()], [0, false, false]);
        o.gameplayGlobals.aiDebug().checking().value = 9; assert.equal(g.growth.checking(), 9);
        const replacement = new Uint8Array(9 * 180); new DataView(replacement.buffer).setUint32(8 * 180 + 0x2c, 0xfedcba98, true); o.coreDataGlobals.players = replacement;
        assert.equal(g.subscriber(8), 0xfedcba98); assert.throws(() => g.subscriber(0), RangeError); assert.throws(() => g.subscriber(9), RangeError);
        o.coreDataGlobals.players = new Uint8Array(8); assert.throws(() => g.subscriber(1), RangeError);
    } finally { o.dispose(); }
});

test('Session 지형 root: Player 배치는 현재 실제 건설 부모, 진단은 최초 함수 값, 로그는 현재 서비스', () => {
    const calls = []; const o = scene({ ai: { assertion: (text) => calls.push(['initial', text]) }, player: { log: (level, text) => calls.push([level, text]) } });
    const g = o.terrainModules.generation; const original = o.construction; // 실제 현재 건설 부모를 복원할 참조다.
    try {
        const record = Uint8Array.of(3, 1, 0, 7, 0, 0); const expected = original.playerBounds.placement(2, record);
        assert.deepEqual(g.placement(2, record), expected); o.construction = null; assert.throws(() => g.placement(2, record), /미연결/); o.construction = original;
        o.services.ai.assertion = forbidden; g.growth.assertion('bounds', 'file', 3); assert.deepEqual(calls[0], ['initial', 'bounds']);
        g.logPercent(-42); assert.deepEqual(calls[1], [0x1d, '\nArchipelago Percentage: -42\n']);
        o.services.player.log = (level, text) => calls.push(['current', level, text]); g.logPercent(12); assert.equal(calls[2][0], 'current');
        o.services.player.log = null; assert.throws(() => g.logPercent(1), /미연결/);
    } finally { o.construction = original; o.dispose(); }
});

test('Session 지형: 현재 puzzlePiece의 실제 패턴 수를 같은 CanonDecoder로 전수 대조', () => {
    const o = scene();
    try {
        // 원문64 패턴과4 회전의 현재 자산 프레임을 순회한다. 미션 지형 생성/클리어는 실행하지 않는다.
        for (let pattern = 0; pattern < Math.min(64, TERRITORY_PATTERNS.length); pattern += 1) {
            // 각 회전의 decoder가 유효한 칸만 센다.
            for (let rotation = 0; rotation < 4; rotation += 1) {
                const decoder = new CanonDecoder(o.core.frames[o.piece].codes, TERRITORY_PATTERNS[pattern], rotation * 2, 0, 0, forbidden); let count = 0;
                // 첫 칸을 포함하며 실제 decoder 종단까지 계산한다.
                for (; decoder.valid; decoder.advance()) count += 1;
                assert.equal(o.normal.patternCount(Uint8Array.of(pattern | (rotation << 6), 0, 0, 0, 0, 0)), count);
            }
        }
    } finally { o.dispose(); }
});

test('Session finishOwner: 실제 chaff 조기·현재 Player 기록·invalidType 진단과 현재 전체 배치 관문', () => {
    const o = scene();
    try {
        assert.equal(o.finish.finish(0xffffffff), undefined); // 미션 chaff는 범위 조회 전에 반환한다.
        o.finishState.chaffMode = false; new DataView(o.players.buffer).setInt32(2 * 180 + 0x78, o.finishState.invalidRegion, true); assert.equal(o.finish.finish(2), undefined);
        assert.throws(() => o.finish.placeRandom(o.random, { left: 0, top: 0, right: 1, bottom: 1 }, 0, 1, 0, 0), /cpp:1862/);
        assert.throws(() => o.finishPlacement.hooks.mayPlace({}), /전체 MayPlace 미설치/);
    } finally { o.dispose(); }
});

test('Session 지형 진단: 빠진 실제 장치를 원문 선택 위치에서 거부', () => {
    const o = scene();
    try { assert.throws(() => o.terrainModules.generation.growth.assertion('x', 'source', 17), /source:17/); assert.throws(() => o.terrainModules.diagnostics.context(), /cpp:1828/); }
    finally { o.dispose(); }
});
