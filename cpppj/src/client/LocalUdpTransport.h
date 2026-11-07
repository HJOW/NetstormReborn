// 원문 UDP12의 데이터그램·실제 송신자 주소·지연 반납을 같은 연결표/패킷표에 연결한다.
#pragma once
#include "zacket/LocalMissionTransport.h"
#include <list>
#include <memory>
#include <optional>

namespace netstorm::client { // TCP와 별도 OS 장치를 가지며 같은 Client의 표와 실제 Player를 빌린다.
class LocalUdpTransport final {
public:
    LocalUdpTransport(std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry,std::shared_ptr<o::PacketRegistrationTable> registrations); //생성은소켓을열지않으며같은현재표두개와원문분배효과를필수로받는다.
    ~LocalUdpTransport(); //자기UDP노드/소켓/WSA참조만반납한다.다른backend의현재슬롯을지우지않는다.
    LocalUdpTransport(const LocalUdpTransport&)=delete;LocalUdpTransport& operator=(const LocalUdpTransport&)=delete; //고정노드와callback의this주소를복제하지않는다.
    bool Open(std::uint32_t port); //004e2bd0의현재UDP12를새로열어공개한다.실제OS실패는false/LastError이며검색성공을쓰지않는다.
    void Close(); //004e2d20의현재12만닫고callback깊이가남은물리는분배복귀까지보존한다.
    bool Broadcast(std::span<std::uint8_t> packet,std::uint32_t port); //004e00e0의observer1→선언WORD재조회→sendto→짧으면2통지순서다.대상은IPv4로컬브로드캐스트뿐이다.
    std::size_t Poll(); //현재12의실제데이터그램을하나씩검증/observer0/공통분배하며sentinel이면이번순회를끝낸다.
    std::array<std::uint8_t,20> LocalAddress(); //004e2d60/004e1040의자기hostname첫IPv4다.폐기도메인조회나localhost성공값으로대체하지않는다.
    std::int32_t LastError() const; //마지막실제Winsock실패코드이며성공상태를가장하지않는다.
private:
    struct Node; //실제소켓/4096BYTE현재수신물리/안정된PacketConnection을함께소유한다.
    static constexpr std::uintptr_t kInvalidSocket=~std::uintptr_t{}; //Windows SOCKET의INVALID_SOCKET을헤더없이표현한다.
    void StartSockets(); //첫Open/자기주소조회에서만현재WSA참조를얻는다.
    o::PacketDispatchHooks DispatchHooks(); //같은등록표에UDP고유시계/기록/종료/실제close/free를결합한다.
    Node* Find(std::uint32_t token) const; //자기생존숫자토큰만조회한다.호스트포인터를DWORD로자르지않는다.
    Node* Find(const o::PacketConnection& connection) const; //반납대기중에도같은물리주소를식별한다.
    Node* Current() const; //현재12가자기노드일때만반환한다.옛세대/다른backend는선택하지않는다.
    o::TransportPollResult ReceiveOne(Node& node); //004dfc00의데이터그램크기/4096상한/검증/observer/수신순서다.
    bool SendNode(Node& node,std::span<std::uint8_t> packet,std::optional<std::uint32_t> broadcastPort); //가상+4의send와가상+8의sendto를구별한다.UDP일반send의실제OS실패도원문처럼종료통지한다.
    void Fail(Node& node,std::int32_t error); //004df9f0의마지막version을보존한2번통지만한번수행한다.
    void CloseSocket(Node& node); //현재실제OS핸들을한번반납한다.실패시원래코드를보존한다.
    void CloseReady(Node& node); //closed/depth0일때만같은Registry의소멸/반납을완료한다.
    void CollectRetired(); //최외곽분배대여가끝난노드만물리수거한다.
    o::PacketDispatchHooks hooks_; //실제상위시계/선택적기록효과이며UDP송수신의성공값이아니다.
    std::shared_ptr<o::RawConnectionRegistry> registry_; //TCP10/11과같은현재12슬롯/주소/수명소유자다.
    zacket::LocalMissionTransport dispatcher_; //Open하지않고기존단일/집합분배와같은등록표만재사용한다.
    std::list<std::unique_ptr<Node>> nodes_; //재공개전옛노드와callback대여노드의안정된물리를보존한다.
    std::int32_t lastError_{}; //이장치의실제마지막OS실패다.
    std::size_t activeCalls_{};bool polling_{},socketsStarted_{}; //동기callback/폴링/WSA참조수명을구별한다.
};
}
