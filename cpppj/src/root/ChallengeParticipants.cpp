// CCh 참가자/전투의 실제 필드 초기화와 종료 시 master 재선택을 복원한다.
#include "root/ChallengeDetail.h"
#include <ctime>

namespace netstorm::root { // 같은 raw 참가자/전투와 실제 송신만 사용한다.
using namespace challenge_detail; // 지정 폭의 실제 필드 쓰기를 공유한다.
void ChallengeRuntime::InitializePlayer(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> request) {
    auto player=Player(challenge,participant);Put(player,0,participant);Put(player,4,Read(request,11));const auto name=Text(request,46);if (name.size()+1>40) throw std::out_of_range("R 참가자 이름의 실제40BYTE 범위");PutText(player,0x26,name);
    constexpr std::string_view remove=".?=()[]~\"`;: "; // PE004cd528의 원문 제거 문자 집합이다.
    std::size_t at=0x26;
    while (player[at]) { //0044c8e0처럼 원문 이름 영역에서 겹침 이동하며 끝 이후 기존 바이트를 보존한다.
        if (remove.find(static_cast<char>(player[at]))==std::string_view::npos) { ++at;continue; }const auto tail=Text(player,at);std::memmove(player.data()+at,player.data()+at+1,tail.size());
    }
    player[at]=0;Put(player,0x6c,static_cast<std::uint32_t>(std::clock()));Put(player,0x70,0);Put(player,0x74,0,1);Copy(player,0x4e,request.subspan(20,6));Copy(player,0x54,request.subspan(26,10));Put(player,0x64,((Read(request,37)&0x1b00U)|1U)&~0x200U);
    const auto point=challenges_.Geometry().grid.at(participant);Put(player,0x9c,std::bit_cast<std::uint32_t>(point[0]));Put(player,0xa0,std::bit_cast<std::uint32_t>(point[1]));Put(player,0xa4,0);Put(player,0xa8,0);
    if (Need(hooks_.markedSubscriber)(Read(request,11))) Put(player,0x74,1,1);const auto connection=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(participant);const auto address=Need(hooks_.addressText)(transport_.Connection(connection).address,false);if (address.size()+1>30) throw std::out_of_range("R 참가자 주소의 실제30BYTE 범위");PutText(player,8,address);
    Put(player,0xb0,Read(request,4,2),2);Put(player,0xb2,Read(request,6,2),2);Put(player,0xac,Read(request,10,1));Put(player,0xb4,Read(request,8,2),2);Put(player,0x5e,Read(request,36,1),1);
}
void ChallengeRuntime::RecalculateBattleVersions(std::uint32_t challenge,std::uint32_t battle) {
    if (battle>=10) Assert("bId >= 0 && bId < CL_NUM_CHAL_BATTLES",1104);if (!(Read(Battle(challenge,battle),0)&1U)) Assert("b->isValid()",1107);std::uint32_t maximum=0xffffffffU,minimum=256;
    for (std::size_t slot=0;slot<8;++slot) { // 현재8명의 원문 필드를 같은 signed 비교로 집계한다.
        const auto participant=Read(Battle(challenge,battle),0x20+slot*4);if (!participant) continue;const auto player=Player(challenge,participant);const auto value=Read(player,0xb4,2);if (static_cast<std::int32_t>(value)>=std::bit_cast<std::int32_t>(maximum)) maximum=value;minimum=std::min(minimum,Read(player,0xb2,2));
    }
    Put(Battle(challenge,battle),0x18,maximum);Put(Battle(challenge,battle),0x1c,minimum);
}
void ChallengeRuntime::UpdateBattleMaster(std::uint32_t challenge,std::uint32_t battle) {
    std::int32_t candidate=-1;std::uint32_t masters{};
    for (std::size_t slot=0;slot<8;++slot) { // 현재 명단에서 master 수와 첫 비BADDIAG 후보를 읽는다.
        const auto participant=Read(Battle(challenge,battle),0x20+slot*4);if (!participant) continue;const auto flags=Read(Player(challenge,participant),0x64);if (!(flags&1U)) Assert("p->isValid()",5555);if (flags&4U) ++masters;else if (candidate==-1 && !(flags&0x800U)) candidate=static_cast<std::int32_t>(participant);
    }
    if (masters>1) Assert("foundMaster <= 1",5565);if (masters) return;
    if (candidate!=-1) { auto player=Player(challenge,static_cast<std::uint32_t>(candidate));Put(player,0x64,Read(player,0x64)|4U);Broadcast(challenge,PlayerChange(challenge,static_cast<std::uint32_t>(candidate))); }
    auto record=Battle(challenge,battle);std::fill_n(record.begin()+0x10c,0x40,std::uint8_t{});record[0x40]=0;std::fill_n(record.begin()+0x41,0xa0,std::uint8_t{});
    for (std::size_t slot=0;slot<8;++slot) { // 새 master 선택 뒤 현재 참가자의 ready 비트만 내리고 해당 전투에 변경을 보낸다.
        const auto participant=Read(Battle(challenge,battle),0x20+slot*4);if (!participant) continue;auto player=Player(challenge,participant);Put(player,0x64,Read(player,0x64)&~0x2000U);SendBattle(challenge,battle,PlayerChange(challenge,participant));
    }
    Battle(challenge,battle)[0xe1]=0;Broadcast(challenge,BattleDescription(challenge,battle));
}
void ChallengeRuntime::DeletePlayer(std::uint32_t challenge,std::uint32_t participant) {
    if (!participant || participant>=40) Assert("0 < pId && pId < CL_NUM_CHAL_PLAYERS",1167);
    if (!participant || participant>=40) { Need(hooks_.log)(9,"Cannot delete illegal player number "+std::to_string(participant)+"\n");return; }
    auto player=Player(challenge,participant);if (state_.assertions && !(Read(player,0x64)&1U)) Assert("p->isValid()",1175);if (!(Read(player,0x64)&1U)) { Need(hooks_.log)(9,"Cannot delete invalid player number "+std::to_string(participant)+"\n");return; }
    const auto battle=Read(player,0xa4);if (battle) { const auto slot=Read(player,0xa8);if (slot>=8) throw std::out_of_range("R 전투의 실제8칸 참가자 범위");Put(Battle(challenge,battle),0x20+slot*4,0);RecalculateBattleVersions(challenge,battle);UpdateBattleMaster(challenge,battle); }
    player=Player(challenge,participant);const auto flags=Read(player,0x64);const auto reason=flags&0x80U ? "newchal":(flags&0x20U ? "battle":"disconnect");Need(hooks_.log)(0,"Logout by \""+Text(player,0x26)+"\"["+std::to_string(Read(player,4))+"]: "+reason+"\n");std::fill(player.begin(),player.end(),std::uint8_t{});
}
}
