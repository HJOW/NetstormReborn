// 단독 미션의 실제 Zone/로그인 바이트와 로컬 두 연결(zlocal 의 클라이언트 9 끝·accepted 서버 끝)을 같은 패킷 분배기에 잇는 모듈이다(cpppj `zacket/LocalMissionTransport.cpp/.h`, 10.78 전용).
// 사용:
//   - 패킷 작성/읽기: makeLocalMissionZone()(004b51e0, Zone 512 예약과 네 슬롯·86바이트 길이) / makeGuideMissionZone(handle)(004b50b0, 한 슬롯) / makeLocalMissionLogin(subscriber, handle, fortImage)(004b63d0, 0x27)
//     → LocalMissionPacket { storage(전체 예약), bytes()(현재 선언 WORD 길이만) }. readLocalMissionLogin(packet) → { subscriber, handle, fortImage }(0040a580/0040a4b0 위치·물리 범위 확인).
//   - installLocalMissionZone(zone, players, player, globals, hooks, port)(004b4be0 + 양수 owner 의 0048f3b0): Zone 을 현재 Player9 에 설치한다. globals 는 같은 Session 전역 묶음의 live 객체(속성을 직접 읽고 쓴다),
//     hooks = { loginTimeout, seconds, storeName(subscriber, name), serve(port), assertion(expression) }(도달한 위치에서 필수).
//   - const wire = new LocalMissionTransport(players, server, hooks[, registry, registrations]); wire.registerLogin(receive); wire.open(port[, connectImmediately]); wire.send(id, bytes); wire.flush(); wire.poll(); …
//     players = Player9 1620BYTE(같은 현재 원문), server = 현재 서버 모드(boolean·`{ value }` 상자·함수), hooks = 분배 장치 { now, record, terminate, close, release[, sent, players] }(raw-packet-dispatch.js 와 같은 형식).
//     registry 는 공유 RawConnectionRegistry(없으면 새로 만듦), registrations 는 공유 PacketRegistrationTable(없으면 분배기가 새로 만듦)이다.
// 이름(handle·Player 이름)은 "바이트 = 문자 코드" 이진 문자열로 주고받는다(raw-mission-client-packets.js 와 같은 관례). 종료된 CD 파일 확인 요청 0x7f/응답 0x80 은 AGENTS 의 클라이언트 유효성 검사 비활성화 요구로
// 송수신하지 않는다(cpppj 와 같음). TCP/SPX/OS 소켓 전체를 복원하는 클래스가 아니며 simulconnect 전역을 바꾸지 않는다.
// 기준선 뒤 cpppj 변경 판정(LEFT_JOBS 3.6절, 단위 문서 2절): ② 반영 = 송신 선택을 현재 Registry 노드로(pending/owner 숫자 사본 대신)·가상 송신 장치의 직접 노드 송신·`players` 현재 Player9 조회·`sent` 송신 기록·
//   Track 의 소켓 준비/공통 buffered 장치·재배치 관문의 현재 노드 확인·`owner()` 의 현재 양수 칸 재조회·0x87 의 비NULL 빈 관찰기·`ownsConnection`/`rebindHooks`. ① = 집합 프레임 분배의 공통 부모(RawPacketFrame) 이동(지연 FIFO 만 다르며 웹 제외).
//   ③ 4-D = 생성자의 접속 통지 수신기 등록(RegisterConnectPacketReceiver, 실제 TCP 초기 패킷 복제와 고정 Native 수신기에 기댐). CD(10.72)는 옮기지 않았다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E324: 4-C zacket/LocalMissionTransport 이식으로 추가)
import { PacketConnection, RawPacketDispatch, packetPlayerActive } from './raw-packet-dispatch.js';
import { RawConnectionRegistry } from './raw-connection-registry.js';
import { PENDING_CONNECTION_SLOTS, TRANSCEIVER_SLOTS } from './raw-connection-cleanup.js';
import { currentTransportToken, transportPollResult } from './raw-transport-polling.js';

// 원본 Player 기록 폭/개수, 패킷 헤더 폭, Zone 예약·client 레코드·머리 폭이다. (2026-11-12 WB04-E324)
const PLAYER_BYTES = 180;
const PLAYERS = 9;
const HEADER = 4;
const ZONE_RESERVE = 512;
const ZONE_CLIENT = 71;
const ZONE_HEAD = 15;

// PATCH 초기 패킷 종류(Zone 0x1a·로그인 0x27)와 버전, 집합 프레임 종류, 연결 닫힘 flags(0x40), 0x87 bootstrap 의 선언 길이이다. (2026-11-12 WB04-E324)
const ZONE_TYPE = 0x1a;
const LOGIN_TYPE = 0x27;
const VERSION = 1;
const BATCH_TYPE = 4;
const CLOSED = 0x40;
const BOOTSTRAP_TYPE = 0x87;
const BOOTSTRAP_LENGTH = 0x112;

// buffered 송신 누적의 상한(헤더 4바이트를 더해 WORD 65535 안)이다. (2026-11-12 WB04-E324)
const BUFFERED_LIMIT = 65531;

// 004b51e0 의 실제 네 정적 이름(바이트 문자열)이다. (2026-11-12 WB04-E324)
const ZONE_NAMES = Object.freeze(['zsimpson', 'jgreer', 'kdemarest', 'tbratton']);

// 종료된 CD 파일 확인 요청 0x7f/응답 0x80 이면 참이다: 제품에서 송수신하지 않는다(AGENTS 클라이언트 유효성 검사 비활성화). (2026-11-12 WB04-E324)
const isRetiredCdValidationPacket = (type) => type === 0x7f || type === 0x80;

// 실제 비정렬 little endian BYTE/WORD/DWORD 를 지정 폭으로 읽는다(부호 없음, 범위 밖은 거부). (2026-11-12 WB04-E324)
function read(bytes, offset, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset > bytes.length || width > bytes.length - offset) throw new RangeError('로컬 미션 원문 읽기 범위 오류');
    let value = 0;
    // 낮은 바이트부터 같은 폭의 원본 비트를 모은다.
    for (let index = 0; index < width; index += 1) value = (value | (bytes[offset + index] << (8 * index))) >>> 0;
    return value;
}

// 주변 stale 바이트를 유지하며 지정 원본 폭만 쓴다(범위 밖은 거부). (2026-11-12 WB04-E324)
function put(bytes, offset, value, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset > bytes.length || width > bytes.length - offset) throw new RangeError('로컬 미션 원문 쓰기 범위 오류');
    // 원본 little endian 의 바이트 위치만 바꾼다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 원본 C 문자열의 첫 NUL 까지(NUL 제외)를 같은 저장소의 보기로 빌린다. 저장소 안에 NUL 이 없으면 거부한다. (2026-11-12 WB04-E324)
function text(bytes, offset) {
    if (!Number.isInteger(offset) || offset < 0 || offset > bytes.length) throw new RangeError('로컬 미션 문자열 위치 오류');
    const end = bytes.indexOf(0, offset);
    if (end < 0) throw new RangeError('로컬 미션 원문 문자열 NUL 누락');
    return bytes.subarray(offset, end);
}

// 원본 strcpy 처럼 NUL 까지 복사한다. 선언 필드 폭을 넘어도 실제 물리 저장소 안에서는 쓰기를 유지하고, 저장소를 넘으면 쓰기 전에 거부한다. source 는 NUL 없는 바이트 보기이다. (2026-11-12 WB04-E324)
function copy(bytes, offset, source) {
    if (offset >= bytes.length || source.length >= bytes.length - offset) throw new RangeError('로컬 미션 strcpy 물리 저장소 부족');
    // 원본 바이트를 앞에서부터 옮긴다(겹치는 저장소에서도 앞쪽부터 복사하는 strcpy 순서).
    for (let index = 0; index < source.length; index += 1) bytes[offset + index] = source[index];
    bytes[offset + source.length] = 0;
}

// handle 입력(이진 문자열 또는 Uint8Array)을 첫 NUL 앞까지의 바이트로 바꾼다. 문자 코드가 0xff 를 넘으면 원본 BYTE 문자열이 아니므로 거부한다. (2026-11-12 WB04-E324)
function handleBytes(handle) {
    let bytes;
    if (handle instanceof Uint8Array) bytes = handle;
    else if (typeof handle === 'string') {
        bytes = new Uint8Array(handle.length);
        // 각 문자 코드를 한 BYTE 로 옮긴다.
        for (let index = 0; index < handle.length; index += 1) {
            const code = handle.charCodeAt(index);
            if (code > 0xff) throw new TypeError('로컬 미션 이름은 BYTE 문자열이어야 합니다');
            bytes[index] = code;
        }
    } else throw new TypeError('로컬 미션 이름은 문자열 또는 Uint8Array 여야 합니다');
    const end = bytes.indexOf(0);
    return end < 0 ? bytes : bytes.subarray(0, end);
}

// 바이트 보기를 "바이트 = 문자 코드" 이진 문자열로 바꾼다. (2026-11-12 WB04-E324)
function binary(bytes) {
    let result = '';
    // 큰 이름에서도 인자 개수 한계에 걸리지 않게 한 글자씩 붙인다.
    for (let index = 0; index < bytes.length; index += 1) result += String.fromCharCode(bytes[index]);
    return result;
}

// 선택된 실제 외부 몸체의 누락을 빈 성공으로 바꾸지 않는다(도달한 위치에서 오류). (2026-11-12 WB04-E324)
function need(device) {
    if (typeof device !== 'function') throw new Error('로컬 미션 실제 하위 소비자 미연결');
    return device;
}

// 원본 RawCheckingFlag 처럼 현재 전체 DWORD(값 또는 매번 읽는 함수)의 비영 여부이다. (2026-11-12 WB04-E324)
function flagSet(value) {
    const current = typeof value === 'function' ? value() : value;
    return Number(current) !== 0;
}

// 현재 서버 모드를 읽는다: 함수면 호출, `{ value }` 상자면 값, 그 밖에는 boolean 상수이다. (2026-11-12 WB04-E324)
function liveBoolean(source) {
    if (typeof source === 'function') return () => Boolean(source());
    if (source && typeof source === 'object' && 'value' in source) return () => Boolean(source.value);
    if (typeof source === 'boolean') return () => source;
    throw new TypeError('로컬 전송의 서버 모드는 boolean·{ value } 상자·함수여야 합니다');
}

// 분배 장치 다섯 개(now/record/terminate/close/release)와 선택 장치(sent/players)의 형식을 확인한다. 감싼 함수의 존재로 원래 장치 누락을 가리지 않는다. (2026-11-12 WB04-E324)
function checkHooks(hooks, message) {
    // 필수 다섯 효과가 모두 함수인지 본다.
    for (const name of ['now', 'record', 'terminate', 'close', 'release']) {
        if (typeof hooks?.[name] !== 'function') throw new TypeError(message);
    }
    // 선택 장치는 있으면 함수여야 한다.
    for (const name of ['sent', 'players']) {
        if (hooks[name] !== undefined && hooks[name] !== null && typeof hooks[name] !== 'function') throw new TypeError(`로컬 전송 선택 장치 ${name} 는 함수여야 합니다`);
    }
}

// 원본 예약 전체와 observer 가 확정한 선언 길이를 함께 보관하는 패킷이다. (2026-11-12 WB04-E324)
export class LocalMissionPacket {
    // Zone 512/로그인 FortImage+67 의 전체 물리 바이트이다.
    storage;

    // 전체 예약 바이트로 만든다. (2026-11-12 WB04-E324)
    constructor(storage) {
        this.storage = storage;
    }

    // 실제 송신/State 이미지 복사가 쓰는 현재 선언 WORD 길이만 빌린다(같은 저장소 보기). 선언 길이가 예약보다 크면 거부한다. (2026-11-12 WB04-E324)
    bytes() {
        const length = read(this.storage, 2, 2);
        if (length > this.storage.length) throw new RangeError('로컬 미션 패킷 선언 길이 초과');
        return this.storage.subarray(0, length);
    }
}

// 004df590 의 실제 전체 0xff·종류·버전·최소 4바이트 헤더이다. (2026-11-12 WB04-E324)
function makePacket(reserve, type) {
    const storage = new Uint8Array(reserve).fill(0xff);
    storage[0] = VERSION;
    storage[1] = type;
    put(storage, 2, HEADER, 2);
    return new LocalMissionPacket(storage);
}

// numClients 1 과 별개로 원본이 실제 쓰는 네 슬롯을 모두 작성한 뒤 첫 슬롯 하나 기준의 observer 길이(86)를 적용한다. (2026-11-12 WB04-E324)
export function makeLocalMissionZone() {
    const packet = makePacket(ZONE_RESERVE, ZONE_TYPE);
    const bytes = packet.storage;
    put(bytes, 4, 0x82000004);
    put(bytes, 8, 0, 2);
    put(bytes, 10, 0);
    bytes[14] = 1;
    // 004b51e0 의 네 번 쓰기를 보존한다. State 에 복제되는 길이는 마지막에 첫 슬롯 하나로 확정된다.
    for (let index = 0; index < ZONE_NAMES.length; index += 1) {
        const offset = ZONE_HEAD + index * ZONE_CLIENT;
        bytes[offset] = index;
        put(bytes, offset + 1, index + 1);
        put(bytes, offset + 5, 0);
        copy(bytes, offset + 9, handleBytes(ZONE_NAMES[index]));
        bytes[offset + 41] = 0;
    }
    put(bytes, 2, ZONE_HEAD + bytes[14] * ZONE_CLIENT, 2);
    return packet;
}

// 004b50b0/463b30 은 같은 Zone 표식과 한 슬롯의 현재 handle 만 쓰고 나머지 예약 0xff 를 보존한다. 선언 길이는 관찰 전 4 이다. (2026-11-12 WB04-E324)
export function makeGuideMissionZone(handle) {
    const packet = makePacket(ZONE_RESERVE, ZONE_TYPE);
    const bytes = packet.storage;
    put(bytes, 4, 0x82000004);
    put(bytes, 8, 0, 2);
    put(bytes, 10, 0);
    bytes[14] = 1;
    bytes[ZONE_HEAD] = 0;
    put(bytes, ZONE_HEAD + 1, 1);
    put(bytes, ZONE_HEAD + 5, 0);
    copy(bytes, ZONE_HEAD + 9, handleBytes(handle));
    bytes[ZONE_HEAD + 41] = 0;
    return packet;
}

// 실제 파일 이미지 이후의 0x27 작성만 복원한다(Template 읽기/파일 선택은 상위 원본 부모 책임). nameLen 은 BYTE 로 감기고 선언 길이는 이미지 크기의 하위 WORD 를 쓴다. (2026-11-12 WB04-E324)
export function makeLocalMissionLogin(subscriber, handle, fortImage) {
    const name = handleBytes(handle);
    if (!(fortImage instanceof Uint8Array)) throw new TypeError('로컬 로그인 Fort 이미지는 Uint8Array 여야 합니다');
    if (fortImage.length > 0xffffffff - 67) throw new RangeError('로컬 로그인 이미지 크기 초과');
    const packet = makePacket(fortImage.length + 67, LOGIN_TYPE);
    const bytes = packet.storage;
    put(bytes, 8, subscriber >>> 0);
    bytes[12] = (name.length + 1) & 0xff;
    copy(bytes, 17, name);
    put(bytes, 13, fortImage.length);
    const start = 17 + bytes[12];
    if (start > bytes.length || fortImage.length > bytes.length - start) throw new RangeError('로컬 로그인 실제 이미지 복사 범위 초과');
    bytes.set(fortImage, start);
    put(bytes, 2, 17 + bytes[12] + read(bytes, 13, 2), 2);
    return packet;
}

// 선언 길이를 고쳐 만든 0x27 을 읽되 손상된 원본의 범위 밖 접근은 명시적으로 거부한다: +8 subscriber DWORD, +12 nameLen BYTE(NUL 포함 범위 검증), +13 이미지 크기 DWORD, +17 이름, +17+nameLen 이미지. (2026-11-12 WB04-E324)
export function readLocalMissionLogin(packet) {
    if (!(packet instanceof Uint8Array) || packet.length < 17 || packet[1] !== LOGIN_TYPE) throw new TypeError('실제 0x27 로그인 패킷 누락');
    const start = 17 + packet[12];
    const size = read(packet, 13);
    if (start > packet.length || size > packet.length - start) throw new RangeError('실제 0x27 이미지 물리 범위 부족');
    const name = text(packet, 17);
    if (name.length + 1 > packet[12]) throw new RangeError('실제 0x27 nameLen 의 NUL 범위 부족');
    return { subscriber: read(packet, 8), handle: binary(name), fortImage: packet.subarray(start, start + size) };
}

// PATCH 004b4be0 과 양수 owner 의 0048f3b0 을 실제 Player9 에 적용한다(로그인 성공 flags 2 는 쓰지 않는다). 설정/시각/Chat 콜백 사이에도 현재 Zone·Player·전역을 다시 읽는다.
// globals(live): server·boss(쓰기), mySubscriber·pendingStamp(읽기), activeStamp·activeZoneWord·zoneFlags·earlyCaptureEnd(쓰기), adjustZoneFlags(값/함수 DWORD), missionWorld·zoneFlagOverride·metamap(읽기),
//   currentCoreData(쓰기: Player 칸의 10BYTE 보기 또는 cachedCoreData), cachedCoreData(metamap 일 때 10BYTE). player = PlayerWorldState(templateOwner·battleOwner·playerKnown). (2026-11-12 WB04-E324)
export function installLocalMissionZone(zone, players, player, globals, hooks, port) {
    if (!(players instanceof Uint8Array) || players.length !== PLAYERS * PLAYER_BYTES || !(zone instanceof Uint8Array) || zone.length < ZONE_HEAD) throw new TypeError('로컬 Zone 실제 Player9/헤더 부족');
    const clients = zone[14];
    if (!clients || clients > 8) need(hooks.assertion)('numClients > 0 && numClients < 9');
    if (ZONE_HEAD + clients * ZONE_CLIENT > zone.length) throw new RangeError('로컬 Zone 실제 client 레코드 부족');
    globals.server = true;
    globals.boss = true;
    globals.activeStamp = globals.pendingStamp >>> 0;
    globals.activeZoneWord = read(zone, 10);
    globals.zoneFlags = read(zone, 4);
    if (flagSet(globals.adjustZoneFlags) && !globals.missionWorld) globals.zoneFlags = ((globals.zoneFlags & ~0x3f) | (globals.zoneFlagOverride & 0x3f)) >>> 0;
    if (globals.earlyCaptureEnd !== 0) need(hooks.assertion)('!earlyCaptureTimer.isRunning()');
    const timeout = need(hooks.loginTimeout)();
    const delay = ((timeout | 0) + 120) | 0;
    globals.earlyCaptureEnd = delay + need(hooks.seconds)();
    // 원본 numClients 의 현재 슬롯 순서만 방문한다(매 반복 현재 Zone+14 를 다시 읽는다). 작성돼 있지만 개수 밖인 나머지 슬롯은 설치하지 않는다.
    for (let index = 0; index < zone[14]; index += 1) {
        const offset = ZONE_HEAD + index * ZONE_CLIENT;
        if (offset + ZONE_CLIENT > zone.length) throw new RangeError('현재 로컬 Zone client 원문 부족');
        if (!read(zone, offset + 1)) need(hooks.assertion)('zac->subId');
        const owner = zone[offset] + 1;
        if (owner < 1 || owner >= PLAYERS) throw new RangeError('로컬 Zone 실제 Player 번호 초과');
        const record = owner * PLAYER_BYTES;
        if (read(zone, offset + 1) === (globals.mySubscriber >>> 0)) {
            player.templateOwner = owner;
            player.battleOwner = owner;
            player.playerKnown = 1;
            if (globals.metamap) {
                if (!(globals.cachedCoreData instanceof Uint8Array) || globals.cachedCoreData.length !== 10) throw new TypeError('실제 metamap coreData 10BYTE 부족');
                globals.currentCoreData = globals.cachedCoreData;
            } else globals.currentCoreData = players.subarray(record + 0x6c, record + 0x6c + 10);
            put(players, record + 0x50, (read(players, record + 0x50) | 8) >>> 0);
        }
        copy(players, record + 12, text(zone, offset + 9));
        need(hooks.storeName)(read(zone, offset + 1), binary(text(players, record + 12)));
        put(players, record + 0x2c, read(zone, offset + 1));
        if (!read(players, record + 0x2c)) need(hooks.assertion)('p->subscriberId');
        copy(players, record + 0x30, text(zone, offset + 41));
        if (zone[offset + 5] & 1) put(players, record + 0x50, (read(players, record + 0x50) | 0x200) >>> 0);
    }
    need(hooks.serve)(port);
}

// 연결 노드의 필드를 같은 객체 안에서 초기값으로 되돌린다(원문 `client_={}` 대입: 주소 동일성 유지). (2026-11-12 WB04-E324)
function resetConnection(node) {
    node.flags = 0;
    node.sender = 0;
    node.received = 0;
    node.heartbeat = 0;
    node.depth = 0;
    node.delayed = false;
    node.address.fill(0);
}

// buffered 송신 누적 버퍼이다(아직 flush 하지 않은 바이트 조각과 총 길이). (2026-11-12 WB04-E324)
class OutputBuffer {
    // 아직 flush 하지 않은 송신 조각(사본)과 그 합계 길이이다.
    chunks = [];
    length = 0;

    // 조각 사본을 덧붙인다. (2026-11-12 WB04-E324)
    append(bytes) {
        this.chunks.push(Uint8Array.from(bytes));
        this.length += bytes.length;
    }

    // 누적을 비운다. (2026-11-12 WB04-E324)
    clear() {
        this.chunks = [];
        this.length = 0;
    }
}

// 실제 zlocal 의 클라이언트 9 / accepted 서버 두 끝과 byte 큐를 단독 소유하는 내부 전송이다. pending 번호는 공유 등록소가 고른다. (2026-11-12 WB04-E324)
export class LocalMissionTransport {
    // 같은 현재 Player9 원문(사본 아님)과 004e27b0 이 현재 읽는 서버 모드 조회이다.
    #players;
    #serverMode;
    // 같은 실제 시계/기록/종료/반납 경계(rebindHooks 로 바꿀 수 있음)이다.
    #hooks;
    // 생성/재배치/분배/State 정리가 같은 실제 표를 빌리는 연결 등록소와, 로그인/건설/응답이 공유하는 실제 분배기이다.
    #registry;
    #dispatch;
    // 서로 다른 실제 연결 저장소(클라이언트 9 끝·서버 끝)이다.
    #client = new PacketConnection();
    #server = new PacketConnection();
    // 실제 두 끝의 열림/서버 pending/서버 buffered 모드, per-ID 폴링 재진입과 내부 listener 의 한 수락 세대이다.
    #open = false;
    #pending = false;
    #serverBuffered = false;
    #polling = false;
    #accepted = false;
    // client/server 의 실제 마지막 수신 version BYTE 이며 type 1/2 통지가 보존한다.
    #receiveVersion = [0xff, 0xff];
    // 실제 Client9 접속 생성 완료 여부(flags 변경으로 지우지 않음), accepted 서버 끝의 존재, 반대 끝 소멸 표시이다.
    #clientPresent = false;
    #serverPresent = false;
    #clientPeerLost = false;
    #serverPeerLost = false;
    // 현재 실제 양끝의 등록소 토큰과 첫 빈 pending 번호(소유자 성공값과 다름)이다.
    #clientToken = 0;
    #serverToken = 0;
    #pendingId = 0;
    // pending 재배치 뒤의 owner 기록과 실제 열린 로컬 포트이다.
    #owner = 0;
    #port = 0;
    // 원본 buffered 전송의 아직 flush 하지 않은 바이트와, 다른 끝이 실제로 받을 완성 프레임의 소유 큐이다.
    #clientOutput = new OutputBuffer();
    #serverOutput = new OutputBuffer();
    #toServer = [];
    #toClient = [];

    // 같은 Player9 와 서버 전역/실제 분배 효과를 빌린다. hooks 의 다섯 경계는 모두 실제 소비자여야 한다. 0x87 bootstrap 의 비NULL 빈 관찰기·버전 1/정책 0·길이 0x112 를 공유 표에 둔다
    // (클라이언트 수신기는 추가하지 않는다). 접속 통지 수신기 등록은 4-D 에서 옮긴다(3.6절 ③). (2026-11-12 WB04-E324)
    constructor(players, server, hooks, registry = null, registrations = null) {
        if (!(players instanceof Uint8Array) || players.length !== PLAYERS * PLAYER_BYTES) throw new TypeError('로컬 전송 같은 실제 Player9 누락');
        checkHooks(hooks, '로컬 전송의 실제 분배 장치 누락');
        if (registry !== null && !(registry instanceof RawConnectionRegistry)) throw new TypeError('로컬 전송의 연결 등록소 형식 오류');
        this.#players = players;
        this.#serverMode = liveBoolean(server);
        this.#hooks = hooks;
        this.#registry = registry ?? new RawConnectionRegistry();
        this.#dispatch = new RawPacketDispatch(this.#wrappedHooks(), registrations);
        this.#dispatch.observe(BOOTSTRAP_TYPE, () => {});
        this.#dispatch.version(BOOTSTRAP_TYPE, 1, 0);
        this.#dispatch.requireLength(BOOTSTRAP_TYPE, BOOTSTRAP_LENGTH);
    }

    // C++ 소멸자: 호스트 owner 가 실제로 없애는 자기 두 끝의 남은 별칭/장치만 반납한다(Native 부모를 대신 실행하지 않음). (2026-11-12 WB04-E324)
    dispose() {
        this.#registry.forget(this.#clientToken);
        this.#registry.forget(this.#serverToken);
    }

    // 같은 실제 두 주소의 현재 토큰만 읽는다(다른 주소는 오류). (2026-11-12 WB04-E324)
    #token(connection) {
        if (connection === this.#client) return this.#clientToken;
        if (connection === this.#server) return this.#serverToken;
        throw new Error('로컬 전송의 다른 연결 주소');
    }

    // 현재 실제 끝 하나의 수명(소멸·반납)·가상 송신(같은 노드 직접)·소켓 준비·공통 buffered 장치를 등록소에 묶고 토큰을 받는다. (2026-11-12 WB04-E324)
    #track(node) {
        const isClient = node === this.#client;
        return this.#registry.track(node,
            // 소멸: 실제 close 장치 뒤 반대 끝의 상실을 표시하고 자기 송신 누적과 두 수신 큐를 비운다.
            () => {
                this.#hooks.close(node);
                if (isClient) {
                    this.#serverPeerLost = true;
                    this.#clientOutput.clear();
                } else {
                    this.#clientPeerLost = true;
                    this.#serverOutput.clear();
                }
                this.#toServer = [];
                this.#toClient = [];
            },
            // 반납: 실제 free 장치 뒤 그 끝의 존재(서버면 pending/owner 기록도)를 지운다.
            () => {
                this.#hooks.release(node);
                if (isClient) this.#clientPresent = false;
                else {
                    this.#serverPresent = false;
                    this.#pending = false;
                    this.#owner = 0;
                }
            },
            // 가상 +4 송신: 같은 실제 노드를 ID 재선택/Player 재검사 없이 쓴다(기준선 뒤 ②).
            (packet) => { this.#sendPacket(node.sender | 0, packet, packet, node); },
            // 소켓 준비: 끝이 존재하고 반대 끝이 살아 있으면 1, 아니면 0 이다(기준선 뒤 ②).
            () => ((isClient ? this.#clientPresent && !this.#clientPeerLost : this.#serverPresent && !this.#serverPeerLost) ? 1 : 0),
            // 공통 buffered 입구: flags 2·서버 buffered 모드·송신 누적 초기화이다(기준선 뒤 ②).
            () => {
                node.flags |= 2;
                if (!isClient) this.#serverBuffered = true;
                (isClient ? this.#clientOutput : this.#serverOutput).clear();
            });
    }

    // 실제 분배와 집합 분배가 같은 노드/수명 장치를 쓰도록 감싼 분배 장치이다. 다른 backend(Editor/TCP)가 빌리는 외부 노드는 원래 장치에만 전달하고,
    // 자기 두 끝은 같은 현재 세대만 등록소의 소멸/반납으로 처리한다(이미 반납한 끝의 재요청만 생략). (2026-11-12 WB04-E324)
    #wrappedHooks() {
        return {
            // 시계·기록·종료는 호출 때의 현재 장치(rebindHooks 반영)로 그대로 넘긴다.
            now: () => this.#hooks.now(),
            record: (connection, bytes, length) => this.#hooks.record(connection, bytes, length),
            terminate: (connection) => this.#hooks.terminate(connection),
            // 소멸: 외부 노드는 원래 장치로, 자기 끝은 같은 세대 확인 뒤 등록소 소멸 장치로 보낸다.
            close: (connection) => {
                if (connection !== this.#client && connection !== this.#server) {
                    this.#hooks.close(connection);
                    return;
                }
                const token = this.#token(connection);
                const present = connection === this.#client ? this.#clientPresent : this.#serverPresent;
                if (!this.#registry.owns(token)) {
                    if (present) throw new Error('살아 있는 내부 close 의 현재 토큰 누락');
                    return;
                }
                if (!present || this.#registry.node(token) !== connection) throw new Error('내부 close 의 다른 세대/주소');
                this.#registry.destroy(token);
            },
            // 반납: 외부 노드는 원래 장치로, 자기 끝은 같은 세대 확인 뒤 등록소 반납(별칭 제거 포함)으로 보낸다.
            release: (connection) => {
                if (connection !== this.#client && connection !== this.#server) {
                    this.#hooks.release(connection);
                    return;
                }
                const token = this.#token(connection);
                const present = connection === this.#client ? this.#clientPresent : this.#serverPresent;
                if (!this.#registry.owns(token)) {
                    if (present) throw new Error('살아 있는 내부 free 의 현재 토큰 누락');
                    return;
                }
                if (!present || this.#registry.node(token) !== connection) throw new Error('내부 free 의 다른 세대/주소');
                this.#registry.release(token);
            },
        };
    }

    // 수신 중에는 건드리지 않고 다음 실제 폴링 경계에서만 닫힌 끝(깊이 0·flags 0x40)을 반납한다. (2026-11-12 WB04-E324)
    #retireClosed() {
        // 서버 끝 → 클라이언트 끝 순서로 각 끝의 현재 존재를 매번 다시 읽는다.
        for (const isClient of [false, true]) {
            const node = isClient ? this.#client : this.#server;
            if ((isClient ? this.#clientPresent : this.#serverPresent) && !node.depth && (node.flags & CLOSED)) {
                const token = this.#token(node);
                this.#registry.destroy(token);
                this.#registry.release(token);
            }
        }
    }

    // 0x27 버전 1/정책 0 과 실제 로그인 몸체(004a7800 상위)를 등록한다. 몸체가 없으면 거부한다(대용 flags 2 를 쓰지 않는다). receive(sender, connection, packet). (2026-11-12 WB04-E324)
    registerLogin(receive) {
        if (typeof receive !== 'function') throw new TypeError('실제 0x27 로그인 처리기 누락');
        this.#dispatch.version(LOGIN_TYPE, VERSION, 0);
        this.#dispatch.register(LOGIN_TYPE, receive);
    }

    // 원본 zlocal listener 를 열고 요청하면 client9 만 만든다. 서버 끝은 실제 poll 의 accept 에서 등록한다. 이미 열렸거나 끝이 남았거나 폴링/콜백 중이거나 포트 범위 밖이거나 로그인 처리기가 없으면 거부한다. (2026-11-12 WB04-E324)
    open(port, connectImmediately = true) {
        if (this.#open || this.#clientPresent || this.#serverPresent || this.#polling || this.#dispatch.registrations.connectionCallbackActive()
            || !Number.isInteger(port) || !port || port > 65535 || !this.#dispatch.registration(LOGIN_TYPE).receive) {
            throw new Error('로컬 서버 포트/새 수명/실제 로그인 처리기 계약 오류');
        }
        this.#port = port;
        resetConnection(this.#client);
        resetConnection(this.#server);
        this.#client.sender = 9;
        this.#owner = 0;
        this.#pending = false;
        this.#accepted = false;
        this.#serverBuffered = false;
        this.#clientPresent = false;
        this.#serverPresent = false;
        this.#clientPeerLost = false;
        this.#serverPeerLost = false;
        this.#clientToken = 0;
        this.#serverToken = 0;
        this.#receiveVersion = [0xff, 0xff];
        this.#open = true;
        this.#client.address[0] = 20;
        this.#client.address[2] = (port >> 8) & 0xff;
        this.#client.address[3] = port & 0xff;
        put(this.#client.address, 16, 0x10d);
        this.#server.address.set(this.#client.address);
        if (connectImmediately) this.connect();
    }

    // 이미 열린 로컬 서버에 실제 클라이언트 9 끝을 접속한다(Player 성공 flag 는 쓰지 않는다). 시계 두 번(수신·heartbeat) 뒤 flags 0x26·존재·토큰·9번 칸 등록 순서이며, 시계 예외의 앞선 쓰기는 되돌리지 않는다. (2026-11-12 WB04-E324)
    connect() {
        if (!this.#open || this.#clientPresent || this.#client.flags || this.#owner || this.#accepted) throw new Error('로컬 클라이언트 새 접속/현재 서버 계약 오류');
        if (this.#registry.cleanup().transceivers[9]) throw new Error('현재 Client9 연결 중복 생성');
        this.#client.received = this.#hooks.now();
        this.#client.heartbeat = this.#hooks.now();
        this.#client.flags = 0x26;
        this.#clientPresent = true;
        this.#clientToken = this.#track(this.#client);
        this.#registry.register(9, this.#clientToken);
    }

    // 실제 연결된 내부 client 가 있을 때만 같은 등록소의 첫 빈 pending 에 서버 끝(sender 0·flags 0x24·시계 두 번)을 수락한다. 현재 전역 poll 안에서만 가능하다. (2026-11-12 WB04-E324)
    acceptPending() {
        if (!this.#polling) throw new Error('내부 accept 는 현재 전역 Poll 안에서만 가능합니다');
        if (!this.#open || !this.#clientPresent || this.#accepted || this.#clientPeerLost || !this.#registry.canAccept()) return;
        this.#server.sender = 0;
        this.#server.flags = 0x24;
        this.#server.received = this.#hooks.now();
        this.#server.heartbeat = this.#hooks.now();
        this.#serverPresent = true;
        this.#serverToken = this.#track(this.#server);
        this.#pendingId = this.#registry.accept(this.#serverToken);
        this.#pending = true;
        this.#accepted = true;
    }

    // 004e27b0: 현재 pending 을 같은 연결 객체의 owner 1..8 로 옮기고 buffered 전송을 켠다(새 연결이나 성공 flag 를 합성하지 않음). 서버 모드·열림·현재 pending 번호·현재 노드가 같은 서버 끝이어야 한다. (2026-11-12 WB04-E324)
    assignConnection(pending, owner) {
        if (!this.#serverMode() || !this.#open || !this.#pending || pending !== this.#pendingId || !Number.isInteger(owner) || owner < 1 || owner >= PLAYERS
            || this.owner() || this.connection(pending) !== this.#server) {
            throw new Error('로컬 실제 pending-1 재배치 계약 오류');
        }
        this.#registry.assign(pending, owner, this.#serverMode() ? 1 : 0, (token) => {
            if (token !== this.#serverToken) throw new Error('현재 로컬 재배치의 다른 buffered 연결');
            this.#server.flags |= 2;
            this.#serverBuffered = true;
            this.#serverOutput.clear();
        });
        this.#owner = owner;
        this.#pending = false;
    }

    // 004e2dc0: 이미 길이를 확정한 패킷을 현재 연결로 보낸다(없는 연결의 자연 생략은 false). (2026-11-12 WB04-E324)
    send(destination, packet) {
        return this.#sendPacket(destination, packet, null, null);
    }

    // 실제 thiscall observer 가 길이를 확정하는 원시 패킷을 같은 큐에 넣는다(연결이 선택된 뒤 observer(1) 를 실행하고 그 현재 WORD 로 복사). (2026-11-12 WB04-E324)
    sendMutable(destination, packet) {
        return this.#sendPacket(destination, packet, packet, null);
    }

    // 같은 현재 ID/주소/수명 표를 장치 getter 에 빌린다. (2026-11-12 WB04-E324)
    get registry() {
        return this.#registry;
    }

    // 번호 송신은 현재 Registry 노드를 고르고 1..8 은 현재 Player9 활성 비트를 확인한다. direct(가상 this)는 같은 고정 끝을 직접 고른다. 연결이 선택된 뒤 원시 작성기만 observer(1) 를 부르고,
    // observer 뒤 노드가 반납/재배치됐으면 보내지 않는다. 서버 끝이 아직 buffered 가 아니면 완성 프레임을 바로 상대 큐에, 아니면 송신 누적에 넣고 `sent` 를 기록한다. (2026-11-12 WB04-E324: 기준선 뒤 ② 선택 방식 반영)
    #sendPacket(destination, packet, writable, direct) {
        if (!this.#open) throw new Error('로컬 전송 연결을 열기 전 송신');
        if (packet.length >= HEADER && isRetiredCdValidationPacket(packet[1])) return false;
        const selected = direct ?? (destination > -PENDING_CONNECTION_SLOTS && destination < TRANSCEIVER_SLOTS ? this.connection(destination) : null);
        const client = selected === this.#client;
        if (client && (!this.#clientPresent || !(this.#client.flags & 2) || (this.#client.flags & CLOSED) || this.#clientPeerLost)) return false;
        if (!direct && destination >= 1 && destination < PLAYERS && !packetPlayerActive(this.#players, this.#hooks.players, destination)) return false;
        const server = this.#serverPresent && !this.#serverPeerLost && !(this.#server.flags & CLOSED) && selected === this.#server;
        if (!client && !server) return false;
        if (packet.length < HEADER) throw new RangeError('로컬 송신 헤더 부족');
        const registration = this.#dispatch.registration(packet[1]);
        // observer 뒤 호출자의 실제 패킷 저장소까지 송신 복귀 동안 등록 핀을 유지한다.
        const sessionPin = registration.pin();
        try {
            if (writable) registration.notifyObserver(writable, 1);
            if ((!direct && !this.connection(destination))
                || (client ? !this.#clientPresent || (this.#client.flags & CLOSED) || this.#clientPeerLost : !this.#serverPresent || (this.#server.flags & CLOSED) || this.#serverPeerLost)) return false;
            const length = read(packet, 2, 2);
            if (length < HEADER || length > packet.length) throw new RangeError('로컬 전송 패킷 실제 선언 바이트 부족');
            const output = client ? this.#clientOutput : this.#serverOutput;
            const bytes = packet.subarray(0, length);
            if (!client && !this.#serverBuffered) this.#toClient.push(Uint8Array.from(bytes));
            else {
                if (output.length + length > BUFFERED_LIMIT) throw new RangeError('로컬 buffered 전송 65535바이트 범위 초과');
                output.append(bytes);
            }
            if (this.#hooks.sent) this.#hooks.sent(client ? this.#client : this.#server, packet, read(packet, 2, 2));
            return true;
        } finally {
            sessionPin.release();
        }
    }

    // 원본 초기 0xff 헤더의 첫 BYTE 는 그대로 두고 kind 4/전체 WORD 만 확정해 buffered 데이터를 다른 끝의 수신 큐로 옮긴다. (2026-11-12 WB04-E324)
    #flushEndpoint(serverSide) {
        const output = serverSide ? this.#serverOutput : this.#clientOutput;
        if (!output.length) return;
        const frame = new Uint8Array(output.length + HEADER).fill(0xff);
        frame[1] = BATCH_TYPE;
        put(frame, 2, frame.length, 2);
        let cursor = HEADER;
        // 누적 순서대로 조각을 옮긴다.
        for (const chunk of output.chunks) {
            frame.set(chunk, cursor);
            cursor += chunk.length;
        }
        output.clear();
        (serverSide ? this.#toClient : this.#toServer).push(frame);
    }

    // 현재 선택 ID(0..21)만 집합으로 확정한다. pending 은 이 함수 대상이 아니며 닫힌 끝은 건너뛴다. (2026-11-12 WB04-E324)
    flushOne(id) {
        if (!Number.isInteger(id) || id < 0 || id >= TRANSCEIVER_SLOTS) throw new RangeError('내부 flush 의 정상 ID 범위');
        const node = this.connection(id);
        if (!node || (node.flags & CLOSED)) return;
        this.#flushEndpoint(node === this.#server);
    }

    // 004e2920/004e0150: 현재 0..21 순회로 per-ID 송신을 실행한다. (2026-11-12 WB04-E324)
    flush() {
        if (!this.#open) throw new Error('로컬 연결을 열기 전 flush');
        // 콜백 뒤 다음 칸은 현재 Registry 에서 다시 읽는다.
        for (let id = 0; id < TRANSCEIVER_SLOTS; id += 1) this.flushOne(id);
    }

    // 지연 없는 실제 수신 프레임의 단일/집합 분기이다. 단일은 같은 분배기에 맡기고, 집합(kind 4)은 원본 순서 now → 버전 정책 → 기록 → observer(0) → receive 로 내부 패킷마다 분배한다.
    // 반환은 연결이 계속 쓸 수 있으면 true 이다(닫힘/종료/소멸이면 false). 길이 오류는 terminate 뒤 false, 중첩 집합과 지연 표시 연결은 미복원이라 거부한다. (2026-11-12 WB04-E324)
    dispatchFrame(connection, frame) {
        if (frame.length >= HEADER && isRetiredCdValidationPacket(frame[1])) return !(connection.flags & CLOSED);
        if (frame.length < HEADER || frame[1] !== BATCH_TYPE) return this.#dispatch.dispatch(connection, frame);
        if (connection.flags & CLOSED) return false;
        if (connection.delayed) throw new Error('로컬 집합의 지연 분배 미복원');
        const hooks = this.#hooks;
        const total = read(frame, 2, 2);
        let cursor = HEADER;
        if (total > frame.length) {
            hooks.terminate(connection);
            return false;
        }
        // 현재 집합의 원문 순서를 보존하고 콜백 뒤 다음 등록/연결 필드를 다시 읽는다.
        while (cursor < total) {
            if (total - cursor < HEADER) {
                hooks.terminate(connection);
                return false;
            }
            const length = read(frame, cursor + 2, 2);
            if (!length || length > total - cursor) {
                hooks.terminate(connection);
                return false;
            }
            const packet = frame.subarray(cursor, cursor + length);
            cursor += length;
            if (!this.#dispatch.validate(packet)) continue;
            // 집합 내부에서도 폐기된 검증의 기록/observer/수신기를 호출하지 않는다.
            if (isRetiredCdValidationPacket(packet[1])) continue;
            if (packet[1] === BATCH_TYPE) throw new Error('로컬 중첩 집합 패킷 미복원');
            const type = packet[1];
            const now = hooks.now();
            if (type === 6) connection.heartbeat = now;
            else connection.received = now;
            const registration = this.#dispatch.registration(type);
            const incoming = packet[0] & 0x7f;
            if (incoming !== registration.version) {
                const lower = incoming < registration.version;
                if (registration.mismatch & (lower ? 8 : 4)) {
                    hooks.terminate(connection);
                    return false;
                }
                if (registration.mismatch & (lower ? 2 : 1)) continue;
            }
            // 집합의 기록도 실제 전체 Runtime 정리보다 먼저 대여를 올린다.
            const recordPin = registration.pin();
            try {
                hooks.record(connection, packet, length);
                this.#dispatch.registration(type).notifyObserver(packet, 0);
                const receiverEntry = this.#dispatch.registration(type);
                const pin = receiverEntry.pin();
                try {
                    const receive = receiverEntry.receive;
                    if (receive) {
                        connection.depth += 1;
                        try {
                            receive(connection.sender, connection, packet);
                        } finally {
                            connection.depth -= 1;
                        }
                    }
                } finally {
                    pin.release();
                }
            } finally {
                recordPin.release();
            }
        }
        if (cursor !== total) {
            hooks.terminate(connection);
            return false;
        }
        if (!connection.depth && (connection.flags & CLOSED)) {
            const wrapped = this.#wrappedHooks();
            wrapped.close(connection);
            wrapped.release(connection);
            return false;
        }
        return true;
    }

    // 지정 끝의 실제 한 프레임을 꺼내(콜백 동안 이 함수가 소유) 마지막 수신 version 을 기록하고, 단일 observer(0)(004dfc00, 집합과 폐기된 검증은 제외) 뒤 분배한다. (2026-11-12 WB04-E324)
    #receiveEndpoint(serverSide) {
        const queue = serverSide ? this.#toServer : this.#toClient;
        if (!queue.length) return transportPollResult();
        const bytes = queue.shift();
        if (bytes.length) this.#receiveVersion[serverSide ? 1 : 0] = bytes[0];
        if (!this.#dispatch.validate(bytes)) return transportPollResult(true, false);
        const entry = this.#dispatch.registration(bytes[1]);
        // observer 부터 뒤의 분배까지 실제 Runtime 대여를 끊지 않는다.
        const sessionPin = entry.pin();
        try {
            if (bytes[1] !== BATCH_TYPE && !isRetiredCdValidationPacket(bytes[1])) entry.notifyObserver(bytes, 0);
            return transportPollResult(true, this.dispatchFrame(serverSide ? this.#server : this.#client, bytes));
        } finally {
            sessionPin.release();
        }
    }

    // 실제 per-ID 수신의 중첩을 막고 이전 닫힘 끝을 반납한다(반납 실패면 폴링 대여를 되돌린다). (2026-11-12 WB04-E324)
    beginPoll() {
        if (this.#polling) throw new Error('내부 전송 Poll 중첩 호출');
        this.#polling = true;
        try {
            this.#retireClosed();
        } catch (error) {
            this.#polling = false;
            throw error;
        }
    }

    // 이 owner 의 현재 ID 한 프레임만 소비하고 결과를 돌려준다. 접속 완료 표시(flags 0x20)는 type 1 통지로, 반대 끝 상실은 한 번만 type 2 통지로 직접 분배한다(004dfa40/004df9f0, observer 전처리 없음). (2026-11-12 WB04-E324)
    receiveOne(id) {
        if (!this.#polling) throw new Error('내부 per-ID 수신의 Poll 대여 누락');
        const node = this.connection(id);
        if (!node) return transportPollResult();
        if (node.flags & CLOSED) {
            this.#retireClosed();
            return transportPollResult();
        }
        const side = node === this.#server ? 1 : 0;
        if (node.flags & 0x20) {
            node.flags &= ~0x20;
            this.dispatchFrame(node, Uint8Array.of(this.#receiveVersion[side], 1, 4, 0));
        }
        if (this.connection(id) !== node || (node.flags & CLOSED)) return transportPollResult();
        if (node === this.#client ? this.#clientPeerLost : this.#serverPeerLost) {
            if (!(node.flags & 0x10)) {
                node.flags |= 0x10;
                this.dispatchFrame(node, Uint8Array.of(this.#receiveVersion[side], 2, 4, 0));
            }
            return transportPollResult();
        }
        return this.#receiveEndpoint(node === this.#server);
    }

    // 정상/예외 복귀 모두 현재 per-ID 폴링 대여를 끝낸다(고정 내부 노드는 물리 삭제가 없다). (2026-11-12 WB04-E324)
    endPoll() {
        this.#polling = false;
    }

    // 004e28b0: 수락 → pending -1..-31 각 한 번 → 정상 0..21(같은 ID 를 again 동안 반복) 순서로 읽어 같은 등록표에 분배하고 소비한 프레임 수를 돌려준다. (2026-11-12 WB04-E324)
    poll() {
        this.beginPoll();
        let count = 0;
        try {
            this.acceptPending();
            // 각 현재 pending 은 한 번만 방문한다.
            for (let id = -1; id > -PENDING_CONNECTION_SLOTS; id -= 1) {
                if (this.receiveOne(id).consumed) count += 1;
            }
            // 재배치된 현재 ID 도 다시 조회하며 again 이면 같은 ID 를 반복한다.
            for (let id = 0; id < TRANSCEIVER_SLOTS; id += 1) {
                // 한 ID 의 프레임을 again 이 거짓이 될 때까지 읽는다.
                for (;;) {
                    const result = this.receiveOne(id);
                    if (result.consumed) count += 1;
                    if (!result.again) break;
                }
            }
        } finally {
            this.endPoll();
        }
        return count;
    }

    // 같은 현재 ID 가 자기 실제 토큰인지 확인해 끝을 돌려준다(다른 backend 의 노드는 null). ID 범위 밖은 오류이다. (2026-11-12 WB04-E324)
    connection(id) {
        const token = currentTransportToken(this.#registry, id);
        if (token && this.#clientPresent && token === this.#clientToken) return this.#client;
        if (token && this.#serverPresent && token === this.#serverToken) return this.#server;
        return null;
    }

    // 존재·반대 끝 상실·닫힘만 읽는다(buffered bit 2 는 연결 상태가 아님). (2026-11-12 WB04-E324)
    connectionOpen(id) {
        const node = this.connection(id);
        return Boolean(node) && !(node.flags & CLOSED) && (node === this.#client ? !this.#clientPeerLost : !this.#serverPeerLost);
    }

    // Connect/Accept 는 내부 큐의 즉시 완료이므로 열렸으면 1, 아니면 0 이다(진행 중 OS 상태를 합성하지 않음). (2026-11-12 WB04-E324)
    connectionStatus(id) {
        return this.connectionOpen(id) ? 1 : 0;
    }

    // 자기 두 끝만 실제 등록소 장치로 반납하고 내부 listener 를 닫는다(다른 TCP/Editor 의 공유 표나 새 세대를 지우지 않음). 수신 중(깊이>0)인 끝은 닫힘 표시만 하고 다음 폴링에서 반납한다. (2026-11-12 WB04-E324)
    closeAll() {
        this.#open = false;
        // 자기 두 고정 노드의 현재 생존 토큰만 반납한다.
        for (const node of [this.#client, this.#server]) {
            const token = node === this.#client ? this.#clientToken : this.#serverToken;
            if (!token || !this.#registry.owns(token)) continue;
            node.flags |= CLOSED;
            if (!node.depth) {
                this.#registry.destroy(token);
                this.#registry.release(token);
            }
        }
    }

    // 현재 번호 재배치와 별개인 고정 양끝의 실제 물리 주소인지 본다(기준선 뒤 ②: CompositeMissionTransport/Session 의 Source 선택). (2026-11-12 WB04-E324)
    ownsConnection(connection) {
        return connection === this.#client || connection === this.#server;
    }

    // 후속 실제 로컬 패킷(건설/준비/서버 응답 등)이 공유하는 이 한 분배기이다. (2026-11-12 WB04-E324)
    get dispatch() {
        return this.#dispatch;
    }

    // 같은 내부 listener·두 끝·버퍼·깊이·등록표를 유지하고 현재 World 분배 장치만 바꾼다(기준선 뒤 ②: CompositeMissionTransport 의 장치 인계). (2026-11-12 WB04-E324)
    rebindHooks(hooks) {
        checkHooks(hooks, '내부 전송 인계의 실제 분배 장치 누락');
        this.#hooks = hooks;
    }

    // 원본 서버 ID 9 방향에서 들어오는 현재 연결 객체(클라이언트 끝)이다. (2026-11-12 WB04-E324)
    get client() {
        return this.#client;
    }

    // 원본 pending -1/재배치 owner 의 같은 현재 연결 객체(서버 끝)이다. (2026-11-12 WB04-E324)
    get server() {
        return this.#server;
    }

    // RawBattleBroadcast 가 같은 연결 객체의 송신을 쓰도록 해당 owner 의 소비자를 만든다(호출 때 현재 Player flags 1 과 연결 위치를 send 에서 다시 확인). (2026-11-12 WB04-E324)
    peer(owner) {
        return (packet) => { this.send(owner, packet); };
    }

    // 실제 두 끝의 열린 수명만 보고한다(flags 2 나 Ready 를 대신하지 않음). (2026-11-12 WB04-E324)
    get isOpen() {
        return this.#open;
    }

    // 004e2b10(9) 의 실제 Client9 생성 완료만 조회한다(flags 2/종료 flag/서버 열림과 별개). (2026-11-12 WB04-E324)
    get hasClientConnection() {
        return this.#clientPresent;
    }

    // 서버 끝이 존재하고 그 sender 가 1..8 이며 현재 그 양수 칸의 토큰이 서버 끝이면 그 번호, 아니면 0 이다(별도 성공 owner 를 합성하지 않음, 기준선 뒤 ②). (2026-11-12 WB04-E324)
    owner() {
        const owner = this.#server.sender >>> 0;
        return this.#serverPresent && owner > 0 && owner < PLAYERS && currentTransportToken(this.#registry, owner) === this.#serverToken ? owner : 0;
    }

    // 상대가 실제로 받을 수 있게 flush 된 프레임 수이다. (2026-11-12 WB04-E324)
    get queuedFrames() {
        return this.#toServer.length + this.#toClient.length;
    }

    // 실제 열린 로컬 포트이다(열기 전 0). (2026-11-12 WB04-E324)
    get port() {
        return this.#port;
    }
}
