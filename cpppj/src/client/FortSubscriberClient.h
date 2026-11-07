// World 없는 등록 메시지도 실제 Client의 같은 Fort 저장소를 변경한다.
#pragma once
#include "client/CampaignTemplateStorage.h"
#include <functional>

namespace netstorm::client { // 장기 Template/Client 전역과 원문 Subscriber 변경 부모의 접합부다.
class Client; // 설정/현재 사용자/난수/표면 모드의 실제 장기 소유자다.
void SynchronizeClientRegisteredSubscriber(Client& client,CampaignTemplateStorage& storage,std::uint32_t id,std::span<const std::uint8_t> hiddenPlayers,const std::function<TemplateFileWriteHooks()>& files); // 이름 NULL의004b32f0→004bd400→004bd360을 실제 저장까지 연결하며 Session을 만들지 않는다.
}
