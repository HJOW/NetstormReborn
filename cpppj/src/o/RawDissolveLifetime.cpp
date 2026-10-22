// 원본은 삭제된 포인터/용량 값을 다시0으로 쓰지 않으며 C++ 소유권만 실제 반납한다.
#include "o/RawDissolveLifetime.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kObjectBytes=120,kProcess=18,kDummy=110,kCreateFlags=2,kAttachFlags=16; //원본 wrapper와 공통 입자 생성기의 인수다.
// 현재 raw의 비정렬 좌표 네 바이트만 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t word{};
    for (std::size_t index=0;index<4;++index) word|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8); //주변 유닛 필드를 포함하지 않는다.
    return std::bit_cast<float>(word);
}
}
// 세계 경계의 누락은 등록/더미 생성을 시작하기 전에 발견한다.
RawDissolveLifetime::RawDissolveLifetime(const SidPool& pool,RawRenderRegistry& registry,DissolveLifetimeHooks hooks)
    :pool_(pool),registry_(registry),hooks_(std::move(hooks)) {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || &pool_!=&registry_.Pool() || !hooks_.reserve || !hooks_.create || !hooks_.pop || !hooks_.attach || !hooks_.depend || !hooks_.capture || !hooks_.prepare) throw std::invalid_argument("분해 판본/실제 수명 경계 누락");
}
// OOM의 원본 null 역참조는 재현하지 않으며 정상 생성과 구별한다.
bool RawDissolveLifetime::ReserveObject() const { return hooks_.reserve(kObjectBytes); }
// source 삭제 전에 캡처를 끝내고 첫 Frame까지 clock은 시작하지 않는다.
void RawDissolveLifetime::Initialize(DissolveFrameFields& fields,std::uint32_t token,Sid source,Sid first,Sid second,std::int32_t xOriented,Sid& dummy) const {
    static_cast<void>(pool_.Slot(source));auto& value=fields.common;registry_.Construct(value.registration,token,{},kProcess);
    value.particles.clear();value.total=0;value.lastTime=value.elapsed=0;value.first=false;fields.particlesAllocated=false;
    dummy=hooks_.create(kDummy,kCreateFlags);const auto raw=pool_.Slot(source);hooks_.pop(dummy,Float(raw,14),Float(raw,18),kAttachFlags);hooks_.attach(value.registration,kProcess,dummy,kAttachFlags);
    if (value.registration.form<1 || value.registration.form>65535) throw std::out_of_range("분해 현재 Form 번호 오류");
    hooks_.depend(Sid{static_cast<std::uint16_t>(value.registration.form)},dummy);value.image.pixels.clear();fields.opaqueCount=0;fields.clockStarted=false;
    value.image=hooks_.capture(source);hooks_.prepare(fields,first,second,xOriented);
}
// 투명 이미지의 new0도 실제 비영 포인터이므로 입자 확보 여부를 따로 추적한다.
void RawDissolveLifetime::Destroy(DissolveFrameFields& fields,std::uint32_t flags,const std::function<void(ShrapnelRelease)>& release) const {
    if (!release) throw std::invalid_argument("분해 실제 반납 경계 누락");auto& value=fields.common;
    if (!value.image.pixels.empty()) { release(ShrapnelRelease::Source);std::vector<std::uint8_t>().swap(value.image.pixels); }
    if (fields.particlesAllocated) { release(ShrapnelRelease::Particles);std::vector<ShrapnelParticle>().swap(value.particles);fields.particlesAllocated=false; }
    registry_.Destroy(value.registration);if (flags&1U) release(ShrapnelRelease::Object);
}
}
