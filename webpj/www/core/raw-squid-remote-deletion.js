// PATCH 53h/62h 원격 삭제, 62h 송신 관찰기, 2dh 내용물 수신을 처리한다(cpppj RawSquidMutationReceive.cpp).
// 사용: receiveRemoteDeletedSquid(pool, packet, destroy) 또는 receiveRemoteDeletedSquids(pool, packet, destroy), observeRemoteDeletedSquids(packet, sending, assertion).
// 내용물은 receiveContentSquid(pool, packet, { contentType, take, word, firstPop, attach })로 실제 슬롯에 반영한다.

// 선택한 효과만 호출 시점에 요구한다.
function need(effect) { if (typeof effect !== 'function') throw new Error('객체 변경 수신의 선택된 실제 장치 미연결'); return effect; }
// 현재 패킷의 해당 WORD만 물리 폭을 확인해 읽는다.
function word(packet, at) {
    if (!(packet instanceof Uint8Array) || at < 0 || at + 2 > packet.length) throw new RangeError('원격 삭제의 현재 WORD 물리 부족');
    return packet[at] | (packet[at + 1] << 8);
}
// 현재 count DWORD를 다시 읽어 callback이 바꾼 다음 반복 상한을 보존한다.
function count(packet) {
    if (!(packet instanceof Uint8Array) || packet.length < 8) throw new RangeError('원격 다중 삭제의 현재 count DWORD 물리 부족');
    return new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getInt32(4, true);
}
// 두 원격 삭제 수신은 같은 PATCH Pool을 사용한다.
function requirePool(pool) { if (pool?.edition !== 'patch1078') throw new TypeError('원격 삭제의 PATCH 판본 불일치'); }

// 53h는 SID 0과 물리 상한 밖만 건너뛰고, 현재 슬롯이 free가 아니면 flags 0으로 삭제한다.
export function receiveRemoteDeletedSquid(pool, packet, destroy) {
    requirePool(pool); const sid = word(packet, 4);
    if (!sid || sid >= pool.capacity) return;
    if (!(pool.slot(sid)[11] & 1)) need(destroy)(sid, 0);
}

// 62h는 전체를 선검사하지 않고 각 현재 SID와 호출 뒤의 현재 count를 원본 순서로 읽는다.
export function receiveRemoteDeletedSquids(pool, packet, destroy) {
    requirePool(pool); let limit = count(packet); if (limit <= 0) return;
    let index = 0;
    // 앞선 실제 삭제를 유지하며 각 WORD만 소비 위치에서 읽는다.
    do {
        const sid = word(packet, 8 + index * 2);
        if (!(pool.slot(sid)[11] & 1)) need(destroy)(sid, 0);
        limit = count(packet); index += 1;
    } while (index < limit);
}

// 송신 관찰기는 count -1만 진단하고 진단 복귀 뒤 현재 하위 WORD로 길이를 쓴다.
export function observeRemoteDeletedSquids(packet, sending, assertion) {
    if (!sending) return;
    if (count(packet) === -1) need(assertion)('count != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x9ac);
    const length = ((word(packet, 4) + 4) * 2) & 0xffff; packet[2] = length & 0xff; packet[3] = length >>> 8;
}

// 2dh 내용물 수신은 Take 전 상태를 기억하고 현재 Pool의 필드·첫 Pop·부착을 원본 순서로 처리한다. (2026-11-18 WB04-E731)
export function receiveContentSquid(pool, packet, hooks) {
    if (pool?.edition !== 'patch1078' || !(packet instanceof Uint8Array) || packet.length < 18) throw new TypeError('내용물 수신의 PATCH18BYTE 계약');
    const incoming = word(packet, 4);
    const oldState = pool.slot(incoming)[11];
    const object = need(hooks.take)(need(hooks.contentType)(), incoming);
    need(hooks.word)(object, word(packet, 6));

    const raw = pool.physical(object);
    const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    view.setUint16(18, word(packet, 8), true);
    view.setUint16(20, word(packet, 10), true);
    view.setUint16(24, word(packet, 12), true);
    raw[23] = packet[16];

    const received = packet[17];
    raw[11] = received | 4;
    let flags = 8;
    if ((received & 0x10) || (oldState & 1)) {
        flags = (need(hooks.firstPop)(object) | 8) >>> 0;
        raw[11] &= 0xef;
    }
    need(hooks.attach)(object, word(packet, 14), flags);
}

// 65h 감시 Form의 필드를 현재 슬롯에 기록한 다음 부모에 부착한다. (2026-11-18 WB04-E732)
export function receiveDependencySquid(pool, packet, hooks) {
    if (pool?.edition !== 'patch1078' || !(packet instanceof Uint8Array) || packet.length < 13) throw new TypeError('감시 수신의 PATCH13BYTE 계약');
    const object = need(hooks.take)(need(hooks.dependType)(), word(packet, 4));
    const raw = pool.physical(object);
    const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    raw[11] = packet[8];
    view.setUint16(18, word(packet, 9), true);
    view.setUint16(20, word(packet, 11), true);
    raw[11] |= 4;
    need(hooks.dependencyAttach)(object, word(packet, 6), 0);
}
