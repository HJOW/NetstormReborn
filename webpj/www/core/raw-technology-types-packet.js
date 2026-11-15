// 69h 기술 타입 사용 목록 패킷의 누적(원본 0041c340)·수신(004b49e0)·송신 관찰기(0040af10)·등록을 옮긴 모듈이다(cpppj `o/RawTechnologyTypesPacket.cpp/.h`, 10.78 전용).
// 사용:
//   addTechnologyTypes(input, typeUse) → input(Uint8Array, 첫 NUL 까지)의 각 BYTE 를 현재 목록 typeUse(256바이트, NUL 종료)의 첫 일치 또는 첫 빈칸에 합친다. 별도 개수/정렬/종료 NUL 쓰기가 없고 BYTE 0x80..0xff 도 그대로 비교한다.
//   receiveTechnologyTypesPacket(packet, typeUse, indicate) → payload+5 를 합집합한 뒤 기존/신규 전체 현재 목록을 첫 NUL 까지 다시 순회하며 항목마다 indicate(unsigned BYTE 값)를 호출한다(188 TYPE 사전 검사 없음).
//   observeTechnologyTypesPacket(packet, sending, assertion) → 송신일 때만 packet[4] 가 0xff 이면 진단(`len != (unsigned char)-1`, 파일 `\Ns\O\_p_packets.cpp`, 줄 0xa5f) 뒤 복귀하면 현재 BYTE 를 다시 읽어 길이+5 를 +2 하위 WORD 로 쓴다.
//   registerTechnologyTypesPacket(dispatch, typeUse, indicate, assertion) → 69h 의 방향 전용 관찰기·버전 1/정책 0·가변 길이(0xffff)·직접 호출/분배 겸용 수신기를 같은 슬롯에 설치한다.
// 목록은 unsigned BYTE 문자열이며 CoreData/TYPE 전체 표나 ACK 를 복사하지 않는다. sender/known/서버/phase 관문을 추가하지 않는다. 앞선 부분 쓰기는 되돌리지 않으며, 입력이 모자라거나 목록에 종료 NUL 이 없으면
// 선택된 위치의 물리 오류(RangeError)로 구분한다. 표시/진단 장치는 도달한 경로에서만 요구한다(빈 목록에서는 표시 장치를 요구하지 않는다).
// 원본 관측 fixture 가 없어 cpppj 소스 계약을 tests/raw-technology-types-packet.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E560: 기술 타입 69h 패킷 이식)

// 69h 패킷 종류·목록 크기(256)·페이로드 시작(+5)·길이 BYTE 위치(+4)와 진단 문장·파일·줄이다. (2026-11-15 WB04-E560)
const PACKET_TYPE = 0x69;
const TYPE_USE_BYTES = 256;
const PAYLOAD_AT = 5;
const LENGTH_AT = 4;
const ASSERT_TEXT = 'len != (unsigned char)-1';
const ASSERT_FILE = '\\Ns\\O\\_p_packets.cpp';
const ASSERT_LINE = 0xa5f;
// 선언 길이 = 길이 BYTE + 5(앞쪽 5바이트 헤더 몫)이다. 진단이 복귀하고 BYTE 가 255 로 남으면 260 이 그대로 하위 WORD 에 쓰인다. (2026-11-15 WB04-E560)
const LENGTH_ADD = 5;

// 선택된 표시/진단 장치가 함수가 아니면 명시 오류를 낸다(빈 목록·정상 길이에서는 요구하지 않는다). (2026-11-15 WB04-E560)
function need(device) {
    if (typeof device !== 'function') throw new Error('69h의 현재 기술표시/진단 장치 미연결');
    return device;
}

// 선택한 위치의 BYTE 한 개를 읽는다. 물리 범위 밖이면 다음 BYTE 를 미리 검사하거나 이전 쓰기를 되돌리지 않고 그 자리에서 RangeError 이다. (2026-11-15 WB04-E560)
function byteAt(bytes, index) {
    if (index >= bytes.length) throw new RangeError('69h 타입 목록의 선택된 현재 BYTE 물리 부족');
    return bytes[index];
}

// 0041c340: 입력 BYTE 마다 현재 목록의 첫 NUL 또는 첫 일치만 선택한다. 입력의 첫 NUL 을 읽을 때까지 순차 접근하므로 모자란 뒤쪽 입력 앞의 저장은 남는다. (2026-11-15 WB04-E560)
export function addTechnologyTypes(input, typeUse) {
    // 실제 첫 NUL 을 읽을 때까지 입력을 한 BYTE 씩 소비한다.
    for (let source = 0; ; source += 1) {
        const value = byteAt(input, source);
        if (!value) return;
        // 이번 입력이 선택할 현재 목록 위치이다(정상 255종+NUL 을 넘는 손상 상태는 index 256 읽기에서 오류가 된다).
        let index = 0;
        for (;;) {
            const current = byteAt(typeUse, index);
            if (!current || current === value) break;
            index += 1;
        }
        // 선택한 첫 빈칸에만 쓰며 현재 원천 BYTE 를 다시 읽는다.
        if (!byteAt(typeUse, index)) typeUse[index] = byteAt(input, source);
    }
}

// 004b49e0: payload+5 합집합이 끝난 뒤에만 전체 현재 목록을 표시한다. 각 표시 뒤 다음 목록 BYTE 를 다시 읽으므로 표시 효과가 바꾼 다음 항목/종료점도 그 자리에서 선택한다. (2026-11-15 WB04-E560)
export function receiveTechnologyTypesPacket(packet, typeUse, indicate) {
    if (packet.length < PAYLOAD_AT) throw new RangeError('69h payload+5의 실제 물리 부족');
    // 길이 BYTE(+4)는 제한/검증/복사 길이로 읽지 않는다.
    addTechnologyTypes(packet.subarray(PAYLOAD_AT), typeUse);
    // 신규값뿐 아니라 기존 목록도 현재 첫 NUL 까지 원문 순서대로 다시 전달한다.
    for (let index = 0; ; index += 1) {
        const current = byteAt(typeUse, index);
        if (!current) return;
        need(indicate)(current);
    }
}

// 0040af10: 수신 방향은 패킷 필드를 읽지 않는다. 송신은 진단 정상 복귀 뒤 현재 len BYTE 를 다시 읽어 +5 한 값(0xff 그대로면 260)의 하위 WORD 를 선언 길이로 쓴다. (2026-11-15 WB04-E560)
export function observeTechnologyTypesPacket(packet, sending, assertion) {
    if (!sending) return;
    if (byteAt(packet, LENGTH_AT) === 0xff) need(assertion)(ASSERT_TEXT, ASSERT_FILE, ASSERT_LINE);
    const length = byteAt(packet, LENGTH_AT) + LENGTH_ADD;
    // 앞선 +4 읽기가 성공했으므로 선언 WORD 물리도 존재한다.
    packet[2] = length & 0xff;
    packet[3] = (length >>> 8) & 0xff;
}

// 69h 등록: 직접 호출(null 연결 허용)과 framed 수신이 같은 callable 을 쓰고 sender/연결은 본문에서 읽지 않는다. typeUse/indicate 는 등록 수명 동안 유지한다. (2026-11-15 WB04-E560)
export function registerTechnologyTypesPacket(dispatch, typeUse, indicate, assertion) {
    if (typeUse.length !== TYPE_USE_BYTES) throw new TypeError('69h 타입 사용 목록은 256바이트여야 합니다.');
    dispatch.observePacket(PACKET_TYPE, (packet, sending) => observeTechnologyTypesPacket(packet, sending, assertion));
    dispatch.version(PACKET_TYPE, 1, 0);
    dispatch.requireLength(PACKET_TYPE, 0xffff);
    dispatch.registerDirect(PACKET_TYPE, (sender, connection, packet) => receiveTechnologyTypesPacket(packet, typeUse, indicate));
}
