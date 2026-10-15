// 실제 두 PE의 제목 분기와 설정/미션 title 조회 순서·최종 문자열을 대조한다.
#include "RawSceneSupport.h"
#include "client/WindowTitle.h"
#include "o/ConfigInterface.h"
#include <sstream>
using namespace netstorm::client;
namespace {
// 현재 판본의 독립 native fixture를 전체 재생한다. 기대 분기는 계산하지 않는다.
void CheckWindowTitle(std::string_view edition) {
    std::size_t count{}; // 이 판본에서 실제 대조한 고유 입력 수다.
    // 독립 원본의 고유 입력과 관찰 결과를 순서대로 대조한다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_WINDOW_TITLE_FIXTURE).rows) {
        if (row[0]!=edition) continue;
        WindowTitleState state; // 원본과 같은 현재 요새/모드/Template/미션 입력이다.
        if (row[1]!="#NULL") state.fortName=row[1];
        state.editor=std::stoi(row[2])!=0;state.mission=std::stoi(row[3])!=0;
        state.templatePresent=std::stoi(row[4])!=0;state.templateReadOnly=std::stoi(row[5])!=0;
        state.demo=std::stoi(row[6])!=0;state.testBattle=std::stoi(row[7])!=0;
        std::string trace,result; // 실제 C++ 소비자 요청 순서와 최종 적용 바이트다.
        const auto append=[&](std::string text) { if (!trace.empty()) trace+='|';trace+=text; }; // 각 원본 요청을 순서대로 한 필드에 쓴다.
        RefreshWindowTitle(state,{
            [&](std::string_view key,std::optional<std::string_view> arg) {
                const auto value=arg ? std::string(*arg) : "<null>";append(std::string(key)+"="+value);return "<"+std::string(key)+":"+value+">";
            },
            [&] { append("title");return row[8]; },
            [&](std::string_view title) { result=title; }});
        CHECK(trace==row[9] && result==row[10]);++count;
    }
    CHECK(count==576);
}
}
// PATCH의 미션/편집/readonly 우선순위와 NULL/빈 문자열 인자를 실제 원본과 비교한다.
TEST_CASE(WindowTitle_PatchNative) { CheckWindowTitle("originals"); }
// CD의 별도 문자열 복사 몸체도 동일한 실제 요청/결과와 비교한다.
TEST_CASE(WindowTitle_CdNative) { CheckWindowTitle("originalCD"); }
// 실제 설정 치환을 거쳐 메뉴/미션 제목을 만들며 파일명 fallback을 넣지 않는다.
TEST_CASE(WindowTitle_ActualConfigSubstitution) {
    netstorm::o::ConfigInterface config({}); // 장치를 쓰지 않는 실제 ConfigInterface다.
    config.Configuration().Assign("blankWindowName = NetStorm Menu\nmissionWindowName = NetStorm: {local.1}\n");
    WindowTitleState state;std::string result; // 메뉴에서 일반 미션으로 전환하는 현재 입력/적용 제목이다.
    WindowTitleHooks hooks{
        [&](std::string_view key,std::optional<std::string_view> argument) { return config.PathSpec(key,argument); },
        [] { return std::string("The War Begins"); },[&](std::string_view value) { result=value; }};
    RefreshWindowTitle(state,hooks);CHECK(result=="NetStorm Menu");state.fortName="thewarbegins";state.mission=true;
    RefreshWindowTitle(state,hooks);CHECK(result=="NetStorm: The War Begins");
}
