// 원본 SDK의 실제14개 입력과 타이머 현재 쓰기/선택 장치 순서를 비교한다.
#include "TestSupport.h"
#include "client/MovieTimer.h"
#include <fstream>
#include <sstream>
#include <string>

// 최초 기한·대기·late 재설정·ready/fast·간격0·DWORD 감김을 실제 SDK 관측과 대조한다.
TEST_CASE(MovieTimer_NativeSdk14DeadlineRebaseAndWrap) {
    using namespace netstorm::client;std::ifstream file(NETSTORM_MOVIE_TIMER_FIXTURE);CHECK(file.good());std::string line;int count{}; // 독립 최초 관측 파일이다.
    // 각 SDK 입력은 새 타이머와 명시 시계를 사용한다.
    while (std::getline(file,line)) {
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();std::istringstream fields(line);MovieTimerState state;std::uint32_t now{},result{},ready{},due{};std::string expected,trace;
        fields>>state.fast>>state.ready>>state.due>>state.interval>>now>>result>>ready>>due>>expected;CHECK(!fields.fail());MovieTimerHooks hooks;
        hooks.milliseconds=[&] { trace="time";return now; };hooks.yield=[&] { if (!trace.empty()) trace+=';';trace+="yield"; };
        CHECK(static_cast<std::uint32_t>(WaitMovieTimer(state,hooks))==result);CHECK(state.ready==ready && state.due==due);if (trace.empty()) trace="-";CHECK(trace==expected);++count;
    }
    CHECK(count==14);
}
