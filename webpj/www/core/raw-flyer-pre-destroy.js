// 플라이어(맨 오 워 등) 삭제 전처리(0045e7e0)의 현재 +8 WORD 대상 정리와 Damageable 하위 순서를 옮긴 모듈이다(cpppj `o/RawFlyerPreDestroy.cpp/.h`, 10.78 전용).
// 순서: 슬롯의 +8 WORD(DWORD 나 종속 head 로 넓혀 읽지 않음)가 비0 이면 그 대상을 가상 삭제(destroy(target, 0))한 뒤, callback 안에서 raw 가 바뀌었더라도 원본 this 와 flags 로 Damageable 전처리(0044b4b0)를 한 번 호출한다.
// 실제 가상표 지원 범위는 상위 분배기가 확인한다. 풀 판본이나 하위 장치가 없으면 효과 전에 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E14: 플라이어 삭제 전처리 이식으로 추가)

// 같은 10.78 풀과 현재 월드의 하위 destroy(sid, flags)·damageable(sid, flags)를 연결한다. 모든 참조는 객체보다 오래 살아야 한다. (2026-11-08 WB04-E14)
export class RawFlyerPreDestroy {
    #pool;
    #hooks;

    // 미복원 판본/하위 누락은 효과 전에 거부한다.
    constructor(pool, hooks) {
        if (pool?.edition !== 'patch1078' || typeof hooks?.destroy !== 'function' || typeof hooks.damageable !== 'function') throw new TypeError('플라이어 전처리 판본/실제 하위 누락');
        this.#pool = pool;
        this.#hooks = hooks;
    }

    // 현재 WORD 가 비0 이면 먼저 삭제하고 callback 뒤 같은 raw 자산을 Damageable 로 처리한다.
    preDestroy(sid, flags) {
        const raw = this.#pool.slot(sid);
        const target = raw[8] | (raw[9] << 8);
        if (target) this.#hooks.destroy(target, 0);
        this.#hooks.damageable(sid, flags);
    }
}
