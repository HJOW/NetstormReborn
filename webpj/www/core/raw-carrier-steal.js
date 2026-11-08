// 크리스탈 크랩(Carrier)이 다른 운반체의 내용물을 포획하는 본문(429ed0 Try·42ab80 Halt·42abb0 PostPop)과 별도 60h 수신(4267c0)을 옮긴 모듈이다(cpppj `o/RawCarrierSteal.cpp/.h`, 10.78 전용).
// 포획은 로컬 수신/음향 뒤 송신하고 현재 기증자의 이동을 제거한다. 원본의 역방향 동맹/원소유자/현재 내용물 조회와 로컬 선행 효과를 유지하며, 효과 뒤 현재 전역(boss·편집·사제 타입·패킷)을 다시 읽는다.
//  - try(parent): boss 가 아니거나 부모에 이미 내용물이 있거나(find mask 0x20) 편집 모드면 10. 아니면 부모 좌표 칸(signed64 절삭의 하위 DWORD)의 일반 finder(flags 0)를 현재 체인 순서로 순회해 기증자 flags2 가 0x210000 이고 역방향 동맹
//    `alliances[기증자 소유자×9 + 부모 소유자]` 가 0 이며 크랩 타입이 아닌 후보의 내용물(find mask 0x20)을 찾는다. 내용물이 사제 타입이고 내용물 소유자(low7)가 부모 소유자이면 건너뛴다. 첫 적합 내용물은 10바이트 명령 `[프로토콜&0x7f, 메시지, 길이(2), 부모(2), 내용물(2), 기증자(2)]` 로
//    로컬 수신(receive) → 송신(broadcast) → 기증자의 이동 제거(movementType → removeMovement) → 내용물 TYPE flags2 에 0x8000 이 있으면 귀환(goHome) 뒤 12 를 반환하고 성공 뒤 다음 후보를 읽지 않는다. 없으면 10.
//  - halt(parent): 모든 크랩이 공유하는 카운터(부모별 타이머가 아님)를 증가시켜(DWORD 감김) signed % 4 로 만들고 0 이면 try, 아니면 공통 정지 검사(commonHalt)를 반환한다.
//  - postPop(parent, flags): extra&9 가 없고 flags&1 이며 Regular 할당이 되면 0x492 예약(1.0)을, flags&1 이 없고 boss 가 아니면 운반체 검사를 한 뒤 Damageable PostPop 을 한 번 실행한다.
//  - receive(owner, flags, packet): 8바이트 미만 거부. 내용물 또는 새 부모(패킷을 효과마다 다시 읽음)가 free 면 반환하고 아니면 내용물 분리(flags 0) → 새 부모에 부착(flags 0) → 미리보기 예약 → `crabSteal.wav` 재생을 한다(owner/flags 와 기증자 WORD 는 읽지 않음).
//  - handles(parent): 크랩 0x503f20 과 예약 187 의 0x503ff0 가상 표(둘 다 42abb0 사용)를 받는다.
// 모든 참조는 같은 세션의 현재 값을 읽는다: boss/editor/localOwner/carrierType/priestType 은 getter, haltCounter 는 `{ value }`, header 는 `{ message, protocol, length }`, alliances 는 81 DWORD, hooks 는 CarrierStealHooks 의 JS 대응이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E92: 크리스탈 크랩 포획 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 크랩/예약 187 의 가상 표, 포획 상수(사제 아닌 기증자 flags2·귀환 flags2·소유자 low7), 성공/실패 반환, Regular 사건과 재생 음향이다. (2026-11-08 WB04-E92)
const CRAB_TABLES = Object.freeze([0x503f20, 0x503ff0]);
const DONOR_FLAGS2 = 0x210000;
const GO_HOME_FLAGS2 = 0x8000;
const FAILED = 10;
const SUCCEEDED = 12;
const RESERVE_EVENT = 0x492;
const STEAL_SOUND = 'crabSteal.wav';
// 슬롯의 TYPE(+10)·상태(+11)·내용물 TYPE WORD(+18)·내용물 소유자 BYTE(+12)·소유자(+34)·extra(+40)·좌표(+14/+18) 위치이다. (2026-11-08 WB04-E92)
const TYPE = 10;
const STATE = 11;
const OWNER_BYTE = 12;
const KIND = 18;
const OWNER = 34;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;
const PLAYER_SLOTS = 9;

// 선택된 실제 경계가 없으면 빈 성공으로 대체하지 않는다. (2026-11-08 WB04-E92)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('크리스탈 크랩 포획 실제 하위 미연결');
    return hook;
}

// 비정렬 WORD/DWORD 를 원본 폭만큼 읽는다. (2026-11-08 WB04-E92)
function read(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 지정 WORD 를 쓴다. (2026-11-08 WB04-E92)
function putWord(raw, offset, value) {
    raw[offset] = value & 0xff;
    raw[offset + 1] = (value >>> 8) & 0xff;
}

// 비정렬 float 좌표를 읽는다. (2026-11-08 WB04-E92)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 원본 4202f0 의 41d770 signed64 절삭과 하위 signed32 를 보존한다(비유한/범위 밖은 0). (2026-11-08 WB04-E92)
function cell(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

export class RawCarrierSteal {
    // 부모/기증자/내용물의 같은 50바이트 풀, 실제 점 finder 의 해시, 현재 genus·내용물 TYPE 표, 현재 전역 getter 들, 방향별 동맹, 헤더와 공유 정지 카운터, 실제 도달한 하위이다.
    #pool;
    #hash;
    #types;
    #boss;
    #editor;
    #localOwner;
    #alliances;
    #carrierType;
    #priestType;
    #header;
    #haltCounter;
    #hooks;

    // 10.78 의 실제 전역을 공유하며 10.72 의 다른 포획 본문을 이 몸체와 같다고 추정하지 않는다. (2026-11-08 WB04-E92)
    constructor(pool, hash, types, boss, editor, localOwner, alliances, carrierType, priestType, header, haltCounter, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || alliances.length !== 81) throw new TypeError('크리스탈 크랩 포획 판본/현재표 오류');
        if ([boss, editor, localOwner, carrierType, priestType].some((getter) => typeof getter !== 'function') || typeof haltCounter !== 'object' || haltCounter === null) throw new TypeError('크리스탈 크랩 포획 현재 전역 연결 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#boss = boss;
        this.#editor = editor;
        this.#localOwner = localOwner;
        this.#alliances = alliances;
        this.#carrierType = carrierType;
        this.#priestType = priestType;
        this.#header = header;
        this.#haltCounter = haltCounter;
        this.#hooks = hooks ?? {};
    }

    // 실제 원본과 같은 풀만 반환한다(다른 세션의 내용물/공간 연결 검사용). (2026-11-08 WB04-E92)
    get pool() { return this.#pool; }

    // 크랩과 예약 187 의 두 실제 +20 은 모두 42abb0 이다. TYPE/가상 Halt 재정의는 이 주소 판단에 섞지 않는다. (2026-11-08 WB04-E92)
    handles(parent) {
        return CRAB_TABLES.includes(read(this.#pool.slot(parent), 0));
    }

    // 최초의 extra/flags 관문 뒤 현재 boss 를 읽으며 파생 예약 뒤 공통 장부를 한 번 실행한다. (2026-11-08 WB04-E92)
    postPop(parent, flags) {
        const hooks = this.#hooks;
        if (!(this.#pool.slot(parent)[EXTRA] & 9) && (flags & 1) && need(hooks.allocateRegular)()) need(hooks.regular)(parent, RESERVE_EVENT, 1.0);
        if (!(flags & 1) && !this.#boss()) need(hooks.check)(parent);
        need(hooks.damageablePostPop)(parent, flags >>> 0);
    }

    // inc DWORD 감김 뒤 signed % 4 이며 모든 부모가 같은 counter 를 사용한다. (2026-11-08 WB04-E92)
    halt(parent) {
        const counter = this.#haltCounter;
        counter.value = (((counter.value + 1) | 0) % 4) >>> 0;
        return counter.value === 0 ? this.try(parent) : need(this.#hooks.commonHalt)(parent);
    }

    // free 만 검사하고 dead/소유자/기증자 필터를 추가하지 않는다. 부모 WORD 는 효과 뒤 다시 읽는다. (2026-11-08 WB04-E92)
    receive(owner, flags, packet) {
        if (packet.length < 8) throw new RangeError('크리스탈 크랩 포획 명령 길이 오류');
        const hooks = this.#hooks;
        const pool = this.#pool;
        const content = read(packet, 6, 2);
        if ((pool.slot(content)[STATE] & 1) || (pool.slot(read(packet, 4, 2))[STATE] & 1)) return;
        need(hooks.detach)(content, 0);
        need(hooks.attach)(content, read(packet, 4, 2), 0);
        need(hooks.preview)(read(packet, 4, 2));
        need(hooks.sound)(STEAL_SOUND, 0, 0, 0, 1, 0);
    }

    // 현재 후보의 genus 0x210000/역방향 동맹/크랩 제외 뒤 첫 적합 내용물을 포획한다. 실제 포획 뒤 12, 나머지 10 이다. (2026-11-08 WB04-E92)
    try(parent) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        if (!this.#boss() || need(hooks.find)(parent, 0x20, 0, 0) || this.#editor()) return FAILED;
        const raw = pool.slot(parent);
        const x = cell(coordinate(raw, X_AT));
        const y = cell(coordinate(raw, Y_AT));
        const finder = new RawSquidFinder(pool, this.#hash, this.#types);
        // finder 의 현재 체인 순서를 유지하고 성공 뒤 다음 후보를 읽지 않는다.
        for (let candidate = finder.begin({ left: x, top: y, right: x, bottom: y }, 0); candidate; candidate = finder.next()) {
            const donor = pool.slot(candidate);
            if (donor[TYPE] >= this.#types.length) throw new RangeError('포획 현재 기증자TYPE 오류');
            if (!(this.#types[donor[TYPE]].flags2 & DONOR_FLAGS2)) continue;
            const first = donor[OWNER];
            const second = pool.slot(parent)[OWNER];
            if (first >= PLAYER_SLOTS || second >= PLAYER_SLOTS) throw new RangeError('포획 현재 동맹owner 오류');
            if (this.#alliances[first * PLAYER_SLOTS + second] || donor[TYPE] === this.#carrierType()) continue;
            const content = need(hooks.find)(candidate, 0x20, 0, 0);
            if (!content) continue;
            const contained = pool.slot(content);
            if (read(contained, KIND, 2) === this.#priestType() && (contained[OWNER_BYTE] & 0x7f) === pool.slot(parent)[OWNER]) continue;
            const header = this.#header;
            const packet = new Uint8Array(10);
            packet[0] = header.protocol & 0x7f;
            packet[1] = header.message & 0xff;
            packet[2] = header.length & 0xff;
            packet[3] = (header.length >>> 8) & 0xff;
            putWord(packet, 4, parent);
            putWord(packet, 6, content);
            putWord(packet, 8, candidate);
            this.receive(this.#localOwner(), 0, packet);
            need(hooks.broadcast)(packet);
            const movement = need(hooks.movementType)(pool.slot(candidate)[TYPE]);
            need(hooks.removeMovement)(candidate, movement);
            const kind = read(pool.slot(content), KIND, 2);
            if (kind >= this.#types.length) throw new RangeError('포획 뒤 현재 내용물TYPE 오류');
            if (this.#types[kind].flags2 & GO_HOME_FLAGS2) need(hooks.goHome)(parent);
            return SUCCEEDED;
        }
        return FAILED;
    }
}
