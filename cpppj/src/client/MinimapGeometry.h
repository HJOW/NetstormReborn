// 원본 Sky Overview의 crop·프레임 사각형·바깥 드래그 계산이다.
#pragma once
#include "client/Screen.h"

namespace netstorm::client {
// 실제 minmapGump의 크기 및+a0/+a4/+a8 확장 필드다.
struct MinimapGeometryState {
    float width{128},height{128}; // 배율 적용 전 원본+3c/+40 크기다.
    std::int32_t cropX{},cropY{}; // 공유128×128 bitmap에서 잘라낼 원점이다.
    std::uint32_t scrolling{}; // 실제 눌림 동안만 Rebuild가 crop 자동 추적을 중단한다.
};
// 원본 카메라 float와 지도 표시 사각형/칸 배율을 같은 시점에 읽는다.
struct MinimapGeometryView {
    std::array<std::uint32_t,2> camera{}; //0059a900/904의 현재float 비트다.
    ScreenRect viewport{}; // 지도 영역의 left/top/right/bottom이다.
    std::int32_t pixelsX{16},pixelsY{11}; //0059a908/90c의 현재 signed 칸 배율이다.
};
// 실제 bitmap 블릿 원점과 카메라 outline의 right/bottom을 제외하는 사각형이다.
struct MinimapDrawPlan {
    ScreenPoint source{},destination{}; // 원본 source crop와 대상 Gump의 화면 원점이다.
    ScreenPoint size{}; // ftol한 Gump의 가로·세로 블릿 크기다.
    ScreenRect camera{}; // 004a24b0의 오른쪽/아래 제외 좌표이며 화면 clip은 호출자가 적용한다.
};
// 원본00480270: 드래그 중에는 자연 반환하고 나머지는 현재 카메라 주변 crop을 계산한다.
bool RebuildMinimap(MinimapGeometryState& state,const MinimapGeometryView& view);
// 원본004803f0: signed crop+local을 확장 정밀도로 두 배한 뒤 float로 저장하는 카메라 목표다.
std::array<float,2> MinimapTarget(const MinimapGeometryState& state,float localX,float localY);
// 원본00480440: 경계 밖에서 crop을4픽셀씩 이동한 뒤 같은 현재 좌표의 목표를 계산한다.
std::array<float,2> DragMinimap(MinimapGeometryState& state,float localX,float localY);
// 원본004805e0: 부착 전 crop+크기가128을 넘는 축의 crop만 절삭해 보정한다.
void ClampMinimapAttachment(MinimapGeometryState& state);
// 원본00480660의 crop 블릿과 실제 지도 viewport outline 좌표다.
MinimapDrawPlan PlanMinimapDraw(const MinimapGeometryState& state,const MinimapGeometryView& view,ScreenPoint destination);
}
