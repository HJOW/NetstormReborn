// 그라비테이션·힐·미티어 세 주문 효과의 최초 Regular 예약 접두를 옮긴 모듈이다(cpppj `o/RawBombPostPop.cpp/.h`, 10.78 전용; 원본 0041f870/0041f920/0041fab0).
//  - handles(parent): 현재 슬롯의 가상 표가 세 표(502c38 그라비테이션·502cf0 힐·502db0 미티어)일 때만 참이다(표시 TYPE 과 무관).
//  - prefix(parent, flags): flags 비트 1 이 있고 이미 예약된 Regular 가 없으며 새 예약이 할당될 때만 지연 시간을 정해 예약한다. 그라비테이션은 0.05(float), 힐·미티어는 공유 게임 난수 한 번(65536 미만)으로
//    `rand/65536 × |high − low| + low`(low = 힐 0, 미티어 0.01; high = 0.05) 이며 힐은 double 0.05 를 더한 뒤 한 번 float 로 저장한다. 중복이거나 할당 실패면 난수를 소비하지 않는다.
//    시전/공격 프레임은 실행하지 않고 공통 장부는 상위 분배(SquidPostPop)가 한 번 실행한다.
// 훅 { hasRegular(parent, 1), reserveRegular(), addRegular(parent, 1, delay) } 는 원본 호출 위치에 도달했을 때만 요구한다(없으면 TypeError). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E51: RawBombPostPop 이식으로 추가)

// 세 실제 가상 표(+20 = 41f870/41f920/41fab0)이다. (2026-11-08 WB04-E51)
const GRAVITY = 0x502c38;
const HEAL = 0x502cf0;
const METEOR = 0x502db0;

// 난수 상한(65536)과 지연 폭, 그라비테이션 지연, 힐이 더하는 double 상수이다. (2026-11-08 WB04-E51)
const RANDOM_RANGE = 65536;
const HIGH_DELAY = Math.fround(0.05);
const HEAL_BONUS = 0.05;
const GRAVITY_DELAY = Math.fround(0.05);
const METEOR_LOW = Math.fround(0.01);
const HEAL_LOW = 0;

// 현재 슬롯의 첫 DWORD 를 리틀엔디언으로 읽는다(TYPE/상태에 추가 제약을 두지 않는다). (2026-11-08 WB04-E51)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 도달한 콜백만 요구해 호출 시점에 한 번 읽는다. (2026-11-08 WB04-E51)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('주문 효과 Pop 의 선택 장치 미연결');
    return hook;
}

// 주문 효과 Pop 접두 실행기이다. 사용: new RawBombPostPop(pool, random, { hasRegular, reserveRegular, addRegular }). random 은 GameRandom 이다. (2026-11-08 WB04-E51)
export class RawBombPostPop {
    #pool;
    #random;
    #hooks;

    // PATCH 전용 장치의 판본을 확인한다. (2026-11-08 WB04-E51)
    constructor(pool, random, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('주문 효과 Pop 은 10.78 전용');
        this.#pool = pool;
        this.#random = random;
        this.#hooks = hooks ?? {};
    }

    // 공통 후처리 연결에서 다른 월드의 풀을 거부하기 위한 같은 풀 참조이다. (2026-11-08 WB04-E51)
    get pool() {
        return this.#pool;
    }

    // 현재 슬롯이 세 주문 효과의 가상 표인지 확인한다. (2026-11-08 WB04-E51)
    handles(parent) {
        const table = vtableOf(this.#pool.slot(parent));
        return table === GRAVITY || table === HEAL || table === METEOR;
    }

    // 선택한 가상 몸체는 외부 조회가 표를 바꿔도 이번 호출 동안 유지한다. (2026-11-08 WB04-E51)
    prefix(parent, flags) {
        const table = vtableOf(this.#pool.slot(parent));
        if (table !== GRAVITY && table !== HEAL && table !== METEOR) throw new Error('미지원 주문 효과 Pop 가상 표');
        const hooks = this.#hooks;
        if (!(flags & 1) || need(hooks.hasRegular)(parent, 1) || !need(hooks.reserveRegular)()) return;
        let delay = GRAVITY_DELAY;
        if (table !== GRAVITY) {
            const low = table === HEAL ? HEAL_LOW : METEOR_LOW;
            const high = HIGH_DELAY;
            // 할당 실패/중복이면 공유 난수를 소비하지 않는다(이 줄에 도달했을 때만 한 번 소비).
            const selected = this.#random.next(RANDOM_RANGE) * (1.0 / RANDOM_RANGE) * Math.abs(high - low) + low;
            delay = Math.fround(selected + (table === HEAL ? HEAL_BONUS : 0.0));
        }
        need(hooks.addRegular)(parent, 1, delay);
    }
}
