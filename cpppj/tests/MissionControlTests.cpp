// 실제 Abort 사건·명령 이름/토큰/큐와 원문 실패/중단 화면의 재시작 버튼을 대조한다.
#include "DudeSpawnSupport.h"
#include "client/MissionGuideInput.h"
#include "client/Mission.h"
#include "client/DialogScript.h"

using namespace netstorm::client;
namespace {
// 원문 TSV의 문자열을 바이트 그대로 복원한다. 빈 hex는 빈 문자열이다.
std::string ControlBytes(std::string_view encoded) {
    std::string result;
    // 한 바이트씩 읽어 signed char나 UTF-8 치환으로 원문을 바꾸지 않는다.
    for (std::size_t index=0;index<encoded.size();index+=2) result+=static_cast<char>(std::stoul(std::string(encoded.substr(index,2)),nullptr,16));
    return result;
}
// 원본 전체 활성 큐와 비교할 실제 입력 큐를 순서대로 직렬화한다.
std::string ControlQueue(InputQueue& queue) {
    std::string result;
    // 큐의 끝까지 실제 Pop을 사용한다. 이 원문 입력은 DEL 변환값과 겹치지 않는다.
    while (!queue.Empty()) {
        const auto event=queue.Pop(false);if (!result.empty()) result+=';';
        result+=std::to_string(event.code)+':'+std::to_string(std::bit_cast<std::uint32_t>(event.x))+':'+std::to_string(std::bit_cast<std::uint32_t>(event.y));
    }
    return result.empty() ? "-" : result;
}
}

// Guide Abort/미처리 사건960개와 실제 이름/토큰/큐42개의 최초 원문을 직접 대조한다.
TEST_CASE(MissionControl_Native1002) {
    const auto rows=LoadFixture(NETSTORM_MISSION_ABORT_FIXTURE).rows;CHECK(rows.size()==1002);
    // 각 행은 새 큐/고리를 가지며 앞선 관측의 결과를 재사용하지 않는다.
    for (const auto& row:rows) {
        if (row[0]=="A") {
            CHECK(row.size()==3);const auto fields=Split(row[1],',');CHECK(fields.size()==6);std::array<std::uint32_t,6> input{};
            // 원문 DWORD를 그대로 공급한다. 기대 분기는 이 준비 단계에서 계산하지 않는다.
            for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
            InputQueue queue;std::string trace;
            // 독립 명시 입력의 기존 활성 큐만 채운다.
            for (std::uint32_t index=0;index<input[4];++index) queue.Push(0x300U+index,static_cast<std::int32_t>(index+10),static_cast<std::int32_t>(index+20));
            const InputEvent event{input[1],std::bit_cast<std::int32_t>(input[2]),std::bit_cast<std::int32_t>(input[3])}; // 원본 세 DWORD의 비트를 보존한다.
            std::uint32_t result{};
            if (DispatchMissionAbort(event,queue,[&](std::string_view section) { trace="tell:"+Hex({reinterpret_cast<const std::uint8_t*>(section.data()),section.size()}); })) result=1;
            else if (input[0]) { trace="input:"+std::to_string(input[1])+':'+std::to_string(input[2])+':'+std::to_string(input[3]);result=input[5]; } // 미처리 사건의 명시 가상 반환 경계다.
            CHECK(std::to_string(result)+'|'+ControlQueue(queue)+'|'+(trace.empty() ? "-" : trace)==row[2]);
        } else {
            CHECK(row[0]=="S" && row.size()==5);const auto source=ControlBytes(row[1]);const auto comma=source.find(',');CHECK(comma!=source.npos);
            InputQueue queue;ScriptStringRing strings;CHECK(QueueMissionControlAction(source.substr(0,comma),source.substr(comma+1),queue,strings));
            CHECK(!queue.Empty());const auto event=queue.Pop(false);CHECK(queue.Empty() && event.x==event.y && strings.Index()==Number(row[3]));
            const auto expected=Split(row[2],',');CHECK(expected.size()==2 && event.code==Number(expected[0]));
            if (expected[1].front()=='s') CHECK(strings.Resolve(std::bit_cast<std::uint32_t>(event.x))==ControlBytes(expected[1].substr(1)));
            else CHECK(std::bit_cast<std::uint32_t>(event.x)==Number(expected[1].substr(1)));
        }
    }
}

// 보호 TryAgain/ABORT의 실제 버튼 메타데이터를 같은 parser/명령 큐에 전달한다. 창은 만들지 않는다.
TEST_CASE(MissionControl_AuthoredReplayAndAbortConfirmation) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\n[END]\n");MissionScript script(config,"tell");CHECK(script.Loaded());
    InputQueue queue;ScriptStringRing strings;std::vector<DialogPage> pages;
    const auto show=[&](std::string_view section) { const auto body=script.Section(section);CHECK(body);HtmlTextParserState state;HtmlTextParserGlobals globals;pages.push_back(ParseDialogPage(*body,state,globals,{})); }; // 실제 원문/HTML 메타 추출을 쓰며 글자 표시/월드 수명은 선택하지 않는다.
    CHECK(DispatchMissionAbort({kMissionAbortCommand,99,0},queue,show) && pages.size()==1 && queue.Empty());
    show("TryAgain");CHECK(pages.size()==2);
    // 두 실제 화면의 Replay Mission 버튼은 모두 같은 원본 MissionRestart/0을 예약한다.
    for (const auto& page:pages) {
        const auto item=std::find_if(page.items.begin(),page.items.end(),[](const DialogItem& value) { return value.label=="Replay Mission"; });CHECK(item!=page.items.end());
        CHECK(QueueMissionControlAction(item->action.command,item->action.argument,queue,strings));const auto event=queue.Pop(false);CHECK(event.code==kMissionReloadCommand && event.x==0 && event.y==0 && queue.Empty());
    }
    CHECK(DispatchMissionAbort({kMissionAbortCommand,0,1},queue,show) && pages.size()==2);const auto leave=queue.Pop(false);CHECK(leave.code==kLeaveBattleCommand && leave.x==2 && leave.y==2);
    CHECK(!QueueMissionControlAction("Unimplemented", "0",queue,strings) && queue.Empty() && strings.Index()==0);
}
