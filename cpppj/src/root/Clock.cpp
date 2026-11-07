// 실제 timeGetTime/epoch를 사용하는 R의 FullTime 순서와 별도 현재 통계를 복원한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // 불필요한 Windows 헤더 충돌을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 함수와 Windows 매크로 충돌을 피한다.
#endif
#include <windows.h>
#include <mmsystem.h>
#include "root/Clock.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <ctime>
#include <stdexcept>

namespace netstorm::root { // 현재 wall과 새 지역 FullTime을 구별하는 실제 서버 장치다.
namespace { // 원문의 signed history 나머지/부호 감김 경계다.
std::size_t HistoryIndex(std::uint32_t frame) { const auto index=std::bit_cast<std::int32_t>(frame)%10;if (index<0) throw std::out_of_range("R FullTime frame 감김 뒤 history 음수 물리 범위");return static_cast<std::size_t>(index); } // 원문 음수 배열 접근을 다른 정상칸으로 감싸지 않는다.
}
Clock::Clock(ClockState& state,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion):state_(state),assertion_(std::move(assertion)) {}
double Clock::Capture(bool synchronized) {
    const auto milliseconds=state_.baseSeconds*1000U+(::timeGetTime()-state_.originMilliseconds-(synchronized ? state_.synchronizationMilliseconds:0U));return double(milliseconds%1000U)/1000.0+double(milliseconds/1000U);
} // 원문의 DWORD 감김과 remainder/1000+quotient 계산을 유지한다.
double Clock::CaptureWall() { return Capture(false); }
double Clock::CaptureGame() {
    if (state_.synchronizationMilliseconds && state_.pausedTotal!=0.0) { if (!assertion_) throw std::logic_error("R FullTime의 실제 assert 장치 미연결");assertion_("serverDiffMils == 0.0 || timeStoppedAccumulator == 0.0","c:\\ns\\zacket\\fulltime.cpp",77); }
    return state_.pauseDepth ? state_.pausedGame:Capture(true)-state_.pausedTotal;
}
void Clock::Pause() { if (!state_.pauseDepth) state_.pausedGame=CaptureGame();++state_.pauseDepth; } // 최초 정지만 캡처하고 DWORD 깊이 증가를 합치지 않는다.
void Clock::Resume() { if (state_.pauseDepth && !--state_.pauseDepth) state_.pausedTotal=(CaptureGame()-state_.pausedGame)+state_.pausedTotal; } // 마지막 해제에서만 누적 정지 시간을 계산한다.
bool Clock::IsPaused() const { return std::bit_cast<std::int32_t>(state_.pauseDepth)>0; } // 원문의 signed 양수 판정이다.
void Clock::ResetAccumulator() { state_.pausedTotal=0.0; } // 기준/정지 깊이/현재 프레임은 바꾸지 않는다.
std::uint32_t Clock::Truncate(double seconds) {
    constexpr double limit=9223372036854775808.0; // x87 signed64 변환 범위의 정확한2^63 경계다.
    if (!std::isfinite(seconds) || seconds<-limit || seconds>=limit) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(seconds)));
} // invalid의 signed64 indefinite 8000000000000000도 하위DWORD는0이다.
std::uint32_t Clock::Now() const { return Truncate(state_.wall); }
void Clock::Tick() {
    const auto previous=state_.game;state_.delta=previous;state_.game=CaptureGame();state_.wall=CaptureWall();if (state_.firstGame==0.0) state_.firstGame=state_.game;state_.delta=state_.game-previous;
    if (!(state_.initializationFlags&1U)) { state_.initializationFlags|=1U;state_.history.fill(0.0); }const auto frame=std::bit_cast<std::int32_t>(state_.frameNumber);state_.history[HistoryIndex(state_.frameNumber)]=state_.wall;
    if (frame>0) { const auto duration=state_.wall-state_.history[HistoryIndex(state_.frameNumber+9U)];state_.instantaneousFps=duration>0.0 ? 1.0/duration:-1.0; }
    if (frame>10) { const auto duration=state_.wall-state_.history[HistoryIndex(state_.frameNumber+1U)];state_.averageFps=duration>0.0 ? 10.0/duration:-1.0;state_.averageFrameSeconds=1.0/state_.averageFps; }++state_.frameNumber; // 실제9간격 표본에 numerator10을 쓰는 원문 통계를 유지한다.
}
void Clock::Initialize() {
    constexpr std::uint32_t epoch=0x32c85780U; //00473e40이 실제 time32에서 감산하는 기준 epoch다.
    const auto process=GetCurrentProcess();const auto previous=GetPriorityClass(process);SetPriorityClass(process,HIGH_PRIORITY_CLASS);SYSTEMTIME time{};GetSystemTime(&time);const auto second=time.wSecond;
    do { GetSystemTime(&time); } while (time.wSecond==second); // 원문처럼 초가 바뀔 때까지 실제 UTC 초 경계를 기다린다.
    state_.originMilliseconds=::timeGetTime();state_.baseSeconds=static_cast<std::uint32_t>(::_time32(nullptr))-epoch;
    try { Tick(); }catch (...) { SetPriorityClass(process,previous);throw; }SetPriorityClass(process,previous); // 호스트 예외에서도 실제 프로세스 우선순위 대여를 반납한다.
}
void Clock::Bind(TransceiverHooks& hooks) { hooks.now=[this] { return Now(); };hooks.captureNow=[this] { return Truncate(CaptureWall()); }; }
}
