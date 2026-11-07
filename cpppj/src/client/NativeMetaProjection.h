// 메타 표시의 원근 카메라·최초 기준 배율·같은 raw 객체의 배율 필드를 복원한다.
#pragma once
#include "client/RawCameraView.h"
#include "client/Screen.h"
#include <array>
#include <span>

namespace netstorm::client { // 원문 AngleCamera와 Metadisplay의 계산만 공유하며 화면/통신을 실행하지 않는다.
struct MetaProjectionCamera { // AngleCamera의 세 float와 애니메이션 DWORD를 원문 필드 순서로 보관한다.
    float sine{},cosine{}; //00544d6c/70의 실제 기울기이며 기본 생성자는 원문 각도의 sin/cos를 저장한다.
    float height{};std::uint32_t animating{}; //00544d74의 높이와00544d78의 현재 CameraRig 개수다. 높이 setter는 개수를 바꾸지 않는다.
};
MetaProjectionCamera CreateMetaProjectionCamera(); //00419570의 고정 각도·두0 저장을 수행하며 OS/CS 객체 생성은 하지 않는다.
struct MetaProjectionState { // Client 수명의 카메라와 최초 보정 전역을 CS 기록과 구별해 보관한다.
    MetaProjectionCamera camera{CreateMetaProjectionCamera()}; // 같은 Client의 장기 카메라이며 CS Reset/Session 교체로 재생성하지 않는다.
    std::uint32_t calibrated{}; //005930a0의 최초 BSS0이며 기준 투영/진단/저장이 끝나면1이다.
    float referenceHeight{22.0F},referenceScale{1.0F}; //00540adc/ae0의 보호 PE 초기22/1이며 최초 보정은60/계산된 배율을 쓴다.
};
struct MetaProjectionView { // 투영 한 번에 필요한 현재 Renderer/viewport/화면 크기의 실제 주소를 빌린다.
    const RawCameraViewState& renderer; // Native 현재 Renderer 앞64BYTE이며 전역 snapshot 사본으로 대체하지 않는다.
    const ScreenRect& viewport; //005ca9cc/d0/d4/d8의 현재 경계 네 DWORD다.
    const std::int32_t& width;const std::int32_t& height; //005c78f0/f4의 실제 화면 크기이며 viewport 폭/높이와 구별한다.
};
struct MetaProjectedPoint { // 원근 좌표·배율과 후행 UI 클리핑 결과를 호출자에게 함께 반환한다.
    std::array<float,2> position{}; //00418a30/479680이 반환하는 두 저장된 float 좌표다.
    std::uint32_t clipped{}; // 투영 평면 및 메타 UI 배율 범위 판정을 차례로 반영하는 실제0/1이다.
    float scale{}; // 원문 float 출력 배율이며 raw의16.16 저장값과 별개다.
};
void SetMetaProjectionHeight(MetaProjectionCamera& camera,float height); //004189f0의+8만 저장하고 기울기/애니메이션 개수를 보존한다.
std::int32_t MetaProjectionDenominator(const MetaProjectionCamera& camera); //00418a00의 높이20 미만 공식/FTOL 하위DWORD이며 그 밖은1이다.
std::array<float,2> MetaViewportCenter(const MetaProjectionView& view); //00497e50의 경계합/2→현재 픽셀 더하기→현재 칸 float 나누기다.
std::array<float,2> MetaScreenCenter(const MetaProjectionView& view); //00497ec0의 화면크기/2와 같은 현재 Renderer 픽셀/칸으로 계산한다.
MetaProjectedPoint ProjectMetaCoordinates(const MetaProjectionCamera& camera,const MetaProjectionView& view,float x,float y,float z); //00418a30의 원근/평면 클리핑과 명시 float 저장 위치를 보존한다.
void CalibrateMetaProjection(MetaProjectionState& state,const MetaProjectionView& view,const CameraViewAssertion& assertion); //00479760 첫 분기의 임시 카메라/0,0,4 투영→진단→기준배율/높이60/flag1이다.
MetaProjectedPoint ProjectMetaObject(std::span<std::uint8_t> raw,const MetaProjectionState& state,const MetaProjectionView& view,const std::uint32_t& screenFlags,float x,float y,float z,bool normalize); //00479680의 실제raw+8 WORD/+1a DWORD 쓰기와 뒤쪽 UI 범위 판정이다.
}
