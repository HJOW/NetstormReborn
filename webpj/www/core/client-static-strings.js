// Client 수명의 원본 임시 문자열 고리와 별도 파일 이름 고리를 소유한다.
// 사용: new ClientStaticStrings(fileNames).initialize(10240), reserve(size), formatCaptureRange(first, last).
import { RawFileName } from './raw-file-name.js';
import { formatStaticString, initializeStaticStrings, reserveStaticString } from './raw-static-strings.js';

// 원본 주소와 혼동하지 않는 호스트 할당 토큰 최초값과 할당 간격이다.
const FIRST_ADDRESS = 0x20000000;
const ADDRESS_GAP = 4096;
// 고리 밖 서식 바이트에는 실제 할당 토큰과 겹치지 않는 원본 대체 주소를 쓴다.
const EXTERNAL_FORMAT_ADDRESS = 0x10000000;

// 한 정수 인자의 원본 CRT 변환·폭·정밀도를 좁은 문자열로 만든다.
function formatIntegerPattern(pattern, value) {
    let output = '', argumentsUsed = 0;
    // 서식을 처음부터 읽으며 미지원 변환과 미공급 두 번째 인자를 거부한다.
    for (let index = 0; index < pattern.length;) {
        if (pattern[index] !== '%') { output += pattern[index++]; continue; }
        if (pattern[index + 1] === '%') { output += '%'; index += 2; continue; }
        const match = /^%([-+ #0]*)(\d*)(?:\.(\d*))?([diuoxXc])/.exec(pattern.slice(index));
        if (!match) throw new Error('임시 고리 한 정수 외 CRT 서식 미연결');
        if (++argumentsUsed > 1) throw new Error('임시 고리 실제 값인자 하나 초과');
        const [, flags, widthText, precisionText, kind] = match, width = Number(widthText || 0);
        const precision = precisionText === undefined ? null : Number(precisionText || 0), signed = kind === 'd' || kind === 'i';
        const number = signed ? value | 0 : value >>> 0; let prefix = '', digits;
        if (kind === 'c') digits = String.fromCharCode(number & 255);
        else {
            const magnitude = signed ? Math.abs(number) : number, radix = kind === 'o' ? 8 : kind === 'x' || kind === 'X' ? 16 : 10;
            digits = magnitude.toString(radix); if (kind === 'X') digits = digits.toUpperCase();
            if (precision === 0 && magnitude === 0) digits = '';
            if (precision !== null) digits = digits.padStart(precision, '0');
            if (signed) prefix = number < 0 ? '-' : flags.includes('+') ? '+' : flags.includes(' ') ? ' ' : '';
            else if (flags.includes('#') && kind === 'o' && !digits.startsWith('0')) prefix = '0';
            else if (flags.includes('#') && magnitude !== 0 && (kind === 'x' || kind === 'X')) prefix = kind === 'x' ? '0x' : '0X';
        }
        const padding = Math.max(width - prefix.length - digits.length, 0);
        if (flags.includes('-')) output += prefix + digits + ' '.repeat(padding);
        else if (flags.includes('0') && (precision === null || kind === 'c')) output += prefix + '0'.repeat(padding) + digits;
        else output += ' '.repeat(padding) + prefix + digits;
        index += match[0].length;
    }
    return output;
}

// 현재 실제 할당을 유지하며 원본 공통 고리 부모를 같은 네 전역에 연결한다.
export class ClientStaticStrings {
    // 같은 원본 네 DWORD와 실제 할당 목록, 다음 토큰, 파일 이름 고리다.
    #globals = { size: 0, base: 0, current: 0, last: 0 };
    #allocations = new Map(); #nextBase = FIRST_ADDRESS; #fileNames; #assertion;

    // 기존 파일 이름 고리가 있으면 그대로 빌리고 진단은 선택된 위치에서만 실행한다.
    constructor(fileNames = null, assertion = (line) => console.info(`Staticstring 원문 진단:${line}`)) {
        this.#fileNames = fileNames ?? new RawFileName(); this.#assertion = assertion;
    }

    // 같은 전역과 파일 이름 고리의 실제 참조를 제공한다.
    globals() { return this.#globals; }
    // 파일 경로의 별도 고리는 임시 문자열 예약으로 초기화하지 않는다.
    fileNames() { return this.#fileNames; }

    // 요청한 크기를 독립 물리로 만들고 겹치지 않는 DWORD 토큰을 발급한다.
    #allocate(size) {
        const next = this.#nextBase + size + ADDRESS_GAP;
        if (next > 0xffffffff) throw new RangeError('임시 고리 주소 토큰 소진');
        const address = this.#nextBase; this.#allocations.set(address, new Uint8Array(size)); this.#nextBase = next; return address;
    }

    // 지정 주소부터 같은 할당의 끝까지 실제 뷰를 빌린다.
    #tail(address) {
        let selected = null;
        // 원본 상한 탐색처럼 주소보다 크지 않은 마지막 할당을 찾는다.
        for (const [base, bytes] of this.#allocations) if (base <= address && (!selected || base > selected.base)) selected = { base, bytes };
        if (!selected || address - selected.base > selected.bytes.length) throw new RangeError('임시 고리 실제 주소 범위');
        return selected.bytes.subarray(address - selected.base);
    }

    // 원본 store 전에 전체 물리 폭을 검사한다.
    #memory(address, size) {
        const bytes = this.#tail(address); if (size > bytes.length) throw new RangeError('임시 고리 원문 접근 물리 폭'); return bytes.subarray(0, size);
    }

    // 고리 본체가 같은 할당과 진단을 사용하도록 장치를 빌려준다.
    #hooks() {
        return { allocate: (size) => this.#allocate(size), memory: (address, size) => this.#memory(address, size), assertion: (line) => this.#assertion(line) };
    }

    // 명시 시작은 지정 크기를 쓰며 이전 할당의 수명은 유지한다.
    initialize(size) { initializeStaticStrings(size, this.#globals, this.#hooks()); }

    // 예약 폭이 아닌 실제 할당 끝까지 같은 뷰를 돌려준다.
    reserve(size) { return this.#tail(reserveStaticString(size, this.#globals, this.#hooks())); }

    // 원본 정수 하나와 NUL 종료 서식을 같은 고리에 작성하며 목적과 서식이 같으면 작성하지 않는다.
    formatInteger(size, pattern, value) {
        if (!(pattern instanceof Uint8Array)) throw new TypeError('임시 고리 정수 서식 BYTE 배열 필요');
        let source = EXTERNAL_FORMAT_ADDRESS;
        // 이전 할당의 같은 물리 주소라면 별칭 판정에 실제 토큰을 쓴다.
        for (const [base, bytes] of this.#allocations) {
            if (pattern.buffer === bytes.buffer && pattern.byteOffset >= bytes.byteOffset && pattern.byteOffset < bytes.byteOffset + bytes.length) {
                source = base + pattern.byteOffset - bytes.byteOffset; break;
            }
        }
        const hooks = this.#hooks();
        // 서식은 예약 뒤 현재 바이트를 읽고 실제 물리 끝을 넘는 앞쪽 쓰기를 보존한다.
        hooks.format = (destination) => {
            const end = pattern.indexOf(0); if (end < 0) throw new RangeError('임시 고리 정수 서식의 NUL 범위');
            let sourceText = '';
            // C 좁은 문자열의 각 BYTE를 같은 문자 코드로 옮긴다.
            for (let index = 0; index < end; index += 1) sourceText += String.fromCharCode(pattern[index]);
            const text = formatIntegerPattern(sourceText, value | 0);
            // 원본은 예약 폭이 아니라 물리 할당 끝까지 NUL 포함 쓴다.
            for (let index = 0; index <= text.length; index += 1) this.#memory((destination + index) >>> 0, 1)[0] = index === text.length ? 0 : text.charCodeAt(index);
        };
        return this.#tail(formatStaticString(size, source, this.#globals, hooks));
    }

    // 연속 캡처 안내를 80바이트 예약 뒤 실제 물리 끝까지 NUL 포함 작성한다.
    formatCaptureRange(first, last) {
        const bytes = this.reserve(80), text = `Wrote ${String(first).split('\0', 1)[0]} through ${String(last).split('\0', 1)[0]}.\n`;
        // 원본의 좁은 문자열 바이트와 종료 NUL을 앞에서부터 쓴다.
        for (let index = 0; index <= text.length; index += 1) {
            if (index >= bytes.length) throw new RangeError('연속 저장 안내의 실제 공유 고리 물리');
            const value = index === text.length ? 0 : text.charCodeAt(index);
            if (value > 255) throw new RangeError('연속 저장 안내의 원본 바이트 범위');
            bytes[index] = value;
        }
        return bytes;
    }
}
