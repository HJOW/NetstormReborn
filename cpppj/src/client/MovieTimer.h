// 원본 Smackw32.dll의 오디오 시계 없는 SmackWait 가지를 복원한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::client { // 영상의 원래 프레임 간격을 게임 시계/화면 FPS와 구별한다.
struct MovieTimerState {
    std::uint32_t fast{},ready{},due{0xffffffffU},interval{}; // SDK+39c/+408/+40c/+410의 현재DWORD다. interval은10µs 단위다.
};
struct MovieTimerHooks {
    std::function<std::uint32_t()> milliseconds; // SDK의 timeGetTime 입력이다. ready/fast 가지에서는 읽지 않는다.
    std::function<void()> yield; // SDK406160의 대기 중 서비스 경계다. 독립 PCM 출력은 메인 루프의 기존 대기를 사용한다.
};
// SDK00403000..00403137의 fallback 판단/100배DWORD 감김·최초 기한/지연 재설정을 보존한다.
bool WaitMovieTimer(MovieTimerState& state,const MovieTimerHooks& hooks);
}
