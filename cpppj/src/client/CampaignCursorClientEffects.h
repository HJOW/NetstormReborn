// 실제 Client의 장치와 현재 Gump 트리를 같은 캠페인 커서에 공급한다.
#pragma once
#include "client/RawCampaignCursor.h"

namespace netstorm::client {
class Client; // 장치·글꼴·설정의 장기 소유자다.
class RawButtonControls; // 현재 표시와 입력이 공유하는 단일 Gump 트리다.
// 현재 트리 선택·재귀 수신·게임 부모 SID·직접 수신을 연결한다. 게임 계산과 전송은 유지한다.
void BindCampaignCursorGumps(CampaignCursorEffects& effects,RawButtonControls& controls);
// 클라이언트와 트리는 반환한 모든 콜백보다 오래 살아야 한다. 미복원 효과는 기존 입력을 보존한다.
CampaignCursorEffects MakeCampaignCursorClientEffects(Client& client,RawButtonControls& controls,
    CampaignCursorEffects effects={});
}
