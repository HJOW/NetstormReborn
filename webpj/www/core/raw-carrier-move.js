// 운반체(Carrier)의 이동 요청(427340)과 서버 수신(428570), 그리고 AI/사용자가 공유하는 18바이트 이동 명령 작성기(40a850)를 옮긴 모듈이다(cpppj `o/RawCarrierMove.cpp/.h`, 10.78 전용).
// 요청은 송신까지만 하고 서버를 직접 실행하지 않으며, 수신은 소유권/정지 관문을 생략하지 않고 외부 효과 뒤 원본 위치에서 패킷과 현재 TYPE 을 다시 읽는다.
//  - writeCarrierMoveCommand(state, parent, destination, sacrifice, packet): 현재 요청 헤더(`state.request` = { message, protocol, length })로 18바이트를 쓴다. byte0 = protocol & 0x7f, byte1 = message, +2 length(WORD), +4 부모 SID(WORD),
//    +6/+10 목적지 float 비트, +14 희생 DWORD. 길이를 4 로 보정하지 않고 protocol 의 bit7 만 지우며, 저장소가 18바이트 미만이면 쓰기 전에 거부하고 뒤 바이트는 보존한다.
//  - request(parent, destination, sacrifice): 현재 선택이 이 부모면 선택을 해제(select(0,0,1))한다. genus(flags2)에 0x30000(이동/운반 계열)이 없으면 진단(0x571)을 부르고 끝낸다. 비행(0x20000)이 아니면 부모 위치에서 목적지까지
//    지면 연결(connected)이 있어야 하며, 이동 종류(movement)→경로(path) 조회 뒤 기존 경로의 목적지 float 쌍(+0x186dc/+0x186e0)이 요청과 같으면 억제한다. 아니면 18바이트 명령을 만들어 송신(send(9, packet))한다.
//  - receive(owner, packet): 패킷이 18바이트 미만이면 거부한다. 부모(WORD +4, 효과마다 다시 읽음)가 삭제/죽음(state&3)이거나 이동 계열이 아니거나 정지(0x5105)·차단(0x493) 사건이 있으면 끝낸다. 소유자가 다르면 fort 모드에서만 허용한다.
//    포획 후보가 없을 때 귀환 장소가 있으면 접근(target)만 하고, 없으면 간헐천(genus 0x10000000)의 경로에 대상을 쓰거나 제단(genus 0x400000)이 있고 부모가 제단 계열이며 허용되고 패킷의 희생 DWORD 가 있으면 희생을 시작한다.
//    그 밖은 경로 시작(start, 마지막 인자 1)을 부르고 경로가 만들어지면 이동 회피 깊이 0·대상 (0, 0)을 쓴다.
// state = CarrierMoveState, types 는 188 TYPE({ flags2 }), fort 는 fort 모드 getter 또는 `{ value }`(편집 전역과 구별), hooks = { selected, select, assertion, connected, movement, path[RawPathRuntime|null], send, regular, candidate[{ sid, nugget }],
// returnPlace, target, genusAt, geyser[RawPathRuntime|null], altarAllowed, sacrifice, start[RawPathRuntime|null] } 이며 도달한 훅이 없으면 오류이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E99: 운반체 이동 이식으로 추가)

// 슬롯의 TYPE(+10)·상태(+11)·소유자(+34)·좌표(+14/+18) 위치, 패킷의 부모/목적지/희생 위치와 길이이다. (2026-11-08 WB04-E99)
const TYPE_AT = 10;
const STATE_AT = 11;
const OWNER_AT = 34;
const X_AT = 14;
const Y_AT = 18;
const PACKET_PARENT = 4;
const PACKET_X = 6;
const PACKET_Y = 10;
const PACKET_SACRIFICE = 14;
const PACKET_SIZE = 18;
const TYPE_COUNT = 188;
// 이동 계열/비행/제단 genus 비트와 정지·차단 사건 번호, 간헐천·제단 genus 조회 번호, 기존 경로의 목적지 float 위치이다. (2026-11-08 WB04-E99)
const MOVER = 0x30000;
const FLYER = 0x20000;
const ALTAR = 0x200000;
const STOP_EVENT = 0x5105;
const BLOCK_EVENT = 0x493;
const GEYSER_GENUS = 0x10000000;
const ALTAR_GENUS = 0x400000;
const PATH_DESTINATION_X = 0x186dc;
const PATH_DESTINATION_Y = 0x186e0;

// 도달한 실제 하위 누락을 성공으로 바꾸지 않는다. (2026-11-08 WB04-E99)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('운반체 이동의 실제 하위 미연결');
    return hook;
}

// 현재 요청 헤더는 TYPE/에디터 상태와 별개인 원본 52edec 의 BYTE/BYTE/WORD 이다(송신 효과가 바꿀 수 있어 매번 읽는다). (2026-11-08 WB04-E99)
export class CarrierMoveState {
    // 40a850 이 매번 읽는 원본 초기 헤더이다.
    request = { message: 0x40, protocol: 1, length: 18 };
}

// 단정도 float 를 비트(uint32)로 바꾼다. (2026-11-08 WB04-E99)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 0040a850: 현재 헤더와 18바이트 원문을 작성한다. 선택/동일 목적지 관문은 호출자가 수행한다. (2026-11-08 WB04-E99)
export function writeCarrierMoveCommand(state, parent, destination, sacrifice, packet) {
    if (packet.length < PACKET_SIZE) throw new RangeError('이동 명령 18바이트 저장소 부족');
    const view = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
    packet[0] = state.request.protocol & 0x7f;
    packet[1] = state.request.message & 0xff;
    view.setUint16(2, state.request.length & 0xffff, true);
    view.setUint16(PACKET_PARENT, parent & 0xffff, true);
    view.setUint32(PACKET_X, floatBits(destination.x), true);
    view.setUint32(PACKET_Y, floatBits(destination.y), true);
    view.setUint32(PACKET_SACRIFICE, sacrifice >>> 0, true);
    return PACKET_SIZE;
}

// 운반체 이동 요청/수신 처리기이다. (2026-11-08 WB04-E99)
export class RawCarrierMove {
    // 실제 운반체의 물리 50바이트 저장소이다. (2026-11-08 WB04-E99)
    #pool;
    // 현재 188 TYPE 표이다. (2026-11-08 WB04-E99)
    #types;
    // 594fb8 의 현재 fort 모드(getter 또는 { value })이다. (2026-11-08 WB04-E99)
    #fort;
    // 현재 요청 헤더 상태이다. (2026-11-08 WB04-E99)
    #state;
    // 도달한 실제 하위의 외부 연결이다. (2026-11-08 WB04-E99)
    #hooks;

    // PATCH 10.78 의 현재 참조와 188 TYPE 를 빌린다. (2026-11-08 WB04-E99)
    constructor(pool, types, fort, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT) throw new Error('운반체 이동 판본/TYPE 오류');
        this.#pool = pool;
        this.#types = types;
        this.#fort = fort;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 다른 Session 의 경로/풀 혼합을 확인할 때 쓰는 같은 풀이다. (2026-11-08 WB04-E99)
    get pool() { return this.#pool; }

    // 현재 fort 모드이다. (2026-11-08 WB04-E99)
    #isFort() { return Boolean(typeof this.#fort === 'function' ? this.#fort() : this.#fort.value); }

    // 가상 효과 전후 TYPE 를 캐시하지 않고 현재 raw TYPE 의 flags2 를 다시 읽는다. (2026-11-08 WB04-E99)
    #genus(parent) {
        const kind = this.#pool.slot(parent)[TYPE_AT];
        if (kind >= this.#types.length) throw new RangeError('운반체 이동 현재 TYPE 범위');
        return this.#types[kind].flags2 >>> 0;
    }

    // 선택 해제는 carrier/연결 관문보다 먼저 실행된다. 선택이 이 부모가 아니면 아무 것도 하지 않는다. (2026-11-08 WB04-E99)
    request(parent, destination, sacrifice) {
        const hooks = this.#hooks;
        if (need(hooks.selected)() === parent) need(hooks.select)(0, 0, 1);
        if (!(this.#genus(parent) & MOVER)) need(hooks.assertion)(0x571);
        if (!(this.#genus(parent) & MOVER)) return;
        if (!(this.#genus(parent) & FLYER)) {
            const raw = this.#pool.slot(parent);
            const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            const source = { x: view.getFloat32(X_AT, true), y: view.getFloat32(Y_AT, true) };
            if (!need(hooks.connected)(source, destination)) return;
        }
        const movement = need(hooks.movement)(this.#pool.slot(parent)[TYPE_AT]);
        const path = need(hooks.path)(parent, movement);
        if (path) {
            const bytes = path.bytes;
            const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
            if (view.getFloat32(PATH_DESTINATION_X, true) === destination.x && view.getFloat32(PATH_DESTINATION_Y, true) === destination.y) return;
        }
        const packet = new Uint8Array(PACKET_SIZE);
        writeCarrierMoveCommand(this.#state, parent, destination, sacrifice, packet);
        need(hooks.send)(9, packet);
    }

    // 소유자는 전체 DWORD 로 비교하며 fort 모드일 때만 다른 소유자의 이동을 허용한다. packet 은 효과 뒤 원본 위치에서 다시 읽는다. (2026-11-08 WB04-E99)
    receive(owner, packet) {
        const hooks = this.#hooks;
        if (packet.length < PACKET_SIZE) throw new RangeError('운반체 이동 packet 물리 길이 부족');
        const view = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
        // 외부 효과 뒤 원본이 다시 읽는 부모 WORD 와 목적지 float 이다.
        const number = () => view.getUint16(PACKET_PARENT, true);
        const point = () => ({ x: view.getFloat32(PACKET_X, true), y: view.getFloat32(PACKET_Y, true) });
        const parent = number();
        if (this.#pool.slot(parent)[STATE_AT] & 3) return;
        if (!(this.#genus(parent) & MOVER)) return;
        if (need(hooks.regular)(parent, STOP_EVENT)) return;
        if (this.#pool.slot(parent)[OWNER_AT] !== (owner >>> 0) && !this.#isFort()) return;
        if (need(hooks.regular)(number(), BLOCK_EVENT)) return;
        if (!need(hooks.candidate)(parent, point()).sid) {
            const place = need(hooks.returnPlace)(point());
            if (place) { need(hooks.target)(number(), place); return; }
            const geyser = need(hooks.genusAt)(point(), GEYSER_GENUS);
            if (geyser) {
                const path = need(hooks.geyser)(parent, geyser);
                if (path) { path.setTargets(geyser, 0); return; }
            } else {
                const altar = need(hooks.genusAt)(point(), ALTAR_GENUS);
                if (altar && (this.#genus(parent) & ALTAR) && need(hooks.altarAllowed)(altar, number()) && view.getUint32(PACKET_SACRIFICE, true)) {
                    need(hooks.sacrifice)(number(), altar);
                    return;
                }
            }
        }
        // 원본 마지막 인자 1 이며 앞선 직접 UI 경로의 replaceStart 1 과 다르다.
        const path = need(hooks.start)({ parent: number(), destination: point(), replaceStart: 0, helper: 0, tailWord: 0, tailByte: 0, reserved: 1 });
        if (path) {
            path.setMoveAwayDepth(0);
            path.setTargets(0, 0);
        }
    }
}
