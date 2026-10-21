// 원본 signed 레벨 나머지와 비용의 float 축소·unordered 자금 관문을 보존한다.
#include "o/RawFactoryInput.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택되지 않은 하위는 요구하지 않으며 누락된 효과를 빈 성공으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Factory 실제 입력 하위 미연결");return function; }
constexpr std::uint32_t kCommandMask=0x03ffffffU; //원본 입력의 상위6개 수정 비트를 제외한다.
}
// 가상 입력은 호출할 때 실행하며 생성 시 장치/게임 효과를 일으키지 않는다.
RawFactoryInput::RawFactoryInput(const SidPool& pool,const FactoryInputState& state,FactoryInputHooks hooks)
    :pool_(pool),state_(state),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Factory 입력은 같은 PATCH 풀만 지원합니다"); }
// 편집기에서도 먼저SP를읽고 진행중검사는 자금관문을 통과한 뒤에만 한다.
std::uint32_t RawFactoryInput::Input(Sid parent,std::uint32_t code,std::uint32_t packedArgument) const {
    const auto command=code&kCommandMask;
    if (command==state_.cycleCommand) { const auto next=std::bit_cast<std::int32_t>(Need(hooks_.level)(parent)+1U)%3;Need(hooks_.setLevel)(parent,std::bit_cast<std::uint32_t>(next));Need(hooks_.adjustFrame)(parent);return 1; }
    if (command==state_.upgradeCommand) {
        const auto cost=static_cast<float>(Need(hooks_.cost)(pool_.Slot(parent)[10]));const auto money=Need(hooks_.money)(state_.localOwner);
        if (!state_.editor && !(cost<=money)) { Need(hooks_.tell)("upgradeNoBucks");return 1; }
        if (Need(hooks_.upgrading)(parent)) { Need(hooks_.tell)("alreadyUpgrading");return 1; }
        Need(hooks_.request)(parent);return 1;
    }
    if (command==state_.researchCommand) return Need(hooks_.research)(parent,code,packedArgument);
    return Need(hooks_.parent)(parent,code,packedArgument);
}
// 호출자가 같은 실제 raw 풀을 사용하는지 확인한다.
const SidPool& RawFactoryInput::Pool() const { return pool_; }
}
