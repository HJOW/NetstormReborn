// 원문 10.78 TCP의 4BYTE 프레임·pending 등록·부분 송수신을 Windows LAN 소유자로 옮긴다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Windows 헤더의 불필요한 소켓 구형 선언을 제외한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 min/max와 Windows 매크로가 충돌하지 않게 한다.
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "client/LocalTcpTransport.h"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 원문 Ztrans/Zocket의 OS 소켓과 패킷 분배를 명시적으로 나눈다.
namespace { // TCP 주소·원문 비정렬 헤더와 오류 변환만 제공한다.
constexpr std::size_t kHeader=4,kWireLimit=65535,kPlayerStride=180,kPlayerCount=9; // 원문 헤더/WORD 길이/Player9의 실제 폭이다.
constexpr std::uint32_t kClosed=0x40,kBuffered=2,kConnecting=0x20,kTerminated=0x10; // PacketConnection 상위 필드이며 하위 socket 상태와 구별한다.
constexpr std::uint32_t kTcp=0x10; // 원문20BYTE 주소 +16의 TCP 프로토콜 비트다.
constexpr std::size_t kWriteBacklog=100000; //004df2c0의 기본 송신 backlog 상한이며 would-block 때 무한 누적하지 않는다.

// 부호 확장이나 호스트 포인터 절삭 없이 SOCKET 정수 핸들을 복원한다.
SOCKET Socket(std::uintptr_t value) { return static_cast<SOCKET>(value); }
// 원문 비정렬 little endian 필드의 필요한 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("TCP 원문 필드 읽기 범위");std::uint32_t value{};
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[at+i])<<(i*8); // 지정 필드의 낮은 BYTE부터 모은다.
    return value;
}
// 원문 폭만 쓰며 인접한 version/type/주소를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("TCP 원문 필드 쓰기 범위");
    for (std::size_t i=0;i<width;++i) bytes[at+i]=static_cast<std::uint8_t>(value>>(i*8)); // 낮은 지정 BYTE부터 쓴다.
}
// 폐기된 CD/클라이언트 유효성 요청·응답은 observer/실제 wire 이전에 제외한다.
bool Retired(std::uint8_t type) { return type==0x7f || type==0x80; }
// 직접 집합을 넘긴 caller도 폐기된 내부 요청을 송신하지 못하게 같은 헤더 범위를 확인한다.
bool RetiredFrame(std::span<const std::uint8_t> packet) {
    if (Retired(packet[1])) return true;if (packet[1]!=4) return false;const auto total=Read(packet,2,2);if (total<kHeader || total>packet.size()) throw std::out_of_range("TCP 집합 송신 선언 길이 범위");std::size_t at=kHeader;
    while (at<total) { // 현재 aggregate의 내부 원문 패킷 경계만 읽는다.
        if (total-at<kHeader) throw std::out_of_range("TCP 집합 송신 내부 헤더 부족");const auto length=Read(packet,at+2,2);if (length<kHeader || length>total-at) throw std::out_of_range("TCP 집합 송신 내부 길이 오류");if (Retired(packet[at+1])) return true;if (packet[at+1]==4) throw std::logic_error("TCP 중첩 집합 송신 미복원");at+=length;
    }
    return false;
}
// DNS/레거시 프로토콜 파서를 호출하지 않고 정확한 IPv4 literal 네 부분만 읽는다.
std::array<std::uint8_t,4> ParseAddress(std::string_view text) {
    std::array<std::uint8_t,4> result{};std::size_t at{};
    for (std::size_t i=0;i<result.size();++i) { // 네 개의 0..255 십진 필드만 허용한다.
        const auto end=i+1==result.size() ? text.size():text.find('.',at);if (end==std::string_view::npos || end==at) throw std::invalid_argument("LAN TCP IPv4 literal이 필요합니다");unsigned value{};const auto parsed=std::from_chars(text.data()+at,text.data()+end,value);
        if (parsed.ec!=std::errc{} || parsed.ptr!=text.data()+end || value>255) throw std::invalid_argument("LAN TCP IPv4 필드 범위 오류");result[i]=static_cast<std::uint8_t>(value);at=end+1;
    }
    return result;
}
// 로컬 서버 범위는 loopback·RFC1918·IPv4 link-local이며 외부 공식 서버 접속은 허용하지 않는다.
bool LocalAddress(const std::array<std::uint8_t,4>& address) {
    return address[0]==127 || address[0]==10 || (address[0]==172 && address[1]>=16 && address[1]<=31) || (address[0]==192 && address[1]==168) || (address[0]==169 && address[1]==254);
}
// sockaddr의 network endian IP/port를 원문20BYTE에 그대로 기록한다.
std::array<std::uint8_t,20> AddressBytes(const sockaddr_in& address) {
    std::array<std::uint8_t,20> result{};Put(result,0,AF_INET,2);std::memcpy(result.data()+2,&address.sin_port,2);std::memcpy(result.data()+4,&address.sin_addr,4);Put(result,16,kTcp);return result;
}
// 명시 IPv4 바이트를 Windows sockaddr_in의 실제 네트워크 바이트 순서로 옮긴다.
sockaddr_in SocketAddress(const std::array<std::uint8_t,4>& address,std::uint32_t port) {
    if (!port || port>65535) throw std::invalid_argument("LAN TCP 포트 범위 오류");sockaddr_in result{};result.sin_family=AF_INET;result.sin_port=htons(static_cast<u_short>(port));std::memcpy(&result.sin_addr,address.data(),address.size());return result;
}
// 오류를 정상 접속/빈 성공으로 바꾸지 않고 작업명과 원래 Winsock 코드를 함께 전달한다.
std::runtime_error SocketError(std::string_view operation,std::int32_t code) { return std::runtime_error(std::string(operation)+" 실패 (Winsock "+std::to_string(code)+")"); }
// 준비 도중 예외가 발생한 미등록 socket만 반납한다.
struct SocketOwner {
    SOCKET socket{INVALID_SOCKET}; // Adopt 전까지 이 작은 owner가 핸들을 갖는다.
    ~SocketOwner() { if (socket!=INVALID_SOCKET) closesocket(socket); } // 예외 복귀의 미등록 핸들을 누수시키지 않는다.
    SOCKET Release() { const auto value=socket;socket=INVALID_SOCKET;return value; } // 같은 핸들의 소유권을 Node에 한 번 넘긴다.
};
// 동기 callback 동안 재진입 Poll이 노드 저장소를 수거하지 못하게 한다.
struct CallGuard {
    std::size_t& count; // 전송 owner보다 짧은 현재 호출의 대여 수다.
    explicit CallGuard(std::size_t& value):count(value) { ++count; } // 선택된 실제 호출의 대여를 시작한다.
    ~CallGuard() { --count; } // 정상/예외 복귀 모두 현재 대여를 반납한다.
};
}

// 현재 registry callback 동안에도 주소가 바뀌지 않는 실제 TCP 연결 하나다.
struct LocalTcpTransport::Node {
    o::PacketConnection connection; // 같은 sender/depth/flags/주소가 모든 패킷 소비자에 전달된다.
    std::uintptr_t socket{kInvalidSocket}; // 이 노드가 소유하는 OS socket이다.
    std::uint32_t token{}; // 장기 Registry에 등록한 재사용하지 않는 토큰이다.
    std::vector<std::uint8_t> input,wire,aggregate; // 부분 수신·확정 송신·아직 헤더를 확정하지 않은 payload다.
    std::size_t sent{}; // wire에서 이미 전달한 접두 길이이며 would-block이면 유지한다.
    std::int32_t error{}; // 이 노드의 실제 마지막 OS 실패다.
    std::uint8_t version{0xff}; // type1/2가 보존하는 현재 원문 수신 버퍼의 첫 BYTE다.
    bool connecting{},connected{},notified{},retired{}; // OS 접속 진행·완료·최초 통지·실제 registry 반납을 구별한다.
    ~Node() { if (socket!=kInvalidSocket) closesocket(Socket(socket)); } // 최종 owner 정리/등록 실패의 남은 핸들만 반납한다.
};

// 생성 자체는 네트워크를 시작하지 않으며 기존 집합 분배기를 Open 없이 사용한다.
LocalTcpTransport::LocalTcpTransport(std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry,std::shared_ptr<o::PacketRegistrationTable> registrations)
    :players_(players),server_(server),hooks_(std::move(hooks)),registry_(registry ? std::move(registry):std::make_shared<o::RawConnectionRegistry>()),dispatcher_(players,server,DispatchHooks(),registry_,std::move(registrations)) {
    if (players.size()!=kPlayerStride*kPlayerCount || !hooks_.now || !hooks_.record || !hooks_.terminate || !hooks_.close || !hooks_.release) throw std::invalid_argument("LAN TCP의 실제 Player9/현재 분배 장치 누락");
}
// 최종 owner 소멸은 사용자 callback 예외에 의존하지 않고 자기 별칭/핸들을 반납한다.
LocalTcpTransport::~LocalTcpTransport() {
    for (auto& node:nodes_) registry_->Forget(node->token); // 다른 전송 토큰을 지우거나 해제된 노드를 다시 읽지 않는다.
    nodes_.clear();if (listener_!=kInvalidSocket) closesocket(Socket(listener_));if (socketsStarted_) WSACleanup();
}
// 원문 single/aggregate 분배의 선택된 close/free를 같은 Registry 노드에 전달한다.
o::PacketDispatchHooks LocalTcpTransport::DispatchHooks() {
    o::PacketDispatchHooks result;result.now=[this] { return hooks_.now(); };result.record=[this](auto& node,auto bytes,auto length) { hooks_.record(node,bytes,length); };
    result.terminate=[this](auto& connection) { auto* node=Find(connection);if (!node) throw std::logic_error("TCP 종료의 다른 연결 owner");Fail(*node,0); };
    result.close=[this](auto& connection) {
        auto* node=Find(connection);if (!node) throw std::logic_error("TCP close의 다른 연결 owner"); // 물리 보관 중인 retired 노드도 같은 주소로 식별한다.
        if (node->retired) { if (registry_->Owns(node->token)) throw std::logic_error("반납한 TCP close 토큰의 잘못된 재등록");return; } // 이미 실제 반납한 동일 할당만 중복 생략한다.
        if (!registry_->Owns(node->token) || &registry_->Node(node->token)!=&connection) throw std::logic_error("TCP close의 다른 세대/주소");registry_->Destroy(node->token);
    };
    result.release=[this](auto& connection) {
        auto* node=Find(connection);if (!node) throw std::logic_error("TCP free의 다른 연결 owner"); // 실제 노드 수거 전까지 retired 주소는 유지한다.
        if (node->retired) { if (registry_->Owns(node->token)) throw std::logic_error("반납한 TCP free 토큰의 잘못된 재등록");return; } // 이전 실제 Release의 완료를 확인한 경우만 생략한다.
        if (!registry_->Owns(node->token) || &registry_->Node(node->token)!=&connection) throw std::logic_error("TCP free의 다른 세대/주소");registry_->Release(node->token);
    };return result;
}
// 같은 owner의 첫 명시 LAN 동작만 Winsock 참조를 얻는다.
void LocalTcpTransport::StartSockets() {
    if (socketsStarted_) return;WSADATA data{};const auto error=WSAStartup(MAKEWORD(2,2),&data);if (error) { lastError_=error;throw SocketError("WSAStartup",error); }socketsStarted_=true;
}
// 토큰은 host 포인터를32비트로 자르지 않고 안정된 자기 노드에서 찾는다.
LocalTcpTransport::Node* LocalTcpTransport::Find(std::uint32_t token) const {
    for (const auto& node:nodes_) if (node->token==token && !node->retired) return node.get(); // 현재 자기 노드만 순서와 무관하게 찾는다.
    return nullptr;
}
// 분배 callback은 자기 노드의 동일 PacketConnection 주소만 받을 수 있다.
LocalTcpTransport::Node* LocalTcpTransport::Find(const o::PacketConnection& connection) const {
    for (const auto& node:nodes_) if (&node->connection==&connection) return node.get(); // 같은 물리 할당은 외곽 수거 전까지 retired도 식별한다. 현재 ID 조회는 토큰용 Find가 계속 제외한다.
    return nullptr;
}
// 원문의 정상22칸/음수 pending1..31의 현재 토큰을 읽는다.
std::uint32_t LocalTcpTransport::Token(std::int32_t id) const {
    const auto state=registry_->Cleanup();if (id>=0 && id<static_cast<std::int32_t>(state.transceivers.size())) return state.transceivers[static_cast<std::size_t>(id)];
    if (id<0 && id>-static_cast<std::int32_t>(state.pending.size())) return state.pending[static_cast<std::size_t>(-id)];throw std::out_of_range("TCP 현재 연결 번호 범위");
}
// 현재 칸이 다른 내부 전송의 토큰이면 이 OS owner로 간주하지 않는다.
LocalTcpTransport::Node* LocalTcpTransport::Current(std::int32_t id) const { return Find(Token(id)); }
// caller가 넘긴 socket의 소유권을 실패 시에도 책임지고 실제 노드/장치와 함께 등록한다.
LocalTcpTransport::Node& LocalTcpTransport::Adopt(std::uintptr_t socket,std::int32_t id,bool connecting,bool buffered,const std::array<std::uint8_t,20>& address) {
    const CallGuard call(activeCalls_);
    SocketOwner guard{Socket(socket)};auto node=std::make_unique<Node>();node->socket=static_cast<std::uintptr_t>(guard.Release());node->connection.sender=std::bit_cast<std::uint32_t>(id);node->connection.address=address;node->connection.flags=4U|kConnecting|(buffered ? kBuffered:0U);node->connection.received=hooks_.now();node->connection.heartbeat=hooks_.now();node->connecting=connecting;node->connected=!connecting;auto* pointer=node.get();nodes_.push_back(std::move(node));
    try {
        pointer->token=registry_->Track(pointer->connection,[this,pointer] { const CallGuard callback(activeCalls_);CloseSocket(*pointer);hooks_.close(pointer->connection); },[this,pointer] { const CallGuard callback(activeCalls_);hooks_.release(pointer->connection);pointer->retired=true; },[this,pointer](auto packet) { static_cast<void>(SendMutable(std::bit_cast<std::int32_t>(pointer->connection.sender),packet)); });
        if (id<0) pointer->connection.sender=std::bit_cast<std::uint32_t>(registry_->Accept(pointer->token));else registry_->Register(id,pointer->token);
    } catch (...) { registry_->Forget(pointer->token);nodes_.pop_back();throw; }
    return *pointer;
}
// TCP host 선택 시에만 listener를 열며 기존 단독 내부 서버는 소켓 없이 유지된다.
void LocalTcpTransport::Listen(std::uint32_t port,std::string_view bindAddress) {
    if (listener_!=kInvalidSocket) throw std::logic_error("LAN TCP listener 중복 생성");const auto parsed=ParseAddress(bindAddress);if (parsed!=std::array<std::uint8_t,4>{} && !LocalAddress(parsed)) throw std::invalid_argument("LAN TCP 바인드 주소가 로컬 범위 밖입니다");const auto address=SocketAddress(parsed,port);StartSockets();SocketOwner socket{::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)};
    if (socket.socket==INVALID_SOCKET) { lastError_=WSAGetLastError();throw SocketError("socket",lastError_); }u_long nonblocking=1;
    if (ioctlsocket(socket.socket,FIONBIO,&nonblocking)==SOCKET_ERROR || bind(socket.socket,reinterpret_cast<const sockaddr*>(&address),sizeof(address))==SOCKET_ERROR || listen(socket.socket,31)==SOCKET_ERROR) { lastError_=WSAGetLastError();throw SocketError("LAN listen",lastError_); }
    listener_=static_cast<std::uintptr_t>(socket.Release());port_=port;
}
// 상위 방 수명은 accepted/기존 peer를 남긴 채 신규 수락만 끌 수 있다.
void LocalTcpTransport::StopListening() {
    if (listener_==kInvalidSocket) return;if (closesocket(Socket(listener_))==SOCKET_ERROR) { lastError_=WSAGetLastError();throw SocketError("listener close",lastError_); }listener_=kInvalidSocket;port_=0;
}
// 명시 LAN IPv4만 사용하며 원문처럼 현재 ID9의 실제 socket을 등록한다.
void LocalTcpTransport::Connect(std::string_view address,std::uint32_t port,std::int32_t id) {
    const auto parsed=ParseAddress(address);if (!LocalAddress(parsed)) throw std::invalid_argument("LAN TCP는 loopback/사설/link-local IPv4만 접속합니다");if (id<0 || id>=static_cast<std::int32_t>(o::kTransceiverSlots) || Token(id)) throw std::logic_error("LAN TCP 접속 ID의 빈 현재 칸 필요");const auto endpoint=SocketAddress(parsed,port);StartSockets();SocketOwner socket{::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)};
    if (socket.socket==INVALID_SOCKET) { lastError_=WSAGetLastError();throw SocketError("socket",lastError_); }u_long nonblocking=1;if (ioctlsocket(socket.socket,FIONBIO,&nonblocking)==SOCKET_ERROR) { lastError_=WSAGetLastError();throw SocketError("TCP nonblocking",lastError_); }
    const auto result=connect(socket.socket,reinterpret_cast<const sockaddr*>(&endpoint),sizeof(endpoint));const auto error=result==SOCKET_ERROR ? WSAGetLastError():0;
    if (error && error!=WSAEWOULDBLOCK && error!=WSAEINPROGRESS && error!=WSAEALREADY) { lastError_=error;throw SocketError("LAN connect",error); }static_cast<void>(Adopt(static_cast<std::uintptr_t>(socket.Release()),id,error!=0,true,AddressBytes(endpoint)));
}
// 원문 주소의 network endian 포트/IP를 host 문자열로 안전하게 변환하며 DNS를 조회하지 않는다.
void LocalTcpTransport::Connect(std::span<const std::uint8_t> address,std::uint32_t port,std::int32_t id) {
    if (address.size()!=20 || Read(address,0,2)!=AF_INET || !(Read(address,16)&kTcp) || (Read(address,16)&0x628U)) throw std::invalid_argument("LAN TCP의 AF_INET/TCP20BYTE 주소 필요");if (!port) port=(std::uint32_t(address[2])<<8)|address[3];
    const auto text=std::to_string(address[4])+"."+std::to_string(address[5])+"."+std::to_string(address[6])+"."+std::to_string(address[7]);Connect(text,port,id);
}
// 원문 같은 노드의 sender/등록 위치와 buffered 상태만 바꾼다.
void LocalTcpTransport::AssignConnection(std::int32_t pending,std::int32_t owner) {
    auto* node=Current(pending);if (!node || pending>=0) throw std::logic_error("LAN TCP 재배치의 현재 accepted 노드 누락");registry_->Assign(pending,owner,server_ ? 1U:0U,[this,node](auto token) { if (token!=node->token || !Find(token)) throw std::logic_error("LAN TCP buffered 재배치 토큰 불일치");node->connection.flags|=kBuffered;node->aggregate.clear(); });
}
// listener의 accept 성공 때만 고정 노드와 음수 pending을 만든다.
void LocalTcpTransport::Accept() {
    if (listener_==kInvalidSocket) return;
    while (registry_->CanAccept()) { // 원문31개 대기 연결 상한을 현재 표에서 매번 읽는다.
        sockaddr_in address{};int size=sizeof(address);SocketOwner socket{accept(Socket(listener_),reinterpret_cast<sockaddr*>(&address),&size)};
        if (socket.socket==INVALID_SOCKET) { const auto error=WSAGetLastError();if (error==WSAEWOULDBLOCK) return;lastError_=error;throw SocketError("LAN accept",error); }
        std::array<std::uint8_t,4> bytes{};std::memcpy(bytes.data(),&address.sin_addr,bytes.size());if (address.sin_family!=AF_INET || !LocalAddress(bytes)) { lastError_=WSAEACCES;continue; }u_long nonblocking=1;
        if (ioctlsocket(socket.socket,FIONBIO,&nonblocking)==SOCKET_ERROR) { lastError_=WSAGetLastError();throw SocketError("accepted nonblocking",lastError_); }static_cast<void>(Adopt(static_cast<std::uintptr_t>(socket.Release()),-1,false,false,AddressBytes(address)));
    }
}
// 실제 socket readiness와 SO_ERROR를 확인한 뒤 원문 connected 통지를 한 번 보낸다.
bool LocalTcpTransport::CompleteConnection(Node& node) {
    if (node.retired || (node.connection.flags&kClosed) || node.socket==kInvalidSocket) return false;
    if (node.connecting) {
        fd_set writable,errors;FD_ZERO(&writable);FD_ZERO(&errors);FD_SET(Socket(node.socket),&writable);FD_SET(Socket(node.socket),&errors);timeval timeout{};const auto ready=select(0,nullptr,&writable,&errors,&timeout);
        if (ready==SOCKET_ERROR) { Fail(node,WSAGetLastError());return false; }if (!ready) return false;int error{},length=sizeof(error);
        if (getsockopt(Socket(node.socket),SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&length)==SOCKET_ERROR) error=WSAGetLastError();if (error) { Fail(node,error);return false; }node.connecting=false;node.connected=true;
    }
    if (!node.notified) { node.notified=true;node.connection.flags&=~kConnecting;Notify(node,1); }
    return !node.retired && !(node.connection.flags&kClosed);
}
// 원문 통지는 기존 입력 버퍼의 version BYTE를 유지하고 직접 분배한다.
void LocalTcpTransport::Notify(Node& node,std::uint8_t type) {
    std::array<std::uint8_t,4> bytes{node.version,type,4,0};static_cast<void>(dispatcher_.DispatchFrame(node.connection,bytes));
}
// 한 번만 type2를 알리고 현재 depth0에서 같은 실제 close/free를 수행한다.
void LocalTcpTransport::Fail(Node& node,std::int32_t error) {
    if (node.retired) return;node.error=error;if (error) lastError_=error;
    if (!(node.connection.flags&kTerminated)) { node.connection.flags|=kTerminated;Notify(node,2); }
    if (node.retired) return;hooks_.terminate(node.connection);node.connection.flags|=kClosed;CloseReady(node);
}
// 실제 OS close 실패는 정상 반납으로 표시하지 않고 후속 정리에서 같은 핸들을 다시 선택할 수 있게 한다.
void LocalTcpTransport::CloseSocket(Node& node) {
    if (node.socket==kInvalidSocket) return;if (closesocket(Socket(node.socket))==SOCKET_ERROR) { node.error=lastError_=WSAGetLastError();throw SocketError("TCP close",node.error); }node.socket=kInvalidSocket;node.connected=false;node.connecting=false;
}
// 수신 callback이 끝난 뒤에만 같은 Registry가 실제 노드의 반납을 확정한다.
void LocalTcpTransport::CloseReady(Node& node) {
    if (node.retired || !(node.connection.flags&kClosed) || node.connection.depth) return;if (!registry_->Owns(node.token)) { CloseSocket(node);node.retired=true;return; }registry_->Destroy(node.token);registry_->Release(node.token);
}
// 외부 State cleanup과 같은 closed/depth 지연 경계를 사용한다.
void LocalTcpTransport::Close(std::int32_t id) { if (auto* node=Current(id)) { node->connection.flags|=kClosed;CloseReady(*node); } }
// 공유 표 전체를0으로 덮지 않고 자기 owner의 현재 노드만 반납한다.
void LocalTcpTransport::CloseAll() {
    for (auto& node:nodes_) { if (!node->retired) { node->connection.flags|=kClosed;CloseReady(*node); } } // 다른 전송/이미 반납된 토큰은 선택하지 않는다.
    StopListening(); // 물리 노드 수거는 외곽 Poll 뒤에만 수행하여 송신/close callback의 현재 참조를 보존한다.
}
// 실제 등록 반납이 끝나고 callback 대여가 끝난 노드만 지운다.
void LocalTcpTransport::CollectRetired() {
    if (activeCalls_) throw std::logic_error("LAN TCP 진행 중 callback의 노드 조기 수거");
    for (auto it=nodes_.begin();it!=nodes_.end();) { // 물리 erase는 Poll 외곽에서만 수행한다.
        if ((*it)->retired && !(*it)->connection.depth && !registry_->Owns((*it)->token)) it=nodes_.erase(it);else ++it;
    }
}
// 원문4BYTE header와 선언 길이를 TCP stream에서 정확히 한 프레임만 모은다.
bool LocalTcpTransport::ReceiveOne(Node& node,bool* consumed) {
    if (consumed) *consumed=false;if (!CompleteConnection(node)) return false;
    for (;;) { // header/body가 나뉘어 오면 현재 부분 버퍼를 다음 Poll에 그대로 남긴다.
        std::size_t required=kHeader;if (node.input.size()>=kHeader) { required=Read(node.input,2,2);if (required<kHeader || required>kWireLimit) { Fail(node,WSAEMSGSIZE);return false; } }
        if (node.input.size()<required) {
            const auto before=node.input.size();node.input.resize(required);const auto count=recv(Socket(node.socket),reinterpret_cast<char*>(node.input.data()+before),static_cast<int>(required-before),0);
            if (count==SOCKET_ERROR) { const auto error=WSAGetLastError();node.input.resize(before);if (error==WSAEWOULDBLOCK) return false;Fail(node,error);return false; }
            if (!count) { node.input.resize(before);Fail(node,0);return false; }node.input.resize(before+static_cast<std::size_t>(count));if (node.input.size()<required) return false;if (required==kHeader) continue;
        }
        auto frame=std::move(node.input);node.input.clear();if (consumed) *consumed=true;node.version=frame[0];if (Retired(frame[1])) return true;if (!dispatcher_.Dispatch().Validate(frame)) return false;
        const auto observer=dispatcher_.Dispatch().Registration(frame[1]);const auto sessionPin=observer.Pin(); // observer부터 뒤 분배까지 실제 Runtime 대여를 유지한다.
        if (frame[1]!=4) observer.NotifyObserver(frame,0); //004dfc00의 단일 observer0이며 집합 내부 observer는 기존 분배기가 한 번 실행한다.
        if (node.retired || (node.connection.flags&kClosed)) { CloseReady(node);return false; }return dispatcher_.DispatchFrame(node.connection,frame) && !node.retired;
    }
}
// 이미 작성된 패킷과 mutable observer 경계를 같은 현재 gate에 전달한다.
bool LocalTcpTransport::Send(std::int32_t destination,std::span<const std::uint8_t> packet) { return SendPacket(destination,packet,{}); }
// 원문 thiscall observer가 같은 mutable 버퍼의 길이를 확정하게 한다.
bool LocalTcpTransport::SendMutable(std::int32_t destination,std::span<std::uint8_t> packet) { return SendPacket(destination,packet,packet); }
//004e2dc0의 Player/연결 gate를 실행한 뒤에만 observer와 실제 송신을 선택한다.
bool LocalTcpTransport::SendPacket(std::int32_t destination,std::span<const std::uint8_t> packet,std::span<std::uint8_t> writable) {
    const CallGuard call(activeCalls_);if (packet.size()>=kHeader && Retired(packet[1])) return false;
    if (destination>=1 && destination<static_cast<std::int32_t>(kPlayerCount) && !(players_[static_cast<std::size_t>(destination)*kPlayerStride+0x50]&1U)) return false;auto* node=Current(destination);if (!node || (node->connection.flags&kClosed) || node->socket==kInvalidSocket) return false;
    if (packet.size()<kHeader) throw std::out_of_range("LAN TCP 송신 헤더 부족");if (RetiredFrame(packet)) return false;
    const auto observer=dispatcher_.Dispatch().Registration(packet[1]);const auto sessionPin=observer.Pin(); // callback이 caller의 패킷 owner를 반납해도 실제 송신 복귀까지 유지한다.
    if (!writable.empty()) observer.NotifyObserver(writable,1);if (Current(destination)!=node || node->retired || (node->connection.flags&kClosed)) return false;const auto length=Read(packet,2,2);if (length<kHeader || length>packet.size()) throw std::out_of_range("LAN TCP 송신 선언 길이 범위");if (RetiredFrame(packet)) return false;if (packet[1]==4 && (node->connection.flags&kBuffered)) throw std::logic_error("LAN TCP buffered 안의 직접 집합 송신 미복원");SendNode(*node,packet.first(length));return !node->retired && !(node->connection.flags&kClosed);
}
// buffered payload는 집합 확정까지 보관하며 unbuffered도 부분 send의 나머지를 잃지 않는다.
void LocalTcpTransport::SendNode(Node& node,std::span<const std::uint8_t> packet) {
    if (node.connection.flags&kBuffered) {
        if (node.aggregate.size()+packet.size()+kHeader>kWireLimit) { Finalize(node);Drain(node);if (node.retired || (node.connection.flags&kClosed)) return; }
        if (packet.size()+kHeader>kWireLimit) { Fail(node,WSAEMSGSIZE);return; }node.aggregate.insert(node.aggregate.end(),packet.begin(),packet.end());
    } else { Compact(node);if (node.wire.size()+packet.size()>kWriteBacklog) { Fail(node,WSAEMSGSIZE);return; }node.wire.insert(node.wire.end(),packet.begin(),packet.end());Drain(node); }
}
// 원문0xff version/종류4/전체WORD를 payload 앞에 붙여 wire 순서에 보관한다.
void LocalTcpTransport::Finalize(Node& node) {
    if (node.aggregate.empty()) return;Compact(node);if (node.aggregate.size()+kHeader>kWireLimit || node.wire.size()+node.aggregate.size()+kHeader>kWriteBacklog) { Fail(node,WSAEMSGSIZE);return; }const auto start=node.wire.size();node.wire.resize(start+kHeader,0xff);node.wire[start+1]=4;Put(node.wire,start+2,static_cast<std::uint32_t>(node.aggregate.size()+kHeader),2);node.wire.insert(node.wire.end(),node.aggregate.begin(),node.aggregate.end());node.aggregate.clear();
}
// 부분 send가 이미 소비한 접두만 지워 원문 미전달 데이터의 순서를 보존한다.
void LocalTcpTransport::Compact(Node& node) { if (node.sent) { node.wire.erase(node.wire.begin(),node.wire.begin()+static_cast<std::ptrdiff_t>(node.sent));node.sent=0; } }
// 실제 양수 send만큼만 접두를 소비하고 would-block에서는 현재 바이트/순서를 유지한다.
void LocalTcpTransport::Drain(Node& node) {
    if (node.retired || !node.connected || (node.connection.flags&kClosed)) return;
    while (node.sent<node.wire.size()) { // 한 연결의 현재 확정 프레임 순서를 OS가 받는 만큼만 전진한다.
        const auto amount=std::min(node.wire.size()-node.sent,static_cast<std::size_t>(std::numeric_limits<int>::max()));const auto count=send(Socket(node.socket),reinterpret_cast<const char*>(node.wire.data()+node.sent),static_cast<int>(amount),0);
        if (count==SOCKET_ERROR) { const auto error=WSAGetLastError();if (error==WSAEWOULDBLOCK) return;Fail(node,error);return; }if (!count) { Fail(node,WSAECONNRESET);return; }node.sent+=static_cast<std::size_t>(count);
    }
    node.wire.clear();node.sent=0;
}
// 원문 정상22칸의 현재 순서이며 아직 음수인 pending은 buffered flush 대상이 아니다.
void LocalTcpTransport::FlushOne(std::int32_t id) {
    if (id<0 || id>=static_cast<std::int32_t>(o::kTransceiverSlots)) throw std::out_of_range("TCP flush 정상 ID 범위");const CallGuard call(activeCalls_);if (auto* node=Current(id)) { Finalize(*node);Drain(*node); }
}
// 단독 TCP 사용도 같은 현재0..21 순회이며 callback 뒤 현재 ID를 다시 읽는다.
void LocalTcpTransport::Flush() {
    const CallGuard call(activeCalls_);
    for (std::int32_t id=0;id<static_cast<std::int32_t>(o::kTransceiverSlots);++id) FlushOne(id); // 다른 backend 노드를 선택하지 않는다.
}
// 전역 coordinator와 단독 Poll이 같은 실제 진입/수거 guard를 쓴다.
void LocalTcpTransport::BeginPoll() {
    if (polling_ || activeCalls_) throw std::logic_error("LAN TCP Poll 중첩 호출");polling_=true;
    try { for (auto& node:nodes_) CloseReady(*node); }catch (...) { polling_=false;throw; } // 이전 callback이 남긴 자기 closed/depth0 노드만 반납한다.
}
// socket accept 성공만 현재 전역 Registry에 pending으로 등록한다.
void LocalTcpTransport::AcceptPending() { if (!polling_) throw std::logic_error("TCP accept의 Poll 대여 누락");Accept(); }
// 한 현재 자기 ID의 partial/완성 수신과 미전달 송신만 진행한다.
o::TransportPollResult LocalTcpTransport::ReceiveOne(std::int32_t id) {
    if (!polling_) throw std::logic_error("TCP per-ID 수신의 Poll 대여 누락");const CallGuard call(activeCalls_);auto* node=Current(id);if (!node) return {};bool consumed=false;const bool again=ReceiveOne(*node,&consumed);Drain(*node);return {consumed,again};
}
// callback/Send 대여가 모두 끝난 최외곽에서만 실제 Node를 물리 erase한다.
void LocalTcpTransport::EndPoll() { if (activeCalls_) throw std::logic_error("TCP callback 복귀 전 Poll 대여 반납 금지");polling_=false;CollectRetired(); }
// 기존 단독 호출도 per-ID 경계만 조립하여 원문 listener/pending/normal 순서를 유지한다.
std::size_t LocalTcpTransport::Poll() {
    BeginPoll();std::size_t count{};
    try {
        AcceptPending();
        for (std::int32_t id=-1;id>-static_cast<std::int32_t>(o::kPendingConnectionSlots);--id) { const auto result=ReceiveOne(id);if (result.consumed) ++count; } // 각 현재 pending은 한 번만 받는다.
        for (std::int32_t id=0;id<static_cast<std::int32_t>(o::kTransceiverSlots);++id) { for (;;) { const auto result=ReceiveOne(id);if (result.consumed) ++count;if (!result.again) break; } } // 재배치/반납 뒤 현재 노드를 다시 선택한다.
    }catch (...) { polling_=false;throw; }EndPoll();return count;
}
// 모든 상위 packet 등록/현재 observer가 이 한 표를 공유한다.
o::RawPacketDispatch& LocalTcpTransport::Dispatch() { return dispatcher_.Dispatch(); }
// State/Chat/상위 sender가 같은 실제 장기 표를 빌린다.
const std::shared_ptr<o::RawConnectionRegistry>& LocalTcpTransport::Registry() const { return registry_; }
// 호출할 때마다 실제 owner의 현재 Player/연결 관문을 다시 선택한다.
o::BattlePacketSend LocalTcpTransport::Peer(std::uint32_t owner) { return [this,owner](auto packet) { static_cast<void>(Send(std::bit_cast<std::int32_t>(owner),packet)); }; }
// 현재 생존 자기 노드의 같은 필드를 잠깐 빌린다.
o::PacketConnection* LocalTcpTransport::Connection(std::int32_t id) { auto* node=Current(id);return node ? &node->connection:nullptr; }
// buffered/OS 접속 완료와 별개인 객체 존재다.
bool LocalTcpTransport::HasConnection(std::int32_t id) const { return Current(id)!=nullptr; }
// 실제 핸들/상위closed를 확인하며 buffered bit2를 성공값으로 사용하지 않는다.
bool LocalTcpTransport::IsOpen(std::int32_t id) const { const auto* node=Current(id);return node && node->socket!=kInvalidSocket && !(node->connection.flags&kClosed); }
// 접속 진행은0이며 실제 OS 실패가 남아 있는 동안에만-1이다.
std::int32_t LocalTcpTransport::Status(std::int32_t id) const { const auto* node=Current(id);return !node ? 0:node->error ? -1:node->connected && !(node->connection.flags&kClosed) ? 1:0; }
// 원문20BYTE 또는0.0.0.0:0의 AF_INET/TCP 표기를 돌려준다.
std::array<std::uint8_t,20> LocalTcpTransport::Address(std::int32_t id) const { const auto* node=Current(id);if (node) return node->connection.address;std::array<std::uint8_t,20> result{};Put(result,0,AF_INET,2);Put(result,16,kTcp);return result; }
// listener는 접속 노드나 등록완료 flag와 별개다.
bool LocalTcpTransport::IsListening() const { return listener_!=kInvalidSocket; }
// 성공한 현재 listener의 포트만 공개한다.
std::uint32_t LocalTcpTransport::ListeningPort() const { return port_; }
// UI 진단은 실제 마지막 Winsock 오류를 그대로 읽는다.
std::int32_t LocalTcpTransport::LastError() const { return lastError_; }
// 아직 OS에 전달하지 않은 자기 송신 바이트만 센다.
std::size_t LocalTcpTransport::QueuedBytes() const {
    std::size_t total{};for (const auto& node:nodes_) { if (!node->retired) total+=node->wire.size()-node->sent+node->aggregate.size(); } // 반납 완료 노드의 과거 큐는 제외한다.
    return total;
}
}
