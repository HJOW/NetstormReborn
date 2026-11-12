// 4dece0 의 observer 호출 뒤 실제 길이/순방향 DWORD·BYTE 복사와, Client 수명의 소유 복제 token 장부(`RawPacketCopies`)를 옮긴 모듈이다(cpppj `o/RawPacketCopy.h/.cpp`, 10.78 전용, 기준선 1f00a25a 뒤 추가된 새 파일, 3.6절 ③).
//  - duplicateRawPacket(packet, capacity, destination, dispatch): packet 이 null 이면 아무것도 하지 않고 present=false 로 조기 반환한다. 아니면 BYTE 1 의 타입 항목을 읽어 호출 동안 핀을 유지하고 observer 에 방향 1(송신)로 알린 뒤
//    **그 시점의 WORD 2 길이**를 읽는다(observer 가 길이를 바꿔도 현재 값을 쓴다). destination 이 없고 capacity 가 0 이면 길이만큼 새로 할당(allocation)하고 present=true, 아니면 capacity < length 이면 빈 결과, 그 밖은 destination 에
//    낮은 주소부터 DWORD 단위 `floor(length/4)` 번 복사한 뒤 남은 0~3BYTE 를 같은 방향으로 복사한다(memmove 가 아니라 앞방향 복사라 겹쳐도 원문 순서). 읽기/쓰기 물리가 부족하면 그 지점에서 RangeError 이다.
//  - RawPacketCopies: 복제본과 State+80 의 포인터 대용 DWORD token 을 Client 최종 반납까지 보관한다. token 은 1 부터 재사용 없이 발급하고 0 은 원문 NULL 이다. duplicate(packet, dispatch)는 원문 4dece0(packet, 0, NULL)의 실제 소유 할당만
//    장부에 넣고 NULL 결과는 0 을 돌려준다. adopt(bytes)는 이미 소유한 바이트를 그대로 인계하고, acquire(token)은 같은 바이트를 돌려주며(없으면 오류, 빈 Zone 으로 바꾸지 않음), release(token)은 0 을 무시하고 현재 token 을 한 번만 반납한다.
// 웹 적응: C++ shared_ptr 수명 대신 JS GC 를 쓴다(① 판정). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E335: 4-D ClientLocalStateGlobals 의 Zone 소유 장부로 추가)

// 복제 결과(present: 원문 반환 포인터의 존재 여부, bytes: 복사된 길이만큼의 목적지, allocation: 자동 할당 분기의 전체 바이트)이다. 길이 0 의 소유 할당과 NULL 결과를 구별한다. (2026-11-12 WB04-E335)
function emptyResult() {
    return { present: false, bytes: null, allocation: null };
}

// 바이트 배열에서 리틀 엔디언 값을 읽는다. 범위가 모자라면 RangeError 이다. (2026-11-12 WB04-E335)
function read(bytes, at, width) {
    if (at > bytes.length || width > bytes.length - at) throw new RangeError('패킷 복제의 실제 읽기 물리 부족');
    let value = 0;
    // 낮은 바이트부터 조합한다(상위 바이트는 2^24 곱으로 부호 없이 유지).
    for (let index = 0; index < width; index += 1) value += bytes[at + index] * 2 ** (index * 8);
    return value;
}

// 바이트 배열에 리틀 엔디언 값을 쓴다. 범위가 모자라면 RangeError 이다. (2026-11-12 WB04-E335)
function write(bytes, at, value, width) {
    if (at > bytes.length || width > bytes.length - at) throw new RangeError('패킷 복제의 실제 쓰기 물리 부족');
    // 낮은 바이트부터 쓴다.
    for (let index = 0; index < width; index += 1) bytes[at + index] = Math.floor(value / 2 ** (index * 8)) & 0xff;
}

// 4dece0: observer 알림 뒤 현재 길이로 복사한다. dispatch 는 `registration(type)` 이 `pin()`/`notifyObserver(packet, sending)` 을 가진 항목을 돌려주는 RawPacketDispatch 이다. (2026-11-12 WB04-E335)
export function duplicateRawPacket(packet, capacity, destination, dispatch) {
    if (!packet) return emptyResult();
    const entry = dispatch.registration(read(packet, 1, 1));
    const pin = entry.pin();
    try {
        entry.notifyObserver(packet, 1);
        const length = read(packet, 2, 2);
        let allocation = null;
        let present;
        let target = destination ?? null;
        // 목적지가 없고 capacity 가 0 이면 길이만큼 자동 할당한다.
        if (!target && capacity === 0) {
            allocation = new Uint8Array(length);
            target = allocation;
            present = true;
        } else {
            if ((capacity | 0) < length) return emptyResult();
            present = target !== null;
        }
        // 목적지가 없는데 capacity 만 충분한 입력은 길이 0 이 아니면 첫 쓰기에서 물리 부족 오류가 난다(빈 목적지 취급).
        const sink = target ?? new Uint8Array(0);
        let offset = 0;
        // 낮은 주소부터 DWORD 수만큼 복사한다(겹치는 목적지도 원문 순서).
        for (let word = 0; word < (length >>> 2); word += 1, offset += 4) write(sink, offset, read(packet, offset, 4), 4);
        // 남은 0~3BYTE 를 같은 방향으로 복사한다.
        for (let byte = 0; byte < (length & 3); byte += 1, offset += 1) write(sink, offset, read(packet, offset, 1), 1);
        return { present, bytes: sink.subarray(0, length), allocation };
    } finally {
        pin.release();
    }
}

// Client 수명의 소유 복제본과 State+80 의 포인터 대용 token 장부이다. 사용: const copies = new RawPacketCopies(); const token = copies.duplicate(packet, dispatch); copies.acquire(token); copies.release(token). (2026-11-12 WB04-E335)
export class RawPacketCopies {
    // token → 소유 바이트 장부이다(미소비/덮어쓴 token 의 물리도 Client 최종 반납까지 보유).
    #copies = new Map();
    // 다음에 발급할 token 이다(0 은 원문 NULL 이며 재사용하지 않는다).
    #nextToken;

    // 최초 token 1 만 준비한다. firstToken 은 소진 경계를 시험하는 웹 시험 장치이며 실제 Client 는 쓰지 않는다. (2026-11-12 WB04-E335)
    constructor({ firstToken = 1 } = {}) {
        this.#nextToken = firstToken >>> 0;
    }

    // 완성된 실제 할당의 소유권을 장부에 넣은 뒤에만 token 을 공개한다. 소진되면 오류이다. (2026-11-12 WB04-E335)
    #hold(allocation) {
        if (!allocation) throw new Error('소유 패킷 복제의 실제 할당 누락');
        if (!this.#nextToken) throw new RangeError('소유 패킷 복제의 DWORD token 소진');
        const token = this.#nextToken;
        this.#copies.set(token, allocation);
        this.#nextToken = (this.#nextToken + 1) >>> 0;
        return token;
    }

    // 원문 4dece0(packet, 0, NULL)의 실제 소유 할당만 장부에 넣는다. NULL 결과(패킷 없음·capacity 부족)는 token 0 이다. (2026-11-12 WB04-E335)
    duplicate(packet, dispatch) {
        const copy = duplicateRawPacket(packet, 0, null, dispatch);
        return copy.present ? this.#hold(copy.allocation) : 0;
    }

    // 이미 소유한 바이트를 복사 없이 그대로 인계하고 token 을 돌려준다(새 observer 호출도 없음). (2026-11-12 WB04-E335)
    adopt(bytes) {
        return this.#hold(bytes);
    }

    // 현재 token 의 같은 바이트를 돌려준다. token 0 이거나 장부에 없으면 오류이며 빈 Zone 으로 바꾸지 않는다. (2026-11-12 WB04-E335)
    acquire(token) {
        const found = this.#copies.get(token >>> 0);
        if (!token || found === undefined) throw new RangeError('소유 Zone의 현재 token 물리 누락');
        return found;
    }

    // token 0 은 무시하고 현재 token 은 한 번만 반납한다. 다른 State 필드는 호출자가 원문 위치에서 지운다. (2026-11-12 WB04-E335)
    release(token) {
        if (token && !this.#copies.delete(token >>> 0)) throw new RangeError('소유 Zone의 현재 token 해제 누락/중복');
    }

    // 장부에 남은 복제본 수이다(시험·진단용). (2026-11-12 WB04-E335)
    get size() {
        return this.#copies.size;
    }
}
