// 원본50입자·현재 전역 읽기 위치와 공유3색/Stop 진단의 복귀 순서를 보존한다.
#include "o/RawEvilRayLifetime.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 원문 EvilRay의 독립 생성/통지 몸체다.
namespace { // 원본 PE 상수와 선택 callable 수명만 이 파일에 한정한다.
constexpr std::uint32_t kParticleCount=50,kParticleStride=24,kPaletteBytes=8; //00453103/공통 입자 배열/공유 색 객체의 실제 폭이다.
//00508e60/68/58과00508e78/70/68의 binary64 비트를 원문 RGB 인자 순서로 사용한다.
constexpr std::array<double,3> kFrom{std::bit_cast<double>(std::uint64_t{0x3fe0303030303030}),std::bit_cast<double>(std::uint64_t{0x3fe3535353535353}),std::bit_cast<double>(std::uint64_t{0x3fd4545454545454})},
    kTo{std::bit_cast<double>(std::uint64_t{0x3febfbfbfbfbfbfc}),std::bit_cast<double>(std::uint64_t{0x3ff1414141414141}),std::bit_cast<double>(std::uint64_t{0x3fe3535353535353})};
}
std::vector<std::uint32_t> BuildEvilRayPalette(std::span<const std::uint32_t,kParticlePaletteColors> palette) { // 공통 원문 정렬/선택/병합 몸체를 재사용한다.
    const std::array<ParticleGradient,1> gradients{BuildParticleGradient(palette,kFrom,kTo,3)}; // 원문 요청3개를 실제 선택 수와 구분한다.
    return MergeParticleGradients(gradients);
}
RawEvilRayLifetime::RawEvilRayLifetime(const SidPool& pool,RawRenderRegistry& registry,EvilRayLifetimeState& state,EvilRayLifetimeHooks hooks)
    :registry_(registry),state_(state),hooks_(std::move(hooks)) { // 다른 판본/월드로의 등록을 생성 전에 거부한다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || &pool!=&registry.Pool()) throw std::invalid_argument("EvilRay 실제 생성 월드/판본 오류");
}
void RawEvilRayLifetime::Initialize(EvilRayFields& fields,std::uint32_t token,Sid parent) const { // 원문 공통 등록 콜백이 바꾼 source는 뒤에서 현재 값으로 읽는다.
    const auto type=state_.ProcessType(); // 등록/확보 앞에서 전달할 TYPE 인자를 한 번 저장한다.
    registry_.Construct(fields.registration,token,parent,type);
    fields.particles.clear();fields.total=0;fields.lastTime=fields.elapsed=0;fields.first=false;
    fields.nextEmission=0;fields.source=state_.Source();fields.stopping=0;
    const auto reserve=hooks_.reserve; // 확보가 자기 연결을 바꾸어도 이번 선택 장치는 호출 종료까지 유지한다.
    if (!reserve) throw std::logic_error("EvilRay 입자 확보 장치 미연결");
    if (!reserve(kParticleCount*kParticleStride)) throw std::bad_alloc();
    fields.particles.assign(kParticleCount,FieryTailParticle{});
    if (!state_.palette) {
        const auto allocate=hooks_.reserve; // 첫 공유 객체의 원문 확보 위치에서 현재 장치를 다시 선택한다.
        if (!allocate) throw std::logic_error("EvilRay 공유 객체 확보 장치 미연결");
        if (!allocate(kPaletteBytes)) throw std::bad_alloc();
        const auto create=hooks_.createPalette;if (!create) throw std::logic_error("EvilRay 공유 객체 생성 장치 미연결");state_.palette=create();
        if (!state_.palette) throw std::bad_alloc();
        const auto build=hooks_.buildPalette;if (!build) throw std::logic_error("EvilRay 공유 색 계산 장치 미연결");build(state_.palette);
    }
}
void RawEvilRayLifetime::Stop(EvilRayFields& fields,std::uint32_t mode) const { // 다른 mode는 현재 stopping 값을 쓰지 않고 복귀한다.
    if (mode!=3 || fields.stopping) {
        const auto assertion=hooks_.assertion; // 진단의 재진입/연결 반납에도 이번 callable은 살아 있다.
        if (!assertion) throw std::logic_error("EvilRay 중단 진단 장치 미연결");
        assertion(mode!=3 ? "false" : "stopping == false",".\\Evilray.cpp",mode!=3 ? 0x4eU : 0x48U);
        if (mode!=3) return;
    }
    fields.stopping=1;
}
void RawEvilRayLifetime::Destroy(EvilRayFields& fields,std::uint32_t flags,const std::function<void(EvilRayRelease)>& release) const { // 개별 소멸은 같은 공유 색 객체를 지우지 않는다.
    if (!release) throw std::invalid_argument("EvilRay 반납 장치 미연결");
    if (!fields.particles.empty()) { release(EvilRayRelease::Particles);std::vector<FieryTailParticle>().swap(fields.particles); }
    registry_.Destroy(fields.registration);
    if (flags&1U) release(EvilRayRelease::Object);
}
}
