// 원본 10.78 contained(내용물) Form 생성 보조(004afa70)를 옮긴 모듈이다. 생산 칸·저장 내용물 등에 넣는 Form 을 만든다.
//  - 순서: 부모 범위 확인 → flags 0 이면 0x20 → 부착 flags 로 할당 flags(0x20 예측 1, 0x10 client 2, 그 밖 0) → 현재 contained TYPE 으로 생성
//    → +18 TYPE WORD·+12 인자 WORD → 생성된 TYPE 이 현재 다리 TYPE 이면 표면 알림 → +30 품질 BYTE·+20 flags WORD → 가상 부착(+0x50).
//  - contained/다리 TYPE 은 호출 시점마다 state 의 getter 로 다시 읽는다(자식 효과가 바꾼 값을 사용).
// 근거: cpppj `o/RawContainedCreate.cpp/.h`. 원본 관측 fixture 는 없고 cpppj 계약 검사(FormCreationCurrent·FormLifecycle)를 옮긴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-06 WB04-D5a: Content 호스트의 BindCreate 와 함께 추가)

// 원문 초기 전역값(contained 6·다리 82)을 쓰는 독립 입력 상태이다. 실제 세션은 같은 GameAuxState/World 의 현재 값을 getter 로 준다. (2026-11-06 WB04-D5a)
export const INDEPENDENT_CONTAINED_STATE = Object.freeze({ containedType: () => 6, bridgeType: () => 82 });

// 슬롯 WORD 만 쓴다(이웃 보존). (2026-11-06 WB04-D5a)
function putWord(raw, offset, value) {
    raw[offset] = value & 0xff;
    raw[offset + 1] = (value >>> 8) & 0xff;
}

// contained Form 생성기이다. state 는 { containedType(), bridgeType() }, hooks 는 { create(type, flags) → SID, notifySurface(sid), attach(sid, parent, flags) } (셋 다 필수).
// 사용: new RawContainedCreate(pool, state, makeContainedCreateHooks(pool, factory, hooks)).create(parent, type, argument, flags, attachFlags, quality). (2026-11-06 WB04-D5a)
export class RawContainedCreate {
    // 같은 풀·현재 전역 getter·가상 경계이다.
    #pool;
    #state;
    #hooks;

    // 세 가상 경계가 모두 있어야 한다.
    constructor(pool, state = INDEPENDENT_CONTAINED_STATE, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('contained 생성은 10.78 SidPool 만 받습니다.');
        if (typeof state?.containedType !== 'function' || typeof state?.bridgeType !== 'function') throw new TypeError('contained 생성 상태는 두 getter 가 필요합니다.');
        if (typeof hooks.create !== 'function' || typeof hooks.notifySurface !== 'function' || typeof hooks.attach !== 'function') throw new TypeError('contained 생성 경계 누락');
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 같은 풀인지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 004afa70: 원문 순서로 생성·필드 쓰기·표면 알림·부착을 한다. flags 는 DWORD 전체가 0 일 때만 0x20 으로 바꾸며 raw 에는 하위 WORD/BYTE 만 쓴다.
    create(parent, type, argument, flags, attachFlags, quality) {
        const pool = this.#pool;
        if (!parent || parent >= pool.capacity) throw new RangeError('contained 부모 SID 범위 오류');
        const listFlags = flags === 0 ? 0x20 : flags;
        const factoryFlags = (attachFlags & 0x20) ? 1 : ((attachFlags & 0x10) ? 2 : 0);
        const sid = this.#hooks.create(this.#state.containedType() >>> 0, factoryFlags);
        if (sid < 5 || sid >= pool.capacity) throw new RangeError('contained 생성 SID 범위 오류');
        let raw = pool.allocated(sid);
        putWord(raw, 18, type);
        putWord(raw, 12, argument);
        if (raw[10] === (this.#state.bridgeType() >>> 0)) this.#hooks.notifySurface(sid);
        raw = pool.allocated(sid);
        raw[30] = quality & 0xff;
        putWord(raw, 20, listFlags);
        this.#hooks.attach(sid, parent, attachFlags);
        return sid;
    }
}

// 같은 풀의 실제 SquidFactory.create(현재 TYPE 생성 부모)를 create 경계로 연결한다. 표면 알림·부착은 호출자 장치를 유지한다. (2026-11-06 WB04-D5a)
export function makeContainedCreateHooks(pool, factory, hooks = {}) {
    if (factory?.pool !== pool) throw new TypeError('contained 생성/factory SID 풀이 다릅니다.');
    return { ...hooks, create: (type, flags) => factory.create(type, flags) };
}
