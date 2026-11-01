// 실제 Client HTML의 이름 조회기와 Menu 둘째 인자를 보호 PE43개 및 같은 메타/고리/큐에서 확인한다.
#include "client/GameMenuCommands.h"
#include "client/HtmlWindowMetadata.h"
#include "client/ScenarioEffectInput.h"
#include "RawSceneSupport.h"
#include <algorithm>
#include <array>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

// 게임/World/MP 장치를 만들지 않고 현재 HTML의 동일 조회기를 원문43개의 대소문자 변형으로 비교한다.
TEST_CASE(HtmlScriptCommandNames_Pe43ActualResolver) {
    const auto source=LoadFixture(NETSTORM_HTML_SOLO_COMMAND_NAMES_FIXTURE).rows;CHECK(source.size()==44 && source[0].size()==4 && source[0][0]=="name" && source[0][3]=="command");if (source.size()!=44) return;
    const auto rows=std::span(source).subspan(1); // 최초 원문의 schema 한 줄을 그대로 보존하고 이름43행만 대조한다.
    for (const auto& row:rows) { // 첫 원문의 주소/이름/코드를 기준으로 현재 조회 결과만 읽는다.
        CHECK(row.size()==4);if (row.size()!=4) return;const auto expected=static_cast<std::uint32_t>(std::stoul(row[3],nullptr,16));std::array<std::string,4> variants{row[0],row[0],row[0],row[0]};
        for (std::size_t index=0;index<row[0].size();++index) { // 원본 ASCII 이름의 양쪽 대소문자와 교대 표기를 만든다.
            const char ch=row[0][index],low=ch>='A'&&ch<='Z' ? static_cast<char>(ch+32):ch,high=ch>='a'&&ch<='z' ? static_cast<char>(ch-32):ch;
            variants[1][index]=low;variants[2][index]=high;variants[3][index]=index&1 ? low:high;
        }
        for (const auto& name:variants) CHECK(HtmlScriptCommandValue(name)==expected); // 기존 부모의 동일 조회기이며 별도 기대 매핑으로 우회하지 않는다.
    }
    CHECK(HtmlScriptCommandValue("Show Technology")==0xffffffffU && HtmlScriptCommandValue("MisionBegin")==0xffffffffU && HtmlScriptCommandValue("AbortMission")==0xffffffffU);
}

// 실제 메타의 Menu 명령 캡처/숫자 인자와 입력 큐를 거쳐 명시 무동작 본문이 소비되는지 확인한다.
TEST_CASE(HtmlScriptCommandNames_MenuSecondArgumentUsesSameResolverAndQueue) {
    HtmlWindowGumpNode node{};ScriptStringRing strings;std::uint32_t command{},parameter{},menus{};std::string label;HtmlWindowMetadataHooks hooks; // Gump/메뉴 장치는 명시 경계이며 게임 설치 성공을 만들지 않는다.
    hooks.node=[&](auto token) { CHECK(token==1);return &node; };hooks.clearButtons=[](auto) {};hooks.commands.lookup=HtmlScriptCommandValue;hooks.storeText=[&](auto text) { return strings.Store(text); };
    hooks.beginMenu=[&](auto flags) { CHECK(flags==0x40000);++menus; };hooks.menuText=[&](auto text,auto code,auto value) { label=text;command=code;parameter=value; };
    std::array<char,8192> text{};const std::string source="$Menu=Trace,DumpSquids,6008 \n";std::copy(source.begin(),source.end(),text.begin());const auto index=strings.Index();
    CHECK(HtmlWindowMetadata(std::move(hooks)).Process(1,text)==1 && menus==1 && node.menuColumns==1 && label=="Trace" && command==0x0200011bU && parameter==6008 && !text[0] && strings.Index()==index);
    InputQueue queue;queue.PushCommand(command,std::bit_cast<std::int32_t>(parameter),std::bit_cast<std::int32_t>(parameter));const auto event=queue.Pop(false);CHECK(event.code==command && event.x==6008 && event.y==6008 && queue.Empty());CHECK(DispatchScenarioEffectInput(event,{}));
}
