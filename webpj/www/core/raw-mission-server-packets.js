// 로그인 서버의 현재 Player/중앙섬 metadata 작성과 원본 송신 순서를 옮긴 모듈이다(cpppj `o/RawMissionServerPackets.cpp/.h`, 10.78: 0048f9f0 Player 송신(0x2b), 004a6b40 metadata 송신(0x72), 미션 Zone 의 004b3bc0 수신 자연 경로, 40b110 observer).
// 사용: const packets = new RawMissionServerPackets(players, state, hooks); packets.sendPlayer(owner, target); packets.sendMetadata(target); registerMissionMetadata(dispatch, packets);
//   players = Player9 기록 9×180BYTE(이름 정제도 여기에 직접 반영), state = 원본 현재 전역 묶음(속성으로 읽는 시점마다 현재값 조회): server(00540bc0, 진단 뒤에도 작성은 계속)·metamap(00594fc8, Player 번호를 읽을 때마다 진단)·localOwner(00540c70)·
//   centralCreated(005c85a0)·archipelago(005c8bb4, packet 에는 낮은 BYTE 만)·zoneAddress(005c86c0 의 6BYTE, 앞 DWORD 의 bit 8 = 0x100 을 읽음),
//   hooks = 큰 전송/주소 포맷과 중앙섬 생성만 실제 외부 하위: assertion(text)·peerAddress(owner)·send(target, bytes)(전체 mutable 예약을 연결 gate 에 넘기고 장치가 observer 뒤 선언 길이를 확정)·log(text)·creatingIslands()·
//   setArchipelago(v)·setCentralCreated(v)·layout()·randomCentral(zone6)·surroundedCentral()·predictableCount(). 선택한 장치는 도달한 위치에서만 요구하고 없으면 앞선 부분 효과를 남긴 채 거부한다.
// 이름 정제와 Player(0x2b) 송신 observer 는 `raw-mission-client-packets.js` 의 같은 몸체를 쓴다(서버 시험이 두 모듈을 함께 건다).
// 기준선 뒤 cpppj 변경(9cb0547f5·ede61f05d·eeae45c69, 3.6절 ②): 중앙섬/군도 분리와 metadata 상태, 로그인 writer 의 전체 mutable 송신, 0x72 observer 본체(40b110)를 반영했다. 고정 Native 수신기 대여 분기는 E306 의 ③ 과 같은 묶음이라 옮기지 않았다.
// CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E315: 4-C RawMissionServerPackets 이식)
import { sanitizeMissionPlayerName } from './raw-mission-client-packets.js';

// 실제 Player 폭/개수와 Player 패킷 송신 스택 예약 폭(100BYTE)이다. (2026-11-11 WB04-E315)
const STRIDE = 180;
const COUNT = 9;
const PLAYER_PACKET_RESERVE = 100;

// 선택한 실제 경계가 없으면 이전 부분 효과를 보존한 채 오류를 낸다. (2026-11-11 WB04-E315)
function need(device) {
    if (typeof device !== 'function') throw new Error('미션 서버 패킷의 실제 하위 미연결');
    return device;
}

// 물리 원문의 지정 폭을 낮은 바이트부터 읽는다(범위 밖은 오류). (2026-11-11 WB04-E315)
function get(bytes, offset, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset + width > bytes.length) throw new RangeError('미션 서버 패킷 읽기 물리 범위');
    let value = 0;
    // 낮은 바이트부터 little endian 비트를 결합한다.
    for (let index = 0; index < width; index += 1) value |= bytes[offset + index] << (index * 8);
    return value >>> 0;
}

// 주변 stale 바이트를 보존하며 지정 BYTE/WORD/DWORD 만 쓴다(범위 밖은 오류). (2026-11-11 WB04-E315)
function put(bytes, offset, value, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset + width > bytes.length) throw new RangeError('미션 서버 패킷 쓰기 물리 범위');
    // 원본 지정 폭의 낮은 바이트만 변경한다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}

// NUL 까지의 문자열(바이트열)을 복사하되 호스트 저장소 밖에 쓰는 잘못된 이름은 거부한다: 끝에 NUL 을 쓴다. (2026-11-11 WB04-E315)
function copyString(bytes, offset, value) {
    const end = value.indexOf(0);
    const text = end < 0 ? value : value.subarray(0, end);
    if (offset >= bytes.length || text.length >= bytes.length - offset) throw new RangeError('미션 서버 이름 복사 물리 범위');
    bytes.set(text, offset);
    bytes[offset + text.length] = 0;
}

// 문자열을 바이트열로 바꾼다(문자 코드 하위 바이트). (2026-11-11 WB04-E315)
function bytesOf(text) {
    return Uint8Array.from(text, (char) => char.charCodeAt(0) & 0xff);
}

// 004df590 의 전체 0xff 예약과 실제 version/type/초기 헤더 길이를 가진 패킷을 만든다. (2026-11-11 WB04-E315)
function makePacket(type, size, length) {
    const packet = new Uint8Array(size).fill(0xff);
    packet[0] = 1;
    packet[1] = type;
    put(packet, 2, length, 2);
    return packet;
}

// 0x72 observer(40b110): 송신(sending ≠ 0)에서만 실제 패킷의 길이 DWORD 가 -1 이면 진단하고(복귀하면 계속) 선언 WORD 에 낮은 WORD + 10 을 쓴다. 수신 방향은 무동작이다. (2026-11-11 WB04-E315)
export function observeMissionMetadataPacket(packet, sending, assertion) {
    if (!sending) return;
    if (get(packet, 6) === 0xffffffff) need(assertion)('len != (int)-1');
    put(packet, 2, get(packet, 6, 2) + 10, 2);
}

// PATCH 0048f9f0/004a6b40 와 미션 Zone 의 004b3bc0 자연 경로를 같은 Player 에 연결하는 서버 패킷 몸체이다. (2026-11-11 WB04-E315)
export class RawMissionServerPackets {
    // 사용: 같은 Player9 와 전역은 처리기보다 오래 살아야 한다. 보호 원본 이름을 직접 수정하는 API 가 아니다. (2026-11-11 WB04-E315)
    constructor(players, state, hooks = {}) {
        if (!(players instanceof Uint8Array) || players.length !== STRIDE * COUNT || state?.zoneAddress?.length !== 6) throw new RangeError('미션 서버의 같은 Player9/Zone6BYTE 누락');
        this.players = players;
        this.state = state;
        this.hooks = hooks;
    }

    // 0048f550 의 현재 metamap 진단은 번호를 조회할 때마다 다시 실행한다(진단이 복귀하면 계속). 범위 밖 번호는 거부한다. (2026-11-11 WB04-E315)
    owner(owner) {
        if (this.state.metamap) need(this.hooks.assertion)('!inMetamapMode');
        if (!Number.isInteger(owner) || owner < 0 || owner >= COUNT) throw new RangeError('미션 서버 현재 Player 번호 범위');
        return owner & 0xff;
    }

    // 현재 Player 저장소를 복제하지 않고 필요한 폭만 읽는다. (2026-11-11 WB04-E315)
    read(owner, offset, width = 4) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= COUNT || offset + width > STRIDE) throw new RangeError('미션 서버 현재 Player 필드 범위');
        return get(this.players, owner * STRIDE + offset, width);
    }

    // 현재 Player 이름을 실제 원본 정제 뒤 byte 단위 packet(0x2b)에 쓰고 해당 target 으로 보낸다. 이름 정제/현재 주소 하위가 바꾼 flags/동맹/필드는 후속 원본 위치에서 다시 읽는다. (2026-11-11 WB04-E315)
    sendPlayer(owner, target) {
        const hooks = this.hooks;
        if (!this.state.server) need(hooks.assertion)('server');
        const packet = makePacket(0x2b, PLAYER_PACKET_RESERVE, 4);
        packet[0x32] = this.owner(owner);
        const name = this.players.subarray(owner * STRIDE + 12);
        sanitizeMissionPlayerName(this.read(owner, 0x2c), name);
        const end = name.indexOf(0);
        if (end < 0) throw new RangeError('미션 서버 현재 Player 이름 NUL 누락');
        packet[0x33] = (end + 1) & 0xff;
        copyString(packet, 0x3b, name.subarray(0, end));
        put(packet, 0x26, this.read(owner, 0x2c));
        if (!(this.read(owner, 0x50, 1) & 1)) packet[4] = 0;
        else {
            const address = need(hooks.peerAddress)(this.owner(owner));
            copyString(packet, 4, bytesOf(address));
        }
        put(packet, 0x2a, this.read(owner, 0x50));
        put(packet, 0x22, this.read(owner, 0x54));
        packet[0x34] = this.read(owner, 0x5c, 1);
        put(packet, 0x35, this.read(owner, 0x60, 2), 2);
        packet[0x37] = this.read(owner, 0xa0, 1);
        packet[0x38] = this.read(owner, 0xa4, 1);
        packet[0x39] = this.read(owner, 0xa5, 1);
        packet[0x3a] = this.read(owner, 0x71, 1);
        // 현재 연결/Player 관문 뒤 40a590 이 같은 100BYTE 예약의 이름 길이를 읽고 선언 WORD 를 확정한다.
        need(hooks.send)(target | 0, packet);
    }

    // Zone 관문 뒤 실제 안내/배치/밀도/현재 생성 전역과 원문 두 중앙섬 경로를 실행한다: bit 0x100 이 없으면 로그만 남기고 반환하고, 이름이 `$` 로 시작하면 현재 배치별 중앙섬을 만든다(특수 배치 2 는 centralCreated/예측수 로그 없이 반환). (2026-11-11 WB04-E315)
    receiveMetadata(packet) {
        const state = this.state;
        const hooks = this.hooks;
        if (!(get(state.zoneAddress, 0) & 0x100)) {
            need(hooks.log)('No Central Island.\n');
            return;
        }
        if (packet.length <= 10) throw new RangeError('중앙섬 metadata 원문 부족');
        if (packet[10] !== 0x24) return;
        need(hooks.creatingIslands)();
        need(hooks.log)('createArchipelago... ');
        need(hooks.setArchipelago)(get(packet, 5, 1));
        // 원문 특수 배치는 centralCreated/예측수 로그 없이 반환한다.
        if (need(hooks.layout)() === 2) {
            need(hooks.surroundedCentral)();
            return;
        }
        need(hooks.randomCentral)(Uint8Array.from(state.zoneAddress.subarray(0, 6)));
        need(hooks.setCentralCreated)(1);
        need(hooks.log)(`predictableCount = ${need(hooks.predictableCount)() | 0}\n`);
    }

    // 같은 순수 observer 를 쓰되 기존 호출자의 진단 장치 수명을 유지한다. (2026-11-11 WB04-E315)
    observeMetadata(packet, sending) {
        observeMissionMetadataPacket(packet, sending, this.hooks.assertion);
    }

    // 실제 `$Random` 작성 → 현재 자기 처리(중앙섬이 아직 없으면 같은 수신 몸체를 먼저 실행) → 현재 로컬 번호 비교 → 다른 대상 송신을 유지한다. (2026-11-11 WB04-E315)
    sendMetadata(target) {
        const state = this.state;
        const hooks = this.hooks;
        const name = bytesOf('$Random');
        const packet = makePacket(0x72, name.length + 11, 4);
        packet[4] = 0;
        put(packet, 6, name.length + 1);
        copyString(packet, 10, name);
        packet[5] = state.archipelago & 0xff;
        if (!state.centralCreated) {
            if (!state.server) need(hooks.assertion)('server');
            this.receiveMetadata(packet);
        }
        // 자기 처리/현재 번호 비교 뒤 같은 전체 예약을 송신하고 40b110 의 observer 에 길이 확정을 맡긴다.
        if ((target >>> 0) !== (state.localOwner >>> 0)) need(hooks.send)(target | 0, packet);
    }
}

// 원본이 실제 등록한 0x72 의 observer(40b110)·버전 1/정책 0·현재 수신기를 같은 분배기에 등록한다. 길이는 현재 +6 WORD + 10 이다. (2026-11-11 WB04-E315)
export function registerMissionMetadata(dispatch, packets) {
    dispatch.version(0x72, 1, 0);
    dispatch.observePacket(0x72, (packet, sending) => { packets.observeMetadata(packet, sending); });
    dispatch.register(0x72, (sender, connection, bytes) => { packets.receiveMetadata(bytes); });
}
