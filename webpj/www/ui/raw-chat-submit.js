// 원본 0042da00 의 대상/본문 해석과 0042d1b0 의 실제 수신자 배열 추가를 옮긴다(cpppj client/RawChatSubmit.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 같은 manager 물리와 현재 Identity/Chat 행을 빌리는 채팅 제출 부품이다. 입력의 고정 패턴(`^[CR LF TAB SPACE]*([^:]*)($|:[공백]*(.*))`)을 두 인접 2048바이트 캡처(본문·대상 접두)로 나누고, 접두의 각 이름을 Player/Chat 식별자 또는 여덟 채널 키로 해석해 수신자 배열과
// 채널 마스크를 갱신한 뒤 본문을 분배(0043f6f0)하거나 빈 본문이면 입력줄을 숨긴다. 전체 Regexp/네트워크 전송을 대신하지 않는다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: submitChatInput(manager, inputBytes, identity60, work4097, { memory, allocate, release, assertion, resolve, translate, records, inputVisible, dispatch }); addChatRecipient(manager, subscriber, hooks).
// (2026-11-14 WB04-E446: RawMenuControls 선행 Chat 제출 이식)
import { RawPlayerIdentity } from '../core/raw-player-identity.js';

// 인접한 본문 2048 / 접두 2048 과 마지막 캡처 NUL 의 실제 접근 폭(4097)이다.
export const CHAT_SUBMIT_WORK_BYTES = 4097;
// 대상/본문 패턴에 명시된 네 공백(CR LF TAB SPACE)이며 VT/FF 는 제외한다.
const SPACE = '\r\n\t ';
// 00531768 의 첫 NULL 앞 여덟 실제 키다(빈 키도 건너뛰지 않는다).
const CHANNELS = ['', '', '', 'BATTLE', '', 'EVERYONE', 'ALLIES', 'OBSERVER'];
// 캡처 한 칸의 최대 복사 길이(2048)와 접두 안 한 이름의 최대 복사 길이(40), Chat 기록 100행×48바이트, 직접 대상 수신자 마스크(0x20)다.
const CAPTURE_BYTES = 2048; const NAME_BYTES = 40; const RECORD_COUNT = 100; const RECORD_STRIDE = 48; const WHISPER_MASK = 0x20;

// 실제로 도달한 장치만 요구하며 미선택 송신/표시를 가짜 성공으로 대체하지 않는다(C++ logic_error → Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('Chat 제출의 실제 선택 장치 미연결'); return hook; }

// DWORD 의 실제 물리 범위를 확인하고 원문 little endian 값을 읽는다(범위 밖은 RangeError).
function read(bytes, at) {
    if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Chat 제출 DWORD의 실제 물리 범위');
    let value = 0; for (let index = 0; index < 4; index += 1) value += bytes[at + index] * 2 ** (index * 8); // 인접 네 바이트만 읽는다.
    return value;
}

// 원문 DWORD store 와 같은 네 바이트만 쓰며 인접 꼬리를 지우지 않는다.
function write(bytes, at, value) {
    if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Chat 제출 store의 실제 물리 범위');
    const word = value >>> 0; for (let index = 0; index < 4; index += 1) bytes[at + index] = (word >>> (index * 8)) & 0xff; // 하위 BYTE 부터 같은 DWORD 를 쓴다.
}

// 같은 토큰의 순수 뷰를 필요한 위치에서 다시 빌린다.
const memoryOf = (hooks, token) => need(hooks.memory)(token);

// 바이트 문자열 또는 Uint8Array 를 첫 NUL 앞의 바이트 문자열로 바꾼다.
function z(value) {
    const text = typeof value === 'string' ? value : Array.from(value, (byte) => String.fromCharCode(byte)).join('');
    const nul = text.indexOf('\0'); return nul < 0 ? text : text.slice(0, nul);
}

// 같은 실제 Uint8Array 의 NUL 앞만 바이트 문자열로 읽는다(인접 캡처까지 이어지는 문자열도 유지, NUL 이 없으면 RangeError).
function textOf(bytes) {
    const end = bytes.indexOf(0); if (end < 0) throw new RangeError('Chat 제출 문자열의 실제 NUL 없음');
    let value = ''; for (let index = 0; index < end; index += 1) value += String.fromCharCode(bytes[index]);
    return value;
}

// Regexp capture 의 최대 2048바이트 복사와 그 다음 NUL 을 같은 인접 작업 공간에 쓴다(범위 밖은 RangeError).
function capture(work, at, value) {
    const count = Math.min(value.length, CAPTURE_BYTES); if (at > work.length || count + 1 > work.length - at) throw new RangeError('Chat 캡처의 실제 물리 범위');
    // 캡처 문자열을 바이트로 복사한다.
    for (let index = 0; index < count; index += 1) work[at + index] = value.charCodeAt(index) & 0xff;
    work[at + count] = 0;
}

// C locale 의 좁은 문자열 대소문자 동등 비교만 수행한다(첫 NUL 앞만 비교, 비ASCII 는 변환하지 않는다).
function equal(first, second) {
    first = z(first); second = z(second); if (first.length !== second.length) return false;
    const fold = (byte) => (byte >= 65 && byte <= 90 ? byte + 32 : byte);
    // 같은 길이의 각 바이트를 앞에서부터 비교한다.
    for (let index = 0; index < first.length; index += 1) if (fold(first.charCodeAt(index) & 0xff) !== fold(second.charCodeAt(index) & 0xff)) return false;
    return true;
}

// 문자열에서 SPACE 가 아닌 첫 위치(없으면 -1)와, 주어진 위치 이후 첫 SPACE 위치(없으면 -1)다.
function firstNotSpace(value) { for (let index = 0; index < value.length; index += 1) if (!SPACE.includes(value[index])) return index; return -1; }
function firstSpace(value, from) { for (let index = from; index < value.length; index += 1) if (SPACE.includes(value[index])) return index; return -1; }

// 실패한 식별자 다음에 여덟 채널 키를 원문 순서로 현재 언어 번역해 지역 이름과 비교한다(번역 호출 뒤에도 이름을 다시 읽는다). 일치하는 키의 위치, 없으면 -1.
function channelOf(identity, hooks) {
    for (let index = 0; index < CHANNELS.length; index += 1) {
        const value = need(hooks.translate)(CHANNELS[index]);
        if (equal(textOf(identity.subarray(8)), value)) return index;
    }
    return -1;
}

// 0042d1b0: 같은 manager 의 중복 DWORD 를 먼저 찾고(signed count), capacity(+4)가 count(+0)보다 크지 않으면 32슬롯을 늘린 뒤(새 할당 크기 = capacity×4+128, 실패 0 이면 진단 0x112 를 알리고 계속, capacity 전체를 복사, 이전 배열 반납, +8/+4 갱신)
// 현재 count 위치에 추가하고 count 를 1 늘린다. 모든 값은 원문 위치에서 현재 물리를 다시 읽는다.
export function addChatRecipient(manager, subscriber, hooks) {
    const object = memoryOf(hooks, manager); const wanted = subscriber >>> 0;
    // 현재 count 의 실제 DWORD 를 순서대로 비교해 중복을 보존하지 않는다.
    for (let index = 0; (index | 0) < (read(object, 0) | 0); index += 1) {
        if (read(memoryOf(hooks, read(object, 8)), (index * 4) >>> 0) === wanted) return;
    }
    if ((read(object, 4) | 0) <= (read(object, 0) | 0)) {
        const buffer = need(hooks.allocate)(((read(object, 4) * 4) + 128) >>> 0);
        if (!buffer) need(hooks.assertion)(0x112);
        // 원문은 count 가 아니라 capacity 전체를 복사한다.
        for (let index = 0; (index | 0) < (read(object, 4) | 0); index += 1) write(memoryOf(hooks, buffer), (index * 4) >>> 0, read(memoryOf(hooks, read(object, 8)), (index * 4) >>> 0));
        need(hooks.release)(read(object, 8)); write(object, 8, buffer); write(object, 4, read(object, 4) + 32);
    }
    write(memoryOf(hooks, read(object, 8)), (read(object, 0) * 4) >>> 0, wanted); write(object, 0, read(object, 0) + 1);
}

// 0042da00: manager 의 수신자 count/직접 표시를 0 으로 쓰고 작업 공간(≥4097바이트)을 입력에 맞춰 채운다. 선행 공백을 건너뛴 입력에 ':' 가 있으면 뒤 본문(앞 공백 제거)을 본문 캡처(0), ':' 앞을 접두 캡처(2048)로 쓰고 직접 표시(+12)를 1 로 둔 뒤
// 접두의 각 공백 구분 이름(최대 40바이트 복사, 위치는 전체 길이만큼 전진)을 식별자 지역(60바이트)에 쓰고 resolve → 성공이면 addChatRecipient, 실패면 채널 키 비교(일치하면 마스크 비트, 직접 표시 0 으로 지움,
// 일치가 없으면 수신자 0·마스크 0x20·입력 전체를 본문 캡처에 되돌리고 중단)한다. ':' 가 없으면 입력 전체가 본문이고 마스크 0x20 이다. 마스크를 +16 에 쓰고 비영이면 Chat 기록 100행(+40 signed 플래그, +44 subscriber)을 순서대로 읽어
// 마스크와 겹치는 행의 subscriber 를 추가한다(행마다 현재 기록을 다시 읽는다). 마지막으로 본문이 비면 입력줄 숨김(0), 아니면 NUL 포함 본문을 분배한다.
// hooks = { memory(token), allocate(bytes) → token|0, release(token), assertion(line), resolve(record) → boolean, translate(key) → 번역, records() → 100×48바이트 기록, inputVisible(0), dispatch(bodyBytes) } 이다.
export function submitChatInput(manager, input, identity, work, hooks) {
    const object = memoryOf(hooks, manager); write(object, 0, 0); write(object, 12, 0);
    if (work.length < CHAT_SUBMIT_WORK_BYTES) throw new RangeError('Chat 제출의 두 인접 캡처 물리 부족'); work = work.subarray(0, CHAT_SUBMIT_WORK_BYTES);
    const text = z(input); let source = text; const first = firstNotSpace(source); source = first < 0 ? '' : source.slice(first);
    const colon = source.indexOf(':'); let mask = 0; work[0] = 0; // 실제 첫 capture3 실패도 본문 첫 BYTE 에 NUL 을 쓴다.
    if (colon >= 0) {
        let body = source.slice(colon + 1); const begin = firstNotSpace(body); body = begin < 0 ? '' : body.slice(begin);
        capture(work, 0, body); capture(work, CAPTURE_BYTES, source.slice(0, colon)); RawPlayerIdentity.initialize(identity, 0); write(object, 12, 1);
        let cursor = 0; // 반복 토큰 Regexp 의 현재 접두 버퍼 상대 위치다.
        // 접두의 각 이름을 전체 캡처 끝으로 이동하며 40바이트 이름 절단과 구별한다.
        for (;;) {
            const prefix = textOf(work.subarray(CAPTURE_BYTES)); if (cursor > prefix.length) throw new RangeError('Chat 접두 토큰의 실제 다음 위치'); const remaining = prefix.slice(cursor);
            const start = firstNotSpace(remaining); if (start < 0) break; const end = firstSpace(remaining, start); const length = (end < 0 ? remaining.length : end) - start;
            write(identity, 0, 0); write(identity, 4, 0); identity[8] = 0; write(identity, 48, 0);
            const copied = Math.min(length, NAME_BYTES);
            // 이름의 앞 40바이트만 식별자 지역에 복사한다.
            for (let index = 0; index < copied; index += 1) identity[8 + index] = remaining.charCodeAt(start + index) & 0xff;
            identity[8 + copied] = 0;
            if (need(hooks.resolve)(identity)) addChatRecipient(manager, read(identity, 4), hooks);
            else {
                const channel = channelOf(identity, hooks); write(object, 12, 0);
                if (channel < 0) {
                    write(object, 0, 0); mask = WHISPER_MASK; const bytes = Math.min(text.length, CAPTURE_BYTES);
                    // 입력 전체를 본문 캡처로 되돌리고 나머지 2048 칸을 0 으로 채운다.
                    for (let index = 0; index < bytes; index += 1) work[index] = text.charCodeAt(index) & 0xff;
                    work.fill(0, bytes, CAPTURE_BYTES); work[CAPTURE_BYTES - 1] = 0; break;
                }
                mask |= 1 << (channel & 31);
            }
            cursor += start + length;
        }
    } else { capture(work, 0, source); mask = WHISPER_MASK; }
    write(object, 16, mask);
    if (mask) {
        // 같은 100행을 고정 순서로 읽으며 선택된 추가 뒤 다음 행은 현재 기록을 다시 조회한다.
        for (let index = 0; index < RECORD_COUNT; index += 1) {
            const records = need(hooks.records)(); if (records.length < RECORD_COUNT * RECORD_STRIDE) throw new RangeError('Chat 현재100행의 실제 물리 부족');
            const row = records.subarray(index * RECORD_STRIDE, (index + 1) * RECORD_STRIDE); const subscriber = read(row, 44); const flags = (row[40] << 24) >> 24;
            if (subscriber && (mask & flags)) addChatRecipient(manager, subscriber, hooks);
        }
    }
    const body = textOf(work); if (body.length === 0) need(hooks.inputVisible)(0); else need(hooks.dispatch)(work.subarray(0, body.length + 1));
}
