// 독립 원문480관측을 통해 메뉴 없는 객체 명령의 대상·뗌·반환 무시를 대조한다.
#include "TestSupport.h"
#include "client/ScenarioObjectInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 원문 TSV 필드를 값의 재계산 없이 읽는 검사 도구다.
// 탭으로 구분된 실제 입력과 관측 열을 분리한다.
std::vector<std::string> ObjectInputFields(std::string_view line) {
    std::vector<std::string> fields;std::size_t begin{};
    for (;;) { const auto end=line.find('\t',begin);fields.emplace_back(line.substr(begin,end==line.npos ? line.npos : end-begin));if (end==line.npos) return fields;begin=end+1; } // 마지막 열까지 순서대로 보존한다.
}
}

// 실제 원문 이름 다섯 개가 같은 parser/입력 큐로 숫자 SID를 전달한다.
TEST_CASE(ScenarioObjectInput_OriginalNamesUseActualScriptQueue) {
    using namespace netstorm::client;
    InputQueue queue;ScriptStringRing strings;
    for (const auto name:{"MakeMine","InflictDamage","DestroyPiece","TurnToObelisk","mElTdOwN"}) { // 대소문자는 기존 원문 비교 규칙을 따른다.
        const auto code=ScenarioObjectScriptCode(name);CHECK(code!=0xffffffffU && GameMenuScriptCode(name)==code && IsScenarioObjectCommand(code));
        QueueScriptCommand(std::string(name)+",6008",queue,strings,{GameMenuScriptCode,{}});
        const auto event=queue.Pop(false);CHECK(event.code==code && event.x==6008 && event.y==6008 && queue.Empty());
    }
    CHECK(ScenarioObjectScriptCode("Destroy")==0xffffffffU && ScenarioObjectScriptCode("MakeMin")==0xffffffffU); // 원문에 없는 별칭을 만들지 않는다.
}

// 가상 몸체의 효과를 실행하지 않고 원문 일반 입력의 전체 호출 기록을 재생한다.
TEST_CASE(ScenarioObjectInput_ReplaysNative480WithoutMenuLifetime) {
    using namespace netstorm::client;
    std::ifstream file(NETSTORM_SCENARIO_OBJECT_INPUT_FIXTURE);if (!file) throw std::runtime_error("시나리오 객체 입력 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t rows{};
    while (std::getline(file,line)) { // 앞6열만 입력으로 사용하며 뒤3열은 독립 원문 관측과 비교한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto fields=ObjectInputFields(line);if (fields.size()!=9) throw std::runtime_error("시나리오 객체 입력 fixture 열 폭");
        const auto code=static_cast<std::uint32_t>(std::stoul(fields[1])),first=static_cast<std::uint32_t>(std::stoul(fields[2])),second=static_cast<std::uint32_t>(std::stoul(fields[3])),result=static_cast<std::uint32_t>(std::stoul(fields[4]));
        const InputEvent event{code,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)};std::string trace;std::size_t calls{};
        CHECK(DispatchScenarioObjectInput(event,[&](auto sid,auto command) {
            ++calls;trace=std::to_string(sid.value)+':'+std::to_string(command)+':'+std::to_string(first)+':'+std::to_string(second)+':'+std::to_string(result);return result;
        }));
        CHECK((trace.empty() ? "-" : trace)==fields[6]);CHECK(calls<=1 && fields[8]=="0");++rows; // 팝업 조회 수는 원문 전체 입력의 근거로 보존하며 객체 분배 함수의 효과로 주장하지 않는다.
    }
    CHECK(rows==480);
}

// 입력 관문에서 호출하지 않는 하위는 메뉴/월드 없이도 요구하지 않는다.
TEST_CASE(ScenarioObjectInput_OnlyReachedTargetRequiresCurrentWorld) {
    using namespace netstorm::client;
    CHECK(!DispatchScenarioObjectInput({0x02000017U,0,50},{}));
    for (const auto name:{"MakeMine","InflictDamage","DestroyPiece","TurnToObelisk","Meltdown"}) { // 모든 공통 명령에 같은 앞단이 적용된다.
        const auto code=ScenarioObjectScriptCode(name);
        CHECK(DispatchScenarioObjectInput({code|InputCode::kRelease,0,50},{}));CHECK(DispatchScenarioObjectInput({code,0,0},{}));CHECK(DispatchScenarioObjectInput({code,0,65536},{}));
        bool missing{};try { static_cast<void>(DispatchScenarioObjectInput({code,0,50},{})); }catch (const std::logic_error&) { missing=true; }CHECK(missing);
    }
}
