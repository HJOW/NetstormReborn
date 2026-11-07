// 원문 길드/요청의 필드 순서와 공백/줄바꿈·정수 폭을 실제 사용자 홈 파일에 쓴다.
#include "root/SharedFiles.h"
#include "root/SharedFilesDetail.h"

namespace netstorm::root { // 설치 폴더를 수정하지 않고 기존 Saves 우선 reader와 같은 파일명을 쓴다.
using namespace shared_files_detail; // C문자열 첫NUL 경계와 DWORD의 signed 표현을 공유한다.
namespace { // 원문 vector size의 실제 unsigned DWORD 삽입 폭이다.
std::uint32_t Count(std::size_t value) { if (value>std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("R 길드 저장의 DWORD 개수 범위");return static_cast<std::uint32_t>(value); } // 원문 크기 밖 호스트 할당을 조용히 절삭하지 않는다.
}
void SharedFileStorage::SaveGuilds(std::span<const Guild> guilds) const {
    std::ofstream output(WritableFile("guild.txt"),std::ios::out|std::ios::trunc);output<<Count(guilds.size())<<std::endl;
    for (const auto& guild:guilds) { //5c220은 현재 행/회원 순서와 마지막 공백을 그대로 출력한다. 이름/비밀번호를 인용하거나 이스케이프하지 않는다.
        output<<std::bit_cast<std::int32_t>(guild.recruiting)<<' '<<CString(guild.name)<<' '<<CString(guild.password)<<' '<<Count(guild.members.size())<<' ';
        for (const auto subscriber:guild.members) output<<std::bit_cast<std::int32_t>(subscriber)<<' '; // 각 회원은 signed DWORD이며 중복도 같은 순서로 쓴다.
        output<<std::endl;
    }
}
void SharedFileStorage::SaveGuildRequests(std::span<const GuildRequest> requests) const {
    std::ofstream output(WritableFile("guildreq.txt"),std::ios::out|std::ios::trunc);output<<Count(requests.size())<<std::endl;
    for (const auto& request:requests) output<<CString(request.guild)<<' '<<CString(request.playerName)<<' '<<std::bit_cast<std::int32_t>(request.subscriber)<<std::endl<<' '<<std::bit_cast<std::int32_t>(request.approved)<<std::endl; //5dde0은 subscriber 뒤에도 endl을 넣어 승인값을 선두공백이 있는 별도 줄에 쓴다. 각 endl은 실제 flush한다.
}
}
