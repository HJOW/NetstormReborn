// 원본 공통 생성자가 쓰지 않는 필드는 유지하고 실제 부착/확보/반납 순서를 지킨다.
#include "o/RawFieryTailLifetime.h"
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 보호10.78 PE의 process11·120입자·24바이트 stride·공유 객체8바이트다.
constexpr std::uint32_t kProcessType=11,kParticleCount=120,kParticleBytes=24,kPaletteBytes=8;
// Fiery Tail 파생 생성자가 마지막에 쓰는 깊이와 최대 수명이다.
constexpr std::int32_t kDepth=-25,kMaximumLife=4;
}
// 다른 월드/판본과 메모리 경계 누락은 등록 전에 거부한다.
RawFieryTailLifetime::RawFieryTailLifetime(const SidPool& pool,RawRenderRegistry& registry,
    FieryTailLifetimeState& state,FieryTailLifetimeHooks hooks)
    :registry_(registry),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || &pool!=&registry.Pool() || !hooks_.reserve)
        throw std::invalid_argument("Fiery Tail 실제 생성 월드/확보 경계 오류");
}
// 공통 등록이 끝나야 System 필드를 초기화한다. 생성 중 프레임 재진입은 상위 호스트가 차단한다.
void RawFieryTailLifetime::Initialize(FieryTailFrameFields& fields,std::uint32_t token,Sid parent) const {
    registry_.Construct(fields.registration,token,parent,kProcessType);
    fields.particles.clear();fields.total=0;fields.lastTime=fields.elapsed=0;fields.first=false;
    fields.previousX=fields.previousY=0;
    if (!hooks_.reserve(kParticleCount*kParticleBytes)) throw std::bad_alloc();
    fields.particles.assign(kParticleCount,FieryTailParticle{});
    if (!state_.palette) {
        if (!hooks_.createPalette || !hooks_.buildPalette) throw std::logic_error("Fiery Tail 첫 공유 색 표 경계 누락");
        if (!hooks_.reserve(kPaletteBytes)) throw std::bad_alloc();
        state_.palette=hooks_.createPalette();if (!state_.palette) throw std::bad_alloc();
        hooks_.buildPalette(state_.palette);
    }
    fields.maximumLife=kMaximumLife;fields.palette=state_.palette;fields.offset=0;fields.registration.zOrder=kDepth;
}
// 실제 free는 개수/포인터 필드를 다시 쓰지 않는다. 호스트 컨테이너의 저장소만 반납한다.
void RawFieryTailLifetime::Destroy(FieryTailFrameFields& fields,std::uint32_t flags,
    const std::function<void(FieryTailRelease)>& release) const {
    if (!release) throw std::invalid_argument("Fiery Tail 반납 경계 누락");
    if (!fields.particles.empty()) { release(FieryTailRelease::Particles);std::vector<FieryTailParticle>().swap(fields.particles); }
    registry_.Destroy(fields.registration);
    if (flags&1U) release(FieryTailRelease::Object);
}
}
