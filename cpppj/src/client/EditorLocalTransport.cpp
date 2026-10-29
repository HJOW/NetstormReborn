// Editor에도 실제 zlocal 큐가 있으므로 사용자 명령을 서버 처리기로 동기 우회하지 않는다.
#include "client/EditorLocalTransport.h"
#include "o/RawConnectionCleanup.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 로그인 없는 Editor의 원문 전송 수명과 기존 패킷 분배를 조립한다.
namespace { // 원문의 고정 연결·헤더와 도달한 장치의 유효성만 처리한다.
constexpr std::uint32_t kInitialFlags=0x24U,kConnecting=0x20U,kClosed=0x40U; //004df2c0 초기값과004dfc00/004df440의 실제 상태 비트다.
constexpr std::size_t kReceiveHeaderSize=4; //004df2c0이ff로채운수신버퍼에서합성type1/2가사용하는헤더폭이다.
template<class Function> const Function& Need(const Function& function) { // 선택된 외부 장치가 없으면 선행 효과를 보존하며 그 자리에서 중단한다.
    if (!function) throw std::logic_error("Editor 로컬 전송의 실제 장치 누락");return function;
}
}
EditorLocalTransport::EditorLocalTransport(std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry)
    :hooks_(std::move(hooks)),players_(players),registry_(registry?std::move(registry):std::make_shared<o::RawConnectionRegistry>()),dispatcher_(players_,server,WrappedHooks(),registry_) {
    Need(hooks_.now);Need(hooks_.record);Need(hooks_.terminate);Need(hooks_.close);Need(hooks_.release);
}
EditorLocalTransport::~EditorLocalTransport() { registry_->Forget(clientToken_);registry_->Forget(pendingToken_); }
// 같은 전송의 현재 실제 노드만 숫자 토큰으로 변환한다.
std::uint32_t EditorLocalTransport::Token(const o::PacketConnection& connection) const { if (&connection==&client_) return clientToken_;if (&connection==&pending_) return pendingToken_;throw std::logic_error("Editor 다른 연결의 토큰 조회"); }
// 외부 장치 성공 뒤 실제 큐/존재를 반납하고 그 다음 Registry가 현재 별칭을 정리한다.
std::uint32_t EditorLocalTransport::Track(o::PacketConnection& connection) {
    auto* node=&connection;return registry_->Track(connection,[this,node] { Need(hooks_.close)(*node);SocketClosed(*node); },[this,node] {
        const bool client=node==&client_;Need(hooks_.release)(*node);if (client) { clientAlive_=false;toClient_.clear(); }else { pendingAlive_=false;toPending_.clear(); }
    });
}
o::PacketDispatchHooks EditorLocalTransport::WrappedHooks() {
    o::PacketDispatchHooks result; //기존 분배기가 요구하는 실제 경계만 감싸며 observer/버전/길이 판단은 복제하지 않는다.
    result.now=[this] { return Need(hooks_.now)(); };
    result.record=[this](auto& connection,auto bytes,auto length) { Need(hooks_.record)(connection,bytes,length); };
    result.terminate=[this](auto& connection) { Need(hooks_.terminate)(connection); };
    result.close=[this](auto& connection) { registry_->Destroy(Token(connection)); };
    result.release=[this](auto& connection) { registry_->Release(Token(connection)); };return result;
}
void EditorLocalTransport::Initialize(o::PacketConnection& connection,std::uint32_t sender) {
    connection={};connection.received=Need(hooks_.now)();connection.heartbeat=Need(hooks_.now)();
    connection.sender=sender;connection.flags=kInitialFlags;
    connection.address[0]=20;connection.address[2]=static_cast<std::uint8_t>(port_>>8);connection.address[3]=static_cast<std::uint8_t>(port_);connection.address[16]=0x0d;connection.address[17]=1; // 같은 실제 포트/로컬 주소를 등록 ID 재배치 때도 보존한다.
}
void EditorLocalTransport::Open(std::uint32_t port) {
    if (!port || port>65535U || listenerAlive_ || clientAlive_ || pendingAlive_ || polling_)
        throw std::logic_error("Editor 로컬 포트/새 listener·client 수명 계약 오류");
    port_=port;accepted_=false;clientPeerLost_=pendingPeerLost_=false;toPending_.clear();toClient_.clear(); //새 수명의 바이트 큐만 준비하며 Player/State/게임 난수는 수정하지 않는다.
    clientReceive_.assign(kReceiveHeaderSize,0xff);pendingReceive_.assign(kReceiveHeaderSize,0xff); //새양끝의현재수신헤더만원문초기ff로준비한다.
    listenerAlive_=true;Initialize(listener_,0); //004e04c0은client9보다 먼저 listener의 실제 초기화를 수행한다.
    if (registry_->Cleanup().transceivers[9]) throw std::logic_error("Editor 실제 client9 중복 생성");
    clientAlive_=true;Initialize(client_,9);clientToken_=Track(client_);registry_->Register(9,clientToken_); //004e22c0은 buffered bit2를 켜지 않는다.
}
bool EditorLocalTransport::Alive(const o::PacketConnection& connection) const {
    if (&connection==&client_) return clientAlive_;if (&connection==&pending_) return pendingAlive_;
    throw std::logic_error("Editor 로컬 전송의 다른 연결 조회");
}
bool EditorLocalTransport::PeerLost(const o::PacketConnection& connection) const {
    if (&connection==&client_) return clientPeerLost_;if (&connection==&pending_) return pendingPeerLost_;
    throw std::logic_error("Editor 로컬 전송의 다른 socket 조회");
}
void EditorLocalTransport::SocketClosed(o::PacketConnection& connection) {
    if (&connection==&client_) { if (accepted_ && pendingAlive_) pendingPeerLost_=true; }
    else if (&connection==&pending_) { if (clientAlive_) clientPeerLost_=true; }
    else throw std::logic_error("Editor 로컬 전송의 다른 socket 종료");
    toPending_.clear();toClient_.clear(); //상대socket은0x1000때문에남은프레임을읽지못하고소멸한끝의자체송신버퍼도반납된다.
}
std::vector<std::uint8_t>& EditorLocalTransport::ReceiveBuffer(o::PacketConnection& connection) {
    if (&connection==&client_) return clientReceive_;if (&connection==&pending_) return pendingReceive_;
    throw std::logic_error("Editor 로컬 전송의 다른 수신버퍼 조회");
}
bool EditorLocalTransport::Send(std::int32_t destination,std::span<const std::uint8_t> packet) {
    return SendPacket(destination,packet,{});
}
// 원시 작성기의 observer는 복사 전 같은 mutable 패킷을 직접 변경한다.
bool EditorLocalTransport::SendMutable(std::int32_t destination,std::span<std::uint8_t> packet) { return SendPacket(destination,packet,packet); }
// 전송 가능 여부를 먼저 판정하며 observer와 peer 소멸/실제 복사의 원문 순서를 유지한다.
bool EditorLocalTransport::SendPacket(std::int32_t destination,std::span<const std::uint8_t> packet,std::span<std::uint8_t> writable) {
    if (destination< -32 || destination>21) throw std::out_of_range("Editor 현재 transceiver 번호 범위");
    const bool client=destination==9,pending=destination<0 && pendingAlive_ && destination==std::bit_cast<std::int32_t>(pending_.sender); //현재 실제 pending 번호를 사용하며 항상-1로 가정하지 않는다.
    if ((!client && !pending) || !(client ? clientAlive_ : pendingAlive_)) return false;
    auto& connection=client ? client_ : pending_;if (connection.flags&kClosed) return false;
    if (packet.size()<4) throw std::invalid_argument("Editor 송신 실제 헤더4BYTE 부족");
    const auto& registration=dispatcher_.Dispatch().Registration(packet[1]); //가상송신+4는 observer1을 복사보다 먼저 실행한다.
    if (registration.packetObserver) { if (writable.empty()) throw std::logic_error("Editor 송신의 실제 mutable observer this 없음");registration.NotifyObserver(writable,1); }else { const auto observer=registration.observer;if (observer) observer(1); }
    if (PeerLost(connection)) { Terminated(connection);return false; } //004e1c00의실패뒤가상송신은004df9f0을통해상위종료를분배한다.
    const auto length=std::size_t(packet[2])|(std::size_t(packet[3])<<8); //observer가 변경한 현재 원문 선언 길이를 읽는다.
    if (length<4 || length>packet.size()) throw std::out_of_range("Editor 송신 선언 길이의 실제 버퍼 부족");
    (client ? toPending_ : toClient_).emplace_back(packet.begin(),packet.begin()+static_cast<std::ptrdiff_t>(length));
    return true;
}
void EditorLocalTransport::Accept() {
    if (!listenerAlive_ || !clientAlive_ || accepted_ || !registry_->CanAccept()) return;
    accepted_=true;pendingAlive_=true;Initialize(pending_,0);pendingToken_=Track(pending_);static_cast<void>(registry_->Accept(pendingToken_)); //현재 첫 빈칸이 이 실제 accepted 끝의 음수 sender다.
}
void EditorLocalTransport::Connected(o::PacketConnection& connection) {
    if (!(connection.flags&kConnecting) || PeerLost(connection)) return;
    connection.flags&=~kConnecting; //원문은 연결완료효과보다 먼저connecting bit를 해제한다.
    if (dispatcher_.Dispatch().Registration(1).receive) {
        auto& packet=ReceiveBuffer(connection);packet[1]=1;packet[2]=4;packet[3]=0; //004dfa40도현재수신버퍼+0을보존하며type/length만쓴다.
        static_cast<void>(dispatcher_.DispatchFrame(connection,std::span<const std::uint8_t>(packet).first(kReceiveHeaderSize))); //합성type1은socket pre-observer를 거치지 않는004dfa40의 직접 분배다.
    }
}
void EditorLocalTransport::Terminated(o::PacketConnection& connection) {
    if ((connection.flags&0x10U) || !dispatcher_.Dispatch().Registration(2).receive) return;
    connection.flags|=0x10U;auto& packet=ReceiveBuffer(connection);packet[1]=2;packet[2]=4;packet[3]=0; //004df9f0은마지막수신의현재protocol BYTE를보존하며다시ff로만들지않는다.
    static_cast<void>(dispatcher_.DispatchFrame(connection,std::span<const std::uint8_t>(packet).first(kReceiveHeaderSize))); //server의client9종료처럼상위가반납하지않는원문분기도그대로허용한다.
}
EditorLocalTransport::ReceiveResult EditorLocalTransport::Receive(bool pending) {
    auto& connection=pending ? pending_ : client_;if (!Alive(connection)) return {};
    if (connection.flags&kClosed) { if (!connection.depth) Retire(connection);return {}; }
    Connected(connection);if (!Alive(connection) || (connection.flags&kClosed)) return {};
    if (PeerLost(connection)) { Terminated(connection);return {}; } //004e1a50은상대소멸socket에서기존잔여바이트를성공으로읽지않는다.
    auto& queue=pending ? toPending_ : toClient_;if (queue.empty()) return {};
    auto& packet=ReceiveBuffer(connection);packet=std::move(queue.front());queue.pop_front(); //실제끝이현재수신원문을계속보유하여callback의BYTE수정과다음종료헤더가같은버퍼를본다.
    if (!dispatcher_.Dispatch().Validate(packet)) return {true,false}; //004dfc00의선행Validate실패는길이0sentinel을돌려주며같은끝반복을중단한다.
    if (packet[1]!=4) { //단일프레임의socket observer0과집합내observer0은서로다른원문호출위치다.
        const auto registration=dispatcher_.Dispatch().Registration(packet[1]);registration.NotifyObserver(packet,0);
    }
    const bool dispatched=dispatcher_.DispatchFrame(connection,packet);return {true,dispatched}; //분배실패도소비는했지만004dfc00의반환sentinel이그끝반복을중단한다.
}
std::size_t EditorLocalTransport::Poll() {
    if (polling_) throw std::logic_error("Editor 수신callback 안의 중첩 폴링 금지");
    polling_=true;std::size_t count{}; //중첩검사예외에서도선행수신/송신/상태효과는되돌리지않는다.
    try {
        Accept();if (Receive(true).consumed) ++count; //004df560은negative pending각끝을한번만방문한다.
        for (;;) { //004e28b0은정상packet반환일때만등록9의가상수신을반복하며길이0sentinel에서중단한다.
            const auto result=Receive(false);if (result.consumed) ++count;if (!result.again) break;
        }
    }catch (...) { polling_=false;throw; }polling_=false;return count;
}
void EditorLocalTransport::Retire(o::PacketConnection& connection) {
    if (!Alive(connection) || connection.depth) return;
    auto hooks=WrappedHooks();hooks.close(connection);hooks.release(connection); //원문가상소멸뒤소유메모리반납순서이며Player성공값을쓰지않는다.
}
void EditorLocalTransport::Disconnect(std::int32_t destination) {
    registry_->Disconnect(destination); //같은 현재 표가 선택한 실제 끝만 원문 부모로 종료한다. 수신 중이면 최외곽 분배 복귀까지 반납을 미룬다.
}
void EditorLocalTransport::CloseListener() { //listener가상소멸도실제장치에전달하며양끝의명시정리는호출자가순서대로한다.
    if (!listenerAlive_) return;listener_.flags|=kClosed;Need(hooks_.close)(listener_);Need(hooks_.release)(listener_);listenerAlive_=false;
}
o::RawPacketDispatch& EditorLocalTransport::Dispatch() { return dispatcher_.Dispatch(); } //기존등록/observer/version표의같은소유자다.
std::span<const std::uint8_t> EditorLocalTransport::Players() const { return players_; } // 같은 물리 주소/폭만 반환하며 초기화/복사/폴링을 수행하지 않는다.
o::PacketConnection& EditorLocalTransport::Listener() { return listener_; } //client/accepted끝과다른실제listener의고정주소다.
o::PacketConnection& EditorLocalTransport::Client() { return client_; } //연결저장소주소는전체Editor전송수명동안고정된다.
o::PacketConnection& EditorLocalTransport::Pending() { return pending_; } //음수sender를임의localPlayer로바꾸지않는다.
bool EditorLocalTransport::ClientAlive() const { return clientAlive_; } //조회만으로접속/수신을진행하지않는다.
bool EditorLocalTransport::PendingAlive() const { return pendingAlive_; } //수락전false와반납후false를구별할때다른상태도함께관측한다.
bool EditorLocalTransport::ListenerAlive() const { return listenerAlive_; } //새접속수락가능성의현재실제값이다.
std::uint32_t EditorLocalTransport::Port() const { return port_; } //다른포트/임의고정6799를대신반환하지않는다.
std::size_t EditorLocalTransport::QueuedFrames() const { return toPending_.size()+toClient_.size(); } //Flush가필요없는단일프레임큐다.
// 실제 생성/분배/정리가 빌리는 같은 표의 정체성만 조회한다.
const std::shared_ptr<o::RawConnectionRegistry>& EditorLocalTransport::Registry() const { return registry_; }
}
