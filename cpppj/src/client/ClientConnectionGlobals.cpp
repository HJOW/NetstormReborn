// 장기 연결표가 기존 동기화 DWORD의 실제 owner를 함께 유지한다.
#include "client/ClientConnectionGlobals.h"
#include "client/ClientPacketSources.h" //현재 실제 node의 물리 경로만 보관한다.
#include "client/ClientLocalStateGlobals.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { //새 동기화 사본이나 Session별0표를 만들지 않는다.
std::shared_ptr<ClientPacketSources> ClientConnectionGlobals::PacketSources() { if (!packetSources_) packetSources_=std::make_shared<ClientPacketSources>();return packetSources_; } //queue나 실제 Source를 생성하지 않는다.
// 최초 호출은 분포만 준비하며 장치/큐를 실행하지 않는다.
std::shared_ptr<o::PacketDelayGlobals> ClientConnectionGlobals::PacketDelays() { if (!packetDelays_) packetDelays_=std::make_shared<o::PacketDelayGlobals>();return packetDelays_; }
//실제Source들을먼저동기호출동안보유하여Node해제뒤에도같은listener정리를완료한다.
void ClientConnectionGlobals::ShutdownDevices() {
    if (packetSources_) packetSources_->Shutdown([this] { ShutdownTrackedDevices(); });else ShutdownTrackedDevices();
}
ClientConnectionGlobals::ClientConnectionGlobals(std::shared_ptr<ClientLocalStateGlobals> local) {
    if (!local) throw std::invalid_argument("연결표의 실제 Client 동기화 owner 누락");auto& synchronization=local->ClockSynchronization().connection;BindSynchronization(synchronization,std::move(local));
    const auto sources=PacketSources();BindDevicePin([sources](const o::PacketConnection& node)->std::shared_ptr<void> { const auto selected=sources->Resolve(node);if (!selected || !selected->device) throw std::logic_error("현재Client node의실제Source경로없음");return selected->device; },true); //실제Source만등록Node끝까지보유하며World/Session이나대기receiver에pin을붙이지않는다.
}
}
