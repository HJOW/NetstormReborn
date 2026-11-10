// Native 연결표와 실제 로컬 양끝의 주소/수명 장치를 Client 수명에 보관한다.
#pragma once
#include "o/RawConnectionRegistry.h"
#include "o/RawPacketDelay.h" // 같은 Client의 선택 분포/실제 시계/CRT 장치를 Session 밖에 보유한다.

namespace netstorm::client { //동기화 토큰과 서버 권한은 기존 실제 Client owner에서 빌린다.
// 최초 BSS0만 구성하며 Session 획득/반납이 표나 개수를 다시 초기화하지 않는다.
class ClientPacketSources; //실제 장치의 weak Source 경로만 Client 수명으로 소유한다.
class ClientLocalStateGlobals; //동기화 DWORD의 실제 Client owner다.
class ClientConnectionGlobals final:public o::RawConnectionRegistry {
public:
    void ShutdownDevices(); //UI/시계/장기데이터를반납하기전실제global Node와listener를명시정리하여Source순환을끊는다.
    std::shared_ptr<ClientPacketSources> PacketSources(); //필요한 최초 등록 시 장기 weak 경로 장부만 만들고 socket/State/Runtime은 생성하지 않는다.
    std::shared_ptr<o::PacketDelayGlobals> PacketDelays(); //최초 분포/장치 저장소만 만들고 큐/소켓/난수 소비는 실제 Poll까지 미룬다.
    ClientConnectionGlobals()=default; //새 Client의 원문 고정 배열 수명이다.
    explicit ClientConnectionGlobals(std::shared_ptr<ClientLocalStateGlobals> local); //기존005c8bf4의 같은 주소와 수명을 빌린다.
    ClientConnectionGlobals(const ClientConnectionGlobals&)=delete; //실제 연결 주소 표를 복제하지 않는다.
    ClientConnectionGlobals& operator=(const ClientConnectionGlobals&)=delete; //생존 연결의 현재 칸을 값 대입으로 바꾸지 않는다.
    ClientConnectionGlobals(ClientConnectionGlobals&&)=delete; //연결된 실제 표의 주소를 이동하지 않는다.
    ClientConnectionGlobals& operator=(ClientConnectionGlobals&&)=delete; //현재 span의 주소 수명을 유지한다.
private:
    std::shared_ptr<ClientPacketSources> packetSources_; //Meta/Composite/Editor를 강하게 소유하지 않는 실제 Client 장기 장부다.
    std::shared_ptr<o::PacketDelayGlobals> packetDelays_; //005e4784의 실제 장기 선택과 같은 벽시각/CRT owner이며 World마다 초기화하지 않는다.
};
}
