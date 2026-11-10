// 10.78 DataManager 의 연결 섹션과 MemStream 버퍼를 원본 소멸 순서로 반납하는 모듈이다(cpppj `o/RawDataManagerLifetime.cpp/.h`, 기준선 1f00a25a 와 같음).
//  - destroyMemStream(0044d040): 비0 버퍼를 먼저 반납한 뒤 pointer/alloc/end/pos 만 0 으로 만든다. 반납이 필드를 바꿔도 반환 뒤 같은 16 바이트를 원본 순서로 비운다.
//  - destroyDataManager(004bd330 → 0044da60): base 가상 표 식별자를 쓰고 현재 head 를 먼저 unlink 한 뒤 각 노드를 소멸한다. 매 반복 후 head 를 다시 읽으므로 외부 효과가 바꾼 head 를 따른다.
//    manager 는 최소 12 바이트(+0 vptr, +4 head, +8 current), 노드는 최소 60 바이트(+0x38 next)이다. manager.current 와 노드 이름/next 는 보존한다.
// 실제 호스트 포인터 대신 DWORD 핸들로 노드와 두 종류의 할당을 조회한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E274: 4-B DataManager 스트림 묶음 이식)

// 0044da60 이 처음 쓰는 10.78 DataManager base 가상 표 식별자이다. (2026-11-10 WB04-E274)
export const DATA_MANAGER_BASE_VTABLE = 0x00507dd8;

// 비정렬 DWORD(리틀 엔디언)를 읽는다. 호스트 포인터로 해석하지 않는다. (2026-11-10 WB04-E274)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 지정한 DWORD 네 바이트만 쓴다. 인접한 이름/링크/예약 필드를 유지한다. (2026-11-10 WB04-E274)
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 0044d040: MemStream 소멸. releaseBuffer(핸들)가 pointer/pos 를 바꿔도 반환 뒤 같은 16 바이트를 pointer, alloc, end, pos 순서로 0 으로 쓴다. (2026-11-10 WB04-E274)
export function destroyMemStream(stream, releaseBuffer) {
    if (stream.length < 16) throw new RangeError('MemStream 소멸의 실제16BYTE 누락');
    const buffer = read(stream, 0);
    if (buffer) {
        if (typeof releaseBuffer !== 'function') throw new Error('MemStream 실제 버퍼 반납 미연결');
        releaseBuffer(buffer);
    }
    put(stream, 0, 0);
    put(stream, 12, 0);
    put(stream, 8, 0);
    put(stream, 4, 0);
}

// 004bd330 → 0044da60: DataManager 소멸. hooks = { stream(핸들) → 노드 60 바이트 이상, releaseBuffer(핸들), releaseStream(핸들) }.
// 빈 목록도 vptr 를 쓰며 current 를 0 으로 만드는 별도 초기화를 하지 않는다. (2026-11-10 WB04-E274)
export function destroyDataManager(manager, hooks) {
    if (manager.length < 12) throw new RangeError('DataManager 소멸의 실제12BYTE 누락');
    let head = read(manager, 4);
    put(manager, 0, DATA_MANAGER_BASE_VTABLE);
    // 앞선 버퍼/반납 효과가 바꾼 현재 head 를 다음 순회에서 다시 읽는다.
    while (head) {
        if (typeof hooks?.stream !== 'function' || typeof hooks.releaseStream !== 'function') throw new Error('DataManager 실제 섹션 조회/반납 미연결');
        const node = read(manager, 4);
        const stream = hooks.stream(node);
        if (stream.length < 60) throw new RangeError('DataManager 실제60BYTE 섹션 노드 누락');
        put(manager, 4, read(stream, 0x38));
        destroyMemStream(stream, hooks.releaseBuffer);
        hooks.releaseStream(node);
        head = read(manager, 4);
    }
}
