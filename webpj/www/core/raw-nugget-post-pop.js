// 너겟 PostPop(00484f90)의 최초 두 Regular 예약과 Damageable 위임을 옮긴 모듈이다(cpppj `o/RawNuggetPostPop.cpp/.h`, 10.78 전용).
// flags bit0 일 때만 최초 예약을 한다: ① 부모에 이벤트 1 의 Regular 가 아직 없으면(hasRegular(parent, 1)) 28 BYTE 할당(allocate)이 성공했을 때에 한해 난수 random(0.0, 0.3)을 소비하고 construct(parent, 1, payload)로 생성/부착,
//   ② 그 결과와 무관하게 다시 allocate 가 성공하면 random(0.0, 0.5)을 소비하고 construct(parent, 3, payload). 할당이 실패한 예약은 난수를 소비하지 않고 뒤의 예약/기본 처리를 막지 않는다.
// 마지막에는 항상 기본 Damageable 몸체(damageable(parent, flags), 44bf80)를 호출한다. 너겟의 최초 실체와 그림 미리보기(flags 4)가 같은 부모 순서를 쓰며 미리보기에는 예약/난수가 필요 없다. 선택한 경계만 요구한다.
// random 의 반환(float 저장 결과)은 그대로 construct 에 전달한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E16: 너겟 PostPop 이식으로 추가)

// 두 난수 범위 상한의 단정도 값이다(0.3f, 0.5f). (2026-11-08 WB04-E16)
const FIRST_UPPER = Math.fround(0.3);
const SECOND_UPPER = Math.fround(0.5);

// 선택한 실제 경계가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E16)
function need(device) {
    if (typeof device !== 'function') throw new Error('너겟 실제 PostPop 하위 몸체 미연결');
    return device;
}

// pool 은 너겟과 Regular Form 의 공유 풀(10.78), hooks = { hasRegular(sid, 1), allocate(), random(low, high) → float, construct(sid, event, payload), damageable(sid, flags) }. (2026-11-08 WB04-E16)
export class RawNuggetPostPop {
    #pool;
    #hooks;

    // 다른 판본의 풀은 거부한다. 풀/효과의 수명은 이 객체보다 길어야 한다.
    constructor(pool, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('너겟 PostPop은 10.78 몸체입니다');
        this.#pool = pool;
        this.#hooks = hooks;
    }

    // flags bit0 일 때만 최초 예약을 하고 항상 기본 몸체를 호출한다.
    postPop(sid, flags) {
        const hooks = this.#hooks;
        if (flags & 1) {
            if (!need(hooks.hasRegular)(sid, 1) && need(hooks.allocate)()) {
                const payload = need(hooks.random)(0, FIRST_UPPER);
                need(hooks.construct)(sid, 1, payload);
            }
            if (need(hooks.allocate)()) {
                const payload = need(hooks.random)(0, SECOND_UPPER);
                need(hooks.construct)(sid, 3, payload);
            }
        }
        need(hooks.damageable)(sid, flags);
    }

    // 다른 월드로 단일 예약 메모리를 넘기지 않게 검사하는 데 쓰는 풀이다.
    get pool() { return this.#pool; }
}
