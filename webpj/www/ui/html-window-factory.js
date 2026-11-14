// 원문 전체HTML 생성의두번변환·메타·기본버튼·부착·모달과현재serial을 연결한다(cpppj client/HtmlWindowFactory.cpp/.h, 기준선1f00a25a).
// 1d5b1f43의 localizedContent 선택관문도 반영한다. 실제표시codec/한국어기기는호출자가공급한다. 사용: new HtmlWindowFactory(state,hooks).show(source,suppress,width,height). (2026-11-14 WB04-E459)
import { createParserState, hasAlphanumeric } from './html-text.js';

// 원문지역출력/부모링크저장소의공통8192BYTE 한도다.
const CAPACITY = 8192;
// 선택된실제장치만요구한다.
function need(hook) { if (typeof hook !== 'function') throw new Error('HTML 전체 생성 실제 장치 미연결'); return hook; }
// 첫NUL뒤의원문바이트는처리하지않는다.
function cString(value) { return value.split('\0', 1)[0]; }
// 현재실제버퍼의첫NUL앞BYTE를원문문자열로읽는다.
function text(buffer) { const end = buffer.indexOf(0); if (end < 0) throw new RangeError('HTML 전체 생성 지역 NUL 범위 초과'); return Array.from(buffer.subarray(0, end), (byte) => String.fromCharCode(byte)).join(''); }
// 실제고정지역버퍼에첫NUL까지복사한다. 문자열은이미표시부호화된BYTE여야하며UTF16을암묵절단하지않는다.
function copy(buffer, source) {
    const value = cString(source); if (value.length >= buffer.length) throw new RangeError('HTML 전체 생성 지역 NUL 범위 초과');
    // 원문복사BYTE와NUL만쓰고이미존재하는꼬리는보존한다.
    for (let index = 0; index < value.length; index += 1) { const byte = value.charCodeAt(index); if (byte > 255) throw new RangeError('HTML 전체 생성 원문 BYTE문자열 필요'); buffer[index] = byte; }
    buffer[value.length] = 0;
}

// 같은Client장기상태와실제HTML부모/파서/Config/메뉴/수명을빌리는전체공장이다.
export class HtmlWindowFactory {
    // 상태는참조로공유하고선택장치표는C++처럼값복사한다.
    #state; #hooks;
    // 생성자는아직선택하지않은장치를선행조회하지않는다.
    constructor(state, hooks = {}) { this.#state = state; this.#hooks = { ...hooks }; }
    // 생성/효과뒤의현재실제부모주소를다시읽는다.
    #node(token) { const node = token ? need(this.#hooks.node)(token) : null; if (!node) throw new Error('HTML 전체 생성 현재 부모 없음'); return node; }
    // suppress는할당없이첫파서효과만실행한다. 반환은모달효과뒤현재serial이며createdSerial캐시와구별한다.
    show(source, suppress, width, height) {
        const h = this.#hooks; const state = this.#state; const root = suppress ? 0 : need(h.create)(width >>> 0, height >>> 0); state.createdSerial = root ? this.#node(root).serial : 0;
        const substituted = need(h.substitute)(cString(source), CAPACITY); if (cString(substituted).length >= CAPACITY) throw new RangeError('HTML 치환 지역8192BYTE 범위 초과');
        const parser = createParserState(); const first = new Uint8Array(CAPACITY); copy(first, need(h.parse)(parser, root, cString(substituted), CAPACITY)); if (!root) return 0;
        const content = text(first);
        if (!hasAlphanumeric(content) && !(h.localizedContent && need(h.localizedContent)(content))) {
            state.createdSerial = 0; need(h.body)(root, 'xxx'); need(h.attach)(root, 0, 0, need(h.read)(0x563e38), 0); need(h.kill)(root); return 0;
        }
        state.lastCloseTime = need(h.wallSeconds)();
        if (!need(h.metadata)(root, first)) { const fallback = new Uint8Array(CAPACITY); copy(fallback, need(h.fallback)()); need(h.metadata)(root, fallback); }
        const secondSource = text(first).replaceAll('\n', '<br>'); let second = cString(need(h.parse)(parser, root, secondSource, CAPACITY));
        if (second.length + 1 >= CAPACITY) throw new RangeError('HTML 마지막 LF/NUL 지역 범위 초과'); second += '\n'; need(h.body)(root, second);
        const screenHeight = need(h.read)(0x5c78f4) | 0; const parent = need(h.read)(0x563e38); const y = Math.fround((screenHeight - this.#node(root).baseHeight) * 0.5);
        const screenWidth = need(h.read)(0x5c78f0) | 0; const x = Math.fround((screenWidth - this.#node(root).baseWidth) * 0.5);
        need(h.attach)(root, x, y, parent, 0); need(h.modal)(root, true); return this.#node(root).serial;
    }
}
