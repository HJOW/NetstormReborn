// 건설 확정(메시지 0x3f)의 실제 0x200바이트 임시 패킷과 가변길이 확정 helper를 옮긴 모듈이다(cpppj `o/RawConstructionPacket.cpp/.h`, 10.78 00444590→004df590 / 0040a840 / 0040a800).
//  - ConstructionPacketView(packet): 통지 처리기(004441b0)와 SID 읽기(40a840)가 같은 원문 BYTE 버퍼를 빌려 쓰는 읽기 전용 뷰이다. 필드는 생성 때 읽지 않고
//    소비 위치마다 현재 바이트에서 다시 읽는다(type[+4]·x[+5]·y[+9]·argument[+13]·direction[+14]·player[+15]·flags[+16]·quality[+17]·timeLow[+18]·timeHigh[+22]·sidCount[+26]·
//    SID WORD[+0x1b 부터 2바이트씩, 홀수 주소]). 선언 개수나 19개 제한을 앞세우지 않고 요청한 칸만 읽으며, 물리 범위를 넘으면 RangeError 이다.
//  - initializeConstructionPacket(header, packet): 512바이트를 0xff 로 채우고 protocol 의 하위 7비트·message·선언 길이 WORD(최소 4)만 쓴다. 본문은 0xff 로 남는다.
//  - writeConstructionPacket(notice, header, packet): 초기화 뒤 SID 목록을 먼저 쓰고 type·x·y·방향·인자·플레이어·시각 두 DWORD·sidCount·flags·quality 를 쓴다. 512를 반환하며
//    512바이트 뒤의 호출자 버퍼는 건드리지 않는다. SID 가 19개를 넘거나 버퍼가 512 미만이면 효과 전에 거부한다. 이 단계의 선언 길이는 최종화 이전 값이다.
//  - finalizeConstructionPacket(packet, sending, invalidCount): sending 이 0 이면 아무것도 하지 않는다. 아니면 sidCount 가 255 일 때 원본 진단(invalidCount)을 호출한 뒤에도 계속해
//    선언 길이를 sidCount×2+27 로 쓴다. 진단 장치가 없으면 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E134: 건설 패킷 이식으로 추가)

// 임시 패킷 크기(0x200), SID 목록 시작 위치(0x1b), 정상 SID 상한(19), 초기 채움 값이다. (2026-11-08 WB04-E134)
const TEMPORARY_SIZE = 0x200;
const PAYLOAD_START = 0x1b;
const MAX_SIDS = 19;
const FILL = 0xff;

// 지정 폭만 리틀엔디언으로 쓴다. 주변 바이트는 바꾸지 않는다. (2026-11-08 WB04-E134)
function put(bytes, offset, value, width) {
    for (let index = 0; index < width; index += 1) bytes[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// 단정도 float 의 비트(uint32)이다. (2026-11-08 WB04-E134)
const view = new DataView(new ArrayBuffer(4));
function bitsOf(value) {
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 실제 Confirm 입구가 읽는 현재 메시지 등록값이다(PATCH 0052edea/b 의 초기 BYTE, 00544c58 의 선언 길이 WORD 0). 사용: const header = createConstructionPacketHeader(). (2026-11-08 WB04-E134)
export function createConstructionPacketHeader() {
    return { message: 0x3f, protocol: 1, declaredLength: 0 };
}

// 통지 처리기가 물려받는 원문 바이트 버퍼의 읽기 전용 뷰이다. packet 은 방송/직접 수신이 끝날 때까지 호출자가 소유한 같은 현재 버퍼여야 한다. (2026-11-08 WB04-E134)
export class ConstructionPacketView {
    #packet;

    // 미사용 필드나 전체 SID 목록은 생성 때 검사하지 않는다. (2026-11-08 WB04-E134)
    constructor(packet) {
        this.#packet = packet;
    }

    // 지정 필드의 물리 범위만 확인하고 현재 바이트에서 리틀엔디언으로 읽는다. 앞선 부분 효과는 되돌리지 않는다. (2026-11-08 WB04-E134)
    #read(offset, width) {
        if (offset > this.#packet.length || width > this.#packet.length - offset) throw new RangeError('건설 통지의 현재 wire 필드 물리 부족');
        let value = 0;
        for (let byte = 0; byte < width; byte += 1) value += this.#packet[offset + byte] * 2 ** (8 * byte);
        return value;
    }

    // 현재 TYPE BYTE(+4)이다.
    type() { return this.#read(4, 1); }
    // 현재 +5 의 비정렬 float 이다.
    x() { return this.#float(5); }
    // 현재 +9 의 비정렬 float 이다.
    y() { return this.#float(9); }
    // +5 의 float 비트(uint32)이다. NaN 페이로드를 보존해야 하는 소비자가 쓴다.
    xBits() { return this.#read(5, 4); }
    // +9 의 float 비트(uint32)이다.
    yBits() { return this.#read(9, 4); }
    // 진입 때 보관할 인자 BYTE(+13)이다.
    argument() { return this.#read(13, 1); }
    // 진입 때 보관할 방향 BYTE(+14)이다.
    direction() { return this.#read(14, 1); }
    // 소유자 BYTE(+15)이다. 각 하위 효과 뒤 현재 값을 다시 읽는다.
    player() { return this.#read(15, 1); }
    // Clear 뒤 abstract/Builder 선택 위치에서 읽는 flags BYTE(+16)이다.
    flags() { return this.#read(16, 1); }
    // Place 호출 직전에 보관하는 품질 BYTE(+17)이다.
    quality() { return this.#read(17, 1); }
    // Builder 이동 분기에서만 읽는 시각 하위 DWORD(+18)이다.
    timeLow() { return this.#read(18, 4); }
    // Builder 이동 분기에서만 읽는 시각 상위 DWORD(+22)이다.
    timeHigh() { return this.#read(22, 4); }
    // Place 호출 시 고정할 선언 개수 BYTE(+26)이다.
    sidCount() { return this.#read(26, 1); }
    // 40a840 의 홀수 주소 SID WORD 를 실제 칸 소비 때 읽는다.
    sidAt(index) { return this.#read(PAYLOAD_START + index * 2, 2); }

    // 비정렬 단정도 float 를 비트로 조립해 읽는다. (2026-11-08 WB04-E134)
    #float(offset) {
        view.setUint32(0, this.#read(offset, 4), true);
        return view.getFloat32(0, true);
    }
}

// 004df590: 512바이트를 0xff 로 지우고 헤더(protocol 하위 7비트·message·선언 길이 WORD 최소 4)만 먼저 쓴다. decoder/Create 보다 앞서 부르며 본문은 0xff 로 남는다. (2026-11-08 WB04-E134)
export function initializeConstructionPacket(header, packet) {
    if (packet.length < TEMPORARY_SIZE) throw new RangeError('건설 초기 패킷 512BYTE 물리 부족');
    packet.fill(FILL, 0, TEMPORARY_SIZE);
    packet[0] = header.protocol & 0x7f;
    packet[1] = header.message;
    put(packet, 2, Math.max(4, header.declaredLength), 2);
}

// 00444590→004df590/0040a840: 512바이트를 0xff 로 지운 뒤 원본 필드와 SID 를 쓴다. 반환 크기는 512 이다. notice.x/y 는 수, 비트가 중요하면 xBits/yBits(uint32)를 우선한다. (2026-11-08 WB04-E134)
export function writeConstructionPacket(notice, header, packet) {
    if (packet.length < TEMPORARY_SIZE || notice.sids.length > MAX_SIDS) throw new RangeError('건설 패킷의 512바이트/정상 SID 19개 범위 오류');
    initializeConstructionPacket(header, packet);
    // 원본 Confirm 은 현재 decoder 순서의 SID 목록을 먼저 쓴다.
    for (let index = 0; index < notice.sids.length; index += 1) put(packet, PAYLOAD_START + 2 * index, notice.sids[index], 2);
    packet[4] = notice.type & 0xff;
    put(packet, 5, notice.xBits ?? bitsOf(notice.x), 4);
    put(packet, 9, notice.yBits ?? bitsOf(notice.y), 4);
    packet[14] = notice.direction & 0xff;
    packet[13] = notice.argument & 0xff;
    packet[15] = notice.player & 0xff;
    put(packet, 18, notice.timeLow, 4);
    packet[26] = notice.sids.length & 0xff;
    put(packet, 22, notice.timeHigh, 4);
    packet[16] = notice.flags & 0xff;
    packet[17] = notice.quality & 0xff;
    return TEMPORARY_SIZE;
}

// 0040a800: sending 이 비영일 때만 sidCount 로 선언 길이를 갱신한다. sidCount 255 의 원본 진단은 callback 뒤에도 계산을 계속하며 물리 버퍼 밖은 읽지 않는다. (2026-11-08 WB04-E134)
export function finalizeConstructionPacket(packet, sending, invalidCount = null) {
    if (!sending) return;
    if (packet.length < PAYLOAD_START) throw new RangeError('건설 패킷 길이 확정의 현재 버퍼 부족');
    if (packet[26] === 255) {
        if (typeof invalidCount !== 'function') throw new Error('건설 패킷 sidCount 원본 진단 미연결');
        invalidCount();
    }
    put(packet, 2, packet[26] * 2 + PAYLOAD_START, 2);
}
