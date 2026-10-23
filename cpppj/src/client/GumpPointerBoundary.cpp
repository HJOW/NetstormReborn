// 별도 입력 기록의 현재 재조회와 원본 queue 호출 순서를 보존한다.
#include "client/GumpPointerBoundary.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본은 현재 포인터의 좌표를 얻기 위해 Alt를 폴링하며 language3에서만 Ctrl 상태를 함께 읽는다.
constexpr std::uint32_t kPointerPoll=0x00120000,kControlPoll=0x00110000,kCombinedLanguage=3;
//005cafe8의 두 일회 초기화 비트다. edge/tooltip의 뒤쪽 초기화 비트를 지우지 않는다.
constexpr std::uint32_t kLeftInitialized=1,kRightInitialized=2;
// code의 최상위 비트만 검사한다. 다른 modifier/outside/pressedSince 비트는 바꾸지 않는다.
bool Pressed(const InputEvent& event) { return (event.code&InputCode::kRelease)==0; }
// 실제 도달한 예약 분기에서만 필수 콜백을 검사한다. 사건의 세 필드는 그대로 전달한다.
void Queue(const GumpPointerBoundaryHooks& hooks,InputEvent event) {
    if (!hooks.queue) throw std::logic_error("포인터 재진입 release의 실제 입력 큐 미연결");
    hooks.queue(event);
}
}
// 첫 포인터 폴링 뒤 정적 초기화→상태 전환 분기→현재 inside 저장 순서를 실행한다.
InputEvent PrepareGumpPointerBoundary(GumpPointerBoundaryState& state,std::uint32_t currentLanguage,
    const GumpPointerBoundaryHooks& hooks) {
    if (!hooks.poll) throw std::invalid_argument("포인터 앞단의 실제 현재 폴링 필요");
    auto pointer=hooks.poll(kPointerPoll); // 원본4d6398/4d63fd의 첫12BYTE를 그대로 복사한다.
    if (currentLanguage==kCombinedLanguage) {
        const auto control=hooks.poll(kControlPoll); // 원본은 두 사건이 모두 pressed일 때 첫 code에만 release를 더한다.
        if (Pressed(pointer) && Pressed(control)) pointer.code|=InputCode::kRelease;
    }
    if (!(state.initializationFlags&kLeftInitialized)) { state.initializationFlags|=kLeftInitialized;state.leftOutside={}; }
    if (!(state.initializationFlags&kRightInitialized)) { state.initializationFlags|=kRightInitialized;state.rightOutside={}; }
    const bool inside=(pointer.code&InputCode::kOutside)==0; // Guide가 변경하기 전 폴링 사건의 현재 비트26이다.
    if (!state.wasInside) {
        if (inside) {
            const auto left=hooks.poll(InputCode::kLeftButton); // 좌·우를 먼저 모두 폴링한 다음 기록 비교를 시작한다.
            const auto right=hooks.poll(InputCode::kRightButton);
            if (Pressed(state.leftOutside) && !Pressed(left)) Queue(hooks,left);
            if (Pressed(state.rightOutside) && !Pressed(right)) Queue(hooks,right); // 좌 queue 콜백 뒤 현재 우 기록을 다시 읽는다.
        }
    } else if (!inside) {
        state.leftOutside=hooks.poll(InputCode::kLeftButton); // 우 poll보다 먼저 좌 결과를 기록한다.
        state.rightOutside=hooks.poll(InputCode::kRightButton);
    }
    state.wasInside=inside ? 1U : 0U; // 중간 poll/queue 콜백이 이 필드를 바꿔도 마지막에는 현재 포인터 값으로 덮어쓴다.
    return pointer;
}
// 원본52f90과 같은 raw Push를 사용하므로 code의 release/수식 비트 및 signed 좌표를 손실하지 않는다.
InputEvent PrepareGumpPointerBoundary(GumpPointerBoundaryState& state,std::uint32_t currentLanguage,
    std::function<InputEvent(std::uint32_t)> poll,InputQueue& queue) {
    return PrepareGumpPointerBoundary(state,currentLanguage,{std::move(poll),[&queue](InputEvent event) { queue.Push(event.code,event.x,event.y); }});
}
}
