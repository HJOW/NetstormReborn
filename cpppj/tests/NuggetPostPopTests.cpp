// 너겟PostPop의 실제 조회/할당/난수 소비/두 예약과기본 호출을 원문과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawNuggetPostPop.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::test::rawscene;
namespace {
// 원문 경계의 unsigned DWORD 인자를 같은 순서로 기록한다.
void NuggetEvent(std::string& trace,std::string_view name,std::initializer_list<std::uint32_t> values) {
    if (!trace.empty()) trace+=';';trace+=name;
    // 인자의 상위 비트와 순서를 보존한다.
    for (const auto value:values) trace+=':'+std::to_string(value);
}
}
// 기대 분기는 새 독립PE288행에서만 읽는다.
TEST_CASE(NuggetPostPop_NativeAllocationsRandomAndFallback) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);std::size_t count{}; // 동일 부모SID를각입력에서독립사용한다.
    // 최초flags와기존예약/두할당실패를교차한모든원문을재생한다.
    for (const auto& fields:LoadFixture(NETSTORM_NUGGET_POSTPOP_FIXTURE).rows) {
        CHECK(fields.size()==3);if (fields.size()!=3) continue;std::vector<std::uint32_t> row;
        // 입력만 읽고 기대 결과를 계산하지 않는다.
        for (const auto& item:Split(fields[0],',')) row.push_back(Number(item));
        CHECK(row.size()==4);std::uint32_t allocations{},randoms{};std::string trace;
        RawNuggetPostPop logic(pool,NuggetPostPopHooks{
            [&](Sid sid,std::uint32_t event) { NuggetEvent(trace,"H",{sid.value,event});return row[1]!=0; },
            [&] { NuggetEvent(trace,"A",{40});return ((row[2]>>allocations++)&1U)!=0; },
            [&](float low,float high) {
                NuggetEvent(trace,"R",{std::bit_cast<std::uint32_t>(low),std::bit_cast<std::uint32_t>(high)});
                const std::array<std::array<std::uint32_t,2>,4> values{{{0x3dcccccdU,0x3e4ccccdU},{0,0},{0x3e99999aU,0x3f000000U},{0x7fc01234U,0x3e800000U}}}; // 명시 난수 반환입력이다.
                return std::bit_cast<float>(values[row[3]][randoms++]);
            },
            [&](Sid sid,std::uint32_t event,float payload) { NuggetEvent(trace,"C",{event,sid.value,std::bit_cast<std::uint32_t>(payload)}); },
            [&](Sid sid,std::uint32_t flags) { NuggetEvent(trace,"D",{sid.value,flags}); }});
        logic.PostPop(Sid{50},row[0]);CHECK(trace==fields[1] && fields[1]==fields[2]);++count;
    }
    CHECK(count==288);
}
