// 반복마다 현재 목록과 상태를 다시 읽는 원본41e100의 순서를 보존한다.
#include "o/RawBombEffectTimer.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택되지 않은 큰 호출은 요구하지 않으며 선택된 누락은 명시 오류로 남긴다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("상태 효과 타이머의 실제 하위 미연결");return function;
}
}
RawBombEffectTimer::RawBombEffectTimer(SidPool& pool,std::span<const RiftTypeRecord> types,const double& now,BombEffectTimerHooks hooks)
    :pool_(pool),types_(types),now_(now),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("상태 효과 타이머 PATCH 풀/188TYPE 필요");
}
// 원본의 signed 지연을 현재 double 시각에 더하며 대상 중복이나 inactive 상태를 미리 제외하지 않는다.
BombEffectTimerFields RawBombEffectTimer::Construct(std::int32_t delay,std::span<const std::uint32_t> members,std::uint32_t flags) const {
    BombEffectTimerFields result;result.members.reserve(100);result.endTime=static_cast<double>(delay)+now_;result.members.assign(members.begin(),members.begin()+std::min<std::size_t>(100,members.size()));result.flags=flags;return result;
}
Sid RawBombEffectTimer::Target(std::uint32_t member) const {
    if (member>=pool_.Capacity() || member>65535) throw std::out_of_range("상태 효과 목록의 물리 SID 범위");return {static_cast<std::uint16_t>(member)};
}
// 첫 순회와 만료 순회는 독립적으로 목록을 읽으며 콜백이 바꾼 기한/flags/대상 상태를 보존한다.
void RawBombEffectTimer::Frame(BombEffectTimerFields& fields,const std::function<void()>& finish) const {
    for (std::size_t index=0;index<fields.members.size();++index) { // 첫 갱신은 만료 여부와 관계없이 모든 현재 live 대상을 방문한다.
        const auto target=Target(fields.members[index]);if (pool_.Slot(target)[11]&7U) continue;
        Need(hooks_.unpop)(target,0);Need(hooks_.repop)(target,0x40);
    }
    if (!(fields.endTime<=now_)) return;
    for (std::size_t index=0;index<fields.members.size();++index) { // 만료 뒤 목록을 다시 읽으며 같은 대상을 중복 방문할 수 있다.
        const auto target=Target(fields.members[index]);if (pool_.Slot(target)[11]&7U) continue;
        if ((fields.flags&1U) && (pool_.Slot(target)[40]&0x10U)) {
            const auto current=pool_.Slot(target);const_cast<std::uint8_t*>(current.data())[40]&=0xefU;Need(hooks_.refresh)(target); // 검증한 물리 슬롯에 원본처럼 직접 쓰며 예약 슬롯/콜백 후 상태를 재필터하지 않는다.
        }
        if ((fields.flags&2U) && (pool_.Slot(target)[40]&0x20U)) {
            const auto current=pool_.Slot(target);const_cast<std::uint8_t*>(current.data())[40]&=0xdfU;const auto type=pool_.Slot(target)[10]; // 첫 refresh 뒤 바뀐 현재 종류를 읽는다.
            if (type>=types_.size()) throw std::out_of_range("상태 효과 만료의 현재 TYPE 범위");
            if (!(types_[type].flags2&0x200000U)) Need(hooks_.refresh)(target);
            else { Need(hooks_.unpop)(target,0);Need(hooks_.repop)(target,0x40); }
            if (pool_.Slot(target)[10]==136) Need(hooks_.fence)(target);
        }
    }
    Need(finish)();
}
BombEffectTimerProcess::BombEffectTimerProcess(SidPool& pool,std::span<const RiftTypeRecord> types,const double& now,std::int32_t delay,std::span<const std::uint32_t> members,std::uint32_t flags,BombEffectTimerHooks hooks)
    :logic_(pool,types,now,std::move(hooks)),fields_(logic_.Construct(delay,members,flags)) {}
// 삭제 뒤 필드나 this를 다시 읽지 않는다. 원본41bf10의 권한 판단은 같은 Host가 수행한다.
void BombEffectTimerProcess::RunFrame() {
    logic_.Frame(fields_,[this] { if (!Host()) throw std::logic_error("상태 효과 타이머 Host 미부착");Host()->Kill(*this,0); });
}
const BombEffectTimerFields& BombEffectTimerProcess::Fields() const { return fields_; }
}
