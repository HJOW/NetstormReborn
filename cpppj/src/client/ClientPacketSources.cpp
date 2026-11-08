//실제 Source는 선택한 배송 구간에만 보유하고 queue/next7에는 weak 정체성만 남긴다.
#include "client/ClientPacketSources.h"
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
    if (route.device.expired() || !route.select) throw std::invalid_argument("실제 Source 경로의 device/선택 장치 누락");
    std::erase_if(routes_,[](const auto& item) { return item.device.expired(); });routes_.push_back(std::move(route)); //종료된 weak 장부만 정리하며 실제 socket/queue를 닫지 않는다.
}
void ClientPacketSources::Publish(const std::shared_ptr<MetaConnectionRuntime>& source) {
    if (!source) throw std::invalid_argument("실제 Meta Source 공개 누락");std::weak_ptr<MetaConnectionRuntime> weak=source;
    Add({source,[weak](const auto& connection)->std::optional<NativeBattleSource> { const auto current=weak.lock();if (!current || !current->OwnsConnection(connection)) return std::nullopt;return NativeBattleSource{current->SharedNetwork(),current,current}; }});
}
void ClientPacketSources::Publish(const std::shared_ptr<CompositeMissionTransport>& source) {
    if (!source) throw std::invalid_argument("실제 Composite Source 공개 누락");std::weak_ptr<CompositeMissionTransport> weak=source;
    Add({source,[weak](const auto& connection)->std::optional<NativeBattleSource> { const auto current=weak.lock();if (!current || !current->OwnsConnection(connection)) return std::nullopt;return NativeBattleSource{current->SharedNetwork(),current->MetaSource(),current}; }});
}
void ClientPacketSources::Publish(const std::shared_ptr<EditorLocalTransport>& source) {
    if (!source) throw std::invalid_argument("실제 Editor Source 공개 누락");std::weak_ptr<EditorLocalTransport> weak=source;
    Add({source,[weak](const auto& connection)->std::optional<NativeBattleSource> { const auto current=weak.lock();if (!current || !current->OwnsConnection(connection)) return std::nullopt;return NativeBattleSource{MissionNetwork{{},{},{},{},current},{},current}; }});
}
std::optional<NativeBattleSource> ClientPacketSources::Resolve(const o::PacketConnection& connection) const {
    for (auto at=routes_.rbegin();at!=routes_.rend();++at) { const auto select=at->select;if (auto source=select(connection)) return source; } //실제 node가 같은 최신 coordinator를 먼저 선택하며 현재 수신 함수/type을 다시 고르지 않는다.
    return std::nullopt;
}
}
