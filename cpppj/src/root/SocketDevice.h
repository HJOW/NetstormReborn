// R 전송 노드가 소유할 Windows IPv4 소켓 장치를 게임22칸 Registry와 분리한다.
#pragma once
#include "root/NetworkPump.h"
#include <memory>
#include <span>

namespace netstorm::root { // 실제 OS 핸들/WSA 수명과 LAN IPv4 경계만 담당한다.
class SocketDevice final { // 생성은 WSA/소켓을 열지 않으며 명시 Open/Listen에서만 시작한다.
public:
    SocketDevice()=default; // 최초 무효 핸들과 오류0만 구성한다.
    ~SocketDevice(); // 남은 자기 핸들과 마지막 공유 WSA 참조를 반납한다.
    SocketDevice(const SocketDevice&)=delete;SocketDevice& operator=(const SocketDevice&)=delete; // OS 핸들의 닫기 권한을 복제하지 않는다.
    bool OpenDatagram(std::uint32_t port); // IPv4 UDP wildcard bind/broadcast를 열고 실제 실패는false/LastError로 보존한다.
    bool Listen(std::uint32_t port); // R의 TCP wildcard listener를 backlog4로 연다.
    std::int32_t Accept(std::unique_ptr<SocketDevice>& accepted); //88760의 select0→accept이며0은준비없음/-1은오류/1은새 실제 소켓이다.
    std::int32_t Receive(std::span<std::uint8_t> bytes); //88a90의 select0→한번 recv이며 TCP EOF도 오류로 반환한다.
    std::int32_t ReceiveDatagram(std::span<std::uint8_t> bytes,std::array<std::uint8_t,20>& source); //89170의 select0→recvfrom과 같은 network endian20BYTE다.
    std::int32_t Send(std::span<const std::uint8_t> bytes); //88e90의 FIONBIO1→send→FIONBIO0이며 would-block은0/부분 송신은실제길이다.
    std::int32_t SendTo(std::span<const std::uint8_t> bytes,std::span<const std::uint8_t,20> destination); // 실제 로컬 IPv4/broadcast sendto만 허용한다. DNS/구 서버 조회를 하지 않는다.
    std::int32_t Status(); //88590의 UDP0/IPv4 stream의 write/error select0 판정이다.
    std::array<std::uint8_t,20> QueryPeerAddress(); //89620의 실제 getpeername/마지막 오류를 조회하며 주소 마지막DWORD를 보존한다.
    std::array<std::uint8_t,20> LocalAddress(); // 실제 getsockname 결과이며 wildcard를 임의 호스트 IP로 대체하지 않는다.
    const std::array<std::uint8_t,20>& PeerAddress() const { return peer_; } // accept가 받은 같은 실제 peer 주소다.
    std::uintptr_t Handle() const { return socket_; } // OS 핸들의 포인터 폭을 유지한다.
    std::uint32_t Flags() const { return flags_; } // 이 장치가 구현한 IPv4/서버·listener/열림·오류 플래그이며 전체 레거시 socket ABI가 아니다.
    std::int32_t LastError() const { return error_; } // 성공/준비없음으로 이전 오류를 임의 지우지 않는다.
    bool IsOpen() const; // 실제 핸들의 존재만 읽는다.
    void Close(); // 자기 핸들을 한 번 닫고 open bit를 내린다. 다른 연결표는 변경하지 않는다.
    void CloseNative(); //00488430의IPv4 open관문/선택500ms대기·실제close/WSA마지막오류·bit40해제다. 노드종료나부모정리를추가하지않는다.
    std::unique_ptr<SocketDevice> Detach(); //00476368..79의 실제 핸들 읽기/옛 핸들 무효화를 소유권 이전으로 표현한다. WSA 참조/핸들 폭을 보존하고 OS close는 하지 않는다.
    void PrepareTransferredStream(); //004841f0의 새 CCh socket flags444/오류0/초기 주소를 같은 이전 핸들에 구성한다.
    void FinishTransferredStream(); //0044ea1c..3a의 별도 peer 조회 뒤 IPv4일 때만 flags54를 쓴다.
    static std::int32_t Select(std::uintptr_t highestPlusOne,NetworkSocketSets& sets,std::int32_t waitSeconds); // 원문64칸 세 집합을 실제 WinSock에 전달하고 수정된 집합을 되돌린다. nfds는 WinSock에서 무시한다.
private:
    struct System;std::shared_ptr<System> system_; // accept 자식과 같은 WSA2.2 시작/마지막 반납 수명을 공유한다.
    static constexpr std::uintptr_t kInvalidSocket=~std::uintptr_t{}; // 포인터 절삭 없이 Windows INVALID_SOCKET을 표현한다.
    std::uintptr_t socket_{kInvalidSocket};std::uint32_t flags_{};std::int32_t error_{}; // 실제 핸들/구현된 현재 플래그/마지막 OS 오류다.
    std::array<std::uint8_t,20> peer_{}; // TCP accept가 쓴 실제 주소이며 UDP 수신 주소는 caller의 현재20BYTE에 쓴다.
    bool Open(std::uint32_t port,bool stream); // 기존 생존 핸들 교체를 거부하고 실제 socket/bind/listen 순서를 수행한다.
    std::int32_t Ready(bool writing); // 원문0초 select의 error/현재 read 또는 write 준비를 읽는다.
    std::int32_t Error(std::int32_t code); // 같은 마지막 코드와 원문 socket 오류 bit2000을 기록한다.
    std::int32_t Write(std::span<const std::uint8_t> bytes,const std::array<std::uint8_t,20>* destination); // 직접 send와 명시 sendto의 같은 일시 nonblocking/복구를 공유한다.
};
}
