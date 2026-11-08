//41h의 Native 중계는 같은 현재 Client 데이터/UI와 실제 전역 송신을 사용한다.
#pragma once
#include "o/RawPacketDispatch.h"
namespace netstorm::client { //짧은 Meta/Session/World를 callback에 저장하지 않는다.
class Client; //실제 장기 Player/Chat/Registry와 현재 Menu getter의 owner다.
void InstallClientChatPacketReceiver(Client& client,o::RawPacketDispatch& dispatch); //42dc90/40a8d0/version1/lengthffff를 모든 실제 Client 기본표에 설치한다.
}
