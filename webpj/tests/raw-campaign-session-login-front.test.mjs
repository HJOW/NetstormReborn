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

// 원본 설정·벽시각의 호출 순서를 기록하며 실제 전송은 내부 두 끝을 사용한다.
function devices(trace) {
    return { configInt: (key, fallback) => { trace.push(key); return fallback; }, wallSeconds: () => { trace.push('시각'); return 100.5; }, log: (message) => trace.push(message) };
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
