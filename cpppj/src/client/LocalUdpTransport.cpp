// 원문 Ztrans004dfc00/004e00e0과 Zocket004e1d70/004e1e90의 UDP 경계를 복원한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //구형소켓선언을Windows헤더에서제외한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX //표준min/max와Windows매크로의충돌을막는다.
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS //원문자기호스트gethostbyname만사용하며폐기된외부서버해석은추가하지않는다.
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "client/LocalUdpTransport.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace netstorm::client { //각UDP노드는같은Client의실제표에현재번호12로만공개한다.
namespace { //실제데이터그램필드/소켓수명과로컬브로드캐스트주소만다룬다.
constexpr std::uint32_t kClosed=0x40,kTerminated=0x10; //PacketConnection의상위닫힘/종료통지비트다.
constexpr std::size_t kCapacity=4096; //004e2bd0이004dfb20에전달한현재수신물리크기다.
// SOCKET의플랫폼폭을보존하며저장한정수핸들을복원한다.
SOCKET Socket(std::uintptr_t value) { return static_cast<SOCKET>(value); }
// 물리헤더가있는현재패킷의비정렬선언WORD를읽는다.
std::size_t Length(std::span<const std::uint8_t> packet) { if (packet.size()<4) throw std::out_of_range("UDP 실제헤더부족");return std::size_t(packet[2])|(std::size_t(packet[3])<<8); }
// 폐기된유효성검증은직접/집합송신모두에서실제wire이전에제외한다.
bool Retired(std::span<const std::uint8_t> packet) {
    if (packet[1]==0x7f || packet[1]==0x80) return true;if (packet[1]!=4) return false;const auto total=Length(packet);if (total<4 || total>packet.size()) throw std::out_of_range("UDP 집합물리부족");
    for (std::size_t at=4;at<total;) { //단일집합의실제내부패킷경계만확인하며중첩집합은기존TCP처럼거부한다.
        if (total-at<4) throw std::out_of_range("UDP 내부헤더부족");const auto length=Length(packet.subspan(at));if (length<4 || length>total-at) throw std::out_of_range("UDP 내부길이오류");if (packet[at+1]==0x7f || packet[at+1]==0x80) return true;if (packet[at+1]==4) throw std::logic_error("UDP 중첩집합미복원");at+=length;
    }return false;
}
// network endian주소/포트를원문recvfrom결과20BYTE에복사한다.마지막프로토콜DWORD는원문처럼0이다.
std::array<std::uint8_t,20> AddressBytes(const sockaddr_in& address) {
    std::array<std::uint8_t,20> result{};result[0]=AF_INET;std::memcpy(result.data()+2,&address.sin_port,2);std::memcpy(result.data()+4,&address.sin_addr,4);return result;
}
// 첫공개전예외가발생한OS핸들만이작은owner가반납한다.
struct SocketOwner {
    SOCKET socket{INVALID_SOCKET}; //아직Node/Registry에공개하지않은실제핸들이다.
    ~SocketOwner() { if (socket!=INVALID_SOCKET) closesocket(socket); } //미완성열기의소켓을누수시키지않는다.
    SOCKET Release() { const auto value=socket;socket=INVALID_SOCKET;return value; } //Node에같은핸들의반납권한을한번넘긴다.
};
// callback중같은owner의노드를물리수거하지않게한다.
struct CallGuard {
    std::size_t& count; //동일owner가보관하는현재외곽호출대여수다.
    explicit CallGuard(std::size_t& value):count(value) { ++count; } //동기호출대여를시작한다.
    ~CallGuard() { --count; } //예외복귀에서도대여를반납한다.
};
}
struct LocalUdpTransport::Node {
    o::PacketConnection connection; //같은sender12/깊이/주소/닫힘을실제분배기가읽고쓴다.
    std::array<std::uint8_t,kCapacity> input; //Native초기ff뒤수신이덮은BYTE만다음오류통지에서도유지한다.
    std::uintptr_t socket{kInvalidSocket};std::uint32_t token{}; //실제소켓과현재Registry숫자식별자다.
    bool retired{}; //실제반납완료와아직생존하는callback물리를구별한다.
    Node() { input.fill(0xff); } //004df2c0의수신버퍼전체ff초기화를새할당에서만수행한다.
    ~Node() { if (socket!=kInvalidSocket) closesocket(Socket(socket)); } //최종owner/등록실패의남은핸들만반납한다.
};
// TCP와같은표를빌리되실제UDP열기는명시Open까지지연한다.
LocalUdpTransport::LocalUdpTransport(std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry,std::shared_ptr<o::PacketRegistrationTable> registrations)
    :hooks_(std::move(hooks)),registry_(std::move(registry)),dispatcher_(players,server,DispatchHooks(),registry_,std::move(registrations)) {
    if (!registry_ || !hooks_.now || !hooks_.record || !hooks_.terminate || !hooks_.close || !hooks_.release) throw std::invalid_argument("UDP 실제표/분배장치누락");
}
LocalUdpTransport::~LocalUdpTransport() { for (auto& node:nodes_) registry_->Forget(node->token);nodes_.clear();if (socketsStarted_) WSACleanup(); } //다른owner의표/소켓을정리하지않는다.
// UDP오류의실제통지는자기분배기로수행하고물리close/free만Registry에위임한다.
o::PacketDispatchHooks LocalUdpTransport::DispatchHooks() {
    o::PacketDispatchHooks result;result.now=[this] { return hooks_.now(); };result.record=[this](auto& connection,auto bytes,auto length) { hooks_.record(connection,bytes,length); };
    result.terminate=[this](auto& connection) { auto* node=Find(connection);if (!node) throw std::logic_error("UDP 다른노드종료");Fail(*node,0); };
    result.close=[this](auto& connection) { auto* node=Find(connection);if (!node) throw std::logic_error("UDP 다른노드close");if (!node->retired) registry_->Destroy(node->token); };
    result.release=[this](auto& connection) { auto* node=Find(connection);if (!node) throw std::logic_error("UDP 다른노드free");if (!node->retired) registry_->Release(node->token); };return result;
}
void LocalUdpTransport::StartSockets() { if (socketsStarted_) return;WSADATA data{};lastError_=WSAStartup(MAKEWORD(2,2),&data);if (!lastError_) socketsStarted_=true; } //현재장치의WSA참조만얻으며실패코드를남긴다.
LocalUdpTransport::Node* LocalUdpTransport::Find(std::uint32_t token) const { for (const auto& node:nodes_) if (node->token==token && !node->retired) return node.get();return nullptr; } //자기생존토큰만찾는다.
LocalUdpTransport::Node* LocalUdpTransport::Find(const o::PacketConnection& connection) const { for (const auto& node:nodes_) if (&node->connection==&connection) return node.get();return nullptr; } //반납대기중인같은물리도callback복귀까지식별한다.
LocalUdpTransport::Node* LocalUdpTransport::Current() const { return Find(registry_->Cleanup().transceivers[12]); } //Native005e4a48의현재12만조회한다.
// 원문wildcard UDP의broadcast/reuse옵션과blocking소켓을동일하게준비한뒤새현재12를공개한다.
bool LocalUdpTransport::Open(std::uint32_t port) {
    if (port>65535) throw std::invalid_argument("UDP 포트물리범위");const CallGuard call(activeCalls_);const auto failed=[this] { registry_->PublishConnect(12,0);return false; }; //원문새열기실패도현재12를0으로공개하며옛노드의물리를재사용하지않는다.
    StartSockets();if (!socketsStarted_) return failed();SocketOwner socket{::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)};
    if (socket.socket==INVALID_SOCKET) { lastError_=WSAGetLastError();return failed(); }const BOOL enabled=TRUE;sockaddr_in bindAddress{};bindAddress.sin_family=AF_INET;bindAddress.sin_port=htons(static_cast<u_short>(port));
    if (setsockopt(socket.socket,SOL_SOCKET,SO_BROADCAST,reinterpret_cast<const char*>(&enabled),sizeof(enabled))==SOCKET_ERROR || setsockopt(socket.socket,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&enabled),sizeof(enabled))==SOCKET_ERROR) { lastError_=WSAGetLastError();return failed(); }
    if (bind(socket.socket,reinterpret_cast<const sockaddr*>(&bindAddress),sizeof(bindAddress))==SOCKET_ERROR && (setsockopt(socket.socket,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&enabled),sizeof(enabled))==SOCKET_ERROR || bind(socket.socket,reinterpret_cast<const sockaddr*>(&bindAddress),sizeof(bindAddress))==SOCKET_ERROR)) { lastError_=WSAGetLastError();return failed(); } //첫bind실패뒤원문의reuse설정/재시도도보존한다.
    auto node=std::make_unique<Node>();node->socket=static_cast<std::uintptr_t>(socket.Release());node->connection.sender=12;node->connection.flags=4U|0x20U;node->connection.received=hooks_.now();node->connection.heartbeat=hooks_.now();auto* pointer=node.get();nodes_.push_back(std::move(node));
    try { pointer->token=registry_->Track(pointer->connection,[this,pointer] { const CallGuard callback(activeCalls_);CloseSocket(*pointer);hooks_.close(pointer->connection); },[this,pointer] { const CallGuard callback(activeCalls_);hooks_.release(pointer->connection);pointer->retired=true; },[this,pointer](auto packet) { static_cast<void>(SendNode(*pointer,packet,packet,std::nullopt)); },[] { return 0; });registry_->PublishConnect(12,pointer->token); } //실제Registry의close/free호출중에도물리노드를보존한다.UDP004e1800의현재값은0이다.
    catch (...) { registry_->Forget(pointer->token);nodes_.pop_back();throw; }return true;
}
void LocalUdpTransport::Close() { registry_->Disconnect(12); } //깊이1의같은수신노드는슬롯0뒤에도분배종료까지물리를보존한다.
void LocalUdpTransport::CloseSocket(Node& node) { if (node.socket==kInvalidSocket) return;if (closesocket(Socket(node.socket))==SOCKET_ERROR) { lastError_=WSAGetLastError();throw std::runtime_error("UDP close 실패:"+std::to_string(lastError_)); }node.socket=kInvalidSocket; } //현재실제핸들을한번닫는다.
void LocalUdpTransport::CloseReady(Node& node) { if (node.retired || !(node.connection.flags&kClosed) || node.connection.depth) return;if (registry_->Owns(node.token)) { registry_->Destroy(node.token);registry_->Release(node.token); }else { CloseSocket(node);node.retired=true; } } //닫힘/깊이계약뒤같은노드만반납한다.
void LocalUdpTransport::CollectRetired() { if (activeCalls_) throw std::logic_error("UDP callback중물리수거금지");for (auto it=nodes_.begin();it!=nodes_.end();) { if ((*it)->retired && !(*it)->connection.depth && !registry_->Owns((*it)->token)) it=nodes_.erase(it);else ++it; } } //외곽대여복귀뒤에만반납노드를지운다.
// 2번수신기가있을때만Native의같은수신물리앞4BYTE로종료를통지한다.
void LocalUdpTransport::Fail(Node& node,std::int32_t error) {
    if (error) lastError_=error;if (node.retired) return;if (dispatcher_.Dispatch().Registration(2).receive && !(node.connection.flags&kTerminated)) { node.connection.flags|=kTerminated;node.input[1]=2;node.input[2]=4;node.input[3]=0;static_cast<void>(dispatcher_.DispatchFrame(node.connection,std::span<const std::uint8_t>(node.input).first(4))); }CloseReady(node);
}
// observer가고친같은버퍼의길이를읽고별도TCP헤더없이원문바이트만한번보낸다.
bool LocalUdpTransport::Broadcast(std::span<std::uint8_t> packet,std::uint32_t port) {
    if (port>65535) throw std::invalid_argument("UDP 송신포트물리범위");auto* node=Current();return node && SendNode(*node,packet,packet,port);
}
// 같은원문가상this를유지하고observer1뒤현재길이의send또는sendto를선택한다.
bool LocalUdpTransport::SendNode(Node& nodeValue,std::span<const std::uint8_t> packet,std::span<std::uint8_t> writable,std::optional<std::uint32_t> broadcastPort) {
    if (packet.size()<4) throw std::out_of_range("UDP 송신물리헤더부족");if (Retired(packet)) return false;const CallGuard call(activeCalls_);auto* node=&nodeValue;if (node->retired || node->socket==kInvalidSocket || (node->connection.flags&kClosed)) return false;
    const auto observer=dispatcher_.Dispatch().Registration(packet[1]);const auto pin=observer.Pin();if (!writable.empty()) observer.NotifyObserver(writable,1);const auto length=Length(packet);if (length>packet.size()) throw std::out_of_range("UDP 송신선언길이물리초과");if (node->retired || node->socket==kInvalidSocket) return false;
    if (Retired(packet)) return false;const auto record=[this,node,packet] { if (hooks_.sent) hooks_.sent(node->connection,packet,static_cast<std::uint16_t>(Length(packet))); }; //4e00e0은 실제 쓰기/종료 통지가 끝난 뒤 실패 경로도 방향1을 기록한다. 현재 WORD 길이를 명시적으로 같은16비트 인자에 전달한다.
    sockaddr_in target{};target.sin_family=AF_INET;target.sin_port=htons(static_cast<u_short>(broadcastPort.value_or(0)));target.sin_addr.s_addr=INADDR_BROADCAST;u_long nonblocking=1;
    if (ioctlsocket(Socket(node->socket),FIONBIO,&nonblocking)==SOCKET_ERROR) { Fail(*node,WSAGetLastError());record();return false; }
    const auto sent=broadcastPort ? sendto(Socket(node->socket),reinterpret_cast<const char*>(packet.data()),static_cast<int>(length),0,reinterpret_cast<const sockaddr*>(&target),sizeof(target)) : send(Socket(node->socket),reinterpret_cast<const char*>(packet.data()),static_cast<int>(length),0);const auto error=sent==SOCKET_ERROR ? WSAGetLastError():0;nonblocking=0;
    if (ioctlsocket(Socket(node->socket),FIONBIO,&nonblocking)==SOCKET_ERROR) { Fail(*node,WSAGetLastError());record();return false; }if (sent<0 || static_cast<std::size_t>(sent)<length) { Fail(*node,error);record();return false; }record();return true;
}
// select0 뒤 한 데이터그램만 읽는다. 원문의 짧은 헤더는 sentinel이며 연결 종료가 아니다.
o::TransportPollResult LocalUdpTransport::ReceiveOne(Node& node) {
    CloseReady(node);if (node.retired || node.socket==kInvalidSocket || (node.connection.flags&kClosed)) return {};fd_set readable,errors;FD_ZERO(&readable);FD_ZERO(&errors);FD_SET(Socket(node.socket),&readable);FD_SET(Socket(node.socket),&errors);timeval wait{};
    const auto ready=select(0,&readable,nullptr,&errors,&wait);if (ready==SOCKET_ERROR || FD_ISSET(Socket(node.socket),&errors)) { Fail(node,WSAGetLastError());return {}; }if (!FD_ISSET(Socket(node.socket),&readable)) return {};
    sockaddr_in from{};int addressLength=sizeof(from);const auto count=recvfrom(Socket(node.socket),reinterpret_cast<char*>(node.input.data()),static_cast<int>(node.input.size()),0,reinterpret_cast<sockaddr*>(&from),&addressLength);node.connection.address=AddressBytes(from);
    if (count==SOCKET_ERROR) { Fail(node,WSAGetLastError());return {}; }if (count<4) return {count>0,false};auto packet=std::span<std::uint8_t>(node.input).first(static_cast<std::size_t>(count));const auto length=Length(packet);if (!length || length>=kCapacity || length!=static_cast<std::size_t>(count)) { Fail(node,0);return {true,false}; }
    if (packet[1]==0x7f || packet[1]==0x80) return {true,true};if (!dispatcher_.Dispatch().Validate(packet)) return {true,false};const auto observer=dispatcher_.Dispatch().Registration(packet[1]);const auto pin=observer.Pin();if (packet[1]!=4) observer.NotifyObserver(packet,0);if (node.retired || (node.connection.flags&kClosed)) { CloseReady(node);return {true,false}; }return {true,dispatcher_.DispatchFrame(node.connection,packet) && !node.retired};
}
// 자기 현재12만 폴링하며 callback이 슬롯을 바꾸면 다음 순회에서 현재값을 다시 읽는다.
void LocalUdpTransport::BeginPoll() {
    if (polling_ || activeCalls_) throw std::logic_error("UDP 중첩폴링금지");polling_=true;
    try { const CallGuard call(activeCalls_);for (auto& node:nodes_) CloseReady(*node); }catch (...) { polling_=false;throw; } //기존 현재 노드 정리부터 실행하고 실제 수신 전까지는 물리를 수거하지 않는다.
}
o::TransportPollResult LocalUdpTransport::ReceiveOne(std::int32_t id) { if (!polling_) throw std::logic_error("UDP per-ID 수신의 외곽 대여 누락");if (id!=12) return {};const CallGuard call(activeCalls_);auto* node=Current();return node ? ReceiveOne(*node):o::TransportPollResult{}; } //현재 자기12의 한 데이터그램/sentinel만 원문 본체로 처리한다.
void LocalUdpTransport::EndPoll() { if (!polling_) throw std::logic_error("UDP 외곽 반납의 대여 누락");polling_=false;CollectRetired(); } //전체 순회가 복귀한 뒤에만 반납 노드의 실제 물리를 수거한다.
std::size_t LocalUdpTransport::Poll() {
    BeginPoll();std::size_t count{};try { for (;;) { const auto result=ReceiveOne(12);if (result.consumed) ++count;if (!result.again) break; } }catch (...) { EndPoll();throw; }EndPoll();return count; //단독 메타도 같은 per-ID 본체를 반복하며 장치/주소를 새로 만들지 않는다.
}
void LocalUdpTransport::RebindHooks(o::PacketDispatchHooks hooks) { if (!hooks.now || !hooks.record || !hooks.terminate || !hooks.close || !hooks.release) throw std::invalid_argument("UDP 인계의 실제 분배 장치 누락");hooks_=std::move(hooks); } //기존 wrapper는 현재 장치만 다시 읽고 node/socket/depth는 보존한다.
o::PacketConnection* LocalUdpTransport::Connection(std::int32_t id) const { const auto* node=id==12 ? Current():nullptr;return node ? const_cast<o::PacketConnection*>(&node->connection):nullptr; } //같은 안정된 현재 노드의 짧은 대여이며 다른 ID를 UDP로 해석하지 않는다.
bool LocalUdpTransport::IsOpen(std::int32_t id) const { const auto* node=id==12 ? Current():nullptr;return node && node->socket!=kInvalidSocket && !(node->connection.flags&kClosed); } //상위 closed와 실제 socket 존재를 구별한다.
std::int32_t LocalUdpTransport::Status(std::int32_t id) const { const auto* node=id==12 ? Current():nullptr;return node ? registry_->SocketReadiness(node->token):0; } //UDP 원문 준비 조회0도 같은 등록된 실제 가상 장치를 그대로 사용한다.
const std::shared_ptr<o::RawConnectionRegistry>& LocalUdpTransport::Registry() const { return registry_; } //동일한 현재22칸 표만 대여한다.
const std::shared_ptr<o::PacketRegistrationTable>& LocalUdpTransport::Registrations() { return dispatcher_.Dispatch().Registrations(); } //같은 observer/receiver 표의 실제 정체성을 조회한다.
bool LocalUdpTransport::Send(std::span<const std::uint8_t> packet) { auto* node=Current();return node && SendNode(*node,packet,{},std::nullopt); } //이미 작성된 readonly writer의 현재 선언 바이트만 전송한다.
bool LocalUdpTransport::SendMutable(std::span<std::uint8_t> packet) { auto* node=Current();return node && SendNode(*node,packet,packet,std::nullopt); } //실제 mutable writer는 같은 this로 observer1을 실행한다.
// 원문처럼 자기 호스트 이름만 해석하고 첫 AF_INET 주소를 사용한다. 실패 시 family0을 보존한다.
std::array<std::uint8_t,20> LocalUdpTransport::LocalAddress() {
    StartSockets();std::array<std::uint8_t,20> result{};if (!socketsStarted_) return result;std::array<char,80> name{};if (gethostname(name.data(),79)==SOCKET_ERROR) { lastError_=WSAGetLastError();return result; }const auto* host=gethostbyname(name.data());
    if (!host || host->h_addrtype!=AF_INET || host->h_length!=4 || !host->h_addr_list || !host->h_addr_list[0]) { lastError_=WSAGetLastError();return result; }std::memcpy(result.data()+4,host->h_addr_list[0],4);if (result[4] || result[5] || result[6] || result[7]) result[0]=AF_INET;return result;
}
std::int32_t LocalUdpTransport::LastError() const { return lastError_; } //OS실패를진단으로만노출한다.
}
