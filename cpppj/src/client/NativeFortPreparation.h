//Session 없는 현재 Client의 실제 Template/지도 준비를 같은 장기 저장소로 수행한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>

namespace netstorm::client { //World/Meta를 생성하거나 전역의 새 사본을 소유하지 않는 Native 준비 경계다.
class Client;class CampaignTemplateStorage;class RawButtonControls; //현재 Client/Template9/실제 Gump 입력 장치는 caller보다 오래 살아야 한다.
std::array<std::uint8_t,6> PrepareNativeFortDescend(Client& client,CampaignTemplateStorage& templates,RawButtonControls& controls,std::int32_t territory,const std::function<void(std::uint32_t)>& snapshot); //004b3620의 현재자료 준비-only다. snapshot은실제장기저장owner여야하며미연결이면앞선효과뒤선택위치에서실패한다.
}
