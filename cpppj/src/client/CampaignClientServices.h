// 캠페인 세션의 외부 요청을 현재 클라이언트의 설정·입력·음향·UI에 연결한다.
#pragma once
#include "client/RawCampaignSession.h"

namespace netstorm::client {
class Client;
class RawButtonControls;
// 실제 UI 소유자가 공급하는 경계다. 월드 생성 중에도 호출할 수 있어야 한다.
struct CampaignClientUi {
    std::function<void()> refreshWindowTitle; // 현재 MissionScript의 설정 제목을 실제 창에 적용한다.
    std::function<void(std::string_view)> tell; // 현재 미션/Tell 설정의 안내 섹션을 연다.
};
// 클라이언트와 UI 대상은 반환값을 사용하는 세션 및 모든 프로세스보다 오래 살아야 한다.
// 게임 계산·Form 송신·내용물 통지 등 미복원 경계는 채우지 않는다. 실제 GameWorld가 카메라를 공급한다.
RawCampaignSessionServices MakeCampaignClientServices(Client& client,RawButtonControls& controls,CampaignClientUi ui);
}
