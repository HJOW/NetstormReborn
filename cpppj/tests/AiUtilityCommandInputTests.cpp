// 원문 일반 입력/이름/FIFO와 세 하위 경계의 뗌·실패 순서를 창 없이 확인한다.
#include "RawSceneSupport.h"
#include "client/AiUtilityCommandInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include <bit>
#include <filesystem>
using namespace netstorm::client; // 실제 원문 이름과 명령/FIFO를 공유한다.
using namespace netstorm::test::rawscene; // 독립 fixture의 원시 숫자/열 해독만 공유한다.

namespace { // 새 fixture 위치와 이름 인자 BYTE만 해독한다.
FixtureData AiUtilityRows(std::string_view name) { return LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures"/name).string().c_str()); } // 실제 소스 경로로 읽어 다른 CWD에서도 같은 고정 자료를 사용한다.
std::string AiUtilityText(std::string_view hex) { if (hex=="-") return {};std::string result;for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16));return result; } // 원문 BYTE를 UTF8 치환하지 않는다.
}

// 실제 선택된 call과 param2 전체DWORD를 modifier/outside/release마다 비교한다.
TEST_CASE(AiUtilityCommandInput_Native72SelectedParentsAndRelease) {
    std::size_t count{};
    for (const auto& row:AiUtilityRows("ai-utility-commands-x86.tsv").rows) { // 기대 trace는 원문 기계어에서만 읽는다.
        CHECK(row.size()==4);if (row.size()!=4) continue;std::string trace;AiUtilityCommandHooks hooks;hooks.initialGeysers=[&] { trace="Geysers"; };hooks.crankMoney=[&] { trace="Money"; };hooks.parameter=[&](auto value) { trace="Param:"+std::to_string(value); };
        CHECK(DispatchAiUtilityCommandInput({Number(row[0]),37,std::bit_cast<std::int32_t>(Number(row[1]))},hooks));CHECK((trace.empty()?"-":trace)==row[2]);++count;
    }
    CHECK(count==72);CHECK(!DispatchAiUtilityCommandInput({'X',0,0},{}));CHECK(DispatchAiUtilityCommandInput({kBrainCrankMoneyCommand|InputCode::kRelease,0,0},{}));
    CHECK(Throws([&] { DispatchAiUtilityCommandInput({kCreateInitialGeysersCommand,0,0},{}); }));std::size_t calls{};AiUtilityCommandHooks hooks;hooks.parameter=[&](auto value) { CHECK(value==0x80000000U);++calls;throw std::runtime_error("명시 실제 본체 writer 실패"); };CHECK(Throws([&] { DispatchAiUtilityCommandInput({kAiParamSetCommand,123,std::bit_cast<std::int32_t>(0x80000000U)},hooks); }));CHECK(calls==1);
}

// 실제 이름 lookup과 문자열 고리/FIFO가 같은 세 명령값에 도달한다.
TEST_CASE(AiUtilityCommandInput_Native30NamesAndRealQueue) {
    std::size_t count{};
    for (const auto& row:AiUtilityRows("ai-utility-names-x86.tsv").rows) { // 원래/대소문자·숫자/문자열 인자와 미등록을 모두 유지한다.
        CHECK(row.size()==9);if (row.size()!=9) continue;InputQueue queue;ScriptStringRing strings;std::size_t diagnostics{};
        QueueScriptCommand(AiUtilityText(row[1]),queue,strings,{[](auto name) { CHECK(GameMenuScriptCode(name)==HtmlScriptCommandValue(name));return GameMenuScriptCode(name); },[&](auto) { ++diagnostics; }});CHECK(strings.Index()==Number(row[7]));
        if (row[6]=="-") CHECK(queue.Empty() && diagnostics);else { const auto expected=Split(row[6],',');const auto event=queue.Pop(false);CHECK(expected.size()==2 && event.code==Number(expected[0]) && event.x==event.y && queue.Empty() && !diagnostics && IsGameMenuCommand(event.code));if (expected.size()==2) { if (expected[1].starts_with('n')) CHECK(std::bit_cast<std::uint32_t>(event.y)==Number(expected[1].substr(1)));else CHECK(strings.Resolve(std::bit_cast<std::uint32_t>(event.y))==AiUtilityText(expected[1].substr(1))); } }++count;
    }
    CHECK(count==30);CHECK(GameMenuCommandValue(0x5429a8U)==kCreateInitialGeysersCommand && GameMenuCommandValue(0x5429f8U)==kBrainCrankMoneyCommand && GameMenuCommandValue(0x5428d8U)==kAiParamSetCommand);
}
