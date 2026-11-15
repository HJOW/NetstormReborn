// 캠페인/기존 Fort가 공유하는 현재 변환 토큰과 실제255BYTE 할당을 소유한다(cpppj client/FortTypeConversionStorage.cpp/.h, 225553df).
// 사용: owner.current().value는 원문 부모가 쓰고 allocate/bytes/release는 할당 수명만 관리한다. 해제는 현재값을 자동으로0으로 바꾸지 않는다. (WB04-E573)
import { DwordBox } from './client-subscriber-globals.js';
import { currentAllocationBytes } from './current-allocation-bytes.js';

// 원본004bd800이 요청하는 정확한 할당 폭이다.
export const FORT_TYPE_CONVERSION_BYTES = 255;
// 같은 Client가 모든 짧은 Session보다 오래 소유하는 변환표 저장소다. 토큰은 호스트 주소가 아니다.
export class FortTypeConversionStorage {
    // 원문 현재DWORD의 같은 참조 상자와 별도의 발급 순서다.
    #current = new DwordBox(0); #next = 0;
    // 현재 포인터에서 밀려난 과거 할당도 명시 release까지 보유한다.
    #allocations = new Map(); #allocationBytes;
    // 웹의 새 배열은0이며 임의 비영 힙은 읽지 않는다. 명시 공급자는 원문 미초기화 입력을 검사할 때 사용한다.
    constructor(allocationBytes = currentAllocationBytes, initialToken = 0) { this.#allocationBytes = allocationBytes; this.#next = initialToken >>> 0; }
    // 조회와 쓰기가 같은 실제DWORD 상자를 사용한다.
    current() { return this.#current; }
    // 확보/등록 성공 뒤에만 발급 상태를 갱신한다. 현재DWORD 대입과 본문의 zero는 앞당기지 않는다.
    allocate(bytes) {
        if (bytes !== FORT_TYPE_CONVERSION_BYTES) throw new RangeError('Fort 변환표의 실제255BYTE 할당 폭');
        if (this.#next === 0xffffffff) throw new RangeError('Fort 변환표 수명 토큰 소진');
        const token = this.#next + 1, block = new Uint8Array(bytes); this.#allocationBytes(block);
        this.#allocations.set(token, block); this.#next = token; return token;
    }
    // 같은 실제 할당을 반환하며 해제되었거나 없는 토큰은 거부한다.
    bytes(token) { const block = this.#allocations.get(token >>> 0); if (!token || !block) throw new RangeError('Fort 변환표의 해제되거나 없는 실제 주소'); return block; }
    // free(nullptr)는 허용한다. 현재DWORD는 원문 부모의 별도 쓰기까지 보존한다.
    release(token) { token >>>= 0; if (!token) return; if (!this.#allocations.delete(token)) throw new RangeError('Fort 변환표의 이미 반납한 실제 주소'); }
}
