// Session1815..1824의 현재 송신/버퍼·fallback·다리/깊이 계약이다(WB04-E618).
// 공중3종의 새 전용 생성/생산/행동은 실행하지 않는다. 실제 미션/승리 검사도 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { DaisProcess } from '../www/core/raw-dais-process-data.js';
import { ConstructionProcess, createConstructionProcessFields } from '../www/core/raw-construction-process.js';
import { BombEffectTimerProcess } from '../www/core/raw-bomb-effect-timer.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 미선택 외부 서비스는 명시적으로 거부한다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 실제 자산/팔레트이며 별도 근사 월드를 만들지 않는다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 바깥 서비스만 주입하고 실제 하위 조립은 owner의 원문 생성 순서를 사용한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra, sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden, ...extra.sound } }); }
// 첫 저장 객체 전에 같은 모듈을 순서대로 연결한다. 공중 유닛 생성/행동은 포함하지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); o.connectArcSpire(); o.connectFlyer(); o.connectFence(); o.connectGemReward(); o.connectWorldDeletion(); o.connectWorldRegular(); }
// World/Session을 게시하지 않은 실제 동일 조립이다.
function scene(extra) { const o = owner(extra); before(o); o.connectWorldTransmission(); return o; }

// 선행/반복/반납, 실제 원문 버퍼 예약과 Pool/난수 무효과를 확인한다.
test('Session 송신 조립: 같은 함수·저장소·선행 관문과 Runtime 미게시', () => {
    const o = owner(); // 아직 선행 조립을 하지 않은 실제 owner다.
    try {
        assert.throws(() => o.connectWorldTransmission(), /초기화 전/); before(o);
        const seed = o.random.state; const bytes = o.core.pool.bytes(); // 결선 전 실제 난수/풀 물리다.
        o.connectWorldTransmission(); assert.equal(o.transmissionModules.ready, true); assert.equal(o.worldHooks.transmit, o.transmissionModules.hooks);
        assert.equal(o.transmissionModules.bombTimerPacket.length, 222); assert.equal(o.transmissionModules.flyerPacket.length, 64);
        assert.notEqual(o.transmissionModules.flyerPacket, o.transmissionModules.windAviaryPacket);
        assert.equal(o.random.state, seed); assert.deepEqual(o.core.pool.bytes(), bytes);
        assert.throws(() => o.connectWorldTransmission(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectWorldTransmission(), /반납/);
    assert.throws(() => o.transmissionModules.hooks.packet(0), /반납/);
});

// 두 합성 결과의 마지막 선택은 동일 Impl 표식이며 writer는 현재 헤더를 재조회한다.
test('Session Dais 송신: 실제 writer·현재 헤더/버퍼·두 합성의 공유 선택·현재 송신 서비스', () => {
    const packets = []; const o = scene({ formPacket: (bytes) => packets.push(bytes.slice()) }); // 실제 송신 경계의 관찰이다.
    try {
        const first = o.worldHooks.transmit; const second = o.transmissionModules.transmission(); const process = new DaisProcess(); // 유닛 없이 실제 빈 Process만 만든다.
        const aux = o.gameplayGlobals.gameAux(); // 같은 장기 헤더/공통 버퍼 소유자다.
        aux.daisProcessHeader[0] = 0xea; aux.daisProcessHeader[1] = 0x81;
        assert.equal(first.writeData(process, true), 17); assert.equal(second.packet(17).buffer, aux.squidTransmission.buffer.buffer);
        assert.deepEqual([...second.packet(2)], [0x81, 0xea]); assert.equal(process.fields.bytes[24], 0);
        second.transmit(17); assert.deepEqual(packets[0], aux.squidTransmission.buffer.slice(0, 17));
        assert.equal(second.writeData(process, false), 9); assert.equal(first.packet(9)[8], 0);
        assert.equal(first.writeFullData(process, 0x80000000), 17); assert.equal(first.packet(17)[8], 0xff);
        assert.throws(() => second.packet(-1), RangeError); assert.throws(() => second.transmit(2049), RangeError);
        o.services.formPacket = (bytes) => packets.push(Uint8Array.of(bytes.length)); first.transmit(9); assert.deepEqual(packets.at(-1), Uint8Array.of(9));
    } finally { o.dispose(); }
});

// 다른 프로세스는 기본 Form/Regular 하위로 넘기며 각 함수 값은 합성 순간의 사본이다.
test('Session 송신 fallback: 파생 선택 복귀·값형 함수 캡처·미연결 거부', () => {
    const o = scene(); // 원문 송신 허브를 가진 같은 실제 owner다.
    try {
        const calls = []; const bytes = new Uint8Array(32); // 외곽 fallback의 명시 경계다.
        const fallback = { writeData: (...args) => { calls.push(['data', ...args]); return 11; }, writeFullData: (...args) => { calls.push(['full', ...args]); return 12; }, packet: (length) => bytes.subarray(0, length), transmit: (length) => calls.push(['send', length]) };
        const tx = o.transmissionModules.daisTransmission(fallback); const other = {}; // 실제 Dais 이외 객체와 복사할 하위 표다.
        fallback.writeData = forbidden; assert.equal(tx.writeData(other, true), 11); assert.equal(tx.writeFullData(other, 7), 12); tx.transmit(11); assert.equal(tx.packet(4).buffer, bytes.buffer);
        assert.deepEqual(calls, [['data', other, true], ['full', other, 7], ['send', 11]]);
        const missing = o.transmissionModules.daisTransmission({}); // 미설치를 확인하는 별도 하위 표다.
        assert.throws(() => missing.writeData(other, false), /writeData/); assert.throws(() => missing.writeFullData(other, 1), /writeFullData/);
        assert.throws(() => missing.packet(0), /packet/); assert.throws(() => missing.transmit(0), /transmit/);
    } finally { o.dispose(); }
});

// 건설 일반/전체 작성도 같은 원문30BYTE writer와 변경된 장기 헤더를 사용한다.
test('Session 건설 송신: 현재 공통 버퍼·헤더와 실제 필드 dirty/force', () => {
    const sent = []; const o = scene({ formPacket: (bytes) => sent.push(bytes.slice()) }); // 선택된 송신만 기록한다.
    try {
        const fields = createConstructionProcessFields(); fields.start = 1.25; fields.end = 9.5; fields.additional = 0xab; // 실제 값형30 필드다.
        const process = new ConstructionProcess(o.building, fields); const tx = o.worldHooks.transmit; const aux = o.gameplayGlobals.gameAux(); // 같은 현재 writer/버퍼다.
        aux.constructionHeader[0] = 0xb7; aux.constructionHeader[1] = 3;
        assert.equal(tx.writeData(process, true), 26); assert.equal(tx.packet(26)[0], 3); assert.equal(tx.packet(26)[1], 0xb7);
        assert.equal(Buffer.from(tx.packet(26)).readDoubleLE(9), 1.25); assert.equal(Buffer.from(tx.packet(26)).readDoubleLE(17), 9.5); assert.equal(tx.packet(26)[25], 0xab);
        assert.equal(process.fields.dirty, 0); tx.transmit(26); assert.equal(sent.length, 1);
        assert.equal(tx.writeData(process, false), 9); assert.equal(tx.writeFullData(process, 0x80000000), 26);
    } finally { o.dispose(); }
});

// Dais/Construction은 Native에서도 직접 writer이며 실제 broadcast 장치를 선택한다.
test('Session 현재 Native 송신: 직접 writer와 broadcast 선택·장치 누락을 우회하지 않음', () => {
    const o = scene({ formPacket: forbidden }); const storage = o.world.storage; const calls = []; // 현재 Storage 장치의 관찰이다.
    try {
        storage.usesNativeTransmissionDevices = () => true; storage.serializeProcessForm = forbidden; storage.broadcastProcessForm = (length) => calls.push(length);
        const tx = o.worldHooks.transmit; // Native 여부는 실제 작성/송신마다 다시 읽는다.
        assert.equal(tx.writeData(new DaisProcess(), false), 17); tx.transmit(17);
        assert.equal(tx.writeData(new ConstructionProcess(o.building, createConstructionProcessFields()), false), 26); tx.transmit(26);
        assert.deepEqual(calls, [17, 26]); delete storage.broadcastProcessForm; assert.throws(() => tx.transmit(26), TypeError);
    } finally { delete storage.usesNativeTransmissionDevices; delete storage.serializeProcessForm; delete storage.broadcastProcessForm; o.dispose(); }
});

// 공중3종을 생성하지 않고 상태 타이머의 기존 writer와 현재 활성 Player/peer 길이 확정을 확인한다.
test('Session 타이머 송신: 기존 실제7d writer·현재 peer와 선언 길이 확정', () => {
    const sent = []; const o = scene({ formPacket: (bytes) => sent.push(bytes.slice()) }); // 단독 전송 경계만 기록한다.
    try {
        const timer = new BombEffectTimerProcess(o.core.pool, o.core.types, () => 0, {}); const tx = o.worldHooks.transmit; // Init/부착/행동 없는 실제 cdecl 반환값이다.
        timer.fields.members = [10, 11];
        assert.equal(tx.writeData(timer, false), 26); assert.equal(tx.packet(26).buffer, o.transmissionModules.bombTimerPacket.buffer);
        assert.equal(Buffer.from(tx.packet(26)).readUInt16LE(2), 22); tx.transmit(26); assert.equal(Buffer.from(sent[0]).readUInt16LE(2), 22);
        o.players[2 * 180 + 0x50] |= 1; o.services.battlePeers[2] = 99;
        tx.transmit(26); assert.equal(Buffer.from(sent[1]).readUInt16LE(2), 26);
    } finally { o.dispose(); }
});

// 낙하·파편/소리·로딩 깊이는 같은 현재 부모를 호출하고 앞선 부분 효과를 유지한다.
test('Session 후단 훅: 현재 낙하/파편·원문 위치음향·같은 깊이와 의식 준비', () => {
    const sounds = []; const o = scene({ sound: { wallSeconds: () => 0, playOnceAt: (...args) => sounds.push(args) } }); // 위치음향 경계만 기록한다.
    const saved = [o.priests.fallWalker, o.particles.addShrapnel, o.priests.setLoadingDepth, o.assetBindings.setLoadingDepth, o.runtimeModules.prepareRitual]; // 실제 모듈의 원래 함수 값이다.
    try {
        const calls = []; // 현재 조회를 관찰하는 부수 효과다.
        o.priests.fallWalker = (sid) => { calls.push(['fall', sid]); return true; }; assert.equal(o.worldHooks.walkerFall(31), true);
        o.particles.addShrapnel = (...args) => calls.push(['shrapnel', ...args]); o.worldHooks.bridgeEffect({ effect: 'notifyRemoval', sid: 42 });
        o.worldHooks.bridgeEffect({ effect: 'fallSound', x: 2.25, y: -3.5 }); assert.deepEqual(sounds, [[2.25, -3.5, o.soundNames.lookup('bridgeFall.wav'), 0, 0]]);
        assert.throws(() => o.worldHooks.bridgeEffect({ effect: 'other' }), /다른 수명/);
        o.priests.setLoadingDepth = (depth) => calls.push(['priestDepth', depth]); o.assetBindings.setLoadingDepth = (depth) => calls.push(['assetDepth', depth]); o.runtimeModules.prepareRitual = () => calls.push(['ritual']);
        o.daisState = {}; o.worldHooks.loadingDepth(3);
        assert.equal(o.objectState.loadingDepth, 3); assert.equal(o.state.carrierState.loadingDepth, 3);
        assert.deepEqual(calls, [['fall', 31], ['shrapnel', 42, 2], ['priestDepth', 3], ['assetDepth', 3], ['ritual']]);
    } finally { [o.priests.fallWalker, o.particles.addShrapnel, o.priests.setLoadingDepth, o.assetBindings.setLoadingDepth, o.runtimeModules.prepareRitual] = saved; o.daisState = null; o.dispose(); }
});
