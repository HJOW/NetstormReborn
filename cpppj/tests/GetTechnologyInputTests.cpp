// 실제 GetTechnology 입력48관측과 이름표/큐를 대조하고 기존 기술 획득 부모를 재사용한다.
#include "TestSupport.h"
#include "client/ThemeTechnologyCommand.h"
#include "client/GameMenuCommands.h"
#include "client/RawKnowledgeMenu.h"
#include "client/ScriptCommand.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 입력/관측 열을 분리해서 읽는 검사 전용 도구다.
// 빈 열도 원문 TSV의 물리 순서대로 분해한다.
std::vector<std::string> TechnologyInputFields(std::string_view line) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=line.find('\t',begin);result.emplace_back(line.substr(begin,end==line.npos ? line.npos : end-begin));if (end==line.npos) return result;begin=end+1; } // 현재 필드만 읽으며 기대 인자를 계산하지 않는다.
}
}

// 원본 두 이름이 실제 parser/입력 큐에서 기술 번호와 SP 숫자를 보존한다.
TEST_CASE(GetTechnologyInput_TwoOriginalNamesAndQueue) {
    using namespace netstorm::client;
    InputQueue queue;ScriptStringRing strings;
    const auto lookup=[](auto name) { const auto knowledge=KnowledgeScriptCode(name);return knowledge!=0xffffffffU ? knowledge : GameMenuScriptCode(name); }; // 실제 DialogScriptCode와 같은 기술→일반 메뉴 조회 순서다.
    QueueScriptCommand("GetTechnology,97",queue,strings,{lookup,{}});QueueScriptCommand("cHaNgEmOnEy,20000",queue,strings,{lookup,{}});
    const auto technology=queue.Pop(false),money=queue.Pop(false);
    CHECK(technology.code==0x0200009fU && technology.x==97 && technology.y==97);CHECK(money.code==0x02000061U && money.x==20000 && money.y==20000 && queue.Empty());
    CHECK(IsGameMenuCommand(technology.code) && IsGameMenuCommand(money.code));CHECK(lookup("GetTechnolgy")==0xffffffffU); // 원문에 없는 오자는 별칭으로 고치지 않는다.
}

// 원문 일반 입력/실제 thunk48관측과 DWORD 전달·release·무관한 첫 인자를 비교한다.
TEST_CASE(GetTechnologyInput_Native48AndSelectedEffects) {
    using namespace netstorm::client;
    std::ifstream file(NETSTORM_GET_TECHNOLOGY_INPUT_FIXTURE);if (!file) throw std::runtime_error("기술 지급 입력 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t rows{};
    while (std::getline(file,line)) { // 앞6입력 열만 제품에 전달하고 뒤 관측을 직접 비교한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=TechnologyInputFields(line);if (row.size()!=9) throw std::runtime_error("기술 지급 입력 fixture 열 폭");
        const InputEvent event{static_cast<std::uint32_t>(std::stoul(row[1])),std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(std::stoul(row[2]))),std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(std::stoul(row[3])))};
        std::string trace;std::size_t calls{};
        CHECK(DispatchGetTechnologyInput(event,[&](auto type) { ++calls;trace="technology:"+std::to_string(type); }));CHECK((trace.empty() ? "-" : trace)==row[6]);CHECK(calls==((event.code&InputCode::kRelease) ? 0U : 1U));CHECK(row[7]=="1" && row[8]=="0");++rows;
    }
    CHECK(rows==48);CHECK(!DispatchGetTechnologyInput({0x0200009aU,1,97},{}));CHECK(DispatchGetTechnologyInput({0x8200009fU,1,97},{}));
    bool missing{};try { static_cast<void>(DispatchGetTechnologyInput({0x0200009fU,1,97},{})); }catch (const std::logic_error&) { missing=true; }CHECK(missing); // 선택된 부모 누락을 지급 성공으로 바꾸지 않는다.
}
