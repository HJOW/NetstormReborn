// Fierytail.cpp의 생성자와 Particle.cpp의 공유 소멸 순서를 복원한다.
#pragma once
#include "o/RawFieryTailFrame.h"

namespace netstorm::o {
// 같은 프레임/Draw를 공유하는 원본11 불꽃과12 건설 마나의 생성자 차이다.
enum class FieryTailVariant { Fiery,Streak }; // Fiery는 수명4/깊이-25, Streak는 수명2/좌표-8/깊이-20이다.
// 원본0055a750의 월드 공유 저장소다. 개별 효과가 이 색 표를 소유하지 않는다.
struct FieryTailLifetimeState {
    std::uint32_t palette{}; // 첫 생성 때 채우고 후속 효과에서 재사용하는 색 표 토큰이다.
    std::uint32_t* sharedPalette{}; // 건설 마나는 빔과 같은0055a758을 빌리며 일반 불꽃은 자체0055a750을 사용한다.
    std::uint32_t& CurrentPalette() { return sharedPalette ? *sharedPalette : palette; } // 공유 시 실제 장기 owner에 직접 읽고 쓴다.
};
// 실제 메모리 확보와 기존 RGB 색 띠 몸체를 호출하는 경계다.
struct FieryTailLifetimeHooks {
    std::function<bool(std::uint32_t)> reserve; // 입자2880바이트 또는 공유 색 표8바이트의 확보 요청이다.
    std::function<std::uint32_t()> createPalette; // 00487cb0의 빈 색 표를 만들고0이 아닌 토큰을 반환한다.
    std::function<void(std::uint32_t)> buildPalette; // 같은 토큰에 실제8+3색을 채운다. 인접 DWORD를 만들지 않는다.
};
// scalar deleting destructor에서 구별하는 두 실제 반납 대상이다.
enum class FieryTailRelease { Particles,Object };
// PATCH00457ea0/004882c0의 정상 생성·삭제 몸체다. Kernel 소유권은 호출자가 관리한다.
class RawFieryTailLifetime {
public:
    // registry/pool은 같은10.78 월드여야 한다. 공유 상태와 경계는 이 객체보다 오래 살아야 한다.
    RawFieryTailLifetime(const SidPool& pool,RawRenderRegistry& registry,
        FieryTailLifetimeState& state,FieryTailLifetimeHooks hooks,FieryTailVariant variant=FieryTailVariant::Fiery);
    // 이미 확보한 객체에 등록→부착→120입자 zero→첫 공유 색 표→파생 필드 순서로 쓴다.
    // 원본이 쓰지 않는 delta/scaledDelta는 보존한다. 실패한 등록의 정리는 호출자 책임이다.
    void Initialize(FieryTailFrameFields& fields,std::uint32_t token,Sid parent) const;
    // 입자 반납→목록 제거/base 소멸→flags bit0의 객체 반납 순서다. 공유 색 표는 보존한다.
    // release는 예외를 던지거나 등록/수명을 바꾸지 않아야 한다. 같은 객체에 두 번 호출하지 않는다.
    void Destroy(FieryTailFrameFields& fields,std::uint32_t flags,
        const std::function<void(FieryTailRelease)>& release) const;
private:
    RawRenderRegistry& registry_; // 다른 표시 효과와 공유하는 현재 등록 목록이다.
    FieryTailLifetimeState& state_; // 원본의 첫 생성/재사용을 제어하는 월드 저장소다.
    FieryTailLifetimeHooks hooks_; // 실제 확보와 공유 색 표 초기화 경계다.
    FieryTailVariant variant_; // 원본 생성자11/12만 구별하며 공통 입자 진행은 그대로 공유한다.
};
}
