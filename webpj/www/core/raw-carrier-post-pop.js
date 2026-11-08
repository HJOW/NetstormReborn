// Carrier→Damageable→Squid 의 비가상 postPop 호출 순서를 옮긴 모듈이다(cpppj `o/RawCarrierPostPop.cpp/.h`, 10.78 전용).
// 순서: Carrier 본체(00427720)는 최초 flag(비트 1)가 없고 현재 boss 권한이 없을 때만 가상 +0xcc(carrier 확인)를 부르고, 이후 항상 Damageable 본체(0044bf80)를 부른다.
//   Damageable 은 최초 flag 에서 현재 TYPE 의 genus 조회(반환값은 버리지만 접근 자체는 먼저 한다)를 하고, 로딩 깊이가 있고 현재 TYPE 이 간헐천이 아니고 extra 의 abstract/buried 비트(9)가 없고 현재 TYPE flags1 에
//   지면 소유자 갱신 비트(0x400)가 있으면 현재 좌표·소유자(+34)로 지면/받침 소유자 갱신(0044bd20)을 부른 뒤 항상 공통 postPop 을 호출한다.
// 가상 carrier 확인 뒤에도 현재 TYPE/extra/좌표/소유자를 다시 읽고, 10.78 에서는 CD 의 전체 사전 검사를 호출하지 않는다. 장치는 도달할 때만 요구한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E60: Carrier 후처리 이식으로 추가)

// 슬롯의 TYPE/좌표/소유자/extra 위치, 지면 소유자 갱신을 허용하는 TYPE flags1 비트, abstract+buried extra 마스크, genus 조회 마스크이다. (2026-11-08 WB04-E60)
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const CLAIM_GROUND = 0x400;
const ABSTRACT_BURIED = 9;
const GENUS_MASK = 0x130000;
const TYPE_COUNT = 188;

// 선택된 실제 장치가 없으면 그 위치에서 거부한다(자기 연결 변경 중에도 현재 호출 대상을 보존). (2026-11-08 WB04-E60)
function need(device) {
    if (typeof device !== 'function') throw new Error('Carrier후처리의선택된실제장치미연결');
    return device;
}

// 비정렬 좌표의 IEEE float 를 네 바이트에서 읽는다. (2026-11-08 WB04-E60)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 캐리어 후처리 중 조회하는 상태이다: boss(00540bc4)·loadingDepth(005c89b8)·geyserType(00541240)의 독립 값과 실제 게임 전역을 빌리는 getter(shared*)이다. 공유 getter 가 있으면 소비 위치마다 현재 값을 읽는다. (2026-11-08 WB04-E60)
export class CarrierPostPopState {
    // 독립 입력 값과 실제 전역 getter(없으면 null)이다.
    boss = true;
    loadingDepth = 0;
    geyserType = 122;
    sharedBoss = null;
    sharedLoadingDepth = null;
    sharedGeyserType = null;

    // 최초 flag 관문 뒤 현재 권한이다. (2026-11-08 WB04-E60)
    isBoss() {
        return this.sharedBoss ? Boolean(this.sharedBoss()) : this.boss;
    }

    // Carrier 확인 뒤 현재 로딩 관문에서 읽는 깊이이다. (2026-11-08 WB04-E60)
    readLoadingDepth() {
        return this.sharedLoadingDepth ? this.sharedLoadingDepth() >>> 0 : this.loadingDepth >>> 0;
    }

    // 로딩 중 지면 소유자 갱신에서 제외하는 간헐천 TYPE 의 현재 번호이다. (2026-11-08 WB04-E60)
    readGeyserType() {
        return this.sharedGeyserType ? this.sharedGeyserType() >>> 0 : this.geyserType >>> 0;
    }
}

// Carrier 확인→Damageable 지면 처리→공통 장부를 직접 호출한다. 파생 가상 지원은 별도다. pool 은 10.78 SidPool, types 는 188칸 TYPE 표(복사), state 는 CarrierPostPopState,
//   hooks = { carrierCheck(sid), claimGround(x, y, owner), validateBase(sid, flags), base(sid, flags) } (10.78 에서 validateBase 는 호출하지 않는다). (2026-11-08 WB04-E60)
export class RawCarrierPostPop {
    // 객체 슬롯 풀, 복사한 타입 표, 실제 World 에서 대여한 현재 TYPE 표(없으면 null), 상태와 외부 효과 묶음이다.
    #pool;
    #types;
    #currentTypes = null;
    #state;
    #hooks;

    // 판본/타입 표 크기를 확인하며 실제 10.78 장치는 도달할 때만 요구한다. (2026-11-08 WB04-E60)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('carrier postPop 타입 판본/크기 오류');
        this.#pool = pool;
        this.#types = types.map((record) => ({ ...record }));
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 사제 후처리 조합에서 다른 월드의 Carrier 연결을 거부하기 위한 풀이다. (2026-11-08 WB04-E60)
    get pool() {
        return this.#pool;
    }

    // 실제 10.78 World 의 현재 TYPE 표(188칸, 복사 없이 대여)를 연결한다. null/빈 배열이면 소유 사본으로 돌아간다. 잘못된 대여는 기존 연결을 바꾸기 전에 거부한다. (2026-11-08 WB04-E60)
    bindCurrentTypes(types) {
        if (types && types.length > 0 && types.length !== TYPE_COUNT) throw new RangeError('Carrier후처리현재TYPE대여의판본/길이오류');
        this.#currentTypes = types && types.length > 0 ? types : null;
    }

    // 최초 genus/로딩 중 flags1 을 실제 소비 지점에서 조회한다. 낮은 프로세스 TYPE 도 원문처럼 물리 범위 안이면 허용한다. (2026-11-08 WB04-E60)
    #currentType(number) {
        const types = this.#currentTypes ?? this.#types;
        if (number >= types.length) throw new RangeError('Carrier후처리현재TYPE물리범위오류');
        return types[number];
    }

    // 00427720: flags 비트 1 이 없고 boss 가 거짓이면 +0xcc, 이후 항상 Damageable 을 부른다(extra 차단과 무관). (2026-11-08 WB04-E60)
    postPop(sid, flags) {
        if (!(flags & 1) && !this.#state.isBoss()) need(this.#hooks.carrierCheck)(sid);
        this.#runDamageable(sid, flags);
    }

    // 0044bf80: 직접 호출은 carrier 가상 확인을 거치지 않는다. (2026-11-08 WB04-E60)
    damageablePostPop(sid, flags) {
        this.#runDamageable(sid, flags);
    }

    // carrier 가상 확인 이후의 현재 raw 값으로 로딩 조건을 평가하고 항상 공통 postPop 으로 끝난다. (2026-11-08 WB04-E60)
    #runDamageable(sid, flags) {
        const raw = this.#pool.slot(sid);
        // 004ad080 의 반환은 버려도 현재 TYPE 접근 자체는 먼저 수행한다.
        if (flags & 1) void (this.#currentType(raw[TYPE_OFFSET]).flags2 & GENUS_MASK);
        if (this.#state.readLoadingDepth() && raw[TYPE_OFFSET] !== this.#state.readGeyserType() && !(raw[EXTRA_OFFSET] & ABSTRACT_BURIED)
            && (this.#currentType(raw[TYPE_OFFSET]).flags1 & CLAIM_GROUND)) {
            need(this.#hooks.claimGround)(floatAt(raw, X_OFFSET), floatAt(raw, Y_OFFSET), raw[OWNER_OFFSET]);
        }
        need(this.#hooks.base)(sid, flags);
    }
}

// 같은 풀의 실제 공통 장부(SquidPostPop)에 연결하는 훅 묶음을 만든다. +0xcc 와 지면 효과는 해당 파생/월드가 공급한다. 10.78 의 base 는 비가상 공통 몸체(postPopBase)이다. (2026-11-08 WB04-E60)
export function makeCarrierPostPopHooks(pool, base, carrierCheck, claimGround) {
    if (base.pool !== pool) throw new TypeError('carrier/공통 postPop의 SID 풀이 다릅니다');
    return {
        carrierCheck,
        claimGround,
        validateBase: (sid, flags) => base.validateBase(sid, flags),
        base: (sid, flags) => base.postPopBase(sid, flags)
    };
}
