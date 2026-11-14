// 원본 0042e540 의 장기 후보/접두/순환 상태를 실제 Chat 기록과 연결한다(채팅 입력의 이름 자동완성).
// (cpppj client/ChatNameCompletion.cpp/.h, 기준선 1f00a25a, 10.78 전용)
// 이름 자동완성은 Console 의 단어 선택 상태와 별도로 Client 수명에 산다(00548810..0054c89f 의 실제 BSS 배치를 보존). 원문 순서·빈 후보·중복·BYTE 잔류와 signed 인덱스 감김을 유지하고, 물리 상태 밖의 손상은 성공값으로 보정하지 않고 명시 경계(RangeError)로 남긴다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E436: RawMenuControls 선행 Chat 이름 자동완성 이식)

// 00548810 기준의 DWORD/배열 오프셋이다: 순환 인덱스(+0)·질의 길이(+4)·접두(+8)·후보 수(+88)·후보 배열(+90, 슬롯 128바이트)이고 상태 전체는 0x4090 바이트다.
const INDEX = 0; const LENGTH = 4; const PREFIX = 8; const COUNT = 0x88; const CANDIDATES = 0x90; const SLOT = 128; const STATE_BYTES = 0x4090;
// 번역 전 원시 채널 문자열이다(원문 00531768 의 NULL 앞 여덟 개, 빈 값/길이 128 이상은 건너뛴다). 현재 Chat 기록은 100행×48바이트(+44 가 Subscriber, +0 이름)다.
const CHANNELS = ['', '', '', 'BATTLE', '', 'EVERYONE', 'ALLIES', 'OBSERVER']; const RECORD_COUNT = 100; const RECORD_BYTES = 48; const RECORD_TOTAL = RECORD_COUNT * RECORD_BYTES; const SUBSCRIBER = 44;
// 후보 최대 수(슬롯 수)다.
const MAXIMUM = 128;

// 원문 BSS 배치 그대로의 자동완성 상태다(index/length/prefix/count/unused/candidates 의 원시 바이트).
export class ChatNameCompletionState {
    // 0x4090 바이트의 원시 상태다.
    bytes = new Uint8Array(STATE_BYTES);
}

// 비정렬 원문 DWORD 를 그대로 읽는다(범위 밖은 거부).
function read(bytes, offset) {
    if (offset + 4 > bytes.length) throw new RangeError('자동완성 DWORD 읽기 범위');
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}
// 원문 고정 DWORD 위치만 쓴다.
function write(state, offset, value) { value >>>= 0; for (let index = 0; index < 4; index += 1) state.bytes[offset + index] = (value >>> (index * 8)) & 255; }
// 슬롯 128 을 넘어 이어진 원문 C 문자열도 같은 물리에서 읽는다(NUL 이 없으면 거부).
function stringAt(bytes, offset) {
    if (offset >= bytes.length) throw new RangeError('자동완성 문자열 주소 범위');
    let end = offset; while (end < bytes.length && bytes[end] !== 0) end += 1;
    if (end === bytes.length) throw new RangeError('자동완성 문자열 NUL 범위');
    let text = ''; for (let index = offset; index < end; index += 1) text += String.fromCharCode(bytes[index]); return text;
}
// strcpy 의 종료 BYTE 까지 복사하며 번역을 슬롯 폭으로 잘라내지 않는다(첫 NUL 앞까지만).
function copy(state, offset, text) {
    const nul = text.indexOf('\0'); if (nul >= 0) text = text.slice(0, nul);
    if (offset >= state.bytes.length || text.length >= state.bytes.length - offset) throw new RangeError('자동완성 후보 복사 물리 범위');
    for (let index = 0; index < text.length; index += 1) state.bytes[offset + index] = text.charCodeAt(index) & 255;
    state.bytes[offset + text.length] = 0;
}
// 원문 C locale 의 ASCII 대소문자 변환이다.
const lower = (value) => (value >= 65 && value <= 90 ? value + 32 : value);
// strnicmp 처럼 첫 NUL 또는 명시 길이에서 멈춘다. 원문 인접 상태를 읽으며 NUL 이후에는 접근하지 않는다.
function same(state, candidate, length) {
    for (let index = 0; index < length; index += 1) {
        if (candidate + index >= state.bytes.length || PREFIX + index >= state.bytes.length) throw new RangeError('자동완성 비교 물리 범위');
        const a = lower(state.bytes[candidate + index]); const b = lower(state.bytes[PREFIX + index]);
        if (a !== b) return false; if (!a) return true;
    }
    return true;
}

// 이름 자동완성 한 단계다. query 가 null/undefined 면 저장된 후보의 다음/이전 순환이고 문자열이면(빈 문자열 포함) 새 질의로 후보를 다시 수집하고 length 바이트만 접두에 복사한다(나머지 접두는 남긴다).
// hooks = { translate(키) → 번역 바이트 문자열, records() → 현재 Chat 100×48 바이트, assertion(0x354) → 길이 127 이상일 때 원문 진단(복귀하면 복사를 계속) }. 새 질의일 때만 번역과 현재 100행을 요구한다. 범위를 두 번째 벗어났을 때만 undefined 를 반환한다.
// 사용: const name = completeChatName(state, 'ann', 3, false, hooks)
export function completeChatName(state, query, length, backward, hooks) {
    if (query === null || query === undefined) write(state, INDEX, (read(state.bytes, INDEX) + (backward ? 0xffffffff : 1)) >>> 0);
    else {
        let count = 0; // 현재 수집 수이며 이전 후보의 뒤쪽 BYTE 는 지우지 않는다.
        // 번역 전의 빈값/길이를 검사하고 번역 후 문자열을 그대로 복사한다.
        for (const channel of CHANNELS) {
            if (count >= MAXIMUM) break; if (channel.length === 0 || channel.length >= SLOT) continue;
            if (typeof hooks.translate !== 'function') throw new Error('자동완성 실제 번역 필요');
            copy(state, CANDIDATES + count * SLOT, hooks.translate(channel)); count += 1;
        }
        if (typeof hooks.records !== 'function') throw new Error('자동완성 현재 Chat 기록 필요');
        const records = hooks.records(); if (records.length !== RECORD_TOTAL) throw new RangeError('자동완성 실제 100행 필요');
        // 비영 +2c Subscriber 의 빈 이름도 원문처럼 포함한다.
        for (let index = 0; index < RECORD_COUNT; index += 1) {
            if (count >= MAXIMUM) break; if (!read(records, index * RECORD_BYTES + SUBSCRIBER)) continue;
            const name = stringAt(records, index * RECORD_BYTES); if (name.length < SLOT) { copy(state, CANDIDATES + count * SLOT, name); count += 1; }
        }
        write(state, COUNT, count);
        if (length > 126) { if (typeof hooks.assertion !== 'function') throw new Error('자동완성 원문 길이 진단 필요'); hooks.assertion(0x354); }
        const width = length >>> 0; if (width > state.bytes.length - PREFIX) throw new RangeError('자동완성 strncpy 물리 범위');
        const nul = query.indexOf('\0'); const text = nul < 0 ? query : query.slice(0, nul);
        // strncpy 의 NUL 패딩과 미작성 접두 꼬리를 구별한다.
        for (let index = 0; index < width; index += 1) state.bytes[PREFIX + index] = index < text.length ? text.charCodeAt(index) & 255 : 0;
        write(state, LENGTH, width); write(state, INDEX, 0);
    }
    let wrapped = false; // 원문은 범위를 두 번째 벗어났을 때만 NULL 을 반환한다.
    // 정/역방향의 첫 검색과 NULL 후보 순환은 같은 signed 인덱스를 사용한다.
    for (;;) {
        const index = read(state.bytes, INDEX) | 0; const count = read(state.bytes, COUNT) | 0;
        if (index < 0 || index >= count) {
            if (wrapped) return undefined; wrapped = true; write(state, INDEX, count);
            if (!backward) { write(state, INDEX, 0); continue; }
        } else if (same(state, CANDIDATES + index * SLOT, read(state.bytes, LENGTH))) return stringAt(state.bytes, CANDIDATES + index * SLOT);
        write(state, INDEX, (read(state.bytes, INDEX) + (backward ? 0xffffffff : 1)) >>> 0);
    }
}
