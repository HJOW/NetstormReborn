// 실제 R의 인사 응답/빈 수신과 LAN 요청·현재 포트 공지를 복원한다.
#include "root/RootControl.h"
#include "root/ChallengeDetail.h"
#include "root/AddressCodec.h"
#include "o/RawLocalServerDiscovery.h"

namespace netstorm::root { // 서로 다른 게임 Client의 연결표/검색 상태를 만들지 않는다.
using namespace challenge_detail; // 실제 원문 폭과 allocator/현재 packet 바이트를 사용한다.
RootControl::RootControl(ConnectionState& state,TransceiverTransport& transport,Options& options,AddressCodec& addresses,Clock& clock,std::function<void(std::int32_t,std::string_view)> log)
    :state_(state),transport_(transport),options_(options),addresses_(addresses),clock_(clock),log_(std::move(log)) {}
void RootControl::CheckExit() { if (state_.exitRequested) { Need(transport_.Lifecycle().exit)(0);throw std::logic_error("R 인사/LAN _exit 장치가 복귀했습니다"); } }
void RootControl::Assert(std::string_view expression,std::string_view file,std::uint32_t line) { Need(transport_.Lifecycle().assertion)(expression,file,line); }
void RootControl::ObserveGreeting(std::span<std::uint8_t> packet,std::uint32_t sending,bool reply) {
    if (!sending) return;const auto extra=Read(packet,8);if (extra==0xffffffffU) Assert("extraDataLen != (int)-1","c:\\ns\\o\\_p_packets.cpp",reply ? 917:885);Put(packet,2,extra+12U,2); // 원문 DWORD 덧셈의 최종 하위WORD만 쓰며 진단 복귀 뒤에도 계속한다.
}
void RootControl::Install() {
    auto& dispatch=transport_.PacketHandlers();
    for (const auto type:std::array<std::uint8_t,2>{0x0b,0x0c}) { // 두 원문 가변 길이 인사의 현재observer/version·정책만 등록한다.
        dispatch.ObservePacket(type,[this,type](auto packet,auto sending) { ObserveGreeting(packet,sending,type==0x0c); });dispatch.Version(type,1,0);dispatch.RequireLength(type,0xffff);
    }
    dispatch.RegisterDirect(0x0b,[this](auto sender,auto* connection,auto) { ReceiveGreeting(std::bit_cast<std::int32_t>(sender),connection); });dispatch.RegisterDirect(0x0c,[this](auto,auto*,auto) { CheckExit(); }); //0ch의 실제 몸체는 종료 관문 뒤 즉시 반환한다.
    dispatch.ObservePacket(0x66,[](auto,auto) {});dispatch.Version(0x66,1,0);dispatch.RequireLength(0x66,12);dispatch.RegisterDirect(0x66,[this](auto sender,auto*,auto packet) { ReceiveDiscovery(std::bit_cast<std::int32_t>(sender),packet); }); //66h의 원문 observer는 빈 몸체이며 실제 연결 인자는 읽지 않는다.
    for (const auto type:std::array<std::uint8_t,2>{0x11,0x12}) { dispatch.ObservePacket(type,[](auto,auto) {});dispatch.Version(type,1,0);dispatch.RequireLength(type,12); } //444060/4440f0의 빈 observer와 원문 고정 길이/버전만 등록한다.
    dispatch.RegisterDirect(0x11,[this](auto sender,auto*,auto packet) { ReceiveDiagnostic(std::bit_cast<std::int32_t>(sender),packet); });dispatch.RegisterDirect(0x12,[this](auto sender,auto* connection,auto) { ReceiveDiagnosticRequest(std::bit_cast<std::int32_t>(sender),connection); });
}
void RootControl::ReceiveGreeting(std::int32_t sender,o::PacketConnection* connection) {
    CheckExit();const auto current=ReadTransceiver(state_,sender,transport_.Lifecycle());if (current ? &transport_.Connection(current)!=connection:connection!=nullptr) Assert("t == _t","c:\\ns\\r\\r.cpp",462);if (!current) return;auto reply=Packet(0x0c,12);Put(reply,4,10,2);Put(reply,6,71,2);Put(reply,8,0);transport_.Send(current,reply);transport_.Flush(current); // 제품표시11.01과 구별한 원문10.71 프로토콜이며 진단 복귀 후 같은 조회 노드만 사용한다.
}
void RootControl::ReceiveDiscovery(std::int32_t sender,std::span<const std::uint8_t> packet) {
    CheckExit();
    if (!(state_.modeFlags&kRootMode)) {
        if (sender>=256) Assert("id < 256","c:\\ns\\r\\r.cpp",1132);
        if (sender<0) { const auto current=PendingTransceiver(state_,sender);if (current) ClosePending(state_,std::bit_cast<std::int32_t>(transport_.Connection(current).sender),transport_.Lifecycle()); } //4754e0은실제pending을빌리고475520은그노드의현재sender로닫는다.상위ReadTransceiver의음수관문과합치지않는다.
        else if (sender>0 && sender<256) { const auto current=state_.transceivers[static_cast<std::size_t>(sender)];if (current) transport_.Terminate(current); }return;
    }
    if (!Read(packet,4)) Announce(); //Root는명령0에만재공지하며그외명령/검색토큰은읽지않는다.
}
void RootControl::Announce() {
    auto packet=o::MakeLocalServerDiscoveryPacket(1,std::bit_cast<std::uint32_t>(options_.basePort));const auto send=[&](std::uint32_t current,std::span<const std::uint8_t,20> destination) { // 같은66h물리/노드snapshot으로원문가상SendTo세호출을수행한다.
        for (std::uint32_t attempt=0;attempt<3;++attempt) transport_.SendTo(current,packet,destination); // observer/통지의패킷변경을다음호출에보존하며실패를성공으로합성하지않는다.
    };
    const auto udp=ReadTransceiver(state_,std::bit_cast<std::int32_t>(state_.udpBroadcastId),transport_.Lifecycle());
    if (udp) { std::array<std::uint8_t,20> destination{};const auto port=static_cast<std::uint16_t>(options_.broadcastPort);Put(destination,0,2,2);Put(destination,2,port>>8U,1);Put(destination,3,port&255U,1);Put(destination,4,0xffffffffU);send(udp,destination); } //86800의초기0/family2/htons포트·전체broadcast주소이며tailDWORD0이다.
    const auto ipx=ReadTransceiver(state_,std::bit_cast<std::int32_t>(state_.ipxBroadcastId),transport_.Lifecycle());if (ipx) { const auto destination=addresses_.Parse(std::optional<std::string>{"0.ffffffffffff"},options_.broadcastPort);send(ipx,destination); } // 원문의 IPX 목적지 구성은 유지하며 실제 IPX 통신은 현재 IPv4 장치가 지원하지 않는다.
}
}
