// 선택 체력 막대의 현재 HP/최대 HP/색 조회 순서를 원문 그대로 옮긴 모듈이다(cpppj `client/SpriteHitPointBar.cpp/.h`, 원문 49898c, 10.78 전용). 실제 픽셀 출력과 독립인 정수 부모이며 출력기는 이 결과에 막대 기하를 적용한다.
//  - readSpriteHitPointBar(colors, hooks): 정상 색 colors[0]을 먼저 고르고, 현재 HP(hooks.current)와 최대 HP(hooks.maximum)를 읽어 `current ≤ maximum/2` 이면 colors[1], 다시 두 값을 읽어 `current ≤ maximum/4` 이면 colors[2]로 바꾸며,
//    마지막으로 한 번 더 읽은 current/maximum 을 결과로 돌려준다 — 즉 HP 조회 세 쌍(H,M,H,M,H,M)을 각각 현재 값으로 다시 읽는다(호출 사이에 캐시하지 않고 진단/장치가 값을 바꿔도 이어서 읽음).
//    나눗셈은 signed 32비트 정수의 0 방향 절삭이다. 색 배열은 진단 복귀 뒤 현재 값을 읽도록 사본을 만들지 않는다.
//  - 반환: { color, current, maximum }. hooks 는 도달한 읽기에서만 요구한다(없으면 Error). 10.72(CD)의 clamp 경로는 옮기지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E377: 5-A SpriteHitPointBar 이식으로 추가, 기준선 뒤 cpppj 변경 없음)

// 선택된 읽기 장치만 그 호출 시점에 요구한다. (2026-11-13 WB04-E377)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('선택 HP 막대의 실제 조회 장치 미연결');
    return hook;
}

// signed 32비트 정수 나눗셈(0 방향 절삭)이다. (2026-11-13 WB04-E377)
const divide = (value, divisor) => Math.trunc((value | 0) / divisor) | 0;

// 선택 막대의 HP/색 조회를 원문 순서로 실행한다. colors = [정상, 절반 이하, 1/4 이하] 색(현재 값을 매번 읽음), hooks = { current(), maximum() }. 사용: readSpriteHitPointBar([11, 22, 33], hooks). (2026-11-13 WB04-E377)
export function readSpriteHitPointBar(colors, hooks) {
    const result = { color: colors[0], current: 0, maximum: 0 };
    let current = need(hooks.current)() | 0;
    let maximum = need(hooks.maximum)() | 0;
    if (current <= divide(maximum, 2)) result.color = colors[1];
    current = need(hooks.current)() | 0;
    maximum = need(hooks.maximum)() | 0;
    if (current <= divide(maximum, 4)) result.color = colors[2];
    result.current = need(hooks.current)() | 0;
    result.maximum = need(hooks.maximum)() | 0;
    return result;
}
