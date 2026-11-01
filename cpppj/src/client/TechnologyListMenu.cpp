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
void TechnologyListMenu::AppendAll(std::uint32_t target) const { //이미 열린 HTML 열만 사용하며 임의 팝업이나 기술 즉시 지급을 만들지 않는다.
    target&=0xffffU;const auto rawLevel=TechNeed(hooks_.level)(target);const auto level=std::bit_cast<std::int32_t>(rawLevel-1U);TechNeed(hooks_.reference)(target);
    for (std::int32_t theme=0;theme<4;++theme) { //사용하지 않는 두 NULL 수집의 조회/진단 효과도 원문 순서로 유지한다.
        std::int32_t stored{};static_cast<void>(CountCandidates({},stored,theme,level));static_cast<void>(CountKnown({},stored,theme,level));
        const auto name=TechNeed(hooks_.themeName)(theme);const auto label=TechNeed(hooks_.formatString)("~y%s",name);const auto command=TechNeed(hooks_.doNothingCommand)();TechNeed(hooks_.item)(label,command,0,false);AppendTheme(target,theme);TechNeed(hooks_.separator)();
    }
    const auto surface=TechNeed(hooks_.surface)(target);const auto parent=TechNeed(hooks_.currentTechnology)();std::int32_t known{},total{};
    for (std::int32_t index=0;index<TechNeed(hooks_.typeCount)();++index) { //전체 보유 수는 원소/레벨 필터 없이 사제와group10만 제외한다.
        const auto& type=TechNeed(hooks_.type)(static_cast<std::uint32_t>(index));if (static_cast<std::uint32_t>(index)!=TechNeed(hooks_.priestType)() && type.group!=10 && TechNeed(hooks_.owns)(parent,static_cast<std::uint32_t>(index))) ++known;
    }
    static_cast<void>(TechNeed(hooks_.currentTechnology)());
    for (std::int32_t index=0;index<TechNeed(hooks_.typeCount)();++index) { //두 번째 순회에서도 변할 수 있는 현재 TYPE/count를 재조회한다.
        const auto& type=TechNeed(hooks_.type)(static_cast<std::uint32_t>(index));if (static_cast<std::uint32_t>(index)!=TechNeed(hooks_.priestType)() && type.group!=10) ++total;
    }
    std::string label;std::uint32_t command{},parameter=target;bool enabled{}; //마지막 공통 행에만 선택한 분기의 현재 인자를 넘긴다.
    if (total<=known) { //전체 보유의 랭크 분기는 원문대로 정적 복원하며 멀티플레이 신규 검사는 하지 않는다.
        static_cast<void>(TechNeed(hooks_.rank)());const auto firstRank=TechNeed(hooks_.rank)();auto maximum=TechNeed(hooks_.maximumRank)();
        auto next=maximum<static_cast<std::int32_t>(firstRank+1U) ? TechNeed(hooks_.maximumRank)() : static_cast<std::int32_t>(TechNeed(hooks_.rank)()+1U);
        if (next<0) next=0;else { const auto secondRank=TechNeed(hooks_.rank)();maximum=TechNeed(hooks_.maximumRank)();next=maximum<static_cast<std::int32_t>(secondRank+1U) ? TechNeed(hooks_.maximumRank)() : static_cast<std::int32_t>(TechNeed(hooks_.rank)()+1U); }
        const auto pattern=TechNeed(hooks_.translate)("ADVANCE TO RANK %d!!");label=TechNeed(hooks_.formatNumber)(pattern,std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(next)+1U));command=TechNeed(hooks_.learnCommand)();parameter=TechNeed(hooks_.noIslandType)();enabled=true;
    } else if (level==2) {
        const auto pattern=TechNeed(hooks_.translate)("Altar at Max %sLevel");label=TechNeed(hooks_.formatString)(pattern,"");command=TechNeed(hooks_.doNothingCommand)();parameter=0;
    } else {
        if (level<2 && surface && surface<TechNeed(hooks_.poolCapacity)()) { const auto home=TechNeed(hooks_.homeIsland)();const auto island=TechNeed(hooks_.island)(surface);enabled=island==TechNeed(home)(); } //원문은Player 주소를 보관하고 섬getter 뒤 같은 필드를 읽는다.
        const auto pattern=TechNeed(hooks_.translate)("Upgrade Altar to Level %d");label=TechNeed(hooks_.formatNumber)(pattern,std::bit_cast<std::int32_t>(rawLevel+1U));command=TechNeed(hooks_.upgradeCommand)();
    }
    TechNeed(hooks_.item)(label,command,parameter,enabled);const auto money=TechNeed(hooks_.translate)("Get 5,000 ~[IfortGump.3]");const auto learn=TechNeed(hooks_.learnCommand)();const auto nugget=TechNeed(hooks_.nuggetType)();TechNeed(hooks_.item)(money,learn,nugget,true);
}
}
