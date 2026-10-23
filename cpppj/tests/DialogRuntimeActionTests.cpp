// 실제 메타의 raw 캡처→생성 시점 DWORD 고정과 OnExit 해석 순서를 검사한다.
#include "TestSupport.h"
#include "client/DialogScript.h"
#include "client/DialogCommandLifetime.h"
#include <bit>
#include <string>
#include <vector>
using namespace netstorm::client;
namespace {
// 실제 해석기와 원본80칸 문자열 고리를 현재 페이지 메타 처리에 연결한다.
void BindRuntimeMetadata(DialogMetadataHooks& metadata,ScriptStringRing& strings,std::vector<std::string>& order,
    DialogExitCommandState& exit) {
    const auto resolve=MakeDialogExitResolveHooks(strings,{[](std::string_view name) {
        return name=="Tell" ? 0x02000095U : name=="DoNothing" ? 0x02000089U : 0xffffffffU;
    },[](auto) {}}); // 이름에 없는 공백을 이 검사 장치에서 임의 제거하지 않는다.
    metadata.resolveRuntimeAction=[resolve,&order](DialogAction& action,std::string_view name,std::string_view argument) {
        order.push_back(std::string(name)+','+std::string(argument));DialogExitCommandState selected;
        ResolveDialogExitCommand(selected,name,argument,resolve);action.commandCode=selected.command;action.parameter=selected.parameter;
    };
    metadata.resolveOnExit=[resolve,&order,&exit](std::string_view name,std::string_view argument) {
        order.push_back("Exit:"+std::string(name)+','+std::string(argument));ResolveDialogExitCommand(exit,name,argument,resolve);
    };
}
}
// 종류 우선순위의 raw 해석은 Button/Instant/Menu/Timeout/OnExit 순서이며 같은 메타를 두 번 고리에 쓰지 않는다.
TEST_CASE(DialogRuntimeAction_PriorityAndSingleExitResolve) {
    HtmlTextParserState html;HtmlTextParserGlobals globals;ScriptStringRing strings;DialogMetadataHooks metadata;
    std::vector<std::string> order;DialogExitCommandState exit;BindRuntimeMetadata(metadata,strings,order,exit);
    const auto page=ParseDialogPage("Body$Menu=Choice,Tell,menu $Button=Back,Tell,back $Timeout=3,Tell,done $ButtonInstant=Now,DoNothing,0 $OnExit=X,Tell,Blank ",html,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata);
    CHECK(order==std::vector<std::string>({"Tell,back","DoNothing,0","Tell,menu","Tell,done","Exit:Tell,Blank"}));
    CHECK(page.items.size()==3 && page.items[0].label=="Back" && page.items[1].instant && page.items[2].menu);
    CHECK(page.items[0].action.commandCode==0x02000095U && strings.Resolve(page.items[0].action.parameter)=="back");
    CHECK(page.items[1].action.commandCode==0x02000089U && page.items[1].action.parameter==0);
    CHECK(strings.Resolve(page.items[2].action.parameter)=="menu" && strings.Resolve(page.timeout.parameter)=="done");
    CHECK(exit.command==0x02000095U && strings.Resolve(exit.parameter)=="Blank" && strings.Index()==4);
}
// 표시용 Trim/인용 해제 표현과 실제 메타 인자의 첫 BYTE/따옴표/부호를 분리한다.
TEST_CASE(DialogRuntimeAction_RawArgumentKeepsQuotedAndSignedBytes) {
    HtmlTextParserState html;HtmlTextParserGlobals globals;ScriptStringRing strings;DialogMetadataHooks metadata;
    std::vector<std::string> order;DialogExitCommandState exit;BindRuntimeMetadata(metadata,strings,order,exit);
    const auto page=ParseDialogPage("Body$Button=Quoted,Tell,\"99\" $Button=Signed,Tell,-1 $Button=Number,Tell,123tail $Timeout=5,Tell,+2 ",html,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata);
    CHECK(page.items.size()==3 && page.items[0].action.argument=="99" && strings.Resolve(page.items[0].action.parameter)=="\"99\"");
    CHECK(strings.Resolve(page.items[1].action.parameter)=="-1" && page.items[2].action.parameter==123);
    CHECK(strings.Resolve(page.timeout.parameter)=="+2" && strings.Index()==3 && !exit.command);
}
// 해석 후 고리 인덱스/이름 장치가 달라도 선택은 저장된 DWORD를 재해석하지 않고 같은 큐에 보낸다.
TEST_CASE(DialogRuntimeAction_FiresFrozenFieldsWithoutNewLookup) {
    HtmlTextParserState html;HtmlTextParserGlobals globals;ScriptStringRing strings;DialogMetadataHooks metadata;
    std::vector<std::string> order;DialogExitCommandState exit;BindRuntimeMetadata(metadata,strings,order,exit);
    const auto page=ParseDialogPage("Body$Button=Next,Tell,Section ",html,globals,{},netstorm::o::OriginalEdition::Patch1078,metadata);
    const auto selected=page.items[0].action;strings.Store("Other");InputQueue queue;std::uint32_t killed{};
    FireDialogCommand(77,{selected.commandCode,selected.parameter},queue,[&](auto parent) { killed=parent; });
    const auto event=queue.Pop(false);CHECK(order.size()==1 && strings.Index()==2 && killed==77 && queue.Empty());
    CHECK(event.code==0x02000095U && std::bit_cast<std::uint32_t>(event.x)==selected.parameter && event.x==event.y);
    CHECK(strings.Resolve(selected.parameter)=="Section");
}
