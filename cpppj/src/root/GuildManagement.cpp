// 원문 R의 길드/가입요청 변경·조회와 현재 표의 실제 저장을 연결한다.
#include "root/SharedFiles.h"
#include "root/SharedFilesDetail.h"

namespace netstorm::root { // 기존 Config/로그인과 동일한 현재 길드/요청 표를 변경한다.
using namespace shared_files_detail; // 첫NUL/순서와 실제 숫자 폭을 공유한다.
std::optional<std::size_t> SharedFiles::GuildIndex(std::string_view name) const {
    name=CString(name);
    for (std::size_t index=0;index<tables_.guilds.size();++index) if (CString(tables_.guilds[index].name)==name) return index;return std::nullopt; //5c720은 strcmp의 대소문자/첫 일치 순서를 보존한다.
}
std::uint32_t SharedFiles::CreateGuild(std::string_view name,std::string_view password,std::uint32_t subscriber,GuildNameSource source) {
    if (GuildFor(subscriber)) return 2;if (GuildIndex(name)) return 3;if (source==GuildNameSource::OriginalEmptyLiteral) return 4;Guild guild;guild.name=CString(name);guild.recruiting=0;guild.password=CString(password);guild.members.push_back(subscriber);tables_.guilds.push_back(std::move(guild));storage_.SaveGuilds(tables_.guilds);return 1; //87h의 복사본은 빈 이름도 새 할당이므로 원문 리터럴 포인터 비교에 걸리지 않는다.
}
void SharedFiles::RemoveGuild(std::size_t index) { static_cast<void>(tables_.guilds.at(index));tables_.guilds.erase(tables_.guilds.begin()+index);storage_.SaveGuilds(tables_.guilds); } // 실제 레코드가 있는 물리만 제거하며 요청 표는 보존한다.
void SharedFiles::AddGuildMember(std::size_t index,std::uint32_t subscriber) { tables_.guilds.at(index).members.push_back(subscriber);storage_.SaveGuilds(tables_.guilds); }
void SharedFiles::LeaveGuild(std::size_t index,std::uint32_t subscriber) {
    auto& members=tables_.guilds.at(index).members;const auto found=std::find(members.begin(),members.end(),subscriber);if (found==members.end()) return;members.erase(found);if (tables_.guilds.at(index).members.empty()) RemoveGuild(index);else storage_.SaveGuilds(tables_.guilds); // 현재 첫 일치 한 개만 제거하며 마지막 회원이면 길드 자체를 제거한다.
}
void SharedFiles::ToggleGuildRecruiting(std::size_t index) { tables_.guilds.at(index).recruiting=tables_.guilds.at(index).recruiting==1 ? 0U:1U;storage_.SaveGuilds(tables_.guilds); }
void SharedFiles::SetGuildPassword(std::size_t index,std::string_view password) { tables_.guilds.at(index).password=CString(password);storage_.SaveGuilds(tables_.guilds); }
std::optional<std::size_t> SharedFiles::GuildLogin(std::string_view name,std::string_view password,std::uint32_t subscriber) const {
    if (!GuildFor(subscriber)) return std::nullopt;const auto index=GuildIndex(name);if (index && CString(tables_.guilds[*index].password)==CString(password)) return index;return std::nullopt; //5d960은 요청자가 어떤 길드에든 소속되어 있으면 지정한 길드의 비밀번호를 비교하며 같은 길드 소속인지 추가 검사하지 않는다.
}
std::uint32_t SharedFiles::RequestGuildJoin(std::string_view name,std::string_view playerName,std::uint32_t subscriber) {
    name=CString(name);const auto index=GuildIndex(name);if (!index) return 0;if (tables_.guilds[*index].recruiting!=1) return 2;if (GuildFor(subscriber)) return 1;
    for (const auto& request:tables_.guildRequests) if (CString(request.guild)==name && request.subscriber==subscriber) return 3; // 다른 길드 요청/승인상태는 중복 관문에 추가하지 않는다.
    GuildRequest request;request.guild=name;request.playerName=CString(playerName);request.subscriber=subscriber;request.approved=0;tables_.guildRequests.push_back(std::move(request));storage_.SaveGuildRequests(tables_.guildRequests);return 4;
}
bool SharedFiles::ApproveGuildRequest(std::string_view name,std::uint32_t subscriber) {
    name=CString(name);
    for (auto& request:tables_.guildRequests) if (CString(request.guild)==name && request.subscriber==subscriber) { request.approved=1;storage_.SaveGuildRequests(tables_.guildRequests);return true; }return false; //452f50은 첫 현재 요청만 승인하고 즉시 저장하며 이미1이어도 저장한다.
}
void SharedFiles::RemoveGuildRequest(std::size_t index) { static_cast<void>(tables_.guildRequests.at(index));tables_.guildRequests.erase(tables_.guildRequests.begin()+index);storage_.SaveGuildRequests(tables_.guildRequests); }
void SharedFiles::RemoveGuildRequestsFor(std::uint32_t subscriber) {
    if (tables_.guildRequests.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::out_of_range("R 가입요청 제거의 signed 개수 범위");auto index=static_cast<std::int32_t>(tables_.guildRequests.size());
    while (--index>0) if (tables_.guildRequests.at(static_cast<std::size_t>(index)).subscriber==subscriber) RemoveGuildRequest(static_cast<std::size_t>(index)); //5e210의 먼저 감소/양수 관문을 유지해0번은 지우지 않으며 각 실제 제거마다 파일을 저장한다.
}
}
