// 월드 Pool 정리 부모(004af170, 10.78 전용)를 기존 실제 SID 저장소와 실패점에 연결하는 모듈이다(cpppj `o/RawSquidPoolReset.cpp/.h`). Pool 바이트 초기화는 복제하지 않고 이미 옮긴 `SidPool.reset` 을 재사용한다.
//  - resetSquidWorldPool(pool, invalid, capacity = 0): 실제 하위 `pool.reset(capacity)` 가 정상 반환한 뒤에만 같은 삭제 기록과 실패점을 정리한다. 하위가 실패하면 기존 기록·실패점의 뒤 효과를 실행하지 않는다(선행 UI/Kernel/Runtime 반납은 호출자 책임).
//  - finishSquidPoolReset(pool, invalid): 이미 하위 reset 이 끝난 시작 생성자가 재실행 없이 같은 부모 꼬리만 잇는다: 삭제 기록 80DWORD(client/other 각 20쌍) 비우기 → 실패점 y → x 순서로 −10000.0f(0xc61c4000)를 쓴다.
// 실패점 invalid = { x, y }(RawPathPoint)는 손실 위치·미사일 이전점 등 다른 −10000 필드와 별개이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E37: 월드 Pool 정리 부모 이식으로 추가)

// 원문이 두 DWORD 로 쓰는 실패점 값(−10000.0f, 0xc61c4000)이다. (2026-11-08 WB04-E37)
const INVALID_POINT = -10000;

// Native 는 Y 다음 X 의 두 DWORD 만 쓴다. (2026-11-08 WB04-E37)
export function finishSquidPoolReset(pool, invalid) {
    pool.clearDeletionHistory();
    invalid.y = INVALID_POINT;
    invalid.x = INVALID_POINT;
}

// 하위 reset 이 실패하면 예외가 그대로 전파되어 기록·실패점은 바뀌지 않는다. (2026-11-08 WB04-E37)
export function resetSquidWorldPool(pool, invalid, capacity = 0) {
    pool.reset(capacity);
    finishSquidPoolReset(pool, invalid);
}
