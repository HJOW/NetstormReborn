// 원본 Winmain 의 점 명령 등록 0043f660 을 실제 인접 표의 바이트에 실행한다(cpppj client/RawDotCommandRegister.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// state 는 실제 2568(0xa08)바이트 인접 표다: 이름 64칸×32바이트(0x000..0x7ff), 함수 식별자 64×DWORD(0x800), 표시 포인터 65×DWORD(0x900), 개수 DWORD(0xa04). count/함수/표시 포인터를 별도 사본으로 분리하지 않는다.
// 등록 부모는 이름 복사 callback 뒤에 현재 count 를 단 한 번 다시 읽어 끝까지 보관한다(복사 장치가 count 를 바꿔도 같은 캡처 값으로 증가/종단 효과를 계속한다). 진입 관문은 signed count < 64 이며
// 음수 초기 count 의 원시 주소 감김은 host 지원 밖이라 RangeError 로 명시 거부한다. 함수 식별자는 호스트에서 실행하지 않는 원문 근거 값이다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const state = new Uint8Array(DOT_COMMAND_STATE_BYTES); registerDotCommand(state, nameBytes, 0x42f1e0, 1) → 1(등록)/0(가득 참).
// (2026-11-14 WB04-E443: RawMenuControls 선행 점 명령 등록 이식)

// 인접 물리 폭(이름 2048 + 함수 256 + 표시 260 + count 4 바이트)과 이름표/빈 문자열 주소 식별자다.
export const DOT_COMMAND_STATE_BYTES = 0xa08;
export const DOT_COMMAND_NAMES_TOKEN = 0x00531b30;
export const DOT_COMMAND_EMPTY_TOKEN = 0x00503c8b;
// 같은 전역 표 시작에 대한 세 실제 오프셋이다(함수 표/표시 표/개수).
const CALLBACK_OFFSET = 0x800; const VISIBLE_OFFSET = 0x900; const COUNT_OFFSET = 0xa04;

// 원문 DWORD 접근이 실제 할당을 벗어나는 경우만 명시적으로 중단한다(RangeError).
function physical(state, offset, count) {
    if (offset > state.length || count > state.length - offset) throw new RangeError('점 명령 등록의 실제 인접 표 범위');
    return state.subarray(offset, offset + count);
}
// host 정렬·alias 규칙에 기대지 않고 원문의 같은 little endian DWORD 를 읽는다.
function read(state, offset) {
    const bytes = physical(state, offset >>> 0, 4); let value = 0;
    // 낮은 바이트부터 원래 DWORD 를 구성한다.
    for (let index = 0; index < 4; index += 1) value |= bytes[index] << (index * 8);
    return value >>> 0;
}
// CPU DWORD store 와 같은 물리 범위를 먼저 확인한 뒤 같은 네 바이트를 쓴다(인접 필드와 겹치는 실제 쓰기도 자동 분리하지 않는다).
function write(state, offset, value) {
    const bytes = physical(state, offset >>> 0, 4);
    for (let index = 0; index < 4; index += 1) bytes[index] = (value >>> (index * 8)) & 0xff;
}
// 이름 입력(바이트 배열 또는 바이트 문자열)을 바이트 배열로 바꾼다.
const bytesOf = (name) => (typeof name === 'string' ? Uint8Array.from(name, (character) => character.charCodeAt(0) & 0xff) : name);

// CRT strncpy 의 정상 NUL padding 을 물리 span 안에서 수행한다: count 바이트를 순방향으로 복사하되 NUL 을 한 번 읽은 뒤에는 source 를 더 읽지 않고 0 으로 채운다.
// 목적 물리가 count 보다 작거나 NUL 전에 원본 물리가 끝나면 RangeError 다. 이름 31바이트와 NUL 뒤 padding 은 CRT 하위이고 마지막 32번째 바이트는 등록 부모가 쓴다.
export function copyDotCommandName(destination, source, count) {
    source = bytesOf(source);
    if (destination.length < count) throw new RangeError('점 명령 이름 복사의 목적 물리 범위');
    let ended = false;
    // 원문 strncpy 의 count 만큼 순방향으로 복사하거나 padding 한다.
    for (let index = 0; index < count; index += 1) {
        if (!ended && index >= source.length) throw new RangeError('점 명령 이름 복사의 원본 물리 범위');
        const value = ended ? 0 : source[index]; ended = ended || value === 0; destination[index] = value;
    }
}

// 0043f660: 진입 count(signed)가 64 이상이면 0(등록 안 함), 음수면 RangeError 다. 이름은 진입 count×32 위치에 31바이트 복사(hooks.copy, 기본 copyDotCommandName; 명시 null 은 Error)하고,
// 복사 뒤 현재 count 를 한 번 캡처해 함수 식별자(callback)를 함수 표에, 32번째 이름 바이트를 0 으로, 표시 표에 flag 비영이면 이름표 주소+칸×32 아니면 빈 문자열 주소를 쓰며,
// 캡처한 count+1 을 현재 메모리가 아닌 보관 값으로 저장하고 다음 표시 칸을 0 으로 종단한 뒤 1 을 돌려준다. 모든 DWORD 접근은 실제 인접 물리를 넘으면 RangeError 다.
export function registerDotCommand(state, name, callback, flag, hooks = {}) {
    const initial = read(state, COUNT_OFFSET); const signedCount = initial | 0;
    if (signedCount >= 64) return 0;
    if (signedCount < 0) throw new RangeError('점 명령 음수 초기count의 원시 주소는 지원 물리 계약 밖');
    const target = Math.imul(initial, 32) >>> 0; const copy = hooks.copy === undefined ? copyDotCommandName : hooks.copy;
    if (typeof copy !== 'function') throw new Error('점 명령 등록의 실제 strncpy 장치 미연결');
    copy(physical(state, target, 31), name, 31);
    // 0043f683 의 현재 count 를 단 한 번 캡처하며 인접 store 로 덮여도 다시 읽지 않는다.
    const selected = read(state, COUNT_OFFSET);
    write(state, (CALLBACK_OFFSET + Math.imul(selected, 4)) >>> 0, callback);
    physical(state, (Math.imul(selected, 32) + 31) >>> 0, 1)[0] = 0;
    write(state, (VISIBLE_OFFSET + Math.imul(selected, 4)) >>> 0, flag ? (DOT_COMMAND_NAMES_TOKEN + Math.imul(selected, 32)) >>> 0 : DOT_COMMAND_EMPTY_TOKEN);
    // 원문은 현재 메모리가 아닌 보관 ECX 를 증가한다.
    const updated = (selected + 1) >>> 0; write(state, COUNT_OFFSET, updated);
    write(state, (VISIBLE_OFFSET + Math.imul(updated, 4)) >>> 0, 0);
    return 1;
}
