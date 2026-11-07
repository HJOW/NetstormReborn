// 주문 객체의 명령 수신(0x5c, 41e860)에서 원격 비용 처리와 주문 Regular 예약을 옮긴 모듈이다(cpppj `o/RawBombActivation.cpp/.h`, 10.78 전용).
// 6 BYTE 패킷의 부모 WORD(+4)가 가리키는 주문 객체가 살아 있으면(상태 bit0/1 없음) 다음을 수행한다.
//   ① 소유자(+34)가 현재 로컬 owner 와 다르면 원격 비용을 처리한다: 현재 TYPE 의 복호화 비용 단위(cost)를 32비트 부호 반전한 값을 float 로 만들어 subtract(owner, amount)로 원격 소유자 SP 를 증감한다.
//   ② 효과로 바뀔 수 있는 현재 TYPE(+10)의 genus 를 다시 읽어 주문 비트(flags2 0x100)가 없으면 원문 진단(assertion(0x206), Bomb.cpp 206)을 요청한다(복귀 가능).
//   ③ 40 BYTE 할당(allocate(40))이 성공하면 같은 메모리로 construct(event 0, parent, 0.1f)를 호출한다.
// 생성/피해가 아니라 명령 수신과 예약만 다룬다. 패킷의 수신자/연결 인자는 원본이 읽지 않으며 부모 WORD 만 쓴다. 실제 선택된 하위 누락은 무동작 성공으로 처리하지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E17: 주문 활성화 수신 이식으로 추가)

// 예약 지연(단정도 0.1)과 주문 genus 비트, 진단 줄 번호이다. (2026-11-08 WB04-E17)
const ACTIVATION_DELAY = Math.fround(0.1);
const BOMB_GENUS = 0x100;
const BOMB_ASSERT_LINE = 0x206;

// 선택된 하위가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E17)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('주문 활성화 실제 하위 미연결');
    return hook;
}

// pool 은 주문 객체의 물리 50 BYTE 슬롯을 가진 10.78 SidPool, types 는 188칸 TYPE 표({ flags2 }), localOwner() 는 현재 540c70 DWORD getter,
// hooks = { cost(type) → 정수, subtract(owner, float), assertion(줄), allocate(bytes) → bool, construct(event, parent, delay) } 이다. (2026-11-08 WB04-E17)
export class RawBombActivation {
    #pool;
    #types;
    #localOwner;
    #hooks;

    // 외부 효과를 실행하지 않고 현재 저장소/전역만 빌린다.
    constructor(pool, types, localOwner, hooks = {}) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || typeof localOwner !== 'function') throw new TypeError('주문 활성화 판본/TYPE 오류');
        this.#pool = pool;
        this.#types = types;
        this.#localOwner = localOwner;
        this.#hooks = hooks;
    }

    // 6 BYTE 패킷의 부모가 살아 있으면 원격 비용과 Regular 를 처리한다.
    receive(packet) {
        if (packet.length < 6) throw new TypeError('주문 활성화 패킷 길이 부족');
        const parent = packet[4] | (packet[5] << 8);
        if (this.#pool.slot(parent)[11] & 3) return;
        const owner = this.#pool.slot(parent)[34];
        if (owner !== (this.#localOwner() >>> 0)) {
            const cost = need(this.#hooks.cost)(this.#pool.slot(parent)[10]);
            // 원본 neg 의 32비트 감김이다.
            const negative = (0 - (cost >>> 0)) | 0;
            need(this.#hooks.subtract)(owner, Math.fround(negative));
        }
        const type = this.#pool.slot(parent)[10];
        if (type >= this.#types.length) throw new RangeError('주문 활성화 현재 TYPE 범위');
        if (!((this.#types[type].flags2 >>> 0) & BOMB_GENUS)) need(this.#hooks.assertion)(BOMB_ASSERT_LINE);
        if (need(this.#hooks.allocate)(40)) need(this.#hooks.construct)(0, parent, ACTIVATION_DELAY);
    }

    // 다른 Session 의 모듈을 혼합하지 않도록 확인하는 데 쓰는 풀이다.
    get pool() { return this.#pool; }
}
