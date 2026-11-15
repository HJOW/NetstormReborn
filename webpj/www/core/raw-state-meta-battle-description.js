// 일반/메타 71h 전투 설명 패킷의 송신 observer·버전·길이·수신 관문(원본 0040b0d0 등록·004b49c0 수신)을 같은 분배 표에 등록하는 모듈이다(cpppj `o/RawStateMetaBattleDescription.cpp/.h`, 10.78 전용).
// 사용: registerStateMetaBattleDescription(dispatch, currentCs, receive, assertion)
//   dispatch = RawPacketDispatch, currentCs = 현재 CS(Outpost) DWORD 의 { value } 상자·현재 값을 돌려주는 함수·단순 값 중 하나(수신 때마다 다시 읽는다),
//   receive(sender, packet) = 실제 Client CS 자식(0042cad0), assertion(문장) = 원문 packet 파일 ae3 의 복귀형 진단(`descLen != (unsigned char)-1`).
//   71h 에 방향 전용 observer(송신일 때만 packet[9] 가 0xff 면 진단 뒤 길이 WORD(+2)를 packet[9]+10 으로 확정), 버전 1/정책 0, 가변 길이(0xffff), 수신기를 등록한다.
//   수신기는 mode/sender/known 관문 없이 현재 CS 가 비영일 때만 receive 를 호출한다(CS 0 이면 자식을 요구하지도 않는다).
// 같은 분배 표에 실제 Native 연결 수신(004b49c0, `canonicalReceive`)이 이미 있으면 그 값을 빌려 재등록한다(웹 분배기에는 canonical 등록이 없어 독립 입력은 위 본문을 쓴다).
// 도달한 필드/장치만 읽고 쓰며 전체 패킷을 선검사하지 않는다(길이 재조회·BYTE 읽기가 패킷 물리 밖이면 RangeError). 이 모듈은 DOM 을 쓰지 않는다.
// 원본 관측 fixture 가 없어 cpppj 소스 계약을 tests/raw-state-meta-battle-description.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). (2026-11-15 WB04-E558: 71h 설명 수신 이식)

// 71h 패킷 종류·설명 길이 BYTE 와 길이 WORD 의 패킷 안 위치, 헤더+설명의 고정 머리 길이(10)와 가변 길이 표시이다. (2026-11-15 WB04-E558)
const PACKET_TYPE = 0x71;
const DESCRIPTION_LENGTH_AT = 9;
const LENGTH_WORD_AT = 2;
const HEADER_BYTES = 10;
const SENTINEL = 0xff;
const NATIVE_ENTRY = 0x004b49c0;
const VARIABLE_LENGTH = 0xffff;
// 읽기 전용 현재 전역은 { value } 상자·현재 값을 돌려주는 함수·독립 입력의 단순 값 중 하나로 받아 수신마다 다시 읽는다. (2026-11-15 WB04-E558)
const read = (source) => (typeof source === 'function' ? source() : (source !== null && typeof source === 'object' ? source.value : source));

// 선택된 실제 장치(진단/수신 자식)가 함수가 아니면 명시 오류를 낸다(CS 0/정상 길이는 미사용 장치를 요구하지 않는다). (2026-11-15 WB04-E558)
function need(device) {
    if (typeof device !== 'function') throw new Error('전투 설명 State의 선택된 실제 장치 미연결');
    return device;
}

// 현재 BYTE 한 개를 읽는다. 패킷 물리 밖이면 RangeError 이다. (2026-11-15 WB04-E558)
function byteAt(packet, at) {
    if (at >= packet.length) throw new RangeError('전투 설명 observer BYTE 물리 범위');
    return packet[at];
}

// 현재 packet+2 의 WORD 에 길이를 쓴다(낮은 BYTE 먼저). 최소 4바이트가 필요하다. (2026-11-15 WB04-E558)
function writeLength(packet, length) {
    if (packet.length < 4) throw new RangeError('전투 설명 observer WORD 물리 범위');
    packet[LENGTH_WORD_AT] = length & 0xff;
    packet[LENGTH_WORD_AT + 1] = (length >>> 8) & 0xff;
}

// 71h 등록 전체이다. 같은 Native 수신(004b49c0)이 이미 있고 등록 장치가 있으면 그것을 재등록하고, 아니면 observer·버전·길이·수신기를 독립으로 등록한다. (2026-11-15 WB04-E558)
export function registerStateMetaBattleDescription(dispatch, currentCs, receive, assertion) {
    const baseline = dispatch.registration(PACKET_TYPE).canonicalReceive;
    // 동일 F 만 이번 Session 에 대여하고 장기 observer/원문 metadata 는 그대로 유지한다.
    if (baseline && baseline.nativeEntry === NATIVE_ENTRY) {
        if (typeof dispatch.registerCanonical !== 'function') throw new Error('Native 연결 수신 등록 장치 미연결');
        dispatch.registerCanonical(PACKET_TYPE, baseline);
        return;
    }
    // 송신일 때만 설명 길이 BYTE 로 전체 길이를 확정한다. 진단이 BYTE 를 바꿔도 길이 계산에서 현재 값을 다시 읽는다.
    dispatch.observePacket(PACKET_TYPE, (packet, sending) => {
        if (!sending) return;
        if (byteAt(packet, DESCRIPTION_LENGTH_AT) === SENTINEL) need(assertion)('descLen != (unsigned char)-1');
        writeLength(packet, byteAt(packet, DESCRIPTION_LENGTH_AT) + HEADER_BYTES);
    });
    dispatch.version(PACKET_TYPE, 1, 0);
    dispatch.requireLength(PACKET_TYPE, VARIABLE_LENGTH);
    // 원문은 mode/sender/known 관문 없이 현재 CS 비영일 때만 자식을 호출한다.
    dispatch.register(PACKET_TYPE, (sender, connection, packet) => {
        if (read(currentCs)) need(receive)(sender, packet);
    });
}
