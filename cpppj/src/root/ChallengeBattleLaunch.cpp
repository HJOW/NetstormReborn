// CCh 전투 시작의 현재 명단/접속 안내·연결 종료와 재배치 부모를 복원한다.
#include "root/ChallengeDetail.h"
#include "root/Clock.h"
#include "root/Random.h"

namespace netstorm::root { //004570c0/450240/4503b0의 실제 현재 전투와 장치를 사용한다.
using namespace challenge_detail; // 같은 원문 폭·allocator/미작성 바이트·공유 scratch다.
namespace { // 고정 strncpy 필드만 원문과 같은 NUL 패딩으로 작성한다.
void FixedText(std::span<std::uint8_t> packet,std::size_t at,std::size_t count,std::string_view text) { // countBYTE를 모두 쓰되 긴 문자열은 NUL을 임의 추가하지 않는다.
    text=text.substr(0,text.find('\0'));for (std::size_t index=0;index<count;++index) Put(packet,at+index,index<text.size() ? static_cast<unsigned char>(text[index]):0,1); // 이름32/주소29의 실제 패딩 범위다.
}
}
void ChallengeRuntime::SailOut(std::uint32_t challenge,std::uint32_t participant,std::uint32_t battle,bool clearStart) {
    auto player=Player(challenge,participant);Put(player,0x64,Read(player,0x64)|8U);if (clearStart) Put(player,0x64,Read(player,0x64)&~0x10U);const auto record=Battle(challenge,battle);const ChallengePoint center{{std::bit_cast<float>(Read(record,0x10)),std::bit_cast<float>(Read(record,0x14))}};const auto position=FindPosition(challenge,center,10,10);auto request=Packet(0x1e,14,14);Put(request,4,std::bit_cast<std::uint32_t>(position[0]));Put(request,8,std::bit_cast<std::uint32_t>(position[1]));Put(request,12,0,1);Put(request,13,0,1);MoveParticipant(challenge,Read(player,0),request);Put(player,0x64,Read(player,0x64)&~8U); // 원문 실제 레코드id로 이동하고 복귀 뒤 원래 레코드의 임시 bit8만 내린다.
}
void ChallengeRuntime::RepositionBattle(std::uint32_t challenge,std::uint32_t battle,std::optional<std::uint32_t> participant) {
    const auto record=Battle(challenge,battle);if (!(Read(record,0)&1U)) return;
    for (std::uint32_t slot=0;slot<8;++slot) { // 전체는 비영 명단, 지정은 현재 값의 정확한 일치만 선택한다. 이동이 명단을 바꿔도 다음 현재 슬롯을 읽는다.
        const auto member=Read(record,0x20+slot*4);if (participant ? member!=*participant:!member) continue;if (!(Read(Player(challenge,member),0x64)&1U)) continue;SailOut(challenge,member,battle,true);Broadcast(challenge,PlayerChange(challenge,member));
    }
}
void ChallengeRuntime::StartBattle(std::uint32_t challenge,std::uint32_t battle) {
    auto record=Battle(challenge,battle);auto launch=Packet(0x1a,583);std::uint32_t count{},master{};std::optional<std::string> names;
    for (std::uint32_t slot=0;slot<8;++slot) { // 현재 명단을 원문 순서대로 읽고 각 참가자의bit20/공유22h 방송 뒤 같은71BYTE 레코드를 작성한다.
        const auto member=Read(record,0x20+slot*4);if (!member) continue;auto player=Player(challenge,member);Put(player,0x64,Read(player,0x64)|0x20U);Broadcast(challenge,PlayerChange(challenge,member));if (Read(player,0x64)&4U) master=Read(record,0x20+slot*4);const auto at=15U+count*71U;Put(launch,at,slot,1);Put(launch,at+1,Read(player,4));Put(launch,at+5,(Read(player,0x64)&0x800U) ? 1:0);std::fill_n(launch.begin()+at+41,30,std::uint8_t{});const auto current=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(Read(record,0x20+slot*4));const auto address=transport_.Connection(current).address;FixedText(launch,at+41,29,Need(hooks_.addressText)(address,false));FixedText(launch,at+9,32,std::string_view(reinterpret_cast<const char*>(player.data()+0x26),32));++count;if (!names) names.emplace();*names+='['+std::to_string(std::bit_cast<std::int32_t>(Read(player,4)))+']';
    }
    if (!names) throw std::out_of_range("R 전투 시작의 원문 미초기화 로그 문자열 읽기 거부");Need(hooks_.log)(0,"Battle Launch:"+*names+"\n");Put(launch,14,count,1);Copy(launch,4,record.subspan(8,6));std::uint32_t seed{};
    for (std::size_t byte=0;byte<4;++byte) seed=(seed<<8U)|(random_.Next()&0xffU); // 같은 R process 난수를 원문 순서의 네 번만 소비해 큰BYTE부터 seed를 만든다.
    Put(launch,10,seed);if (!master) Assert("masterId",5273);Send(challenge,master,launch);auto peer=transport_.Connection(ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(master)).address;if (Read(peer,0,2)==2 && Read(peer,4)==0x0100007fU) Need(hooks_.localAddress)(peer);const auto address=Need(hooks_.addressText)(peer,false);if (address.size()+1>30) throw std::out_of_range("R 전투 시작 주소의 실제30BYTE 범위");auto connect=Packet(0x1b,38,38);AllocationInput(connect);Header(connect,0x1b,38);PutText(connect,8,address);Put(connect,4,seed);
    for (std::uint32_t slot=0;slot<8;++slot) { const auto member=Read(record,0x20+slot*4);if (member) Send(challenge,member,connect); } // 현재 명단의1bh 송신이며 master 자신도 포함한다.
    for (std::uint32_t participant=0;participant<40;++participant) { const auto current=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(participant);if (current) transport_.Flush(current); } // 전투 안 명단뿐 아니라 현재CCh의40연결을 먼저 전부flush한다.
    for (std::uint32_t slot=0;slot<8;++slot) { const auto member=Read(record,0x20+slot*4);if (member) transport_.Terminate(ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(member)); } // 실제2번 종료가 명단을 지우므로 매번 현재8자리/연결을 다시 읽는다.
    auto removed=Shared(0x24);Put(removed,4,Read(record,4,1),1);Broadcast(challenge,removed);std::fill(record.begin(),record.end(),std::uint8_t{});Put(record,0x18,0xffffffffU);Put(record,0x1c,256);std::memcpy(record.data()+0x150,&clock_.game,8); // 삭제24h 방송 뒤에만 전체344BYTE를0/버전기본값·현재게임시각으로 재설정한다.
}
}
