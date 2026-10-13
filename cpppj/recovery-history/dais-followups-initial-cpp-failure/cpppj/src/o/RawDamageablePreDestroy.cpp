// 효과 사이의 현재 raw/전역 읽기와 finder의 미리 저장된 next를 유지한다.
#include "o/RawDamageablePreDestroy.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 타입·상태·좌표·종속 kind와 효과/해방 억제 flags다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18,kKind=18;
constexpr std::uint32_t kCollapse=0x200000,kExplosion=0x100000,kNoRelease=0x800;
// 원본 spot 좌표의 fadd 상수이며 float 저장 전 double 계산으로 x87 덧셈을 보존한다.
constexpr float kSpotBias=0.9999f;
constexpr std::size_t kSide=256,kCells=kSide*kSide;
// 자산 상태의 free/contained와 extra의 abstract/buried 비트는 각각 9다.
constexpr std::uint8_t kUnsupported=9;
// 정렬되지 않은 raw DWORD/WORD를 낮은 바이트부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 원본 필드 폭만큼 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 유효한 보드 좌표만 절삭한다. 더하기 뒤 별도 float 반올림을 넣지 않는다.
int Cell(float coordinate) {
    const double value=static_cast<double>(coordinate)+static_cast<double>(kSpotBias);
    if (!std::isfinite(value) || value<=-1 || value>=kSide) throw std::out_of_range("Damageable spot 좌표 범위 오류");
    return static_cast<int>(value);
}
}
// 각 미복원 효과를 명시적으로 연결하며 공간 크기를 확인한다.
RawDamageablePreDestroy::RawDamageablePreDestroy(SidPool& pool,std::span<const std::uint8_t> spots,const ContainedFinderState& finderState,
    const DamageablePreDestroyState& state,DamageablePreDestroyHooks hooks)
    :pool_(pool),spots_(spots),finderState_(finderState),state_(state),hooks_(std::move(hooks)) {
    if (spots.size()!=kCells || !hooks_.collapse || !hooks_.explosion || !hooks_.sound || !hooks_.releaseContained || !hooks_.validateBase || !hooks_.base)
        throw std::invalid_argument("Damageable preDestroy 공간/하위 효과 누락");
}
// 기존 가상/일반 Pop 지원을 확장하지 않고 직접 자산과 하위 경계만 검사한다.
void RawDamageablePreDestroy::Validate(Sid sid,std::uint32_t flags) const {
    const auto raw=pool_.AllocatedBytes(sid);
    const auto typeEnd=pool_.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if ((raw[kState]&kUnsupported) || raw[kType]<kFirstAssetTypeNumber || raw[kType]>=typeEnd)
        throw std::invalid_argument("Damageable preDestroy raw 자산 상태/타입 오류");
    if (!(raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kUnsupported)) static_cast<void>(Spot(sid));
    hooks_.validateBase(sid,flags);
}
// 실제 map byte는 효과 뒤 현재 raw 좌표에서 y*256+x 순서로 읽는다.
std::uint8_t RawDamageablePreDestroy::Spot(Sid sid) const {
    const auto raw=pool_.Slot(sid);const int x=Cell(std::bit_cast<float>(Read(raw,kX,4))),y=Cell(std::bit_cast<float>(Read(raw,kY,4)));
    return spots_[static_cast<std::size_t>(y)*kSide+static_cast<std::size_t>(x)];
}
// 붕괴/폭발 효과가 raw 좌표를 바꿀 수 있어 소리 직전에 다시 읽는다.
void RawDamageablePreDestroy::SoundAt(Sid sid,DamageableSound sound) const {
    const auto raw=pool_.Slot(sid);hooks_.sound({sound,std::bit_cast<float>(Read(raw,kX,4)),std::bit_cast<float>(Read(raw,kY,4))});
}
// extra의 진입 조건은 효과 중 extra가 바뀌어도 다시 평가하지 않는다.
void RawDamageablePreDestroy::PreDestroy(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);
    if (!(pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kUnsupported)) {
        if (flags&kCollapse) { hooks_.collapse(sid);SoundAt(sid,DamageableSound::Collapse); }
        if (flags&kExplosion) { hooks_.explosion(sid);SoundAt(sid,DamageableSound::Explosion); }
        if (Spot(sid)&6) {
            RawContainedFinder finder(pool_,finderState_);
            // yesIKnow=1로 dead 부모도 허용한다. 소리 콜백 뒤 현재 kind/타입 전역을 다시 읽는다.
            for (Sid current=finder.Begin(sid,true);current.value;current=finder.Next()) {
                if (Read(pool_.Slot(current),kKind,2)==state_.priestType) hooks_.sound({DamageableSound::PriestFree,0,0});
            }
        }
        if (finderState_.boss && !(flags&kNoRelease)) hooks_.releaseContained(sid);
    }
    hooks_.base(sid,flags);
}
// 파생 연결 어댑터가 참조 대상을 검증한다.
const SidPool& RawDamageablePreDestroy::Pool() const { return pool_; }
// 전역 후처리 훅은 Carrier에 남기고 Damageable 두 경계만 연결한다.
CarrierPreDestroyHooks MakeDamageablePreDestroyHooks(const SidPool& pool,const RawDamageablePreDestroy& damageable,CarrierPreDestroyHooks hooks) {
    if (&pool!=&damageable.Pool()) throw std::invalid_argument("Carrier/Damageable preDestroy의 SID 풀이 다릅니다");
    hooks.validateDamageable=[&damageable](Sid sid,std::uint32_t flags) { damageable.Validate(sid,flags); };
    hooks.damageablePre=[&damageable](Sid sid,std::uint32_t flags) { damageable.PreDestroy(sid,flags); };
    return hooks;
}
}
