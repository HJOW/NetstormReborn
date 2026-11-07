// Root1Ch의 실제 관문과 pending close 전에 소켓을 보존하는 부모를 복원한다.
#include "root/LoginRequest.h"
#include "root/PacketRegistration.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <vector>

namespace netstorm::root { // wire DWORD와 현대 OS 소켓 소유권의 폭을 분리한다.
namespace { // 원문 비정렬 필드/선택 장치를 현재 물리에 연결한다.
// 실제 선택 장치가 없으면 명시 오류이며 로그인 성공을 대신 만들지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("R1Ch의 선택된 실제 장치 미연결");return function; }
// 선택 폭의 실제 필드만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("R1Ch 현재 필드의 실제 물리 부족");std::uint32_t result{};
    for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8);return result; // 낮은 BYTE부터 x86 정수로 조립한다.
}
// 현재 요청/헤더에서 지정 폭만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("R1Ch 내부 필드 쓰기의 실제 물리 부족");
    for (std::size_t byte=0;byte<width;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 원문의 비정렬 DWORD/WORD 폭을 유지한다.
}
}
LoginRequest::LoginRequest(ConnectionState& state,TransceiverTransport& transport,ChallengeStore& challenges,LoginRequestHooks hooks)
    :state_(state),transport_(transport),challenges_(challenges),hooks_(std::move(hooks)) {}
std::unique_ptr<SocketDevice> LoginRequest::ClaimSocket(std::uint32_t token) {
    const auto found=sockets_.find(token);if (found==sockets_.end()) throw std::out_of_range("R CCh가 소비할 생존 socket 이전 토큰 없음");auto socket=std::move(found->second);sockets_.erase(found);return socket;
}
void LoginRequest::Trace(std::uint32_t line) { Need(hooks_.log)(29,std::to_string(line)+"\n"); }
void LoginRequest::Install() {
    auto& dispatch=transport_.PacketHandlers();dispatch.ObservePacket(0x1c,[this](auto packet,auto sending) { ObservePacketFields(0x1c,packet,sending,transport_.Lifecycle().assertion); });dispatch.Version(0x1c,1,0);dispatch.RequireLength(0x1c,0xffff);
    dispatch.RegisterDirect(0x1c,[this](auto sender,auto* connection,auto packet) { if (!connection) throw std::out_of_range("R1Ch 실제 연결 주소의 원문 NULL 읽기 거부");Receive(std::bit_cast<std::int32_t>(sender),*connection,packet); }); // 현재 수신 노드의 깊이 대여 안에서만 소켓 이전/기존 노드 닫힘을 수행한다.
}
void LoginRequest::Receive(std::int32_t sender,o::PacketConnection& connection,std::span<const std::uint8_t> request) {
    if (state_.exitRequested) { Need(transport_.Lifecycle().exit)(0);throw std::logic_error("R1Ch _exit 장치가 복귀했습니다"); }
    if (!(state_.modeFlags&kChallengeMode)) Need(transport_.Lifecycle().assertion)("smModeFlags & smCHAL","c:\\ns\\r\\r.cpp",1206);
    if (Read(connection.address,0,2)==2) {
        const auto local=Need(hooks_.localAddress)();const auto localIP=Read(local,4),peerIP=Read(connection.address,4);if (peerIP!=0x0100007fU && (localIP&0xffffU)!=(peerIP&0xffffU)) return; // 원문의 loopback 한 주소 예외와 낮은WORD 서브넷 비교를 보존한다.
    }
    const auto zone=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Read(request,19,1)));
    const auto valid=[&] { return zone>0 && zone<state_.challengeStateCount && state_.challenges.at(static_cast<std::size_t>(zone))!=0; }; // signed BYTE/현재 설정 개수/현재 토큰을 같은 순서로 평가한다.
    if (sender>0) {
        if (!valid()) { Trace(1245);return; }const auto current=state_.challenges[static_cast<std::size_t>(zone)];if (current) { std::vector<std::uint8_t> packet(request.begin(),request.end());Need(hooks_.login)(current,1,packet); }Trace(1252);return;
    }
    if (!valid()) { Trace(1257);return; }
    if (!(state_.modeFlags&kRootMode)) { Need(hooks_.log)(9,"Login packet to non-root\n");Trace(1263);return; }
    if (!state_.challenges[static_cast<std::size_t>(zone)]) return;
    if (serial_==std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("R socket 이전 숫자 토큰 소진");std::vector<std::uint8_t> packet(request.begin(),request.end());
    const auto token=++serial_;auto [slot,inserted]=sockets_.try_emplace(token);static_cast<void>(inserted); // 이전 전에 소유권 칸을 먼저 확보해 할당 실패에서 실제 핸들을 잃지 않는다.
    try {
        slot->second=transport_.DetachSocket(PendingTransceiver(state_,sender));ClosePending(state_,sender,transport_.Lifecycle()); // 옛 소켓 무효화 뒤 원문 pending close/카운터 변경을 수행한다.
        const auto current=state_.challenges[static_cast<std::size_t>(zone)];if (Read(challenges_.Bytes(current),4)&1U) { Put(packet,41,token,4);Need(hooks_.login)(state_.challenges[static_cast<std::size_t>(zone)],1,packet); }
    } catch (...) { sockets_.erase(token);throw; }
    sockets_.erase(token); // 실제 CCh가 Claim한 성공 경로는 이미 비어 있다. 미소비 이전은 호스트 소유권으로 반납한다.
}
}
