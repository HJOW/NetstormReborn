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
