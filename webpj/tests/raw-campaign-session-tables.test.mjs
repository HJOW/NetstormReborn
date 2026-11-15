// Session 생성자1152..1186을 실제 기반 owner/Player/공용 전송/내부 zlocal로 대조한다(WB04-E576).
// 외부 연결·멀티플레이 게임·전체 Session Load 검사는 추가하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { ClientCoreDataGlobals } from '../www/core/client-core-data-globals.js';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawConstructionCommand } from '../www/core/raw-construction-command.js';
import { LocalMissionTransport, makeLocalMissionLogin } from '../www/core/local-mission-transport.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { RawPriestPostPop } from '../www/core/raw-priest-post-pop.js';
import { SquidForms } from '../www/core/squid-forms.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get, floatToBits } from './raw-support.mjs';

// 같은 보호 GameAssets와 읽기를 요구하지 않는 VFS다. 시험마다 실제 월드/Player는 새로 소유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
const files = { read() { throw new Error('이 생성 구간에서 VFS 읽기 금지'); } };
// 독립 캠페인의 실제 필수 입력만 공급한다.
function scene(extra = {}) { return new RawCampaignSessionOwners(game, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra }); }
// 실제 Player/SP 뒤 이 단위의 생성자 구간까지 연결한다.
function connect(owner) { owner.initializePlayersAndSp(); owner.connectPlayerTablesAndPackets(); return owner.tables; }
// Core1 선택의 같은 물리 주소를 비교할 실제 Client 장부를 만든다.
function clientRecords() { const playerRecords = new ClientPlayerRecords(); const coreDataGlobals = new ClientCoreDataGlobals(); playerRecords.bindCoreData(coreDataGlobals); return { playerRecords, coreDataGlobals }; }
// 선택하면 실패해야 하는 효과다. NULL peer 기본 송신에서는 호출되지 않는다.
function forbidden() { throw new Error('선택되지 않아야 하는 효과'); }

test('Session Player 테이블: Player/SP 뒤 실제 Core1을 선택하고 구간 완료와 전체 Session을 구별한다', () => {
    const records = clientRecords(); const o = scene(records);
    try {
        assert.throws(() => o.connectPlayerTablesAndPackets(), /초기화 전/); o.initializePlayersAndSp(); assert.equal(records.coreDataGlobals.current, null);
        o.connectPlayerTablesAndPackets(); const current = records.coreDataGlobals.current;
        assert.equal(o.tables.ready, true); assert.equal(current.buffer, o.players.buffer); assert.equal(current.byteOffset, o.players.byteOffset + 0xb4 + 0x6c); assert.equal(current.length, 10);
        assert.equal(o.playerState.battleOwner, 1); assert.equal(o.playerState.templateOwner, 1); current[3] = 0x92; assert.equal(o.players[0xb4 + 0x6f], 0x92);
        assert.throws(() => o.connectPlayerTablesAndPackets(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectPlayerTablesAndPackets(), /반납/);
});

test('Session 건설 비용 관문: 현재 missionPost가 실제 명령 선검사의 생략을 바꾼다', () => {
    const o = scene(); const events = [];
    try {
        connect(o); const s = o.state.constructionState; const packet = new Uint8Array(25); packet[4] = 75; o.core.types[75].cost = 100;
        const body = new RawConstructionCommand(o.core.types, s.command, { money: () => { events.push('money'); return 0; }, shortfall: () => events.push('shortfall'), request: () => { events.push('request'); return true; }, reject: forbidden });
        o.globals.missionPost = false; assert.equal(s.place.chargeBlockedRead(), false); body.receive(2, packet); assert.deepEqual(events, ['money', 'shortfall']);
        events.length = 0; o.globals.missionPost = true; assert.equal(s.place.chargeBlockedRead(), true); body.receive(2, packet); assert.deepEqual(events, ['request']);
    } finally { o.dispose(); }
});

test('Session 방송 기본값: 값형 서비스 사본과 NULL peer·현재 로컬/Server·원문 진단을 유지한다', () => {
    const assets = {}; const peers = new Array(9).fill(null); const o = scene({ assets, battlePeers: peers }); const events = [];
    try {
        assert.notEqual(o.services.assets, assets); assert.notEqual(o.services.battlePeers, peers); peers[1] = forbidden; assets.sendGunPacket = forbidden;
        const t = connect(o); assert.equal(t.broadcast.peers, o.services.battlePeers); assert.equal(t.broadcast.players, o.players);
        o.playerState.battleOwner = 7; assert.equal(t.broadcast.localOwner.value, 7); o.core.processState.server = false; assert.equal(t.broadcast.server.value, false);
        const packet = Uint8Array.of(1, 0x27, 4, 0); o.services.assets.sendGunPacket(packet); o.services.pathPacket(packet); o.services.formPacket(packet); t.broadcast.sendMutable(packet);
        assert.equal(assets.sendGunPacket, forbidden); assert.equal(o.services.connectionRegistry, undefined);
        o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => events.push(args)); t.broadcast.sendExcept(-1, packet);
        assert.deepEqual(events, [['server', '\\Ns\\O\\Ztrans.cpp', 0x1c3]]);
    } finally { o.dispose(); }
});

test('Session 송신 서비스: 제공된 네 장치와 실제 자산 송신 함수를 덮어쓰지 않는다', () => {
    const assets = { sendGunPacket: forbidden }; const o = scene({ assets, pathPacket: forbidden, formPacket: forbidden, dependencyTransmit: forbidden });
    try { connect(o); assert.equal(o.services.assets.sendGunPacket, forbidden); assert.equal(o.services.pathPacket, forbidden); assert.equal(o.services.formPacket, forbidden); assert.equal(o.services.dependencyTransmit, forbidden); assert.equal(o.tables.formWriter.pool, o.core.pool); } finally { o.dispose(); }
});

test('Session Form: 같은 공용 헤더·2048BYTE와 실제 writer의13/18 반환 크기·현재 송신 장치를 사용한다', () => {
    const packets = []; const o = scene({ formPacket: (packet) => packets.push({ packet, bytes: Array.from(packet) }) });
    try {
        const t = connect(o); const common = o.gameplayGlobals.gameAux().squidTransmission;
        assert.equal(t.formHeaders, common.forms); assert.equal(t.deletionStamp, common.deletion); assert.equal(t.formBuffer, common.buffer);
        o.core.processState.server = true; const forms = new SquidForms(o.core.pool); const sid = 20000; forms.take(1, sid);
        const raw = o.core.pool.allocated(sid); put(raw, 16, 20001, 2); put(raw, 18, 0, 2); put(raw, 20, 0, 2); raw[11] = 0x34;
        common.buffer.fill(0xa7); Object.assign(common.forms.dependency, { message: 0x65, protocol: 0xff, length: 0x1234 });
        o.services.dependencyTransmit(sid, 0xdeadbeef); assert.equal(packets[0].packet.buffer, common.buffer.buffer); assert.equal(packets[0].packet.byteOffset, common.buffer.byteOffset);
        assert.deepEqual(packets[0].bytes, [0x7f, 0x65, 0x34, 0x12, 0x20, 0x4e, 0x21, 0x4e, 0x34, 0, 0, 0, 0]); assert.equal(common.buffer[13], 0xa7);
        const content = sid + 2; forms.take(6, content); const bytes = o.core.pool.allocated(content);
        put(bytes, 12, 0x1122, 2); put(bytes, 18, 0x3344, 2); put(bytes, 20, 0x5566, 2); put(bytes, 24, 0x7788, 2); put(bytes, 16, 20001, 2); bytes[23] = 0x9a; bytes[11] = 0xbc;
        o.services.formPacket = (packet) => packets.push({ packet, bytes: Array.from(packet) }); o.services.dependencyTransmit(content, 0);
        assert.deepEqual(packets[1].bytes, [1, 0x2d, 18, 0, 0x22, 0x4e, 0x22, 0x11, 0x44, 0x33, 0x66, 0x55, 0x88, 0x77, 0x21, 0x4e, 0x9a, 0xbc]); assert.equal(common.buffer[18], 0xa7);
        common.deletion.sid = sid; common.deletion.token = 3; t.formWriter.writeDeletion(sid, 0xabc00000, common.buffer); assert.equal(common.deletion.sid, 0); assert.equal(get(common.buffer, 6, 2), 0xabc3);
    } finally { o.dispose(); }
});

test('Session 건설 mutable 송신: 실제 내부 zlocal9에서 observer 한 번 뒤 큐·수신으로 진행한다', () => {
    const o = scene(); let wire = null; const events = [];
    try {
        const t = connect(o); const packet = makeLocalMissionLogin(1, 'You', Uint8Array.of(1, 2)).bytes();
        assert.throws(() => t.sendConstructionMutable(9, packet), /Registry 없음/);
        wire = new LocalMissionTransport(o.players, true, { now: () => 100, record: () => {}, terminate: forbidden, close: () => {}, release: () => {} });
        wire.registerLogin((sender, connection, bytes) => { events.push(['receive', bytes[4]]); });
        wire.dispatch.observePacket(0x27, (bytes, outgoing) => { events.push(['observe', outgoing]); if (outgoing) bytes[4] = 7; });
        wire.open(6799); o.services.connectionRegistry = wire.registry; t.sendConstructionMutable(9, packet);
        assert.equal(packet[4], 7); assert.deepEqual(events, [['observe', 1]]); assert.equal(wire.queuedFrames, 0);
        wire.flush(); assert.equal(wire.queuedFrames, 1); assert.equal(wire.poll(), 1); assert.deepEqual(events, [['observe', 1], ['observe', 0], ['receive', 7]]);
        o.services.connectionRegistry = null; assert.throws(() => t.sendConstructionMutable(9, packet), /Registry 없음/);
    } finally { wire?.registry.closeAll(); wire?.dispose(); o.dispose(); }
});

test('Session 공장: 같은 Player9의 현재 +0/+4/+8을 사용하고 실제 append/안정 제거의 꼬리를 보존한다', () => {
    const o = scene();
    try {
        connect(o);
        // 모든 행이 실제 Player 기록의 동적 목록이며 별도 SID 사본을 만들지 않는다.
        for (let owner = 0; owner < 9; owner += 1) {
            const list = o.core.bookkeeping.ownerFactories[owner]; assert.equal(list.matchesCurrentSource(o.playerRecords, owner), true); assert.equal(list.capacity(), get(o.players, owner * 0xb4 + 4));
            list.append(20000 + owner); assert.equal(get(o.players, owner * 0xb4 + 8), 1); assert.equal(list.storage()[0], 20000 + owner);
        }
        const list = o.core.bookkeeping.ownerFactories[1]; list.append(20001); list.append(20003); list.removeAll(20001);
        assert.deepEqual(list.items(), [20003]); assert.equal(get(o.players, 0xb4 + 8), 1); assert.equal(list.storage()[2], 20003);
        put(o.players, 0xb4 + 8, 2); assert.equal(list.readCount(), 2); put(o.players, 0xb4 + 4, 2); assert.equal(list.capacity(), 2);
    } finally { o.dispose(); }
});

test('Session SID: 실제 사제 postPop이 같은 목록/owner 칸에 쓰고 자산 목록도 장기 저장소를 빌린다', () => {
    const o = scene();
    try {
        connect(o); const storage = o.world.storage; const a = o.state.assetState;
        assert.equal(a.currentResidences(), storage.sidLists.residences); assert.equal(a.currentTemples(), storage.sidLists.temples); assert.equal(a.currentOutposts(), storage.sidLists.outposts); assert.equal(a.currentGeysers(), storage.sidLists.geysers);
        const sid = o.core.pool.allocate(0); const raw = o.core.pool.allocated(sid); raw[12] = 1; raw[34] = 1; raw[40] = 0; storage.priestOwnerState[1] = 0x99;
        const body = new RawPriestPostPop(o.core.pool, o.hp, o.priestState.priests, { findRegular: () => true, reserveRegular: forbidden, addRegular: forbidden });
        body.prefix(sid, 1); body.prefix(sid, 1); assert.deepEqual(storage.sidLists.priests.items(), [sid]); assert.equal(storage.priestOwnerState[1], 0); assert.equal(o.priestState.priests.priests.readCount(), 0);
    } finally { o.dispose(); }
});

test('Session Gun: 방향별 실제81DWORD와 현재 Player mask/BYTE를 각 소비 위치에서 읽는다', () => {
    const o = scene();
    try {
        connect(o); const s = o.state; const target = o.core.pool.allocate(0); const raw = o.core.pool.allocated(target); raw[10] = 75; raw[34] = 2; put(raw, 14, floatToBits(22)); put(raw, 18, floatToBits(23));
        o.core.types[75].flags1 |= 0x10; o.core.types[75].group = 3;
        const body = new RawGunTarget(o.core.pool, o.core.hash, o.core.types, Array.from({ length: 188 }, () => ({ range: 20, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 })), o.core.shapes, s.gunTargetState);
        const query = { type: 75, owner: 1, x: 10, y: 11, centerX: 10, centerY: 11, direction: 0, frame: 0 }; put(o.players, 0xb4 + 0x54, 2); assert.equal(body.mayTarget(query, target), true);
        put(o.players, 0xb4 + 0x54, 6); s.gunTargetState.allyMasks[1] = 0; assert.equal(body.mayTarget(query, target), false);
        o.alliances[11] = 0x80000000; o.alliances[19] = 0; assert.equal(s.gunState.isAlly(1, 2), true); assert.equal(s.gunState.globals().alliance(2, 1), 0);
        put(o.players, 0xb4 + 0x50, 0xaabbcc21); assert.equal(s.gunTargetState.playerFlagsRead(1), 0x21);
        assert.throws(() => s.gunTargetState.allyMaskRead(9), RangeError); assert.throws(() => s.gunTargetState.playerFlagsRead(-1), RangeError); assert.throws(() => s.gunState.allianceRead(1, 9), RangeError);
    } finally { o.dispose(); }
});

test('Session 부분 실패: 방송 생성 이전 쓰기와 공장 루프의 완료된 접두를 되돌리지 않는다', () => {
    const records = clientRecords(); const first = scene({ ...records, battlePeers: [] });
    try {
        first.initializePlayersAndSp(); assert.throws(() => first.connectPlayerTablesAndPackets(), /길이 9/); assert.equal(records.coreDataGlobals.current.byteOffset, first.players.byteOffset + 0xb4 + 0x6c);
        assert.equal(typeof first.state.constructionState.command.chargeBlockedRead, 'function'); assert.equal(first.tables.broadcast, null); assert.equal(first.tables.formWriter, null); assert.equal(first.tables.ready, false); assert.throws(() => first.connectPlayerTablesAndPackets(), /반복/);
    } finally { first.dispose(); }
    const second = scene();
    try {
        second.initializePlayersAndSp(); second.core.bookkeeping.ownerFactories[3].entries = [1]; assert.throws(() => second.connectPlayerTablesAndPackets(), /소유/);
        assert.equal(typeof second.services.dependencyTransmit, 'function'); assert.equal(second.priestState.priests.currentPriests(), second.world.storage.sidLists.priests);
        assert.equal(second.core.bookkeeping.ownerFactories[2].matchesCurrentSource(second.playerRecords, 2), true); assert.equal(second.core.bookkeeping.ownerFactories[4].matchesCurrentSource(second.playerRecords, 4), false);
        assert.deepEqual(Array.from(second.state.gunTargetState.allyMasks), [1, 2, 4, 0, 0, 0, 0, 0, 0]); assert.equal(second.state.gunState.allies[2][2], true); assert.equal(second.state.gunState.allianceRead, null); assert.equal(second.tables.ready, false);
    } finally { second.dispose(); }
});
