// 원문 객체 선택 비트 쓰기(41c0b0·44b2b0)·사정거리 종류 판정(474240)·선택 내용물 표시 Regular 생성 입구(426080)를 옮긴 모듈이다(cpppj `o/RawSquidSelection.cpp/.h`, 10.78 전용).
// 선택 비트: 현재 가상 표의 +0x7c 진입이 Damageable 파생(44b2b0)이면 표시 갱신(+0x88)을 먼저 호출하고, 그 효과 뒤의 현재 extra(+40)에서 비트2 하나만 value 의 하위 비트로 바꾼다.
//   기본 진입(41c0b0)은 표시 갱신을 요구하지 않는다. 표에 없거나 확인되지 않은 파생은 쓰기 전에 거부한다. free/dead 필터는 두지 않는 물리 슬롯 쓰기다.
// 내용물 표시: 로딩 깊이가 0 이 아니면 아무것도 하지 않고, extra 의 비트(0·3)가 있으면 생성하지 않는다. TYPE 이 70 미만이면 진단(!s.isForm()) 뒤 현재 TYPE 을 다시 검사한다.
//   그래도 생성할 때는 491 사건과 0.2 간격(float32 0x3e4ccccd)의 Regular 를 요청한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E58: 객체 선택 이식으로 추가)

import { SQUID_SELECTION_VTABLES } from './squid-selection-vtables.js';

// 기본 선택 통지·Damageable 파생 선택 통지 진입점, extra 의 선택 비트(비트2)와 위치, TYPE 위치, Form 최소 TYPE, 내용물 표시 사건 번호이다. (2026-11-08 WB04-E58)
const BASIC_NOTICE = 0x41c0b0;
const DAMAGEABLE_NOTICE = 0x44b2b0;
const EXTRA_OFFSET = 40;
const SELECTED_BIT = 4;
const TYPE_OFFSET = 10;
const FIRST_NON_FORM_TYPE = 70;
const CONTENT_PREVIEW_EVENT = 0x491;
const CONTENT_PREVIEW_INTERVAL_BITS = 0x3e4ccccd;

// 가상 표 → +0x7c 진입점 조회 표이다. (2026-11-08 WB04-E58)
const NOTICE_BY_TABLE = new Map(SQUID_SELECTION_VTABLES.map(([table, notice]) => [table, notice]));

// 0.2 간격의 float32 값이다(비트 0x3e4ccccd). (2026-11-08 WB04-E58)
export const SELECTION_PREVIEW_INTERVAL = (() => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, CONTENT_PREVIEW_INTERVAL_BITS, true);
    return view.getFloat32(0, true);
})();

// 도달한 실제 효과가 없으면 선행 원본 쓰기를 가장하지 않고 거부한다. (2026-11-08 WB04-E58)
function need(device) {
    if (typeof device !== 'function') throw new Error('객체 선택 실제 하위 미연결');
    return device;
}

// 생성자 VT 에 기록된 실제 +0x7c 진입 주소를 돌려준다. 모르는 표는 0 이며 기본 몸체로 대신하지 않는다. (2026-11-08 WB04-E58)
export function squidSelectionEntry(table) {
    return NOTICE_BY_TABLE.get(table >>> 0) ?? 0;
}

// 474240: 사정거리 표시를 만드는 group 0/1/6/3 또는 flags2 0x100(폭탄) 타입의 현재 조건이다(이동 가능 판정이 아님). group 은 부호 있는 DWORD 이다. (2026-11-08 WB04-E58)
export function selectionUsesRange(type) {
    const group = type.group | 0;
    return group === 0 || group === 1 || (type.flags2 & 0x100) !== 0 || group === 6 || group === 3;
}

// 41c0b0/44b2b0 의 현재 raw 쓰기를 같은 풀에서 처리한다. pool 은 10.78 SidPool(physical(sid) 로 물리 슬롯을 쓴다), refresh 는 실제 가상 +0x88 이며 Damageable 파생 +0x7c 에서만 필수이다. (2026-11-08 WB04-E58)
export class RawSquidSelection {
    // 쓰기 대상 풀과 표시 갱신 경계이다.
    #pool;
    #refresh;

    // 10.78 풀만 받는다. (2026-11-08 WB04-E58)
    constructor(pool, refresh) {
        if (pool?.edition !== 'patch1078' || typeof pool.physical !== 'function') throw new TypeError('객체 선택10.78 풀 오류');
        this.#pool = pool;
        this.#refresh = refresh;
    }

    // value 하위 bit0 을 extra 비트2 로 바꾼다. Damageable 파생은 표시 갱신이 extra/VT 를 바꿔도 표시 뒤의 현재 extra 를 읽어 한 비트만 쓴다. (2026-11-08 WB04-E58)
    mark(sid, value) {
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('객체 선택 SID 범위 오류');
        const raw = this.#pool.physical(sid);
        const table = (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
        const entry = squidSelectionEntry(table);
        if (!entry) throw new Error('객체 선택 가상+7c VT 미연결');
        if (entry === DAMAGEABLE_NOTICE) need(this.#refresh)(sid);
        else if (entry !== BASIC_NOTICE) throw new Error('객체 선택 파생 몸체 미연결');
        const extra = this.#pool.physical(sid);
        extra[EXTRA_OFFSET] = ((extra[EXTRA_OFFSET] & ~SELECTED_BIT) | ((value & 1) << 2)) & 0xff;
    }
}

// 426080: 로딩·extra 조건과 TYPE 재검사 뒤 491 내용물 표시의 0.2 간격 Regular 를 요청한다. 진단 콜백이 TYPE 을 고치면 현재 값으로 계속하며 여전히 Form 이면 생성하지 않는다.
//   pool 은 10.78 SidPool, regular(sid, 사건, 간격 float) 와 assertion()(원문 !s.isForm() 진단 요청, 인자 없음)은 선택된 실제 장치이다. (2026-11-08 WB04-E58)
export function startSelectionContentPreview(pool, parent, loadingDepth, regular, assertion) {
    if (pool?.edition !== 'patch1078') throw new TypeError('선택 내용물 표시10.78 풀 오류');
    if (loadingDepth) return;
    if (pool.slot(parent)[EXTRA_OFFSET] & 9) return;
    if (pool.slot(parent)[TYPE_OFFSET] < FIRST_NON_FORM_TYPE) {
        need(assertion)();
        if (pool.slot(parent)[TYPE_OFFSET] < FIRST_NON_FORM_TYPE) return;
    }
    need(regular)(parent, CONTENT_PREVIEW_EVENT, SELECTION_PREVIEW_INTERVAL);
}
