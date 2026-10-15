// 실제 미션 Start와600BYTE AI설정을 같은 Session/Script/프레임 게이트에 연결한다.
#pragma once
#include "client/RawCampaignSession.h"
#include "client/Mission.h"
#include "o/RawMissionStart.h"
#include "o/RawMissionAiSettings.h"
#include "o/RawPriestLookup.h"

namespace netstorm::client {
// 게임 계산을 대신하지 않는 실제 할당 초기 입력/시계/화면 외부 소비자다.
struct CampaignMissionStartEffects {
    std::function<void(std::uint32_t,std::span<std::uint8_t>)> settingsBytes; // 현재 owner의600BYTE caller 미정 바이트를 채운다. 기본값 생성은 실제 모듈이 한다.
    std::function<double()> gameSeconds; // 같은 게임 시계의 현재 double을 원본 읽기 위치마다 반환한다.
    std::function<void()> refreshSp; // 실제0043dad0의 현재 SP Gump 재구성이다.
    std::function<void()> refreshCommandGump; // 실제0042e440의 현재 명령 Gump 갱신이다.
    std::function<void()> randomGeysers; // 실제00486440의 세계 랜덤 간헐천 생성이며 로컬 GenerateLocalGeysers와 다르다.
    std::function<void()> resetCamera; // 같은 실제 Session의 현재 카메라 재설정 소비자다.
    std::function<void(std::string_view)> unknownAbilityAssertion,unknownAbilityLog; // 실제 원본의 미등록 능력 진단 뒤 계속하는 외부 경계다.
};
// 같은 Session/Script/장기 상태는 이 연결보다 오래 살아야 한다. 생성만으로 Start/AI를 실행하지 않는다.
class RawCampaignMissionStartBindings {
public:
    // aiGlobals는 실제005318d4/0052f3c8/진단 전역 저장소다. 기본3/2500의 새 사본을 만들지 않는다.
    RawCampaignMissionStartBindings(RawCampaignSession& session,MissionScript& script,o::MissionFrameGateState& gate,
        o::MissionStartState& state,o::MissionStartGlobals& globals,o::MissionAiSettingsState& aiGlobals,CampaignMissionStartEffects effects);
    // 원본004c2b20 한 객체를 실행하며 마지막 실제 효과가 성공한 뒤 같은 gate.started만 쓴다.
    void Start() const;
    // 원본004c2d40: 현재 전역 기본값→공통(-1)→owner 설정 뒤 같은 Session의 실제 AI 생성/초기 갱신을 실행한다.
    std::uint32_t CreateAi(std::uint32_t owner) const;
    // 프레임/안내 소비자가 별도 Start를 만들지 않고 동일 처리기를 사용한다.
    const o::RawMissionStart& StartBody() const;
    // 같은 프레임 게이트 주소를 확인하며 상태 사본을 만들지 않는다.
    const o::MissionFrameGateState& Gate() const;
    // Start의004921f0과 실제 AI 생성자가 공유하는 live 사제 조회다. 포획 참여 fallback과 구분한다.
    std::uint32_t FindLivePriest(std::uint32_t owner) const;
private:
    // 생성자에서 실제 Session과 Script의 함수만 조립하며 게임 결과를 외부 bool로 받지 않는다.
    o::MissionStartHooks Hooks();
    // 현재 MissionScript를 직접 조회하는 기본값/공통/owner 설정 처리기를 조립한다.
    o::MissionAiSettingsHooks AiHooks();
    RawCampaignSession& session_; // SP/기술/AI/현재 Player9의 유일한 실제 수명 소유자다.
    MissionScript& script_; // 현재 mission Config와 Set/SectionNames가 공유하는 객체다.
    o::MissionFrameGateState& gate_; // 실제 프레임/Guide가 사용하는 같은 started/start/demo/tellAt 필드다.
    o::MissionAiSettingsState& aiGlobals_; // 명시 공급한 실제 stuffRefreshRate/startingSp/진단 전역이다.
    CampaignMissionStartEffects effects_; // 같은 실제 UI/게임 시계/세계 랜덤 생성 외부 경계다.
    o::RawPriestLookup priests_; // 같은 실제 Pool/사제 목록만 매 조회마다 읽는다.
    o::RawMissionAiSettings settings_; // 기존 원본600BYTE 몸체를 같은 전역/Script에 연결한다.
    o::RawMissionStart start_; // 같은 gate와 StartState/Globals를 참조하는 유일한 공통 Start다.
};
}
