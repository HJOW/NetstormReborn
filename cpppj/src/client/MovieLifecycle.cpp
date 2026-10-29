// 근거: 같은 PC의10.78 원문00483680/004837f0/00483bf0와 독립 CPU 관측이다.
#include "client/MovieLifecycle.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client { // 실제 화면/음향 수명과 원문 계산의 순서를 구별한다.
namespace { // 미선택 장치는 요구하지 않고 정수 좌표는DWORD로 감는다.
template<class F> const F& NeedLife(const F& callback) { if (!callback) throw std::logic_error("MovieGump 시작/종료/표시의 실제 장치 미연결");return callback; } // 도달한 장치만 확인한다.
std::int32_t LifeAdd(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a)+std::bit_cast<std::uint32_t>(b)); } // 원문 좌표/폭의 하위32비트다.
std::int32_t LifeFtol(double value) { // 원문 CRT의64비트 절삭 후 하위DWORD이며 비정상 범위의 하위값은0이다.
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
std::int32_t LifeCenter(std::int32_t first,std::int32_t last,std::int32_t width) { // Draw 원문은 음수 나머지에도 unsigned 논리 우측 이동을 쓴다.
    const auto gap=std::bit_cast<std::uint32_t>(last)-std::bit_cast<std::uint32_t>(width)-std::bit_cast<std::uint32_t>(first);
    return LifeAdd(first,static_cast<std::int32_t>(gap>>1));
}
}
// 장치 효과가 현재 Gump/SMK의 치수나 freeze를 바꾸면 그 뒤 원문 위치의 조회에 반영한다.
void BeginMovieLifecycle(MovieLifecycleState& state,const MovieLifecycleHooks& h) {
    if (!NeedLife(h.isOpen)()) NeedLife(h.assertion)(0xe6);
    NeedLife(h.capturePalette)(state.savedPalette);NeedLife(h.shutdownSound)();NeedLife(h.muteSound)();
    if (!NeedLife(h.hasBuffer)()) NeedLife(h.assertion)(0x102);
    NeedLife(h.bindBuffer)();auto size=NeedLife(h.videoSize)();state.playback.image={0,0,size.x,size.y};
    size=NeedLife(h.videoSize)();const auto window=NeedLife(h.windowSize)(); // 원문 setup은 영상 치수를 unsigned로 해석하여 뺀다.
    const auto y=LifeFtol((static_cast<double>(window[1])-std::bit_cast<std::uint32_t>(size.y))*0.5);
    const auto x=LifeFtol((static_cast<double>(window[0])-std::bit_cast<std::uint32_t>(size.x))*0.5);
    auto& rect=state.playback.image;rect={LifeAdd(rect.left,x),LifeAdd(rect.top,y),LifeAdd(rect.right,x),LifeAdd(rect.bottom,y)};
    NeedLife(h.idle)(true);if (state.playback.freeze) { NeedLife(h.pauseUpdates)(true);NeedLife(h.pauseGame)(true); }NeedLife(h.showCursor)(false);
}
// 정지 여부는 각 효과 뒤의 현재 옵션을 읽으며 팔레트 복원은 음향 재초기화보다 먼저다.
void EndMovieLifecycle(MovieLifecycleState& state,const MovieLifecycleHooks& h) {
    const auto bounds=NeedLife(h.bounds)();NeedLife(h.clear)(bounds,0);NeedLife(h.applyPalette)(state.savedPalette);
    if (NeedLife(h.soundEnabled)()) NeedLife(h.restoreSound)(3);
    NeedLife(h.unmuteSound)();NeedLife(h.idle)(false);NeedLife(h.redraw)(false);
    if (state.playback.freeze) { NeedLife(h.pauseUpdates)(false);NeedLife(h.pauseGame)(false); }NeedLife(h.refreshCursor)();
}
// 원문 union 판정으로 현재 clip의 바깥 여백을 채우고 원래 해상도의 버퍼만 복사한다.
void DrawMovieLifecycle(MovieLifecycleState& state,const MovieLifecycleHooks& h) {
    if (!NeedLife(h.isOpen)()) NeedLife(h.assertion)(0x205);
    const auto bounds=NeedLife(h.bounds)();const auto& source=state.playback.image;const ScreenRect image{LifeAdd(source.left,bounds.left),LifeAdd(source.top,bounds.top),LifeAdd(source.right,bounds.left),LifeAdd(source.bottom,bounds.top)};
    const auto clip=NeedLife(h.clip)();const ScreenRect combined{(std::min)(clip.left,image.left),(std::min)(clip.top,image.top),(std::max)(clip.right,image.right),(std::max)(clip.bottom,image.bottom)};
    if (combined.left!=image.left || combined.top!=image.top || combined.right!=image.right || combined.bottom!=image.bottom) NeedLife(h.fill)(clip,NeedLife(h.black)());
    if (state.playback.palettePending) { NeedLife(h.applyPalette)(state.playback.palette);state.playback.palettePending=0; }
    if (NeedLife(h.hasBuffer)()) { const auto size=NeedLife(h.videoSize)();NeedLife(h.blit)({LifeCenter(bounds.left,bounds.right,size.x),LifeCenter(bounds.top,bounds.bottom,size.y)},size); }
}
}
