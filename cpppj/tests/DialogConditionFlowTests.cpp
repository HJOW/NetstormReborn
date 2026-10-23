// 실제Tell의전체치환→현재조건/명령→메타→동일상태2차HTML순서를집중검사한다.
#include "TestSupport.h"
#include "client/DialogScript.h"
#include "client/HtmlConditionEffect.h"
#include "client/DialogExitCommand.h"
#include <array>
#include <bit>
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

// 기존Trim관찰API와페이지필드를보존하고OnExit의실제명령조회에는공백/따옴표원문을준다.
TEST_CASE(DialogConditionFlow_OnExitRawCaptureKeepsLegacyContract) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;DialogMetadataHooks metadata;
    std::vector<std::string> legacy;std::vector<std::array<std::string,2>> raw; // 두계약이각각받은현재필드다.
    metadata.resolveAction=[&](const DialogAction& action) { legacy.push_back(action.command+":"+action.argument); };
    metadata.resolveOnExit=[&](std::string_view command,std::string_view argument) { raw.push_back({std::string(command),std::string(argument)}); };
    const auto page=ParseDialogPage("Body$OnExit = label , Tell ,\"99\"$Button=OK, DoNothing ,0",state,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata);
    CHECK(legacy==std::vector<std::string>({"DoNothing:0","Tell:99"}));
    CHECK(raw.size()==1);if (raw.size()!=1) return;
    CHECK(raw[0][0]==" Tell " && raw[0][1]=="\"99\"");CHECK(page.onExit.command=="Tell" && page.onExit.argument=="99");
    CHECK(page.items.size()==1 && page.items[0].action.command=="DoNothing");
}

// 숨긴OnExit는고리를쓰지않고첫HTML의명령두개가끝난뒤현재메타순서에서마지막값을보관한다.
TEST_CASE(DialogConditionFlow_OnExitHiddenLastAndHtmlQueueOrder) {
    netstorm::o::ConfigInterface config({});HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks html;BindHtmlConditionConfig(html,config);
    ScriptStringRing strings;InputQueue queue;DialogExitCommandState pending;std::vector<std::string> order; // 원본inline큐와현재페이지반납값이다.
    const ScriptCommandHooks commands{[](std::string_view name) { return name=="Config" ? kScriptConfig : name=="Tell" ? 0x02000095U : 0xffffffffU; },{}}; // 검사에서사용한실제원문ID입력이다.
    html.command=[&](auto&,std::string_view command) { order.push_back("html");QueueScriptCommand(command,queue,strings,commands); };
    DialogMetadataHooks metadata;metadata.resolveAction=[&](const auto& action) { order.push_back("legacy:"+action.argument); };
    metadata.resolveOnExit=[&](std::string_view command,std::string_view argument) { order.push_back("exit:"+std::string(argument));ResolveDialogExitCommand(pending,command,argument,MakeDialogExitResolveHooks(strings,commands)); };
    const auto page=ParseDialogPage("<$Config,A=1>Body$OnExit=First,Tell,First<?0>$OnExit=Hidden,Tell,Hidden</?>$Button=OK,DoNothing,0<$Config,B=2>$OnExit=Last,Tell,Last ",state,globals,html,netstorm::o::OriginalEdition::Patch1078,metadata);
    CHECK(order==std::vector<std::string>({"html","html","legacy:0","legacy:First","exit:First","legacy:Last","exit:Last"}));
    CHECK(page.onExit.command=="Tell" && page.onExit.argument=="Last" && pending.command==0x02000095U);CHECK(strings.Index()==4 && strings.Resolve(pending.parameter)=="Last");
    const auto first=queue.Pop(false),second=queue.Pop(false);CHECK(first.code==kScriptConfig && second.code==kScriptConfig && queue.Empty());
    CHECK(strings.Resolve(std::bit_cast<std::uint32_t>(first.y))=="A=1" && strings.Resolve(std::bit_cast<std::uint32_t>(second.y))=="B=2");
    const auto calls=order.size();static_cast<void>(HtmlTextParser(state,globals,html).Parse(page.htmlBody));CHECK(order.size()==calls && strings.Index()==4 && queue.Empty());
}

// regex는원문처럼space에서끝나며따옴표와부호를resolver에그대로전달한다.
TEST_CASE(DialogConditionFlow_OnExitQuotesSpacesAndSigns) {
    const std::array<std::string,4> arguments{"-1","+2","\"99\"","a,b;c"}; // 원문세번째캡처에남는명시바이트입력이다.
    for (const auto& argument:arguments) { // 같은기존resolver가숫자이외첫바이트를문자열고리에전달한다.
        HtmlTextParserState state;HtmlTextParserGlobals globals;DialogExitCommandState pending;ScriptStringRing strings;DialogMetadataHooks metadata;
        metadata.resolveOnExit=[&](std::string_view command,std::string_view raw) { CHECK(command=="Tell" && raw==argument);ResolveDialogExitCommand(pending,command,raw,MakeDialogExitResolveHooks(strings,{[](auto) { return 0x02000095U; },{}})); };
        const auto page=ParseDialogPage("Body$OnExit=X,Tell,"+argument+' ',state,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata);
        CHECK(page.onExit.argument==(argument=="\"99\"" ? "99" : argument) && strings.Index()==1 && strings.Resolve(pending.parameter)==argument);
    }
    HtmlTextParserState state;HtmlTextParserGlobals globals;DialogMetadataHooks metadata;std::vector<std::string> raw;
    metadata.resolveOnExit=[&](auto,auto argument) { raw.emplace_back(argument); };
    static_cast<void>(ParseDialogPage("Body$OnExit=X,Tell,\"A B\"$OnExit=NotMatched,Tell, 99",state,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata));
    CHECK(raw==std::vector<std::string>({"\"A"})); // 공백은regex세번째캡처를끝내고선행공백인자는일치하지않는다.
}

// metadata의영문/숫자도원본4c8d50관문에포함되므로SetSize2는본문없는분기로가지않는다.
TEST_CASE(DialogConditionFlow_SetSizeExitIsContentAndResolvesBeforeSecondPass) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks html;ScriptStringRing strings;InputQueue queue;DialogExitCommandState pending;
    const ScriptCommandHooks commands{[](std::string_view name) { return name=="Config" ? kScriptConfig : name=="QuitApp" ? 0x02000001U : name=="MenuJustClosed" ? 0x020000beU : 0xffffffffU; },{}}; // 보호SetSize2에서사용하는실제ID다.
    html.command=[&](auto&,auto command) { QueueScriptCommand(command,queue,strings,commands); };
    DialogMetadataHooks metadata;metadata.resolveOnExit=[&](auto command,auto argument) { ResolveDialogExitCommand(pending,command,argument,MakeDialogExitResolveHooks(strings,commands)); };
    const auto page=ParseDialogPage("<$Config,SCREENW=800><$Config,SCREENH=600>$OnExit=test,QuitApp,99<$MenuJustClosed,0>",state,globals,html,netstorm::o::OriginalEdition::Patch1078,metadata);
    CHECK(page.hasContent && page.onExit.command=="QuitApp" && pending.command==0x02000001U && pending.parameter==99);CHECK(strings.Index()==2);
    CHECK(queue.Pop(false).code==kScriptConfig);CHECK(queue.Pop(false).code==kScriptConfig);CHECK(queue.Pop(false).code==0x020000beU && queue.Empty());
    const auto index=strings.Index();static_cast<void>(HtmlTextParser(state,globals,html).Parse(page.htmlBody));CHECK(strings.Index()==index && queue.Empty());
}

// 새raw훅으로연결한실제resolver만원문80/256버퍼의NUL경계를검사하고기존TrimAPI는확장하지않는다.
TEST_CASE(DialogConditionFlow_OnExitCapturePhysicalBounds) {
    const std::array<std::array<std::size_t,2>,3> sizes{{{79,255},{80,1},{1,256}}}; // 정상최대와각각한바이트초과입력이다.
    for (std::size_t index=0;index<sizes.size();++index) { // 실제첫HTML/regex캡처/OnExitresolver를한경로로검사한다.
        HtmlTextParserState state;HtmlTextParserGlobals globals;DialogMetadataHooks metadata;DialogExitCommandState pending;ScriptStringRing strings;bool rejected{};std::size_t legacy{};
        const auto command=std::string(sizes[index][0],'C'),argument=std::string(sizes[index][1],'A'); // 기대해석없이길이만명시한원문입력이다.
        metadata.resolveAction=[&](const auto&) { ++legacy; };metadata.resolveOnExit=[&](auto name,auto raw) { ResolveDialogExitCommand(pending,name,raw,MakeDialogExitResolveHooks(strings,{[](auto) { return 0x02000095U; },{}})); };
        try { static_cast<void>(ParseDialogPage("Body$OnExit=X,"+command+','+argument+' ',state,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata)); } catch (const std::length_error&) { rejected=true; }
        CHECK(legacy==1 && rejected==(index!=0));if (!index) CHECK(strings.Resolve(pending.parameter)==argument);else CHECK(!pending.command && strings.Index()==0);
    }
}

// 원문부정문자집합은LF를제외하지않으므로첫HTML이붙인끝LF도raw캡처에서보존한다.
TEST_CASE(DialogConditionFlow_OnExitRawTailLfStaysOwned) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;DialogMetadataHooks metadata;ScriptStringRing strings;DialogExitCommandState pending;
    metadata.resolveOnExit=[&](auto command,auto argument) {
        CHECK(command=="Tell" && argument=="-1\n");ResolveDialogExitCommand(pending,command,argument,MakeDialogExitResolveHooks(strings,{[](auto) { return 0x02000095U; },{}}));
    };
    const auto page=ParseDialogPage("Body$OnExit=X,Tell,-1",state,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata);
    CHECK(page.onExit.argument=="-1" && strings.Resolve(pending.parameter)=="-1\n");
}
