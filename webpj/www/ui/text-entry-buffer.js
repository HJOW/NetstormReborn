// 원본 0x28바이트 문자 편집 버퍼(TextEntryBuffer)를 StyleTextBuffer 와 구별해 옮긴다(원문 00451a80/00451680/004516c0/00451710/00451930/00451550/00451950~00451990/004518f0/004518c0/00451b10/00451800,
// cpppj client/TextEntryBuffer.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 본문/줄 offset 배열은 같은 가상 주소 공간의 DWORD 주소(토큰)이며 주소+내부 offset 합산을 허용한다. 할당/해제/재할당/주소 조회/clipboard 는 호스트 훅이고(보통 TextEntryStorage),
// 이 클래스는 원문의 부분 실패·현재 재조회·선택·LF 줄 offset 규칙만 가진다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다).
// 기준선 뒤 변경(3.6절): cpppj 작업본의 6차 한국어 확장 표시 토큰 분기(Paste 의 maxLength·DisplayTextInput 경로)는 기준선에 없고 웹은 UTF-16 문자열을 쓰므로 이식하지 않는다(이식 범위 밖, 기준선 Paste 만).
// 사용: const state = new TextEntryBufferState(); const buffer = new TextEntryBuffer(state, { allocate, release, resize, bytes, clipboardRead, clipboardWrite }); buffer.construct(); buffer.insert('hello'); buffer.text() === 'hello'.
// (2026-11-14 WB04-E438: RawMenuControls 선행 문자 편집 버퍼 이식)

// 원본 가상표/기본 본문/줄표 할당 크기다.
const BUFFER_VTABLE = 0x00508da8; const INITIAL_DATA = 0x800; const OFFSET_BYTES = 0x200;
// signed 32비트의 하위 비트를 유지하는 보기다.
const signed = (value) => value | 0;
// ADD/SUB 의 하위 DWORD 만 남긴 signed 결과다(호스트 signed 넘침을 피한다).
const add = (a, b) => (a + b) | 0;
const subtract = (a, b) => (a - b) | 0;
// 원시 주소는 부호 확장 없이 32비트에서 합산한다.
const address = (base, offset) => ((base >>> 0) + (offset >>> 0)) >>> 0;
// 호스트 범위를 원본 signed 복사 길이로만 쓴다(음수는 RangeError, out_of_range).
function count(value) {
    if (value < 0) throw new RangeError('편집버퍼 음수메모리길이');
    return value;
}
// 입력을 바이트 배열로 바꾼다. 문자열은 바이트 문자열(글자 하나가 한 바이트, 하위 8비트만)이고 JS 문자열은 길이를 알아 끝 NUL 을 요구하지 않는다. null/undefined/빈 입력은 빈 배열이다.
function bytesOf(text) {
    if (text === null || text === undefined) return new Uint8Array(0);
    if (typeof text === 'string') return Uint8Array.from(text, (character) => character.charCodeAt(0) & 0xff);
    return text;
}
// NUL 이 포함된 입력만 빌린다. 빈 입력은 원문의 null 문자열(길이 0)이고, 바이트 배열에서 NUL 이 없으면 TypeError(invalid_argument)다. 문자열은 첫 NUL 이 있으면 거기서 끝난다.
function cStringLength(text) {
    const bytes = bytesOf(text);
    if (bytes.length === 0) return 0;
    const end = bytes.indexOf(0);
    if (end < 0) {
        if (typeof text === 'string') return bytes.length;
        throw new TypeError('편집버퍼 NUL입력범위 필요');
    }
    return end;
}
// 원본 REP MOVSD→MOVSB 처럼 앞에서부터 4바이트씩 읽고 쓴다. 유효한 겹침도 그 순서를 보존한다. 범위 밖이면 RangeError 다.
function forwardCopy(target, source, total) {
    if (total > target.length || total > source.length) throw new RangeError('편집버퍼 복사범위');
    let offset = 0;
    // 원문 DWORD 복사 순서다: 네 바이트를 먼저 읽은 뒤 쓴다.
    for (; total - offset >= 4; offset += 4) {
        const word = [source[offset], source[offset + 1], source[offset + 2], source[offset + 3]];
        target[offset] = word[0]; target[offset + 1] = word[1]; target[offset + 2] = word[2]; target[offset + 3] = word[3];
    }
    // 마지막 0~3바이트도 앞에서부터 복사한다.
    for (; offset < total; offset += 1) target[offset] = source[offset];
}

// 원본 열 DWORD 상태다(+0 vtable, +4 줄 offset 배열 주소, +8 offsetCapacity, +c lineCount, +10 dirty, +14 선택 시작, +18 선택 끝, +1c 길이, +20 용량, +24 본문 주소).
export class TextEntryBufferState {
    // 할당 가상표와 줄 offset 배열 주소다.
    vtable = 0; offsets = 0;
    // 줄표의 DWORD 칸 수와 현재 표시 줄 수(signed)다.
    offsetCapacity = 0; lineCount = 0;
    // 0 이 아니면 LF 줄 offset 을 다시 만든다.
    dirty = 0;
    // 선택 시작/끝과 본문 길이/용량(signed)이다.
    selectionBegin = 0; selectionEnd = 0; length = 0; capacity = 0;
    // 현재 NUL 문자열 저장 주소다.
    data = 0;
}

// 실제 TextEntry 가 소유하는 상태/메모리는 이 처리기보다 오래 살아야 한다. bytes 조회는 효과를 만들지 않고 allocate/release/resize/clipboard 는 현재 state 를 바꿀 수 있다.
export class TextEntryBuffer {
    // 같은 실제 0x28 필드를 가진 상태와 실제 소유 메모리 훅이다. hooks = { allocate(bytes)→주소|0, release(주소), resize(주소, bytes)→주소|0, bytes(주소)→Uint8Array(남은 범위), clipboardRead(Uint8Array(256)), clipboardWrite(Uint8Array) }.
    // 메모리 네 훅은 필수이고 clipboard 는 실제 호출 분기에서만 필수다.
    #state; #hooks;
    constructor(state, hooks) {
        if (!hooks || ['allocate', 'release', 'resize', 'bytes'].some((name) => typeof hooks[name] !== 'function')) throw new TypeError('편집버퍼 실제메모리장치 필요');
        this.#state = state; this.#hooks = hooks;
    }

    // 토큰은 원시 주소이며 필요한 물리 폭을 소유 저장소에서 검사한다(범위 밖 RangeError).
    #memory(base, total) {
        if (!total) return new Uint8Array(0);
        const bytes = base ? this.#hooks.bytes(base) : new Uint8Array(0);
        if (total > bytes.length) throw new RangeError('편집버퍼 현재메모리범위');
        return bytes.subarray(0, total);
    }
    // 줄표의 정렬 조건을 호스트 주소로 가정하지 않는다(정렬되지 않은 little-endian DWORD 읽기).
    #read(where) { const bytes = this.#memory(where, 4); return (bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24)) >>> 0; }
    // 원본 little-endian DWORD 쓰기다.
    #write(where, value) { const bytes = this.#memory(where, 4); for (let index = 0; index < 4; index += 1) bytes[index] = (value >>> (8 * index)) & 0xff; }
    // NUL 앞에서만 LF 를 찾으며 원문 현재 포인터를 DWORD 로 보존한다. 없으면 0, NUL 없이 범위가 끝나면 RangeError 다.
    #findNewline(where) {
        const bytes = where ? this.#hooks.bytes(where) : new Uint8Array(0);
        for (let index = 0; index < bytes.length; index += 1) {
            if (!bytes[index]) return 0;
            if (bytes[index] === 10) return (where + index) >>> 0;
        }
        throw new RangeError('편집버퍼 줄검색NUL범위');
    }

    // 00451a80: 요청 0 이면 2048바이트·줄 offset 512바이트를 할당한다. 본문 첫 바이트는 아직 쓰지 않는다. 원문은 요청 크기와 무관하게 성공 시 capacity 에 2048을 쓴다.
    construct(requested = 0) {
        const s = this.#state;
        s.vtable = BUFFER_VTABLE; s.selectionBegin = s.selectionEnd = 0; s.capacity = 0;
        const data = this.#hooks.allocate((requested >>> 0) || INITIAL_DATA); s.data = data >>> 0;
        if (data) s.capacity = INITIAL_DATA;
        s.length = 0; s.selectionBegin = s.selectionEnd = 0;
        const offsets = this.#hooks.allocate(OFFSET_BYTES); s.lineCount = 0; s.offsets = offsets >>> 0; s.offsetCapacity = 128; s.dirty = 1;
    }

    // 00451680: data 가 0 이 아닐 때 본문 반납 → data/capacity 0 → 현재 offset 반납 → offset 0 이다. data 가 0 이면 줄표도 반납하지 않는 원문 조건을 보존한다.
    destroy() {
        const s = this.#state;
        s.vtable = BUFFER_VTABLE;
        if (!s.data) return;
        this.#hooks.release(s.data); s.data = 0; s.capacity = 0; this.#hooks.release(s.offsets); s.offsets = 0;
    }

    // 004516c0: 두 signed 위치를 정렬하고 현재 length 로 clamp 한다(방향은 상태에 저장하지 않는다).
    select(begin, end) {
        const s = this.#state; begin |= 0; end |= 0;
        if (end < begin) [begin, end] = [end, begin];
        if (begin < 0) begin = 0; else if (s.length < begin) begin = s.length;
        if (end < begin) end = begin; else if (s.length < end) end = s.length;
        s.selectionBegin = begin; s.selectionEnd = end;
    }

    // 00451710: NUL 앞 바이트로 선택을 교체한다. 빈 입력은 원문의 null→빈 문자열이다. 길이/원래 선택 폭은 콜백 전에 고정하지만 재할당 뒤 본문/현재 선택 끝/현재 length 를 다시 읽는다.
    // 재할당 실패(0)는 아무것도 바꾸지 않고 돌아온다. 입력 범위는 할당 콜백 동안 유효해야 하며 본문 재할당으로 무효가 되는 자기 참조는 호출자가 피한다.
    insert(text) {
        const s = this.#state; const size = cStringLength(text); const source = bytesOf(text);
        const added = size; const removed = subtract(s.selectionEnd, s.selectionBegin);
        if (removed < added) {
            const required = add(subtract(s.length, removed), added);
            if (s.capacity < required) {
                // 필요 길이-1 을 1024 단위로 올림한 새 용량이다(부호 없는 32비트 산술, required 가 0 이하이면 0).
                const last = ((s.length >>> 0) - (removed >>> 0) + (added >>> 0) - 1) >>> 0;
                const capacity = required > 0 ? (((last >>> 10) + 1) << 10) >>> 0 : 0;
                const data = this.#hooks.resize(s.data, capacity);
                if (!data) return;
                s.data = data >>> 0; s.capacity = signed(capacity);
            }
        }
        if (removed !== added) {
            const from = address(s.data, s.selectionEnd); const to = address(from, subtract(added, removed)); const total = count(subtract(s.length, s.selectionEnd));
            if (total) { const target = this.#memory(to, total); const origin = this.#memory(from, total); target.set(origin); }
            s.length = add(s.length, subtract(added, removed));
        }
        const destination = address(s.data, s.selectionBegin); forwardCopy(this.#memory(destination, size), source.subarray(0, size), size);
        const selection = add(added, s.selectionBegin); const end = address(s.data, s.length);
        s.selectionEnd = selection; s.selectionBegin = selection; this.#memory(end, 1)[0] = 0; s.dirty = 1;
    }

    // 00451930: 첫 NUL·길이·선택·dirty 를 초기화한 뒤 같은 insert 를 실행한다(원본 tailcall 로 입력을 같은 순서로 읽으므로 자기 본문 reset 의 첫 NUL 도 보존된다).
    reset(text) {
        const s = this.#state;
        this.#memory(s.data, 1)[0] = 0; s.length = 0; s.selectionBegin = s.selectionEnd = 0; s.dirty = 1; this.insert(text);
    }

    // 00451550: 실제 LF 줄 offset 과 마지막 length 를 쓴다. 할당 실패는 부분 상태(이미 쓴 첫 offset/줄 수/dirty)를 유지하고 false 다. dirty 가 0 이면 true 다.
    ensureLines() {
        const s = this.#state;
        if (!s.dirty) return true;
        s.lineCount = 1; this.#write(s.offsets, 0);
        let newline = this.#findNewline(s.data);
        // 원문 검색 포인터는 메모리 장치 콜백 동안 고정하고 다음 검색에도 재사용한다.
        while (newline) {
            newline = (newline + 1) >>> 0;
            this.#write(address(s.offsets, (s.lineCount >>> 0) * 4 | 0), (newline - s.data) >>> 0); s.lineCount = add(s.lineCount, 1);
            if (s.lineCount >= s.offsetCapacity) {
                const offsets = this.#hooks.allocate((((s.offsetCapacity >>> 0) * 4) + OFFSET_BYTES) >>> 0);
                if (!offsets) return false;
                const total = ((s.offsetCapacity >>> 0) * 4) >>> 0;
                forwardCopy(this.#memory(offsets, total), this.#memory(s.offsets, total), total);
                this.#hooks.release(s.offsets); const capacity = s.offsetCapacity; s.offsets = offsets >>> 0; s.offsetCapacity = add(capacity, 128);
            }
            newline = this.#findNewline(newline);
        }
        this.#write(address(s.offsets, ((s.lineCount >>> 0) * 4) | 0), s.length >>> 0); s.dirty = 0;
        return true;
    }

    // 00451950: ensureLines 결과와 별개로(실패 뒤에도) 현재 줄 수를 돌려준다.
    lineCount() { this.ensureLines(); return this.#state.lineCount; }

    // 00451960: 줄 번호가 범위 밖이면 -1, 아니면 마지막 offset 이 length 이므로 LF 가 있는 행의 길이에는 LF 가 포함된다.
    lineLength(line) {
        this.ensureLines();
        const s = this.#state; line |= 0;
        if (line < 0 || line >= s.lineCount) return -1;
        const where = address(s.offsets, ((line >>> 0) * 4) | 0); const next = (where + 4) >>> 0;
        return signed(this.#read(next) - this.#read(where));
    }

    // 00451990: 줄 본문과 NUL·선택 구간 쌍을 쓴다. 범위/용량 거부는 0xffffffff 이며 출력을 보존한다. 선택 쌍은 줄 복사 성공 뒤에만 쓴다(selection 은 길이 2 배열, 마지막 length 위치의 빈 선택도 원문 조건).
    copyLine(line, output, selection) {
        this.ensureLines();
        const s = this.#state; line |= 0;
        if (line < 0 || line >= s.lineCount) return 0xffffffff;
        const where = address(s.offsets, ((line >>> 0) * 4) | 0); const next = (where + 4) >>> 0;
        if (output.length > 0x7fffffff) throw new RangeError('편집버퍼 출력DWORD범위');
        const lineBytes = (this.#read(next) - this.#read(where)) >>> 0;
        if (output.length < add(signed(lineBytes), 1)) return 0xffffffff;
        const size = count(signed(lineBytes)); if (size >= output.length) throw new RangeError('편집버퍼 줄복사물리범위');
        forwardCopy(output, this.#memory(address(s.data, this.#read(where)), size), size); output[size] = 0;
        const begin = s.selectionBegin;
        if (begin < s.length && (signed(this.#read(next)) <= begin || s.selectionEnd < signed(this.#read(where)))) { selection[0] = -1; selection[1] = -1; return lineBytes; }
        selection[0] = signed(this.#read(where)) < begin ? subtract(begin, signed(this.#read(where))) : 0;
        selection[1] = signed(this.#read(next)) <= s.selectionEnd ? signed(lineBytes) : subtract(s.selectionEnd, signed(this.#read(where)));
        return lineBytes;
    }

    // 004518f0: 현재 전체 NUL 문자열 복사다. 용량 거부는 0xffffffff 이며 출력에 손대지 않는다. 성공은 실제 NUL 까지 복사하고 length 를 돌려준다.
    copyAll(output) {
        const s = this.#state;
        if (output.length > 0x7fffffff) throw new RangeError('편집버퍼 출력DWORD범위');
        if (output.length < add(s.length, 1)) return 0xffffffff;
        const data = this.data(); const end = data.indexOf(0);
        if (end < 0) throw new RangeError('편집버퍼 본문NUL범위');
        const size = end + 1;
        if (size > output.length) throw new RangeError('편집버퍼 전체복사물리범위');
        output.set(data.subarray(0, size)); return s.length >>> 0;
    }

    // 004518c0: 선택이 비어 있으면 전체 본문을 clipboard 에 복사한다(저장된 선택은 바꾸지 않는다). 길이 0 은 효과가 없다.
    copy() {
        const s = this.#state; let begin = s.selectionBegin; let end = s.selectionEnd;
        if (begin === end) { begin = 0; end = s.length; }
        const size = subtract(end, begin);
        if (size <= 0) return;
        if (typeof this.#hooks.clipboardWrite !== 'function') throw new Error('편집버퍼 실제clipboard쓰기 없음');
        this.#hooks.clipboardWrite(this.#memory(address(s.data, begin), count(size)));
    }

    // 00451b10: 빈 선택은 전체 선택으로 바꾼 뒤 복사 → null 삽입 → dirty 1 이다.
    cut() {
        const s = this.#state;
        if (s.selectionBegin === s.selectionEnd) { s.selectionBegin = 0; s.selectionEnd = s.length; }
        this.copy(); this.insert(null); s.dirty = 1;
    }

    // 00451800: clipboard 256바이트(NUL 문자열)에서 제외문자들을 지운 뒤 비어 있지 않으면 삽입한다. 제외문자를 지울 때 현재 칸을 다시 검사하며 빈 clipboard 도 dirty 1 을 남긴다.
    // excluded 는 바이트 문자열/NUL 이 든 바이트 배열이다(기준선 동작, 6차 한국어 확장 토큰 분기는 이식하지 않는다).
    paste(excluded = null) {
        if (typeof this.#hooks.clipboardRead !== 'function') throw new Error('편집버퍼 실제clipboard읽기 없음');
        const text = new Uint8Array(256); this.#hooks.clipboardRead(text); const length = cStringLength(text);
        const excludedBytes = bytesOf(excluded);
        if (excludedBytes.length) {
            const excludedLength = cStringLength(excluded); let index = 0;
            // 원문은 제외문자 삭제 시 현재 칸을 다시 검사한다.
            while (index < text.length && text[index]) {
                if (!excludedBytes.subarray(0, excludedLength).includes(text[index])) index += 1;
                else text.copyWithin(index, index + 1, length + 1);
            }
            text[text.length - 1] = 0;
        }
        if (text[0]) this.insert(text);
        this.#state.dirty = 1;
    }

    // 같은 실제 0x28 필드 참조를 돌려준다(TextEntry/Console 이 현재 필드를 직접 재조회한다).
    state() { return this.#state; }

    // 현재 주소의 실제 남은 저장 범위(문자열/선택/Draw 가 같은 바이트를 쓴다)다. 입력/출력 모두 현재 data 를 조회하며 이전 재할당 뷰를 재사용하지 않는다.
    data() { return this.#state.data ? this.#hooks.bytes(this.#state.data) : new Uint8Array(0); }

    // 원본 length 앞 바이트의 바이트 문자열(복사본)이다. 다음 재할당/해제 이후에는 다시 조회해야 한다.
    text() {
        const bytes = this.#memory(this.#state.data, count(this.#state.length));
        let result = '';
        // 큰 본문도 호출 스택을 넘지 않도록 나눠 만든다.
        for (let at = 0; at < bytes.length; at += 8192) result += String.fromCharCode(...bytes.subarray(at, at + 8192));
        return result;
    }
}
