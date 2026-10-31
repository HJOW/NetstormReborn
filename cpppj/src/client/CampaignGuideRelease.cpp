// Guide 소멸의 실제 전역/AI/Config 순서를 World 소유권과 독립적으로 실행한다.
#include "client/CampaignGuideRelease.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/Mission.h"
#include "client/MissionFortRequest.h"
#include "client/RawCampaignMissionStartBindings.h"
#include "client/RawCampaignTutorialOneBindings.h"
#include "o/OriginalText.h"
#include "o/RawTotalMade.h"
#include "o/RawTutorialMissionRelease.h"
#include <stdexcept>

namespace netstorm::client { // 같은 실제 Guide의 명시 End·교체·반복 해제에서 사용한다.
namespace { // 원문의 전역 AI 반납에 현재 Client 소유자만 연결한다.
void RemoveCurrentGuideBrains(Client& client) { // World가 이미 반납됐으면 실제 같은 Player+B0가 모두 NULL인지 방문 순서대로 확인한다.
    const auto game=client.GameplayGlobals();
    if (game->Borrowed()) {
        auto& session=game->RequireSession();
        if (session.PlayerRecords().data()!=client.PlayerRecords()->Records().data()) throw std::logic_error("Guide AI 반납의 실제 Client Player 소유자 불일치");
        session.Participation().RemoveAllBrains();return; // 일반 해제이며 State reset 증빙을 미리 발급하지 않는다.
    }
    const auto players=client.PlayerRecords()->Records(); // 미초기화·손실된 물리를 가짜 빈 Player로 대체하지 않는다.
    if (players.size()!=o::kInitializedPlayerBytes) throw std::logic_error("Guide AI 반납의 현재 실제 Player9 물리 미초기화");
    for (std::uint32_t owner=1;owner<9;++owner) { // 원문 00413b20처럼 중립0을 제외한 여덟 현재 brain을 각각 읽는다.
        const auto offset=owner*180U+0xb0U;
        const auto brain=std::uint32_t(players[offset])|(std::uint32_t(players[offset+1])<<8)|(std::uint32_t(players[offset+2])<<16)|(std::uint32_t(players[offset+3])<<24);
        if (brain) throw std::logic_error("Guide AI 반납의 현재 비NULL brain 실제 소유자 미연결");
    }
}
}
CampaignGuideRuntime::CampaignGuideRuntime(MissionScript& script,std::uint32_t demo,std::uint32_t mode,std::uint32_t token)
    :saveToken(token),mode64(mode),guided(o::AsciiLower(script.MissionType())=="tutorial") { // 실제 00482bf0의 초기 입력이며 번호 정규화는 단계1에서 실행한다.
    gate.demo=demo;const auto name=o::EncodeOriginalText(script.Name());init.name.assign(name.begin(),name.end());
}
CampaignGuideRuntime::~CampaignGuideRuntime()=default; // 선언 역순으로 Tutorial이 Start보다 먼저 소멸한다.
void ReleaseCampaignGuideMission(Client& client,State& pending,std::unique_ptr<MissionScript>& mission,
    std::unique_ptr<CampaignGuideRuntime>& runtime,std::shared_ptr<MissionFortLoadRequest>& request,
    std::array<std::uint32_t,256>& restrictions,std::uint32_t& missionNumber,std::uint32_t& worldMissionToken,bool& nativeMission) { // 실제 소유 unique_ptr를 직접 반납해 검사별 해제 재구현을 피한다.
    ReleaseMissionFortLoad(pending,request); // 호스트의 이전 Guide 지연 요청만 닫고 Native State/World는 유지한다.
    const auto releaseBase=[&] { // 소멸 본문이 실패해도 원문 unwind처럼 같은 기본 Mission의 Config를 반납한다.
        runtime.reset();worldMissionToken=0; // Tutorial→Start→상태가 Script보다 먼저 끝나며 World 입력은 이 함수의 소유 대상이 아니다.
        ReleaseGuideMissionScript(mission,nativeMission,client.Configuration());
    };
    if (runtime && runtime->guided) {
        auto& game=*client.GameplayGlobals();auto& gate=runtime->gate;
        o::ReleaseTutorialMission({game.DenySalvage(),game.DenyAscend(),missionNumber,gate.tutorialNumber,gate.stage,gate.stageDirty},
            {[&](auto allow) { o::SetTotalMadeAllowAll(restrictions,allow); },[&] { RemoveCurrentGuideBrains(client); },releaseBase});
    } else releaseBase(); // 기본 Mission에는 Tutorial의 허용 표·번호·AI 소멸을 추가하지 않는다.
}
}
