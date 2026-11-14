// 원문9종 HTML메타의 우선순위·현재부모/원시버퍼·명령고리 수명을 연결한다(cpppj client/HtmlWindowMetadata.cpp/.h, 기준선1f00a25a).
// 4edec13c의 NB08 목록삽입 repairInlineList도 명시 옵션으로 반영한다(기본false=원본부분복사). 실제 제품허브는 후속이다. (2026-11-14 WB04-E458)
import { configParseLong, asciiLower } from '../assets/config.js';
import { resolveDialogExitCommand } from './dialog-exit-command.js';

// 원문9regex의 대소문자/종류 우선순위다. d플래그는 callback뒤에도 원래 캡처주소의 현재BYTE를 읽기 위한 위치정보다.
const PATTERNS = [
    /\$Button[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/d,
    /\$ButtonInstant[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/d,
    /\$Menu[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/d,
    /\$Checked[ \t]*=[ \t]*([^,]+),([^,]+),([^,]+),([^,]+),([^$ ]+)/d,
    /\$Timeout[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/d,
    /\$OnExit[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/d,
    /\$Key[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/d,
    /\$Input[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/d,
    /\$Zsend[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)/d,
];
// 도달한 실제 장치만 요구한다.
function need(hook) { if (typeof hook !== 'function') throw new Error('HTML 메타 실제 장치 미연결'); return hook; }
// 실제첫NUL까지의 현재길이다. NUL없는물리를 외부바이트로 채우지 않는다.
function length(buffer) { const end = buffer.indexOf(0); if (end < 0) throw new RangeError('HTML 메타 실제 버퍼 NUL 범위 초과'); return end; }
// 문자열은 원문BYTE하나당한글자인이진문자열이다.
function string(bytes) { return Array.from(bytes, (byte) => String.fromCharCode(byte)).join(''); }
// 원문 캡처의 고정길이/주소를 보존하고 현재BYTE만 읽는다. 끝NUL자리까지 포함한지역버퍼한계를 확인한다.
function capture(buffer, indices, capacity) {
    const [start, end] = indices; if (end - start >= capacity) throw new RangeError('HTML 메타 원문 지역 캡처 NUL 범위 초과');
    if (end > buffer.length) throw new RangeError('HTML 메타 현재 캡처 물리 범위 초과'); return string(buffer.subarray(start, end));
}
// 원문BYTE만 실제목적에 복사하며 인코딩되지않은UTF16을 낮은BYTE로 잘라버리지 않는다.
function write(buffer, value, count = value.length) {
    if (count > buffer.length) throw new RangeError('HTML 메타 원문 치환 복사 범위 초과');
    // 명시한복사길이만 쓰고 뒤쪽BYTE/NUL은 호출자가 원문분기에맞춰 처리한다.
    for (let index = 0; index < count; index += 1) { const byte = value.charCodeAt(index); if (byte > 255) throw new RangeError('HTML 메타 원문 BYTE문자열 필요'); buffer[index] = byte; }
}

// 같은 HTML 부모와 고정메타본문을 사용하는 실제처리기다.
export class HtmlWindowMetadata {
    // 값복사한선택callback표와 명시NB08 옵션이다. 실제장기owner는 각함수closure에서 공유한다.
    #hooks;
    // node만 선행필수이며 commands표도 C++ 값구조체처럼복사한다.
    constructor(hooks) { if (typeof hooks?.node !== 'function') throw new TypeError('HTML 메타 같은 실제 부모 필요'); this.#hooks = { repairInlineList: false, ...hooks, commands: { ...hooks.commands } }; }
    // 표시용사본이아닌현재부모를조회한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node) throw new Error('HTML 메타 현재 부모 없음'); return node; }
    // 초기버튼정리뒤항상Button부터우선검색하며 원문물리버퍼를제자리수정한다.
    process(token, buffer) {
        this.#node(token); const h = this.#hooks; need(h.clearButtons)(token); let buttons = 0;
        // 제거/치환뒤9종고정패턴을처음부터다시검색한다. 종류우선순위는본문위치보다앞선다.
        for (;;) {
            const source = string(buffer.subarray(0, length(buffer))); let match; let kind = 0;
            // 같은종류의첫일치가뒤종류의더이른일치보다우선한다.
            for (; kind < PATTERNS.length; kind += 1) { match = PATTERNS[kind].exec(source); if (match) break; }
            if (kind === PATTERNS.length) break;
            const label = capture(buffer, match.indices[1], 80); const command = capture(buffer, match.indices[2], 80); const argument = capture(buffer, match.indices[3], 256);
            const resolved = { command: 0, parameter: 0 }; resolveDialogExitCommand(resolved, command, argument, { commands: h.commands, storeText: h.storeText }); let replacement = ''; let insert = false;
            if (kind === 3) {
                const checked = capture(buffer, match.indices[4], 256); const enabled = capture(buffer, match.indices[5], 256);
                if (!this.#node(token).menuColumns) { need(h.beginMenu)(0x40000); this.#node(token).menuColumns = (this.#node(token).menuColumns + 1) >>> 0; }
                const allow = configParseLong(enabled) !== 0 || need(h.read)(0x5cab30) !== 0 || need(h.read)(0x55a480) !== 0;
                need(h.checked)(configParseLong(checked), label, resolved.command, resolved.parameter, allow);
            } else if (kind === 2) {
                if (!this.#node(token).menuColumns) { need(h.beginMenu)(0x40000); this.#node(token).menuColumns = (this.#node(token).menuColumns + 1) >>> 0; }
                const lower = asciiLower(label);
                if (lower === '*sololist*' || lower === '*fortlist*') {
                    const node = this.#node(token); const columns = { get value() { return node.menuColumns; }, set value(value) { node.menuColumns = value >>> 0; } }; need(h.maps)(label, columns);
                } else if (lower === '*techlist*') need(h.tech)(resolved.parameter);
                else if (lower === '*themetechlist*') need(h.themeTech)(resolved.parameter & 0xffff, resolved.parameter >> 16);
                else if (label.startsWith('@')) need(h.namedList)(label.slice(1), resolved.command, argument);
                else if (label.startsWith('#')) need(h.fileList)(label.slice(1), resolved.command, argument);
                else if (label.startsWith('%')) { replacement = need(h.inlineList)(label.slice(1), command, argument).split('\0', 1)[0]; insert = true; }
                else need(h.menuText)(label, resolved.command, resolved.parameter);
            } else if (kind === 4) {
                const seconds = configParseLong(label); const deadline = seconds + need(h.wallSeconds)(); const node = this.#node(token); node.timeoutCommand = resolved.command; node.timeoutParameter = resolved.parameter; node.timeoutSeconds = deadline; need(h.idle)(token, true);
            } else if (kind === 5) { this.#node(token).onExitCommand = resolved.command; this.#node(token).onExitParameter = resolved.parameter; }
            else if (kind === 7) { need(h.input)(configParseLong(command), label, argument); need(h.queue)(need(h.read)(0x5429ec), 0, 0); }
            else if (kind === 8) need(h.send)(label, command, argument);
            else if (kind !== 6) { need(h.addButton)(token, label, resolved.command, resolved.parameter, Number(kind === 1)); buttons = (buttons + 1) >>> 0; }
            const start = match.index; const currentLength = length(buffer); if (start > currentLength) throw new RangeError('HTML 메타 현재 일치 시작 범위 초과'); let end = start + Number(buffer[start] === 0x24);
            // 원래캡처끝이아니라 현재다음LF/$/NUL까지지우고LF한개를포함한다.
            while (end < currentLength && buffer[end] !== 10 && buffer[end] !== 0x24) end += 1;
            if (end < currentLength && buffer[end] === 10) end += 1; buffer.copyWithin(start, end, currentLength + 1);
            if (insert) {
                const current = string(buffer.subarray(0, length(buffer))); const merged = current.slice(0, start) + replacement + current.slice(start);
                if (h.repairInlineList) { const clipped = merged.slice(0, buffer.length - 1); write(buffer, clipped); buffer[clipped.length] = 0; }
                else write(buffer, merged, replacement.length);
            }
        }
        return buttons || this.#node(token).menuColumns ? 1 : 0;
    }
}
