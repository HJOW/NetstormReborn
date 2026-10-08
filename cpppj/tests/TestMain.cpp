// 단위 테스트 실행 파일의 진입점
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string_view>

#include "TestSupport.h"
#include "SurfaceLifecycleInspect.h"

// 등록된 테스트를 모두 실행한다. 실패한 검사가 하나라도 있으면 1 을 돌려준다.
int main(int argc,char** argv) {
    // 선택 검사는 원본 파일을 읽을 때만 실행한다. 일반 CTest에는 원본 자산이 필요 없다.
    if (argc!=1) {
        try {
            if ((argc!=3 && argc!=4) || std::string_view(argv[1])!="--inspect-surface-graph" ||
                (argc==4 && std::string_view(argv[3])!="--cd")) throw std::runtime_error("--inspect-surface-graph <game-dir> [--cd]");
            netstorm::test::InspectSurfaceLifecycle(argv[2],argc==4 ? netstorm::o::OriginalEdition::Cd1072 : netstorm::o::OriginalEdition::Patch1078);
        } catch (const std::exception& error) { netstorm::test::ReportFailure(__FILE__,__LINE__,error.what()); }
        return netstorm::test::FailureCount()==0 ? 0 : 1;
    }
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
