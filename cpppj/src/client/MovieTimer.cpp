// 실제 SDK 기계어의 대기/기한 쓰기 순서를 보존한다. NextFrame이 ready를0으로 쓰는 것은 호출자 책임이다.
#include "client/MovieTimer.h"
#include <stdexcept>

namespace netstorm::client { // 원본의 unsigned 비교와 DWORD 덧셈을 그대로 사용한다.
namespace { // 도달한 시계/대기 서비스만 요구하는 내부 경계다.
template<class F> const F& NeedTimer(const F& callback) { if (!callback) throw std::logic_error("영상 SDK 시계의 선택된 장치 미연결");return callback; } // 선택되지 않은 시계/서비스를 요구하지 않는다.
}
bool WaitMovieTimer(MovieTimerState& state,const MovieTimerHooks& hooks) {
    if (state.fast || state.ready) return false;
    const auto now=NeedTimer(hooks.milliseconds)()*100U; // SDK는 밀리초를10µs 단위로 바꾸며32비트로 감는다.
    if (state.due==0xffffffffU) { state.due=now+state.interval;NeedTimer(hooks.yield)();return true; }
    if (now<state.due) { NeedTimer(hooks.yield)();return true; }
    const auto next=state.due+state.interval,late=now-next;state.due=next;
    if (late>state.interval) state.due=now+state.interval;
    if (state.interval) state.ready=1;return false;
}
}
