// 새 원문48입력과 기존 큐를 사용하여 실제 이름 조회·문자열 수명·여섯 음향 인자를 대조한다.
#include "TestSupport.h"
#include "client/GameMenuActions.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 대조용 원시 문자열/필드 해석만 제공하며 제품의 효과를 계산하지 않는다.
// NUL을 포함한 원본 바이트를 치환 없이 복원한다.
std::string BindingBytes(std::string_view text) {
    if (text=="-") return {};
    std::string result;
    for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16)); // 두 문자를 한 BYTE로 읽는다.
    return result;
}
// 마지막 빈 열까지 실제 TSV를 분해한다.
std::vector<std::string> BindingFields(std::string_view text) {
    std::vector<std::string> fields;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);fields.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return fields;begin=end+1; } // 관측 열을 제품 입력으로 계산하지 않는다.
}
}

// 원문 이름표와 실제 parser/고리/입력 큐를 함께 통과하며 숫자 인자와 문자열을 구별한다.
TEST_CASE(ScriptPresentation_FourOriginalNamesReachQueue) {
    using namespace netstorm::client;
    const std::array<std::array<std::string_view,3>,4> cases{{{"pLaYsOuNd","somethingCreated.wav","33554586"},{"VIEWPRIEST","2","33554616"},{"SetAllBridge","1","33554646"},{"DigUp","15026","33554477"}}}; // 원문 PE에서 동결한 네 실제 명령 ID다.
    InputQueue queue;ScriptStringRing strings;
    for (const auto& entry:cases) { // 대소문자만 원문 ASCII 규칙으로 처리하고 인자를 그대로 큐에 넣는다.
        QueueScriptCommand(std::string(entry[0])+','+std::string(entry[1]),queue,strings,{GameMenuScriptCode,[](auto) { throw std::runtime_error("원문에 있는 이름 조회 실패"); }});
        CHECK(!queue.Empty());const auto event=queue.Pop(false);CHECK(event.code==std::stoul(std::string(entry[2])) && event.x==event.y);
        if (entry[0]=="pLaYsOuNd") CHECK(strings.Resolve(static_cast<std::uint32_t>(event.y))==entry[1]);else CHECK(static_cast<std::uint32_t>(event.y)==std::stoul(std::string(entry[1])));
    }
    CHECK(queue.Empty() && GameMenuScriptCode("Movedialogbox")==0xffffffffU); // 원본에 없는 오자는 임의 호환 별칭으로 만들지 않는다.
}

// 실제 일반 입력 본체의48관측과 원문 고정0다섯개·release 무효과·모드 무관성을 비교한다.
TEST_CASE(ScriptPresentation_PlaySoundNative48) {
    using namespace netstorm::client;
    std::ifstream file(NETSTORM_SCRIPT_SOUND_FIXTURE);if (!file) throw std::runtime_error("PlaySound 원문 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t rows{};
    while (std::getline(file,line)) { // 새 원문 관측의 입력6열만 현재 제품의 입력으로 쓴다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=BindingFields(line);if (row.size()!=9) throw std::runtime_error("PlaySound fixture 열 폭");
        const auto event=static_cast<std::uint32_t>(std::stoul(row[1]));const auto raw=BindingBytes(row[3]);ScriptStringRing strings;const auto token=strings.Store(raw);
        std::uint32_t hide=17,redraw=29;std::size_t reads{},sounds{};std::string trace;
        GameMenuActionHooks hooks;hooks.command=[](auto command) { return GameMenuCommandValue(static_cast<std::uint32_t>(command)).value(); };
        hooks.text=[&](auto selected) { ++reads;CHECK(selected==token);return strings.Resolve(selected); };
        hooks.sound=[&](auto name,auto a,auto b,auto c,auto d,auto e) { ++sounds;trace="sound:"+std::string(name)+':'+std::to_string(a)+':'+std::to_string(b)+':'+std::to_string(c)+':'+std::to_string(d)+':'+std::to_string(e); };
        GameMenuActions actions({[](auto)->std::uint32_t { throw std::runtime_error("PlaySound에 없는 모드/소유자 조회"); },hide,redraw},std::move(hooks));
        const auto result=actions.Handle(event,static_cast<std::uint32_t>(std::stoul(row[2])),token);
        CHECK(result && *result==0 && row[7]=="1" && row[8]=="0");CHECK((trace.empty() ? "-" : trace)==BindingBytes(row[6]));CHECK(reads==sounds && sounds==((event&0x80000000U) ? 0U : 1U));CHECK(hide==17 && redraw==29);++rows;
    }
    CHECK(rows==48);
}
