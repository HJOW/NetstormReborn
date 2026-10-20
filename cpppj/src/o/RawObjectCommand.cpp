// 공통 객체 명령의 원본 조기 반환과 DWORD 피해 감김을 구현한다.
#include "o/RawObjectCommand.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택된 실제 하위만 요구하며 미처리 명령을 예외나 성공으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("공통 객체 명령의 실제 가상 효과 미연결");return function;
}
}
// 10.78의 HP DWORD 배치를 다른 판본에 적용하지 않는다.
RawObjectCommand::RawObjectCommand(const SidPool& pool,const ObjectCommandState& state,ObjectCommandHooks hooks)
    :pool_(pool),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("공통 객체 명령은10.78 전용");
}
// 원본은 Hurt가 제한된 모드에서도 처리1을 반환하고, 알 수 없는 명령만 Form 부모에서0을 반환한다.
std::uint32_t RawObjectCommand::Command(Sid parent,std::uint32_t code) const {
    const auto command=code&0x03ffffffU;
    if (command==state_.ownerCommand) { Need(hooks_.owner)(parent,Need(state_.localOwner)());return 1; }
    if (command==state_.hurtCommand) {
        if (Need(state_.editor)() || Need(state_.mission)()) {
            const auto raw=pool_.Slot(parent);std::uint32_t hp{};
            // 원본+1a의 비정렬 DWORD를 읽고 signed overflow 없이 DEC의32비트 감김을 보존한다.
            for (std::size_t index=0;index<4;++index) hp|=std::uint32_t(raw[26+index])<<(8*index);
            Need(hooks_.hit)(parent,std::bit_cast<std::int32_t>(hp-1U),0,Sid{});
        }
        return 1;
    }
    if (command==state_.destroyCommand) { Need(hooks_.destroy)(parent,0);return 1; }
    if (command==state_.describeCommand) { Need(hooks_.describe)(parent);return 1; }
    return 0;
}
}
