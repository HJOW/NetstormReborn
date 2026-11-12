// Client 수명 State/로그인 전역 소유자(client-local-state-globals.js)와 하위 보유 타입(client-help-word.js·raw-monitor-time.js·raw-clock-synchronization.js·raw-packet-copy.js)을 cpppj 시험과 웹 계약으로 확인한다.
//  - cpppj 시험 이식: ClientHelpWordTests 7건(키 부재/빈 값/대소문자/교체/비NULL 빈 문자열/한글 정확 비교/상태 전역의 단일 캐시).
//  - 웹 계약: PE 초기값과 BSS 0, 접근자의 주소 고정, State 다섯 블록의 비중첩, 시간 동기화·주변음·메타 구역 뷰가 같은 상자를 읽고 씀(기존 resetClockSynchronization/stepAmbientSound/clearMetaZoneRecords/RawLocalMissionState/ClientConnectionGlobals 가 이 소유자를 그대로 받음),
//    Zone 앞 6BYTE 뷰와 battle BYTE 의 DWORD 공유, duplicateRawPacket(observer 뒤 현재 길이·할당/외부 목적지/capacity 부족·읽기 쓰기 물리 부족)과 RawPacketCopies(token 1 부터 재사용 없음·소진·인계·획득/반납).
// 아직 Client/Session 이 이 소유자를 만들지 않는다(PORT_MAP session 0). 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E335: 4-D ClientLocalStateGlobals 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientHelpWord } from '../www/core/client-help-word.js';
import { ClientLocalStateGlobals, BoolBox } from '../www/core/client-local-state-globals.js';
import { ClientConnectionGlobals } from '../www/core/client-connection-globals.js';
import { ClockSynchronizationData } from '../www/core/raw-clock-synchronization.js';
import { MonitorTimeState } from '../www/core/raw-monitor-time.js';
import { duplicateRawPacket, RawPacketCopies } from '../www/core/raw-packet-copy.js';
import { RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { PacketRegistrationTable } from '../www/core/packet-registration-table.js';
import { resetClockSynchronization } from '../www/core/raw-clock-sync-reset.js';
import { resetAmbientSoundDeadline } from '../www/core/raw-ambient-sound.js';
import { clearMetaZoneRecords, META_ZONE_BYTES } from '../www/core/raw-meta-zone-cleanup.js';
import { RawLocalMissionState } from '../www/core/raw-local-mission-state.js';

// 예외가 나는지 본다. (2026-11-12 WB04-E335)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

// 패킷 분배기 하나를 만든다(복제 시험은 타입 항목의 observer/pin 만 쓴다). (2026-11-12 WB04-E335)
function makeDispatch() {
    return new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
}

// 타입 type, 선언 길이 length 인 패킷 바이트를 만든다(BYTE 0 은 버전, 1 타입, 2~3 길이, 나머지는 채움). (2026-11-12 WB04-E335)
function makePacket(type, length, fill = 0x10) {
    const bytes = new Uint8Array(length);
    for (let index = 4; index < length; index += 1) bytes[index] = (fill + index) & 0xff;
    bytes[1] = type;
    bytes[2] = length & 0xff;
    bytes[3] = length >>> 8;
    return bytes;
}

// ───── cpppj ClientHelpWordTests 이식 ─────

test('ClientHelpWord: 설정 키가 없거나 빈 문자열이면 최초 NULL 캐시는 NULL 로 남고 반환은 false 이다', () => {
    const cache = new ClientHelpWord();
    assert.equal(cache.get(), null);
    assert.equal(cache.interpret(null), false);
    assert.equal(cache.interpret(undefined), false);
    assert.equal(cache.interpret(''), false);
    assert.equal(cache.get(), null);
    assert.equal(cache.generation, 0);
});

test('ClientHelpWord: 이미 비NULL 이면 키 부재/빈 값은 같은 문자열과 교체 횟수를 유지한다', () => {
    const cache = new ClientHelpWord();
    assert.equal(cache.interpret('Help'), true);
    assert.equal(cache.generation, 1);
    assert.equal(cache.interpret(null), false);
    assert.equal(cache.interpret(''), false);
    assert.equal(cache.get(), 'Help');
    assert.equal(cache.generation, 1);
});

test('ClientHelpWord: 대소문자만 다르면 옛 표기를 유지하고 true 이며 다른 문자열이면 교체하고 횟수를 늘린다', () => {
    const cache = new ClientHelpWord();
    cache.interpret('Help');
    assert.equal(cache.interpret('hELP'), true);
    assert.equal(cache.get(), 'Help');
    assert.equal(cache.generation, 1);
    assert.equal(cache.interpret('Info'), true);
    assert.equal(cache.get(), 'Info');
    assert.equal(cache.generation, 2);
});

test('ClientHelpWord: 비NULL 은 빈 설정값에도 NULL 로 합쳐지지 않는다', () => {
    const cache = new ClientHelpWord();
    assert.equal(cache.interpret('x'), true);
    assert.equal(cache.interpret(''), false);
    assert.notEqual(cache.get(), null);
});

test('ClientHelpWord: 한글은 ASCII 접기를 적용하지 않고 정확히 같을 때만 같은 문구이며 비교는 ASCII 영문자만 접는다', () => {
    const korean = '정보';
    const cache = new ClientHelpWord();
    assert.equal(cache.interpret(korean), true);
    assert.equal(cache.get(), korean);
    assert.equal(cache.generation, 1);
    assert.equal(cache.interpret(korean), true);
    assert.equal(cache.generation, 1);
    assert.equal(ClientHelpWord.equalsIgnoreCase('ABC', 'abc'), true);
    assert.equal(ClientHelpWord.equalsIgnoreCase('abc', 'abd'), false);
    assert.equal(ClientHelpWord.equalsIgnoreCase('ab', 'abc'), false);
    // ASCII 밖(Z 와 z 사이의 기호 '[' 0x5b, '{' 0x7b)과 가운데 점은 접지 않는다.
    assert.equal(ClientHelpWord.equalsIgnoreCase('[', '{'), false);
    assert.equal(ClientHelpWord.equalsIgnoreCase('@', '`'), false);
    assert.equal(ClientHelpWord.equalsIgnoreCase('Ａ', 'ａ'), false);
    assert.equal(cache.interpret('정보 '), true);
    assert.equal(cache.generation, 2);
});

test('ClientHelpWord: 상태 전역의 캐시는 호출마다 같은 객체이며 소비자가 읽는 값과 옵션 해석이 쓰는 값이 같다', () => {
    const local = new ClientLocalStateGlobals();
    const first = local.helpWord();
    assert.equal(first, local.helpWord());
    assert.equal(local.helpWord().get(), null);
    first.interpret('Help');
    assert.equal(local.helpWord().get(), 'Help');
    // 다른 Client 는 독립 캐시를 가진다.
    assert.notEqual(new ClientLocalStateGlobals().helpWord(), first);
});

// ───── ClientLocalStateGlobals ─────

test('PE 초기값과 최초 BSS 0: 모든 칸이 원문 초기값으로 시작하고 Session 반납으로 되돌리지 않는다', () => {
    const local = new ClientLocalStateGlobals();
    const dwordInitials = {
        debugBurst: 0, joinTestBattleCommand: 0x0200002b, viewMetaCommand: 0x02000013, firstMenuPending: 1, userUpdated: 0, tellTips: 1, tipNumber: 1,
        introAvailable: 0, autoDemo: 0, zonePending: 0, sourceType: 0, begin: 0, centralCreated: 0, loginProgressSerial: 0, restoreOwner: 0xffffffff,
        restoreSubscriber: 0xffffffff, noRestartSerial: 0, scriptBreak: 1, abortPending: 0, stateAuxCC: 0, metadataMode: 0, metaConnected: 0,
        connectionNotified: 0, cachedPeerIndex: 0xffffffff, cachedPeerValue: 0xffffffff, cachedPeerFieldA4: 0xffffffff, cachedPeerFieldA8: 0xffffffff,
        hudModeCommand: 0x0200003d, addIslandCommand: 0x02000022, synchronizationConnection: 0, synchronizationReplyMode: 0, gameTicks: 0, cdPresent: 0,
        metaRefresh: 0, metaRebuild: 0, metaCdPresent: 0, metaPending: 0, localServerToken: 0, battleTerritory: 0, metaFirstReset: 1
    };
    // 이름마다 상자의 현재 값과 주소 고정(같은 객체)을 확인한다.
    for (const [name, initial] of Object.entries(dwordInitials)) {
        assert.equal(local[name]().value, initial, name);
        assert.equal(local[name](), local[name](), name);
    }
    for (const name of ['activityEnd', 'captureEnd', 'connectEnd', 'loginEnd', 'reconnectSaveEnd', 'localServerSearchEnd', 'localServerRetryEnd']) {
        assert.equal(local[name]().value, 0, name);
        assert.equal(local[name](), local[name](), name);
    }
    assert.equal(local.invalidTerritory().value, 127);
    assert.equal(local.savedGame().value, false);
    assert.equal(local.simulconnect().value, false);
    assert.ok(local.savedGame() instanceof BoolBox);
    assert.equal(new BoolBox(1).value, true);
    assert.equal(new BoolBox(0).value, false);
    assert.equal(new BoolBox().value, false);
    assert.ok(local.monitorTime() instanceof MonitorTimeState);
    assert.ok(local.clockSamples() instanceof ClockSynchronizationData);
    assert.ok(local.zonePackets() instanceof RawPacketCopies);
    assert.deepEqual([local.ambientSound().deadline, local.ambientSound().kind, local.ambientSound().forced], [0, 0, 0]);
    assert.equal(local.testBattleAddress().length, 64);
    assert.equal(local.previousHandle().length, 64);
    assert.equal(local.playerNoMakeWarnings().length, 255);
    assert.equal(local.metaZoneRecords().length, META_ZONE_BYTES);
    assert.equal(local.zoneWords().length, 2);
    assert.equal(local.zone().length, 6);
    assert.ok(local.blocks().every((block) => block.length === 36 && block.every((word) => word === 0)));
});

test('DWORD 상자는 부호 없는 32비트로, Int32 칸은 signed 로, 불리언은 Boolean 으로 감긴다', () => {
    const local = new ClientLocalStateGlobals();
    local.begin().value = -1;
    assert.equal(local.begin().value, 0xffffffff);
    local.begin().value = 2 ** 32 + 7;
    assert.equal(local.begin().value, 7);
    local.invalidTerritory().value = 0xffffffff;
    assert.equal(local.invalidTerritory().value, -1);
    local.ambientSound().kind = 0x80000000;
    assert.equal(local.ambientSound().kind, -2147483648);
    local.savedGame().value = 5;
    assert.equal(local.savedGame().value, true);
    local.simulconnect().value = 0;
    assert.equal(local.simulconnect().value, false);
    // 시각 기한은 double 그대로 저장한다(DWORD 로 줄이지 않는다).
    local.loginEnd().value = 713.75;
    assert.equal(local.loginEnd().value, 713.75);
    local.activityEnd().value = 2 ** 40 + 0.5;
    assert.equal(local.activityEnd().value, 2 ** 40 + 0.5);
});

test('State 다섯 블록: 36DWORD 다섯 개가 서로 겹치지 않고 state() 의 이름 순서가 old/phase/immediate/current/next 이다', () => {
    const local = new ClientLocalStateGlobals();
    const blocks = local.blocks();
    assert.equal(blocks.length, 5);
    const state = local.state();
    assert.deepEqual(Object.keys(state), ['old', 'phase', 'immediate', 'current', 'next']);
    // 이름마다 같은 배열 객체(정체성)이다. 내용이 모두 0 이라 deepEqual 로는 순서를 구별하지 못한다.
    ['old', 'phase', 'immediate', 'current', 'next'].forEach((name, index) => assert.equal(state[name], blocks[index], name));
    assert.equal(state, local.state());
    assert.equal(local.blocks(), local.blocks());
    assert.equal(local.playerNoMakeWarnings(), local.playerNoMakeWarnings());
    assert.equal(local.zonePackets(), local.zonePackets());
    assert.equal(local.monitorTime(), local.monitorTime());
    assert.equal(local.clockSamples(), local.clockSamples());
    assert.equal(local.zoneWords(), local.zoneWords());
    assert.equal(local.zone(), local.zone());
    assert.equal(local.metaZoneRecords(), local.metaZoneRecords());
    assert.notEqual(local.previousHandle(), local.testBattleAddress());
    // 서로 다른 값으로 채워도 다른 블록이 변하지 않는다(중첩 없음).
    blocks.forEach((block, index) => block.fill(index + 1));
    assert.deepEqual(blocks.map((block) => Array.from(new Set(block))), [[1], [2], [3], [4], [5]]);
    // 기존 RawLocalMissionState 가 이 블록을 그대로 받는다(36DWORD Uint32Array 다섯, 겹침 없음).
    assert.doesNotThrow(() => new RawLocalMissionState(local.state(), {
        player: {}, server: 0, boss: 0, inBattle: 0, missionPost: 0, savedGame: 0, missionWorld: 0, simulconnect: 0, metamap: 0, zonePending: 0, gameTicks: 0, sourceType: 0, cdPresent: 0
    }));
});

test('시간 동기화 뷰: connection/replyMode/gameTicks 가 같은 상자를 읽고 쓰며 resetClockSynchronization 과 ClientConnectionGlobals 가 이 소유자를 그대로 받는다', () => {
    const local = new ClientLocalStateGlobals();
    const clock = local.clockSynchronization();
    assert.equal(clock, local.clockSynchronization());
    clock.connection = 0x80000000;
    clock.replyMode = 0xfffffffe;
    clock.gameTicks = 0x12345678;
    // 세 칸이 서로 다른 값이라 읽기/쓰기가 섞이면 구별된다.
    assert.deepEqual([clock.connection, clock.replyMode, clock.gameTicks], [0x80000000, 0xfffffffe, 0x12345678]);
    assert.equal(local.synchronizationConnection().value, 0x80000000);
    assert.equal(local.synchronizationReplyMode().value, 0xfffffffe);
    assert.equal(local.gameTicks().value, 0x12345678);
    // 상자에 쓴 값이 뷰에도 보인다(복제 없음).
    local.gameTicks().value = 9;
    assert.equal(clock.gameTicks, 9);
    // 부호 있는 입력은 DWORD 로 감긴다.
    clock.connection = -1;
    assert.equal(clock.connection, 0xffffffff);
    // 연결표가 005c8bf4 칸을 빌린다(현재 값은 쓰지 않음).
    const connections = new ClientConnectionGlobals(local);
    assert.ok(connections);
    resetClockSynchronization(local.clockSynchronization());
    assert.deepEqual([local.synchronizationConnection().value, local.synchronizationReplyMode().value, local.gameTicks().value], [0, 0, 0]);
    // 표본과 카운터는 지우지 않는다(세 DWORD 만).
    local.clockSamples().sent = 77;
    local.clockSamples().samples[3][4] = 0xaa;
    resetClockSynchronization(local.clockSynchronization());
    assert.equal(local.clockSamples().sent, 77);
    assert.equal(local.clockSamples().samples[3][4], 0xaa);
});

test('주변음 뷰: deadline 은 activityEnd, kind/forced 는 두 Int32 칸을 빌리고 resetAmbientSoundDeadline 이 같은 기한을 쓴다', () => {
    const local = new ClientLocalStateGlobals();
    const ambient = local.ambientSound();
    assert.equal(ambient, local.ambientSound());
    local.activityEnd().value = 5.5;
    assert.equal(ambient.deadline, 5.5);
    resetAmbientSoundDeadline(ambient, () => 100);
    assert.equal(local.activityEnd().value, 110);
    ambient.kind = 2;
    ambient.forced = -3;
    assert.deepEqual([ambient.kind, ambient.forced], [2, -3]);
    // 두 칸은 서로 독립이다.
    ambient.kind = 0;
    assert.equal(ambient.forced, -3);
});

test('메타 구역 뷰: firstReset 은 PE 초기 1 상자와 같고 records 는 7344BYTE 이며 clearMetaZoneRecords 가 이 뷰를 그대로 받는다', () => {
    const local = new ClientLocalStateGlobals();
    const meta = local.metaZones();
    assert.equal(meta, local.metaZones());
    assert.equal(meta.firstReset, 1);
    assert.equal(meta.records, local.metaZoneRecords());
    assert.equal(meta.records.length, META_ZONE_BYTES);
    meta.records.fill(0xa5);
    // 진입 때 첫 플래그가 비영이면 표 전체를 0 으로 쓰고 플래그를 0 으로 쓴다.
    clearMetaZoneRecords(meta, { clearMarker() { throw new Error('호출 금지'); } });
    assert.equal(local.metaFirstReset().value, 0);
    assert.ok(meta.records.every((byte) => byte === 0));
    meta.firstReset = 0x81;
    assert.equal(local.metaFirstReset().value, 0x81);
    assert.equal(meta.firstReset, 0x81);
});

test('Zone: 8BYTE(DWORD 2칸)의 앞 6BYTE 뷰가 같은 물리이고 인접 상위 WORD 는 Zone 길이에 포함되지 않는다', () => {
    const local = new ClientLocalStateGlobals();
    local.zoneWords()[0] = 0x76543210;
    local.zoneWords()[1] = 0xabcd9876;
    assert.deepEqual(Array.from(local.zone()), [0x10, 0x32, 0x54, 0x76, 0x76, 0x98]);
    local.zone()[5] = 0x11;
    assert.equal(local.zoneWords()[1], 0xabcd1176);
    local.zone().fill(0);
    assert.equal(local.zoneWords()[0], 0);
    assert.equal(local.zoneWords()[1], 0xabcd0000);
});

test('battle: BYTE 쓰기는 같은 DWORD 의 나머지 3BYTE 를 보존하고 DWORD 쓰기가 BYTE 에 보인다', () => {
    const local = new ClientLocalStateGlobals();
    local.battleTerritory().value = 0x11223344;
    assert.equal(local.battleByte().value, 0x44);
    local.battleTerritory().value = 0x11220044;
    local.battleByte().value = 0x1ff;
    assert.equal(local.battleTerritory().value, 0x112200ff);
    local.battleTerritory().value = 0xffffff00;
    assert.equal(local.battleByte().value, 0);
    local.battleByte().value = 7;
    assert.equal(local.battleTerritory().value, 0xffffff07);
});

test('모든 상자는 서로 다른 칸이다: 이름마다 서로 다른 값을 쓰면 다른 이름이 변하지 않는다', () => {
    const local = new ClientLocalStateGlobals();
    const dwordNames = ['debugBurst', 'joinTestBattleCommand', 'viewMetaCommand', 'firstMenuPending', 'userUpdated', 'tellTips', 'tipNumber', 'introAvailable', 'autoDemo', 'zonePending', 'sourceType', 'begin',
        'centralCreated', 'loginProgressSerial', 'restoreOwner', 'restoreSubscriber', 'noRestartSerial', 'scriptBreak', 'abortPending', 'stateAuxCC', 'metadataMode', 'metaConnected', 'connectionNotified',
        'cachedPeerIndex', 'cachedPeerValue', 'cachedPeerFieldA4', 'cachedPeerFieldA8', 'hudModeCommand', 'addIslandCommand', 'synchronizationConnection', 'synchronizationReplyMode', 'gameTicks', 'cdPresent',
        'metaRefresh', 'metaRebuild', 'metaCdPresent', 'metaPending', 'localServerToken', 'battleTerritory', 'metaFirstReset'];
    const doubleNames = ['activityEnd', 'captureEnd', 'connectEnd', 'loginEnd', 'reconnectSaveEnd', 'localServerSearchEnd', 'localServerRetryEnd'];
    // 이름마다 고유한 값(순번 + 0x1000)을 쓴 뒤 전부 다시 읽어 자기 값인지 확인한다.
    dwordNames.forEach((name, index) => { local[name]().value = 0x1000 + index; });
    doubleNames.forEach((name, index) => { local[name]().value = 0.5 + index; });
    dwordNames.forEach((name, index) => assert.equal(local[name]().value, 0x1000 + index, name));
    doubleNames.forEach((name, index) => assert.equal(local[name]().value, 0.5 + index, name));
    // 불리언 두 칸, Int32 한 칸, 두 BYTE 버퍼도 서로 독립이다.
    local.savedGame().value = true;
    assert.equal(local.simulconnect().value, false);
    local.simulconnect().value = true;
    local.savedGame().value = false;
    assert.equal(local.simulconnect().value, true);
    local.invalidTerritory().value = 5;
    assert.deepEqual([local.ambientSound().kind, local.ambientSound().forced], [0, 0]);
    local.previousHandle()[0] = 1;
    assert.equal(local.testBattleAddress()[0], 0);
    local.testBattleAddress()[1] = 2;
    assert.equal(local.previousHandle()[1], 0);
    // 주변음 기한은 activityEnd 이다.
    assert.equal(local.ambientSound().deadline, 0.5);
});

test('두 Client 소유자는 서로 독립이며 한 소유자의 값은 다른 곳에 보이지 않는다', () => {
    const first = new ClientLocalStateGlobals();
    const second = new ClientLocalStateGlobals();
    first.zonePending().value = 0x12345678;
    first.blocks()[3][0] = 3;
    first.zone()[0] = 9;
    first.monitorTime().monitor = 4;
    first.clockSamples().next = 12.5;
    first.playerNoMakeWarnings()[254] = 1;
    first.helpWord().interpret('Help');
    assert.equal(second.zonePending().value, 0);
    assert.equal(second.blocks()[3][0], 0);
    assert.equal(second.zone()[0], 0);
    assert.equal(second.monitorTime().monitor, 0);
    assert.equal(second.clockSamples().next, 0);
    assert.equal(second.playerNoMakeWarnings()[254], 0);
    assert.equal(second.helpWord().get(), null);
});

test('MonitorTimeState: initialized 는 DWORD, monitor/alert 는 signed, deadline 은 double 로 감긴다', () => {
    const state = new MonitorTimeState();
    assert.deepEqual([state.initialized, state.deadline, state.monitor, state.alert], [0, 0, 0, 0]);
    state.initialized = -1;
    assert.equal(state.initialized, 0xffffffff);
    state.monitor = 0xffffffff;
    state.alert = 0x80000000;
    assert.deepEqual([state.monitor, state.alert], [-1, -2147483648]);
    state.deadline = 180.25;
    assert.equal(state.deadline, 180.25);
});

test('ClockSynchronizationData: 70표본×32BYTE, 지연/송신/수신 DWORD, 기한 double', () => {
    const data = new ClockSynchronizationData();
    assert.equal(data.samples.length, 70);
    assert.ok(data.samples.every((sample) => sample.length === 32 && sample.every((byte) => byte === 0)));
    // 표본은 서로 독립 저장소이다.
    data.samples[0][0] = 1;
    assert.equal(data.samples[1][0], 0);
    data.latencyMilliseconds = -1;
    data.sent = 2 ** 32 + 3;
    data.received = 5;
    assert.deepEqual([data.latencyMilliseconds, data.sent, data.received], [0xffffffff, 3, 5]);
    data.next = 1234.5;
    assert.equal(data.next, 1234.5);
});

// ───── RawPacketCopy ─────

test('duplicateRawPacket: packet 이 null 이면 분배기를 읽지 않고 빈 결과이다', () => {
    const dispatch = { registration() { throw new Error('호출 금지'); } };
    assert.deepEqual(duplicateRawPacket(null, 0, null, dispatch), { present: false, bytes: null, allocation: null });
});

test('duplicateRawPacket: observer 를 방향 1 로 한 번 부른 뒤 그 시점의 WORD 길이로 새 할당에 복사한다(observer 가 길이를 바꾸면 바뀐 길이)', () => {
    const dispatch = makeDispatch();
    const seen = [];
    dispatch.observePacket(9, (packet, sending) => {
        seen.push([packet.length, sending]);
        // observer 가 선언 길이를 20 에서 12 로 줄인다.
        packet[2] = 12;
    });
    const packet = makePacket(9, 20);
    const copy = duplicateRawPacket(packet, 0, null, dispatch);
    assert.deepEqual(seen, [[20, 1]]);
    assert.equal(copy.present, true);
    assert.equal(copy.allocation.length, 12);
    assert.deepEqual(Array.from(copy.bytes), Array.from(packet.subarray(0, 12)));
    assert.equal(copy.bytes.buffer, copy.allocation.buffer);
});

test('duplicateRawPacket: 외부 목적지(capacity 충분)는 present, capacity 부족은 빈 결과이며 방향 전용 observer 도 호출한다', () => {
    const dispatch = makeDispatch();
    const directions = [];
    dispatch.observe(5, (sending) => directions.push(sending));
    const packet = makePacket(5, 11);
    const destination = new Uint8Array(16).fill(0xee);
    const copy = duplicateRawPacket(packet, 16, destination, dispatch);
    assert.equal(copy.present, true);
    assert.equal(copy.allocation, null);
    assert.deepEqual(Array.from(destination.subarray(0, 11)), Array.from(packet));
    assert.ok(destination.subarray(11).every((byte) => byte === 0xee));
    assert.deepEqual(directions, [1]);
    // capacity 가 길이보다 작으면 복사하지 않는다(observer 는 이미 호출됨).
    const small = new Uint8Array(4).fill(0xdd);
    const refused = duplicateRawPacket(packet, 10, small, dispatch);
    assert.equal(refused.present, false);
    assert.ok(small.every((byte) => byte === 0xdd));
    assert.deepEqual(directions, [1, 1]);
    // capacity 가 음수(signed)이면 길이 0 이어도 거부 조건을 따른다: -1 < 11
    assert.equal(duplicateRawPacket(packet, -1, null, dispatch).present, false);
    // 목적지가 없는 채 capacity 만 충분하면 present 는 false 이고 길이가 0 이 아니면 쓰기 물리 부족이다.
    assert.ok(throws(() => duplicateRawPacket(packet, 16, null, dispatch), RangeError));
});

test('duplicateRawPacket: 256BYTE 이상 길이(WORD 상위 BYTE), 외부 목적지와 capacity 0, 부호 있는 capacity, 목적지 없는 길이 0, 반환 뷰 길이', () => {
    const dispatch = makeDispatch();
    // 선언 길이 300 은 WORD 두 BYTE 가 모두 필요하다.
    const big = makePacket(5, 300);
    const bigCopy = duplicateRawPacket(big, 0, null, dispatch);
    assert.equal(bigCopy.allocation.length, 300);
    assert.deepEqual(Array.from(bigCopy.bytes), Array.from(big));
    // 외부 목적지가 있으면 capacity 0 은 자동 할당이 아니라 용량 부족 거부이다(길이가 0 이 아닐 때).
    const given = new Uint8Array(32).fill(0x77);
    const noCapacity = duplicateRawPacket(makePacket(5, 12), 0, given, dispatch);
    assert.equal(noCapacity.present, false);
    assert.ok(given.every((byte) => byte === 0x77));
    // capacity 는 signed 32비트로 비교한다: 0xffffffff 는 -1 이라 거부된다.
    assert.equal(duplicateRawPacket(makePacket(5, 12), 0xffffffff, given, dispatch).present, false);
    assert.ok(given.every((byte) => byte === 0x77));
    // 목적지가 없는 채 capacity 가 충분하고 길이가 0 이면 쓰기 없이 present 가 false 이다.
    const zero = makePacket(5, 8);
    zero[2] = 0;
    zero[3] = 0;
    const none = duplicateRawPacket(zero, 5, null, dispatch);
    assert.equal(none.present, false);
    assert.equal(none.bytes.length, 0);
    // 반환 뷰는 복사한 길이만큼이다(목적지가 더 커도).
    const roomy = duplicateRawPacket(makePacket(5, 11), 64, new Uint8Array(64), dispatch);
    assert.equal(roomy.bytes.length, 11);
});

test('duplicateRawPacket: 등록 대여의 핀을 observer 동안 두 겹으로 유지하고 끝나면 모두 반납한다', () => {
    const table = new PacketRegistrationTable();
    const lease = table.bindSession({});
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} }, table);
    const calls = [];
    // 대여 안에서 등록한 observer 가 호출될 때의 활동 수를 기록한다.
    dispatch.observePacket(9, () => calls.push(lease.calls));
    assert.equal(lease.calls, 0);
    const copy = duplicateRawPacket(makePacket(9, 12), 0, null, dispatch);
    assert.equal(copy.present, true);
    // 복제 함수의 핀 하나와 항목 알림의 핀 하나가 겹친다.
    assert.deepEqual(calls, [2]);
    assert.equal(lease.calls, 0);
    // observer 가 예외를 던져도 핀은 반납된다.
    dispatch.observePacket(9, () => { throw new Error('observer 실패'); });
    assert.ok(throws(() => duplicateRawPacket(makePacket(9, 12), 0, null, dispatch)));
    assert.equal(lease.calls, 0);
});

test('duplicateRawPacket: 길이 0 은 소유 할당이 있는 present, 4로 나누어떨어지지 않는 길이는 남은 BYTE 도 복사한다', () => {
    const dispatch = makeDispatch();
    const empty = makePacket(5, 4);
    const none = duplicateRawPacket(empty, 0, null, dispatch);
    assert.equal(none.present, true);
    assert.equal(none.allocation.length, 4);
    // 선언 길이가 0 인 패킷은 새 길이 0 할당을 만든다.
    const zero = makePacket(5, 8);
    zero[2] = 0;
    zero[3] = 0;
    const zeroCopy = duplicateRawPacket(zero, 0, null, dispatch);
    assert.equal(zeroCopy.present, true);
    assert.equal(zeroCopy.allocation.length, 0);
    for (const length of [5, 6, 7, 9]) {
        const packet = makePacket(5, length);
        const copy = duplicateRawPacket(packet, 0, null, dispatch);
        assert.deepEqual(Array.from(copy.bytes), Array.from(packet), `길이 ${length}`);
    }
});

test('duplicateRawPacket: 물리 부족과 타입 범위 밖은 RangeError 이고 겹치는 목적지는 원문 앞방향 복사 순서를 따른다', () => {
    const dispatch = makeDispatch();
    // 헤더(타입/길이) 읽기 물리 부족.
    assert.ok(throws(() => duplicateRawPacket(new Uint8Array(2), 0, null, dispatch), RangeError));
    assert.ok(throws(() => duplicateRawPacket(new Uint8Array(3), 0, null, dispatch), RangeError));
    // 타입이 지원 범위(0..137) 밖이면 분배기가 거부한다.
    const wild = makePacket(200, 8);
    assert.ok(throws(() => duplicateRawPacket(wild, 0, null, dispatch), RangeError));
    // 선언 길이가 실제 버퍼보다 길면 읽기 물리 부족이다.
    const lying = makePacket(5, 8);
    lying[2] = 40;
    assert.ok(throws(() => duplicateRawPacket(lying, 0, null, dispatch), RangeError));
    // 같은 버퍼 안에서 목적지가 원본보다 4BYTE 뒤에 겹치면 앞방향 DWORD 복사로 첫 DWORD 가 반복 전파된다.
    const buffer = new Uint8Array(32);
    buffer.set(makePacket(5, 16, 0x20), 0);
    const source = buffer.subarray(0, 16);
    const overlapped = buffer.subarray(4, 20);
    const first = source.slice(0, 4);
    const copy = duplicateRawPacket(source, 16, overlapped, dispatch);
    assert.equal(copy.present, true);
    // 원본의 DWORD 0 이 목적지 DWORD 0(=원본 DWORD 1 위치)에 쓰여 다음 읽기에서 같은 값이 전파된다.
    assert.deepEqual(Array.from(buffer.subarray(4, 8)), Array.from(first));
    assert.deepEqual(Array.from(buffer.subarray(8, 12)), Array.from(first));
    assert.deepEqual(Array.from(buffer.subarray(12, 16)), Array.from(first));
});

test('RawPacketCopies: duplicate 는 1 부터 재사용 없이 token 을 발급하고 NULL 결과는 0, acquire 는 같은 바이트, release 는 한 번만', () => {
    const dispatch = makeDispatch();
    const copies = new RawPacketCopies();
    const packet = makePacket(5, 12);
    const first = copies.duplicate(packet, dispatch);
    const second = copies.duplicate(packet, dispatch);
    assert.deepEqual([first, second], [1, 2]);
    assert.equal(copies.size, 2);
    assert.deepEqual(Array.from(copies.acquire(first)), Array.from(packet));
    assert.equal(copies.acquire(first), copies.acquire(first));
    assert.notEqual(copies.acquire(first), copies.acquire(second));
    // NULL 패킷은 token 0 이며 장부를 늘리지 않는다.
    assert.equal(copies.duplicate(null, dispatch), 0);
    assert.equal(copies.size, 2);
    // release(0) 은 무시하고 현재 token 은 한 번만 반납한다. 반납한 token 은 재사용하지 않는다.
    copies.release(0);
    copies.release(first);
    assert.equal(copies.size, 1);
    assert.ok(throws(() => copies.release(first), RangeError));
    assert.ok(throws(() => copies.acquire(first), RangeError));
    assert.ok(throws(() => copies.acquire(0), RangeError));
    assert.ok(throws(() => copies.release(99), RangeError));
    assert.equal(copies.duplicate(packet, dispatch), 3);
    // 장부에 넣을 수 없는 입력(빈 할당)은 오류이다.
    assert.ok(throws(() => copies.adopt(null)));
});

test('RawPacketCopies.adopt: 이미 소유한 바이트를 복사 없이 인계하고 token 은 duplicate 와 같은 순서를 쓴다', () => {
    const dispatch = makeDispatch();
    const copies = new RawPacketCopies();
    const owned = new Uint8Array([1, 2, 3]);
    const token = copies.adopt(owned);
    assert.equal(token, 1);
    assert.equal(copies.acquire(token), owned);
    assert.equal(copies.duplicate(makePacket(5, 8), dispatch), 2);
    // observer 는 부르지 않는다.
    const seen = [];
    dispatch.observe(6, (sending) => seen.push(sending));
    copies.adopt(new Uint8Array(2));
    assert.deepEqual(seen, []);
});

test('RawPacketCopies: token 이 DWORD 끝에 닿으면 소진 진단이고 앞서 발급한 token 은 유지된다', () => {
    const copies = new RawPacketCopies({ firstToken: 0xfffffffe });
    assert.equal(copies.adopt(new Uint8Array(1)), 0xfffffffe);
    assert.equal(copies.adopt(new Uint8Array(1)), 0xffffffff);
    // 다음 token 은 0(원문 NULL)이라 발급하지 않는다.
    assert.ok(throws(() => copies.adopt(new Uint8Array(1)), RangeError));
    assert.equal(copies.size, 2);
    assert.ok(copies.acquire(0xffffffff));
});
