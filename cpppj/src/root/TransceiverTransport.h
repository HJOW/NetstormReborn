// R의 실제 소켓/버퍼/숫자 토큰을256/32 연결표와 메인 통신 부모에 연결한다.
#pragma once
#include "root/NetworkPump.h"
#include "o/RawPacketDispatch.h"
#include <list>
#include <memory>

namespace netstorm::root { // 게임 Player/22칸 Registry를 만들지 않는 R 전용 전송 소유자다.
class SocketDevice; // 실제 OS 핸들과 공유 WSA 수명을 소유한다.
struct TransceiverHooks { // 프로세스의 실제 시계/기록/진단/종료 장치는 호출자가 보유한다.
    std::function<std::uint32_t()> now,captureNow; // 현재473b80 전역 시각과 새 지역4739b0 캡처를 구별한다.
    std::function<void(bool,o::PacketConnection&,std::span<const std::uint8_t>,std::uint16_t)> record; // 실제 방향/현재 주소·sender/논리 길이를 원문 기록 장치에 전달한다.
    std::function<void(std::int32_t,std::string_view)> log; // 원문 console/진단의 이미 작성한 메시지를 보낸다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 실제 선택된 R/Zacket 진단이며 복귀하면 뒤의 원문 쓰기를 계속한다.
    std::function<std::string(std::span<const std::uint8_t,20>)> formatAddress; //864b0의 현재 주소1/0/0 서식이다. 타임아웃이 선택할 때만 요구한다.
    std::function<void(std::uint32_t,std::uint32_t)> endParticipant; // 실제 Challenge 토큰/lowBYTE 참가자 종료다.
    std::function<void(std::int32_t)> exit; // 원문_exit(0/1)의 실제 비복귀 장치다.
};
enum class ReceiveKind { Stopped,Sentinel,Packet }; // 원문 NULL/공유 길이0 sentinel/현재 수신 버퍼 포인터를 구별한다.
struct ReceivedFrame { // 현재 버퍼는 다음 이 owner의 변경 호출까지만 빌릴 수 있다.
    ReceiveKind kind{ReceiveKind::Stopped}; // 소켓 종료/아직 완성되지 않음/실제 패킷이다.
    std::span<std::uint8_t> bytes; // Packet일 때만 현재 선언 길이의 실제 mutable 입력을 빌린다.
};
class TransceiverTransport final { // 이 owner는 등록 callback과 모든 빌린 노드/표보다 오래 살아야 한다.
public:
    TransceiverTransport(ConnectionState& connections,NetworkState& network,TransceiverHooks hooks,std::shared_ptr<o::PacketRegistrationTable> registrations={}); // 생성은 네트워크를 열지 않으며 같은 실제 표/원문 패킷 표만 연결한다.
    ~TransceiverTransport(); // 최종 소유자 종료에서 자기 소켓/별칭만 반납하며 사용자 callback을 재호출하지 않는다.
    TransceiverTransport(const TransceiverTransport&)=delete;TransceiverTransport& operator=(const TransceiverTransport&)=delete; // 고정 노드/콜백 this와 OS 소유권을 복제하지 않는다.
    std::uint32_t OpenDatagram(std::uint32_t port,std::uint32_t capacity=512); // R LAN UDP의 실제 새 일반 번호/512BYTE 버퍼다. 실패는0이며 성공 상태를 합성하지 않는다.
    std::uint32_t Listen(std::uint32_t port,std::uint32_t capacity=4096); // listener 자체 버퍼8과 수락 노드4096을 구별하고 최초 pending 초기화를 수행한다.
    void InstallTerminationReceiver(); //477e10이 실제 초기화 위치에서 등록하는2번→75170을 연결한다.
    std::unique_ptr<SocketDevice> DetachSocket(std::uint32_t token); // 현재 대기 노드의 실제 socket만 이전한다. 뒤의 ClosePending은 그 OS 핸들을 닫지 않는다.
    std::uint32_t AdoptTransferredStream(std::unique_ptr<SocketDevice> socket); //004841f0의 sender0/4096/flags444→별도 peer 조회/IPv4 flags54를 구성한다. 표 공개/참가자 번호는 실제 CCh 로그인 부모가 수행한다.
    ReceivedFrame ReceiveOne(std::uint32_t token,bool dispatch=true); //848c0/85aa0의 한 수신/수락 순회다. false는 observer 뒤 실제 패킷 분배만 생략한다.
    void Send(std::uint32_t token,std::span<std::uint8_t> packet); //84e90의 매번 observer1/현재 길이/직접 또는 집합 쓰기/기록이다.
    void SendTo(std::uint32_t token,std::span<std::uint8_t> packet,std::span<const std::uint8_t,20> destination); //85250의 UDP 가상 송신이며 짧은 쓰기는2번 통지다.
    void Flush(std::uint32_t token); //853e0의 집합 확정/실제 부분 송신/접두 이동을 수행한다.
    void SetBuffered(std::uint32_t token,bool enabled); //856a0처럼 켤 때 현재 포인터를 다시4/0으로 쓰고 끌 때 먼저 flush한다.
    void Close(std::uint32_t token); //84820의 killed쓰기→depth0 소멸/flush/지연 물리 반납이다.
    void Terminate(std::uint32_t token); //83ea0의 실제2번 처리기가 있고 최초일 때만 통지한다.
    NetworkPumpResult Pump(std::int32_t waitSeconds,NetworkHooks upper); // 실제 SocketDevice/select/현재 노드를1062 부모에 연결한다. Challenge/보조 객체의 실제 장치는 upper가 제공한다.
    o::RawPacketDispatch& PacketHandlers() { return dispatch_; } // 등록 API 전용이다. 수신 실행은 이 owner의 ReceiveOne을 통해 수명을 보장한다.
    std::uint32_t TokenFor(const o::PacketConnection& connection) const; // 실제 처리기가 받은 같은 연결의 생존 토큰을 조회한다. 외부/반납 세대는0이다.
    std::uint32_t SocketFlags(std::uint32_t token); // CCh 종료가 실제 하위 socket 오류 bit2000을 읽는다. 연결 flags와 혼동하지 않는다.
    std::array<std::uint8_t,20> SocketPeerAddress(std::uint32_t token); // CCh 생성의 별도 두 번째 조회가 보관한 실제 peer를 복사한다. OS 조회를 추가 실행하지 않는다.
    o::PacketConnection& Connection(std::uint32_t token); // 같은 현재 노드를 다음 변경 호출 전까지 빌린다.
    std::uint32_t& NoMainTimeout(std::uint32_t token); // 노드+90의 같은 DWORD다. UDP bind 성공 뒤 원문 부모가1을 쓴다.
    std::int32_t& WriteLimit(std::uint32_t token); // 노드+70의 초기100000/현재 signed 송신 한계다.
    ConnectionHooks Lifecycle(); // 실제 close/현재 sender 쓰기/Challenge 종료/exit를 같은 R 번호 부모에 빌린다.
    bool CallbacksActive() const { return activeCalls_!=0; } // 부모 종료가 이 owner를 callback 중 반납하지 않도록 실제 외곽 대여를 조회한다.
private:
    struct Node;struct CallGuard; // 안정된 노드 물리와 최외곽 반납 관문은 내부 헤더에 정의한다.
    ConnectionState& connections_;NetworkState& network_;TransceiverHooks hooks_; // 원문 현재 표/시각·전역/장치의 장기 참조다.
    o::RawPacketDispatch dispatch_; // PE로 동일을 확인한137종/138길이 표를 사용하며 Player가 없다.
    std::list<std::unique_ptr<Node>> nodes_; // callback 중 닫힌 노드도 외곽 복귀까지 같은 주소로 유지한다.
    std::uint32_t serial_{};std::size_t activeCalls_{};bool pumping_{}; // 재사용하지 않는 숫자 토큰/동기 대여/재진입 관문이다.
    Node& Get(std::uint32_t token); // 생존한 자기 숫자 토큰만 조회한다. 호스트 주소를 DWORD로 자르지 않는다.
    Node& From(o::PacketConnection& connection); // close/release 단계에서도 같은 실제 물리를 찾는다.
    std::uint32_t Adopt(std::unique_ptr<SocketDevice> socket,std::uint32_t sender,std::uint32_t capacity,bool listener=false,std::uint32_t acceptedCapacity=4096); // 두 시각/ff 버퍼를 원문 순서로 만들고 안정된 노드를 소유한다.
    std::uint32_t Open(std::uint32_t port,std::uint32_t capacity,bool listener); // 일반 번호 선택/실제 열기/생성/공개/실패 반납을 공유한다.
    o::PacketDispatchHooks DispatchHooks(); // 실제 시각/방향 기록/종료/소멸/반납을 공통 분배기에 연결한다.
    ReceivedFrame Receive(Node& node,bool dispatch); // 현재 버퍼의 부분 헤더/본문과 observer/분배를 처리한다.
    void Accept(Node& node); // signed pendingCount<31 동안만 실제 수락/새 노드/음수 sender 공개를 반복한다.
    void Emit(Node& node,std::uint8_t type); // 현재 수신 버전 BYTE를 보존한1/2번 직접 통지다.
    void Destroy(Node& node); // killed/depth0 뒤 기존 집합을 flush하고 자기 버퍼/소켓을 반납한다.
    void Collect(); // 최외곽 대여가 없는 retired 물리만 지운다.
    void Flush(Node& node); // 소멸/일반 flush가 같은 버퍼 부모를 사용한다.
    void Send(Node& node,std::span<std::uint8_t> packet,const std::array<std::uint8_t,20>* destination); // 직접/집합/sendto의 observer·기록·오류 통지를 공유한다.
};
}
