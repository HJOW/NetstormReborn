// Early1~6의 공통 프레임과 단계 안내를 같은 실제 Session/Start/UI에 연결한다. 기존 공개 이름은 유지한다.
#pragma once
#include "client/RawCampaignMissionStartBindings.h"
#include <array>

namespace netstorm::client {
// 화면 루프와 실제 입력이 함께 읽는 현재 UI/시계 소비자다.
struct CampaignTutorialOneEffects {
    o::MissionFrameGateHooks frame; // 실제 시계/정지/UI 조회다. 게임 판정과 Tell 분배는 이 연결부가 공급한다.
    std::function<bool(std::string_view)> tell; // 현재 미션에 존재하는 섹션만 실행한다.
    std::function<void()> closeTransient; // 원본66710의 실제 팝업 Kill 요청이다.
    std::function<std::uint32_t()> selected; // Early2/5가 중복 조회하는 현재 실제 선택SID다.
    std::function<void()> resetSelection,clearSelection; // Early2/5의004d5c90(0,0,0)→004d5c60을 별도 실제 몸체로 공급한다.
};
// Early1~6을 같은 현재 실제 소비자에 분배하며 각 판정기의 고유 입력을 유지한다.
class RawCampaignTutorialOneBindings {
public:
    // 같은 장기 Session/Script/Start/gate/state를 빌리며 생성만으로 프레임을 실행하지 않는다.
    RawCampaignTutorialOneBindings(RawCampaignSession& session,MissionScript& script,
        RawCampaignMissionStartBindings& start,o::MissionFrameGateState& gate,o::MissionStartState& state,
        CampaignTutorialOneEffects effects);
    RawCampaignTutorialOneBindings(const RawCampaignTutorialOneBindings&)=delete; // callback의 this와 실제 gate 주소를 고정한다. 이동도 허용하지 않는다.
    RawCampaignTutorialOneBindings& operator=(const RawCampaignTutorialOneBindings&)=delete; // 같은 장기 상태의 참조를 다른 연결 주소로 복제하지 않는다.
    void Frame() const; // 4c4770: 사건0으로 공통 프레임을 한 번 실행한다.
    std::uint32_t Event(const InputEvent& event) const; // Guide First→4c33a0의 단계/시간 안내 명령→현재 사건 순서로 소비한다.
    void TellStage() const; // 4c2a90: 현재 단계 문자열→Tell→팝업 Kill→자기 정지 순서다.
    void TellAt() const; // 4c3470: 사제 생존→현재 목록 Tell→인덱스 증가→다음 예약이다.
    o::RawMissionFrameGate& Gate(); // 후속 UI가 같은 자기 정지 표식을 사용한다.
    std::uint32_t StageShown() const; // 원본+b4의 마지막 TellStage 반환값을 보고한다.
private:
    o::MissionFrameGateHooks Hooks(); // 실제 게임/안내 경계를 묶고 UI/시계 경계는 그대로 보존한다.
    std::uint32_t Run(const InputEvent& event) const; // 프레임은0/0/0, 입력은 실제 세 DWORD를 전달하며 재진입 뒤 바깥 사건을 복원한다.
    RawCampaignSession& session_; // 현재 객체/Player/장부/간헐천의 유일한 저장소다.
    MissionScript& script_; // 현재 미션의 tellAtList/섹션을 직접 읽는다.
    RawCampaignMissionStartBindings& start_; // 같은 Start의 RefreshTellAt을 사용한다.
    o::MissionFrameGateState& state_; // 단계/잠금/정지/시각의 단일 저장소다.
    o::MissionStartState& startState_; // 같은 현재 tellAtIndex를 읽고 쓴다.
    CampaignTutorialOneEffects effects_; // 실제 화면/시계/팝업의 외부 소비자다.
    o::RawMissionFrameGate gate_; // 독립 원본328입력으로 검증된 공통 몸체다.
    std::unique_ptr<o::RawTutorialOne> tutorial_; // 현재 장부와 최근 간헐천 목록을 참조하는 원본 판정기다.
    std::unique_ptr<o::RawTutorialTwo> tutorialTwo_; // 같은 gate와 현재 옵션/장부/생산/선택을 쓰는 Early2 판정기다.
    std::unique_ptr<o::RawTutorialLater> tutorialLater_; // 같은 현재 Start/Player/장부를 사용하는 Early3~6 원본 판정기다.
    mutable std::array<std::uint32_t,3> event_{}; // 공통 gate 호출 동안만 빌리는 현재 사건이다. x/y의 SID·TYPE 비트를 보존한다.
    mutable std::uint32_t stageShown_{}; // 원본+b4이며 TellStage가 성공 여부를 마지막에 기록한다.
};
}
