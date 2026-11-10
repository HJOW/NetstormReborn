// 상태 주문(117/118) 타이머의 11바이트가 아닌 가변 패킷(writer 41e280·누적 읽기 41e230·송신 observer 40b390·부모 선택 41e0a0·신규 수신 41e320)을 옮긴 모듈이다(cpppj `o/RawBombTimerPacket.cpp/.h`, 10.78 전용).
//  - writeBombTimerPacket(fields, form, parent, packet, declaredLength, protocol = 1, message = 0x7d): [protocol&0x7f, message, 선언 길이 WORD(4 미만은 4), form WORD, 부모 WORD, 기한 double 8바이트, 대상 수 WORD, flags DWORD, 대상 WORD 목록]
//    을 쓰고 실제 반환 길이 `22 + 2 × 수`를 돌려준다. 헤더의 선언 길이는 반환 길이로 덮어쓰지 않는다(송신 observer 가 확정). 대상 수는 하위 WORD 만 전송하고 SID 는 이 경계에서만 WORD 로 자른다.
//  - readBombTimerPacket(fields, packet): flags 를 먼저 쓰고, 패킷의 대상 수만큼 WORD 를 읽어 현재 한도(capacity)까지 기존 목록에 덧붙인 뒤(가득 찬 뒤에도 입력 WORD 는 끝까지 읽음) 기한 비트를 복사한다. 중간에 길이가 모자라면 앞선 쓰기는 남긴 채 거부한다.
//  - finalizeBombTimerPacket(packet, writing): 송신일 때만 대상 수(65535 는 원문 진단 bda)에 맞게 선언 길이를 (수 + 11) × 2 로 확정한다.
//  - bombTimerParent(pool, types, members): 순서대로 현재 TYPE 의 genus 0x40000000 인 첫 물리 SID 를 돌려준다(상태 필터 없음, 없으면 0).
//  - receiveBombTimerPacket(pool, state, packet, hooks): boss(진단 cc)·점유 Form 아님(진단 e6 뒤 종료)을 거친 뒤 생성(processType,0,0) → 읽기 → 부착(현재 종류, 패킷 부모, 현재 패킷 form, 0) 순서이며, 생성/읽기 뒤 현재 값을 원본 위치에서 다시 조회한다.
// 상태 { boss, processType } 는 매번 현재 값을 읽는 살아 있는 객체이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E52: RawBombTimerPacket 이식으로 추가)

// 헤더 고정 길이(22)와 대상 WORD 크기, 선언 길이 하한, 대상 수 진단 값, 부모 genus 비트, TYPE 수이다. (2026-11-08 WB04-E52)
const HEADER_SIZE = 22;
const MEMBER_SIZE = 2;
const MIN_DECLARED = 4;
const COUNT_DIAGNOSTIC = 65535;
const PARENT_GENUS = 0x40000000;
const TYPE_COUNT = 188;
const TIMER_MESSAGE = 0x7d;

// 선택한 큰 경계가 없으면 빈 성공 대신 실제 누락을 보고한다. (2026-11-08 WB04-E52)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('상태 타이머 패킷의 실제 하위 미연결');
    return hook;
}

// 현재 패킷에서 비정렬 리틀엔디언 필드를 읽는다(폭 8 은 BigInt, 그 밖은 Number). 버퍼 밖 접근은 거부한다. (2026-11-08 WB04-E52)
function read(packet, offset, width) {
    if (offset > packet.length || width > packet.length - offset) throw new RangeError('상태 타이머 패킷 길이 부족');
    let value = 0n;
    // 하위 바이트부터 원본 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value |= BigInt(packet[offset + index]) << BigInt(8 * index);
    return width === 8 ? value : Number(value);
}

// 확인한 필드 폭만 쓰며 헤더 밖의 호출자 바이트는 보존한다. (2026-11-08 WB04-E52)
function write(packet, offset, value, width) {
    if (offset > packet.length || width > packet.length - offset) throw new RangeError('상태 타이머 출력 길이 부족');
    let rest = BigInt.asUintN(64, BigInt(value));
    // 하위 바이트부터 원본 폭만 기록한다.
    for (let index = 0; index < width; index += 1) {
        packet[offset + index] = Number(rest & 0xffn);
        rest >>= 8n;
    }
}

// 41e280: 현재 선언 길이를 헤더에 쓰고 실제 길이를 돌려준다. (2026-11-08 WB04-E52)
export function writeBombTimerPacket(fields, form, parent, packet, declaredLength, protocol = 1, message = TIMER_MESSAGE) {
    const count = fields.members.length & 0xffff;
    const length = HEADER_SIZE + MEMBER_SIZE * count;
    if (packet.length < length) throw new RangeError('상태 타이머 출력 버퍼 부족');
    write(packet, 0, protocol & 0x7f, 1);
    write(packet, 1, message, 1);
    // 선언 길이는 현재 DWORD 입력이다: 4 미만이면 4, 그 외는 DWORD 값을 그대로 비교한 뒤 하위 WORD 만 쓴다(옛 WORD 매개변수처럼 먼저 16비트로 자르지 않는다). (2026-11-10 WB04-E263: cpppj 기준선 뒤 RawBombTimerPacket 재반영)
    write(packet, 2, Math.max(MIN_DECLARED, declaredLength >>> 0), 2);
    write(packet, 4, form, 2);
    write(packet, 6, parent, 2);
    write(packet, 18, fields.flags, 4);
    write(packet, 16, count, 2);
    write(packet, 8, fields.endBits, 8);
    // SID 는 이 경계에서만 WORD 로 잘라 쓰며 중복과 순서를 유지한다.
    for (let index = 0; index < count; index += 1) write(packet, HEADER_SIZE + MEMBER_SIZE * index, fields.members[index], 2);
    return length;
}

// 41e230: flags → 목록 누적(한도까지) → 기한 순서로 읽는다. (2026-11-08 WB04-E52)
export function readBombTimerPacket(fields, packet) {
    if (fields.capacity > 0x7fffffff || fields.members.length > 0x7fffffff) throw new RangeError('상태 타이머의 손상된 signed 목록 범위');
    fields.flags = read(packet, 18, 4);
    // 가득 찬 뒤에도 원본처럼 입력 WORD 는 끝까지 읽는다(대상 수는 반복마다 현재 패킷에서 다시 읽음).
    for (let index = 0; index < read(packet, 16, 2); index += 1) {
        const member = read(packet, HEADER_SIZE + MEMBER_SIZE * index, 2);
        if (fields.members.length < fields.capacity) fields.members.push(member);
    }
    fields.endBits = read(packet, 8, 8);
}

// 40b390: 송신 observer 로 현재 대상 수에 맞게 선언 길이만 확정한다(수신/쓰기 아님이면 아무것도 하지 않음). (2026-11-08 WB04-E52)
export function finalizeBombTimerPacket(packet, writing) {
    if (!writing) return;
    const count = read(packet, 16, 2);
    if (count === COUNT_DIAGNOSTIC) throw new Error('원본 상태 타이머 대상 수 진단 bda');
    write(packet, 2, (count + 11) * 2, 2);
}

// 41e0a0: 상태 필터 없이 현재 목록에서 TYPE genus 0x40000000 인 첫 물리 SID 를 돌려준다. (2026-11-08 WB04-E52)
export function bombTimerParent(pool, types, members) {
    if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT) throw new TypeError('상태 타이머 부모의 PATCH 풀/188TYPE 필요');
    // 순서대로 현재 TYPE 을 읽으며 free/dead/void 를 추가로 제외하지 않는다.
    for (const member of members) {
        if (member >= pool.capacity || member > 65535) throw new RangeError('상태 타이머 부모의 물리 SID 범위');
        const kind = pool.slot(member)[10];
        if (kind >= types.length) throw new RangeError('상태 타이머 부모의 TYPE 범위');
        if (types[kind].flags2 & PARENT_GENUS) return member;
    }
    return 0;
}

// 41e320: 신규 Form 수신이다. hooks = { assertion(line), create(type, a, b) → token, read(token, packet), attach(token, type, parent, form, flags) }. 상태 { boss, processType } 는 현재 값을 읽는다. (2026-11-08 WB04-E52)
export function receiveBombTimerPacket(pool, state, packet, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('상태 타이머 수신의 PATCH 풀 필요');
    if (state.boss) need(hooks.assertion)(0xcc);
    const initial = read(packet, 4, 2);
    if (!(pool.slot(initial)[11] & 1)) {
        need(hooks.assertion)(0xe6);
        return;
    }
    // factory 의 종류만 먼저 읽고 뒤 부착에서는 현재 종류를 다시 읽는다.
    const token = need(hooks.create)(state.processType, 0, 0);
    need(hooks.read)(token, packet);
    const form = read(packet, 4, 2);
    const parent = read(packet, 6, 2);
    need(hooks.attach)(token, state.processType, parent, form, 0);
}
