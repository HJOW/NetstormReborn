// 원문 건설/연구의 고정F와 현재 Session 선택을 분리하여 지연큐에 이전 Session을 저장하지 않는다.
#pragma once
#include "o/RawPacketDispatch.h"

namespace netstorm::client { // 장기 Client 데이터만 보유하고 실제 본문 호출 때 현재 Runtime을 선택한다.
class ClientGameplayGlobals;class RawSurfaceStorage; // 실제 현재 게임 전역과 진단의 장기 소유자다.
void InstallNativeConstructionPacketReceivers(o::RawPacketDispatch& dispatch,const std::shared_ptr<ClientGameplayGlobals>& game,const std::shared_ptr<RawSurfaceStorage>& storage); //37/3e/3f/58의 고정F와 원문 metadata를 첫 메뉴/Session 전에 설치한다.
void BindNativeConstructionCommandReceivers(o::RawPacketDispatch& dispatch); //현재3e/3f의 같은 불변F에 이번 Session lease만 붙인다.
void BindNativeConstructionDataReceiver(o::RawPacketDispatch& dispatch); //buildingData 확인 뒤 현재37h의 같은F를 대여한다.
void BindNativeFactoryResearchReceiver(o::RawPacketDispatch& dispatch); //현재58h의 같은F를 대여하며 독립 typed 등록은 건드리지 않는다.
}
