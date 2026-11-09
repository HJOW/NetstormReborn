// checking(진단 모드, 005e4794)에서만 선택되는 공통 Form 검사와 총의 순수 지면 소유자 진단을 옮긴 모듈이다(cpppj `o/RawCheckingDiagnostics.h`, 10.78 전용). 게임 동작/생성/공격을 실행하지 않는 같은 raw 자료의 진단 부모이다.
//  - checkRawObjectForms(pool, parent)(004ad730): 자산 부모(TYPE ≤ 69)이거나 free/void(상태 비트 1·4)인 부모는 거짓이고, 그 밖에는 부모의 head 부터 next 고리를 따라 모든 자식이 free/void 가 아니고 자산이 아닌 Form(TYPE > 69)이면
//    참이다(dead 는 허용). 각 next 를 소비하며 이전 진단 결과를 캐시하지 않는다. 호스트만 풀 밖 번호와 순환 체인을 명시 오류로 거부한다(원문은 끝없이 돈다).
//  - diagnoseRawGunIslandOwners(pool, hash, types, parent, footprint, checking, noIslandType, assertion)(0046b830 case 1 의 41def0, flags 0): checking 이 꺼져 있으면 finder/발자국 읽기 전에 즉시 반환한다. 켜져 있으면 발자국(float)을 정수 칸으로
//    바꿔(`checkingStoredInteger`: 상한은 보호 PE 502310 의 float 0.99999 를 더하고 int → float 저장 뒤 다시 ftol, 비유한/64비트 밖은 정수 불확정값의 하위 0) 일반 4단계 finder 로 훑으며, 후보 TYPE 이 현재 noIsland TYPE 이고 checking 이 아직 켜져 있고
//    후보 소유자가 부모 소유자와 다르면 Gunprocess.cpp 0xad 진단을 보고한다. 진단 뒤 전역/부모를 매번 다시 읽는다.
//  - checkingStoredInteger/checkingInteger: 41d770/41d7a0/41def0 의 float 저장 위치를 같은 진단에서만 쓰는 정수 변환이다.
// checking 과 noIslandType 은 `() => 정수` getter 로 소비 위치마다 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E240: checking 진단 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';

// 보호 PE 502310 의 원시 float 상한 편향(0.999 상수와 구별한다)이다. (2026-11-10 WB04-E240)
const UPPER_BIAS = 0.9999899864196777;
// 슬롯 필드 위치(TYPE +10·상태 +11·소유자 +34·head +6·next +4)와 상태 비트(free 1·void 4), 자산 TYPE 상한, 소유자 진단 줄이다. (2026-11-10 WB04-E240)
const TYPE_AT = 10;
const STATE_AT = 11;
const OWNER_AT = 34;
const HEAD_AT = 6;
const NEXT_AT = 4;
const FREE_OR_VOID = 5;
const LAST_ASSET_TYPE = 69;
const GUN_OWNER_LINE = 0xad;

// 004ad730: 자산 부모와 비자산 Form 의 free/void 만 검사하며 dead 는 허용한다. 풀 밖 번호/순환 체인은 호스트가 거부한다. (2026-11-10 WB04-E240)
export function checkRawObjectForms(pool, parent) {
    const raw = pool.slot(parent);
    if (raw[TYPE_AT] <= LAST_ASSET_TYPE || (raw[STATE_AT] & FREE_OR_VOID)) return false;
    let next = raw[HEAD_AT] | (raw[HEAD_AT + 1] << 8); // 원문 head WORD 와 호스트의 순환 한계이다.
    let guard = 0;
    while (next) { // 각 실제 next 를 소비하며 이전 진단 결과를 캐시하지 않는다.
        guard += 1;
        if (next >= pool.capacity || guard > pool.capacity) throw new Error('checkForms 실제 물리 범위 밖/순환 체인');
        const child = pool.slot(next);
        if ((child[STATE_AT] & FREE_OR_VOID) || child[TYPE_AT] > LAST_ASSET_TYPE) return false;
        next = child[NEXT_AT] | (child[NEXT_AT + 1] << 8);
    }
    return true;
}

// ftol: signed 64 불확정값의 하위 0 과 정상 DWORD 감김을 보존한다(비유한·64비트 밖은 0). (2026-11-10 WB04-E240)
export function checkingInteger(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 41d770/41d7a0/41def0 의 float 저장 위치를 같은 진단에서만 쓴다: 상한은 편향 float 를 더하고 int → float 저장 뒤 다시 ftol 한다. (2026-11-10 WB04-E240)
export function checkingStoredInteger(value, upper) {
    const first = checkingInteger(value + (upper ? UPPER_BIAS : 0));
    return checkingInteger(Math.fround(first));
}

// 0046b830 case 1 의 41def0(flags 0) → 현재 noIsland/owner 진단만 실행한다. footprint = { left, top, right, bottom } float 수, checking/noIslandType = () => 정수, assertion(줄) 은 필수(없으면 진단 지점에서 거부). (2026-11-10 WB04-E240)
export function diagnoseRawGunIslandOwners(pool, hash, types, parent, footprint, checking, noIslandType, assertion) {
    if (!checking()) return; // 원문 46ba98 의 바깥 관문은 finder/발자국 읽기 전 즉시 반환한다.
    const finder = new RawSquidFinder(pool, hash, types); // 같은 일반 4단계 finder 를 쓰며 표면 지도/결과 사본으로 바꾸지 않는다.
    const area = {
        left: checkingStoredInteger(footprint.left, false), top: checkingStoredInteger(footprint.top, false),
        right: checkingStoredInteger(footprint.right, true), bottom: checkingStoredInteger(footprint.bottom, true),
    };
    let current = finder.begin(area, 0); // 원문 기본 필터는 모든 현재 후보를 허용한다.
    while (current) { // 다음은 finder 가 보존한 링크를 따르며 진단 callback 뒤 전역/부모를 다시 읽는다.
        if (pool.slot(current)[TYPE_AT] === (noIslandType() >>> 0) && checking() && pool.slot(current)[OWNER_AT] !== pool.slot(parent)[OWNER_AT]) {
            if (typeof assertion !== 'function') throw new TypeError('Gunprocess.cpp 0xad noIsland 소유자 원본 진단 미연결');
            assertion(GUN_OWNER_LINE);
        }
        current = finder.next();
    }
}
