// 캠페인 세션의 외부 요청을 현재 클라이언트의 설정·입력·음향·UI에 연결한다.
#pragma once
#include "client/RawCampaignSession.h"

namespace netstorm::client {
class Client;
class RawButtonControls;
class GameWorld; // 같은 실제 카메라 재설정을 사용하는 현재 월드다.
struct MissionFortLoadRequest; // 실제 State7이 빌릴 현재 요청/Zone과 정리 전 지역 값을 소유한다.
struct CampaignMissionStartEffects; // 실제 미션 Start의 장치/할당 경계를 반환한다.
struct SoloMapCatalogEffects; // 실제 파일 목록의 현재 타입/할당/언어/사용자 경계다.
// 현재 이름의 소유자는 목록보다 오래 살아야 하며 UTF-8이 아닌 원본 좁은 바이트를 반환한다.
SoloMapCatalogEffects MakeSoloMapCatalogClientEffects(Client& client,std::function<std::string()> currentName);
// 실제 UI 소유자가 공급하는 경계다. 월드 생성 중에도 호출할 수 있어야 한다.
struct CampaignClientUi {
    std::function<void()> refreshWindowTitle; // 현재 MissionScript의 설정 제목을 실제 창에 적용한다.
    std::function<void(std::string_view)> tell; // 현재 미션/Tell 설정의 안내 섹션을 연다.
    std::function<void(std::int32_t,std::int32_t,std::int32_t)> removeMessage; // 같은 MessageBoard에서 종류·플레이어·매개변수 검색으로 지연 삭제한다.
    std::function<void(std::span<std::uint8_t>)> receiveChat; // 같은 현재 Chat/Console에 패킷 수신을 전달한다. 실제 게임 UI가 Session보다 오래 산다.
};
// 화면/음향없이같은Client의등록Config/CRT clock/진단을연결한다.Client는반환서비스보다오래살아야한다.
CampaignSubscriberServices MakeCampaignSubscriberClientServices(Client& client);
// 장치 없이도 실제 Client의 장기 Chat/현재 기본이름만 연결한다. 할당·색·전송 경계는 채우지 않는다.
// client는 이 서비스를 사용하는 모든 Session/AI보다 오래 살아야 한다.
void BindCampaignAiClientServices(Client& client,CampaignAiServices& ai);
void BindCampaignLoginStateClientServices(Client& client,RawButtonControls& controls,CampaignLoginServices& login); // 실제10.78의 Renderer/현재 HUD 명령/현재 battlePal만 연결한다. Client와 controls는 모든 로그인 콜백보다 오래 살아야 한다.
void PrepareCampaignMissionStarting(Client& client,MissionFortLoadRequest& request); // 실제 next7을 현재로 소비하고 기본 배경 분모 선택/제목 첫BYTE0/정리 전 지역 값을 같은 Client에 캡처한다.
o::EditorPlayersHooks MakeEditorPlayerClientHooks(Client& client); // 실제 번역과 같은 Client 임시 고리의 정수 서식만 연결한다. 게임 후처리는 Session이 소유한다.
// 클라이언트와 UI 대상은 반환값을 사용하는 세션 및 모든 프로세스보다 오래 살아야 한다.
// 게임 계산·Form 송신·내용물 통지 등 미복원 경계는 채우지 않는다. 실제 GameWorld가 카메라를 공급한다.
RawCampaignSessionServices MakeCampaignClientServices(Client& client,RawButtonControls& controls,CampaignClientUi ui);
// Client/월드/명령 Gump 수명은 반환한 실제 Start 효과보다 오래 유지해야 한다.
CampaignMissionStartEffects MakeCampaignMissionStartEffects(Client& client,GameWorld& world,std::function<void()> refreshCommandGump);
}
