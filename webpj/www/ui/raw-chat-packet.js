// 원본 0042d280 의 일반 채팅 패킷 작성·채널 선택과 0040a8d0 의 송신 길이 확정을 옮긴다(cpppj client/RawChatPacket.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 본문 조합 부모(RawChatMessage)와 실제 전송/수신 장치 사이의 경계다. 패킷 종류 0x41/버전 3, 현재 전역의 재조회, 전투/비전투의 서로 다른 복사 폭, 패킷 바이트/WORD/BYTE 절단을 보존한다.
// 실제 전송 채널·지역 수신은 호스트 훅이다(전송을 빈 성공으로 대체하지 않는다). 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: sendChatPacket(manager, recipients, bodyBytes, { memory, currentName, read, addRecipient, send, log, zeroAddress, localReceive }); observeChatPacket(packet, sending, assertion).
// (2026-11-14 WB04-E446: RawMenuControls 선행 Chat 패킷 이식)

// 원문이 각 호출 위치에서 읽는 현재 네 DWORD 의 주소다: subscriber/전투/채널 하나/메타맵.
const SUBSCRIBER = 0x540c74; const BATTLE = 0x594fbc; const CHANNEL_ONE = 0x594fb8; const METAMAP = 0x594fc8;
// 현재 이름이 비었을 때의 원문 정적 fallback 이름이다.
const NAMELESS = '++Nameless++';

// 도달한 실제 장치만 요구하고 미선택 장치는 미리 요구하지 않는다(C++ logic_error → Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('Chat 패킷의 선택된 실제 장치 미연결'); return hook; }

// 명시 물리 안의 실제 BYTE/WORD/DWORD 만 낮은 BYTE 부터 읽는다(폭 밖의 인접 BYTE 는 읽지 않는다, 범위 밖은 RangeError).
function read(bytes, at, width = 4) {
    if (at > bytes.length || width > bytes.length - at) throw new RangeError('Chat 패킷 읽기의 실제 물리 부족');
    let value = 0; for (let index = 0; index < width; index += 1) value += bytes[at + index] * 2 ** (index * 8);
    return value;
}

// 주변 꼬리를 유지하며 지정 원문 폭만 쓴다(WORD/BYTE 절단도 이 store 에서 발생한다, 범위 밖은 RangeError).
function put(bytes, at, value, width = 4) {
    if (at > bytes.length || width > bytes.length - at) throw new RangeError('Chat 패킷 쓰기의 실제 물리 부족');
    for (let index = 0; index < width; index += 1) bytes[at + index] = Math.floor(value / 2 ** (index * 8)) & 0xff;
}

// 첫 NUL 까지의 현재 바이트를 바이트 문자열로 읽는다(NUL 이 없으면 RangeError).
function text(bytes) {
    const end = bytes.indexOf(0); if (end < 0) throw new RangeError('Chat 패킷 문자열의 실제 NUL 없음');
    let value = ''; for (let index = 0; index < end; index += 1) value += String.fromCharCode(bytes[index]);
    return value;
}

// 0042d280: 현재 이름을 고정하고 자기 subscriber 가 수신자에 있는지 보고(없으면 추가) 전투 모드면 flags(+16, 직접 대상이면 |0x100)를 세 자리 이상 십진 앞 0 패딩으로 본문 앞에 붙인 뒤(본문은 첫 NUL 앞만),
// 15바이트 헤더(종류 3·+1 전역 BYTE·+2 길이 WORD·+5 1·+6 subscriber·+10 이름 크기·+12 본문 크기·+14 수신자 수)·이름·본문·수신자 배열 순으로 0xff 로 채운 패킷을 만들어 채널을 고른다: 전역 594fb8 비영 → 1, 아니면 메타맵 비영 → 11, 아니면 전투 비영 → 9,
// 모두 0 이면 로그(9, "ERROR: No transceiver on which to chat!\n") 뒤 zeroAddress()·localReceive(패킷). 항상 1 을 돌려준다. 이름 크기가 0x7fff 를, 본문이 0xffff 를 넘거나 수신자 배열이 부족하면 RangeError 다. 전역/배열은 원문 위치에서 현재 값을 다시 읽는다.
export function sendChatPacket(manager, recipients, body, hooks) {
    const memory = (token) => need(hooks.memory)(token); const global = (address) => need(hooks.read)(address);
    const name = need(hooks.currentName)(); const empty = text(name).length === 0; let found = false; const object = memory(recipients);
    // 현재 count 와 실제 배열에서 자기 subscriber 중복을 탐색한다.
    for (let index = 0; (index | 0) < (read(object, 0) | 0); index += 1) {
        if (read(memory(read(object, 8)), index * 4) === (global(SUBSCRIBER) >>> 0)) { found = true; break; }
    }
    if (!found) need(hooks.addRecipient)(recipients, global(SUBSCRIBER));
    let battleText = ''; let transformed = false;
    if (global(BATTLE)) {
        let flags = read(memory(manager), 16); if (read(memory(manager), 12)) flags |= 0x100;
        battleText = String(flags | 0); while (battleText.length < 3) battleText = `0${battleText}`; // 원문은 세 자리가 될 때까지 앞에 0 을 삽입한다.
        battleText += text(body); if (global(BATTLE)) transformed = true;
    }
    const actualName = empty ? NAMELESS : text(name); const nameSize = actualName.length + 1;
    let message = body; if (transformed) { message = new Uint8Array(battleText.length); for (let index = 0; index < battleText.length; index += 1) message[index] = battleText.charCodeAt(index) & 0xff; }
    const count = read(object, 0); const recipientsSize = count * 4;
    if (nameSize > 0x7fff || message.length > 0xffff || recipientsSize > memory(read(object, 8)).length) throw new RangeError('Chat 패킷의 실제 이름/본문/수신자 복사 범위 부족');
    const packet = new Uint8Array(15 + nameSize + message.length + recipientsSize).fill(0xff); // 원문 fill 의 미작성 +4 는 ff 이며 뒤쪽 실제 복사까지 안전하게 소유한다.
    packet[1] = global(0x52edf6) & 0xff; put(packet, 2, Math.max(global(0x544c5c) & 0xffff, 4), 2); packet[0] = 3; packet[5] = 1;
    put(packet, 6, global(SUBSCRIBER)); put(packet, 10, nameSize, 2); put(packet, 12, message.length, 2); put(packet, 14, count, 1);
    packet.set(message, 15 + nameSize);
    // 현재 이름 바이트(NUL 앞)를 그대로 복사한다.
    for (let index = 0; index < actualName.length; index += 1) packet[15 + index] = actualName.charCodeAt(index) & 0xff;
    packet[15 + actualName.length] = 0;
    const array = memory(read(object, 8)); packet.set(array.subarray(0, recipientsSize), 15 + nameSize + message.length);
    if (global(CHANNEL_ONE)) need(hooks.send)(1, packet);
    else if (global(METAMAP)) need(hooks.send)(11, packet);
    else if (global(BATTLE)) need(hooks.send)(9, packet);
    else { need(hooks.log)(9, 'ERROR: No transceiver on which to chat!\n'); need(hooks.zeroAddress)(); need(hooks.localReceive)(packet); }
    return 1;
}

// 0040a8d0: 송신이 비영일 때만 현재 이름 크기 WORD(+10)/본문 크기 WORD(+12)/수신자 수 BYTE(+14)로 전체 길이를 합산해 +2 WORD 에 쓴다(합 = 수신자×4+본문+이름+15). 이름 크기 0xffff 면 진단 0x81d, 수신자 수 0xff 면 0x81f 를 알리고 계속한다.
// bufferLen==ffff 의 도달 불가능한 원문 assertion 은 추가하지 않는다. 진단 장치는 도달할 때만 요구한다.
export function observeChatPacket(packet, sending, assertion) {
    if (!sending) return;
    if (read(packet, 10, 2) === 0xffff) need(assertion)(0x81d);
    if (read(packet, 14, 1) === 0xff) need(assertion)(0x81f);
    const length = read(packet, 14, 1) * 4 + read(packet, 12, 2) + read(packet, 10, 2) + 15; put(packet, 2, length, 2);
}
