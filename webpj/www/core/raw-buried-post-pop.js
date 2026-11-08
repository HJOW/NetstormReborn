// 10.78 buried 전용 postPop(00462170)을 같은 현재 게임 전역과 공통 장부에 연결한 모듈이다(cpppj `o/RawBuriedPostPop.cpp/.h`, 10.78 전용).
// 순서: 최초 flag(비트 1)이고 현재 boss 이며 현재 로딩 깊이가 0 이면 보상 타입 선택(00461f00(5))→현재 QA WORD(+12) 하위 16비트 쓰기(free/contained 와 무관한 물리 쓰기)→현재 TYPE 가 현재 Bridge 번호(전체 DWORD 비교)일 때만 표면 통지(004214a0).
//   최초 flag 에서는 두 번 예약(각 new(0x28)의 독립 성공 여부)해 성공한 것만 사건 0/0초, 사건 1/0.01초(float 0x3c23d70a)를 등록한다. 마지막으로 항상 현재 QA WORD 를 읽고(물리 SID 범위 확인) 공통 postPop(004b0d30, 깊이를 한 번 감소)을 호출한다.
// 선택하지 않은 하위 장치는 요구하지 않고, 도달한 장치가 없으면 앞선 원문 쓰기를 보존한 채 오류를 낸다. 현재 boss/로딩 깊이/Bridge 번호는 매 소비 위치에서 getter 로 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E60: buried 후처리 이식으로 추가)

// 10.78 buried 생성자(004625f0)가 기록하는 가상 표, 독립 구성의 초기 Bridge 번호, 0.01초 지연의 float32 비트이다. (2026-11-08 WB04-E60)
export const BURIED_VTABLE = 0x0050a748;
export const BURIED_INITIAL_BRIDGE_TYPE = 82;
const REGULAR_DELAY_BITS = 0x3c23d70a;
const TYPE_OFFSET = 10;
const QA_OFFSET = 12;

// 선택된 실제 하위가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E60)
function need(device) {
    if (typeof device !== 'function') throw new Error('buried PostPop의 실제 하위 소비자 미연결');
    return device;
}

// 비트 패턴을 단정도 float 로 해석한다. (2026-11-08 WB04-E60)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, boss()/loading()/bridgeType() 은 현재 값을 돌려주는 getter(boss 는 DAT00540bc4 권한, loading 은 005c89b8 중첩 깊이, bridgeType 은 005411a0 DWORD),
//   hooks = { chooseType(등급) → 타입, reserveRegular() → bool, addRegular(sid, 사건, 지연), validateBase(sid, flags, pop), base(sid, flags), notifySurface(sid) }. (2026-11-08 WB04-E60)
export class RawBuriedPostPop {
    // 단일 원본 객체 풀과 현재 전역 getter, 외부 호출 경계이다.
    #pool;
    #boss;
    #loading;
    #bridgeType;
    #hooks;

    // 10.78 만 복원했다(CD 00416ac0 은 별도 원문 대조 전 지원하지 않는다). (2026-11-08 WB04-E60)
    constructor(pool, boss, loading, hooks, bridgeType = () => BURIED_INITIAL_BRIDGE_TYPE) {
        if (pool?.edition !== 'patch1078') throw new TypeError('buried PostPop은 PATCH10.78만 복원했습니다');
        if (typeof boss !== 'function' || typeof loading !== 'function' || typeof bridgeType !== 'function') throw new TypeError('buried PostPop의 boss/로딩/Bridge 번호는 getter 여야 합니다.');
        this.#pool = pool;
        this.#boss = boss;
        this.#loading = loading;
        this.#bridgeType = bridgeType;
        this.#hooks = hooks ?? {};
    }

    // 파생 등록 시 동일 월드인지 확인하는 풀이다. (2026-11-08 WB04-E60)
    get pool() {
        return this.#pool;
    }

    // 실제 buried 생성자의 가상 표만 허용한다(가상 표 DWORD 를 호스트 포인터로 실행하지 않는다). (2026-11-08 WB04-E60)
    handles(sid) {
        const raw = this.#pool.slot(sid);
        return ((raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0) === BURIED_VTABLE;
    }

    // 같은 공통 공간/장부 경계를 사전 검사한다(명시 호출이며 실제 postPop 본문 앞에 끼우지 않는다). pop 은 실제 Pop 의 projected 좌표이다. (2026-11-08 WB04-E60)
    validate(sid, flags, pop = null) {
        if (!this.handles(sid)) throw new Error('buried PostPop 실제 가상 표 불일치');
        if (this.#hooks.validateBase) this.#hooks.validateBase(sid, flags, pop);
    }

    // boss 가 꺼져도 최초 예약은 수행하며 로딩 조건은 보상 재추첨만 억제한다. (2026-11-08 WB04-E60)
    postPop(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if ((flags & 1) && this.#boss() && this.#loading() === 0) {
            const type = need(hooks.chooseType)(5);
            // 원본 setQA 는 현재 free/contained 와 무관하게 낮은 WORD 만 저장한다.
            const bytes = pool.physical(sid);
            bytes[QA_OFFSET] = type & 0xff;
            bytes[QA_OFFSET + 1] = (type >>> 8) & 0xff;
            if (pool.slot(sid)[TYPE_OFFSET] === this.#bridgeType()) need(hooks.notifySurface)(sid);
        }
        if (flags & 1) {
            if (need(hooks.reserveRegular)()) need(hooks.addRegular)(sid, 0, 0);
            if (need(hooks.reserveRegular)()) need(hooks.addRegular)(sid, 1, floatOf(REGULAR_DELAY_BITS));
        }
        // 항상 현재 QA WORD 를 읽는다(물리 SID 범위를 확인). 뒤의 0049a840 은 사용하지 않는 주소만 계산하므로 TYPE 표를 역참조하거나 QA 범위를 검사하지 않는다.
        pool.slot(sid);
        need(hooks.base)(sid, flags);
    }
}
