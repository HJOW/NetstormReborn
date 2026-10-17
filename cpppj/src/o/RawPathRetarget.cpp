// 대상이 살아 있으면 새 이동/내용물 조회 없이 현재 경로를 계속 진행한다.
#include "o/RawPathRetarget.h"
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr std::uint32_t kNuggetMask=0x20; // 원본의 내용물 mask이며 타입 번호와 혼동하지 않는다.
// 선택된 큰 하위가 없으면 성공이나 빈 동작으로 대체하지 않는다.
template<class F> const F& Need(const F& f) { if (!f) throw std::logic_error("경로 대상 재확인의 실제 하위 미연결");return f; }
// 정렬되지 않은 현재 Process 부모 DWORD를 원본 폭으로 읽는다.
std::uint32_t Parent(const RawPathRuntime& runtime) {
    const auto raw=runtime.Bytes();std::uint32_t value{};
    // 원본 base+8의 네 바이트만 읽는다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[8+i])<<(i*8);return value;
}
}
// CD의 다른 Halt 본문을 PATCH 구현으로 대신 실행하지 않는다.
RawPathRetarget::RawPathRetarget(const SidPool& pool,const std::uint32_t& nuggetType,PathRetargetHooks hooks)
    :pool_(pool),nuggetType_(nuggetType),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("경로 대상 재확인의 판본 오류");
}
//0번도 실제 물리 슬롯이며 주소 계산만 했던 무효 SID를 미리 조회하지 않는다.
Sid RawPathRetarget::Number(std::uint32_t value) const { if (value>=pool_.Capacity() || value>0xffffU) throw std::out_of_range("경로 대상 재확인의 물리 SID 범위 오류");return Sid{static_cast<std::uint16_t>(value)}; }
// 콜백이 대상 번호를 바꾸어도 최초 대상 포인터의 현재 상태를 읽는다.
bool RawPathRetarget::Dead(std::uint32_t value) const { return (pool_.Slot(Number(value))[11]&2U)!=0; }
// SetTargets는 다른 필드의 현재 값을 그대로 보존한다. 새 이동 호출 뒤에는 Runtime을 다시 읽지 않는다.
void RawPathRetarget::Update(RawPathRuntime& runtime,Sid initialParent) const {
    if (runtime.Bytes().size()!=RawPathRuntime::Layout(OriginalEdition::Patch1078).size) throw std::invalid_argument("경로 대상 재확인의 Runtime 판본 오류");
    const auto savedReturn=runtime.TargetB(),savedGeyser=runtime.TargetA(),currentParent=Parent(runtime); // 두 대상 포인터와 현재 부모는 첫 Find 전에 보관한다.
    if (runtime.TargetB()!=0 && Dead(savedReturn) && Need(hooks_.find)(Number(currentParent),kNuggetMask,nuggetType_,0).value) {
        const auto next=Need(hooks_.chooseReturn)(Number(currentParent));runtime.SetTargets(runtime.TargetA(),next.value);
        if (next.value) Need(hooks_.goReturn)(Number(currentParent),next);return;
    }
    if (runtime.TargetA()!=0 && Dead(savedGeyser) && !Need(hooks_.find)(initialParent,kNuggetMask,nuggetType_,0).value) {
        const auto next=Need(hooks_.chooseGeyser)(Number(currentParent));runtime.SetTargets(next.value,runtime.TargetB());
        if (next.value) { runtime.SetTargets(runtime.TargetA(),0);Need(hooks_.goGeyser)(Number(currentParent),next); }
    }
}
// 같은 실제 풀만 제공한다.
const SidPool& RawPathRetarget::Pool() const { return pool_; }
}
