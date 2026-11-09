// 원본 10.78 다리(Bridge.cpp)의 삭제 전후 훅을 옮긴 모듈이다.
//  - preDestroy(004221b0): 일반(abstract/buried 아님) 다리면 좌표+0.99999(0x3f7fff58) 절삭 칸의 ±1 사각형을 일반 탐색기로 돌며
//    현재 링크 TYPE 이고 두 참조 중 하나가 dead 인 링크를 삭제(DestroyLink)한 뒤 공통 preDestroy 로 간다.
//  - postDestroy(00422300): 일반 다리면 파편 생성 요청 → 낙하 소리(현재 좌표) → 그 칸의 walker(flags2 0x10000) 낙하 → 공통 postDestroy 순서이다.
//  - delayedFallPayload(00421530/00421f90): 두 _ftol 하위 비트를 합친 signed DWORD 를 float 로 바꾼 낙하 사건(0x2692) 값이다.
// 삭제·파편·소리·낙하 효과는 호출자 장치(emit)이며, 각 효과 뒤 다음 후보의 현재 TYPE·참조를 다시 읽는다.
//  - preDestroyNative(bridge, flags, currentLinkType, hooks): 같은 몸체이되 연결 TYPE 을 값이 아니라 현재 전역 getter 로 받아 각 후보 비교 때마다 다시 읽는다(cpppj PreDestroyNative, 기준선 뒤 712e7224 의 Native 연결). BYTE 슬롯 TYPE 과 전체 DWORD 를 비교한다.
// 근거: cpppj `o/RawBridgeLifecycle.cpp/.h`(5879152b·225f2322·712e7224), 원본 관측 `cpppj/tests/fixtures/destroy-x86.tsv` Bridge 64행과 `bridgeeffects-x86.tsv`(10.78: Link 882·Pre 840·Post 600·Delay 242·Pack 242).
// 다리 가상 삭제 분배(가상 표 기록값으로 다리 pre/post 를 고르는 RawSurfaceLifecycle)와 CD 10.72 경로는 이 모듈 밖이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-05 WB04-D2: 공통 삭제기 대조와 함께 추가, 2026-11-10 WB04-E247: preDestroyNative 와 bridgeeffects 관측 대조 추가)

// 낙하 처리 사건 번호이다. (2026-11-05 WB04-D2)
export const BRIDGE_FALL_EVENT = 0x2692;

// 다리 수명 훅이 내는 효과 이름이다. notifyRemoval 은 실제로 Flyingshrapnel 파편 생성(00460600)이다. (2026-11-05 WB04-D2)
export const BRIDGE_LIFECYCLE_EFFECT = Object.freeze({
    destroyLink: 'destroyLink',
    notifyRemoval: 'notifyRemoval',
    fallSound: 'fallSound',
    fallWalker: 'fallWalker',
    basePreDestroy: 'basePreDestroy',
    basePostDestroy: 'basePostDestroy',
});

// 슬롯 필드 위치(10.78)이다: TYPE·상태·첫 참조(+12)·둘째 참조(+8)·x/y·extra. (2026-11-05 WB04-D2)
const TYPE = 10, STATE = 11, FIRST_REFERENCE = 12, SECOND_REFERENCE = 8, X = 14, Y = 18, EXTRA = 40;

// dead 비트·abstract|buried extra 비트·walker genus 이다. (2026-11-05 WB04-D2)
const DEAD = 2, ABSTRACT_OR_BURIED = 9, WALKER = 0x10000;

// preDestroy 범위의 거의 올림 보정값(원본 float 비트 0x3f7fff58)이다. 표면 조회의 0.9999 와 다르다. (2026-11-05 WB04-D2)
const AREA_COORDINATE_BIAS = (() => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, 0x3f7fff58, true);
    return view.getFloat32(0, true);
})();

// 슬롯 WORD 를 읽는다. (2026-11-05 WB04-D2)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 슬롯 float 를 읽는다. (2026-11-05 WB04-D2)
function coordinate(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
}

// x87 덧셈(float 로 다시 좁히지 않음) 뒤 0 방향 절삭이다. ±1 확장까지 int 범위여야 한다. (2026-11-05 WB04-D2)
function cell(value, bias = 0) {
    const wide = value + bias;
    if (!Number.isFinite(wide) || wide <= -0x80000000 || wide >= 0x7fffffff) throw new RangeError('Bridge lifecycle coordinate');
    return Math.trunc(wide);
}

// 원본 CRT _ftol 의 64비트 절삭값 하위 DWORD 이다. 비유한/64비트 밖은 0 이다. (2026-11-05 WB04-D2)
function ftolLow(value) {
    const wide = Math.fround(value);
    if (!(wide > -9223372036854775808 && wide < 9223372036854775808)) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(wide))));
}

// 효과 장치가 있고(일반 다리면) 탐색기 begin/next 가 있는지 먼저 확인한다. (2026-11-05 WB04-D2)
function validateHooks(hooks, ordinary) {
    if (typeof hooks?.emit !== 'function' || (ordinary && (typeof hooks.begin !== 'function' || typeof hooks.next !== 'function'))) {
        throw new TypeError('Bridge lifecycle hooks');
    }
}

// 실제 일반 사각형 탐색기(RawSquidFinder)의 begin/next 를 다리 훅에 공급한다. 반환 훅은 finder 를 빌리므로 다른 탐색과 동시에 쓰면 안 된다.
// 사용: const hooks = makeBridgeLifecycleHooks(finder, (event) => { … }). (2026-11-05 WB04-D2)
export function makeBridgeLifecycleHooks(finder, emit) {
    return {
        // 정수 탐색 범위를 원본 일반 사각형 begin 에 그대로 넘긴다.
        begin: (area) => finder.begin(area),
        // 이전 후보 삭제 뒤의 다음 체인/버킷을 읽는다.
        next: () => finder.next(),
        emit,
    };
}

// 다리 삭제 전후 훅이다. pool 은 SidPool, types 는 188칸 TYPE 레코드 표({ flags2 } 를 읽음)이다.
// 사용: const bridge = new RawBridgeLifecycle(pool, types); bridge.preDestroy(sid, flags, linkType, hooks). (2026-11-05 WB04-D2)
export class RawBridgeLifecycle {
    // 같은 실제 풀이다.
    #pool;
    // TYPE 레코드 표이다(복사하지 않음).
    #types;

    // 10.78 풀과 188칸 표만 받는다.
    constructor(pool, types) {
        if (pool?.edition !== 'patch1078' || types?.length !== 188) throw new TypeError('다리 수명 훅의 10.78 풀/TYPE 표 오류');
        this.#pool = pool;
        this.#types = types;
    }

    // 자기 번호 0/풀 밖은 거부하고 슬롯 읽기 사본을 돌려준다.
    #object(sid) {
        if (!Number.isInteger(sid) || sid === 0 || sid >= this.#pool.capacity) throw new RangeError('Bridge lifecycle SID');
        return this.#pool.slot(sid);
    }

    // extra 의 abstract·buried 비트가 없으면 일반 객체이다.
    #ordinary(sid) {
        return (this.#object(sid)[EXTRA] & ABSTRACT_OR_BURIED) === 0;
    }

    // 00422290: 첫 참조(+12)만 extra 를 검사하고 두 참조(+12/+8)의 dead 를 OR 한다. 참조가 0/풀 밖이면 조용히 false 이다(free/void 는 거르지 않는다).
    linkNeedsDestroy(link) {
        const raw = this.#object(link);
        const first = word(raw, FIRST_REFERENCE);
        const second = word(raw, SECOND_REFERENCE);
        if (first === 0 || first >= this.#pool.capacity || second === 0 || second >= this.#pool.capacity) return false;
        return this.#ordinary(first) && ((this.#object(first)[STATE] | this.#object(second)[STATE]) & DEAD) !== 0;
    }

    // 004221b0: 주변 링크를 원본 탐색 순서로 처리한 뒤 공통 preDestroy 로 간다. 매 후보에서 현재 TYPE·참조를 다시 읽는다. linkType 은 값(독립 typed 경로)이다.
    preDestroy(bridge, flags, linkType, hooks) {
        this.#runPreDestroy(bridge, flags, () => linkType, hooks);
    }

    // 실제 현재 연결 TYPE 전역(getter)을 각 후보 비교에서 다시 읽는 Native 경로이다. 첫 연결 삭제가 전역을 바꾸면 다음 후보는 새 DWORD 로 비교한다. (2026-11-10 WB04-E247)
    preDestroyNative(bridge, flags, currentLinkType, hooks) {
        if (typeof currentLinkType !== 'function') throw new TypeError('다리 preDestroy 의 현재 연결 TYPE getter 미연결');
        this.#runPreDestroy(bridge, flags, currentLinkType, hooks);
    }

    // typed 값 사본 또는 Native 현재 getter 로 같은 실제 순회 몸체를 실행한다. (2026-11-10 WB04-E247)
    #runPreDestroy(bridge, flags, readLinkType, hooks) {
        const ordinary = this.#ordinary(bridge);
        validateHooks(hooks, ordinary);
        if (ordinary) {
            const raw = this.#object(bridge);
            const x = cell(coordinate(raw, X), AREA_COORDINATE_BIAS);
            const y = cell(coordinate(raw, Y), AREA_COORDINATE_BIAS);
            // 원본 탐색 순서와 0 종료 표식을 그대로 쓴다.
            for (let current = hooks.begin({ left: x - 1, top: y - 1, right: x + 1, bottom: y + 1 }); current !== 0; current = hooks.next()) {
                if (this.#object(current)[TYPE] === readLinkType() && this.linkNeedsDestroy(current)) {
                    hooks.emit({ effect: BRIDGE_LIFECYCLE_EFFECT.destroyLink, sid: current, flags: 0 });
                }
            }
        }
        hooks.emit({ effect: BRIDGE_LIFECYCLE_EFFECT.basePreDestroy, sid: bridge, flags });
    }

    // 00422300: 파편 요청 → 소리(그 시점 좌표) → 좌표를 다시 읽은 칸의 walker 낙하 → 공통 postDestroy 이다.
    postDestroy(bridge, flags, hooks) {
        const ordinary = this.#ordinary(bridge);
        validateHooks(hooks, ordinary);
        if (ordinary) {
            hooks.emit({ effect: BRIDGE_LIFECYCLE_EFFECT.notifyRemoval, sid: bridge, flags: 0 });
            let raw = this.#object(bridge);
            hooks.emit({ effect: BRIDGE_LIFECYCLE_EFFECT.fallSound, sid: bridge, flags: 0, x: coordinate(raw, X), y: coordinate(raw, Y) });
            raw = this.#object(bridge);
            const x = cell(coordinate(raw, X));
            const y = cell(coordinate(raw, Y));
            // walker 여부도 낙하 훅이 바꾼 뒤의 현재 TYPE 로 다시 읽는다.
            for (let current = hooks.begin({ left: x, top: y, right: x, bottom: y }); current !== 0; current = hooks.next()) {
                const type = this.#object(current)[TYPE];
                if (type >= this.#types.length) throw new RangeError('Bridge lifecycle type');
                if (((this.#types[type].flags2 >>> 0) & WALKER) !== 0) hooks.emit({ effect: BRIDGE_LIFECYCLE_EFFECT.fallWalker, sid: current, flags: 0 });
            }
        }
        hooks.emit({ effect: BRIDGE_LIFECYCLE_EFFECT.basePostDestroy, sid: bridge, flags });
    }

    // 낙하 사건 값: (ftol(x)&0xff)|(ftol(y)<<8) 의 signed DWORD 수치를 float 로 바꾼다(비트가 아니라 수치).
    static delayedFallPayload(x, y) {
        const packed = ((ftolLow(x) & 0xff) | (ftolLow(y) << 8)) | 0;
        return Math.fround(packed);
    }
}
