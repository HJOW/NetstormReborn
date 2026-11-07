// 전송 별 전체 Poll을 이어 붙이지 않고 현재 전역 ID별 실제 하위 호출만 선택한다.
#include "client/CompositeMissionTransport.h"
#include <exception>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 서버 roster/미션/State 성공은 실제 상위 원문 소비자가 결정한다.
// caller가 공유 owner를 받기 전에는 어떤 listener나 client도 만들지 않는다.
std::shared_ptr<CompositeMissionTransport> CompositeMissionTransport::Create(CompositeTransportRole role,std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry,std::shared_ptr<void> globalsLifetime) {
    return std::shared_ptr<CompositeMissionTransport>(new CompositeMissionTransport(role,players,server,std::move(hooks),std::move(registry),std::move(globalsLifetime)));
}
// 실제 Player9/Server/Registry owner를 확인하고 backend는 같은 표만 빌린다.
CompositeMissionTransport::CompositeMissionTransport(CompositeTransportRole role,std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry,std::shared_ptr<void> globalsLifetime)
    :role_(role),server_(server),players_(players),dispatchHooks_(hooks),globalsLifetime_(std::move(globalsLifetime)),registry_(std::move(registry)),registrations_(std::make_shared<o::PacketRegistrationTable>(o::OriginalEdition::Patch1078)) {
    if (!globalsLifetime_ || !registry_ || players.size()!=9U*180U) throw std::invalid_argument("복합 전송의 실제 Client 물리/Registry/Player9 누락");
    if (role_!=CompositeTransportRole::InternalMission && role_!=CompositeTransportRole::Editor && role_!=CompositeTransportRole::LanHost && role_!=CompositeTransportRole::LanClient) throw std::invalid_argument("지원하지 않는 복합 전송 역할");
    if (role_==CompositeTransportRole::Editor) editor_=std::make_shared<EditorLocalTransport>(players,server_,hooks,registry_,registrations_);
    else if (role_!=CompositeTransportRole::LanClient) internal_=std::make_shared<zacket::LocalMissionTransport>(players,server_,hooks,registry_,registrations_);
    if (role_==CompositeTransportRole::LanHost || role_==CompositeTransportRole::LanClient) tcp_=std::make_shared<LocalTcpTransport>(players,server_,std::move(hooks),registry_,registrations_);
}
// 노드/OS 핸들 반납은 backend의 같은 Registry 수명 장치가 수행한다.
CompositeMissionTransport::~CompositeMissionTransport()=default;
// LAN host도 실제 서버 권한과 준비된 등록만 사용하고 게임 상태를 쓰지 않는다.
void CompositeMissionTransport::Serve(std::uint32_t port) {
    const auto pin=shared_from_this();
    if (role_==CompositeTransportRole::LanClient) {
        if (!server_) throw std::logic_error("서버 권한 없는 LAN client의 listener 요청");if (ConnectionCallbackActive()) throw std::logic_error("수신 callback 중 LAN 서버 역할 전환 거부");
        ListenLan(port); // State15의 실제 TCP listener 효과를 내부 끝 생성보다 먼저 보존한다. 뒤 할당 실패가 이 성공을 되돌리지 않는다.
        internal_=std::make_shared<zacket::LocalMissionTransport>(players_,server_,dispatchHooks_,registry_,registrations_);role_=CompositeTransportRole::LanHost; // 같은 Client/World/Player/Registry/등록 대여를 유지하며 server/boss/Ready는 쓰지 않는다.
    }
    if (editor_) { editor_->Open(port);return; }
    if (!server_) throw std::logic_error("서버 권한 없는 내부/LAN host listener 요청");
    if (tcp_) ListenLan(port); // 원문 TCP listener를 먼저 만들고 실제 수락은 다음 Poll에 남긴다.
    if (!internal_->IsOpen()) internal_->Open(port,false); //004e2430의 같은 내부 listener도 기존 존재를 유지한다. 뒤 생성 실패가 앞서 열린 TCP를 되돌리지 않는다.
}
// 원문 simulconnect Serve가 비서버에서도 선택할 수 있는 실제 수신 listener만 연다.
void CompositeMissionTransport::ListenLan(std::uint32_t port,std::string_view bindAddress) {
    const auto pin=shared_from_this();if (!tcp_) throw std::logic_error("LAN 역할 아닌 전송의 TCP listener 요청");
    if (tcp_->IsListening()) return; //004e2430은 전역 listener가 이미 존재하면 다시 만들지 않는다. simulconnect 뒤 Zone Serve도 같은 실제 listener를 유지한다.
    tcp_->Listen(port,bindAddress);
}
// host 자기 로그인은 내부 끝을 만들고 원격 client만 실제 TCP 주소/포트를 사용한다.
void CompositeMissionTransport::Connect(std::span<const std::uint8_t> address,std::uint32_t port) {
    const auto pin=shared_from_this();if (address.size()!=20) throw std::out_of_range("현재 State8 주소20BYTE 누락");
    if (role_==CompositeTransportRole::LanClient) { tcp_->Connect(address,port,9);return; }
    if (!internal_) throw std::logic_error("Editor 내부 끝은 Open에서 생성되며 Login Connect가 없습니다");internal_->Connect();
}
// 저장 상태의 문자열을 binary sockaddr로 가장하지 않고 같은 현재 역할의 실제 연결을 연다.
void CompositeMissionTransport::ConnectPeerText(std::string_view address,std::uint32_t port) {
    const auto pin=shared_from_this();if (role_==CompositeTransportRole::LanClient) { tcp_->ConnectPeerText(address,port,9);return; }
    if (!internal_) throw std::logic_error("Editor 전송에 재접속 Login 경로 없음");internal_->Connect(); // 명시 내부 역할은 기존 zlocal 끝을 사용하며TCP/DNS를 만들지 않는다.
}
// 실제 현재 token이 있는 backend 한 곳만 선택하며 슬롯을 초기화하지 않는다.
CompositeMissionTransport::Backend CompositeMissionTransport::Select(std::int32_t id) const {
    if (!o::CurrentTransportToken(*registry_,id)) return Backend::Missing;
    if (tcp_ && tcp_->HasConnection(id)) return Backend::Tcp;
    if (internal_ && internal_->Connection(id)) return Backend::Internal;
    if (editor_ && editor_->Connection(id)) return Backend::Editor;return Backend::Unknown;
}
// 현재 accepted 노드에만 원문 Assign을 실행하며 backend 사유 pending 번호를 만들지 않는다.
void CompositeMissionTransport::Assign(std::int32_t pending,std::int32_t owner) {
    const auto pin=shared_from_this();switch (Select(pending)) {
    case Backend::Tcp:tcp_->AssignConnection(pending,owner);return;
    case Backend::Internal:internal_->AssignConnection(pending,owner);return;
    default:throw std::logic_error("현재 accepted 노드 없는 복합 전송 재배치");
    }
}
// 이미 길이를 확정한 writer의 바이트는 실제 선택 backend가 그대로 보낸다.
bool CompositeMissionTransport::Send(std::int32_t target,std::span<const std::uint8_t> packet) {
    const auto pin=shared_from_this();const auto selected=Select(target);if (selected==Backend::Missing) return false;
    if (packet.size()<4) throw std::out_of_range("복합 송신의 원문 헤더 부족");const auto entry=Dispatch().Registration(packet[1]);const auto sessionPin=entry.Pin(); // callback이 caller의 packet 저장소를 가진 Session을 반납해도 송신 복귀까지 유지한다.
    switch (selected) {
    case Backend::Missing:return false;
    case Backend::Tcp:return tcp_->Send(target,packet);
    case Backend::Internal:return internal_->Send(target,packet);
    case Backend::Editor:return editor_->Send(target,packet);
    default:throw std::logic_error("복합 전송이 소유하지 않은 현재 송신 노드");
    }
}
// observer1과 현재 gate/큐는 선택 backend에서 한 번 실행한다.
bool CompositeMissionTransport::SendMutable(std::int32_t target,std::span<std::uint8_t> packet) {
    const auto pin=shared_from_this();const auto selected=Select(target);if (selected==Backend::Missing) return false;
    if (packet.size()<4) throw std::out_of_range("복합 mutable 송신의 원문 헤더 부족");const auto entry=Dispatch().Registration(packet[1]);const auto sessionPin=entry.Pin(); // observer 뒤 caller 버퍼를 읽는 전체 실제 송신 구간을 pin한다.
    switch (selected) {
    case Backend::Missing:return false;
    case Backend::Tcp:return tcp_->SendMutable(target,packet);
    case Backend::Internal:return internal_->SendMutable(target,packet);
    case Backend::Editor:return editor_->SendMutable(target,packet);
    default:throw std::logic_error("복합 전송이 소유하지 않은 현재 mutable 송신 노드");
    }
}
//004e2920의 각 현재 정상 ID에서 실제 자기 owner의 가상 flush만 선택한다.
void CompositeMissionTransport::Flush() {
    const auto pin=shared_from_this();
    for (std::int32_t id=0;id<static_cast<std::int32_t>(o::kTransceiverSlots);++id) { // callback 뒤 다음 ID의 현재 token을 새로 읽는다.
        switch (Select(id)) {
        case Backend::Tcp:tcp_->FlushOne(id);break;
        case Backend::Internal:internal_->FlushOne(id);break;
        case Backend::Missing:case Backend::Editor:break; // Editor unbuffered 큐에는 확정 대기 집합이 없고 송신 때 완성 프레임을 이미 보관한다.
        default:throw std::logic_error("복합 전송이 소유하지 않은 현재 flush 노드");
        }
    }
}
// 현재 ID의 한 프레임만 실제 backend에 요청한다. RawPacketDispatch proxy 호출은 없다.
o::TransportPollResult CompositeMissionTransport::ReceiveOne(std::int32_t id) {
    switch (Select(id)) {
    case Backend::Tcp:return tcp_->ReceiveOne(id);
    case Backend::Internal:return internal_->ReceiveOne(id);
    case Backend::Editor:return editor_->ReceiveOne(id);
    case Backend::Missing:return {};
    default:throw std::logic_error("복합 전송이 소유하지 않은 현재 수신 노드");
    }
}
// 최초 listener 수락 후 pending은 각1회, 정상은 ID별 sentinel까지 반복한다.
std::size_t CompositeMissionTransport::Poll() {
    const auto pin=shared_from_this();if (polling_) throw std::logic_error("복합 전송 Poll 중첩 호출");polling_=true;bool tcpBegun=false,localBegun=false,editorBegun=false;std::size_t count{};
    // backend 종료가 예외를 보고해도 coordinator의 진입 guard 자체는 남기지 않는다.
    struct ResetPolling {
        bool& flag; // 같은 coordinator의 현재 호출 guard다.
        ~ResetPolling() { flag=false; } // backend 종료가 실패해도 다음 진입을 영구 봉쇄하지 않는다.
    } reset{polling_};
    try {
        if (tcp_) { tcp_->BeginPoll();tcpBegun=true; }if (internal_) { internal_->BeginPoll();localBegun=true; }if (editor_) { editor_->BeginPoll();editorBegun=true; }
        if (internal_) internal_->AcceptPending();if (editor_) editor_->AcceptPending();if (tcp_) tcp_->AcceptPending(); //004e28b0은5e4a10의zlocal→5e4a08의TCP→생략SPX 순서다. 같은 프레임의 첫 pending 선택도 이 순서를 따른다.
        for (std::int32_t id=-1;id>-static_cast<std::int32_t>(o::kPendingConnectionSlots);--id) { const auto result=ReceiveOne(id);if (result.consumed) ++count; } //004df560의 현재 각 pending 한 번이다.
        for (std::int32_t id=0;id<static_cast<std::int32_t>(o::kTransceiverSlots);++id) { for (;;) { const auto result=ReceiveOne(id);if (result.consumed) ++count;if (!result.again) break; } } // callback의 재배치/반납을 매 회 현재 표에서 반영한다.
    }catch (...) {
        const auto failure=std::current_exception(); // 원래 수신 실패를 보존하고 모든 backend 대여를 먼저 반납한다.
        if (editorBegun) { try { editor_->EndPoll(); }catch (...) {} } // 원래 예외가 우선이며 나머지 backend 정리를 계속한다.
        if (localBegun) { try { internal_->EndPoll(); }catch (...) {} } // 고정 내부 끝은 noexcept지만 정리 순서를 명시한다.
        if (tcpBegun) { try { tcp_->EndPoll(); }catch (...) {} } // 실제 TCP 조기 수거 진단이 다른 backend 대여를 남기지 않는다.
        std::rethrow_exception(failure);
    }
    std::exception_ptr failure; // 정상 수신 뒤 정리 실패는 모든 backend 반납을 시도한 후 첫 원인으로 전파한다.
    if (editorBegun) { try { editor_->EndPoll(); }catch (...) { failure=std::current_exception(); } }
    if (localBegun) { try { internal_->EndPoll(); }catch (...) { if (!failure) failure=std::current_exception(); } }
    if (tcpBegun) { try { tcp_->EndPoll(); }catch (...) { if (!failure) failure=std::current_exception(); } }
    if (failure) std::rethrow_exception(failure);return count;
}
// 기존 Native Registry가 현재 ID/버퍼/동기화/깊이와 실제 owner close/free를 처리한다.
void CompositeMissionTransport::Close(std::int32_t id) { const auto pin=shared_from_this();registry_->Disconnect(id); }
// 방 종료는 자기 실제 backend의 장치만 반납하며 공유 Registry 전체를0으로 만들지 않는다.
void CompositeMissionTransport::CloseRoom() {
    const auto pin=shared_from_this();
    for (std::int32_t id=0;id<static_cast<std::int32_t>(o::kTransceiverSlots);++id) { const auto backend=Select(id);if (backend!=Backend::Missing && backend!=Backend::Unknown) registry_->Disconnect(id); } // 현재 자기 정상 노드만 원문 번호순 Native 종료한다.
    for (std::int32_t id=-1;id>-static_cast<std::int32_t>(o::kPendingConnectionSlots);--id) { const auto backend=Select(id);if (backend!=Backend::Missing && backend!=Backend::Unknown) registry_->Disconnect(id); } // 현재 자기 pending의 실제 개수/깊이는 같은 Registry 부모가 처리한다.
    if (tcp_) tcp_->CloseAll();if (internal_) internal_->CloseAll(); // 별칭 없이 depth 복귀를 기다리는 자기 노드/실제 listener만 마무리한다.
    if (editor_) editor_->CloseListener();
}
// ID가 현재 자기 노드인지 확인한 뒤에만 같은 고정 주소를 짧게 빌린다.
o::PacketConnection* CompositeMissionTransport::Connection(std::int32_t id) const {
    switch (Select(id)) { case Backend::Missing:return nullptr;case Backend::Tcp:return tcp_->Connection(id);case Backend::Internal:return internal_->Connection(id);case Backend::Editor:return editor_->Connection(id);default:throw std::logic_error("다른 owner의 복합 연결 주소 조회"); }
}
// 존재만 읽고 접속 완료/Ready를 만들지 않는다.
bool CompositeMissionTransport::Exists(std::int32_t id) const { return Connection(id)!=nullptr; }
// 하위 socket/상대 종료/상위closed를 실제 선택 backend에서 읽는다.
bool CompositeMissionTransport::IsOpen(std::int32_t id) const {
    switch (Select(id)) { case Backend::Missing:return false;case Backend::Tcp:return tcp_->IsOpen(id);case Backend::Internal:return internal_->ConnectionOpen(id);case Backend::Editor:{ const auto* node=editor_->Connection(id);return node && !(node->flags&0x40U); }default:throw std::logic_error("다른 owner의 복합 연결 open 조회"); }
}
// TCP의 OS status와 내부 실제 즉시 완료를 구별하며 bit2는 사용하지 않는다.
std::int32_t CompositeMissionTransport::Status(std::int32_t id) const {
    switch (Select(id)) { case Backend::Missing:return 0;case Backend::Tcp:return tcp_->Status(id);case Backend::Internal:return internal_->ConnectionStatus(id);case Backend::Editor:return IsOpen(id) ? 1:0;default:throw std::logic_error("다른 owner의 복합 연결 status 조회"); }
}
// 반환형은 같은 원문20BYTE이며 IPv4/포트의 바이트 순서를 고치지 않는다.
std::array<std::uint8_t,20> CompositeMissionTransport::Address(std::int32_t id) const { static_cast<void>(Connection(id));return registry_->Address(id); }
// 실제 호출 시 현재 표를 재조회하므로 예전 backend/노드 주소를 보관하지 않는다.
o::BattlePacketSend CompositeMissionTransport::Peer(std::uint32_t owner) { return [self=shared_from_this(),owner](auto packet) { static_cast<void>(self->Send(static_cast<std::int32_t>(owner),packet)); }; }
// 어느 backend API로 등록해도 같은 실제 table에 쓰게 한 canonical facade다.
o::RawPacketDispatch& CompositeMissionTransport::Dispatch() { if (internal_) return internal_->Dispatch();if (editor_) return editor_->Dispatch();return tcp_->Dispatch(); }
// 현재 공유 표의 정체성은 Session/State가 그대로 확인한다.
const std::shared_ptr<o::RawConnectionRegistry>& CompositeMissionTransport::Registry() const { return registry_; }
// 대여는 실제 Session/World 전체 owner를 약하게 빌리고 콜백 때만 강하게 pin한다.
std::shared_ptr<o::PacketRegistrationLease> CompositeMissionTransport::BindSession(const std::shared_ptr<void>& owner) { return registrations_->BindSession(owner); }
// 단순 shared World 생존과 별개로 현재 Kernel/Player 물리를 지우는 정리를 막는다.
bool CompositeMissionTransport::ConnectionCallbackActive() const noexcept { return registrations_->ConnectionCallbackActive(); }
// Editor만 기존 특별 설치가 요구하는 실제 내부 전송 owner를 제공한다.
const std::shared_ptr<EditorLocalTransport>& CompositeMissionTransport::EditorTransport() const { if (!editor_) throw std::logic_error("Editor 역할 아닌 복합 전송의 Editor owner 요청");return editor_; }
// 게임 권한을 쓰지 않는 역할 조회다.
CompositeTransportRole CompositeMissionTransport::Role() const { return role_; }
}
