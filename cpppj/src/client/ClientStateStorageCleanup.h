// Native State 정리의 시계동기화·Zone·MetaZone을 같은 Client 저장소에 연결한다.
#pragma once
#include "o/RawStateWorldCleanup.h"

namespace netstorm::client { //전체 State 부모·Runtime 반납·모드 전환을 이 작은 정리에 추가하지 않는다.
class Client;class ClientLocalStateGlobals;class RawSurfaceStorage; //현재 실제 owner와 선택된 Pool을 빌린다.
void ResetClientClockSynchronization(ClientLocalStateGlobals& globals); //004c1f20의 세 실제 DWORD만 지운다.
void ResetClientStateZone(ClientLocalStateGlobals& globals); //004bfd60의 같은 Zone 앞6BYTE만 지운다.
void ClearClientMetaZoneRecords(Client& client,RawSurfaceStorage& storage); //0042b1b0→00479520을 실행하고 선택된 삭제만 현재 Session에 요구한다.
void ResetClientSquidPool(Client& client,RawSurfaceStorage& storage,int capacity=0); //UI/Kernel/Runtime 반납 뒤004af170의 같은 Pool·기록·실패점을 정리한다. Player/모드/Graph는 건드리지 않는다.
void ResetClientStateWorldStorage(Client& client,RawSurfaceStorage& storage,o::StateWorldCleanupEffect effect); //실제 World 반납 뒤 여덟 물리 정리만 선택 실행한다. 다른 Effect는 명시 거부하며 부모 전체를 대신하지 않는다.
}
