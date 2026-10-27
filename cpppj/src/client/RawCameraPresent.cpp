// 원문 signed 비교·세계 크기의 하위32비트 곱/뺄셈·FTOL 저장과 호출 위치를 유지한다.
#include "client/RawCameraPresent.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client { // OS 출력과 독립인 원문 목표 제한 본체다.
namespace { // 원문 계산과 선택된 외부 호출만 공통 처리한다.
// 실제 호출 지점에 도달한 장치만 요구하며 진단의 조기 반환을 앞서 바꾸지 않는다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("카메라 Present 현재 장치 미연결");return callback; }
// 원문은 signed 최소값→ROM0과 비교→FTOL 순서다. int32 입력은 double에서 정확히 표현된다.
std::int32_t Limit(std::int32_t value,std::int32_t maximum) { return CameraFtolLow(static_cast<double>(std::max(0,std::min(value,maximum)))); }
// IMUL/SHL/SUB의 하위32비트 결과를 signed overflow 없이 같은 signed 상한으로 해석한다.
std::int32_t WorldLimit(std::int32_t cells,std::uint32_t scale,std::int32_t screen) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(cells)*scale-static_cast<std::uint32_t>(screen)); }
}
// 원문은 진단을 한 번만 읽고 flip 뒤의 현재 한계·목표·세계/화면 크기를 순서대로 사용한다.
void PresentCameraTarget(RawCameraTargetState& target,const RawCameraPresentGlobals& globals,const RawCameraPresentHooks& hooks) {
    if (globals.diagnostic==1U) return;
    Need(hooks.flip)();
    target.pixels[0]=Limit(target.pixels[0],target.limits[0]);target.pixels[1]=Limit(target.pixels[1],target.limits[1]);
    const auto maximumX=WorldLimit(globals.worldCells,16U,globals.width),maximumY=WorldLimit(globals.worldCells,11U,globals.height); // 현재 zoom의 Renderer limit과 별도로 원문 기본16/11 세계 한계를 적용한다.
    target.pixels[0]=Limit(target.pixels[0],maximumX);target.pixels[1]=Limit(target.pixels[1],maximumY);
    Need(hooks.commit)();
}
}
