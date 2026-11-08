// 같은 실제 연결을 등록/재배치/지연 반환하며 callback 예외의 prefix를 유지한다.
#include "o/RawConnectionRegistry.h"
#include "o/RawPacketCopy.h" //접속 초기 복제도 같은 전체4dece0 observer/길이/앞방향 복사 본체를 사용한다.
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //Client 저장소와 독립 검사의 새 저장소가 같은 본체를 사용한다.
// 현재 ID를 실제 표에서 읽고 없는 연결은 DNS/소켓 없이 같은 zero IPv4 주소를 만든다.
std::array<std::uint8_t,20> RawConnectionRegistry::Address(std::int32_t id) const {
    if (id< -32 || id>21) throw std::out_of_range("Chat 현재 주소의 원본 전송 ID 범위");const auto token=id<0 ? pending_.at(static_cast<std::size_t>(-id)) : transceivers_.at(static_cast<std::size_t>(id));
    if (token) return Node(token).address;std::array<std::uint8_t,20> address{};address[0]=2;return address;
}
void RawConnectionRegistry::BindSynchronization(std::uint32_t& value,std::shared_ptr<void> lifetime) { synchronization_=&value;synchronizationOwner_=std::move(lifetime); }
ConnectionCleanupGlobals RawConnectionRegistry::Cleanup() { return Cleanup(*synchronization_); }
ConnectionCleanupGlobals RawConnectionRegistry::Cleanup(std::uint32_t& synchronization) { return {transceivers_,buffers_,pending_,synchronization,pendingCount_}; }
std::uint32_t RawConnectionRegistry::Track(PacketConnection& connection,std::function<void()> destroy,std::function<void()> release,std::function<void(std::span<std::uint8_t>)> send,std::function<std::int32_t()> readiness) {
    if (!destroy || !release) throw std::invalid_argument("실제 연결 close/free 장치 누락");
    for (const auto& entry:bindings_) if (entry.second.node==&connection) throw std::logic_error("현재 연결의 중복 주소 등록"); //한 노드의 살아 있는 토큰을 중복 발급하지 않는다.
    if (!nextToken_) throw std::overflow_error("연결 opaque 토큰 소진");const auto token=nextToken_++;bindings_.emplace(token,Binding{&connection,std::move(destroy),std::move(release),std::move(send),std::move(readiness)});return token;
}
// 기존 연결 guard는 다른 backend의 같은 현재 노드도 실제 socket 조회로 판단한다.
std::int32_t RawConnectionRegistry::SocketReadiness(std::uint32_t token) const { const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.readiness) throw std::logic_error("현재 연결의 실제 socket 준비 장치 미연결");const auto query=found->second.readiness;return query(); }
//연결슬롯은Player관문보다먼저읽고가상송신이자기를반납해도callback이후같은노드를다시읽지않는다.
bool RawConnectionRegistry::SendMutable(std::int32_t id,std::span<std::uint8_t> packet,std::span<const std::uint8_t> players) const {
    if (id< -32 || id>21) throw std::out_of_range("원문 송신의 현재 transceiver 번호 범위");const auto token=id<0 ? pending_.at(static_cast<std::size_t>(-id)) : transceivers_.at(static_cast<std::size_t>(id));
    if (id>=1 && id<=8) { const auto offset=static_cast<std::size_t>(id)*0xb4+0x50;if (offset>=players.size()) throw std::out_of_range("원문 송신의 현재 Player 필드 범위");if (!(players[offset]&1U)) return false; }
    if (!token) return false;const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.send) throw std::logic_error("현재 살아 있는 송신기의 실제 가상+4 장치 미연결");const auto send=found->second.send;send(packet);return true;
}
bool RawConnectionRegistry::Owns(std::uint32_t token) const { return bindings_.contains(token); }
std::uint32_t RawConnectionRegistry::TokenFor(const PacketConnection& connection) const { for (const auto& entry:bindings_) if (entry.second.node==&connection) return entry.first;throw std::out_of_range("직접 송신의 현재 노드 token 누락"); } // 실제 살아 있는 동일 주소만 선택하며 sender 번호로 다른 노드를 대신하지 않는다.
void RawConnectionRegistry::SendDirect(std::uint32_t token,std::span<std::uint8_t> packet) const { const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.send) throw std::logic_error("직접 연결의 실제 가상 송신 장치 미연결");const auto send=found->second.send;send(packet); } // callback 중 해제에도 선택한 callable의 수명을 보유한다.
// 실제 접속 성공 뒤의 부분 효과만 수행하며 새 접속/Player 상태를 만들지 않는다.
void RawConnectionRegistry::StoreConnectPacket(std::int32_t id,std::span<std::uint8_t> packet,RawPacketDispatch& dispatch) {
    if (id<0 || id>=static_cast<std::int32_t>(buffers_.size())) throw std::out_of_range("접속 초기 패킷의 실제22칸 범위");auto& slot=buffers_[static_cast<std::size_t>(id)];if (slot) { Release(slot);slot=0; } // 기존 복제 반납 뒤에만 새 observer/할당을 수행한다.
    auto copy=DuplicateRawPacket(packet,0,{},dispatch);if (!copy.present) return;
    if (!nextToken_) throw std::overflow_error("접속 초기 패킷 opaque 토큰 소진");const auto token=nextToken_++;connectPackets_.emplace(token,std::move(copy.allocation));slot=token; //4dece0의 전체 복사가 끝난 뒤에만 현재 칸에 공개한다.
}
// 송신 callback이 현재 버퍼 칸을 바꿔도 원문처럼 free 단계에서 다시 읽는다.
void RawConnectionRegistry::SendConnectPacket(std::int32_t id) {
    if (id<0) return;if (id>=static_cast<std::int32_t>(buffers_.size())) throw std::out_of_range("접속 통지의 실제22칸 범위");auto& slot=buffers_[static_cast<std::size_t>(id)];if (!slot) return;const auto found=connectPackets_.find(slot);if (found==connectPackets_.end()) throw std::logic_error("접속 통지의 현재 복제 버퍼 물리 미연결");const auto packet=found->second;
    SendDirect(transceivers_[static_cast<std::size_t>(id)],*packet);if (slot) Release(slot);slot=0; // Player/현재ID 송신 관문을 반복하지 않는다. callback 중 반납된 복제 물리는 이 호출이 끝날 때까지만 C++ pin한다.
}
// 각 실제 backend가 같은 등록표를 빌릴 때 기존 bootstrap/사용자 수신기는 그대로 둔다.
void RegisterConnectPacketReceiver(RawPacketDispatch& dispatch,const std::shared_ptr<RawConnectionRegistry>& registry) {
    if (!registry) throw std::invalid_argument("접속 통지의 실제 Registry 없음");if (!dispatch.Registration(1).receive) dispatch.Register(1,[registry](auto sender,auto&,auto) { registry->SendConnectPacket(std::bit_cast<std::int32_t>(sender)); });
}
PacketConnection& RawConnectionRegistry::Node(std::uint32_t token) const { const auto it=bindings_.find(token);if (it==bindings_.end()) throw std::out_of_range("해제되거나 다른 owner의 연결 토큰");return *it->second.node; }
ConnectionRegistrationHooks RawConnectionRegistry::RegistrationHooks(std::function<void(std::uint32_t)> buffered) {
    return {[this](auto token)->PacketConnection& { return Node(token); },std::move(buffered),[](auto line) { throw std::logic_error("원문 연결 등록 진단:"+std::to_string(line)); }};
}
void RawConnectionRegistry::Register(std::int32_t id,std::uint32_t token) { static_cast<void>(Node(token));RegisterTransceiver(id,token,Cleanup(),RegistrationHooks()); }
// 원문 접속 생성자는 새 노드의 sender를 이미 쓴 뒤 기존 칸을 진단 없이 대체한다.
void RawConnectionRegistry::PublishConnect(std::int32_t id,std::uint32_t token) { static_cast<void>(Node(token));if (id<0 || id>=static_cast<std::int32_t>(transceivers_.size())) throw std::out_of_range("접속 공개의 실제22칸 범위");transceivers_[static_cast<std::size_t>(id)]=token; }
bool RawConnectionRegistry::CanAccept() const { return std::bit_cast<std::int32_t>(pendingCount_)<31; } //004e02f1의jge/4e038c의jl은signed비교이며 저장DWORD의unsigned감산과 구별한다.
std::int32_t RawConnectionRegistry::Accept(std::uint32_t token) {
    auto& connection=Node(token);if (!CanAccept()) throw std::logic_error("현재 pending 개수31 관문");
    for (std::size_t slot=1;slot<pending_.size();++slot) if (!pending_[slot]) { pending_[slot]=token;connection.sender=0U-static_cast<std::uint32_t>(slot);++pendingCount_;return -static_cast<std::int32_t>(slot); } //현재 첫 빈칸을 원문 순서로 선택한다.
    throw std::logic_error("pending 개수와 현재31칸의 불일치");
}
void RawConnectionRegistry::Assign(std::int32_t pending,std::int32_t target,std::uint32_t server,std::function<void(std::uint32_t)> buffered) { static_cast<void>(AssignPendingConnection(pending,target,server,Cleanup(),RegistrationHooks(std::move(buffered)))); }
void RawConnectionRegistry::Destroy(std::uint32_t token) { const auto found=bindings_.find(token);if (found==bindings_.end()) throw std::out_of_range("현재 연결 소멸 장치 누락");const auto operation=found->second.destroy;operation(); }
void RawConnectionRegistry::RemoveAliases(std::uint32_t token) {
    bool mainAlias{};for (auto& current:transceivers_) if (current==token) { current=0;mainAlias=true; } //현재 같은 양수 연결의 칸만 제거하며 새 연결은 지우지 않는다.
    if (mainAlias && *synchronization_==token) *synchronization_=0; //pending 종료는 원문처럼 동기화 토큰을 쓰지 않는다.
    for (std::size_t slot=1;slot<pending_.size();++slot) if (pending_[slot]==token) { pending_[slot]=0;--pendingCount_; } //일반 분배가 반납한 pending의 실제 현재 개수만 감소한다.
}
void RawConnectionRegistry::Release(std::uint32_t token) {
    if (const auto packet=connectPackets_.find(token);packet!=connectPackets_.end()) { connectPackets_.erase(packet);return; } // Native free는 노드 장치와 별개인 복제 버퍼도 반납한다. 현재 칸0 쓰기는 호출 부모가 담당한다.
    const auto found=bindings_.find(token);if (found==bindings_.end()) throw std::out_of_range("현재 연결/송신버퍼 free 장치 누락");const auto operation=found->second.release;operation();
    if (!cleanupDepth_) RemoveAliases(token);bindings_.erase(token); //실제 free 성공 전에 표/장치를 지우지 않으며 Native 부모는 자신의 store를 직접 마친다.
}
void RawConnectionRegistry::Forget(std::uint32_t token) { if (!token) return;RemoveAliases(token);bindings_.erase(token); } //기존 호스트 종료에서 이미 없어지는 자기 노드의 dangling 별칭만 반납한다.
ConnectionCleanupHooks RawConnectionRegistry::CleanupHooks() { return {[this](auto token)->PacketConnection& { return Node(token); },[this](auto token) { Destroy(token); },[this](auto token) { Release(token); },[](auto line) { throw std::logic_error("원문 연결 정리 진단:"+std::to_string(line)); }}; }
void RawConnectionRegistry::RunCleanup(const std::function<void()>& operation) { ++cleanupDepth_;try { operation(); }catch (...) { --cleanupDepth_;throw; }--cleanupDepth_; }
void RawConnectionRegistry::Disconnect(std::int32_t id) { RunCleanup([&] { DisconnectTransceiver(id,Cleanup(),CleanupHooks()); }); }
void RawConnectionRegistry::CloseAll() { RunCleanup([&] { CloseAllTransceivers(Cleanup(),CleanupHooks()); }); }
void RawConnectionRegistry::EndPlayers(std::uint32_t server) { RunCleanup([&] { EndPlayerConnections(server,Cleanup(),CleanupHooks()); }); }
void RawConnectionRegistry::ClosePending() { RunCleanup([&] { CloseAllPendingConnections(Cleanup(),CleanupHooks()); }); }
}
