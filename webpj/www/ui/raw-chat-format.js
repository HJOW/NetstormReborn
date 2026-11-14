// 원본 format/back/front 명령(0042ddf0/0042def0, 슬롯 제거 0042d160)의 고정 스타일 검색·공통 캡처·manager/Config 쓰기를 옮긴다(cpppj client/RawChatFormat.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 범용 정규식 대신 원문 00504a00 의 한 고정 패턴 `(~[^~ CR TAB LF space]+)+` 만 같은 캡처/부분 쓰기로 실행한다(VT/FF·비ASCII 는 스타일 글자로 허용). 외부 callback 뒤 원문이 쓰는 시점에 같은 현재 슬롯을 다시 읽는다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: applyChatFormat(false, { current, subscriber }, '~1~2x', capture(Uint8Array 81+), { memory, assertion, copyFormat, writeConfig, illegalFormat }); stripChatFormatSlots(bytes).
// (2026-11-14 WB04-E442: RawMenuControls 선행 Chat 서식 명령 이식)

// 00495a10 의 최대 80바이트와 끝 NUL 이 접근하는 공통 캡처의 앞부분 바이트 수다.
export const CHAT_FORMAT_CAPTURE_BYTES = 81;

// 실제로 선택한 장치를 빈 성공으로 바꾸지 않는다(logic_error→Error).
function need(callback) { if (typeof callback !== 'function') throw new Error('Chat 서식 명령의 실제 장치 미연결'); return callback; }
// Regexp 와 CRT 가 쓰는 원문 C 문자열의 첫 NUL 앞부분만 바이트 문자열로 빌린다. NUL 이 없으면 RangeError 다.
function text(bytes) {
    const end = bytes.indexOf(0);
    if (end < 0) throw new RangeError('Chat 서식의 실제 NUL 물리 없음');
    let result = ''; for (let index = 0; index < end; index += 1) result += String.fromCharCode(bytes[index]); return result;
}
// 원문 부정 문자 클래스는 ~ 와 CR/TAB/LF/space 만 제외하고 VT/FF·비ASCII 는 허용한다.
const styleByte = (value) => value !== '~' && value !== '\r' && value !== '\t' && value !== '\n' && value !== ' ';

// (~[^…]+)+ 의 첫 부분 검색과 greedy 연속 그룹 0 을 같은 원문 바이트에서 찾는다. 시작할 수 있는 ~ 부터 시도하고 한 글자 이상인 스타일 그룹만 연속으로 취하며, 일치가 없으면 null 이다.
function match(source) {
    const nul = source.indexOf('\0'); const value = nul < 0 ? source : source.slice(0, nul);
    // 먼저 시작할 수 있는 ~ 부터 원문 부분 검색을 시도한다.
    for (let first = 0; first < value.length; first += 1) {
        if (value[first] !== '~') continue;
        let cursor = first; let end = first;
        // 한 글자 이상인 스타일 그룹만 연속 취한다.
        while (cursor < value.length && value[cursor] === '~' && cursor + 1 < value.length && styleByte(value[cursor + 1])) {
            cursor += 1;
            // 해당 그룹의 부정 문자 클래스를 끝까지 읽는다.
            while (cursor < value.length && styleByte(value[cursor])) cursor += 1;
            end = cursor;
        }
        if (end !== first) return value.slice(first, end);
    }
    return null;
}

// 0042d160: `~0`~`~6` 두 바이트를 순방향 제자리 제거한다. 제거된 위치를 다시 검사하고(연속 `~0~1` 도 제거) 끝 NUL 까지 앞으로 복사하며 새 NUL 뒤의 원래 꼬리는 지우지 않는다. 물리 범위를 넘으면 RangeError 다.
export function stripChatFormatSlots(bytes) {
    let cursor = 0;
    // 현재 NUL 까지 매번 실제 바이트를 읽는다.
    for (;;) {
        if (cursor >= bytes.length) throw new RangeError('Chat 슬롯 제거의 실제 물리 범위');
        if (!bytes[cursor]) return;
        if (bytes[cursor] === 0x7e && cursor + 1 < bytes.length && bytes[cursor + 1] >= 0x30 && bytes[cursor + 1] <= 0x36) {
            let source = cursor + 2; let destination = cursor;
            // 원문은 한 바이트씩 NUL 을 포함해 앞으로 옮기며 기존 꼬리를 보존한다.
            for (;;) {
                if (source >= bytes.length) throw new RangeError('Chat 슬롯 제거 복사의 실제 NUL 없음');
                const value = bytes[source]; source += 1; bytes[destination] = value; destination += 1;
                if (!value) break;
            }
        } else cursor += 1;
    }
}

// 0042ddf0(back)/0042def0(front): 현재 manager 가 0 이면 assertion(back 157/front 184) 뒤 계속 → 고정 패턴 검색(없으면 illegalFormat(입력 전체, 첫 NUL 앞) 뒤 0) → 최대 80바이트를 공통 캡처에 쓰고 NUL → 슬롯 제거(stripSlots, 기본 stripChatFormatSlots)
// → 현재 manager 복사(copyFormat(manager, back 0x14/front 0x50, 캡처 문자열, back 60/front 500)) → 그 manager 의 강제 NUL(back 0x4f/front 0x243, memory(manager) 범위 밖이면 RangeError) → 현재 subscriber ID 키(`ID`+chatFormat/chatFormat2)로 writeConfig 순서이고 항상 0 을 돌려준다.
// globals = { current(00548714 현재 manager 토큰), subscriber(00540c74 현재 signed 설정 키 ID) }, capture 는 81바이트 이상이어야 하며 앞 부분만 쓰고 뒤는 보존한다.
export function applyChatFormat(front, globals, input, capture, hooks) {
    if (!globals.current) need(hooks.assertion)(front ? 184 : 157);
    const found = match(input);
    if (!found) { const nul = input.indexOf('\0'); need(hooks.illegalFormat)(nul < 0 ? input : input.slice(0, nul)); return 0; }
    if (capture.length < CHAT_FORMAT_CAPTURE_BYTES) throw new RangeError('Chat 서식 캡처의 실제81BYTE 필요');
    const length = Math.min(found.length, 80);
    // 실제 그룹 0 의 최대 80바이트만 순방향으로 쓰고 뒤는 보존한다.
    for (let index = 0; index < length; index += 1) capture[index] = found.charCodeAt(index) & 0xff;
    capture[length] = 0;
    need(hooks.stripSlots === undefined ? stripChatFormatSlots : hooks.stripSlots)(capture);
    const manager = globals.current;
    need(hooks.copyFormat)(manager, front ? 0x50 : 0x14, text(capture), front ? 500 : 60);
    const bytes = need(hooks.memory)(manager); const terminal = front ? 0x243 : 0x4f;
    if (terminal >= bytes.length) throw new RangeError('Chat 서식 강제 NUL의 실제 manager 범위');
    bytes[terminal] = 0;
    const key = `${globals.subscriber | 0}${front ? 'chatFormat2' : 'chatFormat'}`;
    need(hooks.writeConfig)(key, text(capture));
    return 0;
}
