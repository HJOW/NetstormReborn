// 117/118 상태 주문(은신·마비)의 실제 대상 자격과 대상에 거는 효과(가상 c0)를 같은 물리 풀에 적용하는 모듈이다(cpppj `o/RawBombStatus.cpp/.h`, 10.78 전용).
//  - mayTarget(parent, target): 부모의 현재 가상 표가 502510(은신 자격 41eca0)이면 대상 genus 0x1570004 가 하나라도 있거나 대상 TYPE 이 제단/전초(Outpost)일 때 참,
//    502ff8(마비 자격 41fdb0)이면 대상 genus 0x170000 이 있을 때 참이다. 두 Wind 적용 표(502e68/502f30)의 같은 41fc50 은 자격 판정에 쓰지 않는다.
//  - apply(parent, target): 부모 표(502510/502e68/502f30 은신 41fc50, 502ff8 마비 41fdd0)로 효과를 정하고, 대상이 부모의 +12 WORD(운반체)와 같으면 아무것도 하지 않는다.
//    마비일 때 대상 TYPE 이 사제이고 부모→대상 동맹 표(행 부모 owner, 열 대상 owner)가 참이면 면제한다. 이어 free/dead 를 추가 검사하지 않고 대상 extra 에 은신 0x10/마비 0x20 비트를 직접 쓴다.
//    은신: refresh(대상) 뒤 대상 genus 0x1000004 이면 flare(대상, 1, 1, 현재 플레어 종류, 방향 2). 마비: genus 0x200000(사제)이면 unpop(0)+repop(0x40), 아니면 refresh, 현재 TYPE 이 울타리(136)면 fence, 끝에 flare(대상, 1, 3, 플레어, 방향 3).
// 현재 TYPE 전역(state: altar 164·outpost 126·priest 158·fence 136·flare 106)과 동맹 표는 외부 효과 뒤 다시 읽는다(사본으로 고정하지 않음). 큰 하위(refresh/unpop/repop/fence/flare)는 선택된 지점에서만 요구하고 없으면 TypeError 이다. DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E53: RawBombStatus 이식으로 추가)

// 슬롯 필드 위치와 가상 표 상수, 효과 비트, genus 마스크이다. (2026-11-08 WB04-E53)
const TYPE_OFFSET = 10;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const CARRIER_OFFSET = 12;
const TYPE_COUNT = 188;
const OWNER_COUNT = 9;
const ALLIANCE_COUNT = 81;
const HIDE_TABLES = Object.freeze([0x502510, 0x502e68, 0x502f30]);
const PARALYZE_TABLE = 0x502ff8;
const HIDE_QUALIFY_TABLE = 0x502510;
const HIDE_EXTRA = 0x10;
const PARALYZE_EXTRA = 0x20;
const HIDE_TARGET_GENUS = 0x1570004;
const PARALYZE_TARGET_GENUS = 0x170000;
const FLARE_HIDE_GENUS = 0x1000004;
const PRIEST_GENUS = 0x200000;
const REPOP_FLAGS = 0x40;

// 선택한 실제 효과의 누락을 빈 성공으로 숨기지 않는다. (2026-11-08 WB04-E53)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('상태 주문 실제 하위 미연결');
    return hook;
}

// 슬롯의 비정렬 폭만 리틀엔디언으로 읽는다(주변 필드로 폭을 넓히지 않는다). (2026-11-08 WB04-E53)
function read(raw, offset, width = 4) {
    let result = 0;
    // 낮은 바이트부터 지정 폭만 조합한다.
    for (let index = 0; index < width; index += 1) result += raw[offset + index] * 2 ** (8 * index);
    return result >>> 0;
}

// 상태 주문 실행기이다. 사용: new RawBombStatus(pool, types, state, { refresh, unpop, repop, fence, flare }). state = { altar, outpost, priest, fence, flare, alliances }. (2026-11-08 WB04-E53)
export class RawBombStatus {
    #pool;
    #types;
    #state;
    #hooks;

    // PATCH 풀/188TYPE 와 현재 전역의 수명을 빌린다. (2026-11-08 WB04-E53)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || typeof state !== 'object' || state === null) throw new TypeError('상태 주문 PATCH/188TYPE 필요');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 정확한 은신/마비 적용 표를 구분한다(은신 세 표는 같은 41fc50). 다른 표는 거부한다. (2026-11-08 WB04-E53)
    #kind(parent) {
        const table = read(this.#pool.slot(parent), 0);
        if (HIDE_TABLES.includes(table)) return 1;
        if (table === PARALYZE_TABLE) return 2;
        throw new TypeError('상태 주문의 현재 VT 불일치');
    }

    // 효과 뒤 바뀐 현재 TYPE 의 genus 를 읽는다. (2026-11-08 WB04-E53)
    #genus(sid) {
        const kind = this.#pool.slot(sid)[TYPE_OFFSET];
        if (kind >= this.#types.length) throw new RangeError('상태 주문의 현재 TYPE 범위');
        return this.#types[kind].flags2;
    }

    // 117 의 41eca0 과 118 의 41fdb0 을 정확한 현재 가상 표로 분배한다. (2026-11-08 WB04-E53)
    mayTarget(parent, target) {
        const table = read(this.#pool.slot(parent), 0);
        // Wind 의 같은 은신 적용 함수와 별개인 소환 자격을 혼용하지 않는다.
        if (table !== HIDE_QUALIFY_TABLE && table !== PARALYZE_TABLE) throw new TypeError('상태 주문 자격의 현재 VT 불일치');
        if (this.#kind(parent) === 2) return (this.#genus(target) & PARALYZE_TARGET_GENUS) !== 0;
        if (this.#genus(target) & HIDE_TARGET_GENUS) return true;
        const kind = this.#pool.slot(target)[TYPE_OFFSET];
        return kind === this.#state.altar || kind === this.#state.outpost;
    }

    // 41fc50/41fdd0: 운반체 제외 → 사제 면제 → 상태 비트 직접 쓰기 → 현재 가상 효과 순서이다. (2026-11-08 WB04-E53)
    apply(parent, target) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const state = this.#state;
        const effect = this.#kind(parent);
        if (target === read(pool.slot(parent), CARRIER_OFFSET, 2)) return;
        if (effect === 2 && pool.slot(target)[TYPE_OFFSET] === state.priest) {
            const owner = pool.slot(parent)[OWNER_OFFSET];
            const other = pool.slot(target)[OWNER_OFFSET];
            if (owner >= OWNER_COUNT || other >= OWNER_COUNT || state.alliances?.length !== ALLIANCE_COUNT) throw new RangeError('상태 주문의 현재 동맹 범위');
            if (state.alliances[owner * OWNER_COUNT + other]) return;
        }
        // free/dead 를 추가 검사하지 않는 원본 물리 쓰기이다.
        pool.physical(target)[EXTRA_OFFSET] |= effect === 1 ? HIDE_EXTRA : PARALYZE_EXTRA;
        if (effect === 1) {
            need(hooks.refresh)(target);
            if (this.#genus(target) & FLARE_HIDE_GENUS) need(hooks.flare)(target, 1, 1, state.flare, 2);
            return;
        }
        if (this.#genus(target) & PRIEST_GENUS) {
            need(hooks.unpop)(target, 0);
            need(hooks.repop)(target, REPOP_FLAGS);
        } else {
            need(hooks.refresh)(target);
        }
        if (pool.slot(target)[TYPE_OFFSET] === state.fence) need(hooks.fence)(target);
        need(hooks.flare)(target, 1, 3, state.flare, 3);
    }
}
