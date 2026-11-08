// 건설 확정의 그래프 기준점 조회와 배치 통지의 소유 사제 조회를 실제 몸체에 연결하는 두 어댑터를 옮긴 모듈이다(cpppj `o/RawConstructionSelection.cpp/.h`).
//  - makeConstructionAnchorHooks(pool, anchor, hooks): 0048fdb0 `RawPlayerPlacementAnchor.locate(owner, type, x, y)` 를 확정 경계 `findBuilder(player, type, x, y)` 에 잇는다.
//    반환은 작업장/그래프 기준점 SID(WORD)이다. anchor 와 그 공유 상태는 훅보다 오래 살아야 하고, 나머지 확정 효과는 받은 훅을 유지한다.
//  - makeConstructionPriestLookupHooks(pool, lookup, hooks): 004921f0 `RawPriestLookup.find(player)` 를 배치 통지 경계 `findPriest(player)` 에 잇는다. 매 호출마다 현재 공유 목록을 읽고,
//    목록의 DWORD SID 는 실제 조회가 풀 범위를 검사한 뒤에만 WORD 로 줄여 전달한다. 이동/contained 등의 효과는 별도 연결한다.
// 두 어댑터 모두 풀이 다르면 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E135: 건설 선택 어댑터 이식으로 추가)

// 건설 확정의 findBuilder 를 실제 기준점 몸체로 잇는다. 기준점 몸체가 검증한 DWORD 결과를 기존 SID 경계에 WORD 로 전달한다. (2026-11-08 WB04-E135)
export function makeConstructionAnchorHooks(pool, anchor, hooks) {
    if (pool !== anchor?.pool) throw new TypeError('건설 기준점 어댑터의 풀 연결 오류');
    return { ...hooks, findBuilder: (player, type, x, y) => anchor.locate(player, type, x, y) & 0xffff };
}

// 배치 통지의 findPriest 를 실제 소유 사제 조회로 잇는다. (2026-11-08 WB04-E135)
export function makeConstructionPriestLookupHooks(pool, lookup, hooks) {
    if (pool !== lookup?.pool) throw new TypeError('건설 소유 사제 어댑터의 풀 연결 오류');
    return { ...hooks, findPriest: (player) => lookup.find(player) & 0xffff };
}
