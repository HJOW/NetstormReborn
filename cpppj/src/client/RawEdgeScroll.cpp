// 원본 가장자리의2픽셀 관문·시간 가속·버튼 상태와 카메라 목표 쓰기를 보존한다.
#include "client/RawEdgeScroll.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::client { // OS/Renderer 출력과 독립적인 원문 입력 본체다.
namespace { // 선택된 장치와32비트 정수 계산만 공통 처리한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("가장자리 스크롤 현재 장치 미연결");return callback; } // 실제 호출되는 장치만 요구한다.
std::int32_t Add(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(first)+static_cast<std::uint32_t>(second)); } // 원본 ADD의 하위32비트이며 signed overflow를 실행하지 않는다.
}
// 전역 flags는 입력 앞단과 공유하며 이 구간 밖의 포인터 타이머 비트8 등은 보존한다.
std::array<float,2> ProcessEdgeScroll(RawEdgeScrollState state,const RawEdgeScrollInput& input,const RawEdgeScrollHooks& hooks) {
    std::array<float,2> direction{};
    if (!(state.initializationFlags&4U)) { state.initializationFlags|=4U;state.since=0.0; }
    if (!input.enabled || input.borderX || input.borderY || input.dialog) return direction;
    if (!(Need(hooks.poll)(InputCode::kLeftButton).code&InputCode::kRelease)) return direction;
    if (input.x<2) direction[0]=-1.0F;
    if (input.y<2 && !Need(hooks.menu)(input.menuSerial)) direction[1]=-1.0F;
    if (input.x>=Add(input.width,-1)) direction[0]=1.0F;
    if (input.y>=Add(input.height,-1)) direction[1]=1.0F;
    if ((direction[0]==0.0F && direction[1]==0.0F) || Need(hooks.blockingPopup)()) { state.since=0.0;return direction; }
    if (state.since==0.0) state.since=input.wall;
    const double elapsed=input.wall-state.since;const double accelerated=elapsed*30.0; // 원문 FSUB→FMUL→FADD의 저장 정밀도를 분리하며 FMA로 합치지 않는다.
    const auto amount=static_cast<float>(std::min(input.speed,CameraFtolLow(accelerated+2.0)));
    Need(hooks.closePopups)();
    if (!(Need(hooks.poll)(0x00100000U).code&InputCode::kRelease)) {
        if (std::abs(CameraFtolLow(direction[1]))<std::abs(CameraFtolLow(direction[0]))) direction[1]=0.0F;
        else direction[0]=0.0F;
    }
    const auto dy=CameraFtolLow(static_cast<double>(amount)*direction[1]);const auto dx=CameraFtolLow(static_cast<double>(amount)*direction[0]);
    Need(hooks.request)(dx,dy);return direction;
}
// 목표는 현재 Renderer의1c/20을 사용하며 이전 목표50/54에 이동량을 누적하지 않는다.
void RequestCameraDelta(const RawCameraViewState& view,RawCameraTargetState& target,std::int32_t height,std::int32_t dx,std::int32_t dy,std::uint32_t& metaRebuild) {
    const auto half=height/2;target.pixels[0]=Add(view.pixels[0],dx);
    target.pixels[1]=Add(Add(Add(half,view.pixels[1]),dy),std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(half)));++metaRebuild;
}
}
