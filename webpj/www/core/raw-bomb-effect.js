// 0x5f 주문 효과 패킷의 수신 420bf0(대상 효과·지연 타이머·소리·부모 삭제)과 송신 observer 40aca0 을 옮긴 모듈이다(cpppj `o/RawBombEffect.cpp/.h`, 10.78 전용).
//  - finalizeBombEffectPacket(packet, writing): 쓰기일 때만 현재 대상 수(offset 6 DWORD, 버퍼에 들어가고 100 이하)로 선언 길이 WORD(offset 2)를 `10 + 2 × 수` 로 확정하고 나머지 바이트는 보존한다.
//  - RawBombEffect.receive(packet): 부모 WORD(offset 4)가 free/dead(상태 비트 3)이면 끝낸다. 부모의 운반체(+12 WORD)가 살아 있으면(상태 비트 7 없음) 운반체 시전 프로세스를 종료하고
//    부모 현재 TYPE 비용의 _ftol 하위 DWORD 를 누적 카운터에 더한다(DWORD 감김; Reward/Login 과 같은 자원 DWORD 를 빌리면 그 unsigned 비트를 다시 읽는다). 이어 최초 대상 수를 검사해
//    (signed 음수이거나 버퍼에 못 들어가면 거부) 대상 배열을 확보하고(실패는 오류), 매 반복 현재 패킷의 대상 수를 다시 읽으며 free/dead 가 아닌 대상마다 부모 현재 가상 표로 apply 한 뒤 최초 한도까지 저장한다.
//    권한이 있고 부모 현재 TYPE 이 지연 효과 종류(117/118)이면 종류를 먼저 캡처하고 40바이트를 확보해 지연 객체를 생성한 뒤(확보 실패여도 객체 0 으로 예약 인자·부착은 이어짐) 부착한다.
//    마지막으로 현재 TYPE 의 소리와 현재 가상 표로 부모를 삭제(flags 0x100048)하고, 최초 대상 배열은 정상/예외 모두 한 번만 해제한다.
// TYPE 표 { cost, sound, delay } 는 188개이며 상태 { boss, firstType, secondType, processType, spentCounter, sharedSpentCounter(resourcePoolBits/storeResourcePoolBits 를 가진 객체) } 는 효과 뒤 현재 값을 읽는다.
// 훅 { stopCast, allocate, apply, construct, argument, attach, sound, destroy, release } 는 도달한 지점에서만 요구한다(없으면 TypeError). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E54: RawBombEffect 이식으로 추가)

// 슬롯/패킷 필드 위치와 TYPE 수, 대상 최대 수, 삭제 flags, 상태 마스크이다. (2026-11-08 WB04-E54)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const CARRIER_OFFSET = 12;
const TYPE_COUNT = 188;
const HEADER_SIZE = 10;
const MAX_TARGETS = 100;
const DESTROY_FLAGS = 0x100048;
const DEAD_MASK = 3;
const CASTER_STOPPED_MASK = 7;
const TIMER_ALLOCATION_BYTES = 40;
const SIGNED64_LIMIT = 9223372036854775808;

// 선택한 하위의 누락을 무동작 성공으로 처리하지 않는다. (2026-11-08 WB04-E54)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('Bomb 효과 실제 하위 미연결');
    return hook;
}

// 호출자 소유 패킷의 현재 비정렬 값을 읽으며 실제 버퍼 밖 접근을 거부한다. (2026-11-08 WB04-E54)
function bytesAt(bytes, offset, width) {
    if (offset > bytes.length || width > bytes.length - offset) throw new RangeError('Bomb 효과 패킷 길이 부족');
    let value = 0;
    // 지정한 폭만 리틀엔디언으로 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value;
}

// 실제 signed64 절삭의 하위 DWORD 이며 비유한/범위 밖의 indefinite 도 하위 0 이다. (2026-11-08 WB04-E54)
function costLow(value) {
    if (!(value >= -SIGNED64_LIMIT && value < SIGNED64_LIMIT)) return 0;
    return (Math.trunc(value) | 0) >>> 0;
}

// 실제 원문 40aca0 의 Client 기본 등록 관찰기: 수신 방향(sending=0)은 payload 를 읽지 않고 복귀한다. 송신은 대상 수 DWORD 가 -1 이면 원문 진단(invalidCount)을 부르고(없으면 오류),
// 진단이 복귀한 뒤 현재 대상 수의 하위 WORD 를 다시 읽어 선언 길이를 `(수 + 5) × 2` 의 16비트 값으로 쓴다. 범위 검사 없이 현재 버퍼의 헤더 4BYTE 가 있어야 한다.
// `finalizeBombEffectPacket` 과 달리 수가 100 을 넘어도 거부하지 않는다(기준선 뒤 `01ec27ae` 의 장기 기본 observer). (2026-11-12 WB04-E328)
export function observeBombEffectPacket(packet, sending, invalidCount) {
    if (!sending) return;
    if (bytesAt(packet, 6, 4) === 0xffffffff) {
        if (typeof invalidCount !== 'function') throw new Error('Bomb 효과 count 원문 진단 장치 미연결');
        invalidCount();
    }
    // 진단 복귀 뒤 현재 count 의 하위 WORD 를 다시 읽는다.
    const length = ((bytesAt(packet, 6, 2) + 5) * 2) & 0xffff;
    if (packet.length < 4) throw new RangeError('Bomb 효과 패킷 길이 부족');
    packet[2] = length & 0xff;
    packet[3] = (length >> 8) & 0xff;
}

// 40aca0: 쓰기 관문을 먼저 지키고 실제 버퍼 범위 안에서 길이 WORD 만 변경한다. (2026-11-08 WB04-E54)
export function finalizeBombEffectPacket(packet, writing) {
    if (!writing) return;
    const count = bytesAt(packet, 6, 4);
    if (count > (packet.length - HEADER_SIZE) >> 1 || count > MAX_TARGETS) throw new RangeError('주문 효과 송신 대상 범위');
    const length = HEADER_SIZE + 2 * count;
    packet[2] = length & 0xff;
    packet[3] = (length >> 8) & 0xff;
}

// 주문 효과 수신기이다. 사용: new RawBombEffect(pool, types, state, hooks). (2026-11-08 WB04-E54)
export class RawBombEffect {
    #pool;
    #types;
    #state;
    #hooks;

    // 다른 판본과 현재 TYPE/저장소를 섞지 않는다. 훅은 생성 시점의 사본을 쓴다. (2026-11-08 WB04-E54)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || typeof state !== 'object' || state === null) throw new TypeError('Bomb 효과 판본/TYPE 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 같은 세션의 하위인지 확인하는 풀 참조이다. (2026-11-08 WB04-E54)
    get pool() {
        return this.#pool;
    }

    // 외부 효과 뒤 현재 번호가 바뀔 수 있으므로 호출마다 범위를 검사한다. (2026-11-08 WB04-E54)
    #type(kind) {
        if (kind >= this.#types.length) throw new RangeError('Bomb 효과 현재 TYPE 범위');
        return this.#types[kind];
    }

    // 물리 0번 슬롯과 free/dead 의 추가 필터 없이 원본이 지정한 폭만 읽는다. (2026-11-08 WB04-E54)
    #read(sid, offset, width) {
        return bytesAt(this.#pool.slot(sid), offset, width);
    }

    // 누적 카운터의 현재 DWORD 비트이다(공유 자원 DWORD 를 빌리면 그 unsigned 비트). (2026-11-08 WB04-E54)
    #spent() {
        const shared = this.#state.sharedSpentCounter;
        return shared ? shared.resourcePoolBits >>> 0 : this.#state.spentCounter >>> 0;
    }

    // 누적 카운터에 DWORD 가산 감김을 보존해 저장한다. (2026-11-08 WB04-E54)
    #storeSpent(value) {
        const shared = this.#state.sharedSpentCounter;
        if (shared) shared.storeResourcePoolBits(value >>> 0);
        else this.#state.spentCounter = value >>> 0;
    }

    // 현재 가상 효과는 상태를 바꿀 수 있으며 그 뒤 조회는 원본처럼 다시 수행한다. (2026-11-08 WB04-E54)
    receive(packet) {
        const hooks = this.#hooks;
        const state = this.#state;
        const parent = bytesAt(packet, 4, 2);
        if (this.#read(parent, STATE_OFFSET, 1) & DEAD_MASK) return;
        const carrier = this.#read(parent, CARRIER_OFFSET, 2);
        if (!(this.#read(carrier, STATE_OFFSET, 1) & CASTER_STOPPED_MASK)) {
            need(hooks.stopCast)(carrier);
            this.#storeSpent(this.#spent() + costLow(this.#type(this.#read(parent, TYPE_OFFSET, 1)).cost));
        }
        const capacity = bytesAt(packet, 6, 4);
        if ((capacity | 0) < 0 || capacity > (packet.length - HEADER_SIZE) >> 1) throw new RangeError('Bomb 효과 최초 대상 목록 범위');
        // 원본 NULL 쓰기를 성공으로 처리하지 않고 명시적으로 실패한다.
        if (!need(hooks.allocate)((capacity * 4) >>> 0)) throw new Error('Bomb 효과 대상 배열 확보 실패');
        const targets = new Array(capacity).fill(0);
        let saved = 0;
        try {
            // 순회 조건은 현재 패킷을 다시 읽고, 저장 한도는 최초 capacity 로 유지한다.
            for (let index = 0; (index | 0) < (bytesAt(packet, 6, 4) | 0); index += 1) {
                const target = bytesAt(packet, HEADER_SIZE + 2 * index, 2);
                if (this.#read(target, STATE_OFFSET, 1) & DEAD_MASK) continue;
                need(hooks.apply)(parent, this.#read(parent, 0, 4), target);
                if (saved < capacity) {
                    targets[saved] = target;
                    saved += 1;
                }
            }
            if (state.boss && (this.#read(parent, TYPE_OFFSET, 1) === state.firstType || this.#read(parent, TYPE_OFFSET, 1) === state.secondType)) {
                const first = this.#type(state.firstType);
                const second = this.#type(state.secondType);
                // 두 TYPE 참조와 종류 선택은 new 40 이전에 캡처한다.
                const secondEffect = this.#read(parent, TYPE_OFFSET, 1) !== state.firstType;
                let process = 0;
                if (need(hooks.allocate)(TIMER_ALLOCATION_BYTES)) process = need(hooks.construct)(secondEffect ? second.delay : first.delay, capacity, targets.slice(0, saved), secondEffect ? 2 : 1);
                const argument = need(hooks.argument)(0, 0);
                need(hooks.attach)(process, state.processType, argument, 0, 0);
            }
            need(hooks.sound)([this.#type(this.#read(parent, TYPE_OFFSET, 1)).sound, 0, 0, 0, 1, 0]);
            need(hooks.destroy)(parent, this.#read(parent, 0, 4), DESTROY_FLAGS);
        } catch (error) {
            need(hooks.release)();
            throw error;
        }
        need(hooks.release)();
    }
}
