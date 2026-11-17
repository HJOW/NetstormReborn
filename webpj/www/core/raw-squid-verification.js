// PATCH 4ah/4bh 객체 검증과 이전 SID 간격 순회이다(cpppj RawSquidVerification.cpp).
// 사용: beginSquidVerification(pool, packet, state, hooks), receiveFreeSquidVerification(pool, packet, state, hooks); state는 같은 sid/previous DWORD를 빌린다.

// 선택된 진단/이름/로그만 호출 시점에 요구한다.
function need(effect) { if (typeof effect !== 'function') throw new Error('객체 검증의 선택된 실제 장치 미연결'); return effect; }
// 판본과 DWORD SID를 원본 물리 조회 전에 확인한다.
function requirePool(pool) { if (pool?.edition !== 'patch1078') throw new TypeError('객체 검증의 PATCH 계약'); }
function sidOf(pool, value) { if (value > 65535 || value >= pool.capacity) throw new RangeError('객체 검증의 실제 SID 물리 범위'); return value; }
// 두 바이트 WORD를 zero-extend한다.
function packetSid(packet) { return packet[4] | (packet[5] << 8); }
// 저장된 숫자 인수와 현재 TYPE 이름을 원본 서식의 같은 로그로 전달한다.
function logDifference(pool, hooks, sid, field, server, client) {
    const name = String(need(hooks.typeName)(sidOf(pool, sid))).split('\0', 1)[0];
    const message = `VERIFY ${sid >>> 0} (client type=${name}): ${field} ${server >>> 0} != ${client >>> 0}\n`;
    if (message.length >= 512) throw new RangeError('객체 검증의 실제 로그 범위');
    need(hooks.log)(0x1d, message);
}

// 이전 SID 다음부터 현재 SID 직전까지 같은 물리를 순회하고 각 로그 뒤 슬롯을 다시 읽는다.
export function advanceSquidVerification(pool, state, hooks) {
    requirePool(pool);
    if (!state.previous) { state.previous = state.sid; return; }
    // DWORD 증가와 signed 비교를 원본 순서로 적용하며 현재 SID는 조사하지 않는다.
    while (state.previous !== state.sid) {
        state.previous = (state.previous + 1) >>> 0;
        if ((state.previous | 0) >= 65535) state.previous = 15000;
        if (state.previous === state.sid) return;
        const raw = pool.physical(sidOf(pool, state.previous)), freeOrForm = (raw[11] & 1) || raw[10] < 70;
        if (!freeOrForm) logDifference(pool, hooks, state.previous, 'on server is free||form, but not on client', 1, 0);
        if (raw[10]) logDifference(pool, hooks, state.previous, 'server type != client type', 0, raw[10]);
    }
}

// 4bh 진단 복귀 뒤 현재 입력 SID를 쓰고 같은 이전 간격을 확인한다.
export function beginSquidVerification(pool, packet, state, hooks) {
    requirePool(pool); if (!(packet instanceof Uint8Array) || packet.length < 6) throw new RangeError('검증 시작의6BYTE 부족');
    if (state.sid) need(hooks.assertion)('!sidToVerify', '.\\Clientboard.cpp', 0x4c);
    state.sid = packetSid(packet); advanceSquidVerification(pool, state, hooks);
}

// 4ah는 현재 슬롯의 free/TYPE 비교와 간격 확인 뒤 검증 SID만 지운다.
export function receiveFreeSquidVerification(pool, packet, state, hooks) {
    requirePool(pool); if (!(packet instanceof Uint8Array) || packet.length < 7) throw new RangeError('자유 객체 검증의7BYTE 부족');
    const incoming = packetSid(packet), raw = pool.physical(sidOf(pool, incoming)); state.sid = incoming;
    if (!(raw[11] & 1)) logDifference(pool, hooks, incoming, 'isFree', 1, raw[11] & 1);
    if (packet[6] !== raw[10]) logDifference(pool, hooks, state.sid, 'freeType', packet[6], raw[10]);
    advanceSquidVerification(pool, state, hooks); state.sid = 0;
}
