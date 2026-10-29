// 근거: docs/exe/cpp-movie-playback-20261030.md와 같은 PC의 원본10.78 기계어다.
#include "client/MoviePlayback.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 영상 재생의 원문 판단과 장치 효과의 경계를 유지한다.
namespace { // 선택된 장치 누락을 빈 성공으로 대체하지 않는다.
template<class F> const F& NeedMovie(const F& callback) { if (!callback) throw std::logic_error("MovieGump의 선택된 실제 장치 미연결");return callback; } // 도달한 callback만 요구한다.
std::int32_t AddMovie(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a)+std::bit_cast<std::uint32_t>(b)); } // 원문 좌표 덧셈을32비트로 감는다.
}
// 원문 옵션의 정규화와 잘못된 번호의 로그 전용 처리를 수행한다.
void SetMoviePlaybackOption(MoviePlaybackState& state,std::uint32_t option,std::uint32_t value,const MoviePlaybackHooks& hooks) {
    if (option==1) state.autoKill=value!=0;else if (option==2) state.freeze=value!=0;else NeedMovie(hooks.invalidOption)(option);
}
// 종료 후에 현재 autoKill을 다시 읽으므로 종료 콜백의 변경도 반영한다.
std::uint32_t HandleMoviePlayback(MoviePlaybackState& state,std::uint32_t event,const MoviePlaybackHooks& hooks) {
    const auto code=event&0x03ffffffU; // 원문은 modifier는 지우되 release bit02000000은 남긴다.
    const bool pointer=code==0x10000U || code==0x20000U; // 버튼 누름은 키의 release 관문과 별도로 처리한다.
    const bool key=!(event&0x02000000U) && (code==0x1bU || code==0x20U); // ESC와 Space의 뗌은 종료하지 않는다.
    if (!pointer && !key) return 0;
    NeedMovie(hooks.end)();if (state.autoKill) NeedMovie(hooks.kill)();return 1;
}
// RGB의 모든 성분/예약BYTE를 먼저 쓰고 진단이 복귀해야 pending을 공개한다.
void UpdateMoviePlaybackPalette(MoviePlaybackState& state,std::span<const std::uint8_t,768> rgb,const MoviePlaybackHooks& hooks) {
    // 원문의256색 모두를 매번 복사하며 이전 예약BYTE를 보존하지 않는다.
    for (std::size_t index=0;index<state.palette.size();++index) state.palette[index]={rgb[index*3+2],rgb[index*3+1],rgb[index*3],0};
    NeedMovie(hooks.paletteLog)();state.palettePending=1;
}
// 한 idle에 최대 한 프레임을 해제하며 SDK 대기/종료 앞뒤의 현재값을 각각 조회한다.
void IdleMoviePlayback(MoviePlaybackState& state,std::uint32_t& dirtyOverride,const MoviePlaybackHooks& hooks) {
    if (!NeedMovie(hooks.atEnd)()) {
        if (NeedMovie(hooks.wait)()) return;
        if (NeedMovie(hooks.paletteChanged)()) UpdateMoviePlaybackPalette(state,NeedMovie(hooks.palette)(),hooks);
        NeedMovie(hooks.doFrame)();const auto origin=NeedMovie(hooks.origin)();dirtyOverride=1;
        // 현재 디코더의 바뀐 사각형을 끝까지 읽는다. callback 뒤 현재 영상 내부 위치도 다시 읽는다.
        while (const auto rect=NeedMovie(hooks.nextRect)()) {
            const auto x=AddMovie(state.image.left,origin.x),y=AddMovie(state.image.top,origin.y);
            NeedMovie(hooks.invalidate)({AddMovie(rect->left,x),AddMovie(rect->top,y),AddMovie(rect->right,x),AddMovie(rect->bottom,y)},4);
        }
        dirtyOverride=0;
        if (!NeedMovie(hooks.atEnd)()) { NeedMovie(hooks.nextFrame)();return; }
    }
    NeedMovie(hooks.end)();if (state.autoKill) NeedMovie(hooks.kill)();
}
}
