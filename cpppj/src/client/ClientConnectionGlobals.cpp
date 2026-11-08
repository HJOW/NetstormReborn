// 장기 연결표가 기존 동기화 DWORD의 실제 owner를 함께 유지한다.
#include "client/ClientConnectionGlobals.h"
#include "client/ClientPacketSources.h" //현재 실제 node의 물리 경로만 보관한다.
#include "client/ClientLocalStateGlobals.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { //새 동기화 사본이나 Session별0표를 만들지 않는다.
std::shared_ptr<ClientPacketSources> ClientConnectionGlobals::PacketSources() { if (!packetSources_) packetSources_=std::make_shared<ClientPacketSources>();return packetSources_; } //queue나 실제 Source를 생성하지 않는다.
ClientConnectionGlobals::ClientConnectionGlobals(std::shared_ptr<ClientLocalStateGlobals> local) {
    if (!local) throw std::invalid_argument("연결표의 실제 Client 동기화 owner 누락");auto& synchronization=local->ClockSynchronization().connection;BindSynchronization(synchronization,std::move(local));
}
}
