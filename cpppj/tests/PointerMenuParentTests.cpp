// 원문 부모의 호출순서/인자와 지연 누락 계약을 독립 fixture로 확인한다.
#include "RawSceneSupport.h"
#include "client/RawObjectMenuOpen.h"
#include "client/SpecialPointerTrace.h"
#include <bit>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { //명시 외부 경계의 효과 문자열만 조합한다.
void PointerTrace(std::string& trace,std::string value) { if (!trace.empty()) trace+=';';trace+=value; } //기대값은 fixture에서만 읽는다.
}
TEST_CASE(PointerMenuParent_ReplaysNativeOpen8) { //실제비영DWORD와좌표가열기/소리분기에그대로전달된다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_POINTER_MENU_PARENT_FIXTURE).rows) { //별도PE실행의가상ac/음향인자를재생한다.
        CHECK(row.size()==4);std::string trace;const auto result=OpenPointerObjectMenu([&] { PointerTrace(trace,"O,50,131072,"+row[1]+','+row[2]);return Number(row[0]); },[&](auto name) { PointerTrace(trace,"S,"+std::string(name)+",0,0,0,0,0"); });
        CHECK(result==Number(row[0]));CHECK(trace==row[3]);++count;
    }
    CHECK(count==8);
}
TEST_CASE(PointerMenuParent_ReplaysNativeSpecialTrace36) { //조건단락/빈목록/중복후보의현재진행을원문과비교한다.
    const std::array<std::vector<std::uint32_t>,3> candidates{{{},{50},{50,51,50}}};std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_POINTER_SPECIAL_TRACE_FIXTURE).rows) { //투영과finder는원문관측과같은명시외부입력이다.
        CHECK(row.size()==6);std::string trace;std::size_t index{};const auto& list=candidates.at(Number(row[2]));SpecialPointerTraceHooks hooks;
        hooks.project=[&](auto x,auto y) { PointerTrace(trace,"P,"+std::to_string(x)+','+std::to_string(y));return std::array<float,2>{123.25F,-9.5F}; };
        hooks.begin=[&](auto point) { PointerTrace(trace,"B,"+std::to_string(std::bit_cast<std::uint32_t>(point[0]))+','+std::to_string(std::bit_cast<std::uint32_t>(point[1]))+",0");return list.empty() ? 0U : list[0]; };
        hooks.trace=[&](auto sid) { PointerTrace(trace,"V,"+std::to_string(sid)); };hooks.next=[&] { PointerTrace(trace,"N");++index;return index<list.size() ? list[index] : 0U; };
        TraceSpecialPointer(Number(row[1])!=0,Number(row[0]),{InputCode::kLeftButton,static_cast<std::int32_t>(std::stoll(row[3])),static_cast<std::int32_t>(std::stoll(row[4]))},hooks);
        CHECK((trace.empty() ? "-" : trace)==row[5]);++count;
    }
    CHECK(count==36);
}
TEST_CASE(PointerMenuParent_OnlyReachedHooksAreRequired) { //선택된누락뒤선행효과는유지하고단락된장치는요구하지않는다.
    CHECK(OpenPointerObjectMenu([] { return 0U; },{})==0);std::uint32_t opened{};CHECK(Throws([&] { OpenPointerObjectMenu([&] { ++opened;return 1U; },{}); }));CHECK(opened==1);
    TraceSpecialPointer(false,1,{},{});TraceSpecialPointer(true,0,{},{});SpecialPointerTraceHooks hooks;hooks.project=[](auto,auto) { return std::array<float,2>{}; };hooks.begin=[](auto) { return 0U; };CHECK(!Throws([&] { TraceSpecialPointer(true,1,{},hooks); }));
    hooks.begin=[](auto) { return 50U; };std::uint32_t traced{};hooks.trace=[&](auto) { ++traced; };CHECK(Throws([&] { TraceSpecialPointer(true,1,{},hooks); }));CHECK(traced==1);
}
