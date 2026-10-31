// 대규모 전체 검사 재컴파일 없이 같은 제품 라이브러리의 관련 검사 파일만 실행하는 공용 진입점이다.
#include "TestSupport.h"
#include <exception>
#include <string_view>

// 주 검사와 같은 Registry/실패 집계를 사용하며 --list와 부분 이름 --filter를 지원한다.
int main(int argc,char** argv) {
    const auto& cases=netstorm::test::Registry(); // 이 집중 대상에 실제 링크한 검사만 포함한다.
    if (argc==2 && std::string_view(argv[1])=="--list") {
        for (const auto& test:cases) std::printf("%s\n",test.name); // 본문이나 외부 장치를 실행하지 않는다.
        return 0;
    }
    const bool filtered=argc==3 && std::string_view(argv[1])=="--filter";
    if (argc!=1 && !filtered) return 2;
    std::size_t count{}; // 실제로 호출한 검사 개수이며 등록 수와 구별한다.
    for (const auto& test:cases) { // 선택한 사례를 한 번씩 실행하고 첫 실패 뒤에도 나머지 결과를 수집한다.
        if (filtered && std::string_view(test.name).find(argv[2])==std::string_view::npos) continue;
        const auto before=netstorm::test::FailureCount();++count;
        try { test.function(); }catch (const std::exception& error) { netstorm::test::ReportFailure(__FILE__,__LINE__,error.what()); }
        std::printf("[%s] %s\n",before==netstorm::test::FailureCount()?" OK ":"FAIL",test.name);
    }
    if (!count) netstorm::test::ReportFailure(__FILE__,__LINE__,"필터와 일치하는 검사가 없습니다");
    std::printf("%zu test(s), %d failed check(s)\n",count,netstorm::test::FailureCount());
    return netstorm::test::FailureCount()?1:0;
}
