// 편집 버퍼의 실제 할당과 32비트 주소+오프셋 조회, 0x28 편집 버퍼 객체의 수명을 공통 UI 수명에 소유한다(cpppj client/TextEntryStorage.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 원문 편집 버퍼(본문/줄 offset 배열)와 메모리 주소를 고정 소유자로 유지한다: 단조 증가 가상 주소(0x20000000..0x60000000 미만)에 블록을 두고 주소+내부 오프셋 조회를 허용한다.
// 웹 적응: C++ 은 미초기화 new[] 를 쓰지만 JS Uint8Array 는 0 으로 채워진다(원문의 미정 값 하나로 본다). 시험은 내용에 의존하지 않는다. 전용 PE fixture 는 없다.
// 사용: const storage = new TextEntryStorage(readClipboard, writeClipboard); const buffer = storage.create(token); buffer.construct(); buffer.insert('hi'); buffer.text() === 'hi'; buffer.destroy(); storage.releaseBuffer(token).
// (2026-11-14 WB04-E438: RawMenuControls 선행 문자 편집 저장소 이식)
import { TextEntryBuffer, TextEntryBufferState } from './text-entry-buffer.js';

// 첫 가상 주소와 소진 한계(이 값 이상이 되면 Error)다. 간격은 요청 폭 + 31 을 16 의 배수로 내린 값이다.
const FIRST_ADDRESS = 0x20000000;
const ADDRESS_LIMIT = 0x60000000;

// 편집 저장소 한 개다. 객체 토큰(0x28 객체)과 메모리 주소를 구별한다.
export class TextEntryStorage {
    // 주소순 블록 목록(주소 → { bytes, size })과 오름차순 주소 배열(upper_bound 조회용)이다. 주소는 단조 증가하므로 새 주소는 항상 배열 끝에 붙는다.
    #blocks = new Map(); #addresses = [];
    // 객체 토큰 → { state, handler } 이다(handler 보다 오래 사는 실제 0x28 논리 필드).
    #buffers = new Map();
    // 다음에 줄 가상 주소다.
    #next = FIRST_ADDRESS;
    // 실제 clipboard 읽기/쓰기 장치다(선택된 입력 분기에서만 호출한다).
    #read; #write;

    // clipboard 는 실제 선택된 입력 분기에서만 호출하므로 둘 다 선택 사항이다. read(Uint8Array(256)) 는 NUL 문자열을 채우고 write(Uint8Array) 는 복사한다.
    constructor(read = null, write = null) {
        this.#read = read; this.#write = write;
    }

    // 같은 고정 상태/처리기를 새 0x28 객체 토큰에 구성한다. 토큰 0 이나 중복은 Error 다. 반환한 처리기는 map 삽입/새 객체 추가로 바뀌지 않는다.
    create(token) {
        token >>>= 0;
        if (!token || this.#buffers.has(token)) throw new Error('편집 버퍼 객체 토큰 중복');
        const state = new TextEntryBufferState();
        const hooks = {
            allocate: (size) => this.allocate(size), release: (address) => this.release(address), resize: (address, size) => this.resize(address, size), bytes: (address) => this.bytes(address),
            clipboardRead: (bytes) => { if (typeof this.#read !== 'function') throw new Error('실제 clipboard 읽기 미연결'); this.#read(bytes); },
            clipboardWrite: (bytes) => { if (typeof this.#write !== 'function') throw new Error('실제 clipboard 쓰기 미연결'); this.#write(bytes); },
        };
        const handler = new TextEntryBuffer(state, hooks);
        this.#buffers.set(token, { state, handler });
        return handler;
    }

    // 살아 있는 실제 버퍼 처리기만 돌려준다(없으면 null).
    find(token) { return this.#buffers.get(token >>> 0)?.handler ?? null; }

    // 버퍼 본문 destroy 뒤 객체 기록 자체를 반납한다. 기록이 없거나 본문(data)/줄표(offsets)가 아직 살아 있으면 Error 다(살아 있는 본문을 잃지 않는다).
    releaseBuffer(token) {
        token >>>= 0;
        const found = this.#buffers.get(token);
        if (!found || found.state.data || found.state.offsets) throw new Error('편집 버퍼 본문 반납 전 객체 해제');
        this.#buffers.delete(token);
    }

    // 실제 new[] 를 확보한다(OOM 은 0). 주소 범위 소진은 Error 다. 원문 미정 바이트를 임의 sentinel/0 으로 채우는 것에 의존하지 않는다(웹은 0 으로 채워진다).
    allocate(size) {
        size >>>= 0;
        const step = Math.floor((size + 31) / 16) * 16;
        if (this.#next + step >= ADDRESS_LIMIT) throw new Error('편집 메모리 주소 범위 소진');
        let bytes;
        try { bytes = new Uint8Array(size || 1); } catch (error) { if (error instanceof RangeError) return 0; throw error; }
        const address = this.#next;
        this.#next += step;
        this.#blocks.set(address, { bytes, size }); this.#addresses.push(address);
        return address;
    }

    // 같은 할당의 시작만 free 한다. null(0)은 허용하며 내부 오프셋 포인터를 free 로 잘못 쓰면 Error 다.
    release(address) {
        address >>>= 0;
        if (address && !this.#blocks.delete(address)) throw new Error('편집 메모리 해제 주소 불일치');
        if (address) this.#addresses.splice(this.#addresses.indexOf(address), 1);
    }

    // 실제 새 저장소가 성공한 뒤에만 원본을 반납한다. 주소 0 은 새 할당, 크기 0 은 해제 뒤 0, 실패 시 원래 할당을 유지하고 0 이다. 성공은 내용을 min(새 크기, 옛 크기) 만큼 보존해 이동한다.
    resize(address, size) {
        address >>>= 0; size >>>= 0;
        if (!address) return this.allocate(size);
        const found = this.#blocks.get(address);
        if (!found) throw new Error('편집 메모리 재할당 주소 불일치');
        if (!size) { this.release(address); return 0; }
        const next = this.allocate(size);
        if (!next) return 0;
        this.#blocks.get(next).bytes.set(found.bytes.subarray(0, Math.min(size, found.size)));
        this.release(address);
        return next;
    }

    // 같은 물리에서 주소 + 오프셋 뒤 남은 범위를 빌린다(같은 할당의 끝도 빈 범위로 표현할 수 있다). 없는 주소/오프셋 범위 초과는 RangeError 다.
    bytes(address) {
        address >>>= 0;
        // upper_bound(address) 직전(주소 이하 중 가장 큰) 블록을 이분 탐색한다.
        let low = 0; let high = this.#addresses.length;
        while (low < high) {
            const middle = (low + high) >>> 1;
            if (this.#addresses[middle] <= address) low = middle + 1; else high = middle;
        }
        if (low === 0) throw new RangeError('편집 메모리 주소 없음');
        const start = this.#addresses[low - 1]; const block = this.#blocks.get(start); const offset = address - start;
        if (offset > block.size) throw new RangeError('편집 메모리 오프셋 범위');
        return block.bytes.subarray(offset, block.size);
    }

    // 게임 값을 바꾸지 않는 실제 소유 수 관측이다.
    bufferCount() { return this.#buffers.size; }
    // 문자/줄표/제외문자의 현재 할당 수다.
    allocationCount() { return this.#blocks.size; }
}
