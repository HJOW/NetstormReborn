// 7ah 압축 객체를 35BYTE 공통 갱신으로 복호화한다(cpppj RawSquidCompressedReceive.cpp, 10.78).
// 사용: decodeCompressedSquid(packet, transmissionState, { defaultType, type, maximumHp, assertion }).
// 관찰기는 observeCompressedSquid(packet, sending, assertion)로 송신 길이를 갱신한다.

// 선택한 실제 장치가 없으면 해당 호출 위치에서 중단한다.
function need(effect) { if (typeof effect !== 'function') throw new Error('압축 객체 수신의 선택된 실제 장치 미연결'); return effect; }

// 패킷의 압축 필드를 원문 순서로 소비하고 현재 owner 캐시를 갱신한다. (2026-11-18 WB04-E735)
export function decodeCompressedSquid(packet, state, hooks) {
    if (!(packet instanceof Uint8Array) || packet.length < 9) throw new RangeError('압축 객체의9BYTE 머리 부족');
    const count = packet[8];
    if (count > 64 || packet.length < count + 9) throw new RangeError('압축 객체의 실제64BYTE 지역 복사 범위');
    const data = packet.slice(9, 9 + count), packetView = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
    const mask = packetView.getUint16(6, true), image = new Uint8Array(35), view = new DataView(image.buffer);
    let index = 0;
    image[0] = state.update.protocol & 0x7f; image[1] = state.update.message;
    view.setUint16(2, state.update.length, true); view.setUint16(4, packetView.getUint16(4, true), true);
    view.setFloat32(6, -10000, true); view.setFloat32(10, -10000, true);
    image[16] = need(hooks.defaultType)(); image[18] = state.lastOwner;
    // 충돌 진단이 복귀하면 원문처럼 뒤 필드를 계속 처리한다.
    const diagnose = (expression, line) => need(hooks.assertion)(expression, '.\\Clientboard.cpp', line);
    // 실제 소비할 범위만 검사하고 앞선 효과는 되돌리지 않는다.
    const copy = (destination, width, byteRead = false) => {
        if (index + width > count) { diagnose('index <= dataLength', byteRead ? 0x128 : 0x137); throw new RangeError('압축 객체의 미정의 지역 스택 읽기'); }
        image.set(data.subarray(index, index + width), destination); index += width;
    };
    // 16개 마스크 항목과 타입 기본 HP는 원본의 고정 순서로 소비한다.
    for (let field = 0; field < 16; field += 1) {
        if (mask & (1 << field)) switch (field) {
        case 0: {
            if (mask & 2) diagnose('!testMask( i+1 )', 0x14f);
            copy(6, 1, true); const x = image[6]; view.setFloat32(6, x, true);
            copy(10, 1, true); const y = image[10]; view.setFloat32(10, y, true); break;
        }
        case 1: if (mask & 1) diagnose('!testMask( i-1 )', 0x15c); copy(6, 8); break;
        case 2: if (mask & 8) diagnose('!testMask( i+1 )', 0x169); copy(14, 1); break;
        case 3: if (mask & 4) diagnose('!testMask( i-1 )', 0x174); copy(14, 2); break;
        case 4: copy(16, 1); break;
        case 5: copy(17, 1); break;
        case 6: image[17] |= 0x10; break;
        case 7:
            if (mask & 0x100) diagnose('!testMask( i+1 )', 0x1af);
            if ((state.lastOwner >>> 0) === 0xffffff9d) diagnose('hackySameAsLastPlayerId != CRAZY_PLAYER_ID', 0x1b0);
            image[18] = state.lastOwner; break;
        case 8: if (mask & 0x80) diagnose('!testMask( i-1 )', 0x1bd); copy(18, 1); state.lastOwner = image[18]; break;
        case 9: if (mask & 0x400) diagnose('!testMask( i+1 )', 0x1cb); copy(27, 1); break;
        case 10: if (mask & 0x200) diagnose('!testMask( i-1 )', 0x1d6); copy(27, 2); break;
        case 11: copy(29, 1); break;
        case 12: copy(30, 1); break;
        case 13: copy(34, 1); break;
        default: break;
        }
        if (field === 4) {
            const selected = need(hooks.type)(image[16]);
            if (selected.flags1 & 0x10) view.setUint16(27, need(hooks.maximumHp)(selected), true);
            image[29] = selected.zOrder;
        }
    }
    return image;
}

// 송신 관찰은 진단 뒤의 현재 길이로 WORD를 쓰며 수신 관찰은 패킷을 읽지 않는다.
export function observeCompressedSquid(packet, sending, assertion) {
    if (!sending) return;
    if (!(packet instanceof Uint8Array) || packet.length < 9) throw new RangeError('압축 객체 송신 관찰기의 머리 부족');
    if (packet[8] === 0xff) need(assertion)('dataLength != (unsigned char)-1', '\\Ns\\O\\_p_packets.cpp', 0xb83);
    new DataView(packet.buffer, packet.byteOffset, packet.byteLength).setUint16(2, packet[8] + 9, true);
}
