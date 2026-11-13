// 원문 StyleText의 안정된 조각 표·소유 체인/문맥과 두 번 측정/출력을 이식한다(cpppj client/StyleText.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E418)
import { encodeWindows1252 } from '../assets/text-encoding.js';
import { decodeGumpTextContext } from '../render/gump-text-draw.js';

// 조각 가상 분류와 원문 DWORD 최대 길이/토큰이다.
export const StyleTextFragmentKind = Object.freeze({ TEXT: 0, EXTERNAL: 1, BASE: 2 });
const MAX_DWORD = 0xffffffff;
// 10.78의 짧은 색 명령과 실제 PaletteColorTable 기본 별칭이다.
const COLORS = 'bgijklmnoqrtwxyadpsu'; const BASIC_COLORS = { k: 0, r: 1, g: 2, b: 3, m: 4, y: 5, o: 6, w: 7 };
// 기본색 외 명명 전역의 RGB다. 현재 named 표에서만 번호를 빌린다.
const NAMED_COLORS = { a: [255, 153, 0], d: [0, 255, 102], i: [249, 233, 145], j: [233, 214, 186], l: [151, 215, 210], p: [255, 102, 204], q: [200, 175, 144], s: [99, 66, 255], t: [219, 213, 203], u: [51, 255, 102], x: [47, 24, 17] };
// 같은 실제 소유 필드의 참조를 반복 바인딩해도 같은 별칭 객체를 돌려준다.
const FIELD_ALIASES = new WeakMap();
// 도달한 가상 장치만 요구한다.
function need(fn) { if (typeof fn !== 'function') throw new TypeError('StyleText 선택 가상/장치 경계 필요'); return fn; }
// 순환 체인을 거부하며 정상 방문 순서를 바꾸지 않는다.
function visit(seen, token) { if (seen.has(token)) throw new Error('StyleText 순환 체인'); seen.add(token); }
// 문자열 입력은 실제 원문 바이트 또는 명시 웹 UTF16 코드 단위다.
function span(bytes) { if (bytes instanceof Uint16Array) return bytes; if (bytes instanceof Uint8Array) return new Uint8Array(bytes.buffer, bytes.byteOffset, bytes.byteLength); throw new TypeError('StyleText 바이트/UTF16 범위 필요'); }
// 문자열을8DWORD의 값형 문맥과 함께 보관할 때 사용한다.
function copyContext(words) { if (words?.length !== 8) throw new TypeError('StyleText 문맥8DWORD 필요'); return Uint32Array.from(words); }
// 큰 문자열도 호출 인자 스택 상한 없이 같은 코드 단위로 스냅샷한다.
function codeString(bytes) { let value = ''; for (let offset = 0; offset < bytes.length; offset += 8192) value += String.fromCharCode(...bytes.subarray(offset, offset + 8192)); return value; }
// parser의 내부 문자열 구간을 같은 바이트/UTF16 저장소와 명시 NUL로 복사한다.
function segmentBytes(text, Type) { const bytes = new Type(text.length + 1); for (let i = 0; i < text.length; i += 1) bytes[i] = text.charCodeAt(i); return bytes; }

// 현재 필드 자체가 head를 소유한다. 같은 owner/key는 같은 C++ 참조에 해당한다.
export function styleTextField(owner, key) { let fields = FIELD_ALIASES.get(owner); if (!fields) { fields = new Map(); FIELD_ALIASES.set(owner, fields); } if (!fields.has(key)) fields.set(key, { get value() { return owner[key]; }, set value(value) { owner[key] = value >>> 0; } }); return fields.get(key); }

// 공통 +4 next/+8 metadata·소유 문자열/+10 문맥의 의미별 저장소다.
export class StyleTextFragment {
    // 원문 가상 분류·공통 태그/다음 토큰이다.
    kind = StyleTextFragmentKind.TEXT; next = 0; metadata = 0;
    // 문자열은 NUL을 포함하며 미생성/소멸 상태에서는 빈 저장소다.
    text = new Uint8Array(); context = new Uint32Array(8);
}
// 조각 Map은 삽입 시 기존 주소를 바꾸지 않는다. nextToken0은 소진 상태다.
export class StyleTextState {
    // 실제 단일 소유 표와 단조 증가 자동 토큰이다.
    fragments = new Map(); nextToken = 1;
}
// 실제 도달한 토큰만 확인한다. 호스트 함수 주소로 해석하지 않는다.
function fragment(state, token) { token >>>= 0; const node = token ? state.fragments.get(token) : null; if (!node) throw new RangeError('StyleText 조각 저장소 없음'); return node; }
// 현재 next를 매 방문에서 읽는다.
function fragmentTail(state, head) { const seen = new Set(); for (;;) { visit(seen, head); const next = fragment(state, head).next; if (!next) return head; head = next; } }
// 외부/문자열은 같은 토큰 표를 사용하고 기존 칸을 덮어쓰거나 반납 토큰을 재사용하지 않는다.
export function registerStyleTextFragment(state, input) { let token = state.nextToken >>> 0; while (token && state.fragments.has(token)) token = token === MAX_DWORD ? 0 : token + 1; if (!token) throw new RangeError('StyleText 자동 토큰 발급 소진'); const node = Object.assign(new StyleTextFragment(), { kind: input.kind ?? StyleTextFragmentKind.TEXT, next: input.next >>> 0, metadata: input.metadata >>> 0, text: span(input.text ?? new Uint8Array()).slice(), context: copyContext(input.context ?? new Uint32Array(8)) }); state.fragments.set(token, node); state.nextToken = token === MAX_DWORD ? 0 : token + 1; return token; }
// Append는 단일 조각만 붙이고 새 조각의 이전 next를0으로 만든다.
export function appendStyleTextFragment(state, head, token) { fragment(state, token); if (!head.value) head.value = token; else fragment(state, fragmentTail(state, head.value)).next = token; fragment(state, token).next = 0; }

// 실제 저장소/글꼴·가상 경계와 같은 현재 소유 필드만 사용한다.
export class StyleText {
    // 장기 단일 조각 표와 실제 가상/글꼴/팔레트 장치다.
    #state; #hooks;
    // 파싱/연결만 할 때는 출력 장치를 요구하지 않는다.
    constructor(state, hooks = {}) { this.#state = state; this.#hooks = hooks; }
    // 원문 길이 -1만 NUL을 찾고 명시 길이는 중간 NUL까지 그대로 복사한다.
    newText(bytes, context, length = MAX_DWORD, metadata = 0) { bytes = span(bytes); length >>>= 0; let count = length; if (length === MAX_DWORD) { count = bytes.indexOf(0); if (count < 0) throw new RangeError('StyleText 문자열 범위에 NUL 필요'); } else if (count > bytes.length) throw new RangeError('StyleText 문자열 복사 범위 초과'); const text = new bytes.constructor(count + 1); text.set(bytes.subarray(0, count)); return registerStyleTextFragment(this.#state, { text, context, metadata }); }
    // +8 metadata가 실제 복사 길이다. 새 +8은0이며 문자열 전체 복제로 추정하지 않는다.
    cloneText(token) { const n = fragment(this.#state, token); if (n.kind !== StyleTextFragmentKind.TEXT) throw new TypeError('StyleText 문자열 조각 복제 필요'); return this.newText(n.text, n.context, n.metadata, 0); }
    // 단일 조각의 꼬리 연결이다. head는 실제 소유 필드 별칭이다.
    append(head, token) { appendStyleTextFragment(this.#state, head, token); }
    // source 체인의 첫 next를 지우지 않고 전체 소유권을 이전한다.
    transfer(head, source) { if (head === source) throw new TypeError('StyleText 소유 필드 자기 이전'); if (!head.value) head.value = source.value; else fragment(this.#state, fragmentTail(this.#state, head.value)).next = source.value; source.value = 0; }
    // 문자열/next를 먼저 비운다. external은 실제 자원 소멸 경계에 위임한다.
    destroy(token, flags) { flags &= 255; const n = fragment(this.#state, token); if (n.kind === StyleTextFragmentKind.TEXT) { n.text = new n.text.constructor(); n.next = 0; n.kind = StyleTextFragmentKind.BASE; } else if (n.kind === StyleTextFragmentKind.EXTERNAL) need(this.#hooks.externalDestroy)(token, flags); if (flags & 1) { if (this.#hooks.release) this.#hooks.release(token); this.#state.fragments.delete(token); } }
    // 삭제 전 저장한 next로 순회하고 마지막에 현재 소유 head를0으로 쓴다.
    clear(head) { let token = head.value; const seen = new Set(); while (token) { visit(seen, token); const next = fragment(this.#state, token).next; this.destroy(token, 1); token = next; } head.value = 0; }
    // 캐시 높이/상승만 반환하며 논리 글꼴 높이·그림자 여백은 더하지 않는다.
    baseline(token) { const n = fragment(this.#state, token); if (n.kind === StyleTextFragmentKind.BASE) return [0, 0]; if (n.kind === StyleTextFragmentKind.EXTERNAL) { const result = need(this.#hooks.externalMetrics)(token); return [result[0] | 0, result[1] | 0]; } const result = need(this.#hooks.textMetrics)(n.context); return [result[0] | 0, result[1] | 0]; }
    // 측정은 저장 문맥만 사용하며 Draw override로 크기를 바꾸지 않는다.
    measureFragment(token) { const n = fragment(this.#state, token); if (n.kind === StyleTextFragmentKind.BASE) return [0, 0]; if (n.kind === StyleTextFragmentKind.EXTERNAL) { const result = need(this.#hooks.externalMeasure)(token); return [result[0] | 0, result[1] | 0]; } const output = { width: 0, height: 0 }; need(this.#hooks.textMeasure)(n.text, n.context, output); return [output.width | 0, output.height | 0]; }
    // override의 나머지는 값으로 복사하되 font 슬롯만 현재 저장 문맥의 값을 쓴다.
    drawFragment(token, position, override = null) { const n = fragment(this.#state, token); if (n.kind === StyleTextFragmentKind.BASE) return; position = { x: position.x | 0, y: position.y | 0 }; if (n.kind === StyleTextFragmentKind.EXTERNAL) { need(this.#hooks.externalDraw)(token, position, override); return; } const draw = need(this.#hooks.textDraw); if (override) { const context = copyContext(override); context[1] = n.context[1]; draw(n.text, position, context); } else draw(n.text, position, n.context); }
    // 높이→폭 초기화 뒤 현재 head를 읽고 두 번째 순회도 현재 소유 필드부터 시작한다.
    measure(head, output = { width: 0, height: 0 }) { output.height = 0; output.width = 0; let token = head.value; if (!token) return output; let ascent = 0; const seen = new Set();
        // 첫 순회는 최대 상승만 구하며 가상 호출 뒤 현재 next를 읽는다.
        while (token) { visit(seen, token); ascent = Math.max(ascent, this.baseline(token)[1]); token = fragment(this.#state, token).next; }
        token = head.value; seen.clear();
        // 각 가상 호출 뒤 높이→폭 쓰기와 이후의 현재 next 조회를 유지한다. 출력 alias도 그대로다.
        while (token) { visit(seen, token); const baseline = this.baseline(token); const size = this.measureFragment(token); output.height = Math.max(output.height, (((size[1] - baseline[1]) | 0) + ascent) | 0); output.width = (output.width + size[0]) | 0; token = fragment(this.#state, token).next; }
        return output;
    }
    // Draw head는 값이다. 첫 순회 중 소유 필드가 바뀌어도 시작 토큰을 바꾸지 않는다.
    draw(head, position, override = null) { head >>>= 0; if (!head) return 0; position = { x: position.x | 0, y: position.y | 0 }; let height = 0; let ascent = 0; let token = head; const seen = new Set();
        // 첫 순회의 최대 캐시 높이와 상승을 각각 구한다.
        while (token) { visit(seen, token); const baseline = this.baseline(token); height = Math.max(height, baseline[0]); ascent = Math.max(ascent, baseline[1]); token = fragment(this.#state, token).next; }
        token = head; seen.clear();
        // 출력 뒤에도 이번 측정 폭을 사용하며 각 가상 호출 뒤 현재 next를 따른다.
        while (token) { visit(seen, token); const baseline = this.baseline(token); const size = this.measureFragment(token); this.drawFragment(token, { x: position.x, y: (position.y + ((ascent - baseline[1]) | 0)) | 0 }, override); position.x = (position.x + size[0]) | 0; token = fragment(this.#state, token).next; } return height;
    }
    // 짧은 색 명령에서만 실제 원본 팔레트 장치를 요구한다.
    #color(command) { return need(this.#hooks.color)(command) >>> 0; }
    // 현재 default 문맥 복귀·10.78 제한 글꼴 명령과 전체8DWORD 문맥을 보존한다.
    applyShortCommand(command, context, currentDefault, restricted = false) { switch (command) { case '*': context[0] = 0; break; case '.': context.set(currentDefault); break; case '0': case '1': case '3': case '4': if (!restricted) context[1] = Number(command); break; case '5': case '6': context[1] = Number(command); break; case 'B': context[3] = 2; break; case 'I': context[3] = 1; break; case 'N': context[0] &= ~2; context[3] = 0; break; case 'E': context[0] |= 2; context[5] = this.#color('k'); context[6] = context[7] = 1; break; case 'S': context[3] = 3; break; case 'U': context[3] = 4; break; case 'n': context[2] = currentDefault[2]; break; default: if (!COLORS.includes(command) || command.length !== 1) return false; context[2] = this.#color(command); } return true; }
    // 기존 head 뒤에 구간을 추가한다. 문자열은 기존 Windows1252 기본, UTF16은 명시 코드 배열로 공급한다.
    parse(head, input, initialContext) { let raw = typeof input === 'string' ? encodeWindows1252(input) : span(input); if (raw.length >= MAX_DWORD) throw new RangeError('StyleText 원본 DWORD 문자열 길이 한도 초과'); const nul = raw.indexOf(0); if (nul >= 0) raw = raw.subarray(0, nul); const source = codeString(raw); const context = copyContext(initialContext); const original = copyContext(initialContext); let position = 0; let metadata = 0; const tag = { get value() { return metadata; }, set value(value) { metadata = value >>> 0; } };
        // 명령 앞 구간과 마지막 빈 구간도 각각 현재 문맥/태그의 새 조각이다.
        for (;;) { const mark = source.indexOf('~', position); if (mark < 0) { this.append(head, this.newText(segmentBytes(source.slice(position), raw.constructor), context, MAX_DWORD, metadata)); return; } if (mark > position) this.append(head, this.newText(segmentBytes(source.slice(position, mark), raw.constructor), context, mark - position, metadata)); const command = source[mark + 1] ?? '\0'; let next = mark + 2;
            if (command === '~') this.append(head, this.newText(segmentBytes('~', raw.constructor), context, MAX_DWORD, metadata)); else if (command === '[') { const close = source.indexOf(']', next); if (close >= 0) { need(this.#hooks.bracket)(source.slice(next, close), context, head, tag); next = close + 1; } } else if (!this.applyShortCommand(command, context, original)) next = mark + 1; position = Math.min(next, source.length);
        }
    }
}

// 같은 실제 GumpText의 캐시 메트릭/첫 폭 측정과 선택적 현재 글리프 출력을 연결한다.
export function bindStyleTextFonts(hooks, text, draw = null) { hooks.textMetrics = (words) => text.metrics(decodeGumpTextContext(words)); hooks.textMeasure = (bytes, words, output) => text.measure(bytes, decodeGumpTextContext(words), output); if (draw) hooks.textDraw = (bytes, position, words) => draw.draw(bytes, position, decodeGumpTextContext(words)); }
// 실제 마지막 파일 팔레트의 기본 별칭/명명 전역만 빌린다.
export function bindStyleTextPalette(hooks, palette) { hooks.color = (command) => { if (Object.hasOwn(BASIC_COLORS, command)) return palette.basic[BASIC_COLORS[command]]; const rgb = NAMED_COLORS[command]; if (!rgb) throw new RangeError('StyleText 팔레트 명령 범위 오류'); for (const named of palette.named) if (named.red === rgb[0] && named.green === rgb[1] && named.blue === rgb[2]) return named.index; throw new RangeError('StyleText 원본 색 표 항목 없음'); }; }
// 같은 메뉴 항목의 styled 소유 필드를 매 바인딩 호출 시 현재 저장소에서 조회한다.
export function bindMenuLayoutStyleText(hooks, text) { const lookup = need(hooks.node); const head = (token, index) => { const node = lookup(token); if (!node || index < 0 || index >= node.items.length) throw new RangeError('StyleText 메뉴 항목 범위 오류'); return styleTextField(node.items[index], 'styled'); }; hooks.clearStyle = (token, index) => text.clear(head(token, index)); hooks.parseStyle = (token, index, source, context) => text.parse(head(token, index), source, context); hooks.styled = (token, index) => { const output = text.measure(head(token, index)); return [output.width, output.height]; }; }
