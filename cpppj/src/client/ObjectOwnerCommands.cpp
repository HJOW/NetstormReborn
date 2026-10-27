// 선택된 하위가 없으면 앞선 효과를 보존한 채 실패하며 임의 성공을 만들지 않는다.
#include "client/ObjectOwnerCommands.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //10.78 일반 입력의 실제 소비 순서를 유지한다.
namespace { // 선택한 원문 하위의 지연 계약이다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("객체 소유자 명령의 실제 하위 미연결");return hook; } // 미선택 효과는 요구하지 않는다.
}
// release는 일반 입력의 앞선 거부를 반영하고, 알 수 없는 명령만 caller에 돌려준다.
bool DispatchObjectOwner(const InputEvent& event,std::uint32_t& revision,const ObjectOwnerCommandHooks& hooks) {
    if ((event.code&0x03ffffffU)!=kSetObjectOwnerCommand) return false;
    if (event.code&InputCode::kRelease) return true;
    if (Need(hooks.editor)() || Need(hooks.missionPost)()) {
        const auto selected=Need(hooks.selected)(); // 원문 ESI에 보관하므로 가상74 뒤 현재 selected로 바꾸지 않는다.
        if (selected) {
            Need(hooks.owner)(selected,std::bit_cast<std::uint32_t>(event.y));
            if (Need(hooks.editor)()) Need(hooks.follow)(selected);
        }
        ++revision;
    }
    return true;
}
// 0번/예약/free/dead도 원문 물리 안이면 읽으며 유효한 플레이어 범위는 실제 선택 하위가 판정한다.
void FollowEditorObjectOwner(const o::SidPool& pool,std::uint32_t sid,std::uint32_t& revision,const std::function<void(std::uint32_t,std::uint32_t)>& selectCore) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || sid>=pool.Capacity() || sid>65535U) throw std::out_of_range("편집 객체 owner 조회의10.78 물리 SID 범위");
    const auto owner=pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)})[34];
    if (owner) { Need(selectCore)(owner,1);++revision; }
}
}
