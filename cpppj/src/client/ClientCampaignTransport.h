// 실제 Client의 장기 전역과 현재 World 전체를 복합 전송의 수명 계약에 연결한다.
#pragma once
#include "client/CompositeMissionTransport.h"

namespace netstorm::client { // UI/원문 State가 선택한 역할을 바꾸거나 접속을 시작하지 않는다.
class Client;class GameWorld; // 현재 장기 물리와 실제 Session/표시 소비자의 소유자다.
// 같은 Client storage의 실제 진단만 빌려2a/2b/67/81 관찰기를 Session 대여 밖의 기본 표에 설치한다. 수신기는 추가하지 않는다.
void InstallClientMissionSendObservers(Client& client,o::RawPacketDispatch& dispatch);
// a2 관찰기·C3/관찰 전용 표·0b버전조회·7b현재CS주소 수신을 같은 장기 Client 기본 표에 설치한다. Session 대여 전에 호출한다.
void InstallClientPacketDefaults(Client& client,o::RawPacketDispatch& dispatch);
void InstallClientStateBattleStartReceivers(Client& client,o::RawPacketDispatch& dispatch); //004b4df0/4b8b20의 고정 F와 실제 기본 metadata를 장기 Client에 연결한다.
void InstallClientConnectionEndedReceiver(Client& client,o::RawPacketDispatch& dispatch); //004b6660의 같은 장기 Client 본문을 Meta/일반에 공유하며 실제 저장·방송 하위만 현재 Runtime을 pin한다.
// World 생성 뒤 로그인 시작 전에 호출한다. 같은 전역/Registry를 빌리며 생성 자체는 소켓을 열지 않는다.
std::shared_ptr<CompositeMissionTransport> MakeClientCampaignTransport(Client& client,CompositeTransportRole role,const std::shared_ptr<GameWorld>& world,MissionNetwork network={},std::shared_ptr<MetaConnectionRuntime> meta={});
}
