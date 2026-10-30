// 원문 초기화/프레임 복사와 호스트 화면 전달을 같은 장기 데이터에서 분리한다.
#include "client/ClientCameraGlobals.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // OS 장치가 없는 독립 검사도 같은 데이터 본문을 사용한다.
// Native constructor의 기본 view→한계/목표→snapshot→redraw1→mapRevision++ 순서다.
void ClientCameraGlobals::InitializeNativeRenderer(std::int32_t width,std::int32_t height,std::int32_t worldCells,std::uint32_t& worldRedraw,std::uint32_t& mapRevision) {
    if (width<=0 || height<=0) throw std::invalid_argument("Native Renderer 초기 화면 크기 오류");
    InitializeCameraView(rendererView_);const auto cells=static_cast<float>(worldCells); // 원문이 두 축에 재사용하는 저장된 단정도 world 크기다.
    target_.limits[0]=CameraFtolLow(static_cast<double>(rendererView_.cellPixels[0])*cells-static_cast<double>(width));
    target_.zoom=rendererView_.zoom; // 실제PE +48→+58→+4c이며497f80의 두limit store와 구별한다.
    target_.limits[1]=CameraFtolLow(static_cast<double>(rendererView_.cellPixels[1])*cells-static_cast<double>(height));
    target_.pixels[0]=0;target_.pixels[1]=0;CopyCameraViewSnapshot(snapshot_,rendererView_);worldRedraw=1;++mapRevision;
}
// FILD32×입력float−현재화면정수의 중간 계산을 먼저 float로 축소하지 않는다.
void ClientCameraGlobals::UpdateTargetLimits(std::int32_t width,std::int32_t height,float worldWidth,float worldHeight) {
    target_.limits[0]=CameraFtolLow(static_cast<double>(rendererView_.cellPixels[0])*worldWidth-static_cast<double>(width));
    target_.limits[1]=CameraFtolLow(static_cast<double>(rendererView_.cellPixels[1])*worldHeight-static_cast<double>(height));
}
// 목표 변경은 프레임의 적용 위치까지 기존 현재 Renderer/snapshot 값을 보존한다.
void ClientCameraGlobals::RequestPixels(std::int32_t x,std::int32_t y) { target_.pixels[0]=x;target_.pixels[1]=y; }
// PATCH497f20의 음수16/11 곱→FTOL 하위DWORD→NEG/SUB를 따르며 화면 가장자리 제한은 뒤 Present의 별도 경로다.
void ClientCameraGlobals::RequestCenter(float x,float y,std::int32_t width,std::int32_t height,std::uint32_t& revision) {
    target_.pixels[0]=std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(CameraFtolLow(-16.0*static_cast<double>(x)))-static_cast<std::uint32_t>(width/2));
    target_.pixels[1]=std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(CameraFtolLow(-11.0*static_cast<double>(y)))-static_cast<std::uint32_t>(height/2));
    ++revision; // 두 목표store 뒤에만 같은005c85dc를 증가시킨다.10.72의458800은 중간에 증가하므로 이 본체로 공통화하지 않는다.
}
// 즉시 중심 이동의 부모를 같은 목표/공유 전역에 적용한다. 미연결 commit도 원문 호출 위치의 선행 쓰기는 보존한다.
void ClientCameraGlobals::CenterImmediately(float x,float y,std::int32_t width,std::int32_t height,std::uint32_t& worldRedraw,std::uint32_t& revision,const std::function<void()>& commit) {
    ++worldRedraw;RequestCenter(x,y,width,height,revision);
    if (!commit) throw std::logic_error("즉시 카메라의 실제497960 장치 미연결");
    commit();
}
// 두 setter의 실제 순서만 공유하며 Native497960의 나머지 효과를 noop로 채우지 않는다.
void ClientCameraGlobals::ApplyPixelsToRendererAndSnapshot(std::int32_t x,std::int32_t y) { SetCameraViewPixels(rendererView_,x,y);SetCameraViewPixels(snapshot_,x,y); }
// 현재 논리 Renderer를 프레임 말미에 복사한다. Session 소멸이나 호스트Renderer 생성은 이 위치가 아니다.
void ClientCameraGlobals::CopyRendererSnapshot() { CopyCameraViewSnapshot(snapshot_,rendererView_); }
// 원문 C표시와 달리 실제 PE는 현재분모를 먼저 저장하고 WorldRedraw1을 쓴다.
void ClientCameraGlobals::SelectDenominator(std::int32_t value,std::uint32_t& worldRedraw) { if (!value) value=denominator_.defaultValue;if (value!=denominator_.current) { denominator_.current=value;worldRedraw=1; } }
}
