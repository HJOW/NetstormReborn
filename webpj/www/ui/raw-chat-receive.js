// 원본 0042ed30/0042f0e0 의 일반 채팅 수신·필터·로그 문맥과 선택 UI 효과를 옮긴다(cpppj client/RawChatReceive.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 순수 조회(`hooks.read`) 외의 효과(알림 깨움·소리·본문 추가·표시·로그·뉴스)는 원문에 도달한 위치에서만 실행하며 숨겨지는 메시지도 열 개 옵션을 원문 순서대로 모두 읽는다. 문자열 변환과 현재값 재조회, 실제 8 DWORD 글자 문맥을 원문 순서로 유지한다.
// 패킷은 Uint8Array 이고 본문은 바이트 문자열로 전달한다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: dispatchChatPacket(packet, { read, ignored, config, owner, wake, sound, append, show, log, setNewsTime, news }); receiveChatPacket(chat, packet, hooks).
// (2026-11-14 WB04-E446: RawMenuControls 선행 Chat 수신 이식)

// 원문 옵션 키(오타 포함)를 그대로 조회하는 열 개의 이름이다.
const OPTIONS = ['ShowBattle', 'ShowEveryone', 'ShowAllies', 'ShowObserver', 'ShowWisper', 'ShowMyAllies', 'ShowMyEmenys', 'ShowMyNeutrals', 'ShowChatWindow', 'PlaySoundMessage'];

// 미선택 하위를 미리 요구하지 않으며 실제 장치 누락은 해당 위치에서 중단한다(C++ logic_error → Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('Chat 수신의 선택된 실제 장치 미연결'); return hook; }

// 실제 물리 안의 BYTE/WORD/DWORD 를 낮은 BYTE 부터 읽는다(지정 폭 밖은 읽지 않고 범위 밖은 RangeError).
function read(bytes, at, width = 4) {
    if (at > bytes.length || width > bytes.length - at) throw new RangeError('Chat 수신 필드의 실제 물리 부족');
    let value = 0; for (let index = 0; index < width; index += 1) value += bytes[at + index] * 2 ** (8 * index);
    return value;
}

// 16비트 부호 있는 값으로 바꾼다(nameLen 은 원문 주소 getter 의 signed WORD 다).
const signed16 = (value) => (value << 16) >> 16;

// 본문 시작 = 15 + signed 이름 길이이며 음수/패킷 밖의 호스트 접근은 거부한다(RangeError).
function bodyOffset(packet) {
    const offset = 15 + signed16(read(packet, 10, 2));
    if (offset < 0 || offset > packet.length) throw new RangeError('Chat 수신 본문의 실제 signed 주소 범위');
    return offset;
}

// 지정 위치에서 첫 NUL 앞 문자열을 같은 현재 패킷에서 바이트 문자열로 읽는다(시작 범위 밖/NUL 없음은 RangeError).
function text(bytes, at) {
    if (at > bytes.length) throw new RangeError('Chat 수신 문자열 시작 범위');
    const end = bytes.indexOf(0, at); if (end < 0) throw new RangeError('Chat 수신 문자열의 실제 NUL 없음');
    let value = ''; for (let index = at; index < end; index += 1) value += String.fromCharCode(bytes[index]);
    return value;
}

// 004248a0 이 쓰는 같은 8 DWORD 문맥이다: 글자꼴 5·현재 색·기본 두 1.
const context = (color) => [0, 5, color >>> 0, 0, 0, 0, 1, 1];

// 0042ed30: 일반 수신의 무시/채널/관계 관문과 UI/소리 호출의 현재값 읽기를 분리한다. 먼저 마지막 본문 BYTE 에 NUL 을 쓰고 무시/필터 뒤에도 그 효과를 유지한다. 실제 물리 밖 접근은 RangeError 다.
// hooks = { read(주소) → 현재 모드/ID/채널 마스크/관계/글자색/Chat 토큰의 원문 슬롯, ignored(subscriber) → 0042e300 무시 목록, config(키) → 옵션, owner(subscriber) → 00484550 현재 Identity(실패도 0), wake(token), sound(이름),
//   append(chat, 본문, 8 DWORD 문맥), show(), log(수준, 문자열), setNewsTime(스탬프), news(문자열, 400, 30) } 이다.
export function receiveChatPacket(chat, packet, hooks) {
    const global = (address) => need(hooks.read)(address) >>> 0;
    if (read(packet, 0, 1) > 3) { need(hooks.log)(29, 'Received new format chat packet.\n'); return; }
    if ((read(packet, 5, 1) & 15) !== 1) { need(hooks.log)(29, 'Received non-ASCII chat packet.\n'); return; }
    const body = bodyOffset(packet); const length = read(packet, 12, 2);
    if (body + length === 0 || body + length > packet.length) throw new RangeError('Chat 수신 마지막 본문 BYTE의 실제 물리 부족'); packet[body + length - 1] = 0;
    // 시스템 뉴스: 상위 비트가 켜지고 발신자 0 이면 뉴스 장치(400, 30)로 보낸다. 이름이 있고 첫 BYTE 가 1 이면 시각을 현재 뉴스 시각과 비교한다.
    if ((read(packet, 5, 1) & 0x80) && !read(packet, 6)) {
        if (signed16(read(packet, 10, 2)) > 1 && read(packet, 15, 1) === 1) {
            const stamp = read(packet, 19); if (stamp <= global(0x548748)) return; need(hooks.setNewsTime)(stamp);
        }
        need(hooks.news)(text(packet, bodyOffset(packet)), 400, 30); return;
    }
    if (need(hooks.ignored)(read(packet, 6))) return;
    if (!global(0x594fbc)) {
        need(hooks.wake)(global(0x5ca92c));
        if (read(packet, 6) !== global(0x540c74) && !global(0x594fc8)) need(hooks.sound)('newChatMessage.wav');
        const value = context(global(0x5b5dd8)); need(hooks.append)(chat, text(packet, bodyOffset(packet)), value); need(hooks.show)(); return;
    }
    const options = [];
    // 숨기는 메시지도 열 개 옵션을 모두 원문 순서대로 읽는다.
    for (let index = 0; index < OPTIONS.length; index += 1) options.push(need(hooks.config)(OPTIONS[index]) | 0);
    const mask = global(0x548710); let visible = Boolean(((mask & 8) && options[0]) || ((mask & 32) && options[1]) || ((mask & 64) && options[2]) || ((mask & 128) && options[3]) || ((mask & 256) && options[4]));
    if (visible) {
        const owner = need(hooks.owner)(read(packet, 6)) >>> 0; const relation = (0x595200 + ((owner + global(0x540c70) * 9) >>> 0) * 4) >>> 0;
        if (!global(relation)) visible = options[6] !== 0;
        else if (options[5]) { if (!global(relation)) visible = options[6] !== 0; } else visible = false;
        if (visible && global((0x52f81e + owner) >>> 0) && !options[7]) visible = false;
    }
    let value = text(packet, bodyOffset(packet)); // 비표시 메시지도 현재 원문 문자열 지역을 구성한다.
    if (global(0x594fbc)) { if (value.length < 3) throw new RangeError('Chat 수신의 실제 세자리 접두 물리 부족'); value = `${value.slice(3)}\n`; }
    if (visible) {
        need(hooks.wake)(global(0x5ca92c));
        if (read(packet, 6) !== global(0x540c74) && !global(0x594fc8) && options[9]) need(hooks.sound)('newChatMessage.wav');
        const fields = context(global(0x5b5dd8)); need(hooks.append)(chat, value, fields); if (options[8]) need(hooks.show)();
    }
}

// 0042f0e0: 이 시점의 현재 Chat(00548740)이 0 이면 패킷/다른 장치를 건드리지 않고 조용히 종료하고, 아니면 같은 수신 부모를 실행한다.
export function dispatchChatPacket(packet, hooks) {
    const chat = need(hooks.read)(0x548740) >>> 0; if (chat) receiveChatPacket(chat, packet, hooks);
}
