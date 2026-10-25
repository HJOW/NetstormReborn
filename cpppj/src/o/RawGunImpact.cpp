// 충돌 효과의 원본 생성/프레임/삭제 순서와 두 판본 Kernel 한도를 보존한다.
#include "o/RawGunImpact.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 anim 가상 표와 생성 한도는 원본 PE 값이며 호스트 함수 포인터로 쓰지 않는다.
constexpr std::uint32_t kPatchAnimTable=0x509758,kCdAnimTable=0x503b30;
constexpr std::size_t kPatchLimit=30001,kCdLimit=3001;
// 비정렬 raw의 실제 little endian DWORD를 읽는다.
std::uint32_t ReadImpact(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 지정된 네 바이트만 낮은 자리부터 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 원본 frame의 저장 폭만 덮고 padding/상태를 보존한다.
void WriteInitialFrame(std::span<std::uint8_t> raw,bool patch,std::int32_t frame) {
    const auto bits=std::bit_cast<std::uint32_t>(frame);
    // 패치 DWORD 또는 CD BYTE를 실제 첫 frame 쓰기와 같은 폭으로 저장한다.
    for (std::size_t i=0;i<(patch ? 4U : 1U);++i) raw[(patch ? 36U : 34U)+i]=static_cast<std::uint8_t>(bits>>(8*i));
}
}
// 실제 프레임 표/지정기와 단일 월드 풀의 판본을 검사한다.
RawGunImpact::RawGunImpact(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter,
    GameRandom& random,GunImpactState& state,GunImpactHooks hooks)
    :pool_(pool),frames_(frames),setter_(setter),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (&setter.Pool()!=&pool || frames.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || state.animationType!=160)
        throw std::invalid_argument("충돌 효과 실제 풀/프레임/anim 타입 오류");
}
// 실제 하한float1이며 unordered 상한 비교에서 패치는NaN을보존하고CD는255를선택한다.
std::array<float,2> RawGunImpact::Clamp(float x,float y,OriginalEdition edition) {
    float a=x<1.0f ? 1.0f : x,b=y<1.0f ? 1.0f : y;const bool patch=edition==OriginalEdition::Patch1078;if (patch ? a>255.0f : !(a<=255.0f)) a=255.0f;if (patch ? b>255.0f : !(b<=255.0f)) b=255.0f;return {a,b};
}
// 실제 타입 표의 signed 첫 번호와 개수를 현재 자료로 읽는다.
RiftTypeFrames::LetterRun RawGunImpact::Run(std::uint32_t type,std::uint32_t event) const {
    if (type>=frames_.size() || event>=16) throw std::out_of_range("충돌 효과 실제 글자 구간 오류");return frames_[type].Run(static_cast<std::uint8_t>('A'+event));
}
// 억제와 프로세스 한도에서는 원본처럼 생성·좌표 쓰기 없이 반환한다.
void RawGunImpact::Visual(float x,float y,std::uint32_t variant,float interval) const {
    if (state_.SuppressedTime()!=0 || state_.suppressed) return;if (!hooks_.processCount) throw std::logic_error("충돌 효과 실제 Kernel 개수 조회 누락");const bool patch=pool_.Edition()==OriginalEdition::Patch1078;if (hooks_.processCount()>=(patch ? kPatchLimit : kCdLimit)) return;
    const auto run=Run(state_.animationType,variant);if (!hooks_.create || !hooks_.pop || !hooks_.regular) throw std::logic_error("충돌 효과 실제 생성/Pop/Regular 누락");const auto position=Clamp(x,y,pool_.Edition());const auto sid=hooks_.create(state_.animationType,2);if (!Handles(sid) || pool_.Slot(sid)[10]!=state_.animationType) throw std::logic_error("충돌 효과 Factory 실제 anim 생성 오류");WriteInitialFrame(pool_.AllocatedBytes(sid),patch,run.first);hooks_.pop(sid,position[0],position[1],0);hooks_.regular(sid,variant,interval);
}
// Dude의 실제 반응은 아직 독립 하위 경계이며 누락을 묵시 성공으로 처리하지 않는다.
void RawGunImpact::Ground(float x,float y) const {
    if (!hooks_.notifyDudes) throw std::logic_error("충돌 효과 실제 주변 Dude 반응 미복원");const auto position=Clamp(x,y,pool_.Edition());hooks_.notifyDudes(position[0],position[1]);random_.Next(100);Visual(position[0],position[1],3,0.02f);
}
// 현재 실제 anim 가상 표만 지원하며 count DWORD를 signed int로 해석한다.
float RawGunImpact::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(sid)) throw std::invalid_argument("충돌 효과 실제 anim Handle 가상 표 오류");const auto run=Run(pool_.Slot(sid)[10],event);const auto index=std::bit_cast<std::int32_t>(count);
    if (index>=0 && index<run.count) { setter_.Set(sid,std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(run.first)+count),0);return payload; }
    if (!hooks_.destroy) throw std::logic_error("충돌 효과 실제 anim 종료 삭제 누락");hooks_.destroy(sid,0);return -1.0f;
}
// 주소를 호스트에서 역참조하지 않고 현재 raw 기록값으로 확인한다.
bool RawGunImpact::Handles(Sid sid) const { return sid.value && sid.value<pool_.Capacity() && ReadImpact(pool_.Slot(sid),0)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchAnimTable : kCdAnimTable); }
// 기존 실제 자산/사제 처리기를 합성하며 누락된 다른 타입은 예외로 남긴다.
RegularHandler RawGunImpact::Handler(RegularHandler fallback) const {
    return [this,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { if (Handles(sid)) return Handle(sid,event,count,payload);if (!fallback) throw std::logic_error("충돌 효과 외 실제 Handle 미연결");return fallback(sid,event,count,payload); };
}
// Factory/Host 어댑터가 같은 저장소를 사용하는지 확인할 참조다.
const SidPool& RawGunImpact::Pool() const { return pool_; }
// 실제 C++ 자산 생성과 Regular 생성/부착을 재사용하며 실패를 성공으로 바꾸지 않는다.
GunImpactHooks MakeGunImpactRuntimeHooks(const SidPool& pool,SquidFactory& factory,Kernel& kernel,SquidProcessHost& host,GunImpactHooks hooks) {
    if (&factory.Pool()!=&pool || &host.Pool()!=&pool || kernel.Capacity()!=(pool.Edition()==OriginalEdition::Patch1078 ? kProcessCapacity : kCdProcessCapacity)) throw std::invalid_argument("충돌 효과 실제 Factory/Host/Kernel 판본 오류");hooks.processCount=[&kernel] { return kernel.Size(); };hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };hooks.regular=[&host](Sid sid,std::uint32_t event,float payload) { if (!host.AddRegular(sid,event,payload,0x50)) throw std::logic_error("충돌 효과 실제 Regular 부착 실패"); };return hooks;
}
// barrier와 일반 표적 충돌을 각각 실제 producer/ground 몸체에 연결한다.
MissileHooks MakeMissileGunImpactHooks(const RawGunImpact& impact,MissileHooks hooks) { hooks.impact=[&impact](float x,float y,std::uint32_t variant,float interval) { impact.Visual(x,y,variant,interval); };hooks.hitEffect=[&impact](float x,float y) { impact.Ground(x,y); };return hooks; }
// 기존 Damageable의 실제 ground 사건도 동일한 난수/생성 사슬을 소비한다.
AssetRegularHooks MakeAssetGunImpactHooks(const RawGunImpact& impact,AssetRegularHooks hooks) { hooks.damageGround=[&impact](float x,float y) { impact.Ground(x,y); };return hooks; }
}
