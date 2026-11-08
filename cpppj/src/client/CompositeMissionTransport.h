// 내부 자기끝과 실제 TCP 원격끝을 원문 하나의 Registry/등록표/ID 순서로 조립한다.
#pragma once
#include "client/LocalTcpTransport.h"
#include "client/EditorLocalTransport.h"
#include "o/PacketRegistrationTable.h"
#include "client/MissionNetwork.h" //메타에서 인계한 실제 OS owner/등록표 수명을 같은 coordinator에 결합한다.

namespace netstorm::client { // UI/roster/일반 State는 이 전송 owner의 바깥에서 원문대로 작성한다.
// 전송 경로만 선택하며 server/boss/Player/Ready 값을 새로 쓰지 않는다.
enum class CompositeTransportRole { InternalMission,Editor,LanHost,LanClient };
// 동기 API/콜백 중 자기 owner가 사라지지 않게 shared 생성만 허용한다.
class CompositeMissionTransport final:public std::enable_shared_from_this<CompositeMissionTransport> {
public:
    // globalsLifetime은 players/server/hooks가 빌리는 Client 물리 전체를 실제로 유지해야 한다.
    static std::shared_ptr<CompositeMissionTransport> Create(CompositeTransportRole role,std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry,std::shared_ptr<void> globalsLifetime,MissionNetwork network={});
    ~CompositeMissionTransport(); // backend가 실제 자기 노드/소켓만 반납한다. Player/등록 성공을 합성하지 않는다.
    CompositeMissionTransport(const CompositeMissionTransport&)=delete; // 현재 노드/대여 수명을 복제하지 않는다.
    CompositeMissionTransport& operator=(const CompositeMissionTransport&)=delete; // 같은 장기 표 주소를 대입하지 않는다.
    void Serve(std::uint32_t port); // 실제 서버 권한이 새로 주어진 LAN client도 같은 TCP/등록표를 유지해 host로 전환하고 두 listener를 연다. accept/client9는 뒤 단계다.
    void ListenLan(std::uint32_t port,std::string_view bindAddress="0.0.0.0"); // simulconnect의 LAN client도 실제 TCP listener만 명시 열 수 있다. 게임 서버 권한을 쓰지 않는다.
    void Connect(std::span<const std::uint8_t> address,std::uint32_t port); // host 자기끝/단독은 내부 client9, LAN client는 실제20BYTE TCP다.
    void ConnectPeerText(std::string_view address,std::uint32_t port); // 원문 저장 상태의 문자 주소는 선택한 같은 backend에 전달하고 현재 포트를 적용한다.
    void Assign(std::int32_t pending,std::int32_t owner); // 현재 accepted 노드를 실제 소유 backend에서 재배치한다.
    bool Send(std::int32_t target,std::span<const std::uint8_t> packet); // 현재 ID의 실제 owner만 선택한다.
    bool SendMutable(std::int32_t target,std::span<std::uint8_t> packet); // 선택 backend에서 observer1을 한 번만 실행한다.
    void Flush(); //004e2920의 현재0..21 순서로 같은 ID의 실제 backend만 flush한다.
    std::size_t Poll(); //004e28b0의 listener→pending각1회→정상0..21 반복을 한 번만 조립한다.
    void Close(std::int32_t id); // 같은 Registry의 실제 close/free 장치로 원문 개별 종료한다.
    void CloseRoom(); // 자기 listener/노드만 닫고 다른 owner의 공유 표는 보존한다.
    o::PacketConnection* Connection(std::int32_t id) const; // 현재 자기 노드만 반환하며 미확인 owner는 거부한다.
    bool Exists(std::int32_t id) const; // 객체 존재이며 buffered/open/Ready와 구별한다.
    bool IsOpen(std::int32_t id) const; // 실제 backend의 하위 수명과 현재 closed를 읽는다.
    std::int32_t Status(std::int32_t id) const; // 실제 접속 완료/진행/실패이며 bit2를 쓰지 않는다.
    std::array<std::uint8_t,20> Address(std::int32_t id) const; // 같은 Registry의 현재 주소를 읽는다.
    o::BattlePacketSend Peer(std::uint32_t owner); // 매 송신마다 같은 현재 ID를 다시 조회한다.
    o::RawPacketDispatch& Dispatch(); // 모든 backend가 공유하는 등록표의 canonical 편집 API다.
    const std::shared_ptr<o::RawConnectionRegistry>& Registry() const; // State/Chat/cleanup에 같은 현재 표를 빌린다.
    std::shared_ptr<o::PacketRegistrationLease> BindSession(const std::shared_ptr<void>& owner); // raw this만 아닌 실제 Session/World 전체 owner가 필요하다.
    bool ConnectionCallbackActive() const noexcept; // detached 콜백까지 포함하여 전체 State/Kernel 정리의 진입을 막는다.
    const std::shared_ptr<EditorLocalTransport>& EditorTransport() const; // 기존 Editor services도 같은 실제 내부 owner를 그대로 빌릴 수 있다.
    CompositeTransportRole Role() const; // UI/원문 State가 이미 선택한 전송 경로만 조회한다.
    bool SharesNetwork(const MissionNetwork& network) const; //실제 같은 TCP/UDP/두 표/전체 Meta owner인지 확인해 다른 World의 정리를 허용하지 않는다.
    void RebindWorld(CompositeTransportRole role,std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<void> globalsLifetime); //등록 반납 뒤 같은 coordinator/모든 기존 backend를 새 World에 인계한다.
private:
    // 정적 Create만 실제 장기 owner를 만들며 생성 자체는 소켓/게임을 실행하지 않는다.
    CompositeMissionTransport(CompositeTransportRole role,std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry,std::shared_ptr<void> globalsLifetime,MissionNetwork network);
    // 현재 token의 실제 backend를 구별한다. Unknown은 다른 외부 owner여서 dispatch하지 않는다.
    enum class Backend { Missing,Internal,Editor,Tcp,Udp,Unknown }; //현재12도 같은 실제 UDP 노드로 선택한다.
    Backend Select(std::int32_t id) const; // 매 callback 뒤의 현재 Registry를 새로 읽는다.
    o::TransportPollResult ReceiveOne(std::int32_t id); // 선택 backend의 한 실제 프레임만 받는다.
    CompositeTransportRole role_; // API 호출이 다른 역할을 조용히 성공시키지 않게 하는 선택값이다.
    const bool& server_; // 원문 현재 서버 권한이며 값을 강제로 변경하지 않는다.
    std::span<const std::uint8_t> players_;o::PacketDispatchHooks dispatchHooks_; // 서버 이전 때 같은 실제 Player/장치로 내부 끝을 늦게 만들며 별도 등록표를 초기화하지 않는다.
    std::shared_ptr<void> globalsLifetime_; // Player/server/시계·진단 hooks의 실제 장기 물리를 유지한다.
    std::shared_ptr<o::RawConnectionRegistry> registry_; // 모든 backend와 State가 함께 쓰는 고정 표다.
    std::shared_ptr<o::PacketRegistrationTable> registrations_; // 한 번만 초기화한 실제 전역 등록표다.
    std::shared_ptr<void> inheritedNetwork_; //인계한 전체 메타 callback owner는 실제 backend보다 먼저 반납하지 않는다.
    std::shared_ptr<zacket::LocalMissionTransport> internal_; // 단독/LAN host 자기끝의 실제 내부 listener/client/accepted다.
    std::shared_ptr<EditorLocalTransport> editor_; // Editor 역할에서만 존재하는 실제 unbuffered 내부 owner다.
    std::shared_ptr<LocalTcpTransport> tcp_; // LAN 역할에서만 존재하는 실제 OS TCP owner다.
    std::shared_ptr<LocalUdpTransport> udp_; //메타에서 현재12를 인계받은 경우 같은 전역ID 순회에 포함한다.
    bool polling_{}; // coordinator 자체의 재진입을 막아 같은 ID를 중복 소비하지 않는다.
};
}
