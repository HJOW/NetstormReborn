// 화면 배경 선택의 현재 전역·원문 signed 카운터/IDIV와 선택된 외부 효과만 처리한다.
#include "client/RawRendererClouds.h"
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client { // 픽셀 장치는 같은 원문 순서에서 호출자가 제공한다.
namespace { // 원문 signed 값과 도달한 장치의 계약을 공통 처리한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Renderer 구름의 선택된 실제 장치 미연결");return function; } //생략 경로에는 연결을 요구하지 않는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } //원문 DWORD 비트를 signed 비교로만 해석한다.
std::int32_t Divide(std::int32_t value,std::int32_t denominator) {
    if (!denominator || (value==std::numeric_limits<std::int32_t>::min() && denominator==-1)) throw std::domain_error("Renderer 구름의 원문 IDIV 오류");
    return value/denominator;
} //선택된 위치의 실제 나눗셈 오류를 C++ 미정의 동작이나 가짜0 성공으로 바꾸지 않는다.
ScreenRect CurrentRect(const ScreenRect& clipped,std::int32_t right,std::int32_t bottom) { return {clipped.left,clipped.top,right,bottom}; } //원문 EBP/EDI의 right/bottom은 먼저 보존하고 left/top만 호출 직전에 읽는다.
}

// 사각형4칸의 개수와 원문 진단 뒤 재조회를 보존하며 음수 index의 앞선 메모리 쓰기는 물리 계약 오류로 구별한다.
void AppendRendererCloudDirty(RendererCloudQueue queue,ScreenRect rect,const std::uint32_t& checking,const RendererCloudHooks& h) {
    if (checking && Signed(queue.count)>=4) Need(h.assertion)(0x40cU);
    const auto index=Signed(queue.count);if (index>=4) return;
    if (index<0) throw std::out_of_range("Renderer 구름4칸 앞의 미제공 물리 영역 쓰기");
    queue.rectangles[static_cast<std::size_t>(index)]=rect;++queue.count;
}
// 각 표시 효과 뒤 현재 count를 다시 읽으며 다음 사각형/카메라·분모는 다음 반복에서 조회한다.
void RefreshRendererCloudBackground(RendererCloudQueue queue,const std::int32_t& cameraX,const std::int32_t& cameraY,const std::int32_t& denominator,const RendererCloudHooks& h) {
    for (std::int32_t index=0;index<Signed(queue.count);++index) { //signed0/음수 개수는 표시 없이 마지막 count0만 수행한다.
        if (index>=4) throw std::out_of_range("Renderer 구름4칸 뒤의 미제공 물리 영역 읽기");
        const auto y=cameraY;const auto divisor=denominator;const auto dy=Divide(y,divisor);const auto dx=Divide(cameraX,divisor);
        Need(h.draw)(queue.rectangles[static_cast<std::size_t>(index)],dx,dy,true);
    }
    queue.count=0;
}
// debug 묶음 숨김의 검정 지우기는 UI 전용/배경 덮임 검사보다 먼저 실행하며 그 효과 뒤 현재 관문을 다시 읽는다.
void DrawRendererBackdrop(ScreenRect& clipped,const std::uint32_t& dirtyFlags,const std::uint32_t& covered,const RendererBackdropGlobals& g,const RendererCloudHooks& h) {
    const auto right=clipped.right,bottom=clipped.bottom;
    if (g.hideGroups) Need(h.clear)(CurrentRect(clipped,right,bottom),g.black);
    if ((dirtyFlags&4U) || covered) return;
    const auto flags=g.mode&255U;
    if (!(flags&8U)) { Need(h.clear)(CurrentRect(clipped,right,bottom),g.tint);return; }
    if ((flags&16U) && (flags&1U) && !(flags&2U)) { Need(h.clear)(CurrentRect(clipped,right,bottom),255);return; }
    std::int32_t y{},x{}; //두 경로 모두 원문의 Y→X 읽기/나눗셈 순서로 채운다.
    if (flags&16U) { const auto currentY=g.cameraY;const auto divisor=g.denominator;y=Divide(currentY,divisor);x=Divide(g.cameraX,divisor); }
    else { y=g.cameraY;x=g.cameraX; }
    Need(h.draw)(CurrentRect(clipped,right,bottom),x,y,false);
}
}
