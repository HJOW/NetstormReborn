// 실제 Burning 출력·소멸과 RenderProcess의 프레임 진입 판정을 복원한다.
#pragma once
#include "o/RawBurningFrame.h"

namespace netstorm::o {
// 원본 공유 색수와 실제 읽을 수 있는 인접 DWORD를 구별한다.
struct BurningDrawPalette {
    std::int32_t count{}; // 원본 공유 표의 요청 색수8이다.
    std::span<const std::uint32_t> readable; // 같은 실제 할당의 유효 snapshot이며 임의 끝색을 추가하지 않는다.
};
bool IsBurningArcherCapture(std::span<const std::uint8_t> raw); // 복원한71/128의 실제 두VT만 받는다.
void DrawBurningArcherCapture(std::span<const std::uint8_t> raw,std::int32_t base,std::int32_t muzzleX,std::int32_t muzzleY,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& draw,std::int32_t x,std::int32_t y); // 실제+a4의 현재 그림과 조건부 디스크83을 순서대로 제출한다.
enum class BurningRelease { Image,Rows,Particles,Object }; // 실제 소멸자가 반납하는 순서별 소유 버퍼다.
bool RenderProcessFrameDue(std::int32_t slot,std::uint32_t frame); // 실제487940은 첫7slot 또는(slot XOR frame)의 하위2비트0일 때만+24로 간다.
void DrawBurningVisual(const BurningVisualFields& fields,const BurningFrameState& state,const BurningDrawPalette& palette,
    const std::function<void(std::int32_t,std::int32_t,std::uint8_t)>& pixel); // 실제423ec0의 현재 clip 관문/5픽셀 순서다.
void DestroyBurningVisual(BurningVisualFields& fields,RawRenderRegistry& registry,std::uint32_t flags,
    const std::function<void(BurningRelease)>& release,const std::function<void(std::uint32_t)>& stopSound); // 실제423cd0/4240f0의 반납/소리/등록/base/객체 순서다.
}
