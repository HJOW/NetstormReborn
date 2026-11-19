// cpppj DisplayTextInput의 확정 문자 태그와 기존 표시 바이트를 같은 입력 큐에 연결한다.
// 사용: queueCommittedText(queue, text, x, y); 큐 공간이 부족하면 기존 큐를 보존하고 false를 돌려준다.
import { displayTextCodec } from '../assets/display-text.js';

// 비명령 상위 태그와 Unicode 범위를 원본 입력 사건에서 구별한다.
const COMMITTED = 0x01000000, TAG_MASK = 0x03e00000, CODE_MASK = 0x001fffff, EXTENDED_LEAD = 31;

// 한 사건이 원본의 확정 Unicode 문자 태그인지 확인한다.
export function isCommittedTextInput(event) { return ((event >>> 0) & TAG_MASK) === COMMITTED && ((event >>> 0) & CODE_MASK) <= 0x10ffff; }

// 태그 한 사건을 기존 표시 인코딩의 실제 4바이트로 변환한다.
export function committedTextBytes(event) {
    if (!isCommittedTextInput(event)) throw new TypeError('확정 입력 태그 범위 오류');
    const text = String.fromCodePoint((event >>> 0) & CODE_MASK), bytes = displayTextCodec.encode(text);
    if (bytes.length !== 4 || bytes[0] !== EXTENDED_LEAD) throw new RangeError('표시 인코딩 미지원 확정 문자');
    return bytes;
}

// 모든 문자를 검증해 ASCII/1252는 1바이트, 확장 문자는 태그 한 사건으로 만든다.
export function makeCommittedTextEvents(text, x = 0, y = 0) {
    if (typeof text !== 'string' || !Number.isInteger(x) || x < -0x80000000 || x > 0x7fffffff || !Number.isInteger(y) || y < -0x80000000 || y > 0x7fffffff) throw new TypeError('확정 문자열 또는 signed 좌표 오류');
    const events = [];
    // Unicode 문자별 전체 검증 뒤에만 실제 큐에 전달할 목록을 반환한다.
    for (const character of text) {
        const bytes = displayTextCodec.encode(character), scalar = character.codePointAt(0);
        if (bytes.length === 4 && bytes[0] === EXTENDED_LEAD) events.push({ code: (COMMITTED | scalar) >>> 0, x, y });
        else {
            if (bytes.length !== 1 || bytes[0] < 0x20 || bytes[0] === 0x7f) throw new RangeError('확정 입력 제어문자는 키 사건으로 전달해야 합니다');
            events.push({ code: bytes[0], x, y });
        }
    }
    return events;
}

// 현재 큐의 여유 칸이 본문 전체에 충분할 때만 삽입한다.
export function queueCommittedText(queue, text, x = 0, y = 0) {
    if (typeof queue?.tryPushBatch !== 'function') throw new TypeError('확정 입력의 실제 일괄 큐 필요');
    return queue.tryPushBatch(makeCommittedTextEvents(text, x, y));
}

// 바이트 문자열 또는 실제 Uint8Array를 현재 표시 바이트로 읽는다.
function displayBytes(text) {
    if (typeof text === 'string') return Uint8Array.from(text, (character) => character.charCodeAt(0) & 0xff);
    if (text instanceof Uint8Array) return text;
    throw new TypeError('표시 문자열의 바이트 필요');
}

// 현재 위치가 실제로 해독 가능한 네 바이트 확장 문자일 때만 폭 4를 반환한다.
function displayWidth(bytes, at) {
    if (at + 4 > bytes.length || bytes[at] !== EXTENDED_LEAD) return 1;
    return [...displayTextCodec.decode(bytes.subarray(at, at + 4))].length === 1 ? 4 : 1;
}

// 표시 본문에 유효한 확장 문자가 하나라도 있는지 확인한다.
export function hasExtendedDisplayText(text) {
    const bytes = displayBytes(text);
    // 원본은 모든 바이트 위치를 순서대로 확인한다.
    for (let at = 0; at < bytes.length; at++) if (displayWidth(bytes, at) === 4) return true;
    return false;
}

// 임의의 바이트 위치를 문자 앞 경계로 제한한다.
export function displayTextFloor(text, position) {
    const bytes = displayBytes(text);
    if (!Number.isInteger(position) || position < 0) throw new RangeError('표시 문자 위치 오류');
    position = Math.min(position, bytes.length); let at = 0;
    // 앞 문자부터 폭을 더하며 대상이 토큰 내부면 토큰의 앞을 남긴다.
    while (at < position) { const next = at + displayWidth(bytes, at); if (next > position) return at; at = next; }
    return at;
}

// 임의의 바이트 위치를 문자 뒤 경계로 제한한다.
export function displayTextCeil(text, position) {
    const bytes = displayBytes(text);
    if (!Number.isInteger(position) || position < 0) throw new RangeError('표시 문자 위치 오류');
    position = Math.min(position, bytes.length); const floor = displayTextFloor(bytes, position);
    return floor === position ? position : floor + displayWidth(bytes, floor);
}

// 현재 위치 앞의 완전한 문자 경계를 반환한다.
export function previousDisplayText(text, position) {
    const bytes = displayBytes(text), at = displayTextFloor(bytes, position);
    return at ? displayTextFloor(bytes, at - 1) : 0;
}

// 현재 위치 뒤의 완전한 문자 경계를 반환한다.
export function nextDisplayText(text, position) {
    const bytes = displayBytes(text), at = displayTextFloor(bytes, position);
    return at < bytes.length ? at + displayWidth(bytes, at) : at;
}
