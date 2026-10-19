// 원본 공통 프레임/입력과 Early1~6 판정 사이에 계산 결과를 합성하지 않고 실제 소비자만 묶는다.
#include "client/RawCampaignTutorialOneBindings.h"
#include "o/ConfigInterface.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 보호10.78 PE의542a40/542a3c 초기 DWORD다. 입력 modifier를 제거한 뒤 비교한다.
constexpr std::uint32_t kNextStage=0x020000d3U,kNextTellAt=0x020000d2U;
}
// gate 생성 중에는 callback을 실행하지 않는다. 실제 Session 판정기는 gate 생성 뒤 같은 주소에 연결한다.
RawCampaignTutorialOneBindings::RawCampaignTutorialOneBindings(RawCampaignSession& session,MissionScript& script,
    RawCampaignMissionStartBindings& start,o::MissionFrameGateState& gate,o::MissionStartState& state,
    CampaignTutorialOneEffects effects)
    :session_(session),script_(script),start_(start),state_(gate),startState_(state),effects_(std::move(effects)),gate_(gate,Hooks()) {
    if (!effects_.tell || !effects_.closeTransient || state_.tutorialNumber<1 || state_.tutorialNumber>6 || state_.demo)
        throw std::invalid_argument("Early1~6 실제 미션/UI 연결 계약 오류");
    if (state_.tutorialNumber==1) tutorial_=session_.MakeTutorialOne(gate_);
    else if (state_.tutorialNumber==2) {
        o::TutorialTwoHooks hooks; // 게임 조회/옵션 갱신은 같은 Session이 연결하며 UI 경계만 빌린다.
        hooks.gameSeconds=effects_.frame.gameSeconds;hooks.selected=effects_.selected;
        hooks.resetSelection=effects_.resetSelection;hooks.clearSelection=effects_.clearSelection;
        hooks.tell=effects_.tell;hooks.closeTransientGumps=effects_.closeTransient;
        tutorialTwo_=session_.MakeTutorialTwo(gate_,std::move(hooks));
    } else {
        o::TutorialLaterHooks hooks; // 실제 월드 조회는 Session이 공급하며 UI/시계는 같은 현재 창을 빌린다.
        hooks.gameSeconds=effects_.frame.gameSeconds;hooks.selected=effects_.selected;
        hooks.resetSelection=effects_.resetSelection;hooks.clearSelection=effects_.clearSelection;
        hooks.tell=effects_.tell;hooks.closeTransientGumps=effects_.closeTransient;
        tutorialLater_=session_.MakeTutorialLater(gate_,startState_,std::move(hooks));
    }
}
// 전체 게임 판정은 같은 Session에 맡기고, 기존 프레임 몸체의 실행 순서를 변경하지 않는다.
o::MissionFrameGateHooks RawCampaignTutorialOneBindings::Hooks() {
    auto hooks=effects_.frame; // UI/시계의 실제 현재 조회와 효과를 그대로 보존한다.
    hooks.checkOutcome=[this] { session_.CheckMissionOutcome(); };
    hooks.tellStage=[this] { TellStage(); };hooks.tellAt=[this] { TellAt(); };
    hooks.tutorial=[this](std::uint32_t number,std::uint32_t event) {
        if (number==1 && tutorial_) return tutorial_->Handle(event);
        if (number==2 && tutorialTwo_) return tutorialTwo_->Handle(event);
        if (number>=3 && number<=6 && tutorialLater_) {
            auto current=event_;current[0]=event; // gate가 전달한 현재 명령과 같은 호출의 두 인자를 함께 사용한다.
            return tutorialLater_->Handle(number,current);
        }
        throw std::logic_error("현재 Early1~6 전용 판정 연결 누락");
    };
    return hooks;
}
// 원본 화면 프레임은 입력이 없어도 공통 안내/단계 판정을 한 번 호출한다.
void RawCampaignTutorialOneBindings::Frame() const { static_cast<void>(Run({})); }
// 중첩 Tell/조회가 입력을 다시 분배하거나 예외를 내도 이전 호출의 인자를 남기지 않는다.
std::uint32_t RawCampaignTutorialOneBindings::Run(const InputEvent& event) const {
    const auto saved=event_; // 현재 호출 이전의 세 DWORD다. 게임 상태의 복제본이 아니다.
    event_={event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y)};
    try {
        const auto result=gate_.RunFrame(event.code); // 실제 공통 gate와 선택된 원본 판정기의 반환값이다.
        event_=saved;return result;
    }
    catch (...) { event_=saved;throw; }
}
// 원본 특수 명령은 공통 프레임을 추가 실행하지 않고 즉시1을 반환한다.
std::uint32_t RawCampaignTutorialOneBindings::Event(const InputEvent& event) const {
    const auto code=event.code&0x03ffffffU; // 원본 입력의 modifier만 제거한다.
    if (code==kNextStage) { static_cast<void>(gate_.Advance());return 1; }
    if (code==kNextTellAt) { TellAt();return 1; }
    return Run(event);
}
// Tell이 없는 단계도 마지막 결과0을 기록한다. 창이 생긴 경우에만 원본 후속 정리를 실행한다.
void RawCampaignTutorialOneBindings::TellStage() const {
    std::string name(1,static_cast<char>(state_.stage));name+='.'; // 원본의 signed char 단계와 마침표다.
    const bool shown=effects_.tell(name); // 현재 미션에 존재하는 섹션만 조회한다.
    if (shown) { effects_.closeTransient();gate_.PauseForDialog(); }
    stageShown_=shown ? 1U : 0U;
}
// 원본처럼 사제가 죽었어도 목록 인덱스 증가와 다음 예약은 실행한다.
void RawCampaignTutorialOneBindings::TellAt() const {
    if (!session_.MissionPriestDead(session_.LocalOwner())) {
        const auto item=o::ConfigListItem(script_.Get("tellAtList").value_or(""),std::bit_cast<std::int32_t>(startState_.tellAtIndex)); // 현재 목록/현재 인덱스다.
        if (effects_.tell(item)) { effects_.closeTransient();gate_.PauseForDialog(); }
    }
    ++startState_.tellAtIndex;start_.StartBody().RefreshTellAt();
}
// UI와 공통 프레임이 같은 정지 소유권을 사용한다.
o::RawMissionFrameGate& RawCampaignTutorialOneBindings::Gate() { return gate_; }
// 현재+b4를 별도 진행 결과로 바꾸지 않고 그대로 읽는다.
std::uint32_t RawCampaignTutorialOneBindings::StageShown() const { return stageShown_; }
}
