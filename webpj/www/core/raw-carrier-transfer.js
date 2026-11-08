// 미등록 187 캐리어의 주기적 Halt(42ac60)와 너겟 전달 명령(42a0d0, 수신 426830)의 실제 호출 순서를 옮긴 모듈이다(cpppj `o/RawCarrierTransfer.cpp/.h`, 10.78 전용).
//  - halt(부모): 모든 미등록 187 캐리어가 공유하는 카운터를 DWORD 로 1 올린 뒤 signed 로 4 로 나눈 나머지(음수 나머지를 양수로 정규화하지 않음)로 바꾸고, 0 이면 전달(transfer), 아니면 공통 Halt(42a5f0)를 부른다.
//  - transfer(부모): 권한자이고 부모에 첫 mask 0x20 내용물이 없고 편집 모드가 아니어야 한다(아니면 10). 현재 좌표의 한 칸 일반 finder 를 만든 뒤 공유 cooldown(signed 양수일 때)을 하나 줄이고, cooldown 이 0 인 동안 후보를 돌며 같은 소유자의
//    미등록 187 후보가 현재 너겟 타입 내용물을 가지면 마지막 기증자를 기록하고 cooldown 5, 10바이트 전달 명령(헤더·부모·내용물·기증자)을 로컬에 먼저 수신한 뒤 송출하고 기증자의 이동을 제거, 간헐천 후보로 이동 명령, 내용물 kind 의 genus 0x8000 이면 귀환 요청 후 12 를 반환한다. 없으면 10.
//  - receive(소유자, flags, 패킷): 길이 8 미만은 거부, 내용물/부모가 free 이면 끝, 아니면 내용물을 떼어(flags 0) 부모에 붙이고 미리보기 예약(426080)을 부른다. 선행 두 인자와 packet+8 은 읽지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E74: Carrier 너겟 전달 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';

// 슬롯의 TYPE/상태/좌표/소유자/내용물 kind 위치, 패킷 길이와 위치, 내용물 첫 조회 마스크, cooldown 재설정값과 주기, 반환 코드, genus 귀환 비트, 타입 표 크기이다. (2026-11-08 WB04-E74)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const KIND_OFFSET = 18;
const PACKET_LENGTH = 10;
const FREE = 1;
const CONTENT_MASK = 0x20;
const COOLDOWN = 5;
const HALT_PERIOD = 4;
const MOVING = 10;
const TRANSFERRED = 12;
const HOME_GENUS = 0x8000;
const TYPE_COUNT = 188;

// 모든 미등록 187 캐리어가 공유하는 원본 전역이다: haltCounter(54542c)·cooldown(545424)·lastDonor(545420), 효과 뒤 다시 읽는 현재 타입 전역(emptyType 187, nuggetType 154), 40acf0 의 전달 명령 헤더(message 0x60, protocol 1, length 10; 길이를 4 이상으로 보정하지 않음),
//   실제 005412c0 의 장기 너겟 번호를 빌리는 getter(sharedNuggetType)이다. (2026-11-08 WB04-E74)
export class CarrierTransferState {
    haltCounter = 0;
    cooldown = 0;
    lastDonor = 0;
    emptyType = 187;
    nuggetType = 154;
    packet = { message: 0x60, protocol: 1, length: 10 };
    sharedNuggetType = null;

    // 앞선 내용물 조회 뒤 현재 번호를 읽으며 독립 입력은 기존 154 를 쓴다. (2026-11-08 WB04-E74)
    currentNuggetType() {
        return (this.sharedNuggetType ? this.sharedNuggetType() : this.nuggetType) >>> 0;
    }
}

// 도달한 실제 효과가 없으면 성공/빈 동작으로 대체하지 않는다. (2026-11-08 WB04-E74)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('캐리어 너겟 전달의 실제 하위 미연결');
    return hook;
}

// 비정렬 원본 WORD/DWORD 를 낮은 바이트부터 읽는다. (2026-11-08 WB04-E74)
function read(raw, offset, width = 4) {
    let value = 0;
    // 옆 필드를 합치지 않고 지정 폭만 읽는다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 패킷 필드의 하위 BYTE/WORD 만 기록한다. (2026-11-08 WB04-E74)
function put(raw, offset, value, width = 2) {
    // 원본 지정 폭의 낮은 바이트부터 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E74)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// 41d770 의 signed64 절삭 뒤 하위 signed32 를 사용하는 정상/무효 변환이다. (2026-11-08 WB04-E74)
function cell(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// pool 은 10.78 SidPool, hash 는 같은 일반 점 finder 공간 해시, types 는 188칸 TYPE 표(flags2 와 발자국), boss/editor/localOwner 는 현재 값을 읽는 getter, state 는 CarrierTransferState,
//   hooks = { find(부모, mask, 종류, 순번)→SID, detach(sid, flags), attach(sid, 부모, flags), preview(부모), broadcast(패킷), movementType(타입)→수, removeMovement(부모, 이동 종류), chooseGeyser(부모)→SID, goGeyser(부모, 대상), goHome(부모), commonHalt(부모)→수 } 이며 도달한 것만 필요하다. (2026-11-08 WB04-E74)
export class RawCarrierTransfer {
    // 부모/기증자/내용물의 같은 물리 저장소, 실제 일반 finder 의 네 단계 버킷, 현재 내용물 WORD 로 다시 조회하는 타입 표, 현재 권한/편집/로컬 소유자 getter, 공유 상태, 큰 하위 효과이다.
    #pool;
    #hash;
    #types;
    #boss;
    #editor;
    #localOwner;
    #state;
    #hooks;

    // PATCH 전용 전달을 CD 의 다른 본문으로 간주하지 않는다. (2026-11-08 WB04-E74)
    constructor(pool, hash, types, boss, editor, localOwner, state, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || [boss, editor, localOwner].some((getter) => typeof getter !== 'function')) throw new TypeError('캐리어 너겟 전달 판본/TYPE 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#boss = boss;
        this.#editor = editor;
        this.#localOwner = localOwner;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 다른 세션의 Form/공간 혼합을 검사하는 풀이다. (2026-11-08 WB04-E74)
    get pool() {
        return this.#pool;
    }

    // 426830: 양쪽 free 비트만 검사한다. dead/owner/기증자 필터를 임의로 추가하지 않는다. 선행 소유자/flags 와 packet+8 은 읽지 않는다. (2026-11-08 WB04-E74)
    receive(owner, flags, packet) {
        void owner;
        void flags;
        if (packet.length < 8) throw new RangeError('캐리어 전달 명령 길이 오류');
        const parent = read(packet, 4, 2);
        const content = read(packet, 6, 2);
        if ((this.#pool.slot(content)[STATE_OFFSET] & FREE) || (this.#pool.slot(parent)[STATE_OFFSET] & FREE)) return;
        const hooks = this.#hooks;
        need(hooks.detach)(content, 0);
        need(hooks.attach)(content, parent, 0);
        need(hooks.preview)(parent);
    }

    // 42a0d0: 첫 내용물 조회와 finder 생성 뒤에야 공유 cooldown 을 줄인다. 전달 성공 12, 나머지 10 이다. (2026-11-08 WB04-E74)
    transfer(parent) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (!this.#boss() || need(hooks.find)(parent, CONTENT_MASK, 0, 0) || this.#editor()) return MOVING;
        const raw = this.#pool.slot(parent);
        const x = cell(floatOf(read(raw, X_OFFSET)));
        const y = cell(floatOf(read(raw, Y_OFFSET)));
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        let candidate = finder.begin({ left: x, top: y, right: x, bottom: y }, 0);
        if ((state.cooldown | 0) > 0) state.cooldown = (state.cooldown - 1) >>> 0;
        // 실제 finder 의 현재 후보와 아직 읽지 않은 체인은 효과 뒤의 현재 raw 를 사용한다.
        while (candidate && state.cooldown === 0) {
            const donor = this.#pool.slot(candidate);
            if (donor[OWNER_OFFSET] === this.#pool.slot(parent)[OWNER_OFFSET] && donor[TYPE_OFFSET] === state.emptyType) {
                const content = need(hooks.find)(candidate, CONTENT_MASK, 0, 0);
                if (content && read(this.#pool.slot(content), KIND_OFFSET, 2) === state.currentNuggetType()) {
                    state.lastDonor = candidate;
                    state.cooldown = COOLDOWN;
                    const packet = new Uint8Array(PACKET_LENGTH);
                    packet[1] = state.packet.message & 0xff;
                    put(packet, 2, state.packet.length);
                    packet[0] = state.packet.protocol & 0x7f;
                    put(packet, 4, parent);
                    put(packet, 6, content);
                    put(packet, 8, candidate);
                    this.receive(this.#localOwner() >>> 0, 0, packet);
                    need(hooks.broadcast)(packet);
                    const movement = need(hooks.movementType)(this.#pool.slot(candidate)[TYPE_OFFSET]);
                    need(hooks.removeMovement)(candidate, movement);
                    const target = need(hooks.chooseGeyser)(candidate);
                    if (target) need(hooks.goGeyser)(candidate, target);
                    const kind = read(this.#pool.slot(content), KIND_OFFSET, 2);
                    if (kind >= this.#types.length) throw new RangeError('전달 뒤 현재 내용물 TYPE 범위 오류');
                    if (this.#types[kind].flags2 & HOME_GENUS) need(hooks.goHome)(parent);
                    return TRANSFERRED;
                }
            }
            candidate = finder.next();
        }
        return MOVING;
    }

    // 42ac60: DWORD 감김 뒤 signed % 4 이며 음수 나머지를 양수로 정규화하지 않는다. 나머지가 0 일 때만 전달한다. (2026-11-08 WB04-E74)
    halt(parent) {
        const state = this.#state;
        state.haltCounter = ((((state.haltCounter + 1) | 0) % HALT_PERIOD) >>> 0);
        return state.haltCounter === 0 ? this.transfer(parent) : need(this.#hooks.commonHalt)(parent);
    }
}
