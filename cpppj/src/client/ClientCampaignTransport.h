// 실제 Client의 장기 전역과 현재 World 전체를 복합 전송의 수명 계약에 연결한다.
#pragma once
#include "client/CompositeMissionTransport.h"

namespace netstorm::client { // UI/원문 State가 선택한 역할을 바꾸거나 접속을 시작하지 않는다.
class Client;class GameWorld; // 현재 장기 물리와 실제 Session/표시 소비자의 소유자다.
// World 생성 뒤 로그인 시작 전에 호출한다. 같은 전역/Registry를 빌리며 생성 자체는 소켓을 열지 않는다.
std::shared_ptr<CompositeMissionTransport> MakeClientCampaignTransport(Client& client,CompositeTransportRole role,const std::shared_ptr<GameWorld>& world);
}
