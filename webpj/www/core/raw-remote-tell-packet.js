// 원격 Tell(70h) 문자열 패킷의 쓰기(원본 004c9150·004c9200)·송신 관찰기(0040b090)·수신(004cf980)을 옮긴 모듈이다(cpppj `o/RawRemoteTellPacket.cpp/.h`, 10.78 전용).
// 사용:
//   writeRemoteTellPacket(text, assertion, line) → 264바이트 패킷(전체 0xff 로 채운 뒤 버전 1·타입 0x70·선언 길이 4·0, +4 DWORD=문자열 길이+1, +8 부터 첫 NUL 까지의 문자열과 NUL). text 는 바이트 값(문자 코드 0..255)의 문자열이며 첫 NUL 뒤는 버린다.
//     길이+1 이 255 를 넘으면 진단(`z->tellLength < 256`, 파일 `.\Ui.cpp`, 줄은 호출자가 공급: 4ef/4fb) 뒤 복귀하면 계속하고, 256 을 넘으면 물리 범위 오류(RangeError)이다. 길이 DWORD 는 진단 전에 쓴다.
//   observeRemoteTellPacket(packet, sending, assertion) → 송신일 때만 현재 +4 DWORD 가 0xffffffff 이면 진단(`tellLength != (int)-1`, 파일 `\Ns\O\_p_packets.cpp`, 줄 0xac7) 뒤 복귀하면 현재값을 다시 읽어 길이+8 의 하위 WORD 를 +2 에 쓴다.
//   registerRemoteTellPacket(dispatch, tell, assertion) → 70h 의 버전 1/정책 0·가변 길이(0xffff)·방향 전용 관찰기·수신기를 등록한다.
//   receiveRemoteTellPacket(packet, tell) → +8 의 첫 NUL 문자열을 같은 실제 소비자 tell(문자열)에 전달한다. 선언 길이·sender·연결은 조회하지 않고 송신 길이 검사도 추가하지 않는다.
// 진단 함수 assertion(식, 파일, 줄)은 정상 복귀형이며 선택된 경로에서만 요구한다. 버전/기록/생존 관문은 기존 실제 분배기에 맡긴다.
// 원본 관측 fixture 가 없어 cpppj 소스 계약을 tests/raw-remote-tell-packet.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E559: 원격 Tell 패킷 이식)

// 70h 패킷 종류·고정 크기(264)·본문 시작(+8)·길이 DWORD 위치(+4)와 문자열 한계(길이+1 이 255 이하여야 진단 없음, 256 초과면 물리 범위 밖)이다. (2026-11-15 WB04-E559)
const PACKET_TYPE = 0x70;
const PACKET_BYTES = 264;
const BODY_AT = 8;
const LENGTH_AT = 4;
const ASSERT_LIMIT = 255;
const PHYSICAL_LIMIT = 256;
// 진단 문장·파일과 두 줄 번호(쓰기 호출자가 공급하는 4ef/4fb 는 매개변수)이다. (2026-11-15 WB04-E559)
const WRITE_ASSERT_TEXT = 'z->tellLength < 256';
const WRITE_ASSERT_FILE = '.\\Ui.cpp';
const OBSERVE_ASSERT_TEXT = 'tellLength != (int)-1';
const OBSERVE_ASSERT_FILE = '\\Ns\\O\\_p_packets.cpp';
const OBSERVE_ASSERT_LINE = 0xac7;

// 선택된 진단/소비자 장치가 함수가 아니면 명시 오류를 낸다(선택되지 않은 진단은 요구하지 않는다). (2026-11-15 WB04-E559)
function need(device) {
    if (typeof device !== 'function') throw new Error('원격 Tell 실제 장치 미연결');
    return device;
}

// 현재 +4 의 네 BYTE 만 unsigned 로 읽는다(헤더 8바이트가 없으면 물리 오류). (2026-11-15 WB04-E559)
function readLength(packet) {
    if (packet.length < BODY_AT) throw new RangeError('원격 Tell 헤더 물리 누락');
    return (packet[LENGTH_AT] | (packet[LENGTH_AT + 1] << 8) | (packet[LENGTH_AT + 2] << 16) | (packet[LENGTH_AT + 3] << 24)) >>> 0;
}

// 004c9150/4c9200: 264바이트 전체 0xff 채움·헤더·길이 DWORD·NUL 복사이다. 길이 DWORD 는 진단 전에 쓰고, 256 초과는 진단 뒤 물리 범위 오류이다. (2026-11-15 WB04-E559)
export function writeRemoteTellPacket(text, assertion, line) {
    // 첫 NUL 앞까지만 쓴다(NUL 이 없으면 전체).
    const nul = text.indexOf('\0');
    const body = nul < 0 ? text : text.slice(0, nul);
    const packet = new Uint8Array(PACKET_BYTES).fill(0xff);
    packet[0] = 1;
    packet[1] = PACKET_TYPE;
    packet[2] = 4;
    packet[3] = 0;
    const length = body.length + 1;
    // 원문의 현재 strlen+1 DWORD 를 진단 전에 낮은 BYTE 부터 쓴다.
    for (let index = 0; index < 4; index += 1) packet[LENGTH_AT + index] = (length >>> (index * 8)) & 0xff;
    if (length > ASSERT_LIMIT) need(assertion)(WRITE_ASSERT_TEXT, WRITE_ASSERT_FILE, line);
    if (length > PHYSICAL_LIMIT) throw new RangeError('원격 Tell264BYTE 복사 물리 범위');
    for (let index = 0; index < body.length; index += 1) packet[BODY_AT + index] = body.charCodeAt(index) & 0xff;
    packet[BODY_AT + body.length] = 0;
    return packet;
}

// 0040b090: 송신에서만 현재 +4 DWORD 의 sentinel 진단 뒤 다시 읽어 길이+8 의 하위 WORD 를 선언 길이로 쓴다(32비트 합이 감긴다). (2026-11-15 WB04-E559)
export function observeRemoteTellPacket(packet, sending, assertion) {
    if (!sending) return;
    if (readLength(packet) === 0xffffffff) need(assertion)(OBSERVE_ASSERT_TEXT, OBSERVE_ASSERT_FILE, OBSERVE_ASSERT_LINE);
    const length = (readLength(packet) + 8) >>> 0;
    packet[2] = length & 0xff;
    packet[3] = (length >>> 8) & 0xff;
}

// 004cf980: +8 의 첫 NUL 문자열을 현재 Tell 소비자에 전달한다. 헤더만 있는 패킷·NUL 없는 패킷은 물리 오류이다. (2026-11-15 WB04-E559)
export function receiveRemoteTellPacket(packet, tell) {
    if (packet.length < BODY_AT + 1) throw new RangeError('원격 Tell 실제 문자열 물리 누락');
    const end = packet.indexOf(0, BODY_AT);
    if (end < 0) throw new RangeError('원격 Tell 실제 NUL 누락');
    let text = '';
    for (let index = BODY_AT; index < end; index += 1) text += String.fromCharCode(packet[index]);
    need(tell)(text);
}

// 실제 70h 를 분배기에 등록한다. 수신기는 sender 와 연결을 쓰지 않고 packet 만 소비자에 넘긴다. (2026-11-15 WB04-E559)
export function registerRemoteTellPacket(dispatch, tell, assertion) {
    dispatch.version(PACKET_TYPE, 1, 0);
    dispatch.requireLength(PACKET_TYPE, 0xffff);
    dispatch.observePacket(PACKET_TYPE, (packet, sending) => observeRemoteTellPacket(packet, sending, assertion));
    dispatch.register(PACKET_TYPE, (sender, connection, packet) => receiveRemoteTellPacket(packet, tell));
}
