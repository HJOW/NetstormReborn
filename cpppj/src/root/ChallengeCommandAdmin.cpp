//87h 관리자 명단/현재 제재·하위소켓 닫힘과 실제 페이지 응답을 연결한다.
#include "root/CommandDetail.h"

namespace netstorm::root { // 별도 검사 상태/가짜 참가자나 연결을 만들지 않는다.
using namespace challenge_detail;using namespace command_detail; // 현재 x86 필드 폭과 페이지 치환을 공유한다.
std::span<std::uint8_t> ChallengeRuntime::CommandPlayer(std::uint32_t challenge,std::uint32_t participant) { auto bytes=challenges_.Bytes(challenge);const auto at=std::uint32_t(kChallengeParticipantOffset)+participant*std::uint32_t(kChallengeParticipantBytes);if (at>=bytes.size()) throw std::out_of_range("R 명령 참가자의 실제 CCh 물리 범위");return bytes.subspan(at); } // 원문 DWORD 곱/덧셈 뒤 실제 남은 물리만 빌리며 개별 필드는 Read/Put/Text가 확인한다.
std::string ChallengeRuntime::CommandPlayerRow(std::int32_t zone,std::int32_t participant,std::string_view source) {
    const auto current=state_.challenges.at(static_cast<std::size_t>(std::bit_cast<std::uint32_t>(zone)));if (!current) return {};const auto player=CommandPlayer(current,std::bit_cast<std::uint32_t>(participant));if (!(Read(player,0x64)&1U)) return {};std::string page(source);
    CommandPages::Replace(page,"{IP}",Text(player,0x76));CommandPages::ReplaceInteger(page,"{ID}",participant);CommandPages::Replace(page,"{NAME}",Text(player,0x26));CommandPages::ReplaceInteger(page,"{MAJOR}",static_cast<std::int32_t>(Read(player,0xb0,2)));CommandPages::ReplaceInteger(page,"{MINOR}",static_cast<std::int32_t>(Read(player,0xb2,2)));CommandPages::ReplaceInteger(page,"{ADMIN}",static_cast<std::int32_t>(Read(player,0x75,1)));CommandPages::ReplaceInteger(page,"{BATTLE}",Signed(Read(player,0xa4)));CommandPages::ReplaceInteger(page,"{SLOT}",Signed(Read(player,0xa8)));CommandPages::ReplaceInteger(page,"{SHUTUP}",static_cast<std::int32_t>(Read(player,0x74,1)));CommandPages::ReplaceInteger(page,"{ZONE}",zone);CommandPages::ReplaceInteger(page,"{SID}",Signed(Read(player,4)));CommandPages::Replace(page,"{MCOLOR}",Read(player,0x74,1) ? "~g":"~r");CommandPages::Replace(page,"{ACOLOR}",Read(player,0x75,1) ? "~g":"~r");return page; //452660은 현재 필드의 이 순서/부호와 비영 flag 색을 사용한다.
}
void ChallengeRuntime::AdminPageCommand(std::uint32_t challenge,std::uint32_t participant,std::string_view source,const std::string& command,const std::string& argument,std::string_view section) {
    if (!Read(CommandPlayer(challenge,participant),0x75,1)) return; // 비영권한만 검사하며 원문에 없는 소속/관리자값1 관문을 추가하지 않는다.
    if (command=="GETPLAYER") {
        const auto pair=Pair(argument);if (!pair) return;const auto [zone,first]=*pair;auto page=CommandPages::Section(source,"PlayerList");const auto pattern=CommandPages::Section(source,"Player");auto next=CommandPages::Section(source,"NEXTPLAYER");Need(hooks_.expandCommandPage)(page);Need(hooks_.expandCommandPage)(next);CommandPages::ReplaceInteger(page,"{LOCALZONE}",zone);CommandPages::ReplaceInteger(page,"{VIEWMIN}",first);CommandPages::ReplaceInteger(page,"{VIEWMAX}",Add(first,5));std::string rows;
        for (auto index=first;index<Add(first,6);index=Add(index,1)) rows+=CommandPlayerRow(zone,index,pattern); // 원문은 물리40칸과 별개인 최대50 안내/현재 시작부터6개를 그대로 조회한다.
        CommandPages::Replace(page,"{PLAYERLIST}",rows);CommandPages::ReplaceInteger(next,"{START}",Add(first,6));CommandPages::ReplaceInteger(next,"{ZONE}",zone);CommandPages::ReplaceInteger(next,"{MAXPLAYERS}",50);CommandPages::ReplaceInteger(next,"{BACK}",Add(first,-6));CommandPages::Replace(page,"{NAVMENU}",next);SendCommandPage(challenge,participant,page);return; // 이 분기는 빈페이지도 송신한다.
    }
    if (command=="SHUTUP" || command=="ADMIN" || command=="KICK" || command=="FORTBAN" || command=="IPBAN") {
        const auto pair=Pair(argument);if (!pair) return;const auto [zone,member]=*pair;const auto current=state_.challenges.at(static_cast<std::size_t>(std::bit_cast<std::uint32_t>(zone)));
        if (current) { auto target=CommandPlayer(current,std::bit_cast<std::uint32_t>(member));if (Read(target,0x64)&1U) {
            if (command=="SHUTUP") { const bool marked=Read(target,0x74,1)==0;CommandFiles().MarkSubscriber(Read(target,4),marked);Put(target,0x74,marked ? 1:0,1);CommandFiles().SaveMarkedSubscribers(); }
            else if (command=="ADMIN") Put(target,0x75,Read(target,0x75,1) ? 0:1,1);
            else { if (command=="FORTBAN") CommandFiles().BanAndSave(Read(target,4));else if (command=="IPBAN") CommandFiles().BanAddressAndSave(Text(target,0x76));const auto connection=Read(challenges_.Bytes(current),std::uint32_t(kChallengeConnectionOffset)+std::bit_cast<std::uint32_t>(member)*4U);transport_.CloseSocket(connection); } // 현재 하위 socket만 닫으며 노드 Terminate/참가자 즉시 삭제를 추가하지 않는다.
        }}SendCommandPage(challenge,participant,"<$Tell,ZSEND(ADMIN,GETPLAYER,1X1)>");return; // 현재 대상이 없거나 무효여도 원문 고정 새로고침 응답을 보낸다.
    }
    const auto page=CommandPage(source,section);if (!page.empty()) SendCommandPage(challenge,participant,page); // 알 수 없는 관리자 세부명령은 원문 요청의 페이지 이름을 그대로 사용한다.
}
}
