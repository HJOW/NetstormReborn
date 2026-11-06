// 범위: 정상 시작 상태의 32비트 밀리초 시계·중첩 정지·프레임 시각 고정.
// 원본 초기 시간 오프셋과 FPS 통계는 아직 옮기지 않았다. OS timeGetTime 호출은 외부 입력으로 대체한다.
#include "o/GameClock.h"
#include <limits>
#include <stdexcept>
#include <utility> // 실제 장기 소유권을 복제 상태 없이 시계 대여에 이동한다.

namespace netstorm::o {
// 원본 DAT_0050a668: 밀리초 나머지를 초로 환산하는 배율.
constexpr double kMillisecondSeconds = 0.001;
// 원본 시작 밀리초 기준을 기록한다.
GameClock::GameClock(std::uint32_t originMilliseconds) : origin_(originMilliseconds) {}
// 원본과 같은 unsigned 감산으로 32비트 래핑을 재현한다.
double GameClock::WallSeconds(std::uint32_t milliseconds) const {
    const std::uint32_t elapsed = milliseconds - origin_;
    return static_cast<double>(elapsed / 1000) + static_cast<double>(elapsed % 1000) * kMillisecondSeconds;
}
// 정지 시간은 프레임 시간차와 별도로 누적한다.
double GameClock::GameSeconds(std::uint32_t milliseconds) const {
    if (pauseDepth_!=0) return pausedGame_;const auto elapsed=(milliseconds-origin_)-(synchronizationTicks_ ? *synchronizationTicks_ : 0U); // 원문은 signed 보정 비트도 unsigned 밀리초 감김으로 감산한다.
    return (static_cast<double>(elapsed/1000)+static_cast<double>(elapsed%1000)*kMillisecondSeconds)-pausedTotal_;
}
void GameClock::BindSynchronizationTicks(const std::uint32_t& ticks,std::shared_ptr<void> owner) { synchronizationOwner_=std::move(owner);synchronizationTicks_=&ticks; } // 같은 Native writer/cleanup 결과를 매 게임시각 조회에 직접 반영하고 실제 소유자를 유지한다.
// 첫 정지에서 저장하고 추가 정지는 카운터만 증가시킨다.
void GameClock::Pause(std::uint32_t milliseconds) {
    if (pauseDepth_ == std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Pause depth overflow");
    if (pauseDepth_ == 0) pausedGame_ = GameSeconds(milliseconds);
    ++pauseDepth_;
}
// 정지되지 않았으면 원본처럼 아무 작업도 하지 않는다.
void GameClock::Resume(std::uint32_t milliseconds) {
    if (pauseDepth_ != 0 && --pauseDepth_ == 0) pausedTotal_ = (GameSeconds(milliseconds) - pausedGame_) + pausedTotal_;
}
//원문13BYTE는0055b4f8에+0.0만쓰며현재시각조회/정지해제/프레임재캡처나난수를호출하지않는다.
void GameClock::Reset() { pausedTotal_=0.0; }
// 게임 시계 정지 여부를 반환한다.
bool GameClock::IsPaused() const { return pauseDepth_ != 0; }
// 갱신 전체가 같은 시각을 사용하도록 한 번 계산해 스냅샷을 반환한다.
FrameTime GameClock::Capture(std::uint32_t milliseconds) {
    const auto game = GameSeconds(milliseconds);
    frame_.delta = game - frame_.game;
    frame_.game = game;
    frame_.wall = WallSeconds(milliseconds);
    ++frame_.number;
    return frame_;
}
}
