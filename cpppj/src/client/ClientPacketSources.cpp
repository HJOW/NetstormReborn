//실제 Source는 선택한 배송 구간에만 보유하고 queue/next7에는 weak 정체성만 남긴다.
#include "client/ClientPacketSources.h"
#include "client/LocalTcpTransport.h" //App종료때실제남은listener를같은device에서닫는다.
#include "client/LocalUdpTransport.h" //같은실제12노드를마무리하는기존Close입구다.
#include "client/MetaConnectionRuntime.h"
#include "client/CompositeMissionTransport.h"
#include "client/EditorLocalTransport.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netstorm::client { //실제 node의 안정된 주소를 숫자 ID/현재 phase와 구별한다.
WeakNativeBattleSource::WeakNativeBattleSource(const NativeBattleSource& source):tcp_(source.network.tcp),udp_(source.network.udp),lifetime_(source.network.lifetime),device_(source.device),composite_(source.network.composite),editor_(source.network.editor),meta_(source.meta) {} //실제 control block만 저장하며 새 명목 owner를 생성하지 않는다.
NativeBattleSource WeakNativeBattleSource::Lock() const {
    auto device=device_.lock();if (!device) return {}; //실제 장치가 없으면 가짜 Meta 없이 원래 State8이 새 연결을 생성한다.
    return {{tcp_.lock(),udp_.lock(),lifetime_.lock(),composite_.lock(),editor_.lock()},meta_.lock(),std::move(device)};
}
void ClientPacketSources::Add(Route route) {
    if (route.device.expired() || !route.shutdown || !route.select) throw std::invalid_argument("실제 Source 경로의 device/선택 장치 누락");
    std::erase_if(routes_,[](const auto& item) { return item.device.expired(); });routes_.push_back(std::move(route)); //종료된 weak 장부만 정리하며 실제 socket/queue를 닫지 않는다.
}
void ClientPacketSources::Publish(const std::shared_ptr<MetaConnectionRuntime>& source) {
    if (!source) throw std::invalid_argument("실제 Meta Source 공개 누락");std::weak_ptr<MetaConnectionRuntime> weak=source;
    Add({source,[weak] { if (const auto current=weak.lock()) { auto network=current->SharedNetwork();network.tcp->CloseAll();network.udp->Close(); } },[weak](const auto& connection)->std::optional<NativeBattleSource> { const auto current=weak.lock();if (!current || !current->OwnsConnection(connection)) return std::nullopt;return NativeBattleSource{current->SharedNetwork(),current,current}; }});
}
void ClientPacketSources::Publish(const std::shared_ptr<CompositeMissionTransport>& source) {
    if (!source) throw std::invalid_argument("실제 Composite Source 공개 누락");std::weak_ptr<CompositeMissionTransport> weak=source;
    Add({source,[weak] { if (const auto current=weak.lock()) current->CloseRoom(); },[weak](const auto& connection)->std::optional<NativeBattleSource> { const auto current=weak.lock();if (!current || !current->OwnsConnection(connection)) return std::nullopt;return NativeBattleSource{current->SharedNetwork(),current->MetaSource(),current}; }});
}
void ClientPacketSources::Publish(const std::shared_ptr<EditorLocalTransport>& source) {
    if (!source) throw std::invalid_argument("실제 Editor Source 공개 누락");std::weak_ptr<EditorLocalTransport> weak=source;
    Add({source,[weak] { if (const auto current=weak.lock()) current->CloseListener(); },[weak](const auto& connection)->std::optional<NativeBattleSource> { const auto current=weak.lock();if (!current || !current->OwnsConnection(connection)) return std::nullopt;return NativeBattleSource{MissionNetwork{{},{},{},{},current},{},current}; }});
}
//Node순환을끊기전실제Source controlblock을동기보유하고기존Source닫힘뒤에weak경로를반납한다.
void ClientPacketSources::Shutdown(const std::function<void()>& closeNodes) {
    if (!closeNodes) throw std::invalid_argument("App최종Node정리장치없음");const auto routes=routes_;std::vector<std::shared_ptr<void>> owners;
    for (const auto& route:routes) if (auto owner=route.device.lock()) owners.push_back(std::move(owner)); //같은현재실제controlblock만보유하며World나가짜lease를생성하지않는다.
    closeNodes();
    for (const auto& route:routes) { const auto effect=route.shutdown;effect(); } //호출값과모든실제Source를유지한채listener/내부장치를마무리한다.
    routes_.clear(); //이미Node와listener정리가끝난뒤에만약한주소검색을닫고이함수끝에서실제owner를반납한다.
}
std::optional<NativeBattleSource> ClientPacketSources::Resolve(const o::PacketConnection& connection) const {
    for (auto at=routes_.rbegin();at!=routes_.rend();++at) { const auto select=at->select;if (auto source=select(connection)) return source; } //실제 node가 같은 최신 coordinator를 먼저 선택하며 현재 수신 함수/type을 다시 고르지 않는다.
    return std::nullopt;
}
}
