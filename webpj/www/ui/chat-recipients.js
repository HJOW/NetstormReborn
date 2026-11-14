// ChatWindow/Console 이 공유하는 원본 대상 이름 검사를 복원한다(원문 0042dc70→0042d910).
// (cpppj client/ChatRecipients.cpp/.h, 기준선 1f00a25a, 10.78 전용)
// 같은 현재 Identity/번역을 빌리는 표시 전용 검사이며 패킷을 보내지 않는다. manager 0/NULL 은 false, 나머지는 모든 토큰이 Identity/채널에 있으면 true 다. 고정 Regexp 005049e8 만 직역하며 범용 정규식/송수신/재접속은 수행하지 않는다.
// scratch 는 60바이트 레코드다. 같은 원문 부분 쓰기·40바이트 이름과 인접 주소 첫 BYTE 의 NUL 을 보존한다. 원문 바이트를 UTF-8/유니코드 공백으로 바꾸지 않는다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E436: RawMenuControls 선행 Chat 대상 이름 검사 이식)
import { RawPlayerIdentity } from '../core/raw-player-identity.js';

// 005049e8 의 정확한 네 구분자다(CR LF TAB SPACE). VT/FF/NBSP 는 이름의 일부다.
const SEPARATORS = '\r\n\t ';
// 00531768 의 첫 NULL 앞 여덟 포인터다. 빈 문자열도 건너뛰지 않는다(Identity 실패 때만 순서대로 번역/비교).
const CHANNELS = ['', '', '', 'BATTLE', '', 'EVERYONE', 'ALLIES', 'OBSERVER'];
// 레코드의 이름 시작(+8)과 이름 칸 폭(40), 이름 뒤 첫 NUL 이 놓일 수 있는 레코드 끝(60)이다.
const NAME_OFFSET = 8; const NAME_BYTES = 40; const RECORD_BYTES = 60;

// 하위가 실제 선택되기 전에는 장치 누락을 검사하지 않는다.
function need(fn) { if (typeof fn !== 'function') throw new Error('채팅 대상 이름 검사의 선택 장치 미연결'); return fn; }
// C 문자열 첫 NUL 뒤 바이트는 이름/번역 비교에 포함하지 않는다. 바이트 문자열(또는 Uint8Array)을 바이트 문자열로 바꾼다.
function z(text) {
    const value = typeof text === 'string' ? text : Array.from(text, (byte) => String.fromCharCode(byte)).join('');
    const nul = value.indexOf('\0'); return nul < 0 ? value : value.slice(0, nul);
}
// 이름 40바이트 복사 뒤 NUL 은 인접 주소 필드의 첫 BYTE 일 수도 있다. 원문 지역 전체 범위(+8..+60)에서만 찾는다.
function nameOf(record) {
    let end = NAME_OFFSET; while (end < RECORD_BYTES && record[end] !== 0) end += 1;
    if (end === RECORD_BYTES) throw new RangeError('채팅 대상 지역 이름의 NUL 물리 끝');
    let name = ''; for (let index = NAME_OFFSET; index < end; index += 1) name += String.fromCharCode(record[index]); return name;
}
// 004841b0 은 같은 지역의 선택 필드만 지우며 앞선 Resolve 가 바꾼 mode/미작성 이름 꼬리를 보존한다(앞 8바이트, 이름 첫 바이트, +48..+52).
function reset(record) { record.fill(0, 0, 8); record[8] = 0; record.fill(0, 48, 52); }
// CRT stricmp 의 C locale 처럼 ASCII 알파벳만 접고 확장 바이트는 그대로 비교한다. 첫 NUL 뒤는 비교하지 않고 길이가 다르면 다르다.
function equal(first, second) {
    first = z(first); second = z(second); if (first.length !== second.length) return false;
    const fold = (byte) => (byte >= 65 && byte <= 90 ? byte + 32 : byte);
    // 같은 길이의 각 바이트를 앞에서부터 비교한다.
    for (let index = 0; index < first.length; index += 1) if (fold(first.charCodeAt(index) & 255) !== fold(second.charCodeAt(index) & 255)) return false;
    return true;
}

// 고정 ^[CR LF TAB SPACE]*([^CR LF TAB SPACE]+)(.*)를 반복하며 전체 토큰의 다음 시작점은 절단 전 끝이다. text 는 바이트 문자열/Uint8Array/null 이고 공백뿐인 나머지는 원문 regex 검색 실패처럼 true 이며 하위 조회를 하지 않는다.
// hooks = { resolve(record) → boolean (004841c0 의 현재 Player/Chat 이름 조회, record 를 직접 수정), translate(key) → 채널 이름 번역(004de9a0, 빈 키도 조회) }. 사용: checkChatRecipients(managerToken, 'Ann EVERYONE', new Uint8Array(60), hooks)
export function checkChatRecipients(manager, text, scratch, hooks) {
    if (!manager || text === null || text === undefined) return false;
    RawPlayerIdentity.initialize(scratch, 0); let remaining = z(text);
    for (;;) {
        let begin = 0; while (begin < remaining.length && SEPARATORS.includes(remaining[begin])) begin += 1;
        if (begin === remaining.length) return true;
        remaining = remaining.slice(begin);
        let end = 0; while (end < remaining.length && !SEPARATORS.includes(remaining[end])) end += 1;
        const length = end;
        reset(scratch);
        const copied = Math.min(length, NAME_BYTES);
        for (let index = 0; index < copied; index += 1) scratch[NAME_OFFSET + index] = remaining.charCodeAt(index) & 255;
        scratch[NAME_OFFSET + copied] = 0;
        if (!need(hooks.resolve)(scratch)) {
            let found = false;
            // 원문은 Identity 실패 때만 여덟 포인터를 순서대로 번역/비교한다.
            for (const key of CHANNELS) {
                const translated = need(hooks.translate)(key);
                if (equal(nameOf(scratch), translated)) { found = true; break; }
            }
            if (!found) return false;
        }
        remaining = remaining.slice(length); // 40바이트로 절단한 이름과 별개로 실제 캡처 2 시작점까지 전진한다.
    }
}
