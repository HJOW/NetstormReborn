// 파일 팔레트 읽기 뒤 원본004a4923..004a49a2의 clear 관문과 하위 순서만 소유한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::client {
// 실제 Screen이 제공하는 하위 장치다. GDI 자연 생략과 미복원 DirectDraw 오류는 각 하위가 결정한다.
struct ScreenPaletteApplyHooks {
    std::function<void()> lock; //004a14e0이며 중첩 깊이와 같은 현재 draw surface를 사용한다.
    std::function<void()> fillBlack; //004a2060(0,0,현재width,현재height,0)이다. lock 뒤 현재 크기를 읽는다.
    std::function<void()> unlock; //현재깊이--/0일때현재표면Unlock/복귀뒤buffer·pitch0을 실제Screen이 소유한다.
    std::function<void()> waitVerticalBlank; //004a1fb0이다. GDI는bit1관문에서자연반환하며 DirectDraw는 필수장치다.
    std::function<void()> presentPreservingClip; //004a46d0이다. viewport Update 전후 기존clip을 보존한다.
    std::function<void()> flip; //004a16d0이다. flipping0은자연반환하며 실제flip은필수장치다.
};
// loaded는파일을읽은뒤항상1이다. clear비영/현재flags비영일때만각필수하위를원문순서로호출한다.
inline void ApplyScreenPaletteClear(std::uint32_t clear,const std::uint32_t& flags,std::uint32_t& loaded,const ScreenPaletteApplyHooks& hooks) {
    loaded=1;
    if (clear!=0&&flags!=0) {
        hooks.lock();
        hooks.fillBlack();
        hooks.unlock();
        hooks.waitVerticalBlank();
        hooks.presentPreservingClip();
        hooks.flip();
    }
}
}
