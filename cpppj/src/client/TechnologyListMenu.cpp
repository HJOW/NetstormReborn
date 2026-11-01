// 원문은 고정40칸을 넘는 후보도 세고 목록 순회 때 저장된 개수의 나머지로 다시 읽는다.
#include "client/TechnologyListMenu.h"
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client { //가득 찬 목록을 확장하거나 이미 가진 기술 행을 숨기지 않는다.
namespace { //도달한 실제 하위만 요구한다.
template<class F> const F& TechNeed(const F& hook) { if (!hook) throw std::logic_error("기술 목록의 실제 장치 미연결");return hook; } //선행 수집/GUI 효과를 빈 성공으로 숨기지 않는다.
}
TechnologyListMenu::TechnologyListMenu(TechnologyListMenuHooks hooks):hooks_(std::move(hooks)) {} //구성만으로 게임 상태나 현재 메뉴를 바꾸지 않는다.
std::int32_t TechnologyListMenu::Count(std::span<std::uint32_t> output,std::int32_t& stored,std::int32_t theme,std::int32_t level,bool known) const { //theme5는 레벨도 무시하는 원문의 전체 원소 관문이다.
    if ((level<0 || level>2) && level!=-1) TechNeed(hooks_.assertion)(known ? 0xb94U : 0xbadU);
    const auto parent=TechNeed(hooks_.currentTechnology)();std::int32_t count{};
    for (std::int32_t index=0;index<TechNeed(hooks_.typeCount)();++index) { //소유 조회 뒤 바뀐 현재 count를 다음 조건에서 읽는다.
        const auto& type=TechNeed(hooks_.type)(static_cast<std::uint32_t>(index));
        if (static_cast<std::uint32_t>(index)==TechNeed(hooks_.priestType)() || type.group==10 || (theme!=5 && (type.theme!=theme || type.level!=level))) continue;
        if (known && !TechNeed(hooks_.owns)(parent,static_cast<std::uint32_t>(index))) continue;
        if (!output.empty() && stored<static_cast<std::int32_t>(output.size())) { if (stored<0) throw std::out_of_range("기술 목록의 음수 저장 인덱스");output[static_cast<std::size_t>(stored++)]=static_cast<std::uint32_t>(index); }
        ++count;
    }
    return count;
}
std::int32_t TechnologyListMenu::CountKnown(std::span<std::uint32_t> output,std::int32_t& stored,std::int32_t theme,std::int32_t level) const { return Count(output,stored,theme,level,true); } //실제 소유 종류WORD가 비0인 원소만 센다.
std::int32_t TechnologyListMenu::CountCandidates(std::span<std::uint32_t> output,std::int32_t& stored,std::int32_t theme,std::int32_t level) const { return Count(output,stored,theme,level,false); } //모든 후보를 세며 미선택 소유 장치는 요구하지 않는다.
void TechnologyListMenu::AppendTheme(std::uint32_t target,std::int32_t theme) const { //타입 번호가 아닌 실제 대상 SID의 가상 레벨이다.
    const auto level=std::bit_cast<std::int32_t>(TechNeed(hooks_.level)(target)-1U);std::array<std::uint32_t,40> known{},candidates{};std::int32_t knownStored{},candidateStored{}; //원문은 각0xa0BYTE를 따로 확보한다. 호스트 고정 배열은 내용 없는 물리를 읽지 않는다.
    static_cast<void>(CountKnown(known,knownStored,theme,level));const auto count=CountCandidates(candidates,candidateStored,theme,level);const auto parent=TechNeed(hooks_.currentTechnology)();
    for (std::int32_t current=0;current<=level;++current) { //음수 level이면 행을 만들지 않으며 앞선 두 helper 효과는 유지한다.
        for (std::int32_t index=0;index<count;++index) { //원문 전체 후보 수만큼 방문하고 고정 저장 개수로 나머지를 구한다.
            const auto number=candidateStored ? candidates[static_cast<std::size_t>(index%candidateStored)] : 0U;const auto& type=TechNeed(hooks_.type)(number);
            if (type.level!=current || number==TechNeed(hooks_.templeProduct)()) continue;
            const auto owned=TechNeed(hooks_.owns)(parent,number);const auto pattern=TechNeed(hooks_.translate)("%s (Level %d)");const auto displayLevel=type.level+1;const auto description=TechNeed(hooks_.description)(number);const auto label=TechNeed(hooks_.format)(pattern,description,displayLevel); //원문은 설명 getter보다 먼저 현재 level+1을 push한다.
            const auto command=TechNeed(hooks_.learnCommand)();TechNeed(hooks_.item)(label,command,number,owned==0);
        }
        if (current==std::numeric_limits<std::int32_t>::max()) break; //정상 실제 가상 레벨의 한계를 넘는 호스트 signed 증가만 명시 종료한다.
    }
}
}
