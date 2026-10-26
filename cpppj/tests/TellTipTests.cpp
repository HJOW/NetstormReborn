// 원본 TellTip의 실제 일반 입력388관측과 현재 번호/설정·부분 효과를 검증한다.
#include "RawSceneSupport.h"
#include "client/TellTip.h"

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 실제 callback 순서를 관찰하며 다음 번호의 기대값은 Native fixture에서 읽는다.
// 원문과 같은 세미콜론 trace를 보관한다.
void TipTrace(std::string& trace,std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
}
// 일반 입력의 signed/감김/서로 다른 인자와 Guide/Gump 선소비까지 원본 출력과 비교한다.
TEST_CASE(TellTip_ReplaysNativeInput388SignedClampAndOkAdvance) {
    const auto fixture=LoadFixture(NETSTORM_TELL_TIP_FIXTURE);std::size_t checked=0;
    for (const auto& row:fixture.rows) { // 앞4열만 입력이며 최종 번호와 trace를 기대값으로 읽는다.
        CHECK(row.size()==8 || row.size()==10);const InputEvent event{Number(row[0]),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2]))};auto number=Number(row[3]);std::string trace;
        TellTipHooks hooks;hooks.format=[&](auto value) {
            if (event.y==1 || event.y>=2) TipTrace(trace,"number:"+std::to_string(number)); // 원문 직접 store는 별도 장치가 아니므로 최초 서식 경계의 현재 값으로 관찰한다.
            TipTrace(trace,"format:"+std::to_string(value));return std::to_string(value);
        };
        hooks.setConfig=[&](auto key,auto value) { TipTrace(trace,"set:"+std::string(key)+':'+std::string(value)); };
        hooks.tell=[&](auto section) { TipTrace(trace,"tell:"+std::string(section)); };
        if (row.size()==8 || (!Number(row[8]) && !Number(row[9]))) CHECK(DispatchTellTip(event,number,hooks)); // 기존 Guide/Gump 소비 관문이 이미 처리한 입력은 다시 분배하지 않는다.
        CHECK(number==Number(row[4]));CHECK((trace.empty() ? "-" : trace)==row[5]);CHECK(row[6]=="0" && Number(row[7])>0);++checked;
    }
    CHECK(checked==388);
}
// OK3도 증가/Config를 실행하지만 Tips·Blank를 요청하지 않고 기존 HTML 수명을 따른다.
TEST_CASE(TellTip_OkAndReleaseRequireOnlySelectedEffects) {
    std::uint32_t number=1;TellTipHooks hooks;std::string key,text;hooks.format=[](auto value) { return std::to_string(value); };hooks.setConfig=[&](auto name,auto value) { key=name;text=value; };
    CHECK(DispatchTellTip({kTellTipCommand,99,3},number,hooks));CHECK(number==2 && key=="tipNumber" && text=="2");
    CHECK(DispatchTellTip({kTellTipCommand|InputCode::kRelease,0,2},number,{}));CHECK(number==2);CHECK(!DispatchTellTip({0x020000ecU,0,2},number,{}));CHECK(number==2);
}
// 번호 쓰기 뒤 실패·설정 callback의 현재값 변경을 보존하며 뒤에 별도 번호 동기화를 하지 않는다.
TEST_CASE(TellTip_PartialFailureAndCurrentConfigMutation) {
    std::uint32_t number=10;CHECK(Throws([&] { DispatchTellTip({kTellTipCommand,0,1},number,{}); }));CHECK(number==9);
    TellTipHooks hooks;std::string trace;hooks.format=[&](auto value) { TipTrace(trace,"format:"+std::to_string(value));return std::to_string(value); };
    hooks.setConfig=[&](auto key,auto value) { CHECK(key=="tipNumber" && value=="10");number=77;TipTrace(trace,"set"); };hooks.tell=[&](auto section) { CHECK(section=="Tips" && number==77);TipTrace(trace,"tell"); };
    CHECK(DispatchTellTip({kTellTipCommand,0,2},number,hooks));CHECK(number==77 && trace=="format:10;set;tell");
    hooks.setConfig={};CHECK(Throws([&] { DispatchTellTip({kTellTipCommand,0,2},number,hooks); }));CHECK(number==78);
}
// Config callback이 사건 메모리를 바꾸어도 원문 ESI에 잡힌 최초 인자로 OK의 안내 생략을 결정한다.
TEST_CASE(TellTip_ConfigCallbackCannotChangeCapturedOkParameter) {
    InputEvent event{kTellTipCommand,0,3};std::uint32_t number=1;TellTipHooks hooks;
    hooks.format=[](auto value) { return std::to_string(value); };
    hooks.setConfig=[&](auto key,auto value) { CHECK(key=="tipNumber" && value=="2");event.y=2; };
    CHECK(DispatchTellTip(event,number,hooks));CHECK(number==2 && event.y==2);
}
