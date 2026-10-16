// 원본 중간 생성 상태와 callback 뒤의 현재 raw 읽기를 유지한다.
#include "o/RawShrapnelLifetime.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 보호10.78의 실제 객체 크기/프로세스/더미 타입/flags다.
constexpr std::uint32_t kObjectBytes=104,kProcess=16,kDummy=110,kCreateFlags=2,kAttachFlags=16;
// 세 가상 생성자가 저장하는 정확한 binary32 중력이다.
constexpr std::array<std::uint32_t,3> kGravity{0x40c00000,0x3e75c28f,0x3f99999a};
// 현재 비정렬 좌표 DWORD만 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 네 low byte의 원래 위치를 보존한다.
    for (std::size_t index=0;index<4;++index) bits|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return std::bit_cast<float>(bits);
}
}
// 생성 도중 효과 누락을 발견하지 않도록 실제 경계를 먼저 확인한다.
RawShrapnelLifetime::RawShrapnelLifetime(const SidPool& pool,RawRenderRegistry& registry,ShrapnelLifetimeHooks hooks)
    :pool_(pool),registry_(registry),hooks_(std::move(hooks)) {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || &pool_!=&registry_.Pool() || !hooks_.reserve || !hooks_.create || !hooks_.pop || !hooks_.attach || !hooks_.depend || !hooks_.capture || !hooks_.prepare) throw std::invalid_argument("파편 수명 판본/실제 경계 누락");
}
// 실제 wrapper에서 이 실패는 생성자/등록/더미 생성에 진입하지 않는다.
bool RawShrapnelLifetime::ReserveObject() const { return hooks_.reserve(kObjectBytes); }
// 초기 깊이는 SID0에서 오며 최종 깊이는 캡처/준비 후의 현재 source에서 온다.
void RawShrapnelLifetime::Initialize(ShrapnelFrameFields& fields,std::uint32_t token,Sid source,ShrapnelKind kind,Sid& dummy) const {
    const auto variant=static_cast<std::size_t>(kind);if (variant>=kGravity.size()) throw std::invalid_argument("파편 가상 생성기 범위 오류");
    static_cast<void>(pool_.Slot(source));registry_.Construct(fields.registration,token,{},kProcess);
    fields.particles.clear();fields.total=0;fields.lastTime=fields.elapsed=0;fields.first=false;
    dummy=hooks_.create(kDummy,kCreateFlags);const auto raw=pool_.Slot(source);
    hooks_.pop(dummy,Float(raw,14),Float(raw,18),kAttachFlags);hooks_.attach(fields.registration,kProcess,dummy,kAttachFlags);
    if (fields.registration.form<1 || fields.registration.form>65535) throw std::out_of_range("파편 현재 Form 번호 오류");
    hooks_.depend(Sid{static_cast<std::uint16_t>(fields.registration.form)},dummy);
    if (kind==ShrapnelKind::Variant2) fields.registration.zOrder=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(fields.registration.zOrder)+1U);
    fields.image.pixels.clear();fields.gravity=std::bit_cast<float>(kGravity[variant]);
    fields.image=hooks_.capture(source);hooks_.prepare(fields,kind);fields.registration.zOrder=std::bit_cast<std::int8_t>(pool_.Slot(source)[35]);
}
// 원본은 해제한 raw 포인터/용량을0으로 다시 쓰지 않는다. 호스트 vector 소유권만 실제 반납한다.
void RawShrapnelLifetime::Destroy(ShrapnelFrameFields& fields,std::uint32_t flags,const std::function<void(ShrapnelRelease)>& release) const {
    if (!release) throw std::invalid_argument("파편 실제 반납 경계 누락");
    if (!fields.image.pixels.empty()) { release(ShrapnelRelease::Source);std::vector<std::uint8_t>().swap(fields.image.pixels); }
    if (!fields.particles.empty()) { release(ShrapnelRelease::Particles);std::vector<ShrapnelParticle>().swap(fields.particles); }
    registry_.Destroy(fields.registration);if (flags&1U) release(ShrapnelRelease::Object);
}
}
