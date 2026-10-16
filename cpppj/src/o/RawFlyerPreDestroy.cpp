// +8 WORD를 DWORD나 종속 head로 바꾸지 않고 실제 두 호출 순서를 유지한다.
#include "o/RawFlyerPreDestroy.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 미복원 판본/하위 누락은 효과 전에 거부한다.
RawFlyerPreDestroy::RawFlyerPreDestroy(const SidPool& pool,FlyerPreDestroyHooks hooks):pool_(pool),hooks_(std::move(hooks)) {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || !hooks_.destroy || !hooks_.damageable) throw std::invalid_argument("플라이어 전처리 판본/실제 하위 누락");
}
// callback의 raw 변경 뒤에도 원본 this와 flags로 하위 몸체를 한 번 호출한다.
void RawFlyerPreDestroy::PreDestroy(Sid sid,std::uint32_t flags) const {
    const auto raw=pool_.Slot(sid);const Sid target{static_cast<std::uint16_t>(raw[8]|static_cast<std::uint16_t>(raw[9])<<8)};
    if (target.value) hooks_.destroy(target,0);hooks_.damageable(sid,flags);
}
}
