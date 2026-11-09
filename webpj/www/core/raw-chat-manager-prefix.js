// Chat.cpp 의 실제 manager 생성/초기화(0042e000·0042e050)와 Chatgump.cpp 의 랭크 prefix(0042d620·0042f480·0042f540)를 같은 현재 전역에 적용하는 모듈이다(cpppj `o/RawChatManagerPrefix.cpp/.h`, 10.78 전용).
//  - constructChatManager(object, hooks)(0042e000): 별도 내부 0x80 할당 → 같은 object+8 에 토큰 쓰기 → 할당이 0 이면 진단(220) → 지정한 필드(+0, +0xc, +0x14, +0x50, +0x244, +4)만 쓰고 같은 토큰을 반환한다(내부 할당은 초기화하지 않음).
//  - initializeChatManager(globals, hooks)(0042e050): 새 0xa98 할당 → 현재 manager 에 대입(생성자 호출) → 명령 3개 등록("format"·"back"·"front") → 현재 subscriber 로 만든 설정 키 두 개를 읽음("<subscriber>chatFormat"·"<subscriber>chatFormat2")
//    → manager 가 없으면 진단(898) → 현재 manager 별 strncpy(+0x14, 60 / +0x50, 500)와 NUL 쓰기(+0x4f / +0x243).
//  - setChatManagerRankPrefix(manager, source)(0042d620): 같은 source 에서 NUL 포함 순방향 복사한다(+0x244). 의미상 256 절단/입력창 접두 변경을 추가하지 않는다.
//  - chatPrefixIcons(output, temporary, rank, hooks)(0042f480): temporary[0] = 0 → 랭크가 0 이상이면 최대값으로 clamp(매 비교마다 getter 를 다시 읽음) → 작은 아이콘 개수로 나눈 몫이 0 보다 크면 'C'(몫−1)·항상 'B'(나머지) 아이콘 서식을 덧붙임
//    → temporary 256 BYTE 를 DWORD 단위로 output 에 복사한다. 나눗셈 인자는 진단이 getter 를 바꿔도 캡처한 값을 쓴다.
//  - refreshChatManagerRankPrefix(globals, hooks, output, temporary)(0042f540): 현재 manager/CoreData 활성 관문 뒤 현재 Core+4 BYTE → formatter → 다시 현재 manager 의 setter.
//  - copyChatManagerFormat/appendChatRankIcon/bindChatManagerMemoryCopy: 실제 strncpy 의 NUL padding 과 고정 아이콘 서식 append, 누락된 CRT 경계만 같은 memory 에 연결하는 어댑터이다.
// 원문 직접 쓰기·현재값 재조회·호출 중단의 앞선 효과를 보존한다. 전체 Chat 전송/창 생성과 구별한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E229: Chat manager 랭크 prefix 이식으로 추가)

// 0042e050 의 실제 manager 할당 폭, 0042e000 의 별도 내부 할당 폭(초기화하지 않음), 랭크 문자열 위치(ChatWindow+b8 입력 접두와 별개), 0042f480 이 복사하는 NUL 뒤까지의 256 BYTE 이다. (2026-11-10 WB04-E229)
export const CHAT_MANAGER_BYTES = 0xa98;
export const CHAT_MANAGER_BUFFER_BYTES = 0x80;
export const CHAT_MANAGER_RANK_PREFIX_OFFSET = 0x244;
export const CHAT_RANK_OUTPUT_BYTES = 256;

// 원문 idiv 의 CPU fault(0 으로 나누기·INT_MIN/-1)를 표현하는 오류이다. 범위 밖 접근(RangeError)과 구별한다. (2026-11-10 WB04-E229)
export class ChatManagerDivideError extends Error {}

// 도달한 실제 장치만 요구하며 다른 원문 분기 장치를 선행 검사하지 않는다. 누락은 TypeError 이다. (2026-11-10 WB04-E229)
function need(fn) {
    if (typeof fn !== 'function') throw new TypeError('Chatmanager 실제 장치 누락');
    return fn;
}

// 원문 한 store 의 전체 폭을 확인한 뒤 little endian 필드만 수정한다. 인접 미작성 BYTE 는 보존한다. (2026-11-10 WB04-E229)
function write(memory, offset, value, width = 4) {
    if (offset > memory.length || width > memory.length - offset) throw new RangeError('Chatmanager store 물리 범위');
    for (let byte = 0; byte < width; byte += 1) memory[offset + byte] = (value >>> (byte * 8)) & 0xff;
}

// 각 사용 위치에서 토큰의 같은 실제 할당을 빌린다. 순수 memory 조회는 주소/전역을 바꾸지 않아야 한다. (2026-11-10 WB04-E229)
const memoryOf = (hooks, token) => need(hooks.memory)(token);

// 원문 지역 C문자열만 읽으며 source 의 첫 NUL 뒤 문자는 서식/복사에 쓰지 않는다. (2026-11-10 WB04-E229)
function cString(source) {
    const end = source.indexOf('\0');
    return end === -1 ? source : source.slice(0, end);
}

// 같은 현재 subscriber 의 signed %d 와 고정 suffix 를 지역 문자열로 작성한다. (2026-11-10 WB04-E229)
const key = (subscriber, suffix) => `${subscriber | 0}${suffix}`;

// 실제 CRT 의 고정 폭 복사와 NUL padding 을 같은 대상 물리에 적용한다. count 가 대상보다 크면 쓰기 전에 RangeError 이다. (2026-11-10 WB04-E229)
export function copyChatManagerFormat(destination, source, count) {
    if (count > destination.length) throw new RangeError('Chatmanager strncpy 물리 범위');
    const text = cString(source);
    for (let index = 0; index < count; index += 1) destination[index] = index < text.length ? text.charCodeAt(index) & 0xff : 0; // 원문 strncpy 의 NUL padding 까지 실제 작성한다.
}

// 원문 고정 그림 서식("~[Iicon.<kind><value>]")을 현재 C문자열 끝에 NUL 포함 append 한다. NUL 이 없거나 물리가 부족하면 RangeError 이다. (2026-11-10 WB04-E229)
export function appendChatRankIcon(destination, kind, value) {
    const end = destination.indexOf(0);
    if (end === -1) throw new RangeError('Chat 랭크 append 의 NUL 없음');
    const text = `~[Iicon.${kind}${value | 0}]`; // 고정 서식의 signed 십진수이다.
    if (text.length + 1 > destination.length - end) throw new RangeError('Chat 랭크 append 물리 범위');
    for (let index = 0; index < text.length; index += 1) destination[end + index] = text.charCodeAt(index); // 기존 문자열 뒤에만 작성한다.
    destination[end + text.length] = 0;
}

// 실제 메모리를 빌리는 누락 CRT 경계만 채우고 사용자 callback 은 그대로 보존한다(새 hooks 객체를 돌려준다). (2026-11-10 WB04-E229)
export function bindChatManagerMemoryCopy(hooks) {
    const bound = { ...hooks };
    if (typeof bound.copyFormat !== 'function') {
        const memory = hooks.memory;
        bound.copyFormat = (token, offset, text, count) => {
            const bytes = need(memory)(token);
            if (offset > bytes.length) throw new RangeError('Chatmanager CRT target 물리 범위');
            copyChatManagerFormat(bytes.subarray(offset), text, count);
        }; // 호출자는 memory 를 소유한 실제 할당 장치보다 오래 이 hook 를 보관하지 않는다.
    }
    return bound;
}

// 0042e000: sparse 쓰기만 수행하고 별도 내부 할당은 초기화하지 않는다. 반환은 같은 object 토큰이다. (2026-11-10 WB04-E229)
export function constructChatManager(object, hooks) {
    const internal = need(hooks.allocate)(CHAT_MANAGER_BUFFER_BYTES);
    write(memoryOf(hooks, object), 8, internal);
    if (!internal) need(hooks.assertion)(0xdc);
    write(memoryOf(hooks, object), 0, 0);
    write(memoryOf(hooks, object), 0xc, 0);
    write(memoryOf(hooks, object), 0x14, 0, 1);
    write(memoryOf(hooks, object), 0x50, 0, 1);
    write(memoryOf(hooks, object), CHAT_MANAGER_RANK_PREFIX_OFFSET, 0, 1);
    write(memoryOf(hooks, object), 4, 0x20);
    return object;
}

// 0042e050: 현재 manager/Subscriber 를 하위 복귀 뒤 원문 위치에서 재조회한다.
//  globals = { current: { value }(00548714 현재 manager 토큰), subscriber: () => n(00540c74), coreDataActive: () => n(00595348) },
//  hooks = { allocate(n) → 토큰, memory(token) → Uint8Array, assertion(line), registerCommand(name, callback, flags), readConfig(key) → string|null, copyFormat(token, offset, text, count),
//            maximumRank(), minorCount(), appendIcon(span, kind, value)(기본 appendChatRankIcon), core() → Uint8Array }. (2026-11-10 WB04-E229)
export function initializeChatManager(globals, hooks) {
    const object = need(hooks.allocate)(CHAT_MANAGER_BYTES);
    globals.current.value = object ? constructChatManager(object, hooks) : 0;
    need(hooks.registerCommand)('format', 0x42ddf0, 1);
    need(hooks.registerCommand)('back', 0x42ddf0, 1);
    need(hooks.registerCommand)('front', 0x42def0, 1);
    const front = need(hooks.readConfig)(key(globals.subscriber(), 'chatFormat')); // 첫 지역 복사는 두 번째 Config callback 뒤에도 같은 내용을 유지한다.
    const back = need(hooks.readConfig)(key(globals.subscriber(), 'chatFormat2'));
    if (!globals.current.value) need(hooks.assertion)(0x382);
    const first = globals.current.value;
    need(hooks.copyFormat)(first, 0x14, front != null ? cString(front) : '', 60);
    write(memoryOf(hooks, first), 0x4f, 0, 1);
    const second = globals.current.value;
    need(hooks.copyFormat)(second, 0x50, back != null ? cString(back) : '', 500);
    write(memoryOf(hooks, second), 0x243, 0, 1);
}

// 0042d620: 순방향 겹침 복사를 물리 범위 안에서 그대로 수행한다. source 가 manager 의 일부 뷰여도 매 BYTE 현재 값을 읽는다. (2026-11-10 WB04-E229)
export function setChatManagerRankPrefix(manager, source) {
    for (let index = 0; ; index += 1) { // 원문 source BYTE 를 읽고 같은 순서로 써서 겹치는 버퍼의 효과도 보존한다.
        if (index >= source.length) throw new RangeError('Chat 랭크 prefix source 물리 범위');
        const value = source[index];
        write(manager, CHAT_MANAGER_RANK_PREFIX_OFFSET + index, value, 1);
        if (!value) return;
    }
}

// 0042f480: signed clamp·캡처 divisor·전체 256 BYTE 출력 순서를 수행한다. output/temporary 는 각각 실제 최소 256 BYTE 여야 하며 미정 temporary 를 임의로 0 으로 채우지 않는다.
// 반환은 output 의 앞 256 BYTE 뷰이다. (2026-11-10 WB04-E229)
export function chatPrefixIcons(output, temporary, rank, hooks) {
    if (temporary.length < CHAT_RANK_OUTPUT_BYTES) throw new RangeError('Chat 랭크 temporary256 물리 범위');
    temporary[0] = 0;
    let value = rank | 0;
    if (value >= 0) {
        const first = need(hooks.maximumRank)();
        const compared = first < value ? need(hooks.maximumRank)() : value;
        if (compared < 0) value = 0;
        else if (need(hooks.maximumRank)() < value) value = need(hooks.maximumRank)();
        const minor = need(hooks.minorCount)();
        if (minor <= 0) need(hooks.assertion)(0x9aa);
        if (minor === 0 || (value === -2147483648 && minor === -1)) throw new ChatManagerDivideError('Chat 랭크 원문 idiv fault');
        const quotient = Math.trunc(value / minor); // 진단이 현재 getter 를 바꿔도 캡처한 divisor 를 다시 읽지 않는다.
        const remainder = value % minor;
        const append = hooks.appendIcon ?? appendChatRankIcon;
        if (quotient > 0) need(append)(temporary.subarray(0, CHAT_RANK_OUTPUT_BYTES), 'C', quotient - 1);
        need(append)(temporary.subarray(0, CHAT_RANK_OUTPUT_BYTES), 'B', remainder);
    }
    for (let offset = 0; offset < CHAT_RANK_OUTPUT_BYTES; offset += 4) { // rep movsd 의 순방향 DWORD 단위 읽기/쓰기를 보존한다.
        if (offset + 4 > output.length) throw new RangeError('Chat 랭크 output256 물리 범위');
        const word = [temporary[offset], temporary[offset + 1], temporary[offset + 2], temporary[offset + 3]];
        for (let byte = 0; byte < 4; byte += 1) output[offset + byte] = word[byte];
    }
    return output.subarray(0, CHAT_RANK_OUTPUT_BYTES);
}

// 0042f540: 실제 자연 관문(현재 manager 와 CoreData 활성) 뒤 현재 Core+4 BYTE 로 formatter 를 돌리고 다시 읽은 현재 manager 에 setter 를 적용한다. (2026-11-10 WB04-E229)
export function refreshChatManagerRankPrefix(globals, hooks, output, temporary) {
    if (!globals.current.value || !globals.coreDataActive()) return;
    const core = need(hooks.core)();
    if (core.length < 5) throw new RangeError('Chat 랭크 현재 CoreData+4 물리 범위');
    const text = chatPrefixIcons(output, temporary, core[4], hooks);
    setChatManagerRankPrefix(memoryOf(hooks, globals.current.value), text);
}
