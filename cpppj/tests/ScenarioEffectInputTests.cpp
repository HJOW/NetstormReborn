// 대본 표시 입력576관측과 실제 이름/큐·장치 호출 순서를 대조한다.
#include "TestSupport.h"
#include "client/ScenarioEffectInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 독립 원문 관측의 필드만 복원한다.
// TSV의 입력/관측 열을 순서대로 읽는다.
std::vector<std::string> EffectInputFields(std::string_view line) {
    std::vector<std::string> fields;std::size_t begin{};
    for (;;) { const auto end=line.find('\t',begin);fields.emplace_back(line.substr(begin,end==line.npos ? line.npos : end-begin));if (end==line.npos) return fields;begin=end+1; } // 빈 마지막 열도 유지한다.
}
}

// 보호 PE의 세 이름은 실제 parser/입력 큐를 공유한다.
TEST_CASE(ScenarioEffectInput_OriginalNamesAndQueue) {
    using namespace netstorm::client;
    for (const auto name:{"StartSmoke","Lightning","cReAtErAnDoMgUnS"}) { // 이름의 ASCII 대소문자를 무시한다.
        const auto code=ScenarioEffectScriptCode(name);CHECK(code!=0xffffffffU && GameMenuScriptCode(name)==code && IsScenarioEffectCommand(code));
        InputQueue queue;ScriptStringRing strings;QueueScriptCommand(std::string(name)+",6008",queue,strings,{GameMenuScriptCode,{}});
        const auto event=queue.Pop(false);CHECK(event.code==code && event.x==6008 && event.y==6008 && queue.Empty());
    }
    CHECK(ScenarioEffectScriptCode("Smoke")==0xffffffffU && ScenarioEffectScriptCode("CreateRandomGun")==0xffffffffU);
}

// 원문과 같은 생성 인자/무동작·폭 보관/기술 조회 순서를 비교한다.
TEST_CASE(ScenarioEffectInput_ReplaysNative576AndCapturedWidth) {
    using namespace netstorm::client;
    std::ifstream file(NETSTORM_SCENARIO_EFFECT_INPUT_FIXTURE);if (!file) throw std::runtime_error("시나리오 표시 입력 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t rows{};
    while (std::getline(file,line)) { // 앞6열만 입력으로 쓰고 뒤3열은 실제 원문 결과와 비교한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto fields=EffectInputFields(line);if (fields.size()!=9) throw std::runtime_error("시나리오 표시 입력 열 폭");
        const auto code=static_cast<std::uint32_t>(std::stoul(fields[1])),second=static_cast<std::uint32_t>(std::stoul(fields[2])),technology=static_cast<std::uint32_t>(std::stoul(fields[4]));
        std::int32_t width=std::stoi(fields[3]);std::string trace;ScenarioEffectInputHooks hooks;
        const auto append=[&](std::string entry) { if (!trace.empty()) trace+=';';trace+=entry; }; // 실제 장치 호출 순서만 문자열로 기록한다.
        hooks.width=[&] { return width; };hooks.technology=[&] { append("technology");width=1920;return technology; };
        hooks.smoke=[&](auto parent,auto x,auto y) { append("smoke:"+std::to_string(parent)+':'+std::to_string(x)+':'+std::to_string(y)); };
        hooks.lightning=[&](auto parent,auto from,auto to,float life) { append("lightning:"+std::to_string(parent)+':'+std::to_string(from[0])+':'+std::to_string(from[1])+':'+std::to_string(to[0])+':'+std::to_string(to[1])+':'+std::to_string(std::bit_cast<std::uint32_t>(life))); };
        CHECK(DispatchScenarioEffectInput({code,305419896,std::bit_cast<std::int32_t>(second)},hooks));CHECK((trace.empty() ? "-" : trace)==fields[6]);CHECK(fields[7]=="1" && fields[8]=="0");++rows;
    }
    CHECK(rows==576);
}

// 원문에서 도달하지 않는 장치는 요구하지 않고, 선택된 부모 누락은 그대로 실패한다.
TEST_CASE(ScenarioEffectInput_ReleaseNoopAndReachedMissingDevice) {
    using namespace netstorm::client;
    CHECK(!DispatchScenarioEffectInput({0x02000017U,0,0},{}));CHECK(DispatchScenarioEffectInput({ScenarioEffectScriptCode("CreateRandomGuns"),0,0},{}));
    for (const auto name:{"StartSmoke","Lightning","CreateRandomGuns"}) CHECK(DispatchScenarioEffectInput({ScenarioEffectScriptCode(name)|InputCode::kRelease,0,6008},{})); // 모든 뗌은 원문 공통 관문에서 끝난다.
    for (const auto name:{"StartSmoke","Lightning"}) { // 누름만 실제 표시 생성/현재 조회를 요구한다.
        bool missing{};try { static_cast<void>(DispatchScenarioEffectInput({ScenarioEffectScriptCode(name),0,0},{})); }catch (const std::logic_error&) { missing=true; }CHECK(missing);
    }
}
