#include "client/PaletteFlash.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본의 signed 32비트 덧셈·256 상한 비교·byte 저장을 유지한다. 음수 밝기는 아래로 제한하지 않고 byte로 감긴다.
std::uint8_t Brighten(std::uint8_t channel,std::int32_t offset) {
    const auto sum=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(channel)+static_cast<std::uint32_t>(offset));
    return sum<256 ? static_cast<std::uint8_t>(sum) : 255;
}
// 원래 색보다 작으면 1 증가, 크면 1 감소한다. 예약 바이트는 수정하지 않는다.
void Approach(std::uint8_t& channel,std::uint8_t original) {
    if (channel<original) ++channel;else if (channel>original) --channel;
}
}
// 빈 경계와 원본 배열 밖의 입력은 안전하게 거부한다.
PaletteFlash::PaletteFlash(PaletteFlashState& state,PaletteFlashHooks hooks):state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.wallSeconds || !hooks_.fullScreen || !hooks_.readPalette || !hooks_.applyPalette || !hooks_.restorePalette)
        throw std::invalid_argument("Incomplete palette flash hooks");
    if (state_.start>256 || state_.count>256-state_.start) throw std::out_of_range("Palette flash color range");
}
// 초기화에서 표시하지 않는다. 첫 Frame이 시간 진행 뒤 변경 색을 적용한다.
void PaletteFlash::Init() {
    state_.last=hooks_.wallSeconds();state_.interval=state_.duration/64.0;
    hooks_.readPalette(state_.start,state_.count,state_.original.data());
    hooks_.readPalette(state_.start,state_.count,state_.current.data());state_.initialized=true;
    // 부분 팔레트만 변경한다. RGBQUAD의 네 번째 예약 바이트는 기존 값을 보존한다.
    for (unsigned i=0;i<state_.count;++i) {
        auto& color=state_.current[i];
        if (state_.offset==0) color=state_.solid;
        else { color.red=Brighten(color.red,state_.offset);color.green=Brighten(color.green,state_.offset);color.blue=Brighten(color.blue,state_.offset); }
    }
    state_.steps=kThunderSteps;
}
// 지연된 프레임에서는 남은 단계를 한꺼번에 진행하되 last에 interval을 반복 더하는 원본 반올림 순서를 유지한다.
bool PaletteFlash::Frame() {
    if (!state_.initialized) return false;
    double elapsed=hooks_.wallSeconds()-state_.last;
    // 정확히 interval인 시각에서는 진행하지 않는다. 한 번에 최대 남은 단계만 실행하므로 0/음수 간격에서도 끝난다.
    while (state_.interval<elapsed && state_.steps>0) {
        // 각 색의 세 채널을 독립적으로 원래 값에 접근시킨다.
        for (unsigned i=0;i<state_.count;++i) {
            auto& color=state_.current[i];const auto& original=state_.original[i];
            Approach(color.red,original.red);Approach(color.green,original.green);Approach(color.blue,original.blue);
        }
        --state_.steps;state_.last+=state_.interval;elapsed-=state_.interval;
    }
    if (state_.steps<1 || state_.steps>kThunderSteps) {
        if (hooks_.fullScreen()) hooks_.restorePalette();
        state_.initialized=false;return false;
    }
    if (hooks_.fullScreen()) hooks_.applyPalette(state_.start,state_.count,state_.current.data());
    return true;
}
// 종료 프레임은 initialized를 먼저 끄므로 파괴 때 두 번째 복구가 일어나지 않는다.
void PaletteFlash::Cancel() {
    if (state_.initialized && hooks_.fullScreen()) hooks_.restorePalette();
    state_.initialized=false;
}
}
