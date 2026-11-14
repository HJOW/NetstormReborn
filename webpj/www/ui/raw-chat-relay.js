// 0042dc90 채팅 중계와 41h 등록을 기준선 1f00a25a의 RawChatRelay.cpp/.h에서 옮긴다.
// 사용: relayChatPacket(sender, mutablePacket, { value: mask }, hooks). 주소→현재 전투 접두→서버 자기 수신자→로컬 수신 순서다.
// mask는 ClientChatWindowGlobals.receivedMask를 접근자로 빌려야 한다. 다른 subscriber로의 LAN 중계는 웹 제외 오류다.
// 기준선 뒤 canonical 장기 수신기 재등록은 분배표 소유자와 함께 4-D에서 연결한다. (2026-11-14 WB04-E449)
import { observeChatPacket } from './raw-chat-packet.js';

// 선택된 실제 효과만 요구한다. 미선택 장치는 요구하지 않는다.
function need(hook) { if (typeof hook !== 'function') throw new Error('Chat 중계의 선택된 실제 장치 미연결'); return hook; }

// 비정렬 필드를 지정 폭으로 읽으며 실제 버퍼 밖의 주소는 거부한다.
function read(bytes, at, width = 4) {
    if (at < 0 || at > bytes.length || width > bytes.length - at) throw new RangeError('Chat 중계의 실제 필드 범위 부족');
    let value = 0;
    // 낮은 BYTE부터 합산해 DWORD 부호를 바꾸지 않는다.
    for (let index = 0; index < width; index += 1) value += bytes[at + index] * 2 ** (8 * index);
    return value;
}

// 수신자 첫 DWORD만 바꾸며 뒤의 목록과 미작성 꼬리는 그대로 둔다.
function put(bytes, at, value) {
    if (at < 0 || at > bytes.length || bytes.length - at < 4) throw new RangeError('Chat 중계의 실제 DWORD 쓰기 범위 부족');
    // 네 BYTE만 리틀엔디언으로 쓴다.
    for (let index = 0; index < 4; index += 1) bytes[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// nameLen WORD는 signed로 읽어 원문 본문 주소를 구한다.
function body(packet) {
    const at = 15 + (read(packet, 10, 2) << 16 >> 16);
    if (at < 0 || at > packet.length) throw new RangeError('Chat 중계의 실제 signed 본문 주소');
    return at;
}

// 처음 NUL 앞 최대 세 BYTE에만 CRT atol의 공백/부호/숫자 규칙을 적용한다.
function prefixMask(bytes) {
    const end = bytes.indexOf(0); if (end < 0) throw new RangeError('Chat 중계 접두의 실제 NUL 없음');
    const length = Math.min(3, end); let at = 0;
    // CRT의 여섯 ASCII 공백만 건너뛴다.
    while (at < length && [32, 13, 10, 9, 11, 12].includes(bytes[at])) at += 1;
    let negative = false;
    if (at < length && (bytes[at] === 45 || bytes[at] === 43)) { negative = bytes[at] === 45; at += 1; }
    let value = 0;
    // 첫 비숫자 또는 세 BYTE 끝에서 멈춘다.
    while (at < length && bytes[at] >= 48 && bytes[at] <= 57) { value = value * 10 + bytes[at] - 48; at += 1; }
    return (negative ? -value : value) >>> 0;
}

// 원문 주소 준비 이후의 현재 전역을 매 위치에서 다시 읽고 같은 패킷과 마스크를 변경한다.
export function relayChatPacket(sender, packet, mask, hooks) {
    const address = need(hooks.address)(sender >>> 0); const global = (at) => need(hooks.read)(at) >>> 0;
    if (global(0x594fbc)) {
        const at = body(packet); const length = read(packet, 12, 2);
        if (at + length === 0 || at + length > packet.length) throw new RangeError('Chat 중계의 실제 마지막 본문 BYTE 없음');
        packet[at + length - 1] = 0; mask.value = prefixMask(packet.subarray(at));
    }
    let local = false;
    if (global(0x540bc0)) {
        const count = read(packet, 14, 1); packet[14] = 1; if (!count) return;
        // 최초 count만 고정하고 현재 본문 위치/수신자 항목/자기 subscriber는 반복마다 읽는다.
        for (let index = 0; index < count; index += 1) {
            const start = body(packet) + read(packet, 12, 2); const subscriber = read(packet, start + index * 4);
            put(packet, start, subscriber);
            if (subscriber === global(0x540c74)) local = true;
            else throw new Error('웹 제외: 다른 subscriber로의 LAN 채팅 중계');
        }
        if (!local) return;
    }
    need(hooks.receive)(packet, address);
}

// 같은 분배표에 원문 버전/관찰기/중계 수신기를 등록한다. 송신 관찰기가 mutable 패킷의 길이를 확정한다.
export function registerChatPacket(dispatch, mask, hooks) {
    const current = { ...hooks }; const assertion = current.assertion; // C++ 값 캡처처럼 함수 목록을 고정하고 각 함수가 읽는 실제 전역만 현재값으로 빌린다.
    dispatch.version(0x41, 1, 0);
    dispatch.observePacket(0x41, (packet, sending) => observeChatPacket(packet, sending, assertion));
    dispatch.register(0x41, (sender, connection, packet) => relayChatPacket(sender, packet, mask, current));
}
