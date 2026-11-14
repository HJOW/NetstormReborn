// 원본 일반 채팅 본문의 채널/대상 아이콘·이름/스타일 조합과 say/wrapper 호출 순서를 옮긴다(원문 0042d640/0042dc30/0042f1e0, cpppj client/RawChatMessage.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 패킷 작성(RawChatPacket)과 분리된 부모다. manager 는 2712바이트 객체이고 본문 출력은 그 안의 +0x298(2048바이트)에 쓰인다. 원문 포인터는 호스트에서 같은 소유자의 불투명 토큰으로 표현하며 모든 물리 접근은 `hooks.memory(token)` 의 같은 Uint8Array 를 다시 빌려 실제 바이트에서 한다.
// 문자열은 "바이트 문자열"(문자 하나 = 바이트 하나)이다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: sayChatMessage(inputBytes, nameWork256, { current, subscriber }, { memory, currentName, nameForSubscriber, deliver, inputVisible, formatName?, stripSlots? }).
// (2026-11-14 WB04-E446: RawMenuControls 선행 Chat 본문 조합 이식)
import { stripChatFormatSlots } from './raw-chat-format.js';

// manager 2712바이트 안의 2048바이트 본문 출력 시작 오프셋과 0042d640 의 지역 대상 이름 버퍼 폭(256)이다.
export const CHAT_MESSAGE_OFFSET = 0x298; export const CHAT_MESSAGE_NAME_BYTES = 256;

// 선택된 경계만 요구하고 미복원 송신을 성공으로 바꾸지 않는다(C++ logic_error → Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('Chat 본문 조합의 실제 장치 미연결'); return hook; }

// 명시 물리 안의 첫 NUL 까지를 바이트 문자열로 읽는다(NUL 이 없으면 RangeError, C++ out_of_range).
function text(bytes) {
    const end = bytes.indexOf(0); if (end < 0) throw new RangeError('Chat 본문의 실제 NUL 물리 없음');
    let value = ''; for (let index = 0; index < end; index += 1) value += String.fromCharCode(bytes[index]);
    return value;
}

// 같은 토큰의 순수 뷰를 필요한 읽기/쓰기 위치에서 다시 빌린다.
const memoryOf = (hooks, token) => need(hooks.memory)(token);

// 실제 비정렬 DWORD 의 네 바이트를 읽는다(범위 밖이면 RangeError).
function read(bytes, at) {
    if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Chat 본문 DWORD 물리 범위');
    let value = 0; for (let index = 0; index < 4; index += 1) value += bytes[at + index] * 2 ** (8 * index); // 원문의 little endian 네 BYTE 다.
    return value;
}

// manager 안의 현재 출력 물리를 반환하고 원문 영역을 임의로 확장하지 않는다.
function output(hooks, manager) {
    const bytes = memoryOf(hooks, manager); if (bytes.length <= CHAT_MESSAGE_OFFSET) throw new RangeError('Chat 본문 출력 manager 물리 부족');
    return bytes.subarray(CHAT_MESSAGE_OFFSET);
}

// 입력 NUL 앞부분과 끝 NUL 만 순방향으로 복사하며 뒤쪽 흔적은 보존한다(목적지가 부족하면 RangeError).
function copy(destination, value) {
    const cut = value.indexOf('\0'); const body = cut < 0 ? value : value.slice(0, cut);
    if (destination.length <= body.length) throw new RangeError('Chat 본문 복사의 실제 물리 부족');
    // 같은 원문 순방향 문자 복사를 수행한다.
    for (let index = 0; index < body.length; index += 1) destination[index] = body.charCodeAt(index) & 0xff;
    destination[body.length] = 0;
}

// 실제 현재 출력 끝을 매번 찾은 뒤 같은 물리에 이어 쓴다.
function append(hooks, manager, value) { const bytes = output(hooks, manager); copy(bytes.subarray(text(bytes).length), value); }

// 0042d250 의 signed count 와 첫 비자기 subscriber 를 실제 대상 manager 에서 찾는다. 비교 기준 배열 토큰은 먼저 고정하고 반환할 때는 현재 배열 포인터를 다시 읽는다. 없으면 0.
function firstOther(recipients, subscriber, hooks) {
    if ((read(memoryOf(hooks, recipients), 0) | 0) <= 0) return 0;
    const buffer = read(memoryOf(hooks, recipients), 8); let index = 0;
    // 첫 비자기 항목에서 현재 배열 항목을 돌려준다.
    do {
        if (read(memoryOf(hooks, buffer), index * 4) !== (subscriber >>> 0)) return read(memoryOf(hooks, read(memoryOf(hooks, recipients), 8)), index * 4);
        index += 1;
    } while ((index | 0) < (read(memoryOf(hooks, recipients), 0) | 0));
    return 0;
}

// 0042d640 이 쓰는 세 고정 %s 서식만 NUL 포함으로 작성하며 이름 안의 %s 를 다시 해석하지 않는다. 그 밖의 서식은 Error 다.
export function writeChatNameFormat(destination, pattern, name, other) {
    const cut = (value) => { const at = value.indexOf('\0'); return at < 0 ? value : value.slice(0, at); };
    pattern = cut(pattern); name = cut(name); other = cut(other); let result;
    if (pattern === '%s> ') result = `${name}> `; else if (pattern === '%s ') result = `${name} `;
    else if (pattern === '%s (to %s)> ') result = `${name} (to ${other})> `; else throw new Error('Chat 이름의 미지원 원문 서식');
    copy(destination, result);
}

// 0042d640: 본문 빈 관문 → 현재 채널/대상 아이콘(back)과 현재 manager 의 back 서식 → 이름 → 현재 front/본문 → 슬롯 제거 → 전달 순서다. 이름 시작 위치는 조회 전 고정하되 front/본문은 하위 콜백 뒤 현재 값에서 다시 읽는다.
// manager +16 의 비트 8/0x40/0x80 은 아이콘 D36/D37/D39 를 이어 붙이고 0x20 은 D41 을 처음부터 덮어쓴다. +12 가 비영이면 대상 수 1 이면 D38, 아니면 D40 을 붙인다. 그 뒤 manager +0x14 의 back 서식 문자열을 붙인다.
// hooks = { memory(token), currentName() → 현재 이름(바이트 문자열, 빈 값은 ++Nameless++), nameForSubscriber(nameWork, subscriber) → 0/비영(조회 성공 여부가 아님), formatName?(출력, 서식, 이름, 상대) 기본 writeChatNameFormat,
//   stripSlots?(출력) 기본 stripChatFormatSlots, deliver(manager, recipients, bytes) → DWORD, inputVisible(0) }, globals = { current, subscriber }(읽는 시점의 현재 값).
export function formatChatMessage(manager, recipients, body, pattern, nameWork, globals, hooks) {
    if (text(body).length === 0) return 0;
    output(hooks, manager)[0] = 0; const count = read(memoryOf(hooks, recipients), 0);
    if (read(memoryOf(hooks, manager), 16) & 8) append(hooks, manager, '~[Iicon.D36]');
    if (read(memoryOf(hooks, manager), 16) & 0x40) append(hooks, manager, '~[Iicon.D37]');
    if (read(memoryOf(hooks, manager), 16) & 0x80) append(hooks, manager, '~[Iicon.D39]');
    if (read(memoryOf(hooks, manager), 16) & 0x20) copy(output(hooks, manager), '~[Iicon.D41]');
    if (read(memoryOf(hooks, manager), 12)) append(hooks, manager, count === 1 ? '~[Iicon.D38]' : '~[Iicon.D40]');
    append(hooks, manager, text(memoryOf(hooks, manager).subarray(0x14)));
    const nameAt = text(output(hooks, manager)).length; if (pattern === null || pattern === undefined) pattern = '%s> ';
    if (nameWork.length < CHAT_MESSAGE_NAME_BYTES) throw new RangeError('Chat 대상 이름의 실제256BYTE 필요'); nameWork[0] = 0;
    if (read(memoryOf(hooks, recipients), 12)) {
        const other = firstOther(recipients, globals.subscriber, hooks);
        if (other && !need(hooks.nameForSubscriber)(nameWork, other)) nameWork[0] = 0;
    }
    let name = need(hooks.currentName)(); { const at = name.indexOf('\0'); if (at >= 0) name = name.slice(0, at); } if (name.length === 0) name = '++Nameless++';
    const formatName = hooks.formatName === undefined ? writeChatNameFormat : hooks.formatName;
    need(formatName)(output(hooks, manager).subarray(nameAt), pattern, name, text(nameWork));
    if (!read(memoryOf(hooks, recipients), 12)) append(hooks, manager, text(memoryOf(hooks, manager).subarray(0x50)));
    append(hooks, manager, text(body)); const stripSlots = hooks.stripSlots === undefined ? stripChatFormatSlots : hooks.stripSlots; need(stripSlots)(output(hooks, manager));
    const bytes = output(hooks, manager); const length = text(bytes).length + 1; return need(hooks.deliver)(manager, recipients, bytes.subarray(0, length));
}

// 0042dc30: 선행 '!' 는 이름 뒤 공백 서식("%s "), 직접 대상(manager +12 비영)이면 "%s (to %s)> ", 그 밖에는 "%s> " 서식으로 같은 manager 를 대상/수신자로 전달한다. 입력이 비어 있으면 RangeError 다.
export function formatChatInput(manager, input, nameWork, globals, hooks) {
    if (input.length === 0) throw new RangeError('Chat wrapper의 실제 첫 입력 BYTE 없음');
    if (input[0] === 0x21) return formatChatMessage(manager, manager, input.subarray(1), '%s ', nameWork, globals, hooks);
    return formatChatMessage(manager, manager, input, read(memoryOf(hooks, manager), 12) ? '%s (to %s)> ' : '%s> ', nameWork, globals, hooks);
}

// 0042f1e0: 현재 manager 0 이면 아무것도 하지 않고, 입력이 비면 RangeError, 앞 공백이면 무시, 빈 본문(첫 BYTE 0)이면 입력줄 숨김(inputVisible(0))만 하며, 나머지는 wrapper 로 전달한다. 하위 전달값과 무관하게 항상 0 을 반환한다.
export function sayChatMessage(input, nameWork, globals, hooks) {
    if (!globals.current) return 0; if (input.length === 0) throw new RangeError('Chat say의 실제 첫 입력 BYTE 없음'); if (input[0] === 0x20) return 0;
    if (!input[0]) { need(hooks.inputVisible)(0); return 0; }
    formatChatInput(globals.current, input, nameWork, globals, hooks); return 0;
}
