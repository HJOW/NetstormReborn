// 원본 Ztrans 의 현재 Player/연결 표를 읽는 두 송신 분배 몸체와 훨리기그 명령(packet 0x86)·섬 정리(packet 0x35) 패킷 작성기를 옮긴 모듈이다(cpppj `o/RawBattleBroadcast.cpp/.h`, 10.78: 004e2e30 전체 송신, 004e2e80 제외 송신).
// 사용: const broadcast = new RawBattleBroadcast(players, peers, localOwner, server[, assertion]); broadcast.send(packet); broadcast.sendExcept(excluded, packet);
//   players = Player9 기록 9×180BYTE(`Uint8Array`, 접속 BYTE 는 각 +0x50 bit 0), peers = 길이 9 의 연결 송신 장치 표(가상+4, NULL 연결은 null/undefined), localOwner·server = 현재값을 매번 다시 읽는 `{ value }` 상자.
//   원본 송신은 접속 플래그만으로 보내지 않고 연결 객체의 존재도 확인한다. 0 번 Player 는 순회하지 않는다(1..8).
// 기준선 뒤 cpppj 변경(77a0c3a47 BindMutableSend/SendMutable, 167e7b79c 제외 송신의 실제 server 진단 복귀)은 3.6절 ② 로 반영했다. CD(10.72)는 옮기지 않았다(Player 폭 0xac 판본 없음).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E311: 4-C RawBattleBroadcast 이식)

// 10.78 Player 기록 폭(180BYTE)과 접속 BYTE 위치(+0x50), Player 수(0..8)이다. (2026-11-11 WB04-E311)
const PLAYER_STRIDE = 0xb4;
const CONNECTED_OFFSET = 0x50;
const PLAYER_COUNT = 9;

// 두 패킷 모두 20BYTE 이다(보호 PE 0052edf4/0052ede0 의 WORD 길이). (2026-11-11 WB04-E311)
const PACKET_LENGTH = 20;

// little endian DWORD 를 해당 위치의 네 바이트에만 쓴다(낮은 바이트부터). (2026-11-11 WB04-E311)
function putDword(packet, at, value) {
    // 정확히 해당 DWORD 의 네 바이트만 변경한다.
    for (let index = 0; index < 4; index += 1) packet[at + index] = (value >>> (8 * index)) & 0xff;
}

// 실제 40a890/45c94d/45cc4e 의 헤더·SID·명령 DWORD 만 쓴다: version 1, message 0x86, 길이 20, +4 부모 SID, +12 명령. 호출자가 쓰지 않는 +8/+16 은 입력 바이트를 유지한다. 20BYTE 미만이면 쓰기 전에 거부한다. (2026-11-11 WB04-E311)
export function writeFlyerCommandPacket(packet, parent, command) {
    if (packet.length < PACKET_LENGTH) throw new RangeError('훨리기그 방송 패킷20BYTE 필요');
    packet[0] = 1;
    packet[1] = 0x86;
    packet[2] = PACKET_LENGTH;
    packet[3] = 0;
    putDword(packet, 4, parent >>> 0);
    putDword(packet, 12, command >>> 0);
}

// 실제 46e6e7/40a740 의 20BYTE packet 0x35 를 쓴다: version 1, message 0x35, 길이 20, +4 x, +8 y, +12 flags, +16 owner. 좌표는 기존 섬 정리가 만든 16.16 DWORD 비트이며 다시 실수로 바꾸지 않는다. 20BYTE 바깥의 기존 저장소는 유지한다. (2026-11-11 WB04-E311)
export function writeDropIslandPacket(packet, x, y, flags, owner) {
    if (packet.length < PACKET_LENGTH) throw new RangeError('섬 정리 방송 패킷20BYTE 필요');
    packet[0] = 1;
    packet[1] = 0x35;
    packet[2] = PACKET_LENGTH;
    packet[3] = 0;
    // 원본 호출자의 +4/+8/+12/+16 순서이다.
    const payload = [x, y, flags, owner];
    // 각 원문 DWORD 를 차례로 쓴다.
    for (let field = 0; field < payload.length; field += 1) putDword(packet, 4 + field * 4, payload[field] >>> 0);
}

// 같은 Player 기록/연결 객체를 매 순회 다시 읽는 송신 분배 몸체이다. 단독 캠페인은 모두 NULL 연결인 표를 쓴다. (2026-11-11 WB04-E311)
export class RawBattleBroadcast {
    // 사용: players·peers·localOwner·server 는 이 객체보다 오래 살아야 한다(빌린다). assertion 은 제외 송신에서 server 가 0 일 때 부르는 실제 server 진단 장치이다. (2026-11-11 WB04-E311)
    constructor(players, peers, localOwner, server, assertion = null) {
        if (!(players instanceof Uint8Array) || players.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new RangeError('송신 분배 Player9 크기 오류');
        if (!Array.isArray(peers) || peers.length !== PLAYER_COUNT) throw new TypeError('송신 분배 연결 표는 길이 9 배열이어야 합니다.');
        // 상자 두 개는 현재값을 읽을 때마다 쓰므로 { value } 모양이어야 한다.
        for (const box of [localOwner, server]) {
            if (!box || typeof box !== 'object' || !('value' in box)) throw new TypeError('송신 분배의 현재값은 { value } 상자여야 합니다.');
        }
        if (assertion !== null && assertion !== undefined && typeof assertion !== 'function') throw new TypeError('송신 분배의 server 진단 장치는 함수여야 합니다.');
        this.players = players;
        this.peers = peers;
        this.localOwner = localOwner;
        this.server = server;
        this.assertion = assertion ?? null;
        this.mutableSend = null;
    }

    // PATCH 004e2e30: 현재 접속한 1..8 의 NULL 아닌 연결 중 로컬을 제외하고 같은 packet 을 가상 송신한다. localOwner 는 후보마다 다시 읽어 가상 송신의 상태 변경을 다음 후보에 반영한다. (2026-11-11 WB04-E311)
    send(packet) {
        // 원본 순회는 1 부터 8 까지이며 Player 0 은 건너뛴다.
        for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
            if (!(this.players[owner * PLAYER_STRIDE + CONNECTED_OFFSET] & 1) || !this.peers[owner] || owner === this.localOwner.value) continue;
            const send = this.peers[owner];
            send(packet);
        }
    }

    // 실제 target 별 가상 송신(실제 등록소의 송신)을 연결한다. observer 를 별도로 선행 호출하지 않으며, NULL 연결뿐인 독립 호출은 장치 없이 자연 반환한다. (2026-11-11 WB04-E311: 기준선 뒤 77a0c3a47)
    bindMutableSend(send) {
        if (send !== null && send !== undefined && typeof send !== 'function') throw new TypeError('mutable 송신 장치는 함수여야 합니다.');
        this.mutableSend = send ?? null;
    }

    // 같은 선택 순서(접속·NULL 연결·현재 로컬) 뒤 target 마다 현재 observer 가 같은 버퍼를 수정할 수 있게 `send(owner, packet)` 를 부른다. 앞 대상이 바꾼 packet 은 다음 대상 송신에도 보존된다. (2026-11-11 WB04-E311: 기준선 뒤 77a0c3a47)
    sendMutable(packet) {
        // 각 후보를 기존 send 와 같은 순서로 조회한다.
        for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
            if (!(this.players[owner * PLAYER_STRIDE + CONNECTED_OFFSET] & 1) || !this.peers[owner] || owner === this.localOwner.value) continue;
            if (!this.mutableSend) throw new Error('선택된 실제 방송 연결의 mutable 가상 송신 미연결');
            const send = this.mutableSend;
            send(owner, packet);
        }
    }

    // PATCH 004e2e80: server 를 확인하고(아니면 진단 장치, 정상 복귀하면 서버 값을 재검사하지 않고 계속), 시작 시 로컬 owner 를 보관해 이후에는 접속/연결 표만 다시 읽으며 지정 제외 번호(int32)와 로컬을 뺀 연결에 보낸다. (2026-11-11 WB04-E311)
    sendExcept(excluded, packet) {
        if (!this.server.value) {
            if (!this.assertion) throw new Error('원본 제외 송신의 실제 server 진단 장치 미연결');
            const assertion = this.assertion;
            assertion();
        }
        const local = this.localOwner.value;
        // 원본 후보 순서와 지정 제외/로컬 제외 조건을 유지한다.
        for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
            if (!(this.players[owner * PLAYER_STRIDE + CONNECTED_OFFSET] & 1) || owner === (excluded | 0) || owner === local || !this.peers[owner]) continue;
            const send = this.peers[owner];
            send(packet);
        }
    }
}
