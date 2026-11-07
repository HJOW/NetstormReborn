// R의 존 선택/참가자 통계/친구 검색과 음수 응답 뒤 닫힘 순서를 복원한다.
#include "root/RootDirectory.h"
#include "root/PacketRegistration.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::root { // R의10.71 프로토콜 자료와 제품 표시11.01은 서로 다른 값이다.
namespace { // 비정렬 x86 필드와 실제 물리 경계만 조작한다.
// 실제 선택 필드만 읽으며 외부 패킷/잘못된 CCh가 호스트 메모리를 벗어나지 않게 한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("R Root 목록 필드의 실제 물리 부족");
    std::uint32_t result{};for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8);return result; // 낮은 BYTE부터 x86 정수로 조립한다.
}
// 지정 폭만 덮고 원문 allocator가 채운 나머지ff 바이트를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("R Root 응답 쓰기의 실제 물리 부족");
    for (std::size_t byte=0;byte<width;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 원문 BYTE/WORD/DWORD 폭으로 쓴다.
}
}
ChallengeDirectoryView::ChallengeDirectoryView(std::span<const std::uint8_t> bytes):bytes_(bytes) {
    if (bytes.size()<kRootChallengeBytes) throw std::out_of_range("R CCh 실제2b80BYTE 저장소 누락");
}
std::int32_t ChallengeDirectoryView::Count() const {
    std::int32_t count{};for (std::size_t participant=1;participant<40;++participant) if (Read(bytes_,0xd78+participant*0xbc+0x64)&1U) ++count;return count; // 원문은0번을 제외한 현재 flags DWORD를 매번 읽는다.
}
std::pair<std::int32_t,std::int32_t> ChallengeDirectoryView::Statistics() const {
    std::int32_t count{},levels{};
    for (std::size_t participant=1;participant<40;++participant) { // 실제 활성 참가자만 레벨 BYTE를 합산한다. 관전자/0번 추정 필터는 추가하지 않는다.
        const auto at=0xd78+participant*0xbc;if (!(Read(bytes_,at+0x64)&1U)) continue;++count;levels+=static_cast<std::int32_t>(Read(bytes_,at+0x5e,1));
    }
    return {count,count>0 ? levels/count:0};
}
bool ChallengeDirectoryView::ContainsSubscriber(std::uint32_t subscriber) const {
    for (std::size_t participant=0;participant<40;++participant) if (Read(bytes_,0xd78+participant*0xbc+4)==subscriber) return true;return false; // 원문 친구 검색은0번도 포함하며 flags를 읽지 않는다.
}
std::uint32_t ChallengeDirectoryView::Transceiver(std::uint32_t participant) const {
    if (participant>=40) throw std::out_of_range("R 복합 번호의 실제 참가자40칸 범위");return Read(bytes_,0x2ad8+participant*4);
}
RootDirectory::RootDirectory(ConnectionState& state,TransceiverTransport& transport,RootDirectoryGlobals globals,std::function<std::span<const std::uint8_t>(std::uint32_t)> challenge)
    :state_(state),transport_(transport),globals_(globals),challenge_(std::move(challenge)) {
    if (!challenge_) throw std::invalid_argument("R Root의 실제 CCh 관리자 조회 장치 누락");
}
ChallengeDirectoryView RootDirectory::Challenge(std::uint32_t index) const {
    const auto token=state_.challenges.at(index);if (!token) throw std::out_of_range("R Root가 선택한 현재 CCh가 NULL입니다");return ChallengeDirectoryView(challenge_(token));
}
void RootDirectory::Assert(std::string_view expression,std::string_view file,std::uint32_t line) const {
    const auto hooks=transport_.Lifecycle();if (!hooks.assertion) throw std::logic_error("R Root의 실제 진단 장치 누락");hooks.assertion(expression,file,line);
}
void RootDirectory::Install() {
    auto& dispatch=transport_.PacketHandlers();dispatch.ObservePacket(0x16,[this](auto packet,auto sending) { ObservePacketFields(0x16,packet,sending,transport_.Lifecycle().assertion); });dispatch.Version(0x16,1,0);dispatch.RequireLength(0x16,0xffff);
    dispatch.ObservePacket(0x0f,[this](auto packet,auto sending) { ObservePacketFields(0x0f,packet,sending,transport_.Lifecycle().assertion); });dispatch.Version(0x0f,1,0);dispatch.RequireLength(0x0f,0xffff);
    dispatch.RegisterDirect(0x16,[this](auto sender,auto*,auto packet) { Receive(std::bit_cast<std::int32_t>(sender),packet); }); // 현재 수신 노드의 깊이 대여 안에서만 응답/guard를 수행한다.
}
void RootDirectory::Receive(std::int32_t sender,std::span<const std::uint8_t> request) {
    if (state_.exitRequested) { const auto hooks=transport_.Lifecycle();if (!hooks.exit) throw std::logic_error("R Root의 실제 종료 장치 누락");hooks.exit(0);throw std::logic_error("R Root의 _exit 장치가 복귀했습니다"); }
    if (request.size()<14) throw std::out_of_range("R16h 실제 고정 필드14BYTE 부족");
    std::array<std::uint8_t,253> packet;packet.fill(0xff);Put(packet,0,1,1);Put(packet,1,0x0f,1);Put(packet,2,4,2);Put(packet,4,10,2);Put(packet,6,71,2);Put(packet,8,0,1);Put(packet,10,0,1);Put(packet,11,0,1);
    std::int32_t count{};for (const auto token:state_.challenges) if (token) ++count; // 실제51칸 전체의 현재 비NULL 개수다.
    if (std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state_.challengeStateCount)-1U)!=count) Assert("rootServerNumChalStates-1 == activeCount","c:\\ns\\r\\r.cpp",969);
    if (!count) Assert("activeCount != 0","c:\\ns\\r\\r.cpp",970);
    std::int32_t initial=1;if (globals_.groupByLevel) initial=std::bit_cast<std::int32_t>((Read(request,10)-1U)*std::uint32_t(count))/43+1; // IMUL 하위DWORD와 signed IDIV의0방향 나눗셈을 보존한다.
    initial=std::max(1,std::min(initial,count));auto recommended=initial;
    while (recommended>0 && Challenge(static_cast<std::uint32_t>(recommended)).Count()==0) --recommended; // 0044d100의 빈 존이면 먼저 아래로 찾는다.
    recommended=std::max(1,recommended);
    const auto full=[this](std::int32_t index) { // IsFull 뒤 옵션이0이 아닐 때만 Count를 다시 읽는 원문 단락 평가다.
        if (Challenge(static_cast<std::uint32_t>(index)).Count()==39) return true;return globals_.maxPlayers!=0 && Challenge(static_cast<std::uint32_t>(index)).Count()>=globals_.maxPlayers;
    };
    while (recommended<=count && full(recommended)) ++recommended; // 위쪽의 현재 가득 찬 존을 건너뛴다.
    recommended=std::min(count,recommended);
    while (recommended>0 && full(recommended)) --recommended; // 위쪽 한계 뒤에는 아래쪽을 다시 찾는다.
    if (recommended<1 || count<recommended) recommended=initial;Put(packet,9,static_cast<std::uint32_t>(std::max(1,std::min(count,recommended))),1);
    static_cast<void>(Read(request,6));Put(packet,16,globals_.rootServersChecksum,4);Put(packet,12,0,4); // 제공된475660의 지역 서버목록 vector는 두 checksum 분기 모두 길이0이다. 임의 목록을 생성하지 않는다.
    for (std::uint32_t index=0;index<state_.challenges.size();++index) { // 실제 비NULL 존의 번호/활성 수/평균을 현재 순서대로3BYTE씩 쓴다.
        if (!state_.challenges[index]) continue;const auto [players,average]=Challenge(index).Statistics();const auto at=20U+std::uint32_t(packet[10])*3U;
        Put(packet,at,index,1);Put(packet,at+1,static_cast<std::uint32_t>(players),1);Put(packet,at+2,static_cast<std::uint32_t>(average),1);++packet[10];
    }
    for (std::uint32_t index=0;index<state_.challenges.size() && packet[11]<16;++index) { // 존 순서→요청 친구 순서로 찾으며 중복 구독자도 원문처럼 보존한다.
        if (!state_.challenges[index]) continue;
        for (std::int32_t friendIndex=0;friendIndex<std::bit_cast<std::int8_t>(request[5]) && packet[11]<16;++friendIndex) { // signed BYTE 친구 수이며 최대16개의 일치 결과에서 전체 검색을 멈춘다.
            const auto subscriber=Read(request,14U+std::uint32_t(friendIndex)*4U);if (!Challenge(index).ContainsSubscriber(subscriber)) continue;
            const auto at=20U+std::uint32_t(packet[10])*3U+std::uint32_t(packet[11])*5U;Put(packet,at,index,1);Put(packet,at+1,subscriber,4);++packet[11];
        }
    }
    Reply(sender,packet); // 실제 Send의0Fh observer가 최종 길이를 작성한다.
}
void RootDirectory::Reply(std::int32_t sender,std::span<std::uint8_t> packet) {
    if (sender<0) {
        const auto token=PendingTransceiver(state_,sender);auto& connection=transport_.Connection(token);
        const auto close=[&] { ClosePending(state_,std::bit_cast<std::int32_t>(connection.sender),transport_.Lifecycle()); }; // guard는 캡처한 옛 번호 대신 송신 뒤 같은 노드의 현재 sender를 읽는다.
        try { transport_.Send(token,packet); } catch (...) { close();throw; }close();return; // 실제 송신 예외에서도475520 guard의 닫힘을 수행한다.
    }
    if (sender<256) { if (sender>0) { const auto token=state_.transceivers[static_cast<std::uint32_t>(sender)];if (token) transport_.Send(token,packet); }return; }
    const auto index=static_cast<std::uint32_t>(sender>>8);
    if (index>0 && static_cast<std::int32_t>(index)<state_.challengeStateCount && state_.challenges.at(index)) transport_.Send(Challenge(index).Transceiver(static_cast<std::uint32_t>(sender)&0xffU),packet);
}
}
