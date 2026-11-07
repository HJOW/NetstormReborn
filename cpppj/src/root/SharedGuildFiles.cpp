// 길드/가입 요청의 현재 순서와 회원·길드 이름 조건을 실제 공유 파일에서 복원한다.
#include "root/SharedFiles.h"
#include "root/SharedFilesDetail.h"

namespace netstorm::root { // CCh의 현재 길드 이름 조회와 같은 실제 표다.
using namespace shared_files_detail; // signed DWORD 추출 실패/char 지역 물리 경계를 공유한다.
std::optional<std::size_t> SharedFiles::GuildFor(std::uint32_t subscriber) const {
    for (std::size_t index=0;index<tables_.guilds.size();++index) { const auto& members=tables_.guilds[index].members;if (std::find(members.begin(),members.end(),subscriber)!=members.end()) return index; } //0045d7e0의 길드/회원 현재 순서상 첫 일치다.
    return std::nullopt;
}
std::optional<std::string> SharedFiles::GuildName(std::uint32_t subscriber) const { const auto index=GuildFor(subscriber);if (!index) return std::nullopt;return tables_.guilds[*index].name; }
void SharedFiles::LoadGuilds() {
    tables_.guilds.clear();auto input=storage_.OpenWorking("guild.txt");if (!input.is_open()) return; // 입력 파일이 없을 때의 원문 미초기화 count는 빈 호스트 표로 정의한다. 정상 행은 원문 순서/폭을 따른다.
    const auto count=RequiredInteger(input);
    for (std::int32_t row=0;row<count;++row) { // 파일의 signed 행 수가 음수면 원문처럼 진입하지 않는다.
        Guild guild;guild.recruiting=std::bit_cast<std::uint32_t>(RequiredInteger(input));guild.name=RequiredChars(input,32);guild.password=RequiredChars(input,32);const auto members=RequiredInteger(input);
        for (std::int32_t member=0;member<members;++member) { std::int32_t subscriber{};ReadInteger(input,subscriber);guild.members.push_back(std::bit_cast<std::uint32_t>(subscriber)); } // 원문은 각 회원 지역을0으로 초기화하고 추출 실패도 현재0을 삽입한다.
        tables_.guilds.push_back(std::move(guild));
    }
}
void SharedFiles::LoadGuildRequests() {
    tables_.guildRequests.clear();auto input=storage_.OpenWorking("guildreq.txt");if (!input.is_open()) return;const auto count=RequiredInteger(input);
    for (std::int32_t row=0;row<count;++row) { // 원문처럼 모든 행을 먼저 읽고 현재 길드 존재/회원 여부를 검사한다.
        GuildRequest request;request.guild=RequiredChars(input,40);request.playerName=RequiredChars(input,40);request.subscriber=std::bit_cast<std::uint32_t>(RequiredInteger(input));request.approved=std::bit_cast<std::uint32_t>(RequiredInteger(input));
        if (std::find_if(tables_.guilds.begin(),tables_.guilds.end(),[&](const auto& guild) { return guild.name==request.guild; })!=tables_.guilds.end() && !GuildFor(request.subscriber)) tables_.guildRequests.push_back(std::move(request)); //0045c720은 대소문자를 구별한strcmp이며 같은 길드 이름의 첫 존재만 필요하다.
    }
}
}
