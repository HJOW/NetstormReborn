// 같은 실제 연결을 등록/재배치/지연 반환하며 callback 예외의 prefix를 유지한다.
#include "o/RawConnectionRegistry.h"
#include "o/RawPacketCopy.h" //접속 초기 복제도 같은 전체4dece0 observer/길이/앞방향 복사 본체를 사용한다.
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //Client 저장소와 독립 검사의 새 저장소가 같은 본체를 사용한다.
namespace { // Native가 선택한 원문 단언만 같은 Client 장치로 전달한다.
// 진단 callback의 재연결 중에도 선택된 callable 수명을 보유하며 정상 복귀 뒤 caller가 계속한다.
void AssertConnectionNative(const ConnectionRegistryAssertion& assertion,std::uint32_t line) {
    std::string_view expression; // 줄마다 실제 PE 문자열을 사용하며 임의 공통 실패로 바꾸지 않는다.
    switch (line) {
    case 0x1e:expression="-zaMAX_PENDING <= id && id < NUM_TRANSCEIVERS";break;
    case 0xb0:expression="FIRST_REAL_PLAYER <= transId && transId < END_REAL_PLAYERS";break;
    case 0xb3:expression="transId == GAME_SERVER_ID";break;
    case 0xb6:expression="transceivers[transId] == NULL";break;
    case 0xbb:expression="pendingId < 0";break;
    case 0xfa:expression="id < NUM_TRANSCEIVERS";break;
    default:throw std::invalid_argument("Native 연결의 미등록 원문 진단 행");
    }
    const auto diagnose=assertion;if (!diagnose) throw std::logic_error("Native 연결의 선택된 Client 진단 미연결");
    diagnose(expression,"\\Ns\\O\\Ztrans.cpp",line);
}
}
// 현재 ID를 실제 표에서 읽고 없는 연결은 DNS/소켓 없이 같은 zero IPv4 주소를 만든다.
std::array<std::uint8_t,20> RawConnectionRegistry::Address(std::int32_t id) const {
    if (id< -32 || id>21) throw std::out_of_range("Chat 현재 주소의 원본 전송 ID 범위");const auto token=id<0 ? pending_.at(static_cast<std::size_t>(-id)) : transceivers_.at(static_cast<std::size_t>(id));
    if (token) return Node(token).address;std::array<std::uint8_t,20> address{};address[0]=2;return address;
}
//실제Client의weak경로getter만연결하며생성/연결/게임값을바꾸지않는다.
void RawConnectionRegistry::BindDevicePin(std::function<std::shared_ptr<void>(const PacketConnection&)> pin,bool retainNodes) { devicePin_=std::move(pin);retainDeviceOwners_=retainNodes; }
//표에서이미선택한node만사용하고getter/실제owner를이번가상효과복귀까지만유지한다.
std::shared_ptr<void> RawConnectionRegistry::PinBinding(const Binding& binding) const {
    const auto select=devicePin_;if (!select && !binding.deviceOwner) return {};if (!binding.node) throw std::logic_error("현재Binding의실제node없음");
    auto owner=select ? select(*binding.node):binding.deviceOwner;if (!owner) throw std::logic_error("현재Binding의실제Source수명없음");
    if (deviceCallDepth_) { //실제 현재 coordinator와 등록 당시 Source를 부모의 마지막 slot/count 쓰기까지 각각 보유한다.
        const std::array<std::shared_ptr<void>,2> selectedOwners{binding.deviceOwner,owner}; //같은 backend를 재사용한 새 coordinator가 옛 wrapper를 소유한다고 가정하지 않는다.
        for (const auto& selected:selectedOwners) { //NULL인 독립 typed owner는 생략하고 두 실제 control block을 따로 수집한다.
            if (!selected) continue;bool found=false;
            for (const auto& held:deviceCallOwners_) if (!held.owner_before(selected) && !selected.owner_before(held)) { found=true;break; } //alias shared 포인터도 control block 정체성으로 중복 제거한다.
            if (!found) deviceCallOwners_.push_back(selected);
        }
    }return owner;
}
//목록을먼저분리하여실제Source소멸자의Forget/재진입이이미반납중인목록을다시읽지않게한다.
void RawConnectionRegistry::ReleaseDevicePins() noexcept { std::vector<std::shared_ptr<void>> selected;selected.swap(deviceCallOwners_); }
//정상/예외모두카운터를먼저반납하고최외곽부모의원문후속효과뒤에만실제device를반납한다.
void RawConnectionRegistry::RunDeviceScope(const std::function<void()>& operation) { const bool outer=deviceCallDepth_==0;++deviceCallDepth_;try { operation(); }catch (...) { --deviceCallDepth_;if (outer) ReleaseDevicePins();throw; }--deviceCallDepth_;if (outer) ReleaseDevicePins(); }
void RawConnectionRegistry::BindSynchronization(std::uint32_t& value,std::shared_ptr<void> lifetime) { synchronization_=&value;synchronizationOwner_=std::move(lifetime); }
ConnectionCleanupGlobals RawConnectionRegistry::Cleanup() { return Cleanup(*synchronization_); }
ConnectionCleanupGlobals RawConnectionRegistry::Cleanup(std::uint32_t& synchronization) { return {transceivers_,buffers_,pending_,synchronization,pendingCount_}; }
std::uint32_t RawConnectionRegistry::Track(PacketConnection& connection,std::function<void()> destroy,std::function<void()> release,std::function<void(std::span<std::uint8_t>)> send,std::function<std::int32_t()> readiness,std::function<void()> buffered) {
    if (deviceShutdown_) throw std::logic_error("App최종종료뒤새Node등록거부");
    if (!destroy || !release) throw std::invalid_argument("실제 연결 close/free 장치 누락");
    for (const auto& entry:bindings_) if (entry.second.node==&connection) throw std::logic_error("현재 연결의 중복 주소 등록"); //한 노드의 살아 있는 토큰을 중복 발급하지 않는다.
    if (!nextToken_) throw std::overflow_error("연결 opaque 토큰 소진");const auto token=nextToken_++;Binding binding{&connection,std::move(destroy),std::move(release),std::move(send),std::move(buffered),{},std::move(readiness)};if (retainDeviceOwners_) binding.deviceOwner=PinBinding(binding);bindings_.emplace(token,std::move(binding));return token; //실제owner선택/할당이성공한뒤에만Binding을공개하여등록실패의raw callback을남기지않는다.
}
// 기존 연결 guard는 다른 backend의 같은 현재 노드도 실제 socket 조회로 판단한다.
std::int32_t RawConnectionRegistry::SocketReadiness(std::uint32_t token) const { const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.readiness) throw std::logic_error("현재 연결의 실제 socket 준비 장치 미연결");const auto binding=found->second;const auto owner=PinBinding(binding);const auto query=binding.readiness;return query(); }
//연결슬롯은Player관문보다먼저읽고가상송신이자기를반납해도callback이후같은노드를다시읽지않는다.
bool RawConnectionRegistry::SendMutable(std::int32_t id,std::span<std::uint8_t> packet,std::span<const std::uint8_t> players) const {
    if (id< -32 || id>21) throw std::out_of_range("원문 송신의 현재 transceiver 번호 범위");const auto token=id<0 ? pending_.at(static_cast<std::size_t>(-id)) : transceivers_.at(static_cast<std::size_t>(id));
    if (id>=1 && id<=8) { const auto offset=static_cast<std::size_t>(id)*0xb4+0x50;if (offset>=players.size()) throw std::out_of_range("원문 송신의 현재 Player 필드 범위");if (!(players[offset]&1U)) return false; }
    if (!token) return false;const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.send) throw std::logic_error("현재 살아 있는 송신기의 실제 가상+4 장치 미연결");const auto binding=found->second;const auto owner=PinBinding(binding);const auto send=binding.send;send(packet);return true;
}
bool RawConnectionRegistry::Owns(std::uint32_t token) const { return bindings_.contains(token); }
// pending/양수 alias는 진단 뒤 현재 실제 표에서 선택한다. 원문의-32 허용과 실제32칸 물리 상한을 구별한다.
bool RawConnectionRegistry::SendMutableNative(std::int32_t id,std::span<std::uint8_t> packet,std::span<const std::uint8_t> players,const ConnectionRegistryAssertion& assertion) const {
    if (id< -32 || id>21) AssertConnectionNative(assertion,0x1e);
    const auto token=id<0 ? pending_.at(static_cast<std::size_t>(-std::int64_t(id))) : transceivers_.at(static_cast<std::size_t>(id));
    if (id>=1 && id<=8) { const auto offset=static_cast<std::size_t>(id)*0xb4+0x50;if (offset>=players.size()) throw std::out_of_range("원문 송신의 현재 Player 필드 범위");if (!(players[offset]&1U)) return false; }
    if (!token) return false;const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.send) throw std::logic_error("현재 살아 있는 송신기의 실제 가상+4 장치 미연결");
    const auto binding=found->second;const auto owner=PinBinding(binding);const auto send=binding.send;send(packet);return true; // 실제 가상 송신 뒤 같은 노드를 다시 조회하지 않는다.
}
std::uint32_t RawConnectionRegistry::TokenFor(const PacketConnection& connection) const { for (const auto& entry:bindings_) if (entry.second.node==&connection) return entry.first;throw std::out_of_range("직접 송신의 현재 노드 token 누락"); } // 실제 살아 있는 동일 주소만 선택하며 sender 번호로 다른 노드를 대신하지 않는다.
void RawConnectionRegistry::SendDirect(std::uint32_t token,std::span<std::uint8_t> packet) const { const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.send) throw std::logic_error("직접 연결의 실제 가상 송신 장치 미연결");const auto binding=found->second;const auto owner=PinBinding(binding);const auto send=binding.send;send(packet); } // callback 중 해제에도 선택한 callable의 수명을 보유한다.
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
    if (!registry) throw std::invalid_argument("접속 통지의 실제 Registry 없음");if (!dispatch.Registration(1).receive) dispatch.RegisterCanonicalDirect(1,0x004e2f00U,[registry](auto sender,auto*,auto) { registry->SendConnectPacket(std::bit_cast<std::int32_t>(sender)); });
}
PacketConnection& RawConnectionRegistry::Node(std::uint32_t token) const { const auto it=bindings_.find(token);if (it==bindings_.end()) throw std::out_of_range("해제되거나 다른 owner의 연결 토큰");return *it->second.node; }
ConnectionRegistrationHooks RawConnectionRegistry::RegistrationHooks(std::function<void(std::uint32_t)> buffered) {
    if (buffered) { const auto selected=std::move(buffered);buffered=[this,selected](auto token) { const auto found=bindings_.find(token);if (found==bindings_.end()) throw std::logic_error("buffered단계의현재node없음");const auto binding=found->second;const auto owner=PinBinding(binding);selected(token); }; } //기존typedcallback도동일원문선택지점의실제owner만보유하며empty는empty그대로다.
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
void RawConnectionRegistry::Assign(std::int32_t pending,std::int32_t target,std::uint32_t server,std::function<void(std::uint32_t)> buffered) { RunDeviceScope([&] { static_cast<void>(AssignPendingConnection(pending,target,server,Cleanup(),RegistrationHooks(std::move(buffered)))); }); }
// typed의 기존 node/표 순서를 재사용하고 진단의 정상 복귀만 실제 Client 부모에 맡긴다.
ConnectionRegistrationHooks RawConnectionRegistry::NativeRegistrationHooks(std::function<void(std::uint32_t)> buffered,const ConnectionRegistryAssertion& assertion) {
    auto hooks=RegistrationHooks(std::move(buffered));hooks.assertion=[assertion](auto line) { AssertConnectionNative(assertion,line); };return hooks;
}
// b6 진단 뒤 현재 pending을 읽고 sender store 뒤 현재 target을 다시 읽는 기존4e27b0 본체를 그대로 사용한다.
void RawConnectionRegistry::AssignNative(std::int32_t pending,std::int32_t target,std::uint32_t server,std::function<void(std::uint32_t)> buffered,const ConnectionRegistryAssertion& assertion) {
    RunDeviceScope([&] { static_cast<void>(AssignPendingConnection(pending,target,server,Cleanup(),NativeRegistrationHooks(std::move(buffered),assertion))); }); //진단뒤선택device는pending제거/개수쓰기완료뒤에만반납한다.
}
//Native재배치는원문진단/현재slot쓰기부모를그대로공유하고공통buffered위치에서만실제node를선택한다.
void RawConnectionRegistry::AssignNative(std::int32_t pending,std::int32_t target,std::uint32_t server,const ConnectionRegistryAssertion& assertion) { AssignNative(pending,target,server,[this](auto token) { SetBufferedNative(token); },assertion); }
//4df470(this,1)의실제flags/cursor효과와해당node의물리owner를함께선택한다.
void RawConnectionRegistry::SetBufferedNative(std::uint32_t token) const { const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.buffered) throw std::logic_error("현재node의실제공통buffered장치없음");const auto binding=found->second;const auto owner=PinBinding(binding);const auto buffered=binding.buffered;buffered(); }
void RawConnectionRegistry::Destroy(std::uint32_t token) { const auto found=bindings_.find(token);if (found==bindings_.end()) throw std::out_of_range("현재 연결 소멸 장치 누락");const auto binding=found->second;const auto owner=PinBinding(binding);const auto operation=binding.destroy;operation(); }
void RawConnectionRegistry::RemoveAliases(std::uint32_t token) {
    bool mainAlias{};for (auto& current:transceivers_) if (current==token) { current=0;mainAlias=true; } //현재 같은 양수 연결의 칸만 제거하며 새 연결은 지우지 않는다.
    if (mainAlias && *synchronization_==token) *synchronization_=0; //pending 종료는 원문처럼 동기화 토큰을 쓰지 않는다.
    for (std::size_t slot=1;slot<pending_.size();++slot) if (pending_[slot]==token) { pending_[slot]=0;--pendingCount_; } //일반 분배가 반납한 pending의 실제 현재 개수만 감소한다.
}
void RawConnectionRegistry::Release(std::uint32_t token) {
    if (const auto packet=connectPackets_.find(token);packet!=connectPackets_.end()) { connectPackets_.erase(packet);return; } // Native free는 노드 장치와 별개인 복제 버퍼도 반납한다. 현재 칸0 쓰기는 호출 부모가 담당한다.
    const auto found=bindings_.find(token);if (found==bindings_.end()) throw std::out_of_range("현재 연결/송신버퍼 free 장치 누락");const auto binding=found->second;const auto owner=PinBinding(binding);const auto operation=binding.release;operation();
    if (!cleanupDepth_) RemoveAliases(token);bindings_.erase(token); //실제 free 성공 전에 표/장치를 지우지 않으며 Native 부모는 자신의 store를 직접 마친다.
}
//App최종종료는NativeState전환이아니며모든실제콜백복귀뒤UI/시간/장기데이터앞에서만호출한다.
void RawConnectionRegistry::ShutdownTrackedDevices() {
    if (deviceCallDepth_ || cleanupDepth_) throw std::logic_error("진행중Native device부모안의App최종종료");
    for (const auto& entry:bindings_) if (!entry.second.node || entry.second.node->depth) throw std::logic_error("진행중Node배송안의App최종종료"); //깊이를0으로조작하거나현재packet을강제로free하지않는다.
    deviceShutdown_=true;
    RunDeviceScope([&] {
        CloseAll();ClosePending(); //기존0..21/0..31Native방문순서와pendingCount미정규화를그대로사용한다.
        while (!bindings_.empty()) { //전역slot에서덮인실제잔류Node만현재등록순서대로명시close/free한다.App호스트정리이며새Native성공효과가아니다.
            const auto token=bindings_.begin()->first;auto& node=Node(token);DeferConnectionClose(node,[&] { Destroy(token); },[&] { Release(token); });
        }
        for (auto& token:buffers_) { //Native종료부모의후속store완료뒤에만같은현재초기packet토큰을free하고그칸을0으로쓴다.
            if (token) { Release(token);token=0; }
        }
    }); //선택device목록은모든slot/count/초기packet쓰기뒤에반납한다.
}
void RawConnectionRegistry::Forget(std::uint32_t token) { if (!token) return;RemoveAliases(token);bindings_.erase(token); } //기존 호스트 종료에서 이미 없어지는 자기 노드의 dangling 별칭만 반납한다.
ConnectionCleanupHooks RawConnectionRegistry::CleanupHooks() { return {[this](auto token)->PacketConnection& { return Node(token); },[this](auto token) { Destroy(token); },[this](auto token) { Release(token); },[](auto line) { throw std::logic_error("원문 연결 정리 진단:"+std::to_string(line)); }}; }
void RawConnectionRegistry::RunCleanup(const std::function<void()>& operation) { RunDeviceScope([&] { ++cleanupDepth_;try { operation(); }catch (...) { --cleanupDepth_;throw; }--cleanupDepth_; }); }
void RawConnectionRegistry::Disconnect(std::int32_t id) { RunCleanup([&] { DisconnectTransceiver(id,Cleanup(),CleanupHooks()); }); }
// 실제 free/동기화/현재칸0/초기버퍼 재조회는 기존 Cleanup 부모가 처리한다.
ConnectionCleanupHooks RawConnectionRegistry::NativeCleanupHooks(const ConnectionRegistryAssertion& assertion) {
    auto hooks=CleanupHooks();hooks.assertion=[assertion](auto line) { AssertConnectionNative(assertion,line); };return hooks;
}
// 정상 복귀 뒤 도달한 물리 범위만 보호하며 close callback이 남긴 앞 효과를 되돌리지 않는다.
void RawConnectionRegistry::DisconnectNative(std::int32_t id,const ConnectionRegistryAssertion& assertion) {
    RunCleanup([&] { DisconnectTransceiver(id,Cleanup(),NativeCleanupHooks(assertion)); });
}
void RawConnectionRegistry::CloseAll() { RunCleanup([&] { CloseAllTransceivers(Cleanup(),CleanupHooks()); }); }
void RawConnectionRegistry::EndPlayers(std::uint32_t server) { RunCleanup([&] { EndPlayerConnections(server,Cleanup(),CleanupHooks()); }); }
void RawConnectionRegistry::ClosePending() { RunCleanup([&] { CloseAllPendingConnections(Cleanup(),CleanupHooks()); }); }
}
