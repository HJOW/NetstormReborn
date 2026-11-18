// State 정리의 커서 취소를 현재 Session 또는 같은 Client의 실제 초기 커서에 연결한다.
// 사용: cancelClientStateCursor(gameplay, cancelWithoutRuntime). 빠진 선택 경계는 도달할 때 거부한다.

// Session 대여가 있고 실제 커서가 설치된 경우에만 State 전용 취소를 실행한다.
export function cancelClientStateCursor(gameplay, cancelWithoutRuntime) {
    if (gameplay.borrowed()) {
        const session = gameplay.requireSession();
        if (session.hasCursorRuntimeForState()) { session.cancelCursorForState(); return; }
    }
    if (typeof cancelWithoutRuntime !== 'function') throw new Error('전체 State 정리의 선택된 실제 외부 장치 누락');
    cancelWithoutRuntime();
}
