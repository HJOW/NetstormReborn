// 경로(Path) 프로세스의 Halt 안에서 운반 대상이 죽었을 때 새 귀환 신전/간헐천을 고르고 새 이동을 시작하는 분기(489ff0)를 옮긴 모듈이다(cpppj `o/RawPathRetarget.cpp/.h`, 10.78 전용).
// 대상이 살아 있으면 새 이동/내용물 조회 없이 현재 경로를 계속 진행한다. 전체 Halt 의 권한/부모 가상/살아 있음 검사는 `RawPathProcess` 가 수행한다(이 모듈은 대상 사망 분기만 맡는다).
//  - update(runtime, initialParent): 경로 이미지의 두 도착 대상(귀환 targetB·간헐천 targetA)과 현재 부모 DWORD(+8)를 첫 조회 전에 보관한다. 이어
//    ① 귀환 대상이 0 이 아니고 보관한 귀환 대상이 죽었고(dead 비트 2; free 비트 1 과 별개) 현재 부모의 내용물(mask 0x20, 현재 너겟 타입)이 있으면 귀환 후보를 다시 골라 targetB 를 새 후보로 쓰고
//    후보가 있으면 귀환 이동을 시작한 뒤 끝낸다. ② 아니면 간헐천 대상이 0 이 아니고 보관한 간헐천 대상이 죽었고 **Halt 전 부모**(initialParent)에게 내용물이 없으면 간헐천 후보를 다시 골라 targetA 를 쓰고
//    후보가 있으면 targetB 를 0 으로 지운 뒤 간헐천 이동을 시작한다. 이동 호출 중 기존 경로가 삭제될 수 있으므로 호출 뒤에는 경로를 다시 읽지 않는다. 대상 SID 는 실제로 조회할 때만 물리 범위를 확인한다.
// nuggetType 은 각 조회 직전에 다시 읽는 5412c0 의 현재 값(getter 또는 `{ value }`), hooks = { find(parent, mask, type, index)→sid, chooseReturn(parent)→sid, chooseGeyser(parent)→sid, goReturn(parent, target), goGeyser(parent, target) } 이며
// 도달한 훅이 없으면 오류이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E103: 경로 대상 재확인 이식으로 추가)

// 내용물 조회 mask 이다(타입 번호와 혼동하지 않는다). (2026-11-08 WB04-E103)
const NUGGET_MASK = 0x20;
// 슬롯의 상태 위치와 dead 비트, 경로 이미지의 부모 DWORD 위치이다. (2026-11-08 WB04-E103)
const STATE_AT = 11;
const DEAD = 2;
const PARENT_AT = 8;

// 선택된 큰 하위가 없으면 성공이나 빈 동작으로 대체하지 않는다. (2026-11-08 WB04-E103)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('경로 대상 재확인의 실제 하위 미연결');
    return hook;
}

// 경로 대상 재확인 처리기이다. (2026-11-08 WB04-E103)
export class RawPathRetarget {
    // 부모와 보관한 두 대상의 같은 실제 raw 저장소이다. (2026-11-08 WB04-E103)
    #pool;
    // 각 Find 직전에 다시 읽는 5412c0 의 현재 너겟 타입 소유자(getter 또는 { value })이다. (2026-11-08 WB04-E103)
    #nuggetType;
    // 현재 내용물/후보/이동의 큰 실제 경계이다. (2026-11-08 WB04-E103)
    #hooks;

    // PATCH 의 물리 폭과 현재 getter 수명을 확인한다. CD 의 다른 Halt 본문을 PATCH 구현으로 대신 실행하지 않는다. (2026-11-08 WB04-E103)
    constructor(pool, nuggetType, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new Error('경로 대상 재확인의 판본 오류');
        this.#pool = pool;
        this.#nuggetType = nuggetType;
        this.#hooks = hooks;
    }

    // 다른 월드의 경로/조회 연결을 거부하기 위한 같은 풀이다. (2026-11-08 WB04-E103)
    get pool() { return this.#pool; }

    // 현재 너겟 타입이다. (2026-11-08 WB04-E103)
    #type() { return typeof this.#nuggetType === 'function' ? this.#nuggetType() : this.#nuggetType.value; }

    // 0 번도 실제 물리 슬롯이며 주소 계산만 했던 무효 SID 를 미리 조회하지 않는다. DWORD SID 의 물리 범위는 실제로 조회할 때만 확인한다. (2026-11-08 WB04-E103)
    #number(value) {
        if (value >= this.#pool.capacity || value > 0xffff) throw new RangeError('경로 대상 재확인의 물리 SID 범위 오류');
        return value;
    }

    // 콜백이 대상 번호를 바꾸어도 최초 보관한 대상 SID 의 현재 dead 비트(2)만 읽는다. free(1)는 별개이다. (2026-11-08 WB04-E103)
    #dead(value) { return (this.#pool.slot(this.#number(value))[STATE_AT] & DEAD) !== 0; }

    // SetTargets 는 다른 필드의 현재 값을 그대로 보존한다. 새 이동 호출 뒤에는 Runtime 을 다시 읽지 않는다. (2026-11-08 WB04-E103)
    update(runtime, initialParent) {
        const hooks = this.#hooks;
        const savedReturn = runtime.targetB;
        const savedGeyser = runtime.targetA;
        const bytes = runtime.bytes;
        const currentParent = (bytes[PARENT_AT] | (bytes[PARENT_AT + 1] << 8) | (bytes[PARENT_AT + 2] << 16) | (bytes[PARENT_AT + 3] << 24)) >>> 0;
        if (runtime.targetB !== 0 && this.#dead(savedReturn) && need(hooks.find)(this.#number(currentParent), NUGGET_MASK, this.#type(), 0)) {
            const next = need(hooks.chooseReturn)(this.#number(currentParent));
            runtime.setTargets(runtime.targetA, next);
            if (next) need(hooks.goReturn)(this.#number(currentParent), next);
            return;
        }
        if (runtime.targetA !== 0 && this.#dead(savedGeyser) && !need(hooks.find)(initialParent, NUGGET_MASK, this.#type(), 0)) {
            const next = need(hooks.chooseGeyser)(this.#number(currentParent));
            runtime.setTargets(next, runtime.targetB);
            if (next) {
                runtime.setTargets(runtime.targetA, 0);
                need(hooks.goGeyser)(this.#number(currentParent), next);
            }
        }
    }
}
