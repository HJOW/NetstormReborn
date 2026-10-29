// 같은 실제 연결을 등록/재배치/지연 반환하며 callback 예외의 prefix를 유지한다.
#include "o/RawConnectionRegistry.h"
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
std::uint32_t RawConnectionRegistry::Track(PacketConnection& connection,std::function<void()> destroy,std::function<void()> release) {
    if (!destroy || !release) throw std::invalid_argument("실제 연결 close/free 장치 누락");
    for (const auto& entry:bindings_) if (entry.second.node==&connection) throw std::logic_error("현재 연결의 중복 주소 등록"); //한 노드의 살아 있는 토큰을 중복 발급하지 않는다.
    if (!nextToken_) throw std::overflow_error("연결 opaque 토큰 소진");const auto token=nextToken_++;bindings_.emplace(token,Binding{&connection,std::move(destroy),std::move(release)});return token;
}
bool RawConnectionRegistry::Owns(std::uint32_t token) const { return bindings_.contains(token); }
PacketConnection& RawConnectionRegistry::Node(std::uint32_t token) const { const auto it=bindings_.find(token);if (it==bindings_.end()) throw std::out_of_range("해제되거나 다른 owner의 연결 토큰");return *it->second.node; }
ConnectionRegistrationHooks RawConnectionRegistry::RegistrationHooks(std::function<void(std::uint32_t)> buffered) {
    return {[this](auto token)->PacketConnection& { return Node(token); },std::move(buffered),[](auto line) { throw std::logic_error("원문 연결 등록 진단:"+std::to_string(line)); }};
}
void RawConnectionRegistry::Register(std::int32_t id,std::uint32_t token) { static_cast<void>(Node(token));RegisterTransceiver(id,token,Cleanup(),RegistrationHooks()); }
bool RawConnectionRegistry::CanAccept() const { return pendingCount_<31; }
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
