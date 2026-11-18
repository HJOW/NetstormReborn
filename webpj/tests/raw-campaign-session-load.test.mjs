// 실제 TheWarBegins 저장 자료의 Session Load를 검사한다. 플레이·포획·희생·공중 유닛 전용 행동은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { RawCampaignSession } from '../www/core/raw-campaign-session.js';
import { CampaignSessionAssets } from '../www/core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { Kernel } from '../www/core/kernel.js';
import { InputQueue } from '../www/core/input-queue.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { fortSection, parseFort } from '../www/assets/fort.js';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { TypeTable } from '../www/assets/type-table.js';
import { layoutFort, pieceFrameCodes } from '../www/assets/territory.js';
import { SoundList } from '../www/audio/sound-list.js';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { ClientCoreDataGlobals } from '../www/core/client-core-data-globals.js';
import { ClientSubscriberGlobals } from '../www/core/client-subscriber-globals.js';
import { captureMissionFortZone, createMissionFortLoadRequest, writeMissionFortRequest } from '../www/core/mission-fort-request.js';
import { RawMissionAiSettings } from '../www/core/raw-mission-ai-settings.js';
import { RawCampaignMissionStartBindings } from '../www/core/raw-campaign-mission-start-bindings.js';
import { createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { createMissionStartState } from '../www/core/raw-mission-start.js';
import { MissionScript } from '../www/core/mission-script.js';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { ClientConnectionGlobals } from '../www/core/client-connection-globals.js';
import { ClientChatWindowGlobals } from '../www/core/client-chat-window-globals.js';
import { ClientConnectionActivityGlobals } from '../www/core/client-connection-activity-globals.js';
import { installClientPacketDefaults } from '../www/core/client-campaign-transport.js';
import { ClientUserSelectionGlobals } from '../www/core/client-user-selection-globals.js';
import { RawUserHighlight } from '../www/core/raw-user-highlight.js';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { DaisProcess, writeDaisProcessData } from '../www/core/raw-dais-process-data.js';
import { GunProcess } from '../www/core/raw-gun-process.js';
import { findAttachedProcess } from '../www/core/raw-wind-aviary-process.js';
import { RawPathProcess } from '../www/core/raw-path-process.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { loadOriginalForts, readData } from './helpers.mjs';

// 보호된 실제 TYPE·SHP·팔레트와 TheWarBegins Fort를 읽는다.
const data = realCaptureAssets(), game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
const archive = new TaffArchive(readData(ARCHIVE_PATH));
const entry = loadOriginalForts(archive).find((candidate) => /thewarbegins.fort$/i.test(candidate.source));
const fort = parseFort(entry.bytes, createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText)));
// 예상하지 않은 외부 장치는 호출된 자리에서 중단한다.
const forbidden = (name) => () => { throw new Error(`외부 장치 미연결: ${name}`); };

// 게임 논리는 실제 몸체로 실행하고 UI·음향은 관찰하며 명령은 실제 원본 큐에 담는다.
function scene(overrides = {}, savedFort = fort) {
    const trace = [], queue = new InputQueue();
    const s = new RawCampaignSession(game, { read: forbidden('파일') }, savedFort, overrides.missionName ?? 'thewarbegins', {
        unixSeconds: () => 1700000000, soundNames: new SoundList(), carrierRequest: forbidden('전송'), deletion: { selected: () => 0, ...overrides.deletion },
        install: { translateYou: () => Uint8Array.of(89, 111, 117, 0), setConfig: () => trace.push('config'), refreshWindowTitle: () => trace.push('title'), resetCamera: () => trace.push('camera'), ...overrides.install },
        ownership: { refreshProduction: () => trace.push('production'), themeMode: () => 0 },
        assets: { postPop: { playImpactSound: () => trace.push('impact') }, queueCommand: (...args) => queue.pushCommand(...args) },
        templateStorage: overrides.templateStorage, playerRecords: overrides.playerRecords, coreDataGlobals: overrides.coreDataGlobals, subscriberGlobals: overrides.subscriberGlobals, missionFortRequest: overrides.missionFortRequest, missionSubscriber: overrides.missionSubscriber, missionOptions: overrides.missionOptions, fortMission: overrides.fortMission, production: overrides.production, newTechnology: overrides.newTechnology, localLogin: overrides.localLogin,
        gameplayGlobals: overrides.gameplayGlobals, surfaceStorage: overrides.surfaceStorage, processKernel: overrides.processKernel, localStateGlobals: overrides.localStateGlobals, connectionActivityGlobals: overrides.connectionActivityGlobals, connectionRegistry: overrides.connectionRegistry, cameraScreenSize: overrides.cameraScreenSize, developerQueue: overrides.developerQueue, motion: overrides.motion,
        developerHeapTrace: overrides.developerHeapTrace, developerQueryDirectDraw: overrides.developerQueryDirectDraw, developerRecordLabel: overrides.developerRecordLabel,
        sound: { wallSeconds: () => 0, play: forbidden('소리'), playOnceAt: forbidden('위치 소리'), playLoopAt: forbidden('반복 소리'), stop: forbidden('정지'), isPlaying: () => false, ...overrides.sound },
        player: { log: overrides.playerLog ?? (() => trace.push('log')) }, construction: overrides.construction, missionOutcome: overrides.missionOutcome, factoryTell: overrides.factoryTell,
        ai: { assertion: forbidden('AI 진단'), ...overrides.ai },
    });
    return { s, trace, queue };
}

// 실제 120BYTE 섬과 저장 객체를 적재한 다음에만 loaded를 공개한다.
test('CampaignSession TheWarBegins: 실제 저장 객체·Graph와 커서까지 Load', () => {
    const { s, trace, queue } = scene();
    try {
        assert.equal(s.loaded, false); s.load(); assert.equal(s.loaded, true);
        const objects = s.world().objects(), priest = objects.find(({ object }) => object.type === 158 && object.owner === 1);
        assert.ok(priest && priest.sid >= 15000); assert.ok(objects.length > 1000); assert.ok(queue.size > 0);
        assert.deepEqual(trace.slice(0, 2), ['config', 'title']); assert.equal(trace.at(-1), 'camera'); assert.ok(trace.includes('production'));
        assert.throws(() => s.load(), /반복|중복/); assert.equal(s.loaded, true);
    } finally { s.dispose(); }
    assert.throws(() => s.load(), /반납/);
});

// 실제 선택 표시와 로컬 전송·경로·Kernel을 같은 Session에서 사제·골렘·벌룬에 연결한다.
test('CampaignSession TheWarBegins: 선택 해제와 세 운반 유닛의 실제 이동', () => {
    // 각 유닛은 독립 Session과 현재 선택 상태를 사용한다.
    for (const [kind, movement, startPoint, destination] of [[158, 54, null, { x: 90, y: 108 }], [133, 40, [94, 108], { x: 90, y: 108 }], [111, 42, [98, 122], { x: 102, y: 122 }]]) {
        const ui = new ClientUserSelectionGlobals(), state = ui.selectionState(), highlight = new RawUserHighlight(ui.highlightState(), {}), received = [], dumps = [];
        let demo = 0, selection;
        const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {},
            observePacket: (_sender, _boss, _local, packet) => { if (packet[1] === 0x40) received.push(packet.slice()); } };
        const steps = [];
        const { s } = scene({ localLogin, deletion: { selected: () => state.selected }, missionOutcome: { demo: () => demo }, sound: { playOnceAt: () => {} }, motion: { path: { stepSound: (...args) => steps.push(args) } }, ai: { participation: { addChat: () => {}, clearChat: () => {} } } });
        try {
            assert.throws(() => s.makeUserSelection(state), /설치 전/);
            s.startLocalConnection(); s.pumpLocalConnection(); s.pollLocalConnectionState(); s.pumpLocalConnection();
            assert.equal(s.localConnectionReady(), true); const core = s.core();
            const parent = startPoint ? s.world().create(kind, 2) : s.world().objects().find(({ object }) => object.type === kind && object.owner === 1).sid;
            if (startPoint) { s.world().pop(parent, ...startPoint, 0); core.owner.set(parent, 1); }
            selection = s.makeUserSelection(state, { paused: () => 0, enabled: () => 1, dump: (sid) => dumps.push(sid), highlight: (mode) => highlight.highlight(mode), assertion: forbidden('선택 진단'), previewAssertion: forbidden('미리보기 진단') });
            assert.equal(selection.select(parent, 0, 0), parent); assert.equal(state.selected, parent); assert.equal(core.pool.slot(parent)[40] & 4, 4); assert.deepEqual(dumps, [parent]);
            demo = 1; assert.equal(selection.select(0, 0, 1), 0); assert.equal(state.selected, parent); demo = 0;
            assert.equal(findAttachedProcess(core.pool, parent, movement), 0); const money = s.sp(1);
            s.requestCarrierMove(parent, destination); assert.equal(state.selected, 0); assert.equal(core.pool.slot(parent)[40] & 4, 0); assert.equal(received.length, 0);
            s.pumpLocalConnection(); assert.equal(received.length, 1); const packet = new DataView(received[0].buffer, received[0].byteOffset, received[0].byteLength);
            assert.deepEqual([...received[0].subarray(0, 4)], [1, 0x40, 18, 0]); assert.equal(packet.getUint16(4, true), parent); assert.deepEqual([packet.getFloat32(6, true), packet.getFloat32(10, true)], [destination.x, destination.y]);
            const form = findAttachedProcess(core.pool, parent, movement), path = core.processes.fromForm(form); assert.ok(path instanceof RawPathProcess);
            const bytes = new DataView(path.runtime.bytes.buffer, path.runtime.bytes.byteOffset, path.runtime.bytes.byteLength);
            s.requestCarrierMove(parent, { x: bytes.getFloat32(0x186dc, true), y: bytes.getFloat32(0x186e0, true) }); s.pumpLocalConnection(); assert.equal(received.length, 1);
            const begin = core.processState.now, end = path.runtime.time(path.runtime.count - 1);
            // 목적지와 결과를 직접 쓰지 않고 실제 프레임으로 자연 종료를 기다린다.
            for (let frame = 0; frame < 3000 && core.processState.now < end + 0.25; frame += 1) {
                const now = begin + frame * 0.025; s.pumpLocalConnection(); s.runFrame({ game: now, wall: 100 + now - begin, delta: frame ? 0.025 : 0, number: frame }, false);
                if (!core.processes.fromForm(form)) break;
            }
            assert.equal(core.processes.fromForm(form), null); const arrived = s.world().objects().find(({ sid }) => sid === parent);
            assert.deepEqual([arrived.object.x, arrived.object.y], [destination.x, destination.y]); assert.equal(s.sp(1), money);
            assert.deepEqual([core.destroy.preDepth, core.destroy.postDepth, core.bookkeeping.depth], [0, 0, 0]);
        } finally { s.dispose(); }
        assert.throws(() => selection.select(0, 0, 1), /반납/);
    }
});

// 실제 Client의 Player/Core/Subscriber 소유자를 공유한 로그인은 현재 Core 포인터를 같은 Player 행으로 갱신한다.
test('CampaignSession TheWarBegins: 공유 Client Core의 실제 로그인 재선택', () => {
    const subscriber = new ClientSubscriberGlobals(), gameplay = new ClientGameplayGlobals(subscriber);
    const players = new ClientPlayerRecords(), coreData = new ClientCoreDataGlobals(); players.bindCoreData(coreData);
    subscriber.mySubscriber().value = subscriber.loginSubscriber().value;
    const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {} };
    const { s } = scene({ localLogin, playerRecords: players, coreDataGlobals: coreData, subscriberGlobals: subscriber, gameplayGlobals: gameplay,
        ai: { participation: { addChat: () => {}, clearChat: () => {} } } });
    try {
        coreData.current = null;
        s.startLocalConnection(); s.pumpLocalConnection(); s.pollLocalConnectionState(); s.pumpLocalConnection();
        assert.equal(s.localConnectionReady(), true); assert.equal(subscriber.mySubscriber().value, 1);
        const rows = players.records();
        assert.equal(coreData.players, rows); assert.equal(coreData.current.buffer, rows.buffer);
        assert.equal(coreData.current.byteOffset - rows.byteOffset, 180 + 0x6c); assert.equal(coreData.active, 0);
    } finally { s.dispose(); }
});

// Init의 실제 Zone 이미지와 subscriber를 세션이 소비하고 다시 생성하지 않는지 확인한다.
test('CampaignSession TheWarBegins: Init 요청의 Zone으로 실제 로그인', () => {
    const request = createMissionFortLoadRequest(), sequence = { value: 0 };
    writeMissionFortRequest('thewarbegins', { next: request.next, sequence, localAddress: Uint8Array.of(122, 108, 111, 99, 97, 108, 44, 42, 0), port: 6799 }, {
        selectFort: (name) => { request.fort = name; }, duplicate: (packet) => captureMissionFortZone(request, packet),
    });
    request.sequence = sequence.value; new DataView(request.zone.buffer, request.zone.byteOffset).setUint32(16, 7, true);
    const players = new ClientPlayerRecords(), gameplay = new ClientGameplayGlobals();
    const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {} };
    const { s } = scene({ localLogin, playerRecords: players, gameplayGlobals: gameplay, missionFortRequest: request, missionSubscriber: 7,
        ai: { participation: { addChat: () => {}, clearChat: () => {} } } });
    try {
        assert.equal(request.consumed, false); assert.equal(request.zone.length, 86);
        s.startLocalConnection(); s.pumpLocalConnection(); s.pollLocalConnectionState(); s.pumpLocalConnection();
        assert.equal(s.localConnectionReady(), true); assert.equal(gameplay.world.mySubscriber, 7);
        assert.equal(new DataView(players.records().buffer, players.records().byteOffset).getUint32(180 + 0x2c, true), 7);
        assert.equal(request.consumed, true); assert.equal(request.zone.length, 0); assert.equal(request.next[0], 7); assert.equal(request.sequence, 1);
    } finally { s.dispose(); }
});

// 실제 외곽 Session은 내부 로그인에서 같은 월드와 수신기를 설치하고 Player·Ready까지 왕복한다.
test('CampaignSession TheWarBegins: 로컬 로그인과 실제 게임 수신 등록 왕복', () => {
    const events = [], logs = [], gameplay = new ClientGameplayGlobals(), players = new ClientPlayerRecords();
    const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: (text) => logs.push(text), refresh: () => events.push(['refresh']), playerUiBusy: () => false, playerNotice: (...args) => events.push(['notice', ...args]) };
    const { s } = scene({ localLogin, playerRecords: players, gameplayGlobals: gameplay, ai: { participation: { addChat: (...args) => events.push(['chat', ...args]), clearChat: (...args) => events.push(['clearChat', ...args]) } } });
    try {
        assert.equal(s.localConnectionReady(), false); assert.equal(s.pollLocalConnectionState(), false);
        assert.equal(s.localConnectionPhase(), 0); assert.deepEqual(s.localConnectionPackets(), []); assert.equal(players.records()[180 + 0x50] & 0x43, 0);
        s.startLocalConnection(); assert.equal(s.localConnectionPhase(), 8); assert.equal(s.loaded, false);
        s.pumpLocalConnection(); assert.equal(s.pollLocalConnectionState(), true); assert.equal(s.localConnectionPhase(), 10);
        s.pumpLocalConnection();
        assert.equal(s.loaded, true); assert.equal(s.localConnectionPhase(), 14); assert.equal(s.localConnectionReady(), true);
        assert.equal(s.localOwner(), 1); assert.equal(s.playerKnown(), 1); assert.equal(gameplay.world.mySubscriber, 1);
        assert.deepEqual(s.localConnectionPackets(), [0x27, 0x29, 0x67, 0x2b, 0x2b, 0x2b, 0x28, 0x81, 0x84, 0x2a, 0x32, 0x6e]);
        assert.equal(events.filter(([kind]) => kind === 'clearChat').length, 6);
        assert.equal(events.filter(([kind]) => kind === 'notice').length, 6);
        assert.ok(events.some(([kind]) => kind === 'refresh'));
        assert.ok(events.some((event) => event[0] === 'chat' && event[1] === 1 && event[2] === 'You' && event[3] === 0x20));
        assert.equal(new DataView(players.records().buffer, players.records().byteOffset).getUint32(180 + 0x2c, true), 1);
        assert.equal(players.records()[180 + 0x50] & 0x43, 0x43);
        assert.deepEqual(logs.filter((line) => line.startsWith('로컬 미션 상태 ')), ['로컬 미션 상태 8\n', '로컬 미션 상태 9\n', '로컬 미션 상태 10\n', '로컬 미션 상태 13\n', '로컬 미션 상태 14\n']);
        // 실제 67h 수신 뒤 원본 기본값과 파생 에너지 설정을 같은 소유자에서 읽는다.
        assert.deepEqual(Array.from({ length: 22 }, (_, index) => s.battleOption(index)), [0, 2, 2, 0, 3, 0, 2, 1, 1, 0, 0, 1, 0, 1, 1, 2, 2, 1, 2, 2, 0, 0]);
        assert.equal(gameplay.energy.radiusSquared, 900); assert.equal(s.core().types[122].cost, 3000);
        const packets = s.localConnectionPackets();
        s.pumpLocalConnection(); assert.equal(s.localConnectionReady(), true); assert.deepEqual(s.localConnectionPackets(), packets);
        const core = s.core(), priest = s.world().objects().find(({ object }) => object.type === 158 && object.owner === 1);
        assert.ok(priest); assert.deepEqual([core.destroy.preDepth, core.destroy.postDepth, core.bookkeeping.depth], [0, 0, 0]);
        const destination = { x: 90, y: 108 }, money = s.sp(1); assert.equal(findAttachedProcess(core.pool, priest.sid, 54), 0);
        s.requestCarrierMove(priest.sid, destination); assert.equal(findAttachedProcess(core.pool, priest.sid, 54), 0);
        s.pumpLocalConnection(); assert.equal(s.localConnectionPackets().at(-1), 0x40);
        const form = findAttachedProcess(core.pool, priest.sid, 54), path = core.processes.fromForm(form);
        assert.ok(path instanceof RawPathProcess); const start = core.processState.now, end = path.runtime.time(path.runtime.count - 1);
        // 같은 로그인과 실제 Kernel을 유지하며 원본처럼 사제 경로의 자연 종료까지 시각만 전진시킨다.
        for (let frame = 0; frame < 1000 && core.processState.now < end + 0.25; frame += 1) {
            const now = start + frame * 0.025; s.pumpLocalConnection();
            s.runFrame({ game: now, wall: 100.25 + now - start, delta: frame ? 0.025 : 0, number: frame }, false);
            if (!core.processes.fromForm(form)) break;
        }
        assert.equal(core.processes.fromForm(form), null);
        const arrived = s.world().objects().find(({ sid }) => sid === priest.sid);
        assert.deepEqual([arrived.object.x, arrived.object.y], [90, 108]); assert.equal(s.sp(1), money);
        assert.deepEqual([core.destroy.preDepth, core.destroy.postDepth, core.bookkeeping.depth], [0, 0, 0]);
        assert.throws(() => s.startLocalConnection(), /새 수명/);
    } finally { s.dispose(); }
    assert.throws(() => s.pumpLocalConnection(), /반납/);
});

// 로그인된 실제 내부 전송은 사제 이동 요청을 지연 송신해 같은 Host의 경로로 연결한다.
test('CampaignSession TheWarBegins: 실제 로그인 전송의 사제 이동 요청', () => {
    const received = [];
    // 관찰기는 실제 내부 전송이 수신한 이동 패킷과 송신자만 복사한다.
    const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {},
        observePacket: (sender, _boss, _local, packet) => { if (packet[1] === 0x40) received.push([sender, packet.slice()]); } };
    const { s } = scene({ localLogin, ai: { participation: { addChat: () => {}, clearChat: () => {} } } });
    try {
        assert.throws(() => s.requestCarrierMove(50, { x: 90, y: 108 }), /미설치/);
        s.startLocalConnection(); s.pumpLocalConnection(); s.pollLocalConnectionState(); s.pumpLocalConnection();
        assert.equal(s.localConnectionReady(), true);
        const priest = s.world().objects().find(({ object }) => object.type === 158 && object.owner === 1);
        assert.ok(priest);
        const core = s.core(), before = s.localConnectionPackets(), destination = { x: 90, y: 108 }, movement = 54;
        assert.equal(findAttachedProcess(core.pool, priest.sid, movement), 0);
        s.requestCarrierMove(priest.sid, destination);
        assert.deepEqual(s.localConnectionPackets(), before); assert.equal(findAttachedProcess(core.pool, priest.sid, movement), 0);
        s.pumpLocalConnection();
        assert.equal(s.localConnectionPackets().at(-1), 0x40);
        assert.equal(received.length, 1); assert.equal(received[0][0], 1);
        const packet = received[0][1], packetView = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
        assert.deepEqual([...packet.subarray(0, 4)], [1, 0x40, 18, 0]); assert.equal(packetView.getUint16(4, true), priest.sid);
        assert.equal(packetView.getFloat32(6, true), destination.x); assert.equal(packetView.getFloat32(10, true), destination.y); assert.equal(packetView.getUint32(14, true), 0);
        const form = findAttachedProcess(core.pool, priest.sid, movement), path = core.processes.fromForm(form);
        assert.ok(form); assert.ok(path instanceof RawPathProcess);
        const view = new DataView(path.runtime.bytes.buffer, path.runtime.bytes.byteOffset, path.runtime.bytes.byteLength);
        const storedDestination = { x: view.getFloat32(0x186dc, true), y: view.getFloat32(0x186e0, true) };
        assert.deepEqual(storedDestination, { x: 89.5, y: 107.5 });
        const packets = s.localConnectionPackets();
        // 원본 float 인자에서 같은 값이 되는 미세한 double 차이도 중복 목적지로 처리한다.
        s.requestCarrierMove(priest.sid, { x: storedDestination.x + 1e-7, y: storedDestination.y }); s.pumpLocalConnection();
        assert.deepEqual(s.localConnectionPackets(), packets); assert.equal(findAttachedProcess(core.pool, priest.sid, movement), form);
        const start = core.processState.now, arrival = path.runtime.time(path.runtime.count - 1), beforeSp = s.sp(1);
        s.runFrame({ game: start, wall: 100, delta: 0, number: 0 }, true);
        assert.equal(findAttachedProcess(core.pool, priest.sid, movement), form);
        // 실제 Kernel을 일정 간격으로 진행하며 경로가 자연스럽게 끝나는 순간까지만 검사한다.
        for (let frame = 0; frame < 1000 && core.processes.fromForm(form); frame += 1) {
            const now = start + frame * 0.025;
            if (now > arrival + 2) break;
            s.pumpLocalConnection(); s.runFrame({ game: now, wall: 100 + now - start, delta: frame ? 0.025 : 0, number: frame }, false);
        }
        assert.equal(core.processes.fromForm(form), null);
        const arrived = s.world().objects().find(({ sid }) => sid === priest.sid);
        assert.deepEqual([arrived.object.x, arrived.object.y], [90, 108]); assert.equal(s.sp(1), beforeSp);
    } finally { s.dispose(); }
    assert.throws(() => s.requestCarrierMove(50, { x: 90, y: 108 }), /반납/);
});

// 편의 로그인 뒤 같은 실제 mutable 송신 관찰기와 원격 Tell 수신을 연속 검증한다.
test('CampaignSession TheWarBegins: 편의 로그인과 mutable 송신·Tell', () => {
    const gameplay = new ClientGameplayGlobals(), received = [], tells = [];
    const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {},
        observePacket: (_sender, _boss, _local, packet) => { if (packet[1] === 0x31 || packet[1] === 0x70) received.push(packet.slice()); } };
    const { s } = scene({ gameplayGlobals: gameplay, localLogin, factoryTell: (text) => tells.push(text), ai: { participation: { addChat: () => {}, clearChat: () => {} } } });
    const text = 'shm 1', packet = new Uint8Array(9 + text.length); packet.set([1, 0x31, 4, 0]);
    new DataView(packet.buffer).setUint32(4, text.length + 1, true); packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
    try {
        assert.throws(() => s.sendMutableLocalPacket(9, packet), /설치 전/);
        s.loadLocalConnection(); assert.equal(s.localConnectionReady(), true);
        s.sendMutableLocalPacket(9, packet); assert.equal(packet[2] | (packet[3] << 8), packet.length);
        assert.equal(received.length, 0); packet[8] = 0x7a;
        s.sendPlayerTell(9, 'TestSection', false); assert.deepEqual(tells, []);
        s.pumpLocalConnection(); assert.equal(gameplay.gameAux().missionSettings()[1], 1);
        assert.deepEqual(received.map((item) => item[1]), [0x31, 0x70]); assert.deepEqual(tells, ['TestSection']);
    } finally { s.dispose(); }
    assert.throws(() => s.sendMutableLocalPacket(9, packet), /반납/);
    const independent = scene().s;
    try { independent.load(); assert.throws(() => independent.sendMutableLocalPacket(9, packet), /로컬 전송 없는/); } finally { independent.dispose(); }
});

// UI의 원본 패킷은 같은 로그인 전송에 복사되며 Pump 전에는 수신되지 않는다.
test('CampaignSession TheWarBegins: 실제 로컬 패킷 송신과 지연 수신', () => {
    const gameplay = new ClientGameplayGlobals(), received = [];
    const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {},
        observePacket: (sender, _boss, _local, packet) => { if (packet[1] === 0x31) received.push([sender, packet.slice()]); } };
    const { s } = scene({ gameplayGlobals: gameplay, localLogin, ai: { participation: { addChat: () => {}, clearChat: () => {} } } });
    const text = 'shm 1', packet = new Uint8Array(9 + text.length);
    packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
    packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8); const original = packet.slice();
    try {
        assert.throws(() => s.sendLocalPacket(9, packet), /설치 전/);
        s.startLocalConnection(); s.pumpLocalConnection(); s.pollLocalConnectionState(); s.pumpLocalConnection();
        const before = s.localConnectionPackets(); s.sendLocalPacket(9, packet);
        assert.deepEqual(s.localConnectionPackets(), before); assert.equal(gameplay.gameAux().missionSettings()[1], 0); assert.equal(received.length, 0);
        packet[8] = 0x7a; s.pumpLocalConnection();
        assert.equal(gameplay.gameAux().missionSettings()[1], 1); assert.equal(received.length, 1);
        assert.equal(received[0][0], 1); assert.deepEqual(received[0][1], original); assert.equal(s.localConnectionPackets().at(-1), 0x31);
    } finally { s.dispose(); }
    assert.throws(() => s.sendLocalPacket(9, original), /반납/);
    const independent = scene().s;
    try { independent.load(); assert.throws(() => independent.sendLocalPacket(9, original), /Login 전송 없는/); } finally { independent.dispose(); }
});

// 공유 Client 시각은 입력 스냅샷이 덮어쓰지 않으며 준비와 정지 실행은 Kernel을 돌리지 않는다.
test('CampaignSession: 공유 시각과 준비·정지·단일 Kernel 실행', () => {
    const gameplay = new ClientGameplayGlobals(), assets = new CampaignSessionAssets(game, { read: forbidden('파일') });
    const metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
    const storage = new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game, authority: gameplay.authority(), gameAux: gameplay.gameAux() });
    const kernel = new Kernel(), clock = { game: 17.25 }, visits = []; storage.processes.bindDevices(kernel, clock);
    const { s } = scene({ gameplayGlobals: gameplay, surfaceStorage: storage, processKernel: kernel }), core = s.core();
    try {
        s.load();
        const probe = core.kernel.add({ runFrame: () => visits.push(core.processState.now) });
        s.prepareFrame({ game: 1, wall: 100, delta: 0, number: 1 }, false);
        assert.equal(clock.game, 17.25); assert.equal(core.processState.now, 17.25); assert.deepEqual(visits, []);
        s.runFrame({ game: 2, wall: 100, delta: 0, number: 2 }, true);
        assert.equal(clock.game, 17.25); assert.deepEqual(visits, []);
        clock.game = 17.5; s.runFrame({ game: 3, wall: 100.25, delta: 0.25, number: 3 }, false);
        assert.equal(clock.game, 17.5); assert.deepEqual(visits, [17.5]); core.kernel.remove(probe);
    } finally { s.dispose(); }
    assert.throws(() => s.prepareFrame({ game: 0, wall: 0, delta: 0, number: 0 }, true), /반납/);
});

// 원본 캠페인 Fort 두 종류를 실제 내부 전송으로 설치하고 각각 독립된 로그인 수명을 확인한다.
test('CampaignSession: GuardMyBack과 DissolvedAlliance 실제 Fort 로그인', () => {
    const candidates = loadOriginalForts(archive), types = createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText));
    // 두 Fort는 데모와 캠페인이 공유하지만 여기서는 데모 Start·AI 행동을 실행하지 않는다.
    for (const name of ['guardmyback', 'dissolvedalliance']) {
        const source = candidates.find((item) => item.source.toLowerCase() === `tarc:\\d\\${name}.fort`);
        assert.ok(source, `${name} 원본 Fort 누락`);
        const input = parseFort(source.bytes, types), players = new ClientPlayerRecords();
        const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {} };
        const { s } = scene({ missionName: name, localLogin, playerRecords: players, ai: { participation: { addChat: () => {}, clearChat: () => {} } } }, input);
        try {
            s.startLocalConnection(); s.pumpLocalConnection(); assert.equal(s.pollLocalConnectionState(), true); s.pumpLocalConnection();
            assert.equal(s.loaded, true, name); assert.equal(s.localConnectionReady(), true, name); assert.equal(s.localConnectionPhase(), 14, name);
            assert.equal(new DataView(players.records().buffer, players.records().byteOffset).getUint32(180 + 0x2c, true), 1, name);
            assert.ok(s.world().objects().length > 0, name);
            const received = s.localConnectionPackets(); assert.equal(received[0], 0x27, name); assert.equal(received.at(-1), 0x6e, name);
            assert.ok(received.includes(0x29) && received.includes(0x28) && received.includes(0x67), name);
        } finally { s.dispose(); }
    }
});

// 실제 GuardMyBack 대본·Fort·로그인에서 같은 Session의 시작 자금과 AI를 설치한다.
test('CampaignSession GuardMyBack: 실제 로그인 뒤 미션 시작과 동맹', () => {
    const files = new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]);
    const gameData = GameData.build({ files, language: 'english' });
    gameData.configuration.assign('missionSpec="d/{local.1}.english"\nfortSpec="d/{local.1}.fort"\n[END]\n');
    // 일반 미션과 데모는 각각 독립된 실제 Session에서 시작한다.
    for (const demo of [0, 1]) {
        const script = new MissionScript(gameData, 'GuardMyBack'), events = [], packets = [], allocations = [];
        const input = parseFort(script.readFortImage(script.fortName()), createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText)));
        const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100.25, clock: () => 100250, log: () => {}, refresh: () => events.push('login'), playerUiBusy: () => false, playerNotice: () => {},
            observePacket: (_sender, _boss, _local, packet) => { if (packet[1] === 0x3d) packets.push(packet.slice()); } };
        const ai = { allocationBytes: (bytes) => bytes.fill(0xa7), defaultName: () => 'AI', participation: { addChat: () => events.push('chat'), clearChat: () => events.push('chat') } };
        const { s } = scene({ missionName: script.fortName(), localLogin, ai }, input);
        try {
            s.startLocalConnection(); s.pumpLocalConnection(); s.pollLocalConnectionState(); s.pumpLocalConnection();
            assert.equal(s.localConnectionReady(), true); assert.equal(s.sp(1), 0);
            const gate = createMissionFrameGateState(), state = createMissionStartState(); gate.demo = demo;
            const effects = { settingsBytes: (owner, bytes) => { allocations.push(owner); bytes.fill(0xa7); }, gameSeconds: () => 123.5, refreshSp: () => events.push('sp'), refreshCommandGump: () => events.push('commands'),
                resetCamera: () => events.push('camera'), randomGeysers: forbidden('예상하지 않은 간헐천') };
            const start = new RawCampaignMissionStartBindings(s, script, gate, state, s.missionStartSettings(), s.missionAiSettings(), effects);
            start.start();
            assert.equal(gate.started, 1); assert.equal(state.sequence, 1); assert.equal(gate.startSeconds, 123.5); assert.equal(gate.demoEndSeconds, 483.5);
            assert.equal(s.sp(1), 1200); assert.ok(s.findTechnology(1, 133));
            assert.equal(s.missionStartSettings().shared.ordinaryGeysers.value, 1); assert.ok(allocations.length >= 3);
            assert.ok(s.participation().brain(2)); assert.ok(s.participation().brain(3)); assert.ok(s.participation().brain(4));
            assert.equal(Boolean(s.participation().brain(1)), demo !== 0); assert.deepEqual(events.filter((item) => ['sp', 'commands', 'camera'].includes(item)), ['sp', 'commands', 'camera']);
            assert.equal(packets.length, 0); s.pumpLocalConnection(); assert.ok(packets.length > 0); assert.ok(events.includes('chat'));
            // 지연 송신된 동맹 패킷은 실제 9바이트 형식이다.
            for (const packet of packets) assert.deepEqual([...packet.subarray(0, 4)], [1, 0x3d, 9, 0]);
            assert.equal(s.localConnectionReady(), true);
            // AI가 참가한 각 Player의 자신에 대한 동맹 비트를 같은 장부에서 확인한다.
            for (const owner of [2, 3, 4]) assert.equal(s.alliances()[owner * 9 + owner], 1);
        } finally { s.dispose(); script.dispose(); }
    }
});

// 실제 로그인과 Start 뒤 사제 메뉴가 같은 Session 커서에 워크샾을 취득한다.
test('CampaignSession TheWarBegins: 실제 로그인 뒤 사제 메뉴와 커서', () => {
    const files = new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]);
    const gameData = GameData.build({ files, language: 'english' });
    gameData.configuration.assign('missionSpec="d/{local.1}.english"\nfortSpec="d/{local.1}.fort"\n[END]\n');
    const script = new MissionScript(gameData, 'TheWarBegins'), events = [], packets = [], mouse = { x: 0, y: 0 };
    const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {},
        observePacket: (sender, _boss, _local, packet) => { if (packet[1] === 0x3e) packets.push([sender, packet.slice()]); } };
    const ai = { allocationBytes: (bytes) => bytes.fill(0xa7), defaultName: () => 'AI', participation: { addChat: () => {}, clearChat: () => {} } };
    const { s } = scene({ localLogin, ai, construction: { refresh: () => events.push(['construction-refresh']) },
        sound: { playOnceAt: (...args) => events.push(['sound', ...args]), play: (...args) => { events.push(['sound', ...args]); return 1; } } });
    try {
        assert.throws(() => s.cursor(), /설치 전/); assert.throws(() => s.makePriestMenu(), /설치 전/);
        s.startLocalConnection(); s.pumpLocalConnection(); s.pollLocalConnectionState(); s.pumpLocalConnection();
        const gate = createMissionFrameGateState(), state = createMissionStartState();
        const effects = { settingsBytes: (_owner, bytes) => bytes.fill(0xa7), gameSeconds: () => 123.5, refreshSp: () => {}, refreshCommandGump: () => {},
            resetCamera: () => {}, randomGeysers: forbidden('예상하지 않은 간헐천') };
        new RawCampaignMissionStartBindings(s, script, gate, state, s.missionStartSettings(), s.missionAiSettings(), effects).start();
        assert.equal(s.sp(1), 3000);
        const cursor = s.cursor(); cursor.snapToGrid.value = 1;
        cursor.setEffects({ lifecycle: { highlight: (value) => events.push(['highlight', value]), selectCursor: (value) => events.push(['cursor', value]), closePopups: () => events.push(['close']),
            translate: (text) => text, measure: (text) => [text.length * 8, 16], mouse: () => mouse },
        placement: { gumpAt: () => 0, invalidate: (...args) => events.push(['dirty', ...args]) },
        drop: { showSp: (...args) => events.push(['sp', ...args]), battleRefresh: () => events.push(['battle']), refresh: (...args) => events.push(['refresh', ...args]), closeGump: (...args) => events.push(['closeGump', ...args]) },
        send: (target, packet) => s.sendLocalPacket(target, packet) });
        const priest = s.world().objects().find(({ object }) => object.type === 158 && object.owner === 1);
        assert.ok(priest); const menu = s.makePriestMenu({ tell: forbidden('건설 실패 안내') });
        assert.equal(menu.selectBuild(85, priest.sid), true); assert.ok(s.cursorDropSettings().preview); assert.equal(s.cursorDropSettings().costFlags, 0x160);
        assert.equal(s.sp(1), 3000); assert.ok(cursor.lifecycle.previewCount > 0);
        const drop = s.cursorDropSettings(), placement = cursor.placement; let target = null;
        // 공급과 전체 배치 판정이 허용한 현재 지면만 실제 커서 위치에 전달한다.
        for (let y = 4; y < 248 && !target; y += 1) {
            // 허용 여부와 좌표는 같은 현재 월드의 판정을 사용한다.
            for (let x = 4; x < 248; x += 1) {
                if (!s.energySupply().evaluate('', x, y, drop.type, drop.localPlayer).value) continue;
                if (!s.mayPlaceConstruction({ type: drop.type, argument: drop.argument, x, y, flags: drop.direction, owner: drop.localPlayer, mode: 1 })) continue;
                mouse.x = x * 16 - placement.screenOriginX() - placement.hotspotX + Math.trunc(placement.imageWidth / 2);
                mouse.y = y * 11 - placement.screenOriginY() - placement.hotspotY + Math.trunc(placement.imageHeight / 2);
                cursor.update(mouse.x, mouse.y); if (drop.legal) { target = { x, y }; break; }
            }
        }
        assert.ok(target, '현재 실제 커서 허용 위치 없음'); assert.deepEqual([drop.x, drop.y], [target.x, target.y]);
        const before = s.localConnectionPackets(); cursor.drop(); assert.equal(s.sp(1), 2200); assert.deepEqual(s.localConnectionPackets(), before); assert.equal(packets.length, 0);
        s.pumpLocalConnection(); assert.ok(s.localConnectionPackets().slice(before.length).includes(0x3e));
        assert.equal(packets.length, 1); assert.equal(packets[0][0], 1); assert.deepEqual([...packets[0][1].subarray(0, 5)], [1, 0x3e, 25, 0, 85]);
        const workshops = s.world().objects().filter(({ object }) => object.type === 85 && object.owner === 1 && object.x === target.x && object.y === target.y);
        assert.equal(workshops.length, 1); assert.equal(s.core().pool.slot(workshops[0].sid)[40] & 1, 1);
    } finally { s.dispose(); script.dispose(); }
});

// 출하된 일반 31미션과 별도 데모 맵의 실제 스크립트가 고른 Fort를 각각 내부 로그인한다.
test('CampaignSession: 출하 미션 32종의 Fort 선택과 내부 로그인', () => {
    const names = [
        'tutorial1', 'tutorial2', 'tutorial3', 'tutorial4', 'tutorial5', 'tutorial6',
        'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76',
        'thewarbegins', 'masterofwhirligigs', 'savetheisland', 'fragilefortune', 'thunderingpower', 'dissolvedalliance',
        'thenoose', 'rainvsrain', 'runforit', 'guardmyback', 'surrounded', 'breakingthrough', 'totherescue', 'vicious', 'enemyterritory', 'finalconfrontation', 'demostormrages',
    ];
    const files = new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]);
    const gameData = GameData.build({ files, language: 'english' });
    gameData.configuration.assign('missionSpec="d/{local.1}.english"\nfortSpec="d/{local.1}.fort"\n[END]\n');
    const types = createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText));
    // 각 스크립트와 Session을 사용한 순서대로 반납해 직전 미션의 저장소를 재사용하지 않는다.
    for (const name of names) {
        const script = new MissionScript(gameData, name);
        try {
            assert.equal(script.loaded, true, `${name} 스크립트 누락`);
            const fortName = script.fortName(), input = parseFort(script.readFortImage(fortName), types);
            const localLogin = { configInt: (_key, fallback) => fallback, wallSeconds: () => 100, clock: () => 1234, log: () => {}, refresh: () => {}, playerUiBusy: () => false, playerNotice: () => {} };
            const sounds = [];
            const { s } = scene({ missionName: fortName, localLogin, sound: { playOnceAt: (...args) => sounds.push(args) }, ai: { participation: { addChat: () => {}, clearChat: () => {} } } }, input);
            try {
                s.startLocalConnection(); s.pumpLocalConnection(); assert.equal(s.pollLocalConnectionState(), true, name); s.pumpLocalConnection();
                assert.equal(s.localConnectionReady(), true, `${name} / ${fortName}`);
                assert.ok(s.world().objects().length > 0, name);
                const packets = s.localConnectionPackets(); assert.equal(packets[0], 0x27, name); assert.equal(packets.at(-1), 0x6e, name);
            } catch (error) { error.message = `${name} / ${fortName}: ${error.message}`; throw error; } finally { s.dispose(); }
        } finally { script.dispose(); }
    }
});

// 실제 Tutorial1의 BridgeTheGap 전체 Session 지형을 독립 65,536바이트 마스크와 비교한다.
test('CampaignSession Tutorial1: BridgeTheGap 실제 Session 지형', () => {
    const source = loadOriginalForts(archive).find((item) => item.source.toLowerCase() === 'tarc:\\d\\bridgethegap.fort');
    assert.ok(source);
    const input = parseFort(source.bytes, createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText))), sounds = [];
    const { s } = scene({ missionName: 'BridgeTheGap', sound: { playOnceAt: (...args) => sounds.push(args) } }, input);
    try {
        s.load(); const pool = s.core().pool, mask = new Uint8Array(65536).fill(255); let count = 0;
        // 같은 풀의 실제 지면과 edgeFarm에서 현재 영역 번호를 마스크에 쓴다.
        for (const item of s.world().objects()) {
            const raw = pool.slot(item.sid); if (raw[10] !== 97 && raw[10] !== 121) continue;
            const region = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint16(8, true);
            mask[item.object.y * 256 + item.object.x] = region; count += 1;
        }
        assert.equal(count, 763);
        assert.equal(createHash('sha256').update(mask).digest('hex'), 'e16dd045520434a91d8ba43d078c7a11f56f3b8a87303ab8a6b2fa2496882584');
        const storage = s.world().storage, islands = new DataView(storage.islands.buffer, storage.islands.byteOffset), layout = layoutFort(input, pieceFrameCodes(data.table));
        assert.equal(islands.getUint32(0, true), 1); assert.equal(layout.islands.islands.length, 1);
        // 각 축의 16개 청크에서 실제 섬·방향·영역 레코드·순번을 타입 모델과 대조한다.
        for (let x = 0; x < 16; x += 1) {
            // 같은 열의 모든 행을 순서대로 검사한다.
            for (let y = 0; y < 16; y += 1) {
                const actual = storage.chunks.at(x, y), expected = layout.map.at(x, y);
                // BYTE 방향과 문자 방향을 같은 문자로 정규화한다.
                const side = (value) => typeof value === 'number' ? String.fromCharCode(value) : value;
                assert.deepEqual([actual.island, side(actual.side), Array.from(actual.record), actual.ordinal], [expected.island, side(expected.side), Array.from(expected.record), expected.ordinal], `${x},${y}`);
            }
        }
    } finally { s.dispose(); }
});

// 외곽 등록은 설치 전에 닫고 Load 뒤 같은 Session 연구 본체만 분배기에 연결한다.
test('CampaignSession TheWarBegins: 설치된 연구 수신 등록', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        assert.throws(() => s.registerFactoryResearchCommands(dispatch), /미설치/); s.load(); s.registerFactoryResearchCommands(dispatch);
        const entry = dispatch.registration(0x58); assert.equal(typeof entry.receive, 'function'); assert.equal(entry.length, 16); assert.equal(entry.version, 1);
    } finally { s.dispose(); }
    assert.throws(() => s.registerFactoryResearchCommands(dispatch), /반납/);
});

// 건설 등록은 같은 Session의 실제 SP와 안내를 사용하며 비활성 정기 소득은 가산하지 않는다.
test('CampaignSession TheWarBegins: 건설 수신과 실제 SP 통지', () => {
    const notices = [], { s } = scene({ localLogin: { status: (...values) => notices.push(values) } });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        assert.throws(() => s.registerConstructionCommands(dispatch), /미설치/); s.load(); s.registerConstructionCommands(dispatch); s.setSp(1, 100);
        // 원본의 건설·통지·상태·회수·승천·거부·SP 등록이 같은 표에 있다.
        for (const type of [0x3e, 0x3f, 0x37, 0x54, 0x51, 0x2f, 0x6c]) assert.equal(typeof dispatch.registration(type).receive, 'function');
        const packet = Uint8Array.of(1, 0x6c, 11, 0, 1, 1, 0, 50, 0, 0, 0);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true); assert.equal(s.sp(1), 150); assert.deepEqual(notices, [[4, 1, 50]]);
        s.setSp(2, 100); packet[5] = 2; assert.equal(dispatch.dispatch(new PacketConnection({ sender: 2 }), packet), true);
        assert.equal(s.sp(1), 200); assert.equal(s.sp(2), 50); assert.deepEqual(notices[1], [4, 2, 50]);
        packet[6] = 2; assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true); assert.equal(s.sp(1), 200); assert.equal(notices.length, 2);
    } finally { s.dispose(); }
    assert.throws(() => s.registerConstructionCommands(dispatch), /반납/);
});

// 직접 수신 등록은 실제 Session의 Player와 현재 Tell 장치를 사용한다.
test('CampaignSession TheWarBegins: 동맹 요청 직접 수신', () => {
    const records = new ClientPlayerRecords(), tells = [], { s } = scene({ playerRecords: records, factoryTell: (text) => tells.push(text) });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        assert.throws(() => s.registerAllianceNegotiationCommands(dispatch), /미설치/); s.load(); s.registerAllianceNegotiationCommands(dispatch);
        // 원본 세 종류는 직접 수신과 6BYTE 길이를 공유한다.
        for (const type of [0x77, 0x79, 0x6f]) { const entry = dispatch.registration(type); assert.equal(typeof entry.directReceive, 'function'); assert.equal(entry.length, 6); }
        const players = records.records(); players.set(Buffer.from('Bob\0'), 2 * 180 + 12); new DataView(players.buffer, players.byteOffset, players.byteLength).setUint32(2 * 180 + 44, 123, true);
        dispatch.invokeDirect(0x77, 2, null, Uint8Array.of(1, 0x77, 6, 0, 2, 1)); assert.deepEqual(tells, ['AllianceRequested(~r,Bob,2)']);
    } finally { s.dispose(); }
    assert.throws(() => s.registerAllianceNegotiationCommands(dispatch), /반납/);
});

// Carrier 앞단은 같은 Client 설정과 카메라 목표를 사용하고 예약 SID 위치 통지를 건너뛴다.
test('CampaignSession TheWarBegins: Carrier 앞단 설정과 카메라 수신', () => {
    const gameplay = new ClientGameplayGlobals(), local = new ClientLocalStateGlobals();
    const { s } = scene({ gameplayGlobals: gameplay, localStateGlobals: local, cameraScreenSize: () => ({ x: 800, y: 600 }) });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        assert.throws(() => s.registerCarrierCommandPrefix(dispatch), /미설치/); s.load(); s.registerCarrierCommandPrefix(dispatch);
        // 원본 앞단의 등록과 길이를 같은 분배 표에서 확인한다.
        for (const type of [0x77, 0x79, 0x6f, 0x31, 0x32, 0x33, 0x34, 0x35, 0x30]) assert.equal(typeof dispatch.registration(type).receive, 'function');
        assert.equal(dispatch.registration(0x33).length, 18); assert.equal(dispatch.registration(0x34).length, 12); assert.equal(dispatch.registration(0x35).length, 20);
        const revision = local.metaRebuild().value, camera = Uint8Array.of(1, 0x34, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0), view = new DataView(camera.buffer);
        view.setFloat32(4, 10, true); view.setFloat32(8, 20, true);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), camera), true);
        assert.deepEqual([...gameplay.camera().target().pixels], [-240, -80]); assert.equal(local.metaRebuild().value, (revision + 1) >>> 0);
        const settings = new Uint8Array(32); settings.set([1, 0x32, 32, 0]); new DataView(settings.buffer).setUint32(4, 0x12345678, true);
        dispatch.dispatch(new PacketConnection({ sender: 1 }), settings); assert.equal(gameplay.gameAux().missionSettings()[0], 0x12345678);
        const reserved = new Uint8Array(18); reserved.set([1, 0x33, 18, 0, 5, 0]); assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), reserved), true);
    } finally { s.dispose(); }
    assert.throws(() => s.registerCarrierCommandPrefix(dispatch), /반납/);
});

// 일반 Form 부모에만 Process를 붙여 6ah 신규·기존 수신을 확인하며 사제 행동이나 희생은 실행하지 않는다.
test('CampaignSession TheWarBegins: Dais 데이터의 실제 Process 신규·갱신', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); const c = s.core(), parent = c.factory.create(6, 2);
        let form = 15001;
        // 같은 Pool의 실제 비어 있는 서버 Form을 찾는다.
        while (form < c.pool.capacity && !(c.pool.slot(form)[11] & 1)) form += 1;
        assert.ok(form < c.pool.capacity); const source = new DaisProcess(), packet = new Uint8Array(32);
        new DataView(source.fields.bytes.buffer).setBigUint64(32, 0x7ff0123456789abcn, true);
        const length = writeDaisProcessData(source.fields, packet, form, parent, true); source.destroy();
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet.subarray(0, length)), true);
        const current = c.processes.fromForm(form); assert.ok(current instanceof DaisProcess); assert.equal(current.parent, parent); assert.equal(c.pool.slot(form)[10], 58);
        assert.equal(new DataView(current.fields.bytes.buffer).getBigUint64(32, true), 0x7ff0123456789abcn);
        const update = new Uint8Array(17), view = new DataView(update.buffer); update.set([1, 0x6a, 17, 0]); view.setUint32(4, form, true); update[8] = 2; view.setBigUint64(9, 0x3ff4000000000000n, true);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), update), true); assert.equal(c.processes.fromForm(form), current);
        assert.equal(new DataView(current.fields.bytes.buffer).getBigUint64(32, true), 0x3ff4000000000000n);
    } finally { s.dispose(); }
});

// 원본 기본 AI 설정의 같은 600BYTE 입력에 명시 이름·기술·초기 SP만 지정한다.
function localAiProfile() {
    const profile = new Uint8Array(600).fill(0xa7);
    new RawMissionAiSettings({ stuffRefreshRate: 3, startingSp: 2500 }, { get: forbidden('AI 설정') }).initialize(profile);
    profile.set(new TextEncoder().encode('Explicit\0'), 0); profile.set(new TextEncoder().encode('sunCannon\0'), 30); new DataView(profile.buffer).setUint32(0x22c, 777, true);
    return profile;
}

// 실제 두 Player 지역에 생성한 간헐천이 같은 AI 장부에 등록되고 AI를 반납한다.
test('CampaignSession TheWarBegins: 로컬 간헐천과 실제 AI 등록', () => {
    const records = new ClientPlayerRecords(), notices = [];
    const options = {
        playerRecords: records, construction: { wallSeconds: () => 1, configInt: () => 0, refresh: () => notices.push('refresh') }, missionOutcome: { demo: () => 0 },
        ai: { allocationBytes: (bytes) => bytes.fill(0x71), participation: { clearChat: () => notices.push('clear'), addChat: () => notices.push('add') } },
    };
    const { s } = scene(options);
    try {
        s.load(); const token = s.createAi(2, localAiProfile()); records.records()[180 + 0x50] = 2; assert.equal(s.prepareAi(2), true);
        const raw = s.aiMemory().bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength), before = view.getUint32(0x32c, true);
        const count = s.world().objects().filter(({ object }) => object.type === 122).length; s.generateLocalGeysers();
        assert.equal(view.getUint32(0x32c, true), before + 6); assert.equal(s.world().objects().filter(({ object }) => object.type === 122).length, count + 6);
        assert.equal(records.records()[180 + 0x50], 2); assert.equal(records.records()[360 + 0x50] & 0x22, 0x20); assert.equal(s.participation().brain(2), token);
        assert.equal(notices.filter((item) => item === 'refresh').length, 6); s.removeAi(2); assert.equal(s.participation().brain(2), 0);
    } finally { s.dispose(); }
});

// 전체 생성은 로컬 생성 뒤 현재 옵션의 세계 배치까지 같은 World·AI 장부에 반영한다.
test('CampaignSession TheWarBegins: 세계 간헐천 전체 생성과 AI 등록', () => {
    const records = new ClientPlayerRecords(), options = new Uint8Array(87), notices = []; options[18] = 1;
    const { s } = scene({
        playerRecords: records, missionOptions: options, construction: { wallSeconds: () => 1, configInt: () => 0, refresh: () => notices.push('refresh') }, missionOutcome: { demo: () => 0 },
        ai: { allocationBytes: (bytes) => bytes.fill(0x71), participation: { clearChat: () => notices.push('clear'), addChat: () => notices.push('add') } },
    });
    try {
        s.load(); const token = s.createAi(2, localAiProfile()); records.records()[180 + 0x50] = 2; assert.equal(s.prepareAi(2), true);
        const raw = s.aiMemory().bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength), before = view.getUint32(0x32c, true);
        const count = s.world().objects().filter(({ object }) => object.type === 122).length; s.generateWorldGeysers();
        assert.equal(view.getUint32(0x32c, true), before + 17); assert.equal(s.world().objects().filter(({ object }) => object.type === 122).length, count + 17);
        assert.equal(s.participation().brain(2), token); assert.ok(notices.includes('refresh')); s.removeAi(2); assert.equal(s.participation().brain(2), 0);
    } finally { s.dispose(); }
});

// 단일 미션은 비미션 보충을 원본 진단에서 중단하며 앞에 객체를 만들지 않는다.
test('CampaignSession TheWarBegins: 비미션 너겟의 단일 미션 진단', () => {
    const { s, queue } = scene({ construction: { wallSeconds: () => 1 } }), diagnostics = [];
    try {
        s.load(); const before = s.world().objects(), pending = queue.size;
        s.world().storage.bindDisplayDiagnostics({ value: '' }, (expression, file, line) => { diagnostics.push([expression, file, line]); throw new Error('단일 미션 진단'); });
        assert.throws(() => s.placeBattleNuggets({ left: 1, top: 1, right: 50, bottom: 50 }, 1), /단일 미션 진단/);
        assert.deepEqual(diagnostics, [['!inSingle', '\\Ns\\O\\Addrandom.cpp', 0xb4]]);
        assert.deepEqual(s.world().objects(), before); assert.equal(queue.size, pending); assert.equal(s.loaded, true);
    } finally { s.dispose(); }
});

// 공개 기술 표시는 실제 생산 부모의 중복 방지와 같은 Battle snapshot을 사용한다.
test('CampaignSession TheWarBegins: Client 기본 69h와 실제 기술 부모', () => {
    const storage = new CampaignTemplateStorage(), gameplay = new ClientGameplayGlobals(), players = new ClientPlayerRecords(), registry = new ClientConnectionGlobals(), chat = new ClientChatWindowGlobals(), local = new ClientLocalStateGlobals(), activity = new ClientConnectionActivityGlobals();
    const { s } = scene({ gameplayGlobals: gameplay, playerRecords: players, templateStorage: storage, fortMission: { current: () => 0 }, production: { allocationBytes: (bytes) => bytes.fill(0xa5), effects: { configInt: () => 0 } } });
    const runtime = { session: () => s }, dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); gameplay.bindRuntimeOwner(s, runtime);
        const surface = { gameAux: () => gameplay.gameAux(), assertDisplay: (...args) => s.world().storage.assertDisplay(...args) };
        installClientPacketDefaults({ gameplayGlobals: () => gameplay, surfaceStorage: () => surface, connectionGlobals: () => registry, playerRecords: () => players, chatWindowGlobals: () => chat, localStateGlobals: () => local, connectionActivityGlobals: () => activity }, dispatch);
        const packet = Uint8Array.of(1, 0x69, 7, 0, 2, 128, 0);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(gameplay.runtimeCallActive(s), false); assert.equal(gameplay.gameAux().typeUse()[0], 128);
        s.snapshotBattle(1); assert.deepEqual([...fortSection(storage.template(1), 'Technology')], [1, 128, 4]);
    } finally { s.dispose(); }
});

// 공개 기술 표시는 실제 생산 부모의 중복 방지와 같은 Battle snapshot을 사용한다.
test('CampaignSession TheWarBegins: 공개 기술 표시와 실제 생산 부모', () => {
    const storage = new CampaignTemplateStorage();
    const { s } = scene({ templateStorage: storage, fortMission: { current: () => 0 }, production: { allocationBytes: (bytes) => bytes.fill(0xa5), effects: { configInt: () => 0 } } });
    try {
        s.load(); s.indicateTechnology(128); s.indicateTechnology(128); s.snapshotBattle(1);
        assert.deepEqual([...fortSection(storage.template(1), 'Technology')], [1, 128, 4]);
    } finally { s.dispose(); }
    assert.throws(() => s.indicateTechnology(128), /반납/);
});

// 실제 기술 부모에 같은 종류를 한 번만 넣고 반복 획득에도 현재 Battle snapshot과 안내를 수행한다.
test('CampaignSession TheWarBegins: 실제 기술 획득과 반복 snapshot', () => {
    const storage = new CampaignTemplateStorage(), events = [];
    const options = {
        templateStorage: storage, fortMission: { current: () => 0 }, production: { allocationBytes: (bytes) => bytes.fill(0xa5), effects: { configInt: () => 0 } },
        newTechnology: { name: (type) => game.types()[type - 70].definition.name, configure: (key, value) => events.push([key, value]), format: (pattern, type) => pattern.replace('%d', String(type)), tell: (name) => events.push(['tell', name]) },
    };
    const { s } = scene(options), type = game.typeTable.find('sunCannon');
    try {
        s.load(); assert.equal(type, 128); s.learnNewTechnology(type);
        const first = fortSection(storage.template(1), 'Technology').slice(), firstMoney = fortSection(storage.template(1), 'Money').slice();
        assert.deepEqual([...first], [1, 128, 4]); s.learnNewTechnology(type);
        assert.deepEqual(fortSection(storage.template(1), 'Technology'), first); assert.notDeepEqual(fortSection(storage.template(1), 'Money'), firstMoney);
        assert.deepEqual(events, [['global.newTechName', 'suncannon'], ['global.newTechPic', '<!`"128.*`">'], ['tell', 'NewTech'], ['global.newTechName', 'suncannon'], ['global.newTechPic', '<!`"128.*`">'], ['tell', 'NewTech']]);
    } finally { s.dispose(); }
});

// 실제 Battle snapshot의 저장 자식과 재적재 객체를 같은 자료로 비교한다. SID는 비교하지 않는다.
test('CampaignSession TheWarBegins: 실제 Battle snapshot과 재적재', () => {
    const storage = new CampaignTemplateStorage();
    const options = { templateStorage: storage, fortMission: { current: () => 0 }, production: { allocationBytes: (bytes) => bytes.fill(0xa5), effects: { configInt: () => 0 } } };
    const first = scene(options); let saved, before;
    try {
        first.s.load(); before = first.s.world().objects(); first.s.snapshotBattle(1);
        const value = storage.template(1); assert.ok(fortSection(value, 'Terr00').length > 1); assert.ok(fortSection(value, 'Terr01').length > 1);
        assert.equal(fortSection(value, 'Territory').length, 120); assert.equal(fortSection(value, 'Money').length, 4);
        saved = { ...value, sections: value.sections.map((bytes) => bytes.slice()) };
    } finally { first.s.dispose(); }
    const second = scene({ ...options, templateStorage: new CampaignTemplateStorage() }, saved);
    try {
        second.s.load(); const after = second.s.world().objects();
        // SID는 적재 순서에 좌우되므로 표시 자료의 다중집합을 비교한다.
        const keys = (objects) => objects.map(({ object }) => `${object.type},${object.owner},${object.frame},${object.x},${object.y}`).sort();
        assert.equal(second.s.loaded, true); assert.equal(before.length, 1875); assert.deepEqual(keys(after), keys(before));
    } finally { second.s.dispose(); }
});

// 중간 외부 장치 오류는 loaded를 켜지 않고 원문의 앞선 설치를 보존한다.
test('CampaignSession Load: 실패 접두를 유지하고 완료를 가장하지 않는다', () => {
    const { s } = scene({ install: { translateYou: forbidden('번역') } });
    try { assert.throws(() => s.load(), /번역/); assert.equal(s.loaded, false); assert.throws(() => s.load(), /반복|중복/); }
    finally { s.dispose(); }
});

// 일반 Form의 검증 패킷을 같은 Session 분배기와 공유 검증 DWORD에 연결한다.
test('CampaignSession TheWarBegins: 공통 객체 검증 수신과 현재 전역', () => {
    const gameplay = new ClientGameplayGlobals(), logs = [], { s } = scene({ gameplayGlobals: gameplay, playerLog: (level, message) => logs.push([level, message]) });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); const c = s.core(), sid = c.factory.create(6, 2), before = c.pool.slot(sid), aux = gameplay.gameAux();
        const entry = dispatch.registration(0x2c); assert.equal(entry.length, 35); assert.equal(entry.version, 1); assert.equal(typeof entry.receive, 'function');
        aux.verifySid = sid; const packet = new Uint8Array(35); packet.set([1, 0x2c, 35, 0]);
        new DataView(packet.buffer).setUint16(4, sid, true); packet[16] = before[10] + 1; packet[17] = before[11] & 1; packet[18] = before[34];
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true); assert.equal(aux.verifySid, 0); assert.deepEqual(c.pool.slot(sid), before);
        assert.equal(logs.length, 1); assert.equal(logs[0][0], 0x1d); assert.match(logs[0][1], /VERIFY .*: type /);
    } finally { s.dispose(); }
});

// 일반 Form 부모에만 총 Process를 붙이고 기존 PID의 필드를 갱신한다. 발사·유닛 행동은 실행하지 않는다.
test('CampaignSession TheWarBegins: 총 데이터의 실제 Process 신규·갱신', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); const c = s.core(), parent = c.factory.create(6, 2);
        let form = 15001;
        // 같은 Pool의 비어 있는 서버 Form을 선택한다.
        while (form < c.pool.capacity && !(c.pool.slot(form)[11] & 1)) form += 1;
        assert.ok(form < c.pool.capacity); const packet = new Uint8Array(31), view = new DataView(packet.buffer);
        packet.set([1, 0x3a, 31, 0]); view.setUint32(4, form, true); packet[8] = 0xff; view.setUint32(9, parent, true);
        view.setFloat64(13, 12.5, true); view.setUint16(21, 321, true); view.setUint32(23, 5, true); view.setInt32(27, -5, true);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        const current = c.processes.fromForm(form); assert.ok(current instanceof GunProcess); assert.equal(current.parent, parent); assert.equal(c.pool.slot(form)[10], 34);
        assert.deepEqual([current.fields.time, current.fields.target, current.fields.stage, current.fields.group], [12.5, 321, 5, -5]);
        const update = new Uint8Array(13), changed = new DataView(update.buffer); update.set([1, 0x3a, 13, 0]); changed.setUint32(4, form, true); update[8] = 0x10; changed.setInt32(9, 7, true);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), update), true); assert.equal(c.processes.fromForm(form), current); assert.equal(current.fields.group, 7); assert.equal(current.parent, parent);
        const registration = dispatch.registration(0x3a); assert.equal(registration.length, 0xffff); assert.equal(registration.version, 1);
        let unused = form + 1;
        // 실패할 신규 수신은 실제 빈 Form과 Kernel 수명을 유지한다.
        while (unused < c.pool.capacity && !(c.pool.slot(unused)[11] & 1)) unused += 1;
        assert.ok(unused < c.pool.capacity); const count = c.kernel.size, short = new Uint8Array(9);
        short.set([1, 0x3a, 9, 0]); new DataView(short.buffer).setUint32(4, unused, true); short[8] = 0xff;
        assert.throws(() => dispatch.dispatch(new PacketConnection({ sender: 1 }), short), /dirty 필드 부족/);
        assert.equal(c.kernel.size, count); assert.equal(c.pool.slot(unused)[11] & 1, 1);
    } finally { s.dispose(); }
});

// 일반 Form을 부모로 발사·회전 데이터만 수신하며 프레임과 발사는 실행하지 않는다.
test('CampaignSession TheWarBegins: 총 애니메이션 Process 수신', () => {
    const gameplay = new ClientGameplayGlobals(), { s } = scene({ gameplayGlobals: gameplay });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); const c = s.core(), parent = c.factory.create(6, 2), aux = gameplay.gameAux(); aux.sunCannonType = 6;
        // 두 메시지는 현재 Host에 서로 다른 신규 Form을 부착한다.
        for (const [message, processType] of [[0x3b, aux.fireAnimationProcessType], [0x3c, aux.rotateAnimationProcessType]]) {
            let form = 15001;
            // 현재 비어 있는 서버 Form을 찾는다.
            while (form < c.pool.capacity && !(c.pool.slot(form)[11] & 1)) form += 1;
            assert.ok(form < c.pool.capacity); const packet = new Uint8Array(25), view = new DataView(packet.buffer);
            packet.set([1, message, 25, 0]); view.setUint32(4, form, true); packet[8] = 0xff; view.setUint32(9, parent, true);
            view.setUint16(13, parent, true); view.setUint16(15, 321, true); view.setFloat64(17, 12.5, true);
            const entry = dispatch.registration(message); assert.equal(entry.length, 0xffff); assert.equal(entry.version, 1);
            assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
            const current = c.processes.fromForm(form); assert.equal(current.parent, parent); assert.equal(c.pool.slot(form)[10], processType);
            assert.equal(current.fields.target, 321); assert.equal(current.fields.time, 12.5);
            const update = new Uint8Array(11), changed = new DataView(update.buffer); update.set([1, message, 11, 0]); changed.setUint32(4, form, true); update[8] = 4; changed.setUint16(9, 123, true);
            assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), update), true); assert.equal(c.processes.fromForm(form), current); assert.equal(current.fields.target, 123);
        }
    } finally { s.dispose(); }
});

// 실제 Session 분배기의 두 원격 삭제 등록을 확인하고 비대상 빈 입력만 전달한다.
test('CampaignSession TheWarBegins: 원격 삭제 등록과 빈 수신', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        assert.equal(dispatch.registration(0x53).length, 6); assert.equal(dispatch.registration(0x62).length, 0xffff);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), Uint8Array.of(1, 0x53, 6, 0, 0, 0)), true);
        const none = Uint8Array.of(1, 0x62, 8, 0, 255, 255, 255, 255);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), none), true);
    } finally { s.dispose(); }
});

// 실제 Session의 같은 빈 Form SID로 검증 시작·free 응답과 두 장기 DWORD를 확인한다.
test('CampaignSession TheWarBegins: 객체 검증 등록과 공유 상태', () => {
    const gameplay = new ClientGameplayGlobals(), { s } = scene({ gameplayGlobals: gameplay });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); const c = s.core(), aux = gameplay.gameAux(); let sid = 15001;
        // 사용하지 않은 서버 슬롯을 고르고 실제 free TYPE을 그대로 보낸다.
        while (sid < c.pool.capacity && !(c.pool.slot(sid)[11] & 1)) sid += 1;
        assert.ok(sid < c.pool.capacity); const view = c.pool.slot(sid), begin = new Uint8Array(6), free = new Uint8Array(7);
        begin.set([1, 0x4b, 6, 0]); new DataView(begin.buffer).setUint16(4, sid, true);
        free.set([1, 0x4a, 7, 0]); new DataView(free.buffer).setUint16(4, sid, true); free[6] = view[10];
        assert.equal(dispatch.registration(0x4a).length, 7); assert.equal(dispatch.registration(0x4b).length, 6);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), begin), true); assert.deepEqual([aux.verifySid, aux.previousVerifySid], [sid, sid]);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), free), true); assert.deepEqual([aux.verifySid, aux.previousVerifySid], [0, sid]);
        assert.deepEqual(c.pool.slot(sid), view);
    } finally { s.dispose(); }
});

// 일반 Form 부모와 내용물 TYPE 0만 사용해 2dh가 같은 Pool의 정렬 부착에 도달하는지 확인한다.
test('CampaignSession TheWarBegins: 일반 Form 내용물 수신', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const c = s.core(), parent = c.factory.create(6);
        assert.ok(parent >= c.pool.layout.serverFirst);
        let form = c.pool.layout.serverFirst;
        // 현재 비어 있는 서버 슬롯을 고른다.
        while (form < c.pool.capacity && !(c.pool.slot(form)[11] & 1)) form += 1;
        assert.ok(form < c.pool.capacity); const packet = new Uint8Array(18), view = new DataView(packet.buffer);
        packet.set([1, 0x2d, 18, 0]); view.setUint16(4, form, true); view.setUint16(14, parent, true); packet[17] = 0x10;
        assert.equal(dispatch.registration(0x2d).length, 18); assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        const child = c.pool.slot(form), childView = new DataView(child.buffer, child.byteOffset, child.byteLength), parentRaw = c.pool.slot(parent);
        assert.equal(child[10], 6); assert.equal(child[11] & 8, 8); assert.equal(childView.getUint16(16, true), parent);
        assert.equal(new DataView(parentRaw.buffer, parentRaw.byteOffset, parentRaw.byteLength).getUint16(6, true), form);
    } finally { s.dispose(); }
});

// 실제 분배기가 현재 Session 공장에 감시 Form을 생성하고 부모에 부착한다.
test('CampaignSession TheWarBegins: 감시 Form 수신', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const c = s.core(), parent = c.factory.create(6);
        let form = c.pool.layout.serverFirst;
        // 같은 서버 Pool의 비어 있는 슬롯을 고른다.
        while (form < c.pool.capacity && !(c.pool.slot(form)[11] & 1)) form += 1;
        assert.ok(form < c.pool.capacity);
        const packet = new Uint8Array(13), view = new DataView(packet.buffer);
        packet.set([1, 0x65, 13, 0]); view.setUint16(4, form, true); view.setUint16(6, parent, true);
        view.setUint16(9, 12, true); view.setUint16(11, 34, true);
        c.processState.server = false;
        assert.equal(dispatch.registration(0x65).length, 13);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        const child = c.pool.slot(form), fields = new DataView(child.buffer, child.byteOffset, child.byteLength);
        assert.equal(child[10], 1); assert.equal(fields.getUint16(16, true), parent);
        assert.deepEqual([fields.getUint16(18, true), fields.getUint16(20, true)], [12, 34]);
    } finally { s.dispose(); }
});

// 빈 감시 슬롯의 삭제 통지는 같은 Session의 로그와 SP 감사까지 도달한다.
test('CampaignSession TheWarBegins: 일반 삭제 수신', () => {
    const logs = [], { s } = scene({ playerLog: (...args) => logs.push(args) });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const c = s.core(); let sid = c.pool.layout.serverFirst;
        // 비어 있는 슬롯을 찾아 중복 삭제 경로만 검증한다.
        while (sid < c.pool.capacity && !(c.pool.slot(sid)[11] & 1)) sid += 1;
        assert.ok(sid < c.pool.capacity); c.pool.physical(sid)[10] = 1;
        const packet = new Uint8Array(8); packet.set([1, 0x2e, 8, 0]); new DataView(packet.buffer).setUint16(4, sid, true);
        c.processState.boss = false; logs.length = 0;
        assert.equal(dispatch.registration(0x2e).length, 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(logs.length, 1); assert.match(logs[0][1], /^destroying /);
    } finally { s.dispose(); }
});

// 공유 예약 수신은 현재 Session의 Host와 같은 부모 체인을 사용한다.
test('CampaignSession TheWarBegins: 공유 Regular 수신', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const c = s.core(), parent = c.factory.create(6), packet = new Uint8Array(28), view = new DataView(packet.buffer);
        packet.set([1, 0x7e, 28, 0]); view.setUint16(6, parent, true); view.setFloat64(8, 123.5, true);
        view.setUint32(16, 0x12345678, true); view.setFloat32(20, 1.25, true); view.setUint32(24, 7, true);
        assert.equal(dispatch.registration(0x7e).length, 28);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        const process = c.processes.findEvent(parent, 0x12345678);
        assert.ok(process); assert.deepEqual([process.time, process.payload, process.count], [123.5, 1.25, 7]);
    } finally { s.dispose(); }
});

// 압축 검증 패킷은 같은 소유자 캐시와 공통 객체 검증 상태를 통과한다.
test('CampaignSession TheWarBegins: 압축 객체 수신', () => {
    const gameplay = new ClientGameplayGlobals(), logs = [], { s } = scene({ gameplayGlobals: gameplay, playerLog: (...args) => logs.push(args) });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const c = s.core(), sid = c.factory.create(6, 2), before = c.pool.slot(sid), aux = gameplay.gameAux();
        aux.verifySid = sid; const packet = new Uint8Array(12), view = new DataView(packet.buffer);
        packet.set([1, 0x7a, 12, 0]); view.setUint16(4, sid, true); view.setUint16(6, 0x130, true);
        packet[8] = 3; packet[9] = before[10] + 1; packet[10] = before[11] & 1; packet[11] = before[34];
        assert.equal(dispatch.registration(0x7a).length, 0xffff);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(aux.verifySid, 0); assert.equal(aux.squidTransmission.lastOwner, before[34]);
        assert.deepEqual(c.pool.slot(sid), before); assert.match(logs.at(-1)[1], /VERIFY .*: type /);
    } finally { s.dispose(); }
});

// 원격 Tell은 현재 소비자에 전달하고 빈 간헐천 슬롯은 부작용 없이 건너뛴다.
test('CampaignSession TheWarBegins: 원격 Tell과 간헐천 채집 등록', () => {
    const messages = [], { s } = scene({ factoryTell: (text) => messages.push(text) });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const tell = new Uint8Array(14); tell.set([1, 0x70, 14, 0, 6, 0, 0, 0, 72, 101, 108, 108, 111, 0]);
        assert.equal(dispatch.registration(0x70).length, 0xffff);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), tell), true); assert.deepEqual(messages, ['Hello']);
        const c = s.core(); let sid = c.pool.layout.serverFirst;
        // 채집 대상이 이미 반환된 빈 슬롯을 찾는다.
        while (sid < c.pool.capacity && !(c.pool.slot(sid)[11] & 1)) sid += 1;
        assert.ok(sid < c.pool.capacity); const mined = new Uint8Array(6); mined.set([1, 0x5b, 6, 0]); new DataView(mined.buffer).setUint16(4, sid, true);
        assert.equal(dispatch.registration(0x5b).version, 1);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), mined), true);
        assert.equal(c.pool.slot(sid)[11] & 1, 1);
    } finally { s.dispose(); }
});

// 반환된 대상의 네 운반 명령은 같은 Session에서 안전하게 수신을 끝낸다.
test('CampaignSession TheWarBegins: 운반 명령 네 종류 등록', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const c = s.core(); c.processState.server = true; c.processState.boss = true;
        let sid = c.pool.layout.serverFirst;
        // 선택한 슬롯이 네 수신기의 공통 free 관문에 걸리도록 한다.
        while (sid < c.pool.capacity && !(c.pool.slot(sid)[11] & 1)) sid += 1;
        assert.ok(sid < c.pool.capacity);
        // 각 패킷을 해당 원본 고정 길이로 보내며 같은 슬롯이 빈 상태로 남는지 확인한다.
        for (const [type, length] of [[0x40, 18], [0x4f, 9], [0x50, 9], [0x60, 10]]) {
            const packet = new Uint8Array(length), view = new DataView(packet.buffer);
            packet.set([1, type, length, 0]); view.setUint16(4, sid, true); view.setUint16(6, sid, true);
            assert.equal(dispatch.registration(type).length, length);
            assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
            assert.equal(c.pool.slot(sid)[11] & 1, 1);
        }
    } finally { s.dispose(); }
});

// 주문 패킷은 현재 Session의 빈 대상 관문과 타이머 권한 진단으로 전달된다.
test('CampaignSession TheWarBegins: AcidZap과 주문 수신 등록', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const c = s.core(); let sid = c.pool.layout.serverFirst;
        // 반환된 슬롯으로 각 실제 본문의 빈 대상 관문을 확인한다.
        while (sid < c.pool.capacity && !(c.pool.slot(sid)[11] & 1)) sid += 1;
        assert.ok(sid < c.pool.capacity);
        // 효과를 발생시키지 않는 빈 대상 패킷 세 종류를 분배한다.
        for (const [type, length] of [[0x61, 11], [0x5c, 6], [0x5f, 6]]) {
            const packet = new Uint8Array(length); packet.set([1, type, length, 0]); new DataView(packet.buffer).setUint16(4, sid, true);
            assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        }
        assert.equal(dispatch.registration(0x61).length, 11); assert.equal(dispatch.registration(0x5c).length, 6);
        assert.equal(dispatch.registration(0x5f).length, 0xffff); assert.equal(dispatch.registration(0x7d).length, 0xffff);
        c.processState.boss = true;
        const timer = new Uint8Array(6); timer.set([1, 0x7d, 6, 0]); new DataView(timer.buffer).setUint16(4, sid, true);
        assert.throws(() => dispatch.dispatch(new PacketConnection({ sender: 1 }), timer), /타이머 수신 진단/);
    } finally { s.dispose(); }
});

// 의식·기도·Meltdown의 등록과 반환된 슬롯 관문만 확인하고 실제 의식은 실행하지 않는다.
test('CampaignSession TheWarBegins: 의식과 Meltdown 수신 등록', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const c = s.core(); let sid = c.pool.layout.serverFirst;
        // 실제 Pool의 반환된 슬롯을 선택한다.
        while (sid < c.pool.capacity && !(c.pool.slot(sid)[11] & 1)) sid += 1;
        assert.ok(sid < c.pool.capacity); const saved = c.pool.slot(sid), raw = c.pool.physical(sid);
        // 효과가 발생하지 않는 두 반환 슬롯 경로만 분배한다.
        for (const [type, length] of [[0x6b, 10], [0x6d, 8]]) {
            const packet = new Uint8Array(length); packet.set([1, type, length, 0]); new DataView(packet.buffer).setUint16(4, sid, true);
            assert.equal(dispatch.registration(type).length, length);
            assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        }
        raw[11] |= 2;
        const request = new Uint8Array(6); request.set([1, 0x76, 6, 0]); new DataView(request.buffer).setUint16(4, sid, true);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), request), true);
        assert.equal(dispatch.registration(0x75).length, 0xffff); assert.equal(dispatch.registration(0x76).length, 6);
        raw.set(saved);
    } finally { s.dispose(); }
});

// 개발자 패킷의 설정 분기는 같은 Session의 장기 GameAux를 바꾼다.
test('CampaignSession TheWarBegins: 개발자 설정 패킷 수신', () => {
    const gameplay = new ClientGameplayGlobals(), { s } = scene({ gameplayGlobals: gameplay });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const text = 'shm 1', packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(gameplay.gameAux().missionSettings()[1], 1);
        const missing = new Uint8Array(11); missing.set([1, 0x31, 11, 0, 3, 0, 0, 0, 122, 122, 0]);
        assert.throws(() => dispatch.dispatch(new PacketConnection({ sender: 1 }), missing), /나머지 해석기 미연결/);
    } finally { s.dispose(); }
});

// Client 기본 종료 통지가 현재 실제 Session의 Player 방송까지 같은 기록으로 전달된다.
test('CampaignSession TheWarBegins: Client 연결 종료와 실제 Player 방송', () => {
    const gameplay = new ClientGameplayGlobals(), players = new ClientPlayerRecords(), registry = new ClientConnectionGlobals();
    const local = new ClientLocalStateGlobals(), activity = new ClientConnectionActivityGlobals(), chat = new ClientChatWindowGlobals(), calls = [];
    const { s } = scene({ gameplayGlobals: gameplay, playerRecords: players, connectionRegistry: registry, localStateGlobals: local, connectionActivityGlobals: activity });
    const runtime = { session: () => s }, dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); gameplay.bindRuntimeOwner(s, runtime); gameplay.server().value = true; s.core().processState.server = true;
        const bytes = players.records(), view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        view.setUint32(180 + 0x50, 1, true); view.setUint32(2 * 180 + 0x50, 0x43, true); view.setUint32(2 * 180 + 0x2c, 7, true); bytes.set([66, 0], 2 * 180 + 12);
        registry.disconnect = (id) => calls.push(['disconnect', id]); registry.sendMutable = (target, packet) => calls.push(['send', target, packet[1], packet[0x32]]);
        const surface = { gameAux: () => gameplay.gameAux(), assertDisplay: (...args) => s.world().storage.assertDisplay(...args) };
        installClientPacketDefaults({ gameplayGlobals: () => gameplay, surfaceStorage: () => surface, connectionGlobals: () => registry, playerRecords: () => players, chatWindowGlobals: () => chat, localStateGlobals: () => local, connectionActivityGlobals: () => activity }, dispatch);
        activity.frames()[2] = 45; dispatch.registration(2).directReceive(2, null, new Uint8Array());
        assert.equal(view.getUint32(2 * 180 + 0x50, true), 0x102); assert.equal(activity.frames()[2], 0);
        assert.deepEqual(calls, [['disconnect', 2], ['send', 1, 0x2b, 2]]); assert.equal(gameplay.runtimeCallActive(s), false);
    } finally { s.dispose(); }
});

// 실제 Session의 연결 실패 정리와 Player 방송은 같은 현재 기록을 사용한다.
test('CampaignSession TheWarBegins: 연결 실패 정리와 현재 Player 방송', () => {
    const players = new ClientPlayerRecords(), registry = new ClientConnectionGlobals(), sent = [];
    const { s } = scene({ playerRecords: players, connectionRegistry: registry });
    try {
        s.load(); const core = s.core(), bytes = players.records(), view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        view.setUint32(180 + 0x50, 0x49, true); view.setUint32(2 * 180 + 0x50, 1, true); view.setUint32(3 * 180 + 0x50, 1, true);
        view.setUint32(180 + 0x2c, 7, true); bytes.set([65, 0], 180 + 12);
        s.saveServerFailure(); assert.equal(view.getUint32(180 + 0x50, true), 0x108);
        core.processState.server = true;
        registry.sendMutable = (target, packet, current) => { assert.equal(current, bytes); sent.push([target, packet.slice()]); view.setUint32(3 * 180 + 0x50, 0, true); };
        s.broadcastConnectionPlayer(1);
        assert.equal(sent.length, 1); assert.equal(sent[0][0], 2); assert.equal(sent[0][1][1], 0x2b); assert.equal(sent[0][1][0x32], 1);
    } finally { s.dispose(); }
    assert.throws(() => s.saveServerFailure(), /반납/);
});

// 실제 31h 분배는 같은 Player 기록과 현재 추적 소비자를 사용한다.
test('CampaignSession TheWarBegins: 개발자 Player 진단 패킷', () => {
    const playerRecords = new ClientPlayerRecords(), logs = [], { s } = scene({ playerRecords });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); s.world().storage.bindGraphTrace((...args) => logs.push(args));
        const players = playerRecords.records(), view = new DataView(players.buffer, players.byteOffset, players.byteLength);
        players.set([65, 0], 180 + 12); view.setUint32(180 + 0x50, 0x23, true);
        const packet = new Uint8Array(11); packet.set([1, 0x31, 11, 0, 3, 0, 0, 0, 100, 112, 0]);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.ok(logs.some(([level, text]) => level === 29 && text === 'id:1 A Logged in\n'));
    } finally { s.dispose(); }
});

// 실제 31h 수신이 같은 월드 저장소의 스캔 주기와 현재 시각을 갱신한다.
test('CampaignSession TheWarBegins: 개발자 다리 스캔 주기 패킷', () => {
    const logs = [], { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); const storage = s.world().storage;
        storage.bindGraphClock(() => 12.25); storage.bindGraphTrace((...args) => logs.push(args));
        const text = 'sbkp 1.2', packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(storage.bridgeDecayPeriod, Math.fround(1.2));
        assert.deepEqual(storage.bridgeDecayScan, { first: 15000, last: 23001, cursor: 15000, next: 12.25 + Math.fround(1.2) });
        assert.deepEqual(logs, [[29, 'bkPERIOD now 1.200000\nresetting bridge killer\n']]);
    } finally { s.dispose(); }
});

// 캠페인 31h Subscriber 명령은 공유 Client 이름과 ID를 읽고 Fort를 변경하지 않는다.
test('CampaignSession TheWarBegins: 개발자 Subscriber 진단 패킷', () => {
    const subscriber = new ClientSubscriberGlobals(), gameplay = new ClientGameplayGlobals(subscriber), logs = [];
    subscriber.mySubscriber().value = 0xfffffffe; subscriber.handle().set([65, 66, 0]);
    const { s } = scene({ subscriberGlobals: subscriber, gameplayGlobals: gameplay });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); s.world().storage.bindGraphTrace((...args) => logs.push(args));
        subscriber.mySubscriber().value = 0xfffffffe; subscriber.handle().set([65, 66, 0]); // Load 이후 현재 공유 칸을 변경한다.
        const text = 'ssi 3 Name', packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(subscriber.mySubscriber().value, 0xfffffffe); assert.deepEqual([...subscriber.handle().subarray(0, 3)], [65, 66, 0]);
        assert.deepEqual(logs, [[29, 'subid = -2 handle = AB\n']]);
    } finally { s.dispose(); }
});

// 동맹 명령은 같은 Player 참여 상태의 방향 비트와 표를 갱신한다.
test('CampaignSession TheWarBegins: 개발자 동맹 변경 패킷', () => {
    const sends = [], playerRecords = new ClientPlayerRecords(), { s } = scene({ playerRecords, ai: { participation: { sendAlliance: (...args) => sends.push(args) } } });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        const players = playerRecords.records(), view = new DataView(players.buffer, players.byteOffset, players.byteLength);
        const allies = playerRecords.alliances(); view.setUint32(180 + 0x54, view.getUint32(180 + 0x54, true) & ~4, true); allies[11] = 0;
        const text = 'cally 1 2', packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(view.getUint32(180 + 0x54, true) & 4, 4); assert.equal(allies[11], 1);
        assert.equal(sends.length, 1); assert.equal(sends[0][0], 9); assert.equal(sends[0][1], 1); assert.equal(sends[0][2] & 4, 4);
    } finally { s.dispose(); }
});

// 개발자 그래프 진단은 같은 Session의 현재 그래프 기록을 출력한다.
test('CampaignSession TheWarBegins: 개발자 그래프 진단 패킷', () => {
    const logs = [], { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); s.world().storage.bindGraphTrace((...args) => logs.push(args));
        const record = s.world().storage.graph.records[0]; record.surfaces = 7; record.inUse = 1;
        const text = 'dg 0', packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.deepEqual(logs, [[29, 'graph index: 0 numSurface: 7 inUse: 1\n']]);
    } finally { s.dispose(); }
});

// 개발자 삭제 패킷은 같은 Session의 반환된 슬롯을 삭제하지 않는다.
test('CampaignSession TheWarBegins: 개발자 빈 객체 삭제 패킷', () => {
    const { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); const c = s.core(); let sid = c.pool.layout.serverFirst;
        // 현재 Pool에서 반환된 서버 슬롯을 선택한다.
        while (sid < c.pool.capacity && !(c.pool.slot(sid)[11] & 1)) sid += 1;
        assert.ok(sid < c.pool.capacity);
        const text = `de ${sid}`, packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(c.pool.slot(sid)[11] & 1, 1);
    } finally { s.dispose(); }
});

// 불완전한 개발자 HP 명령은 월드 변경 전에 같은 추적 장치로 오류를 알린다.
test('CampaignSession TheWarBegins: 개발자 HP 입력 오류 패킷', () => {
    const logs = [], { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); s.world().storage.bindGraphTrace((...args) => logs.push(args));
        const text = 'slhp 5', packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.deepEqual(logs, [[29, 'bad command.\n']]);
    } finally { s.dispose(); }
});

// 전투 패널 명령은 같은 GameAux 설정을 바꾸고 현재 입력 큐 장치를 호출한다.
test('CampaignSession TheWarBegins: 개발자 패널 크기 패킷', () => {
    const gameplay = new ClientGameplayGlobals(), queued = [], { s } = scene({ gameplayGlobals: gameplay, developerQueue: (...args) => queued.push(args) });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); const text = 'cr 2 4', packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.deepEqual([...gameplay.gameAux().missionSettings()].slice(5), [2, 4]);
        assert.deepEqual(queued, [[0x02000065, 2, 4]]);
    } finally { s.dispose(); }
});

// 실제 31h 분배기는 create/scatter를 같은 Session의 Factory·Pool·CRT·World에 연결한다.
test('CampaignSession TheWarBegins: 개발자 생성과 분산 패킷', () => {
    const gameplay = new ClientGameplayGlobals(), { s } = scene({ gameplayGlobals: gameplay });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        let previous = s.world().objects();
        const created = [];
        // 두 명령을 실제 패킷 경계로 순서대로 전달하고 새 객체를 모은다.
        for (const text of ['create 154 2', 'scatter 154']) {
            const packet = new Uint8Array(9 + text.length);
            packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
            packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
            assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
            const current = s.world().objects();
            created.push(...current.filter(({ sid }) => !previous.some((old) => old.sid === sid))); previous = current;
        }
        assert.equal(created.length, 11); assert.equal(gameplay.gameAux().developerScatterRow, 5);
        assert.deepEqual(created[0].object, { type: 154, owner: 0, frame: 2, x: 20, y: 20 });
        assert.deepEqual(created.slice(1).map(({ object }) => [object.x, object.y]), [[3, 3], [6, 3], [9, 3], [12, 3], [15, 3], [18, 3], [21, 3], [24, 3], [27, 3], [30, 3]]);
    } finally { s.dispose(); }
});

// 실제 31h 분배기는 현재 Client 장치와 기록 관문을 공유한다.
test('CampaignSession TheWarBegins: 개발자 장치 명령 패킷', () => {
    const calls = [], gameplay = new ClientGameplayGlobals(), { s } = scene({ gameplayGlobals: gameplay,
        developerHeapTrace: (value) => calls.push(['heap', value]), developerQueryDirectDraw: () => calls.push(['query']), developerRecordLabel: (label) => calls.push(['label', label]) });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch);
        // 같은 31h 수신기로 각각 실제 장치 명령을 전달한다.
        for (const text of ['ht -1', 'qdd', 'al ignored']) {
            const packet = new Uint8Array(9 + text.length); packet.set([1, 0x31, packet.length, 0]);
            new DataView(packet.buffer).setUint32(4, text.length + 1, true); packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
            assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        }
        gameplay.cursor.recording = 1;
        const text = 'al named', packet = new Uint8Array(9 + text.length); packet.set([1, 0x31, packet.length, 0]);
        new DataView(packet.buffer).setUint32(4, text.length + 1, true); packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.deepEqual(calls, [['heap', true], ['query'], ['label', 'named']]);
    } finally { gameplay.cursor.recording = 0; s.dispose(); }
});

// 실제 31h 분배기는 도움말을 같은 월드의 추적 장치에 순서대로 전달한다.
test('CampaignSession TheWarBegins: 개발자 도움말 패킷', () => {
    const logs = [], { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); s.world().storage.bindGraphTrace((...args) => logs.push(args));
        const text = 'help', packet = new Uint8Array(9 + text.length);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(logs.length, 32);
        assert.deepEqual(logs[0], [29, 'setOwner(so) sid newowner - sets the owner of sid via server.\n']);
        assert.deepEqual(logs[31], [29, 'getPriest(gp) - sticks a enemy priest next to your priest\n']);
    } finally { s.dispose(); }
});

// 실제 세션의 그림 표와 31h 분배기로 같은 프레임 헤더를 추적한다.
test('CampaignSession TheWarBegins: 개발자 그림 진단 패킷', () => {
    const logs = [], { s } = scene(), dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    try {
        s.load(); s.registerCarrierCommandPrefix(dispatch); s.world().storage.bindGraphTrace((...args) => logs.push(args));
        const shape = s.core().shapes[154], header = shape.frames[0], text = 'dad 154', packet = new Uint8Array(9 + text.length);
        assert.ok(shape.loaded && shape.frameCount > 0 && header);
        packet.set([1, 0x31, packet.length, 0]); new DataView(packet.buffer).setUint32(4, text.length + 1, true);
        packet.set(Uint8Array.from(text, (letter) => letter.charCodeAt(0)), 8);
        assert.equal(dispatch.dispatch(new PacketConnection({ sender: 1 }), packet), true);
        assert.equal(logs.length, shape.frameCount);
        assert.deepEqual(logs[0], [29, `0: hx=${header.hotspotX} hy=${header.hotspotY} w=${header.width} h=${header.height} wx=${header.muzzleX} wy=${header.muzzleY}\n`]);
    } finally { s.dispose(); }
});
