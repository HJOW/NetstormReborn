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
// Native _ftol의signed64절삭뒤EAX하위DWORD를보존하고좌표별0..255자격관문은넣지않는다.
std::uint32_t NativeCell(float coordinate) {
    const double value=static_cast<double>(coordinate)+static_cast<double>(kSpotBias);
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) throw std::out_of_range("Damageable Native spot의실제CRT절삭표현범위");
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
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
void RawDamageablePreDestroy::EnableNativeBody() { if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Native Damageable 전처리 판본 오류");nativeBody_=true; } // 장기 실제 owner의 구성 때만 선택하며 게임 상태/난수는 쓰지 않는다.
// 독립 typed의 추가 선검사는 Native 원문 몸체 앞에 실행하지 않는다.
void RawDamageablePreDestroy::Validate(Sid sid,std::uint32_t flags) const {
    if (nativeBody_) return;
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
// 효과뒤현재좌표를선택하고원문의감긴Y*256+X가도달한공간한BYTE만확인한다.
std::uint8_t RawDamageablePreDestroy::SpotNative(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto y=NativeCell(std::bit_cast<float>(Read(raw,kY,4))),x=NativeCell(std::bit_cast<float>(Read(raw,kX,4)));
    const auto index=y*static_cast<std::uint32_t>(kSide)+x;if (index>=spots_.size()) throw std::out_of_range("Damageable Native spot의현재flatBYTE물리부족");return spots_[index];
}
// 붕괴/폭발 효과가 raw 좌표를 바꿀 수 있어 소리 직전에 다시 읽는다.
void RawDamageablePreDestroy::SoundAt(Sid sid,DamageableSound sound) const {
    const auto raw=pool_.Slot(sid);hooks_.sound({sound,std::bit_cast<float>(Read(raw,kX,4)),std::bit_cast<float>(Read(raw,kY,4))});
}
// extra의 진입 조건은 효과 중 extra가 바뀌어도 다시 평가하지 않는다.
void RawDamageablePreDestroy::PreDestroy(Sid sid,std::uint32_t flags) const {
    if (!nativeBody_) Validate(sid,flags);RunPreDestroy(sid,flags,hooks_.releaseContained,nativeBody_);
}
// 선택된 실제 부모는 파편/소리보다앞에서Spot이나공통장부를미리읽지않는다.
void RawDamageablePreDestroy::PreDestroyNative(Sid sid,std::uint32_t flags,const std::function<void(Sid)>& releaseContained) const { RunPreDestroy(sid,flags,releaseContained,true); }
// 두 진입은 현재extra의한번분기와효과뒤현재좌표/전역/종속순서를공유한다.
void RawDamageablePreDestroy::RunPreDestroy(Sid sid,std::uint32_t flags,const std::function<void(Sid)>& releaseContained,bool native) const {
    if (!(pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kUnsupported)) {
        if (flags&kCollapse) { hooks_.collapse(sid);SoundAt(sid,DamageableSound::Collapse); }
        if (flags&kExplosion) { hooks_.explosion(sid);SoundAt(sid,DamageableSound::Explosion); }
        if ((native ? SpotNative(sid):Spot(sid))&6) {
            RawContainedFinder finder(pool_,finderState_);
            // yesIKnow=1로 dead 부모도 허용한다. 소리 콜백 뒤 현재 kind/타입 전역을 다시 읽는다.
            for (Sid current=finder.Begin(sid,true);current.value;current=finder.Next()) {
                if (Read(pool_.Slot(current),kKind,2)==state_.PriestType()) hooks_.sound({DamageableSound::PriestFree,0,0});
            }
        }
        if (finderState_.Boss() && !(flags&kNoRelease)) { if (!releaseContained) throw std::logic_error("Damageable Native 삭제의 선택된 실제 해방 장치 미연결");releaseContained(sid); }
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
