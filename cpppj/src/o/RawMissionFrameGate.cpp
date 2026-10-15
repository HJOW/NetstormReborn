// 실제 기계어의 float 지연+double 게임 시각과 호출 뒤 현재 필드 재조회를 보존한다.
#include "o/RawMissionFrameGate.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본이 안내 잠금과 재개 연속 화면 수에 쓰는 signed DWORD 상수다.
constexpr std::int32_t kLockFrames=10,kResumeFrames=7;
// 원본 DWORD 덧셈은 signed overflow에서도 감기므로 C++의 부호 있는 덧셈을 직접 쓰지 않는다.
std::int32_t Increment(std::int32_t value) {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+1U);
}
}
// 누락 경계를 정상 성공/빈 동작으로 처리하지 않도록 최초 실행 전에 모두 검사한다.
RawMissionFrameGate::RawMissionFrameGate(MissionFrameGateState& state,MissionFrameGateHooks hooks)
    :state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.gameSeconds || !hooks_.paused || !hooks_.checkOutcome || !hooks_.dialogsClear ||
        !hooks_.knowledgeOpen || !hooks_.guideOpen || !hooks_.blockingGumpOpen || !hooks_.tellStage ||
        !hooks_.tellAt || !hooks_.endDemo || !hooks_.tutorial || !hooks_.pause || !hooks_.resume)
        throw std::invalid_argument("미션 공통 프레임의 실제 시계/UI/분배 경계 누락");
}
// 최초 예약과 후속 예약은 같은 카운터를 사용한다. 콜백이 쓴 현재 필드는 다음 원본 읽기에서 반영한다.
std::uint32_t RawMissionFrameGate::RunFrame(std::uint32_t event) const {
    if (!state_.started) return 0;
    state_.stageDirty=0;hooks_.checkOutcome();
    if ((state_.nextStageSeconds<=hooks_.gameSeconds() || hooks_.paused()) && state_.lockFrames>0) {
        if (!hooks_.dialogsClear()) state_.lockFrames=kLockFrames;
        else --state_.lockFrames;
        if (state_.lockFrames<1) hooks_.tellStage();
    }
    if (state_.tellAtSeconds!=0.0 && state_.tellAtSeconds<hooks_.gameSeconds()-state_.startSeconds &&
        hooks_.dialogsClear() && !hooks_.knowledgeOpen()) hooks_.tellAt();
    if (state_.scriptPaused && state_.lockFrames==0) {
        if (!hooks_.guideOpen() && hooks_.dialogsClear() && !hooks_.blockingGumpOpen()) {
            state_.clearFrames=Increment(state_.clearFrames);
            if (state_.clearFrames>=kResumeFrames) {
                if (state_.scriptPaused) { hooks_.resume();state_.scriptPaused=0; }
                state_.clearFrames=0;
            }
        } else state_.clearFrames=0;
    }
    if (state_.demo && state_.demoEndSeconds!=0.0 && state_.demoEndSeconds<=hooks_.gameSeconds()) {
        hooks_.endDemo();state_.demoEndSeconds=0.0;
    }
    if (state_.lockFrames==0 && !state_.demo && state_.tutorialNumber>=1 && state_.tutorialNumber<=6)
        return hooks_.tutorial(state_.tutorialNumber,event);
    return 0;
}
// x87는 fld DWORD[+80] 뒤 fadd QWORD[0055b4d0]를 쓴다. 현재 시각을 float로 축소하지 않는다.
bool RawMissionFrameGate::Advance() const {
    if (hooks_.paused() || state_.lockFrames>=1) return false;
    const auto next=static_cast<std::uint8_t>(static_cast<std::uint32_t>(state_.stage)+1U); // 원본 char 증가의 저8비트다.
    state_.stage=static_cast<std::int32_t>(std::bit_cast<std::int8_t>(next));state_.lockFrames=kLockFrames;
    state_.nextStageSeconds=static_cast<double>(state_.delaySeconds)+hooks_.gameSeconds();state_.timerSeconds=0.0;
    return true;
}
// 다른 정지 보유자가 있어도 이 미션의 첫 정지는 별도 깊이를 잡는다. 이미 자기 깊이가 있으면 재진입을 생략한다.
void RawMissionFrameGate::PauseForDialog() const {
    if (!hooks_.paused() || !state_.scriptPaused) { hooks_.pause();state_.scriptPaused=1; }
}
// 초기0 기한은 유지한다. 현재0이 아닌 경우의 재지연만 원본 가상+20과 같다.
void RawMissionFrameGate::Postpone() const {
    if (state_.nextStageSeconds!=0.0) state_.nextStageSeconds=static_cast<double>(state_.delaySeconds)+hooks_.gameSeconds();
}
// Tutorial1~6과 상위 스크립트가 같은 원본 저장소를 공유한다.
MissionFrameGateState& RawMissionFrameGate::State() const { return state_; }
}
