// Client 수명의 원본 임시 문자열 고리와 별도 파일 이름 고리를 소유한다.
// 사용: new ClientStaticStrings(fileNames).initialize(10240), reserve(size), formatCaptureRange(first, last).
import { RawFileName } from './raw-file-name.js';
import { initializeStaticStrings, reserveStaticString } from './raw-static-strings.js';

// 원본 주소와 혼동하지 않는 호스트 할당 토큰 최초값과 할당 간격이다.
const FIRST_ADDRESS = 0x20000000;
const ADDRESS_GAP = 4096;

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
