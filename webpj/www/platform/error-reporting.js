// 게임 실행 중 예외의 단계와 상태를 브라우저 콘솔에 남긴다. 원래 예외 객체를 함께 전달해 스택과 원인을 보존한다.
// 콘솔 자체의 오류가 게임의 기존 오류 처리나 원래 예외를 가리지 않도록 보고 실패는 무시한다. (2026-11-19 WB04-E862)
export function reportGameError(phase, error, context = {}) {
    try {
        console.error('Netstorm: 실행 중 오류가 발생했습니다.', {
            ...context,
            phase,
            timestamp: new Date().toISOString(),
        }, error);
    } catch {
        // 진단 출력이 불가능해도 호출자가 원래 오류를 처리하게 둔다.
    }
}
