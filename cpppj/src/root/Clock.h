// R FullTime의 현재 시각·중첩 정지/보정·프레임 통계를 같은 프로세스 수명으로 보관한다.
#pragma once
#include "root/TransceiverTransport.h"

namespace netstorm::root { // 게임 Client의 별도 GameClock을 복제/대여하지 않는 서버 시계다.
struct ClockState { //004d1af8..1b54의 현재 DWORD/double과 별도10칸 history다.
    std::uint32_t originMilliseconds{},baseSeconds{},frameNumber{},synchronizationMilliseconds{},pauseDepth{}; //1af8/1afc/1b00/1b04/1b08의 BSS0이다.
    double instantaneousFps{},averageFps{},averageFrameSeconds{}; //1b10/18/20의 현재 통계이며 첫 계산 전은0이다.
    double wall{},game{},pausedTotal{},pausedGame{},firstGame{},delta{}; //1b28/30/38/40/48/50의 현재 FullTime double이다.
    std::uint32_t initializationFlags{};std::array<double,10> history{}; //1aa0의 bit1과1aa8의 실제10개 FullTime0이다.
};
class Clock final { // 같은 상태와 시계 callback은 실제 서버 전송/기록보다 오래 살아야 한다.
public:
    Clock(ClockState& state,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion); // 생성은 OS 시각/우선순위를 변경하지 않는다.
    Clock(const Clock&)=delete;Clock& operator=(const Clock&)=delete; // 실제 상태/대여 callback을 복제하지 않는다.
    void Initialize(); //00473e40의 초 경계 동기화/기준 epoch·최초Tick과 실제 프로세스 우선순위 복원이다.
    double CaptureWall();double CaptureGame(); //004739b0/473880의 서로 다른 실제 timeGetTime 캡처다.
    void Pause();void Resume();bool IsPaused() const;void ResetAccumulator(); //473a60/ac0/a30/b30의 중첩 깊이·누적 정지 시간만 변경한다.
    void Tick(); //00473b80의 게임→별도wall/첫게임·delta/history/FPS/현재 frame DWORD 증가다.
    std::uint32_t Now() const; // 현재 wall FullTime의 ftol64 하위DWORD이며 새 OS 시각을 읽지 않는다.
    ClockState& State() { return state_; }const ClockState& State() const { return state_; } // 메인 루프/예약 처리도 같은 현재 게임 시각·통계를 직접 빌린다.
    void Bind(TransceiverHooks& hooks); // 현재 now와 별도 지역 captureNow를 실제 전송에 연결한다.
    static std::uint32_t Truncate(double seconds); // x87 ftol64의 truncation/invalid indefinite 하위DWORD를 호스트 정수로 옮긴다.
private:
    ClockState& state_;std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion_; // 같은 현재 전역과 원문의 선택된 fulltime.cpp 진단이다.
    double Capture(bool synchronized); //4737d0의 uint32 모듈러 덧셈/감산·나눗셈/나머지 순서다.
};
}
