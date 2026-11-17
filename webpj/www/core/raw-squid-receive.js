// PATCH 00433c30의 공통 객체35BYTE 수신과 검증 비교를 같은 Pool/현재 전역에 적용한다(cpppj RawSquidReceive.cpp/.h, 8ee7666b).
// 사용: receiveRawSquid(pool, packet, { verifySid, typeWas, noMakeWarnings }, hooks). 선택된 실제 생성·배치·진단 장치는 호출자가 공급한다.
import { setRawSquidHitPoints } from './raw-squid-word.js';

// 선택된 효과만 실제 호출 위치에서 요구한다.
function need(effect, name) { if (typeof effect !== 'function') throw new Error(`객체 수신의 실제 ${name} 장치 미연결`); return effect; }
// 비정렬 원문에서 지정 폭만 현재 바이트로 읽는다.
function read(bytes, at, width = 4) {
    if (!(bytes instanceof Uint8Array) || at < 0 || at + width > bytes.length) throw new RangeError('객체 수신의 실제 필드 범위');
    const view = new DataView(bytes.buffer, bytes.byteOffset + at, width);
    if (width === 1) return view.getUint8(0); if (width === 2) return view.getUint16(0, true); return view.getUint32(0, true);
}
// 주변 바이트는 유지하고 지정 WORD 또는 DWORD만 쓴다.
function put(bytes, at, value, width = 4) {
    if (!(bytes instanceof Uint8Array) || at < 0 || at + width > bytes.length) throw new RangeError('객체 수신의 실제 쓰기 범위');
    const view = new DataView(bytes.buffer, bytes.byteOffset + at, width);
    if (width === 2) view.setUint16(0, value, true); else view.setUint32(0, value, true);
}
// 원문 float 비트를 실수로 읽으며 별도 좌표 정규화를 하지 않는다.
function float(bytes, at) { return new DataView(bytes.buffer, bytes.byteOffset + at, 4).getFloat32(0, true); }
// 표시 좌표의 실제 signed 정수 범위를 확인한 뒤 절삭한 하위 WORD를 사용한다.
function coordinate(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('객체 수신 좌표의 실제 정수 범위');
    return Math.trunc(value) & 0xffff;
}

// 같은 실제 Pool의 생성/갱신 또는 검증 분기를 원본 순서대로 실행한다.
export function receiveRawSquid(pool, packet, state, hooks) {
    if (pool?.edition !== 'patch1078' || !(packet instanceof Uint8Array) || packet.length < 35) throw new TypeError('객체 수신의 PATCH35BYTE 물리 계약');
    const incoming = read(packet, 4, 2);
    const assertion = (expression, line) => need(hooks.assertion, 'assertion')(expression, '.\\Clientboard.cpp', line);
    if (!state.verifySid) {
        const selectedType = packet[16], oldState = pool.slot(incoming)[11]; state.typeWas = selectedType;
        const object = need(hooks.take, 'take')(selectedType, incoming), raw = pool.physical(object);
        put(raw, 14, read(packet, 6)); put(raw, 18, read(packet, 10));
        put(raw, 22, coordinate(float(raw, 14) * 16 + 0.5), 2); put(raw, 24, coordinate(float(raw, 18) * 11 + 0.5), 2);
        need(hooks.word, 'word')(object, read(packet, 14, 2));
        put(raw, 42, read(packet, 19)); put(raw, 46, read(packet, 23)); raw[11] = packet[17];
        need(hooks.owner, 'owner')(object, packet[18]); raw[35] = packet[29]; put(raw, 36, read(packet, 30));
        const extra = packet[34] ^ ((raw[40] ^ packet[34]) & 4); raw[40] = ((raw[40] ^ extra) & 2) ^ extra;
        if (need(hooks.flags1, 'flags1')(selectedType) & 0x10) setRawSquidHitPoints(pool, object, (read(packet, 27, 2) << 16) >> 16);
        const receivedState = raw[11]; raw[11] |= 4; let flags = 8;
        if ((receivedState & 0x10) || (oldState & 1)) {
            flags = (need(hooks.firstPop, 'firstPop')(object) | 8) >>> 0; raw[11] &= 0xef;
            const ownerMask = 1 << (raw[34] & 31);
            if (!need(hooks.hasTechnology, 'hasTechnology')(raw[34], raw[10])) {
                if (raw[10] >= state.noMakeWarnings.length) throw new RangeError('객체 수신의 실제 경고 표 범위');
                if (!(state.noMakeWarnings[raw[10]] & ownerMask)) {
                    need(hooks.noMake, 'noMake')(object);
                    if (raw[10] >= state.noMakeWarnings.length) throw new RangeError('Tell 뒤 현재 경고 TYPE 범위');
                    state.noMakeWarnings[raw[10]] = (state.noMakeWarnings[raw[10]] | ownerMask) >>> 0;
                }
                need(hooks.markNoMake, 'markNoMake')(object);
            }
        }
        if (raw[11] & 8) assertion('!(s->isContained())', 0xc5);
        if (!(raw[11] & 8)) need(hooks.pop, 'pop')(object, float(packet, 6), float(packet, 10), flags);
        else need(hooks.containedPop, 'containedPop')(object, read(packet, 6) & 0xffff, flags);
        state.typeWas = 0; return;
    }
    if (incoming !== state.verifySid) assertion('zus->sid == sidToVerify', 0x8d);
    const raw = pool.physical(incoming);
    // 각 불일치는 현재 verify SID/TYPE를 선택하고 실제 로그 장치에 원본 서식으로 전달한다.
    const mismatch = (field, server, client) => {
        if (server === client) return;
        const current = state.verifySid >>> 0; if (current > 65535 || current >= pool.capacity) throw new RangeError('검증 로그의 현재 SID 범위');
        const name = String(need(hooks.typeName, 'typeName')(current)).split('\0', 1)[0];
        const message = `VERIFY ${current} (client type=${name}): ${field} ${server >>> 0} != ${client >>> 0}\n`;
        if (message.length >= 512) throw new RangeError('객체 검증 로그 범위');
        need(hooks.log, 'log')(0x1d, message);
    };
    mismatch('type', packet[16], raw[10]); mismatch('playerId', packet[18], raw[34]); mismatch('freeFlag', packet[17] & 1, raw[11] & 1);
    mismatch('deadFlag', packet[17] & 2, (raw[11] >> 1) & 1); mismatch('containedFlag', packet[17] & 8, (raw[11] >> 3) & 1);
    state.verifySid = 0;
}
