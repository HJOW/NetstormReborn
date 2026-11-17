// 실제 Session 저장소의 Login 앞단과 Zone 설치를 검사한다. 게임 실행과 유닛 행동은 수행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { makeLocalMissionZone } from '../www/core/local-mission-transport.js';
import { RawCampaignSessionLoginFront } from '../www/core/raw-campaign-session-login-front.js';
import { addTechnologyTypes } from '../www/core/raw-technology-types-packet.js';
import { ARCHIVE_PATH } from '../www/assets/game-data.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { TypeTable } from '../www/assets/type-table.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { parseFort } from '../www/assets/fort.js';
import { RawFortImage } from '../www/core/raw-fort-image.js';
import { loadOriginalForts, readData } from './helpers.mjs';
import { PacketConnection } from '../www/core/raw-packet-dispatch.js';
import { readLocalMissionLogin } from '../www/core/local-mission-transport.js';
import { SECTION_NAMES } from '../www/assets/fort.js';

// 원본 설정·벽시각의 호출 순서를 기록하며 실제 전송은 내부 두 끝을 사용한다.
function devices(trace) {
    return { configInt: (key, fallback) => { trace.push(key); return fallback; }, wallSeconds: () => { trace.push('시각'); return 100.5; }, log: (message) => trace.push(message) };
}

// 실제 Tutorial 1 자산과 원래 생성 순서로 Player 참여 부모만 준비한다.
function loginServerOwner() {
    const archive = new TaffArchive(readData(ARCHIVE_PATH)), entry = loadOriginalForts(archive).find((candidate) => /thewarbegins.fort$/i.test(candidate.source));
    const fort = parseFort(entry.bytes, createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText))), o = campaignOwners();
    try { o.connectWorldInstall(fort); o.connectOutcome(); o.connectAiBrains(); return o; } catch (error) { o.dispose(); throw error; }
}

// Zone이 실제 Player 이름·Core 슬롯·권한을 변경하고 미등록 수신기의 서버 열기는 거부한다.
test('Session Login 앞단: 같은 현재 저장소에 Zone 설치와 이름 정제', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, devices(trace));
    try {
        const packet = makeLocalMissionZone(); packet.storage.set(Uint8Array.of(65, 32, 63, 66, 0), 24);
        o.globals.mySubscriber = 1; front.pendingStamp = 0x12345678;
        o.core.processState.server = false; o.state.assetState.writeBoss(false);
        assert.throws(() => front.installZone(packet.bytes()), /실제 로그인 처리기/);
        assert.equal(o.core.processState.server, true); assert.equal(o.state.assetState.readBoss(), true);
        assert.equal(o.connectionActivityGlobals.activeStamp().value, 0x12345678);
        assert.equal(o.localStateGlobals.zoneWords()[0], 0x82000004); assert.equal(o.localStateGlobals.captureEnd().value, 250.5);
        assert.equal(o.playerState.battleOwner, 1); assert.equal(o.playerState.templateOwner, 1); assert.equal(o.playerState.playerKnown, 1);
        assert.equal(Buffer.from(o.players.subarray(192, 195)).toString('ascii'), 'AB\0');
        assert.equal(o.coreDataGlobals.current.buffer, o.players.buffer); assert.equal(o.coreDataGlobals.current.byteOffset, o.players.byteOffset + 180 + 0x6c);
        assert.equal(front.transport.lifetime.port, 0); assert.equal(front.transport.lifetime.hasClientConnection, false);
        assert.deepEqual(trace, ['gameServerPort', 'loginTimeout', '시각']);
    } finally { front.dispose(); o.dispose(); }
    assert.throws(() => front.installZone(makeLocalMissionZone().bytes()), /반납/);
});

// 지정 옵션은 재초기화하지 않고 외부 등록표의 정체성을 확인한다.
test('Session Login 앞단: 현재 옵션 보존과 외부 전송 계약', () => {
    const options = new Uint8Array(87).fill(0x55), o = campaignOwners({ missionOptions: options });
    const front = new RawCampaignSessionLoginFront(o, devices([]));
    try { assert.ok(options.every((value) => value === 0x55)); } finally { front.dispose(); o.dispose(); }
    const next = campaignOwners();
    try { assert.throws(() => new RawCampaignSessionLoginFront(next, { transport: {} }), /수명\/등록표\/반납/); assert.equal(next.battleOptions[1], 2); } finally { next.dispose(); }
});

// 선택 장치가 빠지면 실제 선행 쓰기를 유지하고 성공으로 바꾸지 않는다.
test('Session Login 앞단: Zone 시계 누락의 선행 효과', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, { configInt: (_key, fallback) => fallback });
    try {
        o.core.processState.server = false; o.state.assetState.writeBoss(false);
        assert.throws(() => front.installZone(makeLocalMissionZone().bytes()), /wallSeconds/);
        assert.equal(o.core.processState.server, true); assert.equal(o.state.assetState.readBoss(), true);
        assert.equal(o.localStateGlobals.zoneWords()[0], 0x82000004); assert.equal(front.transport.lifetime.port, 0);
    } finally { front.dispose(); o.dispose(); }
});

// 정제 뒤 빈 이름의 대체는 원본 string_view 길이 안에서만 허용한다.
test('Session Login 앞단: 빈 이름은 원본 보기 길이 부족을 보존', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, devices([]));
    try {
        const packet = makeLocalMissionZone(); packet.storage.set(Uint8Array.of(63, 0), 24);
        assert.throws(() => front.installZone(packet.bytes()), /빈 이름 대체 범위/);
        assert.equal(o.players[192], 0); assert.equal(front.transport.lifetime.port, 0);
    } finally { front.dispose(); o.dispose(); }
});

// 옵션의 결과와 69h 사용표가 같은 실제 장기 owner를 직접 갱신한다.
test('Session Login 옵션: 현재 마나·간헐천 비용·타입 목록 공유', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, devices([])), aux = o.gameplayGlobals.gameAux();
    try {
        front.options.applyDerived();
        assert.equal(o.gameplayGlobals.manaRadius().value, 30); assert.equal(o.energyState.radiusSquared, 900);
        assert.equal(o.core.types[o.geyserType].cost, 3000); assert.equal(aux.refundPercent, 50);
        const types = aux.typeUse(); addTechnologyTypes(Uint8Array.of(72, 0), types); assert.equal(types[0], 72);
        front.options.clearTypes(); assert.ok(types.every((value) => value === 0));
        types[1] = 133; aux.resetForGeneralWorld(); assert.equal(aux.typeUse(), types); assert.equal(types[1], 133);
    } finally { front.dispose(); o.dispose(); }
});

// NaN 비트를 JS 숫자 왕복으로 정규화하지 않고 같은 단정도 저장소에 기록한다.
test('Session Login 옵션: 현재 마나 반경의 NaN 비트 보존', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, devices([]));
    try {
        const ranges = o.gameplayGlobals.manaRanges(), bits = new Uint32Array(ranges.buffer, ranges.byteOffset, ranges.length);
        bits[3] = 0x7f812345; front.options.updateRange();
        assert.equal(o.gameplayGlobals.manaRadius().bits, 0x7fc12345); assert.equal(o.energyState.radiusSquaredBits, 0x7fc12345);
        assert.ok(Number.isNaN(o.gameplayGlobals.manaRadius().value)); assert.ok(Number.isNaN(o.energyState.radiusSquared));
    } finally { front.dispose(); o.dispose(); }
});

// 실제 설정 배열을 송신 순간 읽고 수신 결과를 같은 배열에 쓴다.
test('Session Login 패킷: 같은 미션 설정의 송신·수신', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, devices([])), captured = [];
    try {
        const values = o.gameplayGlobals.gameAux().missionSettings(); values.set([11, 22, 33, 44, 55, 66, 77]);
        front.transport.send = (target, packet) => { captured.push({ target, packet: packet.slice() }); };
        front.settings.send(9);
        assert.equal(captured[0].target, 9); assert.deepEqual([...captured[0].packet.subarray(0, 4)], [1, 0x32, 32, 0]);
        const sent = new DataView(captured[0].packet.buffer); assert.deepEqual(Array.from({ length: 7 }, (_, i) => sent.getUint32(4 + i * 4, true)), [...values]);
        sent.setUint32(16, 0x12345678, true); front.settings.receive(captured[0].packet); assert.equal(values[3], 0x12345678);
        values[0] = 99; front.settings.send(9); assert.equal(new DataView(captured[1].packet.buffer).getUint32(4, true), 99);
    } finally { front.dispose(); o.dispose(); }
});

// Player 송신은 실제 이름을 정제하고 현재 전송의 mutable 경계를 사용한다.
test('Session Login 패킷: 현재 Player와 메타데이터 mutable 송신', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, devices(trace)), captured = [];
    try {
        o.players.set(Buffer.from('A B\0'), 192); new DataView(o.players.buffer, o.players.byteOffset).setUint32(180 + 0x2c, 7, true);
        front.transport.sendMutable = (target, packet) => { captured.push({ target, packet: packet.slice() }); };
        front.metadata.sendPlayer(1, 9);
        assert.equal(captured[0].target, 9); assert.equal(captured[0].packet[1], 0x2b);
        assert.equal(Buffer.from(captured[0].packet.subarray(0x3b, 0x3e)).toString('ascii'), 'AB\0');
        assert.equal(Buffer.from(o.players.subarray(192, 195)).toString('ascii'), 'AB\0');
        front.metadata.sendMetadata(9); assert.equal(captured[1].packet[1], 0x72); assert.ok(trace.includes('No Central Island.\n'));
    } finally { front.dispose(); o.dispose(); }
});

// 현재 Fort manager가 없으면 실제 송신·로그를 실행하지 않는다.
test('Session Login 패킷: 없는 현재 Fort는 송신하지 않음', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, devices(trace));
    try { assert.equal(front.forts.sendFort(1, 9, 1), false); assert.deepEqual(trace, []); } finally { front.dispose(); o.dispose(); }
});

// 보호된 Tutorial 1 Fort의 현재 manager를 실제 이미지 writer와 mutable 송신에 연결한다.
test('Session Login 패킷: 실제 Tutorial 1 Fort 이미지 송신', () => {
    const archive = new TaffArchive(readData(ARCHIVE_PATH)), entry = loadOriginalForts(archive).find((candidate) => /thewarbegins.fort$/i.test(candidate.source));
    const fort = parseFort(entry.bytes, createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText)));
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, devices(trace)), captured = [];
    try {
        o.templateStorage.install(1, fort, 'thewarbegins'); o.players[180 + 0x58] = 7;
        front.transport.sendMutable = (target, packet) => captured.push({ target, packet: packet.slice() });
        assert.equal(front.forts.sendFort(1, 9, 42), true); assert.equal(captured[0].target, 9);
        const packet = captured[0].packet, view = new DataView(packet.buffer), expected = new RawFortImage('patch1078', o.templateStorage.template(1), { typeCount: () => o.core.types.length }).save().bytes();
        assert.equal(packet[1], 0x2a); assert.equal(view.getUint16(2, true), 4); assert.equal(view.getUint32(8, true), 42); assert.equal(packet[16], 7);
        assert.equal(view.getUint32(12, true), expected.length); assert.deepEqual(packet.subarray(17, 17 + expected.length), expected); assert.ok(trace.includes('로컬 Fort 전송\n'));
    } finally { front.dispose(); o.dispose(); }
});

// 시간 응답은 같은 Client 표본과 고정 연결 토큰을 이용한다.
test('Session Login 시계: 같은 Client 표본 초기화와 고정 연결 응답', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, { ...devices(trace), gameSeconds: () => 12.25 }), sent = [];
    const registry = front.transport.registry, connection = new PacketConnection(), token = registry.track(connection, () => {}, () => {}, (packet) => sent.push(packet.slice()));
    try {
        const local = o.localStateGlobals, samples = local.clockSamples(), globals = local.clockSynchronization();
        samples.sent = 3; samples.received = 4; samples.next = 19; samples.samples[0][3] = 0xa5;
        front.clockSynchronization.initializeServer(front.transport.dispatch);
        assert.deepEqual([samples.sent, samples.received, samples.next, samples.samples[0][3], globals.replyMode], [0, 0, 19, 0xa5, 1]);
        const packet = new Uint8Array(32); packet[1] = 5; packet[2] = 32;
        front.clockSynchronization.receive(connection, packet);
        assert.equal(sent.length, 1); assert.equal(new DataView(sent[0].buffer).getFloat64(16, true), 12.25);
        assert.equal(typeof front.transport.dispatch.registration(5).receive, 'function');
    } finally { registry.forget(token); front.dispose(); o.dispose(); }
});

// Commander는 실제 토큰에 현재 표본을 보내며 바뀐 주소의 응답은 거부한다.
test('Session Login 시계: 토큰 표본 송신과 다른 주소 거부', () => {
    const o = campaignOwners(), trace = [], sent = [], connection = new PacketConnection({ flags: 0x20, address: new Uint8Array(20).fill(1) });
    const front = new RawCampaignSessionLoginFront(o, { ...devices(trace), gameSeconds: () => { connection.address[0] = 2; return 8; } });
    const registry = front.transport.registry, token = registry.track(connection, () => {}, () => {}, (packet) => sent.push(packet.slice()));
    try {
        const local = o.localStateGlobals, globals = local.clockSynchronization(), samples = local.clockSamples();
        front.clockSynchronization.attach(front.transport.dispatch, token); front.clockSynchronization.poll();
        assert.equal(globals.connection, token); assert.equal(samples.sent, 1); assert.equal(sent.length, 1); assert.equal(sent[0][1], 5);
        assert.equal(new DataView(sent[0].buffer).getFloat64(8, true), 100.5);
        front.clockSynchronization.initializeServer(front.transport.dispatch);
        assert.throws(() => front.clockSynchronization.receive(connection, new Uint8Array(32)), /다른 시간 응답 주소/);
        assert.equal(sent.length, 1);
    } finally { registry.forget(token); front.dispose(); o.dispose(); }
});

// 상태14의 공통 확정은 실제 Client의 다섯 장기 블록을 직접 쓴다.
test('Session Login 상태: 같은 Client 블록에 상태14 확정', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, devices(trace));
    try {
        const blocks = o.localStateGlobals.state(); blocks.current[0] = 14; blocks.current[9] = 0x12345678; blocks.phase[0] = 9;
        front.state.run();
        assert.equal(blocks.old[0], 9); assert.equal(blocks.phase[0], 14); assert.equal(blocks.phase[9], 0x12345678);
        assert.equal(blocks.current[0], 0); assert.equal(blocks.next[0], 0); assert.ok(trace.includes('로컬 미션 상태 14\n'));
    } finally { front.dispose(); o.dispose(); }
});

// 실제 서버가 열리지 않은 상태8은 선행 기한만 남기고 접속을 성공시키지 않는다.
test('Session Login 상태: 미등록 서버의 상태8 접속 실패 접두', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, devices(trace));
    try {
        const blocks = o.localStateGlobals.state(); blocks.current[0] = 8;
        assert.throws(() => front.state.run(), /로컬 클라이언트 새 접속/);
        assert.equal(o.localStateGlobals.connectEnd().value, 130.5); assert.equal(blocks.current[0], 8); assert.equal(blocks.phase[0], 0);
        assert.equal(front.transport.lifetime.hasClientConnection, false); assert.ok(trace.includes('로컬 미션 상태 8\n'));
    } finally { front.dispose(); o.dispose(); }
});

// 상태9의 실제 만료는 원문처럼 출처와 next만 바꾸고 공통 확정을 건너뛴다.
test('Session Login 상태: 현재 벽시각 만료의 원문 조기 반환', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, devices([]));
    try {
        const blocks = o.localStateGlobals.state(); blocks.current[0] = 9; blocks.phase[0] = 8;
        o.localStateGlobals.connectEnd().value = 50; o.localStateGlobals.sourceType().value = 77;
        front.state.run();
        assert.equal(o.localStateGlobals.sourceType().value, 0); assert.equal(blocks.next[0], 3);
        assert.equal(blocks.current[0], 9); assert.equal(blocks.phase[0], 8);
    } finally { front.dispose(); o.dispose(); }
});

// 실제 Fort 원문의 길이 필드와 각 섹션 payload를 별도로 읽는다.
function fortImageSections(bytes) {
    const result = []; let offset = 2;
    // 고정 35개 섹션을 원문 길이 순서대로 읽는다.
    for (const name of SECTION_NAMES) {
        const length = bytes[offset] | (bytes[offset + 1] << 8); assert.ok(length >= 2 && offset + length <= bytes.length, name);
        result.push(bytes.slice(offset + 2, offset + length)); offset += length;
    }
    return result;
}

// State9은 실제 선택 Fort의 다섯 섹션만 지역 이미지로 읽고 현재 전송에 건넨다.
test('Session Login 파일: State9의 실제 선택 Fort 로그인 이미지', () => {
    const archive = new TaffArchive(readData(ARCHIVE_PATH)), bytes = archive.read('\\d\\thewarbegins.fort'), trace = [], sent = [];
    const o = campaignOwners({ missionFortFileSelect: (name) => { trace.push(`선택:${name}`); return { fileName: 'd/thewarbegins.fort', read: () => { trace.push('읽기'); return Uint8Array.from(bytes); } }; } });
    const front = new RawCampaignSessionLoginFront(o, { ...devices(trace), refresh: () => trace.push('갱신') });
    try {
        o.globals.mySubscriber = 42; o.handle.set(Buffer.from('Tester\0')); o.localStateGlobals.connectEnd().value = 200;
        front.transport.status = () => 1; front.transport.isOpen = () => true; front.transport.send = (target, packet) => sent.push({ target, packet: packet.slice() });
        const blocks = o.localStateGlobals.state(); blocks.current[0] = 9; blocks.phase[0] = 8; front.state.run();
        assert.equal(sent.length, 1); assert.equal(sent[0].target, 9);
        const login = readLocalMissionLogin(sent[0].packet); assert.equal(login.subscriber, 42); assert.equal(login.handle, 'Tester');
        const original = fortImageSections(bytes), actual = fortImageSections(login.fortImage);
        // 원문의 고정 네 섹션과 현재 Terr00만 남긴다.
        for (let index = 0; index < SECTION_NAMES.length; index += 1) {
            const keep = ['Subscriber', 'Territory', 'TypeNames', 'CoreData', 'Terr00'].includes(SECTION_NAMES[index]);
            assert.deepEqual(actual[index], keep ? original[index] : new Uint8Array(0), SECTION_NAMES[index]);
        }
        assert.deepEqual(trace.slice(0, 5), ['로컬 미션 상태 9\n', '시각', '선택:thewarbegins', '읽기', '갱신']);
        assert.equal(o.templateStorage.ownedImages(), 0); assert.equal(blocks.phase[0], 10); assert.equal(blocks.next[0], 10);
    } finally { front.dispose(); o.dispose(); }
});

// 독립 Session은 파일 선택기를 부르지 않고 현재 manager 전체 이미지를 사용한다.
test('Session Login 파일: 독립 State9은 현재 Fort manager 이미지 사용', () => {
    const archive = new TaffArchive(readData(ARCHIVE_PATH)), entry = loadOriginalForts(archive).find((candidate) => /thewarbegins.fort$/i.test(candidate.source));
    const fort = parseFort(entry.bytes, createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText)));
    const o = campaignOwners(), sent = [], front = new RawCampaignSessionLoginFront(o, { ...devices([]), refresh: () => {} });
    try {
        o.templateStorage.install(1, fort, 'thewarbegins'); o.localStateGlobals.connectEnd().value = 200;
        front.transport.status = () => 1; front.transport.isOpen = () => true; front.transport.send = (_target, packet) => sent.push(packet.slice());
        o.localStateGlobals.state().current[0] = 9; front.state.run();
        const expected = new RawFortImage('patch1078', o.templateStorage.template(1), { typeCount: () => o.core.types.length }).save().bytes();
        assert.deepEqual(readLocalMissionLogin(sent[0]).fortImage, expected); assert.equal(o.templateStorage.ownedImages(), 1);
    } finally { front.dispose(); o.dispose(); }
});

// State13은 현재 manager가 없을 때만 선택한 Fort 파일을 실제 부모로 읽는다.
test('Session Login 파일: State13의 누락 Fort 선택과 실제 읽기', () => {
    const archive = new TaffArchive(readData(ARCHIVE_PATH)), bytes = archive.read('\\d\\thewarbegins.fort'), trace = [];
    const o = campaignOwners({
        missionFortFileSelect: (name) => { trace.push(`선택:${name}`); return { fileName: 'd/thewarbegins.fort', read: () => { trace.push('읽기'); return Uint8Array.from(bytes); } }; },
        missionFortLoad: { readOnly: () => true }, install: { refreshWindowTitle: () => trace.push('제목') },
    });
    const front = new RawCampaignSessionLoginFront(o, { ...devices(trace), refresh: () => trace.push('갱신') });
    try {
        front.transport.isOpen = () => false; o.localStateGlobals.state().current[0] = 13; front.state.run();
        assert.equal(o.templateStorage.ownedImages(), 1); assert.equal(o.templateStorage.name(1), 'd/thewarbegins.fort');
        assert.equal(trace.filter((event) => event === '선택:thewarbegins').length, 1); assert.equal(trace.filter((event) => event === '읽기').length, 1);
        assert.ok(trace.includes('제목'));
    } finally { front.dispose(); o.dispose(); }
});

// 타이머 시작은 같은 Client 기한에 예약하고 집계는 현재 Player 기록을 다시 읽는다.
test('Session Login 타이머: 현재 Player 집계와 공유 기한 예약', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, devices(trace));
    try {
        const view = new DataView(o.players.buffer, o.players.byteOffset); view.setUint32(180 + 0x2c, 7, true);
        assert.equal(front.timer.players.subscribers(), 1); front.timer.run(true);
        assert.equal(o.localStateGlobals.loginEnd().value, 130.5); assert.equal(front.timer.state.timeout, 30);
        view.setUint32(180 + 0x2c, 0, true); assert.equal(front.timer.players.subscribers(), 0);
        assert.deepEqual(trace, ['login status timer start\n', 'loginTimeout', '시각']);
    } finally { front.dispose(); o.dispose(); }
});

// 비서버의 실제 연결이 사라지면 같은 요청 phase·기한과 시계를 사용해 중단한다.
test('Session Login 타이머: 현재 연결 부재의 중단 순서', () => {
    const o = campaignOwners(), trace = [], request = { value: 0 };
    const front = new RawCampaignSessionLoginFront(o, { ...devices(trace), requestPhase: request, clock: () => 321, abortBattle: () => trace.push('중단'), refresh: () => trace.push('갱신') });
    try {
        o.core.processState.server = false; o.localStateGlobals.begin().value = 1; o.localStateGlobals.scriptBreak().value = 0;
        front.timer.run(true); front.timer.run(false);
        assert.equal(o.gameplayGlobals.battleClockOrigin().value, 321); assert.equal(o.localStateGlobals.loginEnd().value, 0);
        assert.equal(o.localStateGlobals.begin().value, 0); assert.equal(o.localStateGlobals.scriptBreak().value, 1); assert.equal(request.value, 4);
        assert.deepEqual(trace.slice(-2), ['중단', '갱신']);
    } finally { front.dispose(); o.dispose(); }
});

// 빈 월드의 미션 로그인 완료 관문은 저장 파일 장치 없이 같은 사제 상태와 예약을 정리한다.
test('Session Login 타이머: 미션 저장 생략과 현재 사제 상태 정리', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, { ...devices(trace), clock: () => 11, refresh: () => trace.push('갱신') });
    try {
        front.timer.run(true); front.timer.run(false);
        assert.equal(o.localStateGlobals.loginEnd().value, 0); assert.equal(o.localStateGlobals.begin().value, 0); assert.equal(o.localStateGlobals.scriptBreak().value, 0);
        assert.deepEqual([...o.priestState.priests.currentOwnerState()].slice(1, 9), Array(8).fill(2)); assert.ok(trace.includes('갱신'));
    } finally { front.dispose(); o.dispose(); }
});

// State13은 현재 연결이 열렸을 때만 같은 로그인 타이머를 시작한다.
test('Session Login 타이머: State13의 실제 공유 기한 연결', () => {
    const archive = new TaffArchive(readData(ARCHIVE_PATH)), bytes = archive.read('\\d\\thewarbegins.fort'), trace = [];
    const o = campaignOwners({ install: { refreshWindowTitle: () => trace.push('제목') } });
    const front = new RawCampaignSessionLoginFront(o, { ...devices(trace), refresh: () => trace.push('갱신') });
    try {
        o.templateStorage.installFile(1, {}, bytes, 'd/thewarbegins.fort');
        new DataView(o.headers.buffer, o.headers.byteOffset, o.headers.byteLength).setUint32(20 + 8, 1, true); // 실제 미션 파일의 읽기 전용 표식이다.
        front.transport.isOpen = () => true;
        o.localStateGlobals.state().current[0] = 13; front.state.run();
        assert.equal(o.localStateGlobals.loginEnd().value, 130.5); assert.equal(front.inputMode, 0);
        assert.ok(trace.includes('login status timer start\n'));
    } finally { front.dispose(); o.dispose(); }
});

// 실제 참여 부모가 없는 앞단은 서버 송신 성공을 합성하지 않는다.
test('Session Login 서버: 실제 참여 부모 없이 결선 거부', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, devices([]));
    try { assert.throws(() => front.bindServer(), /실제 참여/); assert.equal(front.server, null); } finally { front.dispose(); o.dispose(); }
});

// Begin·Ready·Reply는 같은 장기 전역을 읽고 전체 mutable 패킷을 현재 전송에 건넨다.
test('Session Login 서버: 현재 Player와 공유 seed·디버그 송신', () => {
    const o = loginServerOwner(), trace = [], packets = [], front = new RawCampaignSessionLoginFront(o, { ...devices(trace), observePacket: () => {} });
    try {
        const server = front.bindServer(), aux = o.gameplayGlobals.gameAux(), local = o.localStateGlobals;
        assert.equal(server.participation, o.participation); assert.throws(() => front.bindServer(), /중복/);
        front.transport.sendMutable = (target, packet) => packets.push({ target, packet: packet.slice() });
        assert.deepEqual([...aux.packet0cHeader()], [0x0c, 1]); assert.equal(aux.packet0cInitialLength, 0);
        aux.storeResourcePoolBits(0xdeadbeef); aux.packet0cHeader().set([0x0d, 3]); aux.packet0cInitialLength = 9; local.debugBurst().value = 1;
        server.sendBegin(1); assert.equal(packets.length, 4); assert.equal(packets[0].packet[1], 0x29);
        assert.equal(new DataView(packets[0].packet.buffer).getUint32(30, true), 0xdeadbeef);
        // 디버그 writer의 세 예약은 같은 현재 헤더와 payload 순서를 쓴다.
        for (const item of packets.slice(1)) {
            assert.equal(item.packet.length, 63012); assert.deepEqual([...item.packet.subarray(0, 4)], [3, 0x0d, 9, 0]);
            assert.equal(new DataView(item.packet.buffer).getUint32(8, true), 63000); assert.equal(item.packet[12 + 257], 1);
        }
        packets.length = 0; o.players[180 + 0x50] = 0x40; o.players[3 * 180 + 0x50] = 0x40;
        server.sendReady(9); assert.equal(new DataView(packets[0].packet.buffer).getUint32(4, true), 10); assert.ok(trace.includes('Ready 송신 target9 mask10\n'));
        local.captureEnd().value = 150.5; server.sendReply(1, 1); assert.equal(new DataView(packets[1].packet.buffer).getFloat64(10, true), 50);
        aux.resetForGeneralWorld(); assert.deepEqual([...aux.packet0cHeader()], [0x0d, 3]); assert.equal(aux.packet0cInitialLength, 9);
    } finally { front.dispose(); o.dispose(); }
});

// 비미션 Core 복원은 false를 숫자 0으로 전달해 불필요한 Chat 초기화를 호출하지 않는다.
test('Session Login 서버: 현재 Core 복원과 저장 여부 관문', () => {
    const o = loginServerOwner(), front = new RawCampaignSessionLoginFront(o, devices([]));
    try {
        const server = front.bindServer(), core = Uint8Array.from({ length: 10 }, (_, index) => index + 10);
        o.globals.missionPost = false; o.localStateGlobals.savedGame().value = false; o.coreDataGlobals.storeMetadata(1, 7, core);
        server.hooks.restoreCoreData(1);
        assert.deepEqual(o.players.subarray(180 + 0x6c, 180 + 0x6c + 10), core);
    } finally { front.dispose(); o.dispose(); }
});
