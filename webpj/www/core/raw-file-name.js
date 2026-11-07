// 원문 경로 이름 작성기(459c60 Format, 459900 Device, 459b20 Name, 459990 Directory, 459890 Full)를 옮긴 모듈이다(cpppj `o/RawFileName.cpp/.h`, 10.78 전용).
// OS 경로나 유니코드로 다시 해석하지 않는 원본 바이트 작성기이다. 256바이트 칸 10개의 고리를 쓰며, 호출마다 번호를 먼저 1 증가(10 나머지)시키고 그 칸을 0 으로 비운 뒤 작성한다.
// 반환값은 문자열 사본이 아니라 고리 칸의 한 구간을 가리키는 `FileNameView`(칸 번호·시작·길이)이며, 열 번째 후속 호출이 같은 칸을 덮으면 내용도 함께 바뀐다(원문 포인터와 같음). 입력으로 이전 뷰를 다시 넘기면
// 출력 칸을 먼저 비운 다음 그 뷰의 현재 바이트를 읽는다. 문자열 바이트는 글자 하나당 바이트 하나(0..255)이다.
//  - format(parts, separator): 부품 {kind, text}를 순서대로 잇는다(-1 루트·-2 디렉터리·-3 파일·-4 확장자, 0 이 오면 그 뒤는 보지 않음). '/'와 '\\'는 현재 구분자 바이트로 바꾸고 부품 순서 규칙 위반은 TypeError 이며 그때까지 쓴 부분 출력과 번호는 보존한다.
//    256바이트 밖 쓰기는 절단하지 않고 RangeError 로 거부한다. 고정 접두 '.\\'와 연결 문자 '\\'는 현재 구분자와 무관하게 역슬래시이다.
//  - device(fileSpec): UNC 이면 서버 끝, 아니면 콜론 다음(없으면 0)까지 장치 접두를 반환하되 전체 복사본의 해당 위치에 NUL 을 쓴다.
//  - name(fileSpec, extension): 전체를 고리에 복사한 뒤 마지막 구분자/콜론 다음 내부 뷰를 반환한다(없으면 빈 뷰). extension=false 는 선두 dot 이 아닌 끝 확장자만 지운다.
//  - directory(fileSpec, level): 전체(-1) 또는 단계별 디렉터리를 반환한다. 파일 시스템의 parent_path 와 다른 dot/UNC/단계 경계를 가지며 원문 NULL 읽기는 TypeError 로 거부한다.
//  - full(fileSpec, prefix): 같은 칸을 비운 뒤 전체 또는 앞 두 바이트(.\\)를 건너뛴 나머지를 첫 NUL 까지 복사한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E28: 원문 경로 이름 작성기 이식으로 추가)

// 고리 칸 수·칸 크기·기본 구분자(역슬래시)이다. (2026-11-08 WB04-E28)
const SLOT_COUNT = 10;
const SLOT_BYTES = 256;
const BACKSLASH = 0x5c;
// 부품 종류 번호이다. (2026-11-08 WB04-E28)
const ROOT = -1;
const DIRECTORY = -2;
const FILE = -3;
const EXTENSION = -4;

// 고리 칸의 한 구간을 가리키는 뷰이다. text 는 항상 현재 바이트를 읽는다(뒤 호출이 칸을 덮으면 함께 바뀐다). (2026-11-08 WB04-E28)
export class FileNameView {
    // 소유 고리, 칸 번호, 칸 안 시작 위치, 길이이다.
    #owner;
    #slot;
    #offset;
    #length;

    // 고리가 만든 뷰만 쓴다.
    constructor(owner, slot, offset, length) {
        this.#owner = owner;
        this.#slot = slot;
        this.#offset = offset;
        this.#length = length;
    }

    // 칸 번호(0..9)이다.
    get slot() { return this.#slot; }

    // 칸 안에서 이 뷰가 시작하는 위치이다(원문 반환 포인터 − 칸 시작).
    get offset() { return this.#offset; }

    // 반환 시점에 정해진 길이이다.
    get length() { return this.#length; }

    // 현재 바이트로 읽은 문자열이다(NUL 도 그대로 포함).
    get text() {
        const bytes = this.#owner.slotBytes(this.#slot);
        let text = '';
        // 뷰의 길이만큼 현재 바이트를 글자로 옮긴다.
        for (let at = this.#offset; at < this.#offset + this.#length; at += 1) text += String.fromCharCode(bytes[at]);
        return text;
    }

    // 문자열 연결 편의이다.
    toString() { return this.text; }
}

// 입력 인자(문자열 또는 뷰)를 현재 바이트의 문자열로 읽는다. 출력 칸을 비운 뒤에 호출해야 별칭이 원문처럼 비워진 값을 읽는다. (2026-11-08 WB04-E28)
function resolve(input) {
    return typeof input === 'string' ? input : input.text;
}

// 첫 NUL 앞까지만 취한다. (2026-11-08 WB04-E28)
function untilNul(text) {
    const end = text.indexOf('\0');
    return end < 0 ? text : text.slice(0, end);
}

// 같은 Client 의 최초 BSS 0 고리와 번호를 가진 작성기이다. 사용: const names = new RawFileName(); names.format([{ kind: -3, text: 'a.dat' }]).text === '.\\a.dat'. (2026-11-08 WB04-E28)
export class RawFileName {
    // 256바이트 칸 10개와 현재 칸 번호이다(최초 0).
    #buffers = Array.from({ length: SLOT_COUNT }, () => new Uint8Array(SLOT_BYTES));
    #index = 0;

    // 조회는 고리를 전진시키지 않는다. 원문 선증가/10 나머지 뒤의 현재 물리 칸 번호이다.
    get index() { return this.#index; }

    // 현재 칸 256바이트를 빌린다(NUL 뒤 남은 바이트까지).
    currentBytes() { return this.#buffers[this.#index]; }

    // 뷰가 칸 바이트를 읽는 통로이다.
    slotBytes(slot) { return this.#buffers[slot]; }

    // 번호를 먼저 올리고 그 칸을 0 으로 비운 뒤 칸을 돌려준다.
    #advance() {
        this.#index = (this.#index + 1) % SLOT_COUNT;
        const bytes = this.#buffers[this.#index];
        bytes.fill(0);
        return bytes;
    }

    // 문자열을 칸 앞에서부터 복사한다(길이가 256 이상이면 호출자가 먼저 거부).
    #copy(bytes, text) {
        // 글자마다 바이트 하나를 옮긴다.
        for (let at = 0; at < text.length; at += 1) bytes[at] = text.charCodeAt(at) & 0xff;
    }

    // 459c60: 부품 목록을 규칙대로 이어 현재 칸에 쓴다. separator 는 00532728 의 현재 비교/치환 바이트(기본 역슬래시)이다.
    format(parts, separator = BACKSLASH) {
        const bytes = this.#advance();
        const separatorChar = String.fromCharCode(separator);
        let size = 0;
        // 원문 memcpy 의 전체 폭을 확인하고 부분 문자열로 성공시키지 않는다.
        const append = (text) => {
            if (size + text.length >= SLOT_BYTES) throw new RangeError('원문 경로 작성기의 256바이트 물리 범위 초과');
            this.#copy(bytes.subarray(size), text);
            size += text.length;
            bytes[size] = 0;
        };
        // 정상 고리의 이전 칸 끝 바이트도 0 이므로 빈 출력의 원문 [-1] 과 같다.
        const last = () => (size ? bytes[size - 1] : 0);
        let previous = 0;
        let firstDirectory = true;
        // 종료 0 뒤의 부품은 조회하지 않으며 각 문자열은 해당 처리 직전에 읽는다.
        for (const part of parts) {
            if (!part.kind) break;
            let text = untilNul(resolve(part.text));
            if (text.length >= SLOT_BYTES) throw new RangeError('원문 경로 작성기의 지역 256바이트 입력 범위 초과');
            // 두 종류의 구분자를 현재 바이트로 치환한다.
            text = text.replace(/[/\\]/g, separatorChar);
            text = untilNul(text);
            // 같은 문자 선행 연속분만 제거한다(전부 같은 문자면 비게 된다).
            const stripLeading = (character) => {
                let at = 0;
                while (at < text.length && text[at] === character) at += 1;
                text = text.slice(at);
            };
            // 원문 slash 루프는 포인터를 증가시켜 마지막 한 문자만 지운다.
            const stripOneTrailing = () => { if (text.length > 1 && text[text.length - 1] === separatorChar) text = text.slice(0, -1); };
            switch (part.kind) {
            case ROOT:
                if (previous) throw new TypeError('원문 경로 루트는 첫 부품이어야 함');
                stripOneTrailing();
                append(text);
                if (text.length > 0 && text[0] !== separatorChar && text[text.length - 1] !== ':') append(':');
                break;
            case DIRECTORY:
                if (previous === FILE || previous === EXTENSION) throw new TypeError('원문 파일/확장자 뒤 디렉터리 부품');
                if (!firstDirectory) stripLeading(separatorChar);
                firstDirectory = false;
                stripOneTrailing();
                if (!previous) {
                    if (!text.includes(separatorChar) && !text.includes(':')) append('.\\');
                } else if (size && last() !== separator && (text.length === 0 || text[0] !== separatorChar)) append('\\');
                append(text);
                if (last() !== separator) append('\\');
                break;
            case FILE:
                if (previous === FILE || previous === EXTENSION) throw new TypeError('원문 파일/확장자 뒤 파일 부품');
                stripLeading(separatorChar);
                stripOneTrailing();
                if (!previous) { if (!text.includes(separatorChar)) append('.\\'); }
                else if (last() !== separator) append('\\');
                append(text);
                break;
            case EXTENSION:
                if (!previous || previous === ROOT || previous === DIRECTORY) throw new TypeError('원문 파일 없는 확장자 부품');
                stripLeading('.');
                // 확장자의 실제 후행 dot 루프는 감소하므로 전부 제거한다.
                while (text.length > 1 && text[text.length - 1] === '.') text = text.slice(0, -1);
                if (last() !== 0x2e) append('.');
                append(text);
                break;
            default:
                throw new TypeError('원문 경로 작성기의 알 수 없는 부품 종류');
            }
            previous = part.kind;
        }
        return new FileNameView(this, this.#index, 0, size);
    }

    // 459900: 같은 고리를 먼저 증가/비운 뒤 이미 작성된 FileSpec 의 장치 접두를 반환한다. NUL 뒤 바이트도 칸에 보존하며 256바이트 밖은 거부한다.
    device(fileSpec, separator = BACKSLASH) {
        const bytes = this.#advance();
        const separatorChar = String.fromCharCode(separator);
        const spec = untilNul(resolve(fileSpec));
        if (spec.length >= SLOT_BYTES) throw new RangeError('원문 FileSpec 장치 조회의 256바이트 물리 범위 초과');
        this.#copy(bytes, spec);
        let size = spec.length;
        if (size >= 2 && spec[0] === separatorChar && spec[1] === separatorChar) {
            // UNC: 서버 끝(세 번째 구분자)에서 끊는다.
            const end = spec.indexOf(separatorChar, 2);
            if (end >= 0) { bytes[end] = 0; size = end; }
        } else {
            // 드라이브: 콜론 다음까지, 없으면 비어 있다.
            const colon = spec.indexOf(':');
            size = colon < 0 ? 0 : colon + 1;
            bytes[size] = 0;
        }
        return new FileNameView(this, this.#index, 0, size);
    }

    // 459b20: 전체를 칸에 복사한 뒤 마지막 구분자/콜론 다음 내부 뷰를 반환한다. 구분자가 없으면 첫 바이트만 NUL 이고 빈 뷰이다.
    name(fileSpec, extension = true, separator = BACKSLASH) {
        const bytes = this.#advance();
        const spec = untilNul(resolve(fileSpec));
        if (spec.length >= SLOT_BYTES) throw new RangeError('원문 FileSpec 이름 조회의 256바이트 물리 범위 초과');
        this.#copy(bytes, spec);
        const end = spec.length;
        let offset = end;
        let found = false;
        // 마지막 구분자/콜론을 뒤에서 찾아 그 다음 위치를 빌린다.
        while (offset > 0) {
            offset -= 1;
            if (bytes[offset] === separator || bytes[offset] === 0x3a) { offset += 1; found = true; break; }
        }
        if (!found) { bytes[0] = 0; return new FileNameView(this, this.#index, 0, 0); }
        let size = end - offset;
        if (!extension && size > 1) {
            // 459550 은 선두 dot 을 확장자로 지우지 않으며 구분자/콜론을 만나면 중단한다.
            for (let at = end - 1; at > offset; at -= 1) {
                if (bytes[at] === 0x2e) { bytes[at] = 0; size = at - offset; break; }
                if (bytes[at] === separator || bytes[at] === 0x3a) break;
            }
        }
        return new FileNameView(this, this.#index, offset, size);
    }

    // 459990: 전체(-1)/단계별 디렉터리와 부분 버퍼를 같은 고리에 반환한다. 원문 NULL 읽기는 현재 칸을 보존한 TypeError 로 거부한다.
    directory(fileSpec, level = -1, separator = BACKSLASH) {
        const bytes = this.#advance();
        const separatorChar = String.fromCharCode(separator);
        const spec = untilNul(resolve(fileSpec));
        if (spec.length >= SLOT_BYTES) throw new RangeError('원문 FileSpec 디렉터리 조회의 256바이트 물리 범위 초과');
        // 현재 첫 NUL 까지의 길이이다(뒤에 남은 이전 복사 바이트는 지우지 않는다).
        const outputLength = () => { const end = bytes.indexOf(0); return end < 0 ? SLOT_BYTES : end; };
        const output = () => new FileNameView(this, this.#index, 0, outputLength());
        // 원문 strcpy 처럼 이번 끝 NUL 까지만 덮어쓴다.
        const copySuffix = (from) => {
            const suffix = spec.slice(from);
            this.#copy(bytes, suffix);
            bytes[suffix.length] = 0;
        };
        let cursor = spec.startsWith(separatorChar.repeat(2)) ? 2 : 0;
        // -1 은 첫 suffix 에서 반환하고 나머지는 명시 단계만 순회한다.
        for (let step = 0; step <= level || level === -1; step += 1) {
            let cut = spec.indexOf(separatorChar, cursor);
            if (cut < 0) throw new TypeError('원문 디렉터리 조회의 NULL 구분자 읽기');
            if (cursor < cut && spec[cut - 1] === '.') cut -= 1;
            if (level === -1) {
                copySuffix(cut);
                const size = outputLength();
                // 원문 역방향 탐색은 offset 0 의 구분자를 검사하지 않는다.
                for (let at = size ? size - 1 : 0; at > 0; at -= 1) {
                    if (bytes[at] === separator) { bytes[at + 1] = 0; break; }
                }
                return output();
            }
            cursor = cut + 1;
            if (spec.indexOf(separatorChar, cursor) < 0) {
                if (step === 0 && level < 1) { bytes[0] = separator; bytes[1] = 0; } else bytes[0] = 0;
                return output();
            }
            copySuffix(cut);
            let next = -1;
            // 출력의 둘째 바이트부터 다음 구분자를 찾는다.
            for (let at = 1; at < outputLength(); at += 1) { if (bytes[at] === separator) { next = at; break; } }
            if (next < 0) throw new TypeError('원문 디렉터리 단계 구분자 없음');
            bytes[next] = 0;
        }
        const size = outputLength();
        if (!size || bytes[size - 1] !== separator) {
            if (size + 1 >= SLOT_BYTES) throw new RangeError('원문 디렉터리 연결의 256바이트 물리 범위 초과');
            // 원문 고정 연결문자는 현재 separator 와 독립적인 역슬래시이다.
            bytes[size] = BACKSLASH;
            bytes[size + 1] = 0;
        }
        return output();
    }

    // 459890: 같은 칸을 비운 뒤 전체 또는 무조건 +2 주소에서 첫 NUL 까지 복사한다. prefix=false 는 두 바이트 이상의 물리 입력을 요구한다.
    full(fileSpec, prefix = true) {
        const bytes = this.#advance();
        let spec = resolve(fileSpec);
        if (!prefix) {
            if (spec.length < 2) throw new TypeError('원문 Full 의 +2 물리 입력 범위가 없음');
            spec = spec.slice(2);
        }
        spec = untilNul(spec);
        if (spec.length >= SLOT_BYTES) throw new RangeError('원문 Full 의 256바이트 물리 범위 초과');
        this.#copy(bytes, spec);
        return new FileNameView(this, this.#index, 0, spec.length);
    }
}
