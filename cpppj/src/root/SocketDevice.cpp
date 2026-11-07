// 원문 R의 IPv4 listen4/select/recv/일시 nonblocking 송신을 Windows10/11 장치에 연결한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Winsock1/2 선언 충돌을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 함수와 Windows 매크로 충돌을 피한다.
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "root/SocketDevice.h"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace netstorm::root { // OS 경계만 구현하며 R 연결표나 가짜 참가자를 만들지 않는다.
namespace { // 같은 실제 주소/포인터 폭과 원문 fd_count 복사만 다룬다.
SOCKET Socket(std::uintptr_t handle) { return static_cast<SOCKET>(handle); } // 저장한 같은 OS 핸들을 복원한다.
std::array<std::uint8_t,20> Address(const sockaddr_in& address) { // network endian16BYTE 뒤 프로토콜 DWORD0을 유지한다.
    std::array<std::uint8_t,20> result{};std::memcpy(result.data(),&address,sizeof(address));return result;
}
bool Local(const sockaddr_in& address,bool broadcast=false) { // 기존 Client와 같은 loopback/RFC1918/link-local 또는 명시 broadcast만 허용한다.
    const auto value=ntohl(address.sin_addr.s_addr),first=value>>24U,second=(value>>16U)&255U;
    return address.sin_family==AF_INET && (first==127 || first==10 || (first==172 && second>=16 && second<=31) || (first==192 && second==168) || (first==169 && second==254) || (broadcast && value==0xffffffffU));
}
int Size(std::size_t size) { if (size>static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::out_of_range("R socket 물리 int 길이 범위");return static_cast<int>(size); } // WinSock int 인자에 크기를 절삭하지 않는다.
void Export(const NetworkSocketSet& from,fd_set& to) { // 원문 직접 추가의 중복을 제거하지 않고64칸을 옮긴다.
    if (from.count>64 || from.count>FD_SETSIZE) throw std::out_of_range("R select64 집합 범위");to.fd_count=static_cast<u_int>(from.count);
    for (std::size_t index=0;index<from.count;++index) to.fd_array[index]=Socket(from.sockets[index]); // 같은 순서/실제 포인터 폭으로 복사한다.
}
void Import(const fd_set& from,NetworkSocketSet& to) { // select가 수정한 실제 집합만 같은 부모에 돌려준다.
    if (from.fd_count>64) throw std::out_of_range("R select 반환 집합 범위");to.count=from.fd_count;
    for (std::size_t index=0;index<to.count;++index) to.sockets[index]=from.fd_array[index]; // 사용하지 않는 tail은 해석하지 않는다.
}
}
struct SocketDevice::System { // 같은 accept 계열이 공유하는 실제 WSA 참조다.
    WSADATA data{};int result{WSAStartup(MAKEWORD(2,2),&data)}; // Windows10/11의2.2를 요청한다. 원본1.1 ABI 자체를 사용하지 않는다.
    ~System() { if (!result) WSACleanup(); } // 성공한 시작 한 번만 마지막 owner에서 반납한다.
};
SocketDevice::~SocketDevice() { Close(); } // 자기 핸들을 반납한 뒤 공유 WSA owner가 소멸한다.
bool SocketDevice::IsOpen() const { return socket_!=kInvalidSocket; } // 실제 소켓0도 현대 OS의 유효한 반환이면 보존한다.
void SocketDevice::Close() { if (IsOpen()) { closesocket(Socket(socket_));socket_=kInvalidSocket; }flags_&=~0x40U; } // 원문 close의 반환 무시와 기존 오류 보존을 유지한다.
void SocketDevice::CloseNative() {
    if (flags_&8U) throw std::logic_error("R 관리 명령의 원문 Zlocal 메모리 장치 미연결");if ((flags_&0x40U) && IsOpen()) { if (flags_&0x400U) Sleep(500);closesocket(Socket(socket_));socket_=kInvalidSocket; }error_=WSAGetLastError();flags_&=~0x40U; // 기존 RAII Close와 달리 원문 호출 시점의 오류를 캡처하고 부모 노드를 보존한다.
}
std::unique_ptr<SocketDevice> SocketDevice::Detach() {
    auto result=std::make_unique<SocketDevice>();result->system_=system_;result->socket_=socket_;result->flags_=flags_;result->error_=error_;result->peer_=peer_;socket_=kInvalidSocket;return result; // 할당 성공 뒤에만 옛 핸들을 무효화한다. 새 소유자가 최종 close 권한과 WSA 수명을 유지한다.
}
void SocketDevice::PrepareTransferredStream() { flags_=0x444U;error_=0;peer_.fill(0); } // 새 CCh inline socket 초기값이며 실제 socket/WSA를 새로 열지 않는다.
void SocketDevice::FinishTransferredStream() { const auto address=QueryPeerAddress();if ((std::uint32_t(address[0])|(std::uint32_t(address[1])<<8U))==AF_INET) flags_=0x54U; } // listener 비트를 상속한 이전 장치를 그대로 CCh stream으로 사용하지 않는다.
std::int32_t SocketDevice::Error(std::int32_t code) { error_=code;flags_|=0x2000U;return -1; } // 현재 실패만 실제 코드로 기록한다.
bool SocketDevice::OpenDatagram(std::uint32_t port) { return Open(port,false); } // 서버 UDP만 열며 패킷/512BYTE 버퍼는 바깥 노드가 소유한다.
bool SocketDevice::Listen(std::uint32_t port) { return Open(port,true); } // 실제 새 TCP listener만 연다.
bool SocketDevice::ConnectStream(std::span<const std::uint8_t,20> destination) {
    if (IsOpen()) throw std::logic_error("R 생존 socket 중복 연결");sockaddr_in address{};std::memcpy(&address,destination.data(),sizeof(address));std::memcpy(&flags_,destination.data()+16,sizeof(flags_));error_=0;
    if (address.sin_family!=AF_INET || (flags_&0x30U)!=0x10U || !(flags_&4U) || (flags_&2U)) throw std::invalid_argument("R 송신 연결의 IPv4/client TCP 장치 범위");
    const auto fail=[this](int code) { error_=code;Close();return false; }; // 원문 생성 실패는 마지막 오류/무효 핸들만 남긴다. 수신·송신 오류의 bit2000을 추가하지 않는다.
    if (!Local(address)) return fail(WSAEACCES); // 외부 IP/DNS에 연결하지 않는 사용자 LAN 정책을 실제 connect 앞에 적용한다.
    if (!system_) system_=std::make_shared<System>();if (system_->result) { const auto code=system_->result;system_.reset();return fail(code); }
    socket_=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if (!IsOpen()) return fail(WSAGetLastError());
    const int enabled=1;if ((flags_&0x100U) && setsockopt(Socket(socket_),IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&enabled),sizeof(enabled))==SOCKET_ERROR) return fail(WSAGetLastError()); // 원문 n 옵션을 실제 TCP_NODELAY에 전달한다.
    u_long nonblocking=1;if ((flags_&1U) && ioctlsocket(Socket(socket_),FIONBIO,&nonblocking)) return fail(WSAGetLastError());
    if (::connect(Socket(socket_),reinterpret_cast<const sockaddr*>(&address),sizeof(address))<0) { const auto code=WSAGetLastError();if (!(flags_&1U) || code!=WSAEWOULDBLOCK) return fail(code); } // nonblocking의 실제 would-block만 열린 연결 시작으로 유지한다.
    flags_|=0x40U;return true; //00478a70의 open 관문이며 연결 완료/실제 응답을 합성하지 않는다.
}
bool SocketDevice::Open(std::uint32_t port,bool stream) {
    if (IsOpen()) throw std::logic_error("R 생존 socket 중복 열기");if (port>65535) throw std::out_of_range("R socket 포트 WORD 범위");
    flags_=stream ? 0x92U:0x22U; // 구현 범위의 TCP/listener/server 또는 UDP/server다. 성공 뒤40을 더한다.
    if (!system_) system_=std::make_shared<System>();if (system_->result) { const auto code=system_->result;system_.reset();Error(code);return false; } // 실패한 WSA 시작을 성공 owner로 캐시하지 않아 다음 명시 열기가 실제 재시도한다.
    socket_=::socket(AF_INET,stream ? SOCK_STREAM:SOCK_DGRAM,stream ? IPPROTO_TCP:IPPROTO_UDP);
    if (!IsOpen()) { Error(WSAGetLastError());return false; }
    const int enabled=1;sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(static_cast<u_short>(port)); // 현재 명시 포트로 wildcard bind한다.
    if (!stream && setsockopt(Socket(socket_),SOL_SOCKET,SO_BROADCAST,reinterpret_cast<const char*>(&enabled),sizeof(enabled))==SOCKET_ERROR) { Error(WSAGetLastError());Close();return false; }
    if (bind(Socket(socket_),reinterpret_cast<const sockaddr*>(&address),sizeof(address))==SOCKET_ERROR) {
        if (setsockopt(Socket(socket_),SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&enabled),sizeof(enabled))==SOCKET_ERROR || bind(Socket(socket_),reinterpret_cast<const sockaddr*>(&address),sizeof(address))==SOCKET_ERROR) { Error(WSAGetLastError());Close();return false; } // 원문 첫 bind 실패 뒤 reuse/재시도를 보존한다.
    }
    if (stream && ::listen(Socket(socket_),4)==SOCKET_ERROR) { Error(WSAGetLastError());Close();return false; } // 제공된 R의 실제 backlog4다.
    flags_|=0x40U;return true;
}
std::int32_t SocketDevice::Ready(bool writing) {
    if (!IsOpen()) return Error(WSAENOTSOCK);fd_set current{},errors{};current.fd_count=errors.fd_count=1;current.fd_array[0]=errors.fd_array[0]=Socket(socket_);timeval timeout{};
    const auto result=::select(0,writing ? nullptr:&current,writing ? &current:nullptr,&errors,&timeout);
    if (result<0 || errors.fd_count) return Error(WSAGetLastError());return current.fd_count ? 1:0;
}
std::int32_t SocketDevice::Status() { if (flags_&0x220U) return 0;return Ready(true); } // 원문 UDP 준비 조회0과 stream select를 구분한다.
std::int32_t SocketDevice::Accept(std::unique_ptr<SocketDevice>& accepted) {
    if (!(flags_&0x80U)) return Error(WSAEINVAL);const auto ready=Ready(false);if (ready<=0) return ready;
    sockaddr_in peer{};int size=sizeof(peer);const auto socket=::accept(Socket(socket_),reinterpret_cast<sockaddr*>(&peer),&size);if (socket==INVALID_SOCKET) return Error(WSAGetLastError());
    if (!Local(peer)) { closesocket(socket);return Error(WSAEACCES); } // 사용자 지시의 LAN 범위를 벗어난 연결을 상위 노드에 공개하지 않는다.
    try { auto child=std::make_unique<SocketDevice>();child->system_=system_;child->socket_=socket;child->flags_=flags_;child->error_=error_;child->peer_=Address(peer);accepted=std::move(child); } catch (...) { closesocket(socket);throw; }
    return 1;
}
std::int32_t SocketDevice::Receive(std::span<std::uint8_t> bytes) {
    const auto ready=Ready(false);if (ready<=0) return ready;const auto result=recv(Socket(socket_),reinterpret_cast<char*>(bytes.data()),Size(bytes.size()),0);if (result<1) return Error(WSAGetLastError());return result; // 원문 한 번 읽기/EOF 오류다.
}
std::int32_t SocketDevice::ReceiveDatagram(std::span<std::uint8_t> bytes,std::array<std::uint8_t,20>& source) {
    if (!(flags_&0x220U)) return 0; // 원문89170은 datagram 기반이 아니면 select/recvfrom을 실행하지 않는다.
    const auto ready=Ready(false);if (ready<=0) return ready;source.fill(0);sockaddr_in peer{};int size=sizeof(peer);const auto result=recvfrom(Socket(socket_),reinterpret_cast<char*>(bytes.data()),Size(bytes.size()),0,reinterpret_cast<sockaddr*>(&peer),&size);
    if (result<0) return Error(WSAGetLastError());source=Address(peer);if (!Local(peer)) return 0;return result; // 실제로 소비한 외부 데이터그램은 사용자 LAN 정책으로 분배하지 않는다.
}
std::int32_t SocketDevice::Send(std::span<const std::uint8_t> bytes) { return Write(bytes,nullptr); } // UDP의 unconnected send 실패도 OS 결과로 보존한다.
std::int32_t SocketDevice::SendTo(std::span<const std::uint8_t> bytes,std::span<const std::uint8_t,20> destination) {
    if (!(flags_&0x220U)) return 0; // 원문893e0은 datagram 기반이 아니면 OS 송신을 호출하지 않는다.
    sockaddr_in address{};std::memcpy(&address,destination.data(),sizeof(address));if (!Local(address,true)) return Error(WSAEACCES);std::array<std::uint8_t,20> current{};std::memcpy(current.data(),destination.data(),20);return Write(bytes,&current);
}
std::int32_t SocketDevice::Write(std::span<const std::uint8_t> bytes,const std::array<std::uint8_t,20>* destination) {
    if (!IsOpen()) return Error(WSAENOTSOCK);const auto length=Size(bytes.size());u_long nonblocking=1;if (ioctlsocket(Socket(socket_),FIONBIO,&nonblocking)) return Error(WSAGetLastError());
    const auto result=destination ? sendto(Socket(socket_),reinterpret_cast<const char*>(bytes.data()),length,0,reinterpret_cast<const sockaddr*>(destination->data()),static_cast<int>(destination->size())):send(Socket(socket_),reinterpret_cast<const char*>(bytes.data()),length,0);
    const auto saved=result<0 ? WSAGetLastError():0;if (result<0) error_=saved;nonblocking=0;if (ioctlsocket(Socket(socket_),FIONBIO,&nonblocking)) return Error(WSAGetLastError());
    if (result<0) return saved==WSAEWOULDBLOCK ? 0:Error(saved);return result; // would-block만 원문처럼0이며 짧은 성공 길이는 감추지 않는다.
}
std::array<std::uint8_t,20> SocketDevice::QueryPeerAddress() {
    sockaddr_in address{};int size=sizeof(address);if (getpeername(Socket(socket_),reinterpret_cast<sockaddr*>(&address),&size)<0) error_=WSAGetLastError(); // 원문은 이 조회 오류에서2000 flag를 추가하지 않는다.
    std::memcpy(peer_.data(),&address,sizeof(address));return peer_; //896d0은 앞16BYTE만 쓰므로 기존 프로토콜 tail을 보존한다.
}
std::array<std::uint8_t,20> SocketDevice::LocalAddress() {
    sockaddr_in address{};int size=sizeof(address);if (getsockname(Socket(socket_),reinterpret_cast<sockaddr*>(&address),&size)==SOCKET_ERROR) { Error(WSAGetLastError());return {}; }return Address(address);
}
std::int32_t SocketDevice::Select(std::uintptr_t highestPlusOne,NetworkSocketSets& sets,std::int32_t waitSeconds) {
    static_cast<void>(highestPlusOne);fd_set reads{},writes{},errors{};Export(sets.read,reads);Export(sets.write,writes);Export(sets.error,errors);timeval timeout{waitSeconds,0};
    const auto result=::select(0,&reads,&writes,&errors,&timeout);Import(reads,sets.read);Import(writes,sets.write);Import(errors,sets.error);return result; // 오류에서도 부모는 현재 수신 순회를 계속한다.
}
}
