// 단일·집합 패킷 프레임을 같은 등록 표와 실제 연결에 분배한다(cpppj o/RawPacketFrame.cpp, 225553df).
// 사용: dispatchPacketFrame(dispatch, connection, frame, hooks). frame은 관찰기와 수신기가 함께 쓰는 변경 가능한 Uint8Array다.
// 소켓 지연 FIFO는 웹 범위 밖이며 선택된 수신기가 있는 지연 연결은 RawPacketDispatch가 거부한다.

// 원본 공통 헤더 폭과 집합 타입, 연결 종료 비트다.
const HEADER = 4;
const BATCH = 4;
const CLOSED = 0x40;

// 폐기된 CD 유효성 검증 요청과 응답은 처리하거나 기록하지 않는다.
function retired(type) { return type === 0x7f || type === 0x80; }

// 실제 프레임의 두 바이트 길이를 little endian으로 읽는다.
function word(bytes, offset) { return bytes[offset] | (bytes[offset + 1] << 8); }

// 원본 순서대로 프레임을 검사하고 각 콜백 뒤 현재 등록과 연결을 다시 읽는다.
export function dispatchPacketFrame(dispatch, connection, frame, hooks) {
    if (frame.length >= HEADER && retired(frame[1])) return !(connection.flags & CLOSED);
    if (frame.length < HEADER || frame[1] !== BATCH) return dispatch.dispatch(connection, frame);
    if (connection.flags & CLOSED) return false;
    const total = word(frame, 2);
    let cursor = HEADER;
    if (total > frame.length) { hooks.terminate(connection); return false; }
    // 각 내부 패킷은 선언 범위와 등록 정책을 통과한 뒤 순서대로 소비한다.
    while (cursor < total) {
        if (total - cursor < HEADER) { hooks.terminate(connection); return false; }
        const length = word(frame, cursor + 2);
        if (!length || length > total - cursor) { hooks.terminate(connection); return false; }
        const packet = frame.subarray(cursor, cursor + length);
        cursor += length;
        if (!dispatch.validate(packet) || retired(packet[1])) continue;
        if (packet[1] === BATCH) throw new Error('공통 중첩 집합 패킷 미복원');
        const type = packet[1], now = hooks.now();
        if (type === 6) connection.heartbeat = now; else connection.received = now;
        const registration = dispatch.registration(type), incoming = packet[0] & 0x7f;
        if (incoming !== registration.version) {
            const lower = incoming < registration.version;
            if (registration.mismatch & (lower ? 8 : 4)) { hooks.terminate(connection); return false; }
            if (registration.mismatch & (lower ? 2 : 1)) continue;
        }
        const pin = registration.pin();
        try {
            hooks.record(connection, packet, length);
            dispatch.registration(type).notifyObserver(packet, 0);
            dispatch.deliverReceiver(type, connection, packet);
        } finally { pin.release(); }
    }
    if (cursor !== total) { hooks.terminate(connection); return false; }
    if (!connection.depth && (connection.flags & CLOSED)) { hooks.close(connection); hooks.release(connection); return false; }
    return true;
}
