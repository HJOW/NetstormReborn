// 실제 Client 의 장기 전역과 현재 World 전체를 복합 전송의 수명 계약에 연결하고, Client 기본 패킷 등록 표(관찰기·버전·길이·빈 C3 수신기)를 장기 분배기에 채우는 모듈이다
// (cpppj `client/ClientCampaignTransport.cpp/.h`, 10.78 전용. 기준선 1f00a25a 은 60줄이었고 기준선 뒤 +324줄 중 단독 미션에 닿는 순수 부분만 옮겼다).
// 사용: const transport = makeClientCampaignTransport(client, CompositeTransportRole.InternalMission, world); const services = bindCompositeCampaignTransport(transport, sessionOwner);
//   World 생성 뒤 로그인 시작 전에 호출한다. 같은 전역/등록소를 빌리고 생성 자체는 연결을 열지 않는다. 기본 등록은 호출자의 `bindSession` 보다 먼저 채워 대여 반납 때 원문 기본 등록이 복구되게 한다.
// client 계약(실제 Client 는 4-D 에서 옮기므로 그 전까지는 이 최소 형태를 시험 장치가 만족한다):
//   edition('patch1078')·playerRecords(){ initialized(), records() Uint8Array(1620) }·gameplayGlobals(){ server() → boolean/{ value }/함수, gameAux(){ missionSettings() 7칸 } }·
//   gameClockOwner(){ wallSeconds(밀리초) }·clientMilliseconds()·connectionGlobals() ClientConnectionGlobals·packetRecordOwner(){ write(송신 여부, connection, packet, length) }·surfaceStorage(){ assertDisplay(식, 파일, 줄), gameAux(){ packet0cHeader() 2칸, packet0cInitialLength() } }·log?(문장).
// world 계약: session(){ playerRecords() → 같은 Player9 배열, recordConnectionPacket(connection, packet) }. World 는 약하게(WeakRef)만 참조해 지난 World 를 붙잡지 않는다.
// 기준선 뒤 cpppj 변경 판정(LEFT_JOBS 3.6절):
//   ② 반영 — 훅의 `players`(State 정리/재초기화 뒤에도 현재 Player base 를 읽음)·`sent`(송신 기록, 방향 1)·`record` 의 방향 0 기록(`af1fbf6c`), `now` 의 Client 시각 공급원(`ClientMilliseconds`), Client 기본 등록 18행 표와 mutable 관찰기 아홉 종
//   (0x0b·0x0c·0x55·0x56·0x57·0x16·0x1c·0x85·0x7b)·0x0b 버전 조회 수신기(4b3af0)·0x32 현재 설정 수신기(44dc20)·미션 송신 관찰기 4종(2a/2b/67/81)·0x3f/0x5f/0x7d 길이 관찰기(`1916bd66`·`ede61f05`·`5318a9b2`·`a76499e3` 등).
//   ① — `shared_ptr` 수명 보유(TransportGlobals 의 세 소유권)와 `PacketSources().Publish`(Source 경로 공개)는 GC 가 대신함.
//   웹 제외 — 훅 `delay`(`PacketDelays`, 실소켓 지연 FIFO)·`MissionNetwork`/Meta 인자(`network`·`meta`)·`RebindWorld` 재진입·Meta 계열 수신기(0x0f 루트 응답·0x1d 도전 로그인·0x26 지오메트리·0x66 서버 탐색·0x71 전투 설명·0x7c 게임 로그인·0x7b 주소 수신기·0x1a/0x1b 전투 시작).
//   ③ 4-D / 후속 — 고정 Native 수신기를 장기 기본 표에 설치하는 묶음(`InstallNative{Login,Gameplay,Carrier,Construction}PacketReceivers` 14·26·15·12F, 채팅 41h, 연결 종료 2번 004b6660, 기술 목록 0x69, 0x30 경로 자료 관찰기(RawPathData 6-P), 0x7a 압축 Squid 관찰기)는
//   대응표 밖 신규 파일이거나 아직 옮기지 않은 모듈이 필요해 이 단위에서 설치하지 않는다(PORT_MAP 메모의 "남음"). 원문에서 고정 Native 수신기로 등록하는 빈 C3 수신기(004437f0)는 웹에 canonical 등록이 없어 일반 직접 수신기(`registerDirect`)로 등록한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E328: 4-C ClientCampaignTransport 1단계 이식)
import { CompositeMissionTransport } from './composite-mission-transport.js';
import { observeMissionClientPacket } from './raw-mission-client-packets.js';
import { receiveCurrentMissionSettingsPacket } from './raw-mission-settings-packet.js';
import { finalizeConstructionPacket } from './raw-construction-packet.js';
import { observeBombEffectPacket } from './raw-bomb-effect.js';
import { finalizeBombTimerPacket } from './raw-bomb-timer-packet.js';

// Client 원문 기본 등록 한 행의 필드 설명: type(0040b4b0 의 패킷 종류)·length(0052ec40 의 고정 길이 WORD, 0xffff 는 가변)·c3Receiver(4437f0 의 비NULL 빈 수신기만 설치하고 관찰 전용 receiver 에는 손대지 않음)·
// mutableObserver(실제 this 버퍼를 읽는 일곱 원문 관찰기와 비NULL 빈 40b380 을 구별). (2026-11-12 WB04-E328)
// 이 열여덟 종만 version 1/policy 0 과 보호 PE 길이에 맞추며 다른 등록에는 손대지 않는다(원문 C3 7종과 폐기 80h 를 뺀 관찰 전용 11종의 정확한 목록).
export const CLIENT_PACKET_DEFAULTS = Object.freeze([
    { type: 0x0c, length: 0xffff, c3Receiver: true, mutableObserver: true },
    { type: 0x42, length: 12, c3Receiver: true, mutableObserver: false },
    { type: 0x4d, length: 10, c3Receiver: true, mutableObserver: false },
    { type: 0x55, length: 0xffff, c3Receiver: true, mutableObserver: true },
    { type: 0x56, length: 0xffff, c3Receiver: true, mutableObserver: true },
    { type: 0x57, length: 0xffff, c3Receiver: true, mutableObserver: true },
    { type: 0x5a, length: 5, c3Receiver: true, mutableObserver: false },
    { type: 0x10, length: 8, c3Receiver: false, mutableObserver: false },
    { type: 0x11, length: 12, c3Receiver: false, mutableObserver: false },
    { type: 0x12, length: 12, c3Receiver: false, mutableObserver: false },
    { type: 0x16, length: 0xffff, c3Receiver: false, mutableObserver: true },
    { type: 0x17, length: 14, c3Receiver: false, mutableObserver: false },
    { type: 0x1c, length: 0xffff, c3Receiver: false, mutableObserver: true },
    { type: 0x1e, length: 14, c3Receiver: false, mutableObserver: false },
    { type: 0x1f, length: 12, c3Receiver: false, mutableObserver: false },
    { type: 0x68, length: 5, c3Receiver: false, mutableObserver: false },
    { type: 0x85, length: 0xffff, c3Receiver: false, mutableObserver: true },
    { type: 0x87, length: 274, c3Receiver: false, mutableObserver: false },
].map((entry) => Object.freeze(entry)));

// 원문 패킷 파일 이름과 줄 번호 진단에 쓰는 PE 문자열이다. (2026-11-12 WB04-E328)
const PACKETS_FILE = '\\Ns\\O\\_p_packets.cpp';

// 원문 비정렬 필드의 지정된 현재 BYTE/WORD/DWORD 를 리틀엔디언으로 읽는다. 물리 버퍼 밖이면 거부하고 지정 폭 바깥의 미작성 tail 은 읽지 않는다. (2026-11-12 WB04-E328)
export function readClientPacketDefault(packet, offset, width) {
    if (offset > packet.length || width > packet.length - offset) throw new RangeError('Client 기본 관찰기의 실제 필드 물리 부족');
    let value = 0;
    // 낮은 BYTE 부터 같은 현재 버퍼를 조립한다.
    for (let byte = 0; byte < width; byte += 1) value += packet[offset + byte] * 2 ** (8 * byte);
    return value;
}

// 원문의 선언 WORD 만 갱신한다(16비트로 감기며 그 밖의 원본 버퍼는 보존). 4BYTE 헤더가 없으면 거부한다. (2026-11-12 WB04-E328)
export function writeClientPacketDefaultLength(packet, length) {
    if (packet.length < 4) throw new RangeError('Client 기본 관찰기의 선언WORD 물리 부족');
    packet[2] = length & 0xff;
    packet[3] = (length >>> 8) & 0xff;
}

// 원문 409fe0/40a020/40aae0/40ab20/40ab60/40a190/40a270/40aed0 및 7b 의 송신 효과만 실행한다: 수신 방향(sending=0)은 필드를 읽기 전에 복귀하고, 송신은 -1 센티넬이면 진단을 부른 뒤
// (진단이 복귀하면) 현재 필드를 다시 읽어 선언 길이를 정한다. 미등록 종류는 거부한다. assertion(식, 파일, 줄). (2026-11-12 WB04-E328)
export function observeClientPacketDefault(type, packet, sending, assertion) {
    if (!sending) return;
    switch (type) {
    case 0x0b:
    case 0x0c:
        if (readClientPacketDefault(packet, 8, 4) === 0xffffffff) assertion('extraDataLen != (int)-1', PACKETS_FILE, type === 0x0b ? 0x36c : 0x38c);
        writeClientPacketDefaultLength(packet, readClientPacketDefault(packet, 8, 2) + 12);
        return;
    case 0x55:
        if (readClientPacketDefault(packet, 4, 4) === 0xffffffff) assertion('count != (int)-1', PACKETS_FILE, 0x8dc);
        writeClientPacketDefaultLength(packet, readClientPacketDefault(packet, 4, 2) + 8);
        return;
    case 0x56:
        if (readClientPacketDefault(packet, 8, 4) === 0xffffffff) assertion('dataLength != (int)-1', PACKETS_FILE, 0x8f8);
        writeClientPacketDefaultLength(packet, readClientPacketDefault(packet, 8, 2) + 12);
        return;
    case 0x57:
        if (readClientPacketDefault(packet, 8, 4) === 0xffffffff) assertion('specLength != (int)-1', PACKETS_FILE, 0x916);
        writeClientPacketDefaultLength(packet, readClientPacketDefault(packet, 8, 2) + 12);
        return;
    case 0x16: {
        if (readClientPacketDefault(packet, 5, 1) === 0xff) assertion('numFriends != (char)-1', PACKETS_FILE, 0x458);
        // 진단 복귀 뒤 현재 signed BYTE 를 다시 읽는다(MOVSX AX).
        const count = (readClientPacketDefault(packet, 5, 1) << 24) >> 24;
        writeClientPacketDefaultLength(packet, count * 4 + 14);
        return;
    }
    case 0x1c:
        if (readClientPacketDefault(packet, 45, 1) === 0xff) assertion('nameLen != (unsigned char)-1', PACKETS_FILE, 0x4ca);
        writeClientPacketDefaultLength(packet, readClientPacketDefault(packet, 45, 1) + 46);
        return;
    case 0x85:
        if (readClientPacketDefault(packet, 5, 2) === 0xffff) assertion('dataLen != (unsigned short)-1', PACKETS_FILE, 0xa2c);
        writeClientPacketDefaultLength(packet, readClientPacketDefault(packet, 5, 2) + 7);
        return;
    case 0x7b: {
        if (readClientPacketDefault(packet, 9, 1) === 0xff) assertion('addrLen != (char)-1', PACKETS_FILE, 0xba8);
        // 진단 정상 복귀 뒤 현재 BYTE 를 MOVSX AX 처럼 다시 읽는다.
        const length = (readClientPacketDefault(packet, 9, 1) << 24) >> 24;
        writeClientPacketDefaultLength(packet, length + 10);
        return;
    }
    default:
        throw new TypeError('Client 기본 표의 미등록 mutable 관찰기 종류');
    }
}

// 같은 Client storage 의 실제 진단만 빌려 2a/2b/67/81 송신 관찰기를 Session 대여 밖의 기본 표에 설치한다(수신기와 version/길이 표는 바꾸지 않음). 진단이 복귀하면 같은 현재 버퍼를 다시 읽는다. (2026-11-12 WB04-E328)
export function installClientMissionSendObservers(client, dispatch) {
    const storage = client.surfaceStorage();
    if (!storage) throw new TypeError('기본 미션 송신 관찰기의 실제 Client storage 없음');
    // 원문 40a540/40a590/40ae80/40b460 의 네 mutable 관찰기이다.
    for (const type of [0x2a, 0x2b, 0x67, 0x81]) {
        dispatch.observePacket(type, (packet, sending) => {
            observeMissionClientPacket(type, packet, sending, (expression, file, line) => { storage.assertDisplay(expression, file, line); });
        });
    }
}

// 0x0b 버전 조회 수신기(4b3af0): 현재 GameAux 의 BYTE 두 개/WORD 하나로 50BYTE 예약 전체를 0xff 로 채운 응답(버전 10.78)을 만들어 현재 sender 의 실제 가상 송신에서 0x0c 관찰기가 한 번 호출되게 보낸다. 수신 payload/연결/phase/known 은 읽지 않는다. (2026-11-12 WB04-E328)
function versionQueryReceiver(client) {
    const aux = client.surfaceStorage().gameAux();
    const registry = client.connectionGlobals();
    const players = client.playerRecords();
    return (sender) => {
        const message = aux.packet0cHeader()[0];
        const protocol = aux.packet0cHeader()[1];
        const initial = aux.packet0cInitialLength();
        // df590 의 실제 50BYTE 예약 전체를 FF 로 채우며 extraDataLen DWORD 도 -1 로 남긴다.
        const packet = new Uint8Array(50).fill(0xff);
        packet[1] = message & 0xff;
        packet[0] = protocol & 0x7f;
        // 생성자는 선언 길이를 최소 4 로 올리고 실제 wire 길이는 현재 송신 관찰기가 나중에 정한다.
        writeClientPacketDefaultLength(packet, initial < 4 ? 4 : initial);
        // 원문 4b3b20/26 은 WORD 10/78 만 쓰며 제품 표시 11.01 과 wire 10.78 을 구별한다.
        packet[4] = 10;
        packet[5] = 0;
        packet[6] = 78;
        packet[7] = 0;
        // 현재 sender/Player/NULL 관문 뒤 선택된 실제 가상 송신에서만 0c 관찰기를 한 번 부른다.
        registry.sendMutable(sender | 0, packet, players.records());
    };
}

// Client 기본 패킷 등록 표를 장기 분배기에 채운다(단독 미션에 닿는 순수 부분). 호출자의 `bindSession` 보다 먼저 호출해 대여 반납 때 원문 기본 등록이 복구되게 한다.
// 순서: 미션 송신 관찰기 4종 → 0x32 현재 설정 → 0x7b 관찰기 → 0x3f/0x5f/0x7d 길이 관찰기 → 열여덟 행 표(관찰기·version 1/policy 0·길이·빈 C3 수신기) → 0x0b 관찰기·버전 조회 수신기.
// 아직 옮기지 않은 묶음은 모듈 머리글의 ③ 목록과 같다. (2026-11-12 WB04-E328)
export function installClientPacketDefaults(client, dispatch) {
    installClientMissionSendObservers(client, dispatch);
    const storage = client.surfaceStorage();
    const aux = client.gameplayGlobals().gameAux();
    // 0x32: 비NULL 빈 관찰기와 version 1/policy 0·고정 32BYTE, 장기 7DWORD 만 쓰는 수신기(송신자/연결/Session/phase 를 조회하지 않음).
    dispatch.observe(0x32, () => {});
    dispatch.version(0x32, 1, 0);
    dispatch.requireLength(0x32, 32);
    dispatch.registerDirect(0x32, (sender, connection, packet) => { receiveCurrentMissionSettingsPacket(aux.missionSettings(), packet); });
    // 0x7b: 40b2f0 은 수신 방향에서 payload 를 읽지 않고 송신 진단 뒤 signed BYTE 길이를 다시 읽는다. 수신기(CS 주소 수신)는 Meta 계열이라 웹 제외이다.
    dispatch.observePacket(0x7b, (packet, sending) => { observeClientPacketDefault(0x7b, packet, sending, (expression, file, line) => { storage.assertDisplay(expression, file, line); }); });
    dispatch.version(0x7b, 1, 0);
    dispatch.requireLength(0x7b, 0xffff);
    // 0x3f/0x5f/0x7d: 수신기는 Session 대여에 맡기고 원문 기본 metadata/관찰기는 반납 뒤에도 남긴다.
    dispatch.observePacket(0x3f, (packet, sending) => {
        finalizeConstructionPacket(packet, Boolean(sending), () => { storage.assertDisplay('sidCount != (unsigned char)-1', PACKETS_FILE, 0x7c9); });
    });
    dispatch.version(0x3f, 1, 0);
    dispatch.requireLength(0x3f, 0xffff);
    dispatch.observePacket(0x5f, (packet, sending) => {
        observeBombEffectPacket(packet, sending, () => { storage.assertDisplay('count != (int)-1', PACKETS_FILE, 0x96b); });
    });
    dispatch.version(0x5f, 1, 0);
    dispatch.requireLength(0x5f, 0xffff);
    dispatch.observePacket(0x7d, (packet, sending) => { finalizeBombTimerPacket(packet, Boolean(sending)); });
    dispatch.version(0x7d, 1, 0);
    dispatch.requireLength(0x7d, 0xffff);
    // 이 열여덟 종만 version 1/policy 0·보호 PE 길이에 맞춘다.
    for (const entry of CLIENT_PACKET_DEFAULTS) {
        if (entry.mutableObserver) {
            dispatch.observePacket(entry.type, (packet, sending) => { observeClientPacketDefault(entry.type, packet, sending, (expression, file, line) => { storage.assertDisplay(expression, file, line); }); });
        } else {
            // 40b380 의 비NULL ret4 이며 NULL 관찰기로 바꾸지 않는다.
            dispatch.observe(entry.type, () => {});
        }
        dispatch.version(entry.type, 1, 0);
        dispatch.requireLength(entry.type, entry.length);
        // 4437f0 의 실제 ret 이며 ACK/반환 성공값/게임 State 를 합성하지 않는다(웹은 canonical 등록이 없어 일반 직접 수신기로 등록).
        if (entry.c3Receiver) dispatch.registerDirect(entry.type, () => {});
    }
    // 0x0b: 409fe0 은 수신 필드를 읽지 않고 송신 때만 현재 extraDataLen/선언 WORD 를 처리한다. Client 40b4b0 의 실제 버전 조회 등록이다.
    dispatch.observePacket(0x0b, (packet, sending) => { observeClientPacketDefault(0x0b, packet, sending, (expression, file, line) => { storage.assertDisplay(expression, file, line); }); });
    dispatch.version(0x0b, 1, 0);
    dispatch.requireLength(0x0b, 0xffff);
    dispatch.registerDirect(0x0b, versionQueryReceiver(client));
}

// 같은 Client/Session 의 물리를 검증하고 연결을 열지 않는 실제 복합 조정자를 만든 뒤 Client 기본 패킷 등록을 채운다. 검증: 단독 판본 10.78·초기화된 Player 기록·World 세션의 Player 배열이 Client 의 같은 배열인지. (2026-11-12 WB04-E328)
export function makeClientCampaignTransport(client, role, world) {
    if (!world || client.edition !== 'patch1078' || !client.playerRecords().initialized() || world.session().playerRecords() !== client.playerRecords().records()) {
        throw new TypeError('현재 Client/World/Player 물리의 복합 전송 수명 계약 오류');
    }
    const players = client.playerRecords();
    const game = client.gameplayGlobals();
    const clock = client.gameClockOwner();
    const record = client.packetRecordOwner();
    const log = client.log ?? ((text) => { console.debug(text); });
    // 현재 World 는 약하게만 참조해 지난 World 를 붙잡지 않는다.
    const runtime = new WeakRef(world);
    const hooks = {
        // State 정리/재초기화 뒤에도 원문 송신은 현재 Player base 를 읽는다.
        players: () => players.records(),
        // 기존 Login 과 같은 현재 OS 밀리초/원점 계산이며 프레임 시각 사본을 읽지 않는다.
        now: () => clock.wallSeconds(client.clientMilliseconds()) >>> 0,
        // 실제 전송 본체의 복사/쓰기 뒤에만 방향 1(송신)을 기록한다.
        sent: (connection, packet, length) => { record.write(true, connection, packet, length); },
        // 원문 방향 0(수신) 기록은 수신기/기존 읽기 관측보다 먼저 실행하며 현재 Login 존재 여부와 별개이다.
        record: (connection, packet, length) => {
            record.write(false, connection, packet, length);
            const current = runtime.deref();
            if (current) current.session().recordConnectionPacket(connection, packet);
        },
        // 원문 분배기의 종료 표시만 쓰고 물리 반납은 실제 하위 전송에 맡긴다.
        terminate: (connection) => { connection.flags |= 0x40; log('로컬 미션 연결 종료'); },
        // 실제 노드 owner 가 close/free 를 수행하는 같은 위치의 진단이다.
        close: () => { log('로컬 연결 큐/소켓 종료'); },
        release: () => { log('로컬 연결 반납'); },
    };
    const transport = CompositeMissionTransport.create(role, players.records(), game.server(), hooks, client.connectionGlobals(), { players, game, clock });
    // 호출자의 bindSession 보다 먼저 같은 기본 표를 채워 임대 반납 때 원문 C3/관찰기/metadata 가 복구되게 한다.
    installClientPacketDefaults(client, transport.dispatch);
    return transport;
}
