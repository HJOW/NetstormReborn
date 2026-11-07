// 주문 효과(Point Blast 계열)의 Regular 본체 41e6a0 을 옮긴 모듈이다: 주문 초기화(count 0)·시전 시간 비교·대상 패킷 배포(cpppj `o/RawBombRegular.cpp/.h`, 10.78 전용).
//  - handle(parent, event, count, payload): event 번호는 읽지 않는다(원본 기본 496e70). count 0 이면 부모의 운반체(+12 WORD)가 있을 때 notice(운반체, 6, 시전시간 × 2)를 알리고 현재 TYPE 의 첫 프레임으로 frame(부모, 첫 프레임, 0),
//    아니면 animate(부모, 1, 0). 권한(boss)이 없으면 payload 를 돌려준다. 있으면 `(float)(count(signed) × payload)` 가 현재 TYPE 의 시전 시간(signed)을 넘을 때만 대상 최대 100개를 검색해
//    210바이트(0xff 채움) 패킷 [protocol&0x7f, message, 선언 길이(최소 4) WORD, 부모 WORD, 대상 수 DWORD, 대상 WORD 목록]을 만들고 broadcast → 현재 owner 로 receive(owner, 0, 패킷) 한 뒤 대상 배열을 해제한다.
//    반환은 수신 뒤(해제 전) 부모 상태 비트 7 이 있으면 −1(자기 종료), 없으면 0 이다. 확보 실패는 시전 성공으로 세지 않고 오류로 거부하며, 검색~수신 도중 예외에도 해제는 반드시 부른다.
// TYPE 두 값 { firstFrame(TYPE+130), castTime(TYPE+1b8, signed) } 는 188개 표이고, 상태 { boss, localOwner, effect: { message, protocol, length } } 는 효과 뒤에도 현재 값을 읽는 살아 있는 객체이다.
// 훅 { notice, frame, animate, allocate, targets, broadcast, receive, release } 는 도달한 지점에서만 요구한다(없으면 TypeError). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E54: RawBombRegular 이식으로 추가)

// 슬롯 필드 위치, 대상 수/패킷 크기, 해제 크기, 상태 비트이다. (2026-11-08 WB04-E54)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const CARRIER_OFFSET = 12;
const TYPE_COUNT = 188;
const TARGET_COUNT = 100;
const PACKET_SIZE = 210;
const ALLOCATION_BYTES = 400;
const STOPPED_MASK = 7;
const CAST_NOTICE_CODE = 6;

// 선택한 경계 누락을 무동작 성공으로 처리하지 않는다. (2026-11-08 WB04-E54)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('Bomb Regular 실제 하위 미연결');
    return hook;
}

// 원본의 비정렬 WORD/DWORD 필드를 리틀엔디언으로 쓴다(지정한 폭 밖의 0xff 초기화 바이트는 유지). (2026-11-08 WB04-E54)
function put(bytes, offset, value, width) {
    // 낮은 바이트부터 지정한 폭만 기록한다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// 주문 Regular 실행기이다. 사용: new RawBombRegular(pool, types, state, hooks). (2026-11-08 WB04-E54)
export class RawBombRegular {
    #pool;
    #types;
    #state;
    #hooks;

    // 다른 판본의 타입 배치와 수신 몸체를 섞지 않는다. 훅은 생성 시점의 사본을 쓴다(원본 C++ 복사 의미). (2026-11-08 WB04-E54)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || typeof state !== 'object' || state === null) throw new TypeError('Bomb Regular 판본/TYPE 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 같은 세션 연결 여부를 확인하는 풀 참조이다. (2026-11-08 WB04-E54)
    get pool() {
        return this.#pool;
    }

    // 효과 뒤 바뀐 현재 TYPE 를 매번 다시 읽는다. (2026-11-08 WB04-E54)
    #type(parent) {
        const type = this.#pool.slot(parent)[TYPE_OFFSET];
        if (type >= this.#types.length) throw new RangeError('Bomb Regular 현재 TYPE 범위');
        return this.#types[type];
    }

    // event 종류와 무관하게 count 0 에서 초기화하고 매 반복에서 현재 권한/시간을 확인한다. 반환은 단정도 float 이다. (2026-11-08 WB04-E54)
    handle(parent, event, count, payload) {
        void event;
        const hooks = this.#hooks;
        const state = this.#state;
        const pool = this.#pool;
        const value = Math.fround(payload);
        if (!count) {
            const raw = pool.slot(parent);
            const carrier = raw[CARRIER_OFFSET] | (raw[CARRIER_OFFSET + 1] << 8);
            if (carrier) need(hooks.notice)(carrier, CAST_NOTICE_CODE, ((this.#type(parent).castTime >>> 0) * 2) >>> 0);
            need(hooks.frame)(parent, this.#type(parent).firstFrame >>> 0, 0);
        } else {
            need(hooks.animate)(parent, 1, 0);
        }
        if (!state.boss) return value;
        // 원본 x87 곱을 float32 로 저장한 뒤 시전 시간과 비교한다.
        const elapsed = Math.fround((count | 0) * value);
        if (!(this.#type(parent).castTime < elapsed)) return value;
        if (!need(hooks.allocate)(ALLOCATION_BYTES)) throw new Error('Bomb Regular 대상 배열 확보 실패');
        const targets = new Array(TARGET_COUNT).fill(0);
        const packet = new Uint8Array(PACKET_SIZE);
        try {
            const countTargets = need(hooks.targets)(parent, targets);
            if (countTargets > targets.length) throw new RangeError('Bomb Regular 대상 100개 초과');
            packet.fill(0xff);
            packet[0] = state.effect.protocol & 0x7f;
            packet[1] = state.effect.message & 0xff;
            put(packet, 2, Math.max(state.effect.length & 0xffff, 4), 2);
            put(packet, 4, parent, 2);
            put(packet, 6, countTargets, 4);
            // 대상 DWORD 의 낮은 WORD 만 복사한다(디컴파일에서 누락된 실제 40ace0 반복).
            for (let index = 0; index < countTargets; index += 1) put(packet, 10 + 2 * index, targets[index], 2);
            need(hooks.broadcast)(packet);
            need(hooks.receive)(state.localOwner, 0, packet);
        } catch (error) {
            need(hooks.release)(ALLOCATION_BYTES);
            throw error;
        }
        // free 이전에 캡처하며 free 의 재진입 변화는 반환값에 영향을 주지 않는다.
        const stopped = (pool.slot(parent)[STATE_OFFSET] & STOPPED_MASK) !== 0;
        need(hooks.release)(ALLOCATION_BYTES);
        return stopped ? -1 : 0;
    }
}
