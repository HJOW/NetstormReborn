// SP 메뉴 명령144개 원문 관측의 현재 관문/float비트/중단 부분효과를 비교한다.
#include "TestSupport.h"
#include "client/GameMenuActions.h"
#include "client/GameMenuCommands.h"
#include <bit>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace { // 원래 일반 분배 뒤의 SP 분기 입력만 명시한다.
using namespace netstorm::client; // 실제 제품의 같은 일반 메뉴 처리기를 사용한다.
struct Stop {}; // 지정한 현재 load/가산 진입에서 중단한다.
// 원래 TSV 열의 순서와 마지막 열을 보존한다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 각 구분자의 원시 경계다.
}
struct Probe { // 게임 금액 계산 대신 실제 외부 효과 진입과 받은 float비트만 읽는다.
    std::uint32_t editor{},mission{},stop{},events{},hide{},redraw{}; // 독립 입력과 기존 처리기가 빌리는 실제 쓰기 전역이다.
    std::optional<std::uint32_t> added;std::string trace; // 실제 가산 효과와 호출 순서다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value;if (++events==stop) throw Stop{}; } // 효과를 적용하기 전 진입을 기록한다.
    GameMenuActions Actions(bool supplyAdd=true) { // 선택되지 않은 메뉴 자식은 설치하지 않는다.
        GameMenuActionHooks h;h.command=[](auto command) { const auto value=GameMenuCommandValue(static_cast<std::uint32_t>(command));if (!value) throw std::logic_error("검사 메뉴 명령 없음");return *value; };
        if (supplyAdd) h.addLocalSp=[this](float value) { const auto bits=std::bit_cast<std::uint32_t>(value);Event("A,"+std::to_string(bits));added=bits; };
        return GameMenuActions({[this](auto address) { if (address==0x5c85a4U) { Event("E,"+std::to_string(editor));return editor; }if (address==0x594fa4U) { Event("M,"+std::to_string(mission));return mission; }throw std::logic_error("검사 메뉴의 예상밖 전역"); },hide,redraw},std::move(h));
    }
};
}
// 동일 보호 PE에서 동결한 입력/전체 trace/실제 인수 비트를144개 대조한다.
TEST_CASE(CheatMoney_Native144CurrentGatesSignedFloatAndPartialEffects) {
    std::ifstream input(NETSTORM_CHEAT_MONEY_FIXTURE);CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { // 각 사례마다 독립 상태를 생성해 앞선 효과를 섞지 않는다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto f=Fields(line);CHECK(f.size()==7);if (f.size()!=7) return;
        Probe p;p.editor=static_cast<std::uint32_t>(std::stoul(f[0]));p.mission=static_cast<std::uint32_t>(std::stoul(f[1]));p.stop=static_cast<std::uint32_t>(std::stoul(f[3]));bool completed=true;
        try { const auto result=p.Actions().Handle(0x02000061U,0,static_cast<std::uint32_t>(std::stoll(f[2])));CHECK(result && *result==0); }catch (const Stop&) { completed=false; }
        CHECK(completed==(f[4]=="1") && p.trace==f[5]);CHECK(p.added.has_value()!=(f[6]=="-"));if (p.added) CHECK(*p.added==std::stoul(f[6]));CHECK(!p.hide && !p.redraw);++count;
    }CHECK(count==144);
}
// release는 기존 상위 일반 분배 계약대로 효과 없이 반환하고 누락 가산은 앞선 관문을 보존한다.
TEST_CASE(CheatMoney_ReleaseAndMissingAddPreserveGateReads) {
    Probe p;p.mission=1;const auto released=p.Actions(false).Handle(0x82000061U,0,10000);CHECK(released && *released==0 && p.trace.empty());
    bool missing{};try { static_cast<void>(p.Actions(false).Handle(0x02000061U,0,10000)); }catch (const std::runtime_error&) { missing=true; }CHECK(missing && p.trace=="E,0;M,1" && !p.added);
}
