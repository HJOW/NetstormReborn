// 아웃포스트의 배치 추가 목록·거리 후보 목록 등록(원문 004557c0)과 삭제 준비 접두(00455840)를 같은 raw 풀과 별도 두 전역 목록에 적용하는 모듈이다(cpppj `o/RawOutpostLifecycle.cpp/.h`, 10.78 전용).
// 스커미시 12차의 Outpost 에 쓰인다(멀티플레이 전용 건물의 규칙 부품).
// 사용:
//   const lifecycle = new RawOutpostLifecycle(pool, additional, nearest, hooks);
//     pool = SidPool(10.78), additional/nearest = 서로 다른 SquidPostPopList(추가 목록·거리 후보 목록; 같은 목록을 두 번 넘기면 거부),
//     hooks = { regionChanged(x, y) — 00455380 지역 소유 투표/갱신(세 번째 인자 0), validatePostPop(sid, flags)/postPop(sid, flags) — 작업장 postPop 004538f0,
//               validatePreDestroy(sid, flags)/preDestroy(sid, flags) — Damageable preDestroy 선검사/본문 } 이며 모두 필수이다.
//   lifecycle.postPop(sid, flags) → 첫 Pop(flags 비트 0)이고 extra 가 abstract/buried(9)가 아닌 일반 객체만 두 목록에 중복 없이 추가하고 지역 통지(현재 x,y)를 한 뒤 작업장 postPop 을 같은 flags 로 부른다.
//   lifecycle.preDestroy(sid, flags) → 일반 객체는 두 목록에서 모든 중복을 제거하고 지역을 통지한 뒤 Damageable preDestroy 를 같은 flags 로 부른다(abstract/buried 는 목록/지역만 생략).
//   RawOutpostLifecycle.preDestroyNative(pool, additional, nearest, sid, flags, hooks) → 실제 455840: 자산/목록/부모 선검사 없이 extra → 두 현재 목록(signed 현재 count) 압축 → 현재 Y→X 좌표 → 지역 통지 → Damageable 부모 순서이다.
//   makeOutpostDamageablePreDestroyHooks(pool, damageable, hooks) → 부모 삭제 경계를 같은 풀의 실제 RawDamageablePreDestroy 의 선검사/본문으로 교체한 훅 묶음이다(풀이 다르면 거부).
// 원본 관측: cpppj `outpostlifecycle-x86.tsv` 의 10.78(originals) 768행(P/D 접두의 두 목록 전체(꼬리 포함)·개수·지역 통지/base 순서·raw 쓰기)을 tests/raw-outpost-lifecycle.test.mjs 가 대조한다.
// CD 10.72/1037 판본 행은 웹 범위 밖이다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E556: 아웃포스트 목록 수명 RawOutpostLifecycle 이식)

// 슬롯의 TYPE(+10)·상태(+11)·좌표 float(+14/+18)·extra(+40) 위치이다. (2026-11-15 WB04-E556)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const EXTRA_OFFSET = 40;
// 상태/extra 의 free+contained(9) 또는 abstract+buried(9) 마스크, 자산 TYPE 의 첫 번호와 10.78 TYPE 수이다. (2026-11-15 WB04-E556)
const BLOCKED = 9;
const FIRST_ASSET_TYPE = 70;
const TYPE_COUNT = 188;

// 비정렬 raw 좌표를 읽어 NaN/부호 있는 0 의 비트도 그대로 효과에 넘긴다. (2026-11-15 WB04-E556)
function coordinate(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
}

// 가득 찼으면 조회/추가를 생략하고, 남은 용량에서는 기존 항목이 있을 때 추가하지 않는다. (2026-11-15 WB04-E556)
function add(list, sid) {
    if (list.readCount() >= list.capacity()) return;
    if (list.items().includes(sid)) return;
    list.append(sid);
}

// 40ea00: signed 현재 count 와 매 반복 같은 현재 포인터를 쓰고 음수 count/비활성 꼬리를 보존한다(count 갱신 외에는 용량을 선조회하거나 꼬리를 지우지 않음). (2026-11-15 WB04-E556)
function removeNative(list, sid) {
    let removed = 0;
    let index = 0;
    // count<=0 이면 배열 포인터도 읽지 않는다.
    while ((index | 0) < (list.readCount() | 0)) {
        const storage = list.storage();
        if (index >= storage.length) throw new RangeError('Outpost 삭제 목록의 현재DWORD 물리 부족');
        const value = storage[index];
        if (value === sid) removed += 1;
        else {
            const destination = index - removed;
            if (destination >= storage.length) throw new RangeError('Outpost 삭제 압축의 현재DWORD 물리 부족');
            storage[destination] = value;
        }
        index += 1;
    }
    list.setCount(list.readCount() - removed);
}

// 선택된 callback 만 요구하며 앞선 목록/지역 효과를 되돌리지 않고 명시 오류로 알린다. (2026-11-15 WB04-E556)
function needNative(hook) {
    if (typeof hook !== 'function') throw new Error('Outpost 삭제의 선택된 실제 장치 미연결');
    return hook;
}

// 아웃포스트의 목록 수명 몸체이다. 두 목록은 factories 와 다른 전역이며 호출자/훅 대상은 이 객체보다 오래 살아야 한다. (2026-11-15 WB04-E556)
export class RawOutpostLifecycle {
    // 빠진 지역/부모 경계와 별도 전역 목록의 잘못된 연결을 생성 시 거부한다.
    constructor(pool, additional, nearest, hooks) {
        if (additional === nearest || typeof hooks?.regionChanged !== 'function' || typeof hooks.validatePostPop !== 'function' || typeof hooks.postPop !== 'function'
            || typeof hooks.validatePreDestroy !== 'function' || typeof hooks.preDestroy !== 'function') throw new TypeError('outpost 별도 목록/필수 효과 연결 오류');
        this.pool = pool;
        this.additional = additional;
        this.nearest = nearest;
        this.hooks = hooks;
    }

    // free/contained 또는 자산이 아닌 슬롯을 효과 전에 거부한다(파생 가상 메서드의 호출자 계약).
    object(sid) {
        const raw = this.pool.slot(sid);
        if (sid < 5 || (raw[STATE_OFFSET] & BLOCKED) || raw[TYPE_OFFSET] < FIRST_ASSET_TYPE || raw[TYPE_OFFSET] >= TYPE_COUNT) throw new RangeError('outpost raw 자산 상태/타입 오류');
        return raw;
    }

    // 004557c0: 목록 쓰기와 지역 효과가 끝난 뒤 작업장 base 를 같은 flags 로 반드시 호출한다.
    postPop(sid, flags) {
        const raw = this.object(sid);
        const ordinary = !(raw[EXTRA_OFFSET] & BLOCKED);
        this.hooks.validatePostPop(sid, flags);
        if ((flags & 1) && ordinary) {
            // 두 목록을 먼저 검사하여 두 번째 손상이 첫 번째만 갱신하는 상황을 막는다.
            this.additional.items();
            this.nearest.items();
            add(this.additional, sid);
            add(this.nearest, sid);
            this.hooks.regionChanged(coordinate(this.pool.slot(sid), X_OFFSET), coordinate(this.pool.slot(sid), Y_OFFSET));
        }
        this.hooks.postPop(sid, flags);
    }

    // 00455840: abstract/buried 는 목록/지역만 생략하며 Damageable base 호출은 생략하지 않는다.
    preDestroy(sid, flags) {
        const raw = this.object(sid);
        const ordinary = !(raw[EXTRA_OFFSET] & BLOCKED);
        this.hooks.validatePreDestroy(sid, flags);
        if (ordinary) {
            this.additional.items();
            this.nearest.items();
            this.additional.removeAll(sid);
            this.nearest.removeAll(sid);
            this.hooks.regionChanged(coordinate(this.pool.slot(sid), X_OFFSET), coordinate(this.pool.slot(sid), Y_OFFSET));
        }
        this.hooks.preDestroy(sid, flags);
    }

    // 실제 455840: raw 자산·두 번째 목록·부모를 앞에서 검사하지 않고 선택된 현재 물리에 순서대로 도달한다. hooks = { regionChanged(x, y), damageablePre(sid, flags) }.
    static preDestroyNative(pool, additional, nearest, sid, flags, hooks) {
        if (!(pool.slot(sid)[EXTRA_OFFSET] & BLOCKED)) {
            // 첫 목록 완료 뒤에만 둘째 목록을 읽는다.
            removeNative(additional, sid);
            removeNative(nearest, sid);
            // 두 압축 뒤 PE 가 읽는 Y→X 의 현재 좌표 비트이다.
            const raw = pool.slot(sid);
            const y = coordinate(raw, Y_OFFSET);
            const x = coordinate(raw, X_OFFSET);
            needNative(hooks.regionChanged)(x, y);
        }
        // extra 분기와 무관하게 지역 효과 뒤 같은 원래 flags 의 부모를 호출한다.
        needNative(hooks.damageablePre)(sid, flags);
    }
}

// outpost 의 부모 삭제 경계를 같은 풀의 실제 Damageable 접두/공통 호출로 교체한 훅 묶음을 만든다(공간이나 원본 주소를 추측하지 않음). (2026-11-15 WB04-E556)
export function makeOutpostDamageablePreDestroyHooks(pool, damageable, hooks) {
    if (pool !== damageable.pool) throw new TypeError('outpost/Damageable SID 풀이 다릅니다');
    return {
        ...hooks,
        validatePreDestroy: (sid, flags) => damageable.validate(sid, flags),
        preDestroy: (sid, flags) => damageable.preDestroy(sid, flags)
    };
}
