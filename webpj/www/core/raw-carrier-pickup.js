// 일반 운반체의 4f 포획 요청(428e00)·수신(426520)·기존 내용물 제거(4264d0)와 실제 서버 객체 → 내용물 변환을 옮긴 모듈이다(cpppj `o/RawCarrierPickup.cpp/.h`, 10.78 전용).
//  - request(부모): 현재 지점의 후보(426900)를 조회하고 후보가 없거나 권한자가 아니면 끝. 단독 미션이 아니면 후보의 genus 를 확인한 뒤 권한을 다시 읽어, 9바이트 패킷(프로토콜 하위 7비트, 메시지, 길이 WORD, 부모 WORD, 후보 WORD, 후보 TYPE)을 대상 9 로 전송한다(요청에서 로컬 수신하지 않음).
//  - receive(소유자, flags, 패킷): 길이 9 미만은 거부. 최초 부모/대상 WORD 를 보관하고 권한자가 아니면 원문 진단(Carrier.cpp 20c) 뒤 권한을 다시 읽어 여전히 아니면 끝. 대상/부모가 free 또는 dead 이거나 TYPE 이 패킷과 다르면 끝. genus 0x2000 이면 오벨리스크 보상 객체로 변환한 뒤 입력 큐에 넣고(현재 명령, 현재 대상, 패킷 부모 WORD),
//    genus 0x2100 이면 기존 내용물을 제거(discard)한다. 대상을 임시 dead 로 만들고 부모에 같은 종류의 ContainedForm 을 생성·부착(플래그는 genus 0x100 이면 1, 아니면 0x20)한 뒤 dead 만 되돌리고, 대상의 종속을 다음 항목을 먼저 읽으며 새 내용물로 옮기고 대상을 삭제한다.
//  - discard(부모): 현재 첫 mask 1 내용물을 찾아 종류(WORD)를 보관하고 삭제한 뒤 삭제 효과 뒤의 현재 특정 타입(silentType)과 비교해 같지 않으면 전역 보충을 부른다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E73: Carrier 포획 이식으로 추가)

import { RawContainedFinder } from './raw-contained-finder.js';

// 슬롯의 TYPE/상태/좌표/인수 WORD/종류 WORD 위치, 상태의 dead 비트, 패킷 최소 길이와 위치, 진단 줄, 내용물 생성 플래그, genus 비트와 TYPE 표 크기이다. (2026-11-08 WB04-E73)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const ARGUMENT_OFFSET = 12;
const KIND_OFFSET = 18;
const DEAD = 2;
const FREE_OR_DEAD = 3;
const PACKET_LENGTH = 9;
const AUTHORITY_ASSERTION_LINE = 0x20c;
const REVEAL_GENUS = 0x2000;
const DISCARD_GENUS = 0x2100;
const CONTENT_VISIBLE_GENUS = 0x100;
const TYPE_COUNT = 188;
const SEND_TARGET = 9;

// 실제 요청 헤더와 입력 명령/특정 타입 전역이다: request({ message, protocol, length })=40a9a0 의 BYTE/BYTE/WORD(메시지 0x4f, 프로토콜 1, 길이 9), command=5429b8 의 입력 명령, silentType=5412f8 의 기존 내용물 삭제 후 보충 생략 타입이다. (2026-11-08 WB04-E73)
export class CarrierPickupState {
    request = { message: 0x4f, protocol: 1, length: 9 };
    command = 0x030000b1;
    silentType = 168;
}

// 선택된 실제 효과 누락을 빈 동작으로 바꾸지 않는다. (2026-11-08 WB04-E73)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('일반 운반체 포획의 실제 하위 미연결');
    return hook;
}

// 비정렬 WORD/DWORD 를 지정 폭만큼 읽는다. (2026-11-08 WB04-E73)
function read(raw, offset, width = 4) {
    let value = 0;
    // 원본의 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 지정 WORD 의 상위값은 버리고 실제 두 바이트만 쓴다. (2026-11-08 WB04-E73)
function putWord(raw, offset, value) {
    raw[offset] = value & 0xff;
    raw[offset + 1] = (value >>> 8) & 0xff;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E73)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, types 는 188칸 TYPE 표(flags2), contained 는 ContainedFinderState, boss/single 은 현재 권한/단독 미션 모드 getter, state 는 CarrierPickupState,
//   hooks = { candidate(부모, x, y)→[SID, bool], send(대상, 패킷), assertion(줄), reveal(대상)→SID, queue(명령, 대상, 패킷 부모 WORD), find(부모, mask, 종류, 순번)→SID, afterDiscard(), create(부모, 종류, 인수, flags, 부착 flags, 품질)→SID,
//   destroy(sid, flags), detach(sid, flags), attach(sid, 새 내용물, flags) } 이며 도달한 것만 필요하다. (2026-11-08 WB04-E73)
export class RawCarrierPickup {
    // 실제 지도 객체/내용물의 단일 물리 저장소, 효과 뒤 다시 조회하는 TYPE 표, 서버 종속 이동의 현재 타입/권한/진단, 현재 권한/미션 모드, 현재 명령과 요청 헤더/특정 타입, 큰 하위 효과이다.
    #pool;
    #types;
    #contained;
    #boss;
    #single;
    #state;
    #hooks;

    // PATCH 10.78 의 판본/TYPE 표 크기를 구성 때 확인한다. 10.72 의 다른 포획 수신을 PATCH 와 같다고 추정하지 않는다. (2026-11-08 WB04-E73)
    constructor(pool, types, contained, boss, single, state, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || typeof boss !== 'function' || typeof single !== 'function') throw new TypeError('일반운반체포획의판본/TYPE 오류');
        this.#pool = pool;
        this.#types = types;
        this.#contained = contained;
        this.#boss = boss;
        this.#single = single;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 다른 월드의 생성·수명 처리기를 섞지 않도록 공개하는 풀이다. (2026-11-08 WB04-E73)
    get pool() {
        return this.#pool;
    }

    // 현재 TYPE 의 실제 flags2 를 읽고 반환 genus 를 보관하지 않는다. (2026-11-08 WB04-E73)
    #genus(sid) {
        const type = this.#pool.slot(sid)[TYPE_OFFSET];
        if (type >= this.#types.length) throw new RangeError('포획 현재 TYPE 범위 오류');
        return this.#types[type].flags2 >>> 0;
    }

    // 생성 효과가 free/void 등을 바꾸어도 원본처럼 현재 물리 BYTE 의 dead 비트만 바꾼다. (2026-11-08 WB04-E73)
    #dead(sid, value) {
        const raw = this.#pool.physical(sid);
        raw[STATE_OFFSET] = value ? raw[STATE_OFFSET] | DEAD : raw[STATE_OFFSET] & ~DEAD & 0xff;
    }

    // 428e00: 후보는 권한 검사 전에 조회한다. 내부 이름/후보 bool 로 다른 프로토콜을 고르지 않는다. (2026-11-08 WB04-E73)
    request(parent) {
        const hooks = this.#hooks;
        const raw = this.#pool.slot(parent);
        const found = need(hooks.candidate)(parent, floatOf(read(raw, X_OFFSET)), floatOf(read(raw, Y_OFFSET)))[0];
        if (!found || !this.#boss()) return;
        // 원본은 후보 슬롯을 그대로 보며 genus 확인 뒤 권한을 다시 읽는다.
        const target = this.#pool.physical(found);
        if (!this.#single()) this.#genus(found);
        if (!this.#boss()) return;
        const header = this.#state.request;
        const packet = new Uint8Array(PACKET_LENGTH);
        packet[0] = header.protocol & 0x7f;
        packet[1] = header.message & 0xff;
        packet[2] = header.length & 0xff;
        packet[3] = (header.length >>> 8) & 0xff;
        putWord(packet, 4, parent);
        putWord(packet, 6, found);
        packet[8] = target[TYPE_OFFSET];
        need(hooks.send)(SEND_TARGET, packet);
    }

    // 4264d0: 삭제한 Form 의 종류는 보관하지만 비교 전역은 삭제 효과 뒤 현재값을 읽는다. (2026-11-08 WB04-E73)
    discard(parent) {
        const hooks = this.#hooks;
        const old = need(hooks.find)(parent, 1, 0, 0);
        if (!old) return;
        const kind = read(this.#pool.slot(old), KIND_OFFSET, 2);
        need(hooks.destroy)(old, 0);
        if (kind !== (this.#state.silentType >>> 0)) need(hooks.afterDiscard)();
    }

    // 426520: 최초 부모는 보관하며 입력 명령의 부모 WORD 만 reveal 뒤 같은 패킷에서 다시 읽는다. 선행 소유자/flags 는 읽지 않는다. (2026-11-08 WB04-E73)
    receive(owner, flags, packet) {
        void owner;
        void flags;
        if (packet.length < PACKET_LENGTH) throw new RangeError('포획요청packet길이오류');
        const hooks = this.#hooks;
        const parent = read(packet, 4, 2);
        let target = read(packet, 6, 2);
        if (!this.#boss()) {
            need(hooks.assertion)(AUTHORITY_ASSERTION_LINE);
            if (!this.#boss()) return;
        }
        if ((this.#pool.slot(target)[STATE_OFFSET] & FREE_OR_DEAD) || (this.#pool.slot(parent)[STATE_OFFSET] & FREE_OR_DEAD) || this.#pool.slot(target)[TYPE_OFFSET] !== packet[8]) return;
        if (this.#genus(target) & REVEAL_GENUS) target = need(hooks.reveal)(target);
        need(hooks.queue)(this.#state.command >>> 0, target, read(packet, 4, 2));
        if (this.#genus(target) & DISCARD_GENUS) this.discard(parent);
        const raw = this.#pool.slot(target);
        const argument = read(raw, ARGUMENT_OFFSET, 2);
        const kind = raw[TYPE_OFFSET];
        this.#dead(target, true);
        const created = need(hooks.create)(parent, kind, argument, (this.#genus(target) & CONTENT_VISIBLE_GENUS) ? 1 : 0x20, 0, 0);
        this.#dead(target, false);
        const finder = new RawContainedFinder(this.#pool, this.#contained);
        let current = finder.begin(target);
        // 현재 종속의 다음을 먼저 조회해야 현재 종속의 Unpop/부모 통지가 다음 링크를 바꾸어도 순서가 유지된다.
        while (current) {
            const saved = current;
            current = finder.next();
            need(hooks.detach)(saved, 0);
            need(hooks.attach)(saved, created, 0);
        }
        need(hooks.destroy)(target, 0);
    }
}
