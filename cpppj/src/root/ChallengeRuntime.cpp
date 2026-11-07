// 실제 CCh 현재 슬롯/메타 패킷 등록/종료 부모를 구현한다.
#include "root/ChallengeDetail.h"

namespace netstorm::root { // Root와 같은 실제 소유자/옵션을 대여한다.
using namespace challenge_detail; // 이 모듈의 비정렬 바이트 helper만 사용한다.
ChallengeRuntime::ChallengeRuntime(ConnectionState& state,TransceiverTransport& transport,ChallengeStore& challenges,Options& options,ChallengeMessages& messages,ChallengeRuntimeHooks hooks)
    :state_(state),transport_(transport),challenges_(challenges),options_(options),messages_(messages),hooks_(std::move(hooks)) {}
std::span<std::uint8_t> ChallengeRuntime::Player(std::uint32_t challenge,std::uint32_t participant) { if (participant>=40) throw std::out_of_range("R CCh 참가자40칸 범위");return challenges_.Bytes(challenge).subspan(kChallengeParticipantOffset+participant*kChallengeParticipantBytes,kChallengeParticipantBytes); }
std::span<std::uint8_t> ChallengeRuntime::Battle(std::uint32_t challenge,std::uint32_t battle) { if (battle>=10) throw std::out_of_range("R CCh 전투10칸 범위");return challenges_.Bytes(challenge).subspan(kChallengeBattleOffset+battle*kChallengeBattleBytes,kChallengeBattleBytes); }
void ChallengeRuntime::Assert(std::string_view expression,std::uint32_t line) { Need(transport_.Lifecycle().assertion)(expression,"c:\\ns\\o\\ch.cpp",line); }
std::span<std::uint8_t> ChallengeRuntime::Shared(std::uint8_t type) { const auto length=transport_.PacketHandlers().Registration(type).length;Header(scratch_,type,length==0xffff ? 4:length);return scratch_; }
void ChallengeRuntime::Send(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet) { const auto current=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(participant);if (current) transport_.Send(current,packet); }
void ChallengeRuntime::Broadcast(std::uint32_t challenge,std::span<std::uint8_t> packet) {
    for (std::uint32_t participant=0;participant<40;++participant) { // 원문 순서대로 현재40슬롯을 읽는다.
        const auto current=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(participant);if (!current) continue;if (!(Read(Player(challenge,participant),0x64)&1U)) Assert("chalPlayer[i].isValid()",2253);Send(challenge,participant,packet);
    }
}
void ChallengeRuntime::SendBattle(std::uint32_t challenge,std::uint32_t battle,std::span<std::uint8_t> packet) {
    if (!(Read(Battle(challenge,battle),0)&1U)) return;
    for (std::size_t slot=0;slot<8;++slot) { // 현재 전투8칸에서 매번 참가자 번호를 다시 읽는다.
        const auto participant=Read(Battle(challenge,battle),0x20+slot*4);if (participant) Send(challenge,participant,packet);
    }
}
void ChallengeRuntime::Observe(std::uint8_t type,std::span<std::uint8_t> packet,std::uint32_t sending) {
    if (!sending) return;
    const auto check=[&](bool failed,std::string_view expression,std::uint32_t line) { if (failed) Need(transport_.Lifecycle().assertion)(expression,"c:\\ns\\o\\_p_packets.cpp",line); }; // 선택된 실제 패킷 진단만 실행한다.
    switch (type) {
    case 0x20:check(Read(packet,18,1)==255,"nameLen != (unsigned char)-1",1329);Put(packet,2,Read(packet,18,1)+53,2);break;
    case 0x21:check(Read(packet,4,1)==255,"numPlayers != (unsigned char)-1",1411);Put(packet,2,Read(packet,4,1)+5,2);break;
    case 0x26:check(Read(packet,4,2)==65535,"modLen != (unsigned short)-1",1512);check(Read(packet,6,1)==255,"contactLen != (unsigned char)-1",1513);Put(packet,2,Read(packet,4,2)+9+Read(packet,6,1),2);break;
    case 0x41:check(Read(packet,10,2)==65535,"nameLen != (short)-1",2074);check(Read(packet,14,1)==255,"addrLen != (unsigned char)-1",2076);Put(packet,2,std::uint32_t(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(packet,10,2))))+15U+std::uint32_t(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(packet,12,2))))+Read(packet,14,1)*4,2);break; // 원문 bufferLen 관문은 WORD와ffffffff 비교여서 항상 거짓이다.
    case 0x71:check(Read(packet,9,1)==255,"descLen != (unsigned char)-1",2784);Put(packet,2,Read(packet,9,1)+10,2);break;
    case 0x7b:check(Read(packet,9,1)==255,"addrLen != (char)-1",2981);Put(packet,2,static_cast<std::uint16_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Read(packet,9,1)))+10),2);break;
    default:break;
    }
}
void ChallengeRuntime::InstallPacketObservers() {
    auto& dispatch=transport_.PacketHandlers();
    for (const auto type:std::array<std::uint8_t,10>{0x17,0x1d,0x20,0x21,0x22,0x23,0x26,0x41,0x71,0x7b}) { // 원문 실제 송신 타입만 등록하며 기존 receiver를 바꾸지 않는다.
        dispatch.ObservePacket(type,[this,type](auto packet,auto sending) { Observe(type,packet,sending); });dispatch.Version(type,type==0x41 ? 3:1,0);
    }
    dispatch.Observe(0x88,{});dispatch.Version(0x88,1,0);dispatch.RequireLength(0x88,98); // 별칭의 실제 고정98BYTE다.
}
void ChallengeRuntime::EndParticipant(std::uint32_t challenge,std::uint32_t participant) {
    const auto oldBattle=Read(Player(challenge,participant),0xa4);Need(hooks_.log)(29,"Terminating "+std::to_string(participant)+"\n");NotifyRoot(challenge,participant,false,0);DeletePlayer(challenge,participant);
    auto bytes=challenges_.Bytes(challenge);const auto current=Read(bytes,kChallengeConnectionOffset+participant*4);
    if (current) { if (transport_.SocketFlags(current)&0x2000U) Need(hooks_.log)(29,"Remote Shutdown "+std::to_string(participant)+"\n");transport_.Close(current);Put(challenges_.Bytes(challenge),kChallengeConnectionOffset+participant*4,0); }
    auto packet=Packet(0x21,50);Put(packet,4,1,1);Put(packet,5,participant,1);Broadcast(challenge,packet);if (oldBattle) UpdateBattleMaster(challenge,oldBattle);
}
}
