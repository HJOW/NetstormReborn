// 실제87h의 전체 길드/페이지 명령과 현재 파일·CCh 통지를 연결한다.
#include "root/CommandDetail.h"

namespace netstorm::root { //452f50의 입력 문자열 사본/현재 전역을 기존 부모에서 사용한다.
using namespace challenge_detail;using namespace command_detail; // 같은 비정렬 폭/페이지 이동과 실제 CRT를 공유한다.
SharedFiles& ChallengeRuntime::CommandFiles() { if (!hooks_.sharedFiles) throw std::logic_error("R 명령의 실제 공유 파일 owner 미연결");return *hooks_.sharedFiles; }
std::string ChallengeRuntime::CommandPage(std::string_view source,std::string_view section,bool expand) { auto page=CommandPages::Section(source,section);if (expand) Need(hooks_.expandCommandPage)(page);return page; }
std::string ChallengeRuntime::GuildRequestRows(std::string_view source,std::string_view guild,std::int32_t first,std::int32_t last) {
    const auto pattern=CommandPage(source,"GuildSingleRequest");std::string result;std::uint32_t ordinal{};
    for (std::size_t index=0;index<CommandFiles().Tables().guildRequests.size();++index) { //452960은 같은 길드만 순번을 올리고 승인/회원 필터 없이 끝까지 검사한다.
        if (CommandFiles().Tables().guildRequests[index].guild!=guild) continue;auto row=pattern;++ordinal;if (Signed(ordinal)<first || last<Signed(ordinal)) continue;CommandPages::ReplaceInteger(row,"{ID}",Signed(ordinal));CommandPages::Replace(row,"{PLAYERNAME}",CommandFiles().Tables().guildRequests[index].playerName);CommandPages::ReplaceInteger(row,"{SUBID}",Signed(CommandFiles().Tables().guildRequests[index].subscriber));result+=row;
    }return result;
}
std::string ChallengeRuntime::OpenGuildRows(std::string_view source,std::uint32_t subscriber,std::int32_t first,std::int32_t last) {
    const auto pattern=CommandPage(source,"GuildView");std::string result;std::uint32_t ordinal{};
    for (std::size_t index=0;index<CommandFiles().Tables().guilds.size();++index) { //452ba0은 모집상태1만 세며 요청 끝 순번을 넘은 첫 행에서 멈춘다.
        if (CommandFiles().Tables().guilds[index].recruiting!=1) continue;auto row=pattern;++ordinal;if (Signed(ordinal)<first || last<Signed(ordinal)) { if (last<Signed(ordinal)) break;continue; }CommandPages::ReplaceInteger(row,"{ID}",Signed(ordinal));CommandPages::Replace(row,"{GUILDNAME}",CommandFiles().Tables().guilds[index].name);CommandPages::ReplaceInteger(row,"{MEMBERS}",Signed(static_cast<std::uint32_t>(CommandFiles().Tables().guilds[index].members.size())));CommandPages::ReplaceInteger(row,"{SUBID}",Signed(subscriber));result+=row;
    }return result;
}
void ChallengeRuntime::ClearGuildName(std::uint32_t challenge,std::uint32_t subscriber) { auto packet=Packet(0x88,98,98);AllocationInput(packet);Header(packet,0x88,98);Put(packet,4,subscriber);Put(packet,8,0,1);Broadcast(challenge,packet); } //451ee0은 이름 첫BYTE만0으로 쓰며 나머지 지역 바이트/같은 CCh 전체 순서를 유지한다.
void ChallengeRuntime::Command(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> packet) {
    const auto source=Need(hooks_.commandTemplates)();const auto operation=Text(packet,0x22),first=Text(packet,0x72),second=Text(packet,0xc2),section=Text(packet,4); //451f30의 매요청 로드가 네 문자열 복사보다 앞선다. 원문의 복사 순서와 대소문자를 유지한다.
    const auto reply=[&](const std::string& page) { if (!page.empty()) SendCommandPage(challenge,participant,page); }; // 대부분의 원문 분기는 빈페이지일 때 송신을 생략한다.
    if (operation=="GET") { reply(CommandPage(source,section));return; }
    if (operation=="GUILD") { auto page=CommandPage(source,first);const auto index=CommandFiles().GuildFor(Read(CommandPlayer(challenge,participant),4));if (index) { const auto name=CommandFiles().Tables().guilds.at(*index).name;const auto status=CommandFiles().Tables().guilds.at(*index).recruiting==1 ? "ON":"OFF";CommandPages::Replace(page,"{GUILDNAME}",name);CommandPages::Replace(page,"{STATUS}",status); }reply(page);return; }
    if (operation=="GUILDREQAPROVE") { const auto index=CommandFiles().GuildFor(Read(CommandPlayer(challenge,participant),4));if (!index) return;const auto name=CommandFiles().Tables().guilds.at(*index).name;CommandFiles().ApproveGuildRequest(name,static_cast<std::uint32_t>(std::atoi(second.c_str())));return; }
    if (operation=="GUILDVIEWREQUEST") { const auto begin=std::atoi(first.c_str()),end=std::atoi(second.c_str());auto page=CommandPage(source,"GuildViewRequest");const auto index=CommandFiles().GuildFor(Read(CommandPlayer(challenge,participant),4));if (!index) return;const auto name=CommandFiles().Tables().guilds.at(*index).name;const auto rows=GuildRequestRows(source,name,begin,end);std::uint32_t count{};for (const auto& request:CommandFiles().Tables().guildRequests) if (request.guild==name) ++count;Navigation(page,begin,end,count,rows);reply(page);return; } //52e10의 요청수는 같은 길드명을 세며 승인여부는 읽지 않는다.
    if (operation=="GUILDSETPENDING") { const auto status=CommandFiles().RequestGuildJoin(first,Text(CommandPlayer(challenge,participant),0x26),Read(CommandPlayer(challenge,participant),4));const auto name=status==0 ? "GuildSubmitError3":(status==1 ? "GuildSubmitError1":(status==2 ? "GuildSubmitError4":(status==3 ? "GuildSubmitError2":"GuildRequestOk")));reply(CommandPage(source,name));return; }
    if (operation=="GUILDSUBMIT") { auto page=CommandPage(source,"GuildSubmitQuestion",false);CommandPages::Replace(page,"{GUILDNAME}",first);CommandPages::Replace(page,"{SUBID}",second);reply(page);return; } // 이 분기와 GUILDSETPASS에는 원문 공통TIME/존 치환 호출이 없다.
    if (operation=="GUILDJOIN") { const auto begin=std::atoi(first.c_str()),end=std::atoi(second.c_str());auto page=CommandPage(source,"GuildJoin");const auto rows=OpenGuildRows(source,Read(CommandPlayer(challenge,participant),4),begin,end);std::uint32_t count{};for (const auto& guild:CommandFiles().Tables().guilds) if (guild.recruiting==1) ++count;Navigation(page,begin,end,count,rows);reply(page);return; } //52ee0은 현재 모집상태1의 길드만 센다.
    if (operation=="GUILDTOGGLEREC") { const auto index=CommandFiles().GuildFor(Read(CommandPlayer(challenge,participant),4));if (!index) return;CommandFiles().ToggleGuildRecruiting(*index);auto page=CommandPage(source,"GuildRecruiting");CommandPages::Replace(page,"{STATUS}",CommandFiles().Tables().guilds.at(*index).recruiting==1 ? "ON":"OFF");reply(page);return; }
    if (operation=="GUILDSETPASS") { auto page=CommandPage(source,"GuildCheckPass",false);CommandPages::Replace(page,"{PASSWORD}",second);reply(page);return; }
    if (operation=="GUILDCHANGEIT") { const auto index=CommandFiles().GuildFor(Read(CommandPlayer(challenge,participant),4));if (!index) return;CommandFiles().SetGuildPassword(*index,first);reply(CommandPage(source,"GuildPassWorked"));return; }
    if (operation=="GUILDDISBAND") { const auto index=CommandFiles().GuildFor(Read(CommandPlayer(challenge,participant),4));if (!index) return;const auto old=CommandFiles().Tables().guilds.at(*index);CommandFiles().RemoveGuild(*index);for (const auto subscriber:old.members) ClearGuildName(challenge,subscriber);return; } // 원문은 제거 전에 길드/회원 사본을 만들고 그 회원들을 같은 CCh에 차례로 빈이름 방송한다.
    if (operation=="GUILDLOGIN") { const auto index=CommandFiles().GuildLogin(first,second,Read(CommandPlayer(challenge,participant),4));reply(CommandPage(source,index ? "GuildAdmin":"GuildLoginCode1"));return; }
    if (operation=="GUILDQUIT") { const auto index=CommandFiles().GuildFor(Read(CommandPlayer(challenge,participant),4));if (!index) return;CommandFiles().LeaveGuild(*index,Read(CommandPlayer(challenge,participant),4));reply(CommandPage(source,"QuitComplete"));ClearGuildName(challenge,Read(CommandPlayer(challenge,participant),4));return; }
    if (operation=="GUILDNEW") { const auto status=CommandFiles().CreateGuild(first,second,Read(CommandPlayer(challenge,participant),4));auto page=CommandPage(source,status==2 ? "GuildErrorCode2":(status==3 ? "GuildErrorCode3":"GuildCode1"));CommandPages::Replace(page,"{GUILDNAME}",first);reply(page);if (status!=2 && status!=3) SendGuildName(challenge,Read(CommandPlayer(challenge,participant),4),0xffffffffU);return; }
    if (operation=="ADMIN") AdminPageCommand(challenge,participant,source,first,second,section); // 나머지 상위 명령은 원문처럼 응답 없이 반환한다.
}
}
