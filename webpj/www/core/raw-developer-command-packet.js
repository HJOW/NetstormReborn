// 개발자 문자열 명령(31h) 패킷의 송신 길이 관찰(원본 0040a6b0)과 수신(0044dba0→0040a6f0)을 옮긴 모듈이다(cpppj `o/RawDeveloperCommandPacket.cpp/.h`, 10.78 전용).
// 사용:
//   observeDeveloperCommandPacket(packet, sending, assertion) → 송신일 때만 현재 +4 DWORD(len)가 0xffffffff 이면 진단(`len != (int)-1`, 파일 `\Ns\O\_p_packets.cpp`, 줄 0x74b) 뒤 복귀하면 현재 값을 다시 읽어 len+8 의 하위 WORD 를 +2 에 쓴다.
//   receiveDeveloperCommandPacket(packet, execute) → +8 의 첫 NUL 문자열을 같은 실제 해석기 execute(문자열)에 전달한다. 선언 길이·sender·연결은 조회하지 않으며 송신 길이 검사를 수신에 추가하지 않는다.
//   registerDeveloperCommandPacket(dispatch, execute, assertion) → 31h 의 버전 1/정책 0·가변 길이(0xffff)·방향 전용 관찰기·수신기를 등록한다(execute·assertion 은 등록 수명 동안 유지).
// 기존 등록과 Native 고정 수신기가 같은 첫 NUL/물리 범위/현재 문자열 효과 본문을 공유한다. 문자열은 바이트 값(문자 코드 0..255)이다. 전송/기록/권한은 기존 실제 분배기를 사용한다.
// 원본 관측 fixture 가 없어 cpppj 소스 계약을 tests/raw-developer-command-packet.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E559: 개발자 명령 패킷 이식)

// 31h 패킷 종류·본문 시작(+8)·길이 DWORD 위치(+4), 진단 문장·파일·줄이다. (2026-11-15 WB04-E559)
const PACKET_TYPE = 0x31;
const BODY_AT = 8;
const LENGTH_AT = 4;
const ASSERT_TEXT = 'len != (int)-1';
const ASSERT_FILE = '\\Ns\\O\\_p_packets.cpp';
const ASSERT_LINE = 0x74b;

// 현재 +4 의 네 BYTE 를 unsigned 로 읽는다(진단이 돌아온 뒤에도 같은 바이트를 다시 읽는다). (2026-11-15 WB04-E559)
const lengthOf = (packet) => (packet[LENGTH_AT] | (packet[LENGTH_AT + 1] << 8) | (packet[LENGTH_AT + 2] << 16) | (packet[LENGTH_AT + 3] << 24)) >>> 0;

// 0040a6b0: 송신에서만 len 진단 뒤 현재 하위 WORD+8 을 헤더에 쓴다(헤더 8바이트가 없으면 물리 오류). (2026-11-15 WB04-E559)
export function observeDeveloperCommandPacket(packet, sending, assertion) {
    if (!sending) return;
    if (packet.length < BODY_AT) throw new RangeError('개발자 명령의 현재 len DWORD 부족');
    if (lengthOf(packet) === 0xffffffff) {
        if (typeof assertion !== 'function') throw new Error('개발자 명령 len 진단 장치 미연결');
        assertion(ASSERT_TEXT, ASSERT_FILE, ASSERT_LINE);
    }
    // 현재 하위 WORD 의 감김을 그대로 보존한다.
    const count = (lengthOf(packet) + 8) >>> 0;
    packet[2] = count & 0xff;
    packet[3] = (count >>> 8) & 0xff;
}

// 0044dba0→0040a6f0(packet, 0): +8 의 첫 NUL 문자열을 해석기에 전달한다. 헤더만 있거나 NUL 이 없으면 물리 오류이고, 해석기가 없으면 앞선 검사 뒤 명시 오류이다. (2026-11-15 WB04-E559)
export function receiveDeveloperCommandPacket(packet, execute) {
    if (packet.length < BODY_AT + 1) throw new RangeError('개발자 명령 문자열 물리 부족');
    const end = packet.indexOf(0, BODY_AT);
    if (end < 0) throw new RangeError('개발자 명령 문자열 NUL 누락');
    if (typeof execute !== 'function') throw new Error('개발자 명령 실제 해석기 미연결');
    let text = '';
    for (let index = BODY_AT; index < end; index += 1) text += String.fromCharCode(packet[index]);
    execute(text);
}

// 실제 31h 를 분배기에 등록한다. 수신기는 sender 와 연결을 쓰지 않고 packet 만 해석기에 넘긴다. (2026-11-15 WB04-E559)
export function registerDeveloperCommandPacket(dispatch, execute, assertion) {
    dispatch.version(PACKET_TYPE, 1, 0);
    dispatch.requireLength(PACKET_TYPE, 0xffff);
    dispatch.observePacket(PACKET_TYPE, (packet, sending) => observeDeveloperCommandPacket(packet, sending, assertion));
    dispatch.register(PACKET_TYPE, (sender, connection, packet) => receiveDeveloperCommandPacket(packet, execute));
}
