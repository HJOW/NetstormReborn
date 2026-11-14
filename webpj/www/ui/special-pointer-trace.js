// 일반 좌클릭 선택 뒤 Ctrl+special 의 단일 지점 가상 70 순회를 복원한다(원문 4da263..2e6).
// (cpppj client/SpecialPointerTrace.cpp/.h, 기준선 1f00a25a, 10.78 전용)
// 일반 좌클릭의 편집 후처리보다 앞에 있는 원문 진단 부모다. 순회 중 현재 풀/로그가 바뀌어도 보관된 후보 목록으로 대체하지 않으며 도달한 경계만 지연 검사한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E435: RawMenuControls 선행 특수 포인터 진단 이식)

// 미선택 경계는 요구하지 않는다(조회 결과가 0 이면 후보/진단을 호출하지 않는다).
function need(value) { if (typeof value !== 'function') throw new Error('Ctrl special 지점 진단의 실제 하위 미연결'); return value; }

// control 이 눌려 있고 special 이 비영일 때만 사건 좌표를 역투영하고 단일 지점 탐색을 시작해 각 후보에 직접 가상 70 을 호출한 뒤 다음 후보를 조회한다.
// hooks = { project(x, y) → [wx, wy](4971e0 사건 좌표 역투영, 폴링 좌표로 바꾸지 않음), begin(point) → 첫 후보(4202f0), trace(token)(현재 객체의 직접 가상 70, Form 재귀 44dab0 과 다름), next() → 다음 후보(가상 효과 뒤 조회) }.
// 가상 70 복귀 뒤 현재 finder 를 진행하며 중복 후보도 임의 제거하지 않는다. 호출자는 Select 뒤의 현재 special 을 전달한다. 사용: traceSpecialPointer(true, special, { code, x, y }, hooks)
export function traceSpecialPointer(control, special, event, hooks) {
    if (!special || !control) return;
    const point = need(hooks.project)(event.x, event.y); let current = need(hooks.begin)(point);
    while (current) { need(hooks.trace)(current); current = need(hooks.next)(); }
}
