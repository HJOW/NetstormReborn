// 로컬 LAN의 원문 TCP 프레임과 현재 연결표를 잇는 전송 부품이다. 실제 Login/UI 접합은 호출자가 제공한다.
#pragma once
#include "zacket/LocalMissionTransport.h"
#include <cstdint>
#include <list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client { // 원문 Ztrans/Zocket의 OS 경계만 Windows TCP 소유자로 옮긴다.
// IPv4 LAN의 TCP listener·accepted 노드·client9를 같은 장기 연결표와 원문 패킷 분배기에 보관한다.
// 생성은 소켓을 열지 않는다. Listen/Connect를 명시 호출해야 하며 단독 미션/Edit의 내부 전송을 대체하지 않는다.
class LocalTcpTransport final {
public:
    LocalTcpTransport(std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry={},std::shared_ptr<o::PacketRegistrationTable> registrations={}); // Player9와 현재 서버 권한/실제 진단 장치는 이 owner보다 오래 살아야 한다.
    ~LocalTcpTransport(); // 최종 owner 종료에서 자기 노드의 별칭과 OS 핸들을 반드시 반납한다. 정상 State cleanup은 CloseAll로 요청한다.
    LocalTcpTransport(const LocalTcpTransport&)=delete; // 실제 socket/노드와 현재 등록표의 반납 권한을 복제하지 않는다.
    LocalTcpTransport& operator=(const LocalTcpTransport&)=delete; // 생존 연결의 고정 주소를 다른 전송으로 대입하지 않는다.
    LocalTcpTransport(LocalTcpTransport&&)=delete; // 등록된 callback의 this와 안정된 노드 주소를 이동하지 않는다.
    LocalTcpTransport& operator=(LocalTcpTransport&&)=delete; // 실제 연결을 가진 owner를 이동 대입하지 않는다.

    void Listen(std::uint32_t port,std::string_view bindAddress="0.0.0.0"); //004e2430/004e02d0의 TCP listener만 연다. bind 주소는 wildcard 또는 로컬 IPv4 literal이다.
    void StopListening(); // 현재 accepted/등록 연결은 보존하고 신규 수락만 중단한다.
    bool Connect(std::string_view address,std::uint32_t port,std::int32_t id=9,std::span<std::uint8_t> initialPacket={}); //004e2620의 기존 socket 비영 재사용 또는 새 LAN 접속을 수행한다. socket 접속 실패는false이며 빈 span은NULL이다.
    bool Connect(std::span<const std::uint8_t> address,std::uint32_t port,std::int32_t id=9,std::span<std::uint8_t> initialPacket={}); // 재사용 관문 뒤20BYTE 주소/포트/초기 패킷을 같은 새 접속 부모에 전달한다.
    bool ConnectPeerText(std::string_view address,std::uint32_t port,std::int32_t id=9,std::span<std::uint8_t> initialPacket={}); // 재사용 관문 뒤IP:port 또는tcp,IP:port,cn을 해석한다. DNS/공개주소는 기존 정책대로 허용하지 않는다.
    void AssignConnection(std::int32_t pending,std::int32_t owner); //004e27b0의 같은 accepted 노드를 owner1..8 또는 비서버9로 이동하고 buffered 모드로 바꾼다.
    void Close(std::int32_t id); // 선택한 자기 연결만 closed로 표시하며 수신 depth가0일 때 실제 OS/등록표 반납을 수행한다.
    void CloseAll(); // 자기 listener와 모든 자기 노드를 닫는다. 다른 내부/외부 전송의 공유 등록표를 지우지 않는다.

    bool Send(std::int32_t destination,std::span<const std::uint8_t> packet); // 이미 길이를 확정한 writer의 선언 WORD 바이트를 현재 자기 연결로 보낸다.
    bool SendMutable(std::int32_t destination,std::span<std::uint8_t> packet); // 현재 Player/연결 관문 뒤 원문 observer(1)를 같은 버퍼에 실행한다.
    void Flush(); //004e2920의 정상22칸 번호순으로 type4를 확정하고 partial send의 나머지를 보관한다.
    std::size_t Poll(); // listener 수락→pending 한 번→정상0..21 순서로 비동기 접속/부분 프레임을 진행한다. GUI 스레드를 기다리지 않는다.

    void BeginPoll(); // 실제 수신 전 closed/depth0 정리와 중첩 guard를 수행한다.
    void AcceptPending(); // 실제 TCP listener의 accept만 수행한다. 생성 전 pending을 만들지 않는다.
    o::TransportPollResult ReceiveOne(std::int32_t id); // 현재 자기 ID의 한 완성 frame/partial 진행만 처리한다.
    void FlushOne(std::int32_t id); // 현재 정상 ID의 실제 partial send/집합만 확정한다.
    void EndPoll(); // 최외곽 callback 대여 뒤에만 retired 노드를 물리 수거한다.

    o::RawPacketDispatch& Dispatch(); // LocalMissionTransport의 집합 분배를 Open 없이 재사용하는 같은 등록표다.
    const std::shared_ptr<o::RawConnectionRegistry>& Registry() const; // 같은 현재 토큰/주소/close 장치를 상위 Session에 빌린다.
    o::BattlePacketSend Peer(std::uint32_t owner); // 기존 RawBattleBroadcast가 실제 owner 연결을 현재값으로 조회하는 송신 장치다.
    o::PacketConnection* Connection(std::int32_t id); // 현재 자기 노드를 잠깐 빌리며 callback/다음 Poll 이후까지 주소를 보관하지 않는다.
    bool HasConnection(std::int32_t id) const; // buffered/연결완료와 별개인 현재 자기 노드의 존재다.
    bool IsOpen(std::int32_t id) const; //004e2b10의 실제 socket 존재/상위 closed 미설정이다. 접속 진행 상태와 구별한다.
    std::int32_t Status(std::int32_t id) const; //004e2b70의 같은 현재 socket 준비 조회이며0/1/-1을 캐시/Packet flags로 대체하지 않는다.
    std::array<std::uint8_t,20> Address(std::int32_t id) const; //004e2a50의 같은 peer20BYTE 또는 원문 zero IPv4 주소다.
    bool IsListening() const; // 실제 listener 핸들의 존재를 반환한다.
    std::uint32_t ListeningPort() const; // 성공한 Listen의 실제 포트이며 listener가 없으면0이다.
    std::int32_t LastError() const; // 마지막 OS 접속/수신/송신 실패 코드를 UI 진단용으로 반환한다.
    std::size_t QueuedBytes() const; // 자기 노드의 아직 전송하지 않은 wire/미확정 집합 바이트 합계다.

private:
    struct Node; // 안정된 PacketConnection 주소·socket·부분 프레임·토큰을 하나로 소유하는 구현 객체다.
    static constexpr std::uintptr_t kInvalidSocket=~std::uintptr_t{}; // SOCKET의 INVALID_SOCKET을 Windows 헤더 없이 표현한다.
    o::PacketDispatchHooks DispatchHooks(); // 같은 현재 분배기의 종료/close/free를 실제 TCP 노드와 Registry에 연결한다.
    void StartSockets(); // 첫 명시 Listen/Connect에서만 Winsock2를 초기화한다.
    Node* Find(std::uint32_t token) const; // 자기 생존 노드만 토큰으로 찾고 다른 전송의 노드를 반환하지 않는다.
    Node* Find(const o::PacketConnection& connection) const; // callback이 준 같은 고정 주소의 자기 노드를 찾는다.
    std::uint32_t Token(std::int32_t id) const; // 공유 표의 현재 pending/정상 칸만 읽는다.
    Node* Current(std::int32_t id) const; // 현재 표와 자기 owner를 함께 확인한다.
    Node& Adopt(std::uintptr_t socket,std::int32_t id,bool connecting,bool buffered,const std::array<std::uint8_t,20>& address,bool publishConnect=false); // 실제 노드/핸들을 소유하며 접속 생성의 직접 공개와 일반 Register/Accept를 구별한다.
    bool ReuseConnect(std::int32_t id) const; // 현재 칸의 실제 준비값이 비영이면 주소/포트/새 패킷을 읽기 전에 자연 반환한다.
    bool ConnectNew(std::string_view address,std::uint32_t port,std::int32_t id,std::span<std::uint8_t> initialPacket); // 기존 준비0/빈 칸에서만 새 socket을 만들고 성공 뒤 칸/초기 패킷을 공개한다.
    std::int32_t SocketReadiness(Node& node) const; //004e1800의 TCP select writable/exception과 오류 시 socket 닫기만 수행하며 type1/2는 알리지 않는다.
    void Accept(); // 원문 pending31칸 상한 아래에서만 실제 accepted 소켓을 등록한다.
    bool CompleteConnection(Node& node); // 같은 socket 준비 조회 뒤 실제 Poll에서만 type1/2를 알린다.
    bool ReceiveOne(Node& node,bool* consumed=nullptr); // 한 선언 프레임만 부분 수신하며 무효/sentinel이면 같은 Poll의 추가 분배를 멈춘다.
    bool SendPacket(std::int32_t destination,std::span<const std::uint8_t> packet,std::span<std::uint8_t> writable,o::PacketConnection* direct=nullptr); // 일반 번호 송신은 gate를 적용하고 이미 선택된 실제 가상 this만 직접 노드/큐를 사용한다.
    void SendNode(Node& node,std::span<const std::uint8_t> packet); // buffered는 미확정 집합에 추가하고 직접 송신은 wire에 넣는다.
    void Finalize(Node& node); // 아직 확정하지 않은 payload를 원문 type4/WORD 헤더로 한 번 감싼다.
    void Compact(Node& node); // 이미 전달한 wire 접두만 지워 원문 partial-send memmove와 같은 미전달 바이트를 남긴다.
    void Drain(Node& node); // 양수 partial send만큼만 소비하고 would-block의 접두는 보관한다.
    void Notify(Node& node,std::uint8_t type); //004dfa40/004df9f0의 type1/2를 단일 observer 전처리 없이 분배한다.
    void Fail(Node& node,std::int32_t error); // 현재 원문 termination을 한 번 통지한 뒤 depth를 지켜 명시 종료한다.
    void CloseReady(Node& node); // closed/depth0 노드의 같은 Registry close/free만 선택한다.
    void CollectRetired(); // 분배 callback이 모두 끝난 뒤 반납 완료 노드만 물리적으로 지운다.
    void CloseSocket(Node& node); // OS 소켓을 한 번만 닫고 자기 장치의 실패를 명시 기록한다.

    std::span<const std::uint8_t> players_; // 현재 Player9×180BYTE의 flags1을 읽으며 사본을 만들지 않는다.
    const bool& server_; // Assign이 원문 위치에서 읽는 실제 현재 서버 권한이다.
    o::PacketDispatchHooks hooks_; // 호출자가 제공하는 현재 시계·진단·상위 종료/반납 장치다.
    std::shared_ptr<o::RawConnectionRegistry> registry_; // Local/Edit/State와 공유할 수 있는 실제 장기 연결표다.
    zacket::LocalMissionTransport dispatcher_; // Open하지 않고 같은 단일/집합 등록표와 분배 순서만 사용한다.
    std::list<std::unique_ptr<Node>> nodes_; // callback/Registry 대여 중 주소가 바뀌지 않는 자기 노드 owner다.
    std::uintptr_t listener_{kInvalidSocket}; // 실제 TCP listener 핸들이며 생성자에서는 열지 않는다.
    std::uint32_t port_{}; // 현재 listener가 성공한 실제 포트다.
    mutable std::int32_t lastError_{}; // 원문의 상태 조회도 갱신할 마지막 실제 Winsock 오류이며 정상값을 합성하지 않는다.
    bool socketsStarted_{},polling_{}; // Winsock 참조 수명과 같은 전송의 중첩 Poll 거부 상태다.
    std::size_t activeCalls_{}; // callback/송신 중 재진입 Poll이 현재 노드를 조기 수거하지 못하게 하는 동기 대여 수다.
};
}
