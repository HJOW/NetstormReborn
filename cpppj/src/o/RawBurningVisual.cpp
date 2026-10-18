// Burning 생성자 숫자쓰기와 실제입자 여유/만료칸 선택을 현재표시 저장소에 적용한다.
#include "o/RawBurningVisual.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 생성자 고정150입자와 실제32비트 메모리의 확보량이다.
constexpr std::uint32_t kParticles=150,kParticleBytes=24,kPaletteBytes=8;
// 보호 PE가 공급하는 주황색 두끝점과 공통 색띠 보정이다.
constexpr std::array<double,3> kFirst{1.0,115.0/255.0,7.0/255.0},kLast{1.0,245.0/255.0,117.0/255.0}; // 실제 cold 생성자의 시작 주황→끝 밝은색 순서다.
constexpr double kGradientNoise=2.4;
}
// 공통표시 등록은 현재같은 풀을 사용하며 정상메모리 경계가 반드시 필요하다.
RawBurningVisual::RawBurningVisual(const SidPool& pool,RawRenderRegistry& registry,BurningVisualState& state,BurningVisualHooks hooks)
    :pool_(pool),registry_(registry),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&registry.Pool() || !hooks_.reserve) throw std::invalid_argument("Burning 표시 등록/메모리 실제월드 오류");
}
// 원본 ctor의 모드인수는0/1 이외도DWORD로보존한다. 현재 두wrapper 관찰은0/1이다.
void RawBurningVisual::Initialize(BurningVisualFields& fields,std::uint32_t token,Sid victim,std::uint32_t mode) const {
    registry_.Construct(fields.registration,token,victim,state_.processType);
    fields.capacity=fields.total=0;fields.particles.clear();fields.lastTime=fields.elapsed=0;fields.palette=0;
    fields.victim=victim;fields.stage=0;fields.image={};fields.rows.clear();fields.selected=0;fields.off=false;fields.sound=0;
    if (!hooks_.reserve(kParticles*kParticleBytes)) throw std::bad_alloc();
    fields.capacity=kParticles;fields.particles.assign(kParticles,BurningParticle{});fields.total=0;fields.mode=mode;
    if (!state_.palette) {
        if (!hooks_.createPalette || !hooks_.gradient || !hooks_.buildPalette) throw std::logic_error("Burning 실제첫팔레트 생성 미복원");
        if (!hooks_.reserve(kPaletteBytes)) throw std::bad_alloc();state_.palette=hooks_.createPalette();
        if (!state_.palette) throw std::bad_alloc();hooks_.gradient(kFirst,kLast,8,kGradientNoise);hooks_.buildPalette(state_.palette);
    }
}
// 원본 공통Particle.New: 아직쓰지않은앞칸을늘리고, 가득찼을때처음 만료된칸을재사용한다.
bool RawBurningVisual::Append(BurningVisualFields& fields,BurningParticle particle) const {
    if (fields.particles.size()!=fields.capacity || fields.total>fields.capacity) throw std::invalid_argument("Burning 현재입자 저장소/개수 오류");
    std::size_t index=fields.total;
    if (index<fields.capacity) ++fields.total;
    else {
        index=0;const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
        // 패치는ordered float<=0에서만재사용하며, CD는동일DWORD의signed<=0를검사한다.
        while (index<fields.total && (patch ? !(fields.particles[index].life<=0) : std::bit_cast<std::int32_t>(fields.particles[index].life)>0)) ++index;
        if (index==fields.total) return false;
    }
    fields.particles[index]=particle;return true;
}
// 후속실제효과 소비자와 SID풀을 공유하는지 검사한다.
const SidPool& RawBurningVisual::Pool() const { return pool_; }
}
