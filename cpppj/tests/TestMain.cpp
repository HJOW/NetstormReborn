// 단위 테스트 실행 파일의 진입점
#include <cstdio>
#include <exception>

#include "TestSupport.h"

// 등록된 테스트를 모두 실행한다. 실패한 검사가 하나라도 있으면 1 을 돌려준다.
int main() {
    const auto& cases = netstorm::test::Registry();
    // 등록 순서대로 실행한다. 한 테스트가 실패해도 나머지를 계속 실행한다.
    for (const auto& testCase : cases) {
        const int failuresBefore = netstorm::test::FailureCount();
        // 파서 예외도 실패로 기록하여 뒤 테스트를 계속 확인한다.
        try { testCase.function(); }
        catch (const std::exception& error) {
            netstorm::test::ReportFailure(__FILE__, __LINE__, error.what());
        }
        const bool passed = netstorm::test::FailureCount() == failuresBefore;
        std::printf("[%s] %s\n", passed ? " OK " : "FAIL", testCase.name);
    }
    std::printf("%d test(s), %d failed check(s)\n",
                static_cast<int>(cases.size()), netstorm::test::FailureCount());
    return netstorm::test::FailureCount() == 0 ? 0 : 1;
}
