// 새 파일명(원본 소속 미확정): 00460cd0~00460e90의 게임 시간·중첩 정지 로직 복원.
#pragma once
#include <cstdint>

namespace netstorm::o {
struct FrameTime {
    double game{}; // DAT_0055b4d0, 프레임 안에서 고정된 게임 시각.
    double wall{}; // DAT_0055b4d8, 정지 중에도 증가하는 벽시계.
    double delta{}; // DAT_0055b4e0, 직전 프레임과의 게임 시간차.
    std::uint64_t number{}; // DAT_0055b4a8의 프레임 카운터. 새 빌드는 폭을 늘린다.
};

class GameClock {
public:
    // OS 시계는 외부에서 공급한다. 시작 밀리초와의 차이는 원본 unsigned 32비트 연산이다.
    explicit GameClock(std::uint32_t originMilliseconds = 0);
    // 00460d70(A) ↔ CD 004011b0: 원본 나눗셈·나머지 순서대로 초를 계산한다.
    double WallSeconds(std::uint32_t milliseconds) const;
    // 00460cd0(A) ↔ CD 00401130: 정지 중이면 저장값, 아니면 누적 정지 시간을 뺀 값.
    double GameSeconds(std::uint32_t milliseconds) const;
    // 00460df0(A) ↔ CD 00401230: 첫 정지에서만 게임 시각을 저장한다.
    void Pause(std::uint32_t milliseconds);
    // 00460e30(A) ↔ CD 00401270: 마지막 정지가 풀릴 때만 정지 시간을 누적한다.
    void Resume(std::uint32_t milliseconds);
    // 00460de0: 중첩 정지 카운터가 양수인지 확인한다.
    bool IsPaused() const;
    // 00460e90(A) ↔ CD 004012d0의 시각 고정·delta·프레임 증가 부분.
    FrameTime Capture(std::uint32_t milliseconds);
private:
    std::uint32_t origin_{};
    std::uint32_t pauseDepth_{};
    double pausedGame_{};
    double pausedTotal_{};
    FrameTime frame_{};
};
}
