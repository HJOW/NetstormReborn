// 원본 StyleText 의 개행 입력 줄 저장소와 폭에 따라 다시 만든 표시 줄(공백 줄바꿈)을 같은 조각 체인에 연결한다.
// (cpppj client/StyleTextBuffer.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 주소·판본 인자는 옮기지 않는다)
// 입력 줄과 표시 복제 조각은 서로 다른 소유권을 가진다: 입력 줄 삭제는 표시 복제를 직접 지우지 않고 다음 양수 폭 layout 이 이전 표시 목록을 반납한다.
// 원본의 엄격한 폭 비교(정확히 같으면 분할 경로)·공백 포함 복사·첫 단어 강제 배치·signed 크기/위치/줄 수의 DWORD 감김을 그대로 유지한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E422: RawMenuControls 선행 StyleTextBuffer 이식)
import { StyleTextFragmentKind } from './style-text-fragments.js';
import { decodeGumpTextContext } from '../render/gump-text-draw.js';

// NUL 탐색 길이 -1 과 자동 표시 줄 토큰의 마지막 DWORD 다.
const DWORD_MAXIMUM = 0xffffffff;
// 원본 signed 입력/문자열 길이의 상한이다(이 값 이상은 원본 signed 범위 밖).
const INT32_MAXIMUM = 0x7fffffff;
// 원본 signed 크기/위치/줄 수 합산의 하위 DWORD 감김을 보존한다.
const add = (first, second) => (first + second) | 0;
// 도달한 가상 장치만 요구한다. 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 알린다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }
// 잘못된 호스트 링크로 무한 순회하는 경우만 거부한다(정상 방문 순서는 바꾸지 않는다).
function visit(seen, token) { if (seen.has(token)) throw new Error('StyleTextBuffer 순환 링크'); seen.add(token); }
// 공백(space)/NUL 앞의 단어 끝을 찾는다(CD 004b5ba0·패치의 동일 인라인 반복). 다른 공백 문자는 단어 안에 유지하고 연속 space 도 하나씩 처리한다.
// 범위 안에 NUL 도 공백도 없이 끝나면 호스트 입력 오류다(원본은 NUL 이 항상 있다).
function wordEnd(bytes, position) {
    // 원본 시작 인덱스부터 단어의 끝만 찾는다.
    while (position < bytes.length && bytes[position] !== 0 && bytes[position] !== 32) position += 1;
    if (position === bytes.length) throw new RangeError('StyleTextBuffer 단어 범위에 NUL 필요');
    return position;
}

// 원본 24바이트 표시 줄의 소유 head·높이·폭·세로 위치·양방향 링크다. 0 링크는 센티널을 뜻한다.
export class StyleTextBufferLine {
    // head: 이 줄이 소유한 조각 체인의 머리 토큰(0 이면 비어 있음).
    head = 0;
    // 줄의 높이/폭/세로 위치(signed 32비트)다.
    height = 0; width = 0; y = 0;
    // 다음/이전 표시 줄 토큰(0 은 센티널)이다.
    next = 0; previous = 0;
}

// +0 max/+4 완료 줄 수/+8 전체 칸/+c 센티널/+24 표시 줄 수/+28 미사용/+2c 변경/+30 줄바꿈 금지다.
// hardLines[lineCount] 는 아직 개행하지 않은 끝 줄이며 완료 줄 수에 포함되지 않는다.
export class StyleTextBufferState {
    // 입력 줄 최대 칸 수/완료 줄 수/표시 줄 수다.
    maxLines = 0; lineCount = 0; softCount = 0;
    // 입력 줄(개행으로 완료된 줄 + 끝 미완료 칸)마다 소유한 조각 체인의 head 다.
    hardLines = new Uint32Array(0);
    // 표시 줄 연결 목록의 센티널(토큰 0)이다.
    sentinel = new StyleTextBufferLine();
    // 안정된 표시 줄 저장소다(토큰 → 줄). 삽입은 기존 줄 주소를 바꾸지 않는다.
    softLines = new Map();
    // 예약 칸/갱신 요청/줄바꿈 금지/다음 자동 표시 줄 토큰이다.
    reserved = 0; dirty = 0; noWrap = 0; nextSoftToken = 1;
    // 초기화가 끝나 저장소를 쓸 수 있는지(소멸 뒤 false)다.
    initialized = false;
    // 원본 +34 통지는 문자열 추가 직전에 입력 바이트를 받는다. 그림 추가에서는 호출하지 않는다.
    onAppend = null;
}

// TextGump 계열이 쓰는 원본 다중 줄 버퍼다. 입력 줄과 표시 복제 조각은 서로 다른 소유권을 가진다.
// 사용: const buffer = new StyleTextBuffer(new StyleTextBufferState(), fragments, style, 128, hooks); buffer.appendText(bytes, context); buffer.layout(width).
export class StyleTextBuffer {
    // 공유 저장소(상태/공통 조각 표/StyleText)와 외부 측정·복제 훅이다.
    #state; #fragments; #style; #hooks;
    // 상태는 새 저장소여야 한다. style 은 fragments 와 같은 조각 표/파생 삭제 경계를 써야 한다. hooks 는 { measureText, cloneExternal } 이다.
    constructor(state, fragments, style, maxLines, hooks = {}) {
        if ((maxLines | 0) !== maxLines || maxLines < 1) throw new TypeError('StyleTextBuffer 양수 최대 줄 수 필요');
        if (state.initialized || state.hardLines.length !== 0 || state.softLines.size !== 0 || state.sentinel.head) throw new TypeError('StyleTextBuffer 새 저장소 필요');
        this.#state = state; this.#fragments = fragments; this.#style = style; this.#hooks = { ...hooks };
        // 원본 생성자처럼 전체 입력 칸을 비우고 표시 센티널/카운터/상태/문자열 통지를 초기화한다.
        state.hardLines = new Uint32Array(maxLines);
        state.maxLines = maxLines; state.lineCount = 0; state.softCount = 0; state.sentinel = new StyleTextBufferLine();
        state.reserved = 0; state.dirty = 0; state.noWrap = 0; state.nextSoftToken = 1; state.onAppend = null; state.initialized = true;
    }

    // 현재 입력/표시 저장소가 살아 있는지 확인한다. 소멸 뒤의 재호출은 호스트 오류로 거부한다.
    #requireStorage() {
        const state = this.#state;
        if (!state.initialized || state.maxLines < 1 || state.hardLines.length !== state.maxLines) throw new TypeError('StyleTextBuffer 입력 저장소 없음');
    }

    // 현재 완료 줄 수에 대응하는 끝 칸을 소유 필드 별칭({ value })으로 돌려준다. 배열 범위 밖 원본 쓰기는 거부한다.
    #currentLine() {
        this.#requireStorage();
        const state = this.#state; const index = state.lineCount;
        if (index < 0 || index >= state.maxLines) throw new RangeError('StyleTextBuffer 끝 줄 범위 초과');
        const lines = state.hardLines;
        return { get value() { return lines[index]; }, set value(value) { lines[index] = value >>> 0; } };
    }

    // 공통 조각 표에서 현재 next/문자열/문맥/태그를 읽는다(그림/문자열과 표시 복제도 같은 표에 있다).
    #fragment(token) {
        const node = token >>> 0 ? this.#fragments.fragments.get(token >>> 0) : null;
        if (!node) throw new RangeError('StyleTextBuffer 공통 조각 없음');
        return node;
    }

    // 0 은 센티널이며 비0 은 안정된 표시 줄 저장소에서 조회한다.
    #line(token) {
        if (!token) return this.#state.sentinel;
        const line = this.#state.softLines.get(token);
        if (!line) throw new RangeError('StyleTextBuffer 표시 줄 없음');
        return line;
    }

    // 표시 줄의 head 를 소유 필드 별칭으로 돌려준다(호출 시점의 줄 객체에 묶인다).
    #lineHead(token) { const line = this.#line(token); return { get value() { return line.head; }, set value(value) { line.head = value >>> 0; } }; }

    // 체인의 각 조각을 next 를 먼저 저장하고 실제 공통 문자열/그림 scalar 삭제로 반납한다. release 경계에서 소유 head 를 관찰할 수 있도록 삭제 중에는 해당 head 를 바꾸지 않는다.
    #deleteChain(head) {
        const seen = new Set();
        // 삭제 전에 next 를 저장한 뒤 조각 하나씩 반납한다.
        while (head) {
            visit(seen, head);
            const next = this.#fragment(head).next;
            this.#style.destroy(head, 1);
            head = next;
        }
    }

    // 문자열 통지는 길이/NUL 탐색 전에 수행한다. 길이가 음수이면 NUL 까지, 아니면 지정 범위를 쓰고 LF 에서만 줄 수를 늘린다.
    // 끝 칸이 차면 완료된 첫 줄을 버리고 모든 칸을 이동한다. 마지막 미완료 구간도 같은 태그를 보관한다. 그림 추가와 달리 문자열 추가는 항상 dirty 를 켠다.
    // 사용: buffer.appendText(bytes, context8, -1, tag). bytes 는 Uint8Array(원문) 또는 명시 UTF16 Uint16Array 다.
    appendText(bytes, context, length = -1, metadata = 0) {
        this.#requireStorage();
        const state = this.#state; const notice = state.onAppend;
        if (notice) notice(bytes);
        length |= 0;
        let count;
        if (length < 0) {
            count = bytes.indexOf(0);
            if (count < 0) throw new TypeError('StyleTextBuffer 음수 길이에 NUL 필요');
        } else count = length;
        if (count > bytes.length) throw new RangeError('StyleTextBuffer 입력 길이 범위 초과');
        if (count > INT32_MAXIMUM) throw new RangeError('StyleTextBuffer 원본 signed 입력 길이 초과');
        let start = 0;
        // LF 앞의 빈 구간도 조각으로 저장한다. CR/탭은 개행으로 정규화하지 않는다.
        for (let position = 0; position < count; position += 1) {
            if (bytes[position] !== 10) continue;
            this.#currentLine();
            if (state.lineCount === state.maxLines - 1) this.removeLine(0);
            const token = this.#style.newText(bytes.subarray(start, position), context, position - start, metadata);
            this.#style.append(this.#currentLine(), token);
            state.lineCount = add(state.lineCount, 1);
            start = position + 1;
        }
        // 마지막 미완료 구간은 끝 칸에 보관하고 완료 줄 수는 늘리지 않는다.
        if (start < count) {
            const token = this.#style.newText(bytes.subarray(start, count), context, count - start, metadata);
            this.#style.append(this.#currentLine(), token);
        }
        state.dirty = 1;
    }

    // 현재 끝 줄에 그림을 더한다. 두 맞춤 값 중 하나라도 비0 이면 Markup.fit 을 호출한다. 줄 수는 늘리지 않고 새 조각의 배율/색 기본값은 Markup 이 정한다.
    // 사용: buffer.appendPicture(markup, type, frame, tag, 65, 60).
    appendPicture(pictures, type, frame, metadata = 0, fitWidth = 0, fitHeight = 0) {
        this.#requireStorage();
        const token = pictures.newPicture(type, frame, metadata);
        if (fitWidth || fitHeight) pictures.fit(token, fitWidth, fitHeight);
        this.#style.append(this.#currentLine(), token);
        this.#state.dirty = 1;
    }

    // 완료 줄을 scalar 삭제한 뒤 미완료 끝 칸까지 포함해 전체 배열을 앞으로 이동한다(끝 칸의 미완료 체인도 함께 옮겨진다).
    // 사용: buffer.removeLine(0) — 완료 줄과 끝 칸 사이만 허용한다.
    removeLine(index) {
        this.#requireStorage();
        const state = this.#state; index |= 0;
        if (index < 0 || index >= state.maxLines - 1 || index >= state.lineCount) throw new RangeError('StyleTextBuffer 삭제 완료 줄 범위 오류');
        this.#deleteChain(state.hardLines[index]);
        // 각 칸을 옮긴 뒤 원래 칸을 0 으로 쓴다. 마지막 미사용 칸도 같은 순서로 비운다.
        for (let position = index + 1; position < state.maxLines; position += 1) { state.hardLines[position - 1] = state.hardLines[position]; state.hardLines[position] = 0; }
        state.lineCount = add(state.lineCount, -1); state.dirty = 1;
    }

    // 입력 원본은 남기고 이전 표시 조각만 scalar 반납한 뒤 센티널 링크를 복구한다. resetCount 는 layout 에서만 적용한다.
    #clearSoft(resetCount) {
        const state = this.#state; const seen = new Set(); let token = state.sentinel.next;
        // 현재 줄의 next 를 삭제 전에 저장하며 마지막에는 센티널로 돌아온다.
        while (token) {
            visit(seen, token);
            const next = this.#line(token).next;
            this.#deleteChain(this.#line(token).head);
            const line = this.#line(token);
            line.head = 0; line.next = 0; line.previous = 0;
            state.softLines.delete(token);
            token = next;
        }
        state.sentinel.next = 0; state.sentinel.previous = 0;
        if (resetCount) state.softCount = 0;
    }

    // 지정 전임 바로 뒤에 빈 표시 줄을 삽입하고 전임 y+높이를 새 y 로 저장한다(이후 다른 줄의 높이가 바뀌어도 다시 계산하지 않는다).
    // 반납된 토큰은 재사용하지 않고 외부에서 먼저 넣은 토큰도 덮어쓰지 않는다. 새 줄 토큰을 돌려준다.
    #newSoftLine(previous) {
        const state = this.#state; const next = this.#line(previous).next;
        let token = state.nextSoftToken;
        // 이미 쓰이는 토큰은 한 칸씩 건너뛴다. 마지막 DWORD 다음은 0 이며 0 은 소진을 뜻한다.
        while (token && state.softLines.has(token)) token = token === DWORD_MAXIMUM ? 0 : token + 1;
        if (!token) throw new RangeError('StyleTextBuffer 표시 줄 토큰 소진');
        const line = Object.assign(new StyleTextBufferLine(), { next, previous, y: add(this.#line(previous).y, this.#line(previous).height) });
        state.softLines.set(token, line);
        state.nextSoftToken = token === DWORD_MAXIMUM ? 0 : token + 1;
        this.#line(next).previous = token; this.#line(previous).next = token;
        state.softCount = add(state.softCount, 1);
        return token;
    }

    // 저장된 NUL 문자열의 측정 범위(NUL 포함)를 구한다. 문맥/태그는 각 복사 직전에 다시 읽는다.
    #textBytes(token) {
        const node = this.#fragment(token);
        if (node.kind !== StyleTextFragmentKind.TEXT) throw new TypeError('StyleTextBuffer 문자열 조각 필요');
        const count = node.text.indexOf(0);
        if (count < 0) throw new TypeError('StyleTextBuffer 저장 문자열에 NUL 필요');
        if (count >= INT32_MAXIMUM) throw new RangeError('StyleTextBuffer 원본 signed 문자열 길이 초과');
        return node.text.subarray(0, count + 1);
    }

    // 폭을 넘는 문자열을 공백 단위로 나눠 현재 줄과 새 줄에 복사하고 마지막 표시 줄 토큰을 돌려준다.
    // 다음 단어까지의 전체 접두 폭(현재 줄 폭 포함)이 제한 폭 미만일 때만 그 단어(뒤 공백 포함)를 받아들인다(정확히 같으면 줄바꿈).
    // 첫 단어가 들어가지 않고 현재 줄 폭이 비0 이면 먼저 새 빈 줄로 옮기고, 빈 줄에서도 크거나 같으면 그 단어를 강제로 배치한다(글자 단위로 쪼개지 않는다).
    // 마지막 단어를 복사할 때는 원래 NUL 도 명시 길이에 포함할 수 있다.
    #splitText(token, line, width) {
        const length = this.#textBytes(token).length - 1;
        let consumed = 0;
        // 이미 배치한 바이트를 제외한 문자열에서 다음 표시 조각을 찾는다.
        while (consumed < length) {
            let accepted = 0; let newLine = false; let copy = true;
            // 이전에 허용한 공백 뒤부터 다음 단어까지의 전체 접두 폭을 측정한다.
            while (consumed + accepted < length) {
                const suffix = this.#textBytes(token).subarray(consumed);
                const end = wordEnd(suffix, accepted);
                const size = this.#hooks.measureText(suffix, this.#fragment(token).context, end);
                if (add(this.#line(line).width, size[0]) >= width) {
                    newLine = true;
                    if (!accepted) {
                        if (this.#line(line).width) copy = false;
                        else accepted = end + 1;
                    }
                    break;
                }
                accepted = end + 1;
            }
            if (copy) {
                const source = this.#fragment(token);
                const fragment = this.#style.newText(this.#textBytes(token).subarray(consumed), source.context, accepted, source.metadata);
                const size = this.#style.measureFragment(fragment);
                this.#line(line).height = Math.max(this.#line(line).height, size[1]);
                this.#line(line).width = add(this.#line(line).width, size[0]);
                this.#style.append(this.#lineHead(line), fragment);
                consumed += accepted;
            }
            if (newLine) line = this.#newSoftLine(line);
        }
        return line;
    }

    // 양수 폭에서 기존 표시 조각을 반납하고 완료 입력 줄만 원본 비교/공백 규칙으로 나눠 표시 줄을 다시 만든다. 폭 0 이하이면 기존 표시 목록도 유지한다.
    // dirty 는 원본처럼 호출자가 관리한다. 마지막 미완료 칸은 순회하지 않는다. 입력 줄마다 빈 표시 줄을 먼저 만든다.
    // 사용: buffer.layout(viewWidth); buffer.totalHeight().
    layout(width) {
        this.#requireStorage();
        width |= 0;
        if (width <= 0) return;
        const state = this.#state;
        this.#clearSoft(true);
        let line = 0;
        // 개행이 완료한 원본 줄만 표시한다.
        for (let index = 0; index < state.lineCount; index += 1) {
            if (index >= state.maxLines) throw new RangeError('StyleTextBuffer 완료 줄 수 범위 초과');
            let token = state.hardLines[index];
            line = this.#newSoftLine(line);
            const seen = new Set();
            // 가상 측정/복제 이후 현재 원본 next 를 다시 읽는다.
            while (token) {
                visit(seen, token);
                if (this.#fragment(token).kind !== StyleTextFragmentKind.TEXT) {
                    const size = this.#style.measureFragment(token);
                    // 비문자 조각의 높이는 이동 전에 현재 줄에 쓴다. 새 줄에서 임의로 다시 보정하지 않는다.
                    this.#line(line).height = Math.max(this.#line(line).height, size[1]);
                    if (add(this.#line(line).width, size[0]) >= width && !state.noWrap && (this.#line(line).head || size[0] < width)) line = this.#newSoftLine(line);
                    this.#line(line).width = add(this.#line(line).width, size[0]);
                    const clone = need(this.#hooks.cloneExternal, 'StyleTextBuffer 비문자 가상 복제 경계 필요')(token);
                    this.#style.append(this.#lineHead(line), clone);
                } else {
                    const size = need(this.#hooks.measureText, 'StyleTextBuffer 현재 문자열 측정 경계 필요')(this.#textBytes(token), this.#fragment(token).context, -1);
                    if (add(this.#line(line).width, size[0]) < width || state.noWrap) {
                        this.#line(line).height = Math.max(this.#line(line).height, size[1]);
                        this.#line(line).width = add(this.#line(line).width, size[0]);
                        const source = this.#fragment(token);
                        const clone = this.#style.newText(this.#textBytes(token), source.context, DWORD_MAXIMUM, source.metadata);
                        this.#style.append(this.#lineHead(line), clone);
                    } else line = this.#splitText(token, line, width);
                }
                token = this.#fragment(token).next;
            }
        }
    }

    // 현재 표시 줄을 softCount 번 순회해 signed 최대 폭을 돌려준다(초기값 0). 센티널이 예상보다 빨리 나오면 잘못된 호스트 카운터를 거부한다.
    maximumWidth() {
        this.#requireStorage();
        let result = 0; let token = this.#state.sentinel.next;
        // 표시 줄 수만큼 연결을 따라가며 최대 폭을 구한다.
        for (let index = 0; index < this.#state.softCount; index += 1) {
            if (!token) throw new RangeError('StyleTextBuffer 표시 줄 수/링크 불일치');
            result = Math.max(result, this.#line(token).width);
            token = this.#line(token).next;
        }
        return result;
    }

    // TextGump 스크롤 상한에 쓰는 마지막 표시 줄의 y+높이다. 표시 줄이 없으면 0 이다.
    totalHeight() {
        this.#requireStorage();
        if (this.#state.softCount < 1) return 0;
        const last = this.#line(this.#state.sentinel.previous);
        return add(last.y, last.height);
    }

    // 완료 입력 줄 → 입력 배열 반납 → 표시 줄 → 센티널 조각 순서로 소유 조각을 반납한다. 카운터는 재초기화하지 않는다.
    // 원본은 미완료 끝 칸을 삭제하지 않는다(같은 누락 유지). 이 칸의 소유 조각은 호출자가 먼저 닫거나 직접 반납해야 한다.
    destroy() {
        this.#requireStorage();
        const state = this.#state;
        // 완료 입력 줄만 scalar 삭제하고 각 해당 칸을 0 으로 쓴다.
        for (let index = 0; index < state.lineCount; index += 1) {
            if (index >= state.maxLines) throw new RangeError('StyleTextBuffer 소멸 완료 줄 수 범위 초과');
            this.#deleteChain(state.hardLines[index]);
            state.hardLines[index] = 0;
        }
        state.hardLines = new Uint32Array(0);
        this.#clearSoft(false);
        this.#deleteChain(state.sentinel.head);
        state.sentinel.head = 0; state.sentinel.next = 0; state.sentinel.previous = 0;
        state.initialized = false;
    }
}

// 실제 GumpText/FontStore 의 첫 폭 표와 별도 논리 슬롯 높이를 명시 길이/NUL 측정에 공급한다. 길이 -1 이면 NUL 까지 잰다.
// 사용: bindStyleTextBufferFonts(hooks, gumpText).
export function bindStyleTextBufferFonts(hooks, text) {
    hooks.measureText = (bytes, context, length) => { const size = { width: 0, height: 0 }; text.measure(bytes, decodeGumpTextContext(context), size, length | 0); return [size.width | 0, size.height | 0]; };
}

// 같은 Markup 의 그림 가상 복제를 공급한다. 복제의 배율은 1·색 치환은 0 으로 새로 만들어진다(입력 조각의 Fit/색 필드를 복사하지 않는다).
// 그림 외 다른 파생 복제는 기존 경계(이전 cloneExternal)를 유지한다.
export function bindStyleTextBufferMarkup(hooks, pictures) {
    const previous = hooks.cloneExternal;
    hooks.cloneExternal = (token) => {
        if (pictures.contains(token)) return pictures.clonePicture(token);
        if (!previous) throw new TypeError('StyleTextBuffer 그림 외 파생 복제 경계 필요');
        return previous(token);
    };
}
