// x87 정수 절삭과 float 중간 저장 위치를 보존한다.
#include "client/MinimapGeometry.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 bitmap의 한 변이다. 화면의 crop 크기와 구별한다.
constexpr double kBitmapSide=128.0;
// 원본 ftol의 signed64 결과 하위DWORD이며 integer-indefinite의 하위값은0이다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 원본 signed /의32BIT 감김을 host UB 없이 처리한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
// DWORD 뺄셈도 하위32BIT로 감아 극단 카메라 입력의signed UB를 피한다.
std::int32_t Subtract(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b)); }
// 카메라의 현재float 절삭과 viewport 원점의 정수 나눗셈을 먼저 더한 뒤2로 나눈다.
std::array<std::int32_t,2> Center(const MinimapGeometryView& view) {
    if (view.pixelsX<=0 || view.pixelsY<=0) throw std::invalid_argument("미니맵의 유효한 현재 칸 배율이 필요합니다");
    return {Add(Truncate(std::bit_cast<float>(view.camera[0])),view.viewport.left/view.pixelsX)/2,
        Add(Truncate(std::bit_cast<float>(view.camera[1])),view.viewport.top/view.pixelsY)/2};
}
// 지도 영역 폭/높이를 현재 픽셀 배율의 두 배로 signed 나눈다.
std::array<std::int32_t,2> Visible(const MinimapGeometryView& view) {
    return {(view.viewport.right-view.viewport.left)/(view.pixelsX*2),(view.viewport.bottom-view.viewport.top)/(view.pixelsY*2)};
}
}
// 자동 추적을 멈추는 조건은+a8이며 전투/정지 상태를 추가하지 않는다.
bool RebuildMinimap(MinimapGeometryState& state,const MinimapGeometryView& view) {
    if (state.scrolling) return false;
    const auto center=Center(view),visible=Visible(view); // signed 정수 연산이 끝난 원본 중간값이다.
    const auto candidateX=Subtract(center[0],Truncate((double(state.width)-visible[0])*0.5)); // 원본은 여백 절반을 먼저ftol하고 center에서 signedDWORD로 뺀다.
    const auto candidateY=Subtract(center[1],Truncate((double(state.height)-visible[1])*0.5)); // 두 번째 축도 여백만 먼저 절삭한다.
    state.cropX=Truncate(std::max(0.0,std::min(double(static_cast<float>(candidateX)),kBitmapSide-double(state.width))));
    state.cropY=Truncate(std::max(0.0,std::min(double(static_cast<float>(candidateY)),kBitmapSide-double(state.height))));return true;
}
// 호출자가 실제497f20을 실행한다. 여기서 화면 픽셀로 바꾸거나 가장자리를clamp하지 않는다.
std::array<float,2> MinimapTarget(const MinimapGeometryState& state,float localX,float localY) {
    return {static_cast<float>((double(state.cropX)+localX)*2.0),static_cast<float>((double(state.cropY)+localY)*2.0)}; // FILD signed32→FADD local→두 배 뒤 마지막float 저장만 수행한다.
}
// 좌/상단은<=0, 우측은>=width, 하단은>=height-1인 원본의 끝점 포함 비교다.
std::array<float,2> DragMinimap(MinimapGeometryState& state,float localX,float localY) {
    if (localX<=0) state.cropX=std::max(0,Add(state.cropX,-4));
    if (state.width<=localX) state.cropX=Truncate(std::min(double(static_cast<float>(Add(state.cropX,4))),kBitmapSide-double(state.width)));
    if (localY<=0) state.cropY=std::max(0,Add(state.cropY,-4));
    if (double(state.height)-1.0<=double(localY)) state.cropY=Truncate(std::min(double(static_cast<float>(Add(state.cropY,4))),kBitmapSide-double(state.height)));
    return MinimapTarget(state,localX,localY);
}
// 원본은 음수 crop을 별도로0으로 바꾸지 않는다.
void ClampMinimapAttachment(MinimapGeometryState& state) {
    if (kBitmapSide<double(static_cast<float>(state.cropX))+state.width) state.cropX=Truncate(kBitmapSide-double(state.width));
    if (kBitmapSide<double(static_cast<float>(state.cropY))+state.height) state.cropY=Truncate(kBitmapSide-double(state.height));
}
// 블릿 크기와 카메라 크기를 다르게 계산하므로 임의 비례 축소를 넣지 않는다.
MinimapDrawPlan PlanMinimapDraw(const MinimapGeometryState& state,const MinimapGeometryView& view,ScreenPoint destination) {
    const auto center=Center(view),visible=Visible(view); // camera와viewport의 원본 정수 중간값이다.
    const auto left=Add(destination.x,Subtract(center[0],state.cropX)),top=Add(destination.y,Subtract(center[1],state.cropY)); // 대상 원점에서crop을 뺀 outline 원점이다.
    return {{state.cropX,state.cropY},destination,{Truncate(std::min(kBitmapSide,double(state.width))),Truncate(std::min(kBitmapSide,double(state.height)))},{left,top,Add(left,visible[0]),Add(top,visible[1])}};
}
}
