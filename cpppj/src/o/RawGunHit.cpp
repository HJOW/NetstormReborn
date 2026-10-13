// 원본 packed 시각의 감김·절삭·DPS 정수화와 판본별 float 저장 순서를 유지한다.
#include "o/RawGunHit.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// WORD bit0은 건물 출력 상태이며 나머지15비트는1/8초 시각이다.
constexpr std::uint32_t kTimeMask=0x7fff,kFlyerOrBalloon=0x120000;
// 원본 정상 CRT __ftol의 유한 int32 절삭이다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("Gun hit 시각/피해 절삭 범위 오류");return static_cast<std::int32_t>(value);
}
// 원본 정수초와 소수8분의1초를 OR하고 필요한 하위15비트를 읽는다.
std::uint32_t Packed(double now) {
    const auto integral=Truncate(now);const auto fraction=Truncate((now-integral)*8.0);return (std::bit_cast<std::uint32_t>(integral)*8U)|std::bit_cast<std::uint32_t>(fraction);
}
}
// 실제 타입/DPS 자료의 판본과 수명을 검사한다.
RawGunHit::RawGunHit(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const GunHitType> hitTypes,GunHitHooks hooks)
    :pool_(pool),types_(types),hitTypes_(hitTypes),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || hitTypes.size()!=types.size()) throw std::invalid_argument("Gun hit 타입/DPS 판본 오류");
}
// 실제 시간 필드를 소유할 수 있는 원본 자산 분류를 확인한다.
std::span<const std::uint8_t> RawGunHit::Firing(Sid firing) const {
    if (!firing.value || firing.value>=pool_.Capacity()) throw std::out_of_range("Gun hit 발사 SID 오류");const auto raw=pool_.Slot(firing);
    if (raw[10]>=types_.size()) throw std::out_of_range("Gun hit 발사 타입 오류");const auto& type=types_[raw[10]];
    if (!(type.flags2&0x100000) && type.group!=0 && type.group!=1) throw std::logic_error("Gun hit 시간 group 원본 assert");return raw;
}
// 시각 분수는 binary 0.125이므로 원본 double 결과와 동일한 비트를 유지한다.
double RawGunHit::Time(Sid firing) const {
    const auto raw=Firing(firing);const std::uint32_t word=static_cast<std::uint32_t>(raw[12])|(static_cast<std::uint32_t>(raw[13])<<8);const auto packed=word>>1;return static_cast<double>(packed>>3)+(packed&7)*0.125;
}
// 원본 현재 시각 WORD 쓰기와 뒤따르는 실제 공간 통지다.
void RawGunHit::SetTime(Sid firing,double now) const {
    Firing(firing);if (!hooks_.notifySurface) throw std::logic_error("Gun hit 실제 공간 통지 경계 누락");auto raw=pool_.AllocatedBytes(firing);const auto packed=Packed(now);const std::uint16_t word=static_cast<std::uint16_t>((packed<<1)|(raw[12]&1U));raw[12]=static_cast<std::uint8_t>(word);raw[13]=static_cast<std::uint8_t>(word>>8);hooks_.notifySurface(firing);
}
// 가상 피해 호출 직전 표적의 live/free/dead와 발사 소유자를 다시 읽는다.
void RawGunHit::Apply(Sid firing,Sid target,std::int32_t damage) const {
    if (!target.value) return;if (target.value>=pool_.Capacity()) throw std::out_of_range("Gun hit 표적 SID 오류");if (pool_.Slot(target)[11]&3) return;
    const auto source=Firing(firing);if (!hooks_.damage) throw std::logic_error("Gun hit 실제 표적 피해 경계 누락");hooks_.damage(target,damage,source[pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U],firing);
}
// Packed(now)를 읽어 실제 양자화 시각차와 원본 float DPS를 사용한다.
void RawGunHit::Hit(Sid firing,Sid target,double now) const {
    const auto source=Firing(firing);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;float dps=hitTypes_[source[10]].damagePerSecond;
    if (!std::isfinite(dps)) throw std::invalid_argument("Gun hit 비유한 DPS");
    if (patch && hitTypes_[source[10]].specialFlyerDamage && target.value) {
        if (target.value>=pool_.Capacity()) throw std::out_of_range("Gun hit DPS 표적 SID 오류");const auto victim=pool_.Slot(target);if (victim[10]>=types_.size()) throw std::out_of_range("Gun hit DPS 표적 타입 오류");if (types_[victim[10]].flags2&kFlyerOrBalloon) dps=hitTypes_[source[10]].flyerDamagePerSecond;
    }
    const auto packed=Packed(now)&kTimeMask;const double elapsed=static_cast<double>(packed>>3)+(packed&7)*0.125-Time(firing);const float storedElapsed=static_cast<float>(elapsed);
    // 두 판본은 x87에 남은 시각차로 곱하고 정확히30초를 넘을 때만3초분으로 바꾼다.
    const double damage=elapsed*static_cast<double>(dps);
    if (damage>=1.0) {
        // CD는 선택된 곱을 float로 저장한 뒤 다시 읽어 절삭하며 패치는 x87 값을 유지한다.
        const double selected=(storedElapsed<0 || storedElapsed>30.0f) ? static_cast<double>(dps)*3.0 : damage;
        Apply(firing,target,Truncate(patch ? selected : static_cast<double>(static_cast<float>(selected))));
    }
    SetTime(firing,now);
}
// 같은 실제 풀을 사용하는 소비자만 연결한다.
const SidPool& RawGunHit::Pool() const { return pool_; }
// Missile 직접 피해와 빗나감/배리어 코너 시각 갱신을 같은 몸체로 연결한다.
MissileHooks MakeMissileGunHitHooks(const RawGunHit& hit,const MissileState& state,MissileHooks hooks) {
    hooks.hit=[&hit,&state](Sid firing,Sid target) { hit.Hit(firing,target,state.now); };hooks.refresh=[&hit,&state](Sid firing) { hit.SetTime(firing,state.now); };return hooks;
}
}
