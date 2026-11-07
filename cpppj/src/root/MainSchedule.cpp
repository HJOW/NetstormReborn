// 실제 서버 예약 요청과 현재 메인 상태·CCh 로그인 차단/전체 안내를 연결한다.
#include "root/MainLoop.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::root { //00479090/479290의 같은 현재게임시각과예약전역을사용한다.
namespace { // 원문 ftol64의lowDWORD를부호있는로그인자로사용한다.
std::int32_t Integer(double value) { return std::bit_cast<std::int32_t>(Clock::Truncate(value)); } // 분출력과초0관문은별도변환으로계산한다.
}
void RequestSchedule(ClockState& clock,MainLoopState& state,Routing& routing,std::uint32_t goingDown,double seconds,const std::function<void(std::int32_t,std::string_view)>& log) {
    if (state.scheduledAt!=0.0) return;if (!log) throw std::logic_error("R 예약 요청의 현재 로그 장치 미연결");log(0,std::string("Starting ")+(goingDown ? "shutdown":"reset")+" in "+std::to_string(Integer(seconds/60.0))+" minutes\n");
    if (!Integer(seconds)) { routing.BroadcastText(std::string("~r~3~EServer is ")+(goingDown ? "going down":"resetting")+" NOW!!!\n");if (goingDown) ++state.stopRequests;else ++state.resetRequests;return; } // 즉시요청은예약/차단/마지막안내전역을쓰지않는다.
    state.scheduledAt=clock.game+seconds;state.goingDown=goingDown;routing.SetLoginBlocked(1);state.lastWarning=clock.game;const auto text=std::string("~r~3~EServer is ")+(state.goingDown ? "going down":"resetting")+" in "+std::to_string(Integer(seconds/60.0))+" minutes.\n";routing.BroadcastText(text);log(29,text); // 같은예약을기존PollSchedule이소비하며메시지성공을가짜예약완료로대신하지않는다.
}
void BindSchedule(ChallengeRuntimeHooks& challenge,ClockState& clock,MainLoopState& state,Routing& routing,std::function<void(std::int32_t,std::string_view)> log) {
    challenge.schedule=[&clock,&state,&routing,log=std::move(log)](auto goingDown,double seconds) { RequestSchedule(clock,state,routing,goingDown,seconds,log); }; // 메인객체의this대신그객체도대여하는동일장기전역을참조한다.
}
}
