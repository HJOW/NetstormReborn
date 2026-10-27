// 실제 원문의 이동/시차·두 setter·dirty 확장·Q16 저장과 외부 효과 뒤 재조회를 보존한다.
#include "client/RawCameraCommit.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client { // OS/게임 실행 없이도 같은 카메라 적용 본체를 사용할 수 있다.
namespace { // 원문 DWORD 감김과 선택된 호출만 공통 처리한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("카메라 적용 현재 장치 미연결");return callback; } // 실제로 선택된 외부 경계만 요구한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b)); } // ADD 하위32비트다.
std::int32_t Sub(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b)); } // SUB/NEG 하위32비트다.
std::int32_t Divide(std::int32_t value,std::int32_t denominator) { // 원문 IDIV의 두 오류를 C++ 미정의 동작으로 실행하지 않고 선택된 위치에서 거부한다.
    if (!denominator || (value==std::numeric_limits<std::int32_t>::min() && denominator==-1)) throw std::domain_error("카메라 시차 원문 IDIV 오류");return value/denominator;
}
std::int32_t Scale(std::int32_t base,std::uint32_t zoom) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(base)*zoom)>>16; } // IMUL 하위32비트 뒤 signed SAR16이다.
std::uint32_t FloatBits(std::int32_t value) { return std::bit_cast<std::uint32_t>(static_cast<float>(value)); } // 원문 FILD32/FSTP32 저장이다.
// 원문의 두 노출 띠를 계산하며 음수 길이일 때 끝 좌표 자체를 NEG하는 순서도 보존한다.
template<class F> void Exposed(const RawCameraCommitViewport& viewport,std::int32_t dx,std::int32_t dy,const F& emit) {
    const auto left=dx>0 ? Add(Sub(viewport.x,dx),viewport.width) : viewport.x;
    auto right=dx>0 ? Add(viewport.x,viewport.width) : Sub(viewport.x,dx);
    const auto top=dy>0 ? Add(Sub(viewport.y,dy),viewport.height) : viewport.y;
    auto bottom=dy>0 ? Add(viewport.y,viewport.height) : Add(viewport.y,dy);
    const auto horizontal=Sub(right,left),vertical=Sub(bottom,top);
    if (horizontal) { if (horizontal<0) right=Sub(0,right);emit(ScreenRect{left,viewport.y,right,Add(viewport.y,viewport.height)}); }
    if (vertical) { if (vertical<0) bottom=Sub(0,bottom);emit(ScreenRect{viewport.x,top,Add(viewport.x,viewport.width),bottom}); }
}
}
// zoom 분기와 일반 이동 분기를 원문과 같이 분리하고 모든 외부 효과 뒤 현재 저장소를 다시 읽는다.
void CommitCameraTarget(RawCameraCommitState state,const RawCameraCommitGlobals& globals,const RawCameraCommitHooks& hooks) {
    auto& view=state.renderer;auto& snapshot=state.snapshot;auto& target=state.target;
    if (target.zoom!=view.zoom) {
        Need(hooks.redrawRoots)();const auto zoom=target.zoom;
        view.cellPixels[0]=Scale(view.baseCellPixels[0],zoom);view.zoomFloatBits=std::bit_cast<std::uint32_t>(static_cast<float>(std::bit_cast<std::int32_t>(zoom))*0x1p-16F);
        view.cellFloatBits[0]=FloatBits(view.cellPixels[0]);view.zoom=zoom;view.cellPixels[1]=Scale(view.baseCellPixels[1],zoom);view.cellFloatBits[1]=FloatBits(view.cellPixels[1]);
        snapshot.zoomFloatBits=std::bit_cast<std::uint32_t>(static_cast<float>(std::bit_cast<std::int32_t>(zoom))*0x1p-16F);
        snapshot.cellPixels[0]=Scale(snapshot.baseCellPixels[0],zoom);snapshot.cellPixels[1]=Scale(snapshot.baseCellPixels[1],zoom);snapshot.zoom=zoom;
        snapshot.cellFloatBits[0]=FloatBits(snapshot.cellPixels[0]);snapshot.cellFloatBits[1]=FloatBits(snapshot.cellPixels[1]);
        SetCameraViewPixels(view,target.pixels[0],target.pixels[1],hooks.assertion);SetCameraViewPixels(snapshot,target.pixels[0],target.pixels[1],hooks.assertion);
        Need(hooks.redrawRoots)();state.worldRedraw=1;return;
    }
    if (target.pixels!=view.pixels) { Need(hooks.lockCursor)(0);Need(hooks.redrawRoots)(); }
    const auto dy=Sub(target.pixels[1],view.pixels[1]),dx=Sub(target.pixels[0],view.pixels[0]);
    Need(hooks.scroll)(globals.viewport.x,globals.viewport.y,globals.viewport.width,globals.viewport.height,dx,dy);
    if (globals.viewport.x>0) Need(hooks.scroll)(0,0,globals.viewport.x,globals.height,0,0);
    if (globals.viewport.height<globals.height) Need(hooks.scroll)(globals.viewport.x,globals.viewport.height,Sub(globals.width,globals.viewport.x),Sub(globals.height,globals.viewport.height),0,0);
    if (globals.screenFlags&16U) {
        if (!(globals.screenFlags&2U)) {
            const auto oldX=Divide(view.pixels[0],globals.denominator),newX=Divide(target.pixels[0],globals.denominator);
            const auto oldY=Divide(view.pixels[1],globals.denominator),newY=Divide(target.pixels[1],globals.denominator);const auto sx=Sub(newX,oldX),sy=Sub(newY,oldY);
            Need(hooks.scrollBackground)(globals.viewport.x,globals.viewport.y,globals.viewport.width,globals.viewport.height,sx,sy);
            Exposed(globals.viewport,sx,sy,[&](ScreenRect rect) { Need(hooks.addBackgroundDirty)(rect); }); // 원문4977f0의 현재 두 노출 띠다.
        } else if (dx || dy) Need(hooks.addDirty)(globals.fullViewport,0);
    }
    SetCameraViewPixels(view,target.pixels[0],target.pixels[1],hooks.assertion);SetCameraViewPixels(snapshot,target.pixels[0],target.pixels[1],hooks.assertion);
    if (!dx && !dy) return;
    Need(hooks.updateMouse)(globals.mouse);
    for (std::size_t index=0;index<state.dirty.size();++index) { // 마우스 효과 뒤 현재 표와 매 반복의 현재 개수를 읽는다.
        auto& entry=state.dirty[index];if (entry.flags&1U) continue;const auto old=entry.rect;
        const ScreenRect shifted{Sub(old.left,dx),Sub(old.top,dy),Sub(old.right,dx),Sub(old.bottom,dy)};
        if (entry.flags&2U) entry.rect={std::min(old.left,shifted.left),std::min(old.top,shifted.top),std::max(old.right,shifted.right),std::max(old.bottom,shifted.bottom)};
        else entry.rect=shifted;
    }
    Exposed(globals.viewport,dx,dy,[&](ScreenRect rect) { Need(hooks.addDirty)(rect,0); }); // 원문497580의 현재 노출 띠 추가다.
    Need(hooks.unlockCursor)();Need(hooks.redrawRoots)();state.redraw=1;
}
}
