// 실제 Guide가 소유한 미션 상태와 해제를 공유하고 World·입력의 수명은 호출자에게 둔다.
#pragma once
#include "client/TutorialInit.h"
#include "o/RawMissionFrameGate.h"
#include "o/RawMissionStart.h"
#include <array>
#include <memory>

namespace netstorm::client { // 같은 제품 Guide와 창 없는 실제 World 검사가 한 소멸 경로를 사용한다.
class Client;class State;class MissionScript; // 현재 Client·지연 큐·등록 Config의 실제 소유자를 빌린다.
class RawCampaignMissionStartBindings;class RawCampaignTutorialOneBindings; // 같은 Session을 빌리는 두 실행기의 완전형은 구현 파일에 둔다.
struct MissionFortLoadRequest; // Guide가 예약한 현재 로드 요청만 함께 반납한다.
struct CampaignGuideRuntime { // 원본 Mission 필드와 그 필드를 빌리는 실행기를 고정 주소에 보유한다.
    std::uint32_t saveToken; // 현재 World 설치를 같은 미션과 연결하는 호스트 토큰이며 게임 규칙 값은 아니다.
    TutorialInitState init; // 원본 +4/+b0 및 Init 지역 문자열의 현재 입력이다.
    o::MissionFrameGateState gate; // demo/started/시각/안내 단계의 유일한 현재 미션 저장소다.
    o::MissionStartState state; // Start의 순번·현재 tellAt 항목과 필드를 같은 수명으로 유지한다.
    std::unique_ptr<RawCampaignMissionStartBindings> start; // 실제 Session 설치 뒤 생성하고 Tutorial보다 오래 유지한다.
    std::uint32_t mode64{}; // Mission+64의 현재 편집 복귀 모드이며 해제 중 임의 정규화하지 않는다.
    bool guided{}; // 실제 Tutorial 가상표를 사용하는 10.78 미션인지 나타낸다.
    std::unique_ptr<RawCampaignTutorialOneBindings> tutorial; // 현재 Start·Session·Script를 빌리는 안내 실행기다.
    CampaignGuideRuntime(MissionScript& script,std::uint32_t demo,std::uint32_t mode,std::uint32_t token); // 생성은 번호·Start 효과를 앞당기지 않는다.
    ~CampaignGuideRuntime(); // Tutorial→Start 순서로 현재 참조를 먼저 반납한다.
};
// 00463ab0→004c3210→00483210을 실제 Client/AI/Config에 연결한다. World·UserInput·시계·음악은 쓰지 않는다.
// 근거: docs/exe/cpp-guide-release-20261101.md. loading 창의 장치 반납은 UberGump가 호출 전에 수행한다.
void ReleaseCampaignGuideMission(Client& client,State& pending,std::unique_ptr<MissionScript>& mission,
    std::unique_ptr<CampaignGuideRuntime>& runtime,std::shared_ptr<MissionFortLoadRequest>& request,
    std::array<std::uint32_t,256>& restrictions,std::uint32_t& missionNumber,std::uint32_t& worldMissionToken,bool& nativeMission);
}
