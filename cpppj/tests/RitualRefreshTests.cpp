// 상위 희생을 실행하지 않고 로컬 완료 분기54개 원문 관측의 순서/부분효과를 비교한다.
#include "TestSupport.h"
#include "o/RawRitualRefresh.h"
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 원문 자식의 독립 반환/현재 owner 변경만 공급한다.
using namespace netstorm::o; // 실제 제품 부모와 같은 판본/훅이다.
struct Stop {}; // 원문과 같은 효과 진입에서만 명시 중단한다.
// 원래 TSV의 모든 열을 순서대로 분리한다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find('\t',begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 마지막 열도 보존한다.
}
struct Probe { // 게임 계산을 기대값 생성기로 사용하지 않는 실제 외부 경계다.
    std::uint32_t first{},second{},stop{},events{},owner{7}; // 두 자식이 쓸 명시 owner와 현재 공유 DWORD다.
    bool core{},snapshot{};std::optional<std::uint32_t> written;std::string trace; // 실제 적용된 효과와 진입 순서다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value;if (++events==stop) throw Stop{}; } // 효과보다 먼저 관측한다.
    RitualRefreshHooks Hooks() { // 실제 부모가 각 콜백의 순서와 인수를 결정한다.
        RitualRefreshHooks h;
        h.core=[this] { Event("C");core=true;owner=first; };
        h.templateOwner=[this] { const auto value=owner;Event("O,"+std::to_string(value));return value; };
        h.coreData=[this](auto selected) { Event("D,"+std::to_string(selected));written=selected;owner=second; };
        h.snapshot=[this] { Event("S");snapshot=true; };return h;
    }
};
}

// 원문에서 동결한54개의 두 현재 load와 세 자식/각 중단 부분효과를 대조한다.
TEST_CASE(RitualRefresh_Native54CurrentOwnerAndPartialEffects) {
    std::ifstream input(NETSTORM_RITUAL_REFRESH_FIXTURE);CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { // 각 행은 새 현재 상태와 독립 경계를 사용한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto fields=Fields(line);CHECK(fields.size()==9);if (fields.size()!=9) return;
        Probe probe;probe.first=static_cast<std::uint32_t>(std::stoul(fields[0]));probe.second=static_cast<std::uint32_t>(std::stoul(fields[1]));probe.stop=static_cast<std::uint32_t>(std::stoul(fields[2]));bool completed=true;
        try { RefreshLocalRitualData(OriginalEdition::Patch1078,probe.Hooks()); }catch (const Stop&) { completed=false; }
        CHECK(completed==(fields[3]=="1") && probe.trace==fields[4]);CHECK(probe.owner==std::stoul(fields[5]) && probe.core==(fields[6]=="1") && probe.snapshot==(fields[8]=="1"));
        CHECK(probe.written.has_value()!=(fields[7]=="-"));if (probe.written) CHECK(*probe.written==std::stoul(fields[7]));++count;
    }CHECK(count==54);
}

// 도달한 기록 경계 누락은 Core/첫 현재 읽기를 보존하고 snapshot 성공으로 바꾸지 않는다.
TEST_CASE(RitualRefresh_SelectedMissingBoundaryAndCd) {
    Probe probe;probe.first=8;auto hooks=probe.Hooks();hooks.coreData={};bool missing{};
    try { RefreshLocalRitualData(OriginalEdition::Patch1078,hooks); }catch (const std::logic_error&) { missing=true; }
    CHECK(missing && probe.core && probe.owner==8 && !probe.written && !probe.snapshot && probe.trace=="C;O,8");
    probe.trace.clear();bool cd{};try { RefreshLocalRitualData(OriginalEdition::Cd1072,probe.Hooks()); }catch (const std::invalid_argument&) { cd=true; }CHECK(cd && probe.trace.empty());
}
