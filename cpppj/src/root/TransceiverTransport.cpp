// 같은 R 번호표/실제 OS 노드/공통 패킷 표의 소유권과 메인 통신 부모를 접합한다.
#include "root/TransceiverInternals.h"
#include <algorithm>

namespace netstorm::root { // 원문 R 전역 표와 고정 노드의 수명을 분리한다.
using transceiver_detail::Need;
TransceiverTransport::TransceiverTransport(ConnectionState& connections,NetworkState& network,TransceiverHooks hooks,std::shared_ptr<o::PacketRegistrationTable> registrations)
    :connections_(connections),network_(network),hooks_(std::move(hooks)),dispatch_(o::OriginalEdition::Patch1078,DispatchHooks(),std::move(registrations)) { // 두 PE의137종/138길이 동일 근거로 기본 프로토콜 표만 공유한다.
    if (!hooks_.now || !hooks_.captureNow || !hooks_.record || !hooks_.assertion || !hooks_.log) throw std::invalid_argument("R 전송 시계/기록/진단 장치 누락");
}
TransceiverTransport::~TransceiverTransport() { // 최종 호스트 소유권 반납은 임의 사용자 callback/실제 게임 종료를 다시 실행하지 않는다.
    for (const auto& node:nodes_) { // 자기 토큰의 일반/대기 별칭만 반납한다. 원문 pendingCount를 임의 초기화하지 않는다.
        for (auto& slot:connections_.transceivers) if (slot==node->token) slot=0;
        for (auto& slot:connections_.pending) if (slot==node->token) slot=0;
    }
}
TransceiverTransport::Node& TransceiverTransport::Get(std::uint32_t token) { // 이미 반납된 세대는 실제 메모리 재사용으로 해석하지 않는다.
    for (const auto& node:nodes_) if (node->token==token && !node->retired && !node->destroyed) return *node; // 자기 생존 숫자 토큰만 조회한다.
    throw std::out_of_range("R 생존 transceiver 토큰 범위");
}
TransceiverTransport::Node& TransceiverTransport::From(o::PacketConnection& connection) { // 소멸/반납 두 단계 사이도 같은 물리를 찾는다.
    for (const auto& node:nodes_) if (&node->connection==&connection) return *node; // 현재 sender가 바뀌어도 주소 정체성은 유지한다.
    throw std::out_of_range("R 다른 전송 owner의 연결");
}
void TransceiverTransport::Collect() { if (!activeCalls_) nodes_.remove_if([](const auto& node) { return node->retired; }); } // close/release callback 바깥에서만 물리를 수거한다.
std::uint32_t TransceiverTransport::Adopt(std::unique_ptr<SocketDevice> socket,std::uint32_t sender,std::uint32_t capacity,bool listener,std::uint32_t acceptedCapacity) {
    if (!socket || capacity<4) throw std::invalid_argument("R 실제 socket/최소 헤더 버퍼 누락");auto node=std::make_unique<Node>();node->socket=std::move(socket);
    node->connection.received=Need(hooks_.captureNow)();node->connection.heartbeat=Need(hooks_.captureNow)(); //842a0의 두 별도 지역 FullTime 캡처가 할당보다 앞선다.
    if (node->socket->Flags()&0x418U) node->connection.address=node->socket->QueryPeerAddress();
    node->connection.sender=sender;node->bufferSize=capacity;node->acceptedCapacity=acceptedCapacity;node->listener=listener;node->input.assign(capacity,0xff);
    if (node->socket->Flags()&0x418U) node->write.assign(capacity,0xff);node->connection.flags=0x24U; // 실제 읽기/쓰기 할당 소유4와 최초접속20이다. buffered2는 부모가 명시 요청한다.
    if (serial_==std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("R 전송 숫자 토큰 소진");node->token=++serial_;const auto token=node->token;nodes_.push_back(std::move(node));return token;
}
std::uint32_t TransceiverTransport::Open(std::uint32_t port,std::uint32_t capacity,bool listener) {
    const CallGuard guard(*this);const auto id=FindFreeTransceiver(connections_);if (!id) return 0;
    auto socket=std::make_unique<SocketDevice>();if (listener) static_cast<void>(socket->Listen(port));else static_cast<void>(socket->OpenDatagram(port)); // 실패한 실제 socket도 원문 버퍼 ctor/공개/반납 순서를 통과한다.
    const auto token=Adopt(std::move(socket),listener ? 0U:id,listener ? 8U:capacity,listener,capacity);if (listener) InitializePending(connections_);
    connections_.transceivers[id]=token;if (!Get(token).socket->IsOpen()) { Close(token);connections_.transceivers[id]=0;return 0; }return id;
}
std::uint32_t TransceiverTransport::OpenDatagram(std::uint32_t port,std::uint32_t capacity) { return Open(port,capacity,false); } // R LAN의 원문 기본512 물리를 사용한다.
std::uint32_t TransceiverTransport::Listen(std::uint32_t port,std::uint32_t capacity) { return Open(port,capacity,true); } // 수락용 물리는 기본4096이며 listener8과 구별한다.
std::uint32_t TransceiverTransport::TokenFor(const o::PacketConnection& connection) const {
    for (const auto& node:nodes_) if (&node->connection==&connection && !node->retired && !node->destroyed) return node->token; // 현재 sender/소유자 번호와 별개인 실제 물리 정체성이다.
    return 0;
}
o::PacketConnection& TransceiverTransport::Connection(std::uint32_t token) { return Get(token).connection; } // 현재 sender/깊이/주소/시각을 같은 노드에서 빌린다.
std::uint32_t& TransceiverTransport::NoMainTimeout(std::uint32_t token) { return Get(token).noMainTimeout; } // 실제+90의 현재 참조다.
std::int32_t& TransceiverTransport::WriteLimit(std::uint32_t token) { return Get(token).writeLimit; } // 실제+70의 현재 signed 참조다.
ConnectionHooks TransceiverTransport::Lifecycle() {
    ConnectionHooks result;result.assertion=hooks_.assertion;result.close=[this](auto token) { Close(token); };result.sender=[this](auto token,auto id) { Get(token).connection.sender=id; }; // pending 등록과 일반 승격은 같은 숫자 노드를 쓴다.
    result.endParticipant=hooks_.endParticipant;result.exit=hooks_.exit;return result;
}
void TransceiverTransport::InstallTerminationReceiver() { // 실제 Root 초기화가 선택한 위치에서만75170을 등록한다.
    dispatch_.Register(2,[this](auto sender,auto&,auto) { ReceiveConnectionEnded(connections_,std::bit_cast<std::int32_t>(sender),Lifecycle()); });
}
o::PacketDispatchHooks TransceiverTransport::DispatchHooks() {
    o::PacketDispatchHooks result;result.now=[this] { return Need(hooks_.now)(); };result.record=[this](auto& connection,auto packet,auto length) { Need(hooks_.record)(false,connection,packet,length); };
    result.terminate=[this](auto& connection) { Emit(From(connection),2); };result.close=[this](auto& connection) { Destroy(From(connection)); };result.release=[this](auto& connection) { From(connection).retired=true; };return result;
}
void TransceiverTransport::Emit(Node& node,std::uint8_t type) { // 제어 패킷은 observer를 추가 호출하지 않고 같은 현재 입력 BYTE0을 유지한다.
    if (!dispatch_.Registration(type).receive || (type==2 && (node.connection.flags&0x10U))) return;
    if (type==2) node.connection.flags|=0x10U;transceiver_detail::Header(node.input,type,4);static_cast<void>(o::DispatchPacketFrame(dispatch_,node.connection,std::span(node.input).first(4),DispatchHooks()));
}
void TransceiverTransport::Terminate(std::uint32_t token) { const CallGuard guard(*this);Emit(Get(token),2); } // 최초 실제 종료 처리기 통지만 요청한다.
void TransceiverTransport::Destroy(Node& node) {
    if (node.destroyed || node.destroying) return;node.destroying=true;
    try { Flush(node); } catch (...) { node.socket->Close();node.destroyed=true;node.destroying=false;throw; } // 예외에서도 자기 OS 핸들은 반납하되 앞선 버퍼 효과는 되돌리지 않는다.
    node.socket->Close();node.input.clear();node.write.clear();node.destroyed=true;node.destroying=false; // 원문84620은 close 전에 같은 buffered flush를 수행한다.
}
void TransceiverTransport::Close(std::uint32_t token) {
    const CallGuard guard(*this);auto& node=Get(token);node.connection.flags|=0x40U;if (!node.connection.depth) { Destroy(node);node.retired=true; } // depth가 있으면 공통 분배 복귀가 실제 소멸/반납을 완료한다.
}
NetworkPumpResult TransceiverTransport::Pump(std::int32_t waitSeconds,NetworkHooks upper) {
    if (pumping_) throw std::logic_error("R 메인 통신 순회 재진입");const CallGuard guard(*this);pumping_=true;
    upper.clock=hooks_.captureNow;upper.lastReceived=[this](auto token) { return Get(token).connection.received; };upper.noMainTimeout=[this](auto token) { return Get(token).noMainTimeout; };
    upper.socketFlags=[this](auto token) { return Get(token).socket->Flags(); };upper.socket=[this](auto token) { return Get(token).socket->Handle(); };upper.socketStatus=[this](auto token) { return Get(token).socket->Status(); };
    upper.describe=[this](auto token) { return Need(hooks_.formatAddress)(Get(token).connection.address); };upper.log=hooks_.log;upper.assertion=hooks_.assertion;upper.exit=hooks_.exit;
    upper.terminate=[this](auto token) { Terminate(token); };upper.closePending=[this](auto id) { ClosePending(connections_,id,Lifecycle()); };upper.select=SocketDevice::Select;
    upper.receive=[this](auto token,auto dispatch) { static_cast<void>(ReceiveOne(token,dispatch!=0)); };upper.flush=[this](auto token) { Flush(token); }; // Challenge/보조 객체의 실제 장치는 upper에 남긴다.
    try { const auto result=PumpNetwork(connections_,network_,waitSeconds,upper);pumping_=false;return result; } catch (...) { pumping_=false;throw; }
}
}
