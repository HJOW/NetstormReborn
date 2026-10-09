// 좌표 캡처와 현재 소유자/로컬 조건을 읽는 시점을 원본 순서대로 유지한다.
#include "o/RawPriestShield.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본의 공통 비정렬 좌표와 소리 요청 이름이다.
constexpr std::size_t kX=14,kY=18;
constexpr std::string_view kShieldSound="priestForceField.wav",kNoticeKey="PriestImmobile";
// 좌표를 산술 변환 없이 읽어 생성 훅이 공급한 NaN/부호 있는 0도 전달한다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // little endian DWORD의 비트를 조립한다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
}
// 실제 조회의 풀 및 필수 경계는 첫 효과 전에 검사한다.
RawPriestShield::RawPriestShield(SidPool& pool,const RawPriestForcefield& lookup,const PriestShieldState& state,PriestShieldHooks hooks)
    :pool_(pool),lookup_(lookup),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&lookup.Pool() || !hooks_.create || !hooks_.setOwner || !hooks_.pop || !hooks_.reserveSound ||
        !hooks_.loadSound || !hooks_.attachSound || !hooks_.noticeSound ||
        (pool.Edition()==OriginalEdition::Patch1078 && (!hooks_.translate || !hooks_.tell)))
        throw std::invalid_argument("사제 보호막 생성 풀/필수 효과 연결 오류");
}
// 생성 뒤 좌표를 캡처하고 소유자 지정/Pop/소리 뒤에는 로컬 조건을 다시 읽는다.
void RawPriestShield::Ensure(Sid priest) const {
    if (lookup_.Find(priest).value) return;
    const Sid shield=hooks_.create(lookup_.Type(),2);
    if (shield.value<5 || shield.value>=pool_.Capacity() || (pool_.Slot(shield)[11]&9))
        throw std::invalid_argument("사제 보호막 생성이 유효한 자산 SID를 반환하지 않았습니다");
    const auto raw=pool_.Slot(priest);const float x=Coordinate(raw,kX),y=Coordinate(raw,kY);
    const auto ownerOffset=pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U;
    hooks_.setOwner(shield,raw[ownerOffset]);hooks_.pop(shield,x,y,0);
    if (hooks_.reserveSound()) {
        const auto sound=hooks_.loadSound(kShieldSound);hooks_.attachSound(shield,sound,0,1);
    }
    if (pool_.Slot(priest)[ownerOffset]==state_.localPlayer && state_.loadingDepth==0 && state_.noticeClock==0.0) {
        hooks_.noticeSound({});
        if (pool_.Edition()==OriginalEdition::Patch1078) { const auto text=hooks_.translate(kNoticeKey);hooks_.tell(text); }
    }
}
// 사제 후처리의 어댑터가 월드를 혼합하지 않도록 실제 풀을 제공한다.
const SidPool& RawPriestShield::Pool() const { return pool_; }
// 두 효과의 같은 풀 연결만 교체하며 미복원 가상 Pop 지원 범위를 넓히지 않는다.
PriestShieldHooks MakePriestShieldCreationHooks(const SidPool& pool,SquidFactory& factory,SquidOwner& owner,PriestShieldHooks hooks) {
    if (&pool!=&factory.Pool() || &pool!=&owner.Pool()) throw std::invalid_argument("보호막/factory/소유자 SID 풀이 다릅니다");
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
    hooks.setOwner=[&owner](Sid sid,std::uint32_t player) { owner.Set(sid,player); };return hooks;
}
}
