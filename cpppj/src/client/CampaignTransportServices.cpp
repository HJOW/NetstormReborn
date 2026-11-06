// 적용 전 준비본: 실제 backend 호출만 연결하며 빈 성공/Player 성공값/소켓 실행을 추가하지 않는다.
#include "client/CampaignTransportServices.h"
#include "client/LocalTcpTransport.h"
#include "client/EditorLocalTransport.h"
#include "client/CompositeMissionTransport.h"
#include "zacket/LocalMissionTransport.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 호스트 State가 주입하는 현재 전송 계약의 독립 backend 바인더다.
namespace { // 원문 주소/현재 표 조회만 수행하는 OS 없는 도구다.
// 같은 Registry의 현재 ID를 읽는다. 노드 주소를 callback 경계 밖으로 보관하지 않는다.
o::PacketConnection* Current(const std::shared_ptr<o::RawConnectionRegistry>& registry,std::int32_t id) {
    auto table=registry->Cleanup();if (id<=-static_cast<std::int32_t>(table.pending.size()) || id>=static_cast<std::int32_t>(table.transceivers.size())) throw std::out_of_range("현재 전송 ID 범위 오류");
    const auto token=id<0 ? table.pending[static_cast<std::size_t>(-id)] : table.transceivers[static_cast<std::size_t>(id)];return token ? &registry->Node(token) : nullptr;
}
// 원문004e0a30(1,0,0)의 IP 또는 wildcard와 포트를 DNS 없이 작성한다.
std::string PeerText(const std::array<std::uint8_t,20>& address) {
    const auto family=static_cast<std::uint32_t>(address[0])|(static_cast<std::uint32_t>(address[1])<<8);
    if (family==6) throw std::logic_error("LAN 전송 바인더의 SPX 주소는 범위 밖입니다");
    const bool wildcard=address[4]==0 && address[5]==0 && address[6]==0 && address[7]==0;
    auto text=wildcard ? std::string("*") : std::to_string(address[4])+'.'+std::to_string(address[5])+'.'+std::to_string(address[6])+'.'+std::to_string(address[7]);
    return text+':'+std::to_string((static_cast<std::uint32_t>(address[2])<<8)|address[3]);
}
// 필수 실제 owner/표만 요구하며 생성자에서 연결이나 수신을 진행하지 않는다.
template<class Transport> CampaignTransportServices Basic(const std::shared_ptr<Transport>& transport) {
    if (!transport || !transport->Registry()) throw std::invalid_argument("실제 전송 owner/현재 Registry가 필요합니다");CampaignTransportServices services;services.lifetime=transport;services.registry=transport->Registry();services.dispatch=&transport->Dispatch();
    services.send=[transport](auto id,auto packet) { return transport->Send(id,packet); };services.sendMutable=[transport](auto id,auto packet) { return transport->SendMutable(id,packet); };services.poll=[transport] { return transport->Poll(); };
    services.address=[registry=services.registry](auto id) { return registry->Address(id); };services.peerAddress=[address=services.address](auto id) { return PeerText(address(id)); };services.close=[registry=services.registry](auto id) { registry->Disconnect(id); };return services;
}
}
// 기존 단독 미션은 같은 내부 두끝을 사용하며 buffered flag를 연결완료로 오인하지 않는다.
CampaignTransportServices BindInternalCampaignTransport(std::shared_ptr<zacket::LocalMissionTransport> transport) {
    auto services=Basic(transport);services.serve=[transport](auto port) { transport->Open(port,false); };
    services.connect=[transport](auto address,auto) { if (address.size()!=20) throw std::out_of_range("현재 내부 State8 주소는20BYTE여야 합니다");transport->Connect(); }; // 내부 선택 모드만 주소 해석 없이 기존 zlocal 서버에 붙는다.
    services.connectPeerText=[transport](auto,auto) { transport->Connect(); }; // 같은 명시 내부 역할은 문자 주소에서도 실제 내부client9만 연다.
    services.assign=[transport](auto pending,auto owner) { transport->AssignConnection(pending,owner); };services.flush=[transport] { transport->Flush(); };services.peer=[transport](auto owner) { return transport->Peer(owner); };
    services.connection=[transport](auto id) { return transport->Connection(id); };
    services.exists=[connection=services.connection](auto id) { return connection(id)!=nullptr; };
    services.isOpen=[transport](auto id) { return transport->ConnectionOpen(id); };
    services.status=[transport](auto id) { return transport->ConnectionStatus(id); };return services; // 실제 내부 Connect/Accept 결과와 상대 수명을 읽고 bit2는 사용하지 않는다.
}
// 실제 TCP 상태/주소와 accepted 노드를 그대로 사용하며 내부 자기끝을 선생성하지 않는다.
CampaignTransportServices BindTcpCampaignTransport(std::shared_ptr<LocalTcpTransport> transport) {
    auto services=Basic(transport);services.serve=[transport](auto port) { transport->Listen(port); };services.listen=services.serve;services.connect=[transport](auto address,auto port) { transport->Connect(address,port,9); };services.connectPeerText=[transport](auto address,auto port) { transport->ConnectPeerText(address,port,9); };
    services.assign=[transport](auto pending,auto owner) { transport->AssignConnection(pending,owner); };services.flush=[transport] { transport->Flush(); };services.peer=[transport](auto owner) { return transport->Peer(owner); };
    services.connection=[transport](auto id) { return transport->Connection(id); };services.exists=[transport](auto id) { return transport->HasConnection(id); };services.isOpen=[transport](auto id) { return transport->IsOpen(id); };services.status=[transport](auto id) { return transport->Status(id); };return services;
}
// Editor에서 존재하지 않는 Login/할당/flush 동작을 빈 성공 callback으로 만들지 않는다.
CampaignTransportServices BindEditorCampaignTransport(std::shared_ptr<EditorLocalTransport> transport) {
    auto services=Basic(transport);
    services.connection=[transport](auto id)->o::PacketConnection* { auto* node=Current(transport->Registry(),id);return node==&transport->Client() || node==&transport->Pending() ? node : nullptr; };
    services.exists=[connection=services.connection](auto id) { return connection(id)!=nullptr; };services.isOpen=[connection=services.connection](auto id) { const auto* node=connection(id);return node && !(node->flags&0x40U); };
    services.status=[open=services.isOpen](auto id)->std::int32_t { return open(id) ? 1 : 0; };
    services.peer=[send=services.send](auto owner) { return [send,owner](auto packet) { static_cast<void>(send(std::bit_cast<std::int32_t>(owner),packet)); }; };return services;
}
// 하나의 실제 coordinator를 모든 writer/State/Chat/배치 부모에 공급하고 Session 수신 대여도 반납한다.
CampaignTransportServices BindCompositeCampaignTransport(std::shared_ptr<CompositeMissionTransport> transport,const std::shared_ptr<void>& sessionOwner) {
    if (!transport) throw std::invalid_argument("복합 전송 실제 owner 누락");auto lease=transport->BindSession(sessionOwner);auto services=Basic(transport);
    services.detachSession=[lease] { lease->Detach(); }; // 자기 세대만 즉시 detach하며 실행 중 callback은 같은 owner pin으로 정상 복귀한다.
    services.callbacksActive=[transport] { return transport->ConnectionCallbackActive(); }; // table의 현재/반납 대여를 함께 조회한다.
    services.address=[transport](auto id) { return transport->Address(id); };services.peerAddress=[address=services.address](auto id) { return PeerText(address(id)); }; // 다른 backend owner의 주소도 canonical 현재 ID 검증을 거친다.
    services.serve=[transport](auto port) { transport->Serve(port); };services.listen=[transport](auto port) { transport->ListenLan(port); };services.connect=[transport](auto address,auto port) { transport->Connect(address,port); };services.assign=[transport](auto pending,auto owner) { transport->Assign(pending,owner); };services.flush=[transport] { transport->Flush(); };services.peer=[transport](auto owner) { return transport->Peer(owner); };
    services.connectPeerText=[transport](auto address,auto port) { transport->ConnectPeerText(address,port); }; // 저장 문자열도 같은 Registry/등록 대여/역할을 사용한다.
    services.connection=[transport](auto id) { return transport->Connection(id); };services.exists=[transport](auto id) { return transport->Exists(id); };services.isOpen=[transport](auto id) { return transport->IsOpen(id); };services.status=[transport](auto id) { return transport->Status(id); };return services;
}
}
