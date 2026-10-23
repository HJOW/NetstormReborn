// 실제Tell의전체치환→현재조건/명령→메타→동일상태2차HTML순서를집중검사한다.
#include "TestSupport.h"
#include "client/DialogScript.h"
#include "client/HtmlConditionEffect.h"
#include <stdexcept>

using namespace netstorm::client;

// 전체Config치환은먼저완료되고조건효과는그결과토큰을현재Config에다시치환한다.
TEST_CASE(DialogConditionFlow_WholeSubstitutionBeforeCondition) {
    netstorm::o::ConfigInterface config({});config.Configuration().Assign("X=1\r\n");
    const auto expanded=config.SubstituteLocal("<?{X}>visible</?><?!{X}>hidden</?>");config.Configuration().Set("X","0");
    HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;BindHtmlConditionConfig(hooks,config);
    const auto page=ParseDialogPage(expanded,state,globals,hooks);
    CHECK(page.hasContent);CHECK(page.htmlBody=="visible<br>");
    CHECK(HtmlTextParser(state,globals,hooks).Parse(page.htmlBody)=="visible\n\n");
    CHECK(state.shown==1 && state.hiddenDepth==0);
}

// 숨긴효과/메타를실행하지않고원본inline색과H/P줄바꿈을두순회에서유지한다.
TEST_CASE(DialogConditionFlow_CurrentEffectsAndInlineStyle) {
    netstorm::o::ConfigInterface config({});HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;
    std::vector<std::string> commands;std::string background;
    hooks.command=[&](auto&,std::string_view raw) { commands.emplace_back(raw); }; // 현재선택된명령의원시NULremainder만관찰한다.
    hooks.background=[&](auto&,std::string_view name) { background=name; }; // 현재선택된배경만기록한다.
    BindHtmlConditionConfig(hooks,config);BindHtmlPictureText(hooks);
    const auto page=ParseDialogPage("<background=\"real\"><?0><background=\"hidden\"><$Config,Hidden=1>$Button=Hidden,Tell,hidden</?>A<i>edge</i><br>B<$Config,Done=1>\n$Button=OK,DoNothing,0",state,globals,hooks);
    CHECK(background=="real" && page.background=="real");CHECK(commands.size()==1);CHECK(page.commands==std::vector<std::string>({"Config,Done=1"}));
    CHECK(page.items.size()==1 && page.items[0].label=="OK" && page.items[0].action.argument=="0");
    const auto rendered=HtmlTextParser(state,globals,hooks).Parse(page.htmlBody);
    CHECK(rendered.find("A~N~jedge~.\nB")!=std::string::npos);CHECK(rendered.find("Hidden")==std::string::npos);CHECK(commands.size()==1);
}

// 닫히지않은false상태를2차순회시새객체로초기화하지않는다.
TEST_CASE(DialogConditionFlow_ReusesHiddenStateOnSecondPass) {
    netstorm::o::ConfigInterface config({});HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;BindHtmlConditionConfig(hooks,config);
    const auto page=ParseDialogPage("visible<?0>hidden",state,globals,hooks);
    CHECK(page.hasContent && page.htmlBody=="visible<br>");CHECK(state.shown==0 && state.hiddenDepth==1);
    CHECK(HtmlTextParser(state,globals,hooks).Parse(page.htmlBody)=="\n");CHECK(state.shown==0 && state.hiddenDepth==1);
}

// 1차평문LF가공백으로변해도마지막인자는원본regex의space/$경계에서끝난다.
TEST_CASE(DialogConditionFlow_MetadataPriorityAndBodyBoundary) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;
    const auto page=ParseDialogPage("Body\n$Menu=Choice,Tell,menu\n$Button=Back,Tell,back\nTail\n$Checked=Locked,MissionBegin,next,1,0\n$Timeout=12,Tell,done",state,globals,{});
    CHECK(page.items.size()==3);CHECK(page.items[0].label=="Back" && page.items[0].action.argument=="back");
    CHECK(page.items[1].menu && page.items[1].label=="Choice");CHECK(page.items[2].checked && !page.items[2].enabled);
    CHECK(!page.items[1].checkedDirective && page.items[2].checkedDirective); // 체크값과 별도로 메타 종류를 현재 메뉴 장치에 보존한다.
    CHECK(page.timeoutSeconds==12 && page.timeout.argument=="done");CHECK(page.htmlBody.find("Tail")==std::string::npos);CHECK(page.htmlBody.find('$')==std::string::npos); // 1차LF가공백이되어메타뒤평문도다음$까지삭제되는원본동작이다.
}

// 효과만있는입력은본문창을요구하지않으며실제미공급효과/입력컨트롤은빈성공으로바꾸지않는다.
TEST_CASE(DialogConditionFlow_NoContentAndExplicitBoundaries) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;std::size_t calls=0;
    hooks.command=[&](auto&,auto) { ++calls; }; // 명령만있는페이지도첫순회의실제효과를한번예약한다.
    const auto page=ParseDialogPage("<$Config,Done=1>",state,globals,hooks);CHECK(!page.hasContent && calls==1);
    CHECK(ParseDialogPage("<b></b>",state,globals,hooks).hasContent); // 원본관문은StyleText제어바이트의영문도검사한다.
    bool rejected=false;try { ParseDialogPage("<$Config,X=1>",state,globals,{}); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    rejected=false;try { ParseDialogPage("$Input=Name,Config,X",state,globals,{}); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    rejected=false;try { ParseDialogPage("<!\"mana\">",state,globals,{}); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}

// 명령보고문자열의따옴표처리가기본HTMLcursor의첫'>'규칙을변경하지않는다.
TEST_CASE(DialogConditionFlow_QuotedCommandKeepsNativeCursorRule) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;std::string raw;
    hooks.command=[&](auto&,std::string_view value) { raw=value; }; // 실제큐에는태그끝이아닌전체NULremainder를전달한다.
    const auto page=ParseDialogPage("before<$Warning,\"A > B\">after",state,globals,hooks);
    CHECK(raw=="Warning,\"A > B\">after");CHECK(page.warnings==std::vector<std::string>({"A > B"}));
    CHECK(page.commands==std::vector<std::string>({"Warning,\"A > B\""}));CHECK(page.htmlBody.find(" B\"after")!=std::string::npos);
}
