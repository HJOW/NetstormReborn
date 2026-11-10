// Carrier·동맹·의식의 고정 F를 이전 Session 수명과 분리한다.
#pragma once
#include "o/RawPacketDispatch.h"

namespace netstorm::client { // 같은 장기 Client 데이터와 호출 시점의 실제 Runtime을 사용한다.
class ClientGameplayGlobals;class RawSurfaceStorage; // 현재 게임 데이터와 진단을 소유하는 장기 장치다.
void InstallNativeCarrierPacketReceivers(o::RawPacketDispatch& dispatch,const std::shared_ptr<ClientGameplayGlobals>& game,const std::shared_ptr<RawSurfaceStorage>& storage); // 현재 Session 조회 없이 검토한15행의 같은 F와 metadata를 기본표에 둔다.
void BindNativeCarrierPacketReceiver(o::RawPacketDispatch& dispatch,std::uint32_t type); // 기존 caller의 등록 위치에서 해당 F를 검증하고 이번 lease만 붙인다.
}
