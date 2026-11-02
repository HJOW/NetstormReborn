// 원문 F12 관측과 실제 즉시 호출/현재 모드/선택된 장치 누락 경계를 확인한다.
#include "RawSceneSupport.h"
#include "client/ScreenMouseKey.h"
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client; // 제품과 같은 사건/장치 분배기를 사용한다.
using namespace netstorm::test::rawscene; // fixture 해독만 재사용하며 기대 조건은 계산하지 않는다.

TEST_CASE(ScreenMouseKey_Native552CurrentModeAndInputBits) { // 실제 PE의 입력 마스크/모드 읽기/장치 호출 순서를 대조한다.
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/screen-mouse-key-x86.tsv").string().c_str());std::size_t count{}; // 새 관측 행의 실제 재생 수다.
    for (const auto& row:fixture.rows) { // 수정키/뗌/폴링 표시/다른 사건과 원시 화면 DWORD를 모두 재생한다.
        CHECK(row.size()==2);if (row.size()!=2) continue;const auto fields=Split(row[0],',');CHECK(fields.size()==2);if (fields.size()!=2) continue;std::string trace; // 원문과 같은 현재 조회/호출의 관측이다.
        const auto acted=DispatchScreenMouseKey({Number(fields[0]),37,-19},[&] { trace="R:"+fields[1];return Number(fields[1]); },[&](auto mode) { trace+=";M:"+std::to_string(mode); });
        CHECK((trace.empty() ? "-" : trace)==row[1]);CHECK(acted==(row[1].find(";M:")!=std::string::npos));++count;
    }
    CHECK(count==552);
}

TEST_CASE(ScreenMouseKey_UsesCurrentModeAndCallsDeviceImmediately) { // 전환 뒤 현재 값을 다시 읽고 기존 입력 큐를 거치지 않는다.
    InputQueue queue;queue.Push('P',17,19);std::uint32_t mode=235,reads{},changes{}; // 실제 큐의 선행 사건과 명시 장치 현재 값이다.
    const auto read=[&] { ++reads;return mode; };const auto change=[&](auto value) { CHECK(!queue.Empty());mode=value;++changes; }; // 효과는 호출 스택 안에서 바로 일어난다.
    CHECK(DispatchScreenMouseKey({0x7b0000U,0,0},read,change) && mode==171 && reads==1 && changes==1);
    CHECK(DispatchScreenMouseKey({0x387b0000U,0,0},read,change) && mode==235 && reads==2 && changes==2);
    CHECK(!DispatchScreenMouseKey({0x807b0000U,0,0},read,change) && mode==235 && reads==2 && changes==2);
    const auto first=queue.Pop(false);CHECK(first.code=='P' && first.x==17 && first.y==19 && queue.Empty());
}

TEST_CASE(ScreenMouseKey_LazyDeviceErrorsAndWindowedMode) { // 뗌/다른 사건은 조회도 하지 않으며 창 모드에는 전환 장치가 필요 없다.
    CHECK(!DispatchScreenMouseKey({0x807b0000U,0,0},{},{}));CHECK(!DispatchScreenMouseKey({0x027b0000U,0,0},{},{}));CHECK(!DispatchScreenMouseKey({0x7a0000U,0,0},{},{}));
    std::uint32_t reads{};CHECK(!DispatchScreenMouseKey({0x7b0000U,0,0},[&] { ++reads;return 10U; },{}) && reads==1);
    bool failed{};try { DispatchScreenMouseKey({0x7b0000U,0,0},{},{}); }catch (const std::logic_error&) { failed=true; }CHECK(failed);
    failed=false;try { DispatchScreenMouseKey({0x7b0000U,0,0},[&] { ++reads;return 235U; },{}); }catch (const std::logic_error&) { failed=true; }CHECK(failed && reads==2); // 선행 조회를 되돌리거나 누락된 실제 효과를 성공으로 채우지 않는다.
}
