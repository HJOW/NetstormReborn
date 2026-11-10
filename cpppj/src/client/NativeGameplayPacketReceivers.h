// 남은 gameplay 고정 F는 이전 Session 대신 장기 Client 전역만 보유한다.
#pragma once
#include "o/RawPacketDispatch.h"
namespace netstorm::client { //배송 때 현재 실제 Runtime을 선택하고 같은 본문을 호출한다.
class ClientGameplayGlobals;class RawSurfaceStorage; //현재 게임 데이터와 진단을 소유하는 장기 장치다.
void InstallNativeGameplayPacketReceivers(o::RawPacketDispatch& dispatch,const std::shared_ptr<ClientGameplayGlobals>& game,const std::shared_ptr<RawSurfaceStorage>& storage); //현재 Session 조회 없이26F/metadata를 기본표에 둔다.
void BindNativeGameplayPacketReceiver(o::RawPacketDispatch& dispatch,std::uint32_t type); //원래 caller 위치에서 같은 F/metadata를 검증하고 이번 lease만 붙인다.
}
