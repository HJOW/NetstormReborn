// 원문 FILD/FMUL/FTOL/FSTP의 저장 경계와 현재 카메라 재조회를 보존한다.
#include "client/RawCameraView.h"
#include "client/RawCursorPosition.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 이 본문은 OS/음향/Renderer 장치를 실행하지 않는다.
namespace {
// 진단은 모든 선행 store 뒤 현재 view를 다시 읽으며 X 다음 Y 순서로 실행한다.
void Diagnose(RawCameraViewState& view,const CameraViewAssertion& assertion,std::uint32_t firstLine) {
    if (std::bit_cast<float>(view.worldBits[0])>=0.0F && view.pixels[0]<0) {
        if (!assertion) throw std::logic_error("카메라 X 원점 원문 진단 미연결");assertion("scrollPosPixX >= 0",firstLine);
    }
    if (std::bit_cast<float>(view.worldBits[1])>=0.0F && view.pixels[1]<0) {
        if (!assertion) throw std::logic_error("카메라 Y 원점 원문 진단 미연결");assertion("scrollPosPixY >= 0",firstLine+3);
    }
}
}
// 정수 indefinite도 하위DWORD0이므로 음수최소/NaN/범위밖을 잘못된 signed cast로 실행하지 않는다.
std::int32_t CameraFtolLow(double value) {
    if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 원문4973e0은 입력 분수값을 먼저 저장하고 픽셀 원점에서 역산하지 않는다.
void SetCameraViewWorld(RawCameraViewState& view,float x,float y,CameraViewAssertion assertion) {
    view.worldBits[0]=std::bit_cast<std::uint32_t>(x);view.worldBits[1]=std::bit_cast<std::uint32_t>(y);
    view.pixels[0]=CameraFtolLow(static_cast<double>(view.cellPixels[0])*static_cast<double>(x));
    view.pixels[1]=CameraFtolLow(static_cast<double>(view.cellPixels[1])*static_cast<double>(y));
    view.pixelFloatBits[0]=std::bit_cast<std::uint32_t>(static_cast<float>(view.pixels[0]));view.pixelFloatBits[1]=std::bit_cast<std::uint32_t>(static_cast<float>(view.pixels[1]));
    Diagnose(view,assertion,0x344);
}
// 원문497490의 Y/X store를 유지하고 기존 독립 검증된 정수→월드 float 몸체를 사용한다.
void SetCameraViewPixels(RawCameraViewState& view,std::int32_t x,std::int32_t y,CameraViewAssertion assertion) {
    view.pixels[1]=y;view.pixels[0]=x;
    const auto world=RawCursorPosition::CameraFromPixels(x,y);view.worldBits[0]=world[0];view.worldBits[1]=world[1];
    view.pixelFloatBits[0]=std::bit_cast<std::uint32_t>(static_cast<float>(x));view.pixelFloatBits[1]=std::bit_cast<std::uint32_t>(static_cast<float>(y));
    Diagnose(view,assertion,0x354);
}
// 전역BSS0과 구별되는 명시 Native Renderer 초기 이미지다. Session/호스트 장치 재생성에서 암묵 호출하지 않는다.
void InitializeCameraView(RawCameraViewState& view,CameraViewAssertion assertion) {
    view.baseCellFloatBits[1]=0x41300000U;view.cellFloatBits[1]=0x41300000U;view.baseCellFloatBits[0]=0x41800000U;view.cellFloatBits[0]=0x41800000U;
    view.baseCellPixels[1]=11;view.baseCellPixels[0]=16;view.zoom=0x10000;view.zoomFloatBits=0x3f800000U;view.cellPixels[0]=16;view.cellPixels[1]=11;
    SetCameraViewWorld(view,0.0F,0.0F,std::move(assertion));
}
// 현재 source의 모든16DWORD를 복사하고 source와 destination의 독립 수명을 유지한다.
void CopyCameraViewSnapshot(RawCameraViewState& destination,const RawCameraViewState& source) { if (&destination!=&source) std::memcpy(&destination,&source,sizeof(source)); }
// 두 축의 ftol 뒤 각각 현재 signed 픽셀 원점을 DWORD 감김으로 뺀다.
std::array<std::int32_t,2> ProjectCameraCoordinates(const std::int32_t& cameraX,const std::int32_t& cameraY,float x,float y) {
    const auto px=std::bit_cast<std::uint32_t>(CameraFtolLow(static_cast<double>(x)*16.0+0.5))-std::bit_cast<std::uint32_t>(cameraX);
    const auto py=std::bit_cast<std::uint32_t>(CameraFtolLow(static_cast<double>(y)*11.0+0.5))-std::bit_cast<std::uint32_t>(cameraY);
    return {std::bit_cast<std::int32_t>(px),std::bit_cast<std::int32_t>(py)};
}
// Audio.View 등의 호스트 복사본이 아닌 같은 전역 snapshot을 호출 때 읽는다.
std::array<std::int32_t,2> ProjectCameraView(const RawCameraViewState& view,float x,float y) { return ProjectCameraCoordinates(view.pixels[0],view.pixels[1],x,y); }
}
