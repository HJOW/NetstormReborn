// 실제 로컬 건설 요청 0x3e 의 25바이트 직렬화와 서버 수신 444af0 을 옮긴 모듈이다(cpppj `o/RawConstructionCommand.cpp/.h`, 10.78: PATCH 0052ede6/7/8 메시지·프로토콜·선언 길이, 40a7c0/4433b0 작성, 444af0 수신).
// 사용: writeConstructionCommand(message, packet); const command = new RawConstructionCommand(typeRecords, state, hooks); command.receive(sender, packet); registerConstructionCommand(dispatch, command, () => prepareSession());
//   message = 로컬 배치(`RawConstructionLocal`)의 서버 요청 본문 {type, x, y, argument(WORD), direction, quality, timeLow, timeHigh}. 직렬화 위치는 [0] 프로토콜 하위 7비트, [1] 메시지 0x3e, [2..3] 선언 길이, [4] 타입, [5..8] x(float), [9..12] y(float), [13..14] 인자, [15] 방향, [16] 품질, [17..20] 시각 하위, [21..24] 시각 상위이다.
//   state = 현재 서버 수신이 실제 읽는 전역(속성으로 매번 현재값을 읽는다): blocked(558e74, 비영이면 타입/패킷 본문을 읽기 전에 반환)·chargeBlocked(594fa4, 비용 선검사 생략)·localPlayer(540c70, sender 전체 DWORD 와 비교)·chargeBlockedRead(선택, 실제 594fa4 를 비용 관문에서 조회하는 함수; 있으면 chargeBlocked 대신 쓴다).
//   hooks = 실제 하위: money(sender → float 잔액, 40ef30)·shortfall(요청, 442b50)·request(요청 → bool, 444760 의 전체 MayPlace/충돌/확정)·reject(요청, Request 실패 뒤 442af0 의 환불/취소). 모두 필수이며 고정 성공값을 공급하지 않는다.
// 규칙: 수신은 차단 → 본문 길이(17) → 현재 타입 번호 범위 → 비용 관문(차단이 없고 sender 가 로컬이 아닐 때만 Money 를 부르고 호출 뒤 현재 타입 비용과 정렬 비교, NaN 은 부족이 아님) → Request → 실패면 Reject 이며, 하위 호출마다 패킷의 현재 필드를 다시 읽는다(시각 8바이트는 읽지 않고 0/0).
// 기준선 뒤 cpppj 변경(5f4b9d304, 3.6절 ②): 현재 건설 전역과 공유 차단/로컬 번호의 효과 뒤 재조회 순서(`chargeBlockedRead`·속성형 `blocked`/`localPlayer`)를 반영했다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E322: 4-C RawConstructionCommand 이식)

// 요청 전체 길이(25)와 수신이 실제 읽는 마지막 품질 BYTE 까지의 폭(17), 필수 타입 표 크기(PATCH 188)이다. (2026-11-11 WB04-E322)
export const CONSTRUCTION_COMMAND_BYTES = 25;
const READ_BYTES = 17;
const TYPE_COUNT = 188;
// 메시지 종류 0x3e 와 기본 프로토콜 1 이다(생성자는 프로토콜의 상위 비트만 제거한다). (2026-11-11 WB04-E322)
const COMMAND_MESSAGE = 0x3e;
const COMMAND_PROTOCOL = 1;

// 단정도 float 를 little endian 비트(uint32)로 바꾼다. (2026-11-11 WB04-E322)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 비정렬 원문에 지정 폭(little endian)만 쓴다. 인접 필드와 호출자 뒤 버퍼는 건드리지 않는다. (2026-11-11 WB04-E322)
function put(bytes, offset, value, width) {
    // 원본이 쓰는 BYTE/WORD/DWORD 만 낮은 자리부터 기록한다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 40a7c0/4433b0 의 비정렬 원본 필드와 0 시각을 25바이트에 직렬화하고 25 를 돌려준다. 저장소가 25바이트보다 작으면 거부한다. header = {message 0x3e, protocol 1, length 25}(생략 가능). (2026-11-11 WB04-E322)
export function writeConstructionCommand(message, packet, header = {}) {
    if (!(packet instanceof Uint8Array) || packet.length < CONSTRUCTION_COMMAND_BYTES) throw new RangeError('건설 요청25바이트 저장소 부족');
    const protocol = header.protocol ?? COMMAND_PROTOCOL;
    packet[0] = protocol & 0x7f;
    packet[1] = (header.message ?? COMMAND_MESSAGE) & 0xff;
    put(packet, 2, header.length ?? CONSTRUCTION_COMMAND_BYTES, 2);
    packet[4] = message.type & 0xff;
    put(packet, 5, floatBits(message.x), 4);
    put(packet, 9, floatBits(message.y), 4);
    put(packet, 13, message.argument, 2);
    packet[15] = message.direction & 0xff;
    packet[16] = message.quality & 0xff;
    put(packet, 17, message.timeLow ?? 0, 4);
    put(packet, 21, message.timeHigh ?? 0, 4);
    return CONSTRUCTION_COMMAND_BYTES;
}

// 서버 수신 444af0: 원본 고정 길이 요청의 재조회/NaN 비교/환불 순서를 보존한다. (2026-11-11 WB04-E322)
export class RawConstructionCommand {
    // 타입 표(`cost` 를 가진 레코드 188개)·현재 상태·실제 하위를 빌린다. 모두 이 객체보다 오래 살아야 하며 PATCH 188개 타입과 필수 하위 넷이 아니면 거부한다. (2026-11-11 WB04-E322)
    constructor(types, state, hooks) {
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || typeof state !== 'object' || state === null
            || typeof hooks?.money !== 'function' || typeof hooks.shortfall !== 'function' || typeof hooks.request !== 'function' || typeof hooks.reject !== 'function') {
            throw new TypeError('실제 건설 요청 수신 연결 오류');
        }
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 현재 TYPE 비용을 Money 호출 뒤 다시 읽기 위해 빌린 타입 표, callback 뒤에도 같은 현재 전역인 상태, 계산을 대신하지 않는 실제 하위이다. (2026-11-11 WB04-E322)
    #types;
    #state;
    #hooks;

    // 원본 sender DWORD/WORD 인자/BYTE 방향·품질을 유지하며 패킷의 시각 자리는 쓰지 않고 0/0 을 전달한다. 호출 경계가 원본 버퍼를 바꿨으면 현재 필드를 다시 읽는다. (2026-11-11 WB04-E322)
    static #read(sender, packet) {
        const view = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
        return {
            type: packet[4], x: view.getFloat32(5, true), y: view.getFloat32(9, true), player: sender >>> 0, argument: packet[13] | (packet[14] << 8), direction: packet[15],
            flags: 0, timeLow: 0, timeHigh: 0, quality: packet[16],
        };
    }

    // 444af0 의 비용 선검사 → Request → 실패 환불 순서이다. NaN 잔액/비용의 선검사는 ordered less 이며 후속 Place 의 부족 판단과 구별한다. type/flags 는 수신 범위를 임의 제한하지 않는다. (2026-11-11 WB04-E322)
    receive(sender, packet) {
        const state = this.#state;
        const hooks = this.#hooks;
        const from = sender >>> 0;
        if ((state.blocked >>> 0) !== 0) return;
        if (!(packet instanceof Uint8Array) || packet.length < READ_BYTES) throw new RangeError('건설 요청 수신 본문 부족');
        const type = packet[4];
        if (type >= this.#types.length) throw new RangeError('건설 요청 현재 TYPE 범위 오류');
        // TYPE 검증 뒤 현재 전역을 읽고 그 뒤 로컬 sender 를 비교한다.
        const blocked = typeof state.chargeBlockedRead === 'function' ? state.chargeBlockedRead() : state.chargeBlocked;
        if (!blocked && from !== (state.localPlayer >>> 0)) {
            // 실제 잔액 경계 뒤 현재 TYPE 비용을 읽는다.
            const money = Math.fround(hooks.money(from));
            if (money < this.#types[type].cost) {
                hooks.shortfall(RawConstructionCommand.#read(from, packet));
                return;
            }
        }
        if (!hooks.request(RawConstructionCommand.#read(from, packet))) hooks.reject(RawConstructionCommand.#read(from, packet));
    }
}

// 실제 빈 observer 는 종류만 등록하며 단일/집합 분배기의 원본 차이를 보존한다: 0x3e 의 버전 1/정책 0/길이 25 와 수신 처리를 등록한다. prepare 는 처리기 호출 직전 Session 의 같은 현재 전역을 공급한다(선택). (2026-11-11 WB04-E322)
export function registerConstructionCommand(dispatch, command, prepare = null) {
    dispatch.observe(COMMAND_MESSAGE, () => {});
    dispatch.version(COMMAND_MESSAGE, COMMAND_PROTOCOL, 0);
    dispatch.requireLength(COMMAND_MESSAGE, CONSTRUCTION_COMMAND_BYTES);
    dispatch.register(COMMAND_MESSAGE, (sender, connection, packet) => {
        if (typeof prepare === 'function') prepare();
        command.receive(sender, packet);
    });
}
