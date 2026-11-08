// R의 현재 관리 표/길드·뉴스와 실제 사용자 홈 파일 장치를 연결한다.
#pragma once
#include "root/ChallengeRuntime.h"
#include "platform/UserDataRoot.h"
#include <filesystem>
#include <fstream>
#include <vector>

namespace netstorm::root { // 원문 관리 파일의 현재 저장소와 프로세스 장치다.
struct Guild { //0045cae0의 현재 길드 레코드이며 문자열/회원은 호스트가 소유한다.
    std::uint32_t recruiting{}; // 첫DWORD는 모집 상태다. 원문1만ON이며 toggle은1→0/그외→1로 쓴다. 길드 번호는 vector 위치다.
    std::string name,password; //+4/+c의 실제 길드 이름/비밀번호이며 파일 입력은 각각32BYTE 지역을 거친다.
    std::vector<std::uint32_t> members; //+14의 현재 회원 DWORD 목록이며 중복/원래 순서를 보존한다.
};
struct GuildRequest { //0045dd00의 현재 길드 가입 요청이다.
    std::string guild,playerName; // 두40BYTE 지역 입력에서 복사한 현재 길드 이름/요청자 이름이다.
    std::uint32_t subscriber{},approved{}; //+10/+14의 실제 구독자/승인 상태 DWORD이며 임의 bool로 축소하지 않는다.
};
enum class GuildNameSource { CopiedString,OriginalEmptyLiteral }; //5c9d0의 빈문자열 내용 비교가 아닌 원문 리터럴 포인터 동일성을 구별한다.87h의 CharString 복사본은 빈값도 CopiedString이다.
struct SharedFileTables { // R의 서로 다른 전역을 하나의 수명 owner에서 보관하되 같은 표로 합치지 않는다.
    std::vector<std::uint32_t> blockedSubscribers,markedSubscribers; //004d7220/004d723c이며 보통 재해석은 현재 표에 추가한다.
    std::vector<std::array<char,16>> blockedAddresses; //004d23e8의 현재16BYTE 정규화 주소/와일드카드 목록이다.
    std::vector<Guild> guilds;std::vector<GuildRequest> guildRequests; //004d0870/004d06cc이며 파일 재로드마다 각각 비운다.
    std::string filter; //004d2328의 토큰을 '/'로 잇는 현재 문자열이다.
    std::int32_t newsTime{},blacklistTime{}; //004d2318/1c의 BSS0 변경 시각이며 뉴스 checksum과 별도다.
};
class SharedFileStorage final { // 원문 CWD 입력과 dataPrefix 입력을 구분하고 모든 자동 쓰기를 Saves로 보낸다.
public:
    SharedFileStorage(std::filesystem::path workingDirectory,std::filesystem::path dataDirectory,platform::UserDataRoot user); // 경로만 보관하며 파일/폴더를 생성하지 않는다.
    void DataDirectory(std::filesystem::path directory); // Configuration::DataPrefix가 확정된 뒤 실제 최종 owner가 갱신한다.
    std::ifstream OpenWorking(std::string_view name) const; // 사용자 Saves→원문 CWD 순서의 텍스트 입력이다. 세.server만 둘 다 없으면 모듈 옆 server 자산을 읽는다.
    std::filesystem::path DataFile(std::string_view name) const; // 사용자 Saves 우선, 그 외 현재 dataPrefix의 실제 전체 읽기/시각 경로다.
    std::filesystem::path WorkingFile(const std::filesystem::path& relative) const; // 사용자 Saves의 같은 파일명 우선,그 외 현재 workingDirectory와 원문 상대 경로를 결합한다.
    std::filesystem::path WritableFile(std::string_view name) const; // 현재 사용자 Saves를 만들고 단일 생성 파일의 실제 wide 경로만 반환한다.
    void SaveBlacklist(std::span<const std::uint32_t> subscribers) const; //00458770의 현재 순서/정수·endl을 사용자 Saves/blacklist.txt에 쓴다.
    void SaveSubscribers(std::string_view name,std::span<const std::uint32_t> subscribers) const; //58770/58860의 현재 signed DWORD/endl 목록을 같은 사용자 Saves 단일파일에 쓴다.
    void SaveAddresses(std::span<const std::array<char,16>> addresses) const; //458060의 현재 IP 문자열/endl 목록을 사용자 Saves/ipbanlist.txt에 쓴다.
    void SaveGuilds(std::span<const Guild> guilds) const; //0045c220의 현재 행/회원·공백/endl을 사용자 Saves/guild.txt에 실제 저장한다.
    void SaveGuildRequests(std::span<const GuildRequest> requests) const; //0045dde0의 요청당 두줄/중간공백·signed DWORD를 사용자 Saves/guildreq.txt에 쓴다.
private:
    std::filesystem::path workingDirectory_,dataDirectory_;platform::UserDataRoot user_; // 실제 프로세스 CWD/현재 데이터 폴더와 기존 사용자 저장 정책이다.
    std::filesystem::path Select(std::string_view name,const std::filesystem::path& fallback) const; // 존재하는 사용자 파일을 우선하며 설치 경로로 쓰기를 되돌리지 않는다.
};
class SharedFiles final { // 같은 현재 표/뉴스/파일 장치가 Config와 CCh callback보다 오래 살아야 한다.
public:
    SharedFiles(SharedFileTables& tables,ChallengeMessages& messages,SharedFileStorage& storage,std::function<void(std::int32_t,std::string_view)> log); // 생성은 IO/등록을 수행하지 않는다.
    SharedFiles(const SharedFiles&)=delete;SharedFiles& operator=(const SharedFiles&)=delete; // callback이 빌리는 현재 owner를 복제하지 않는다.
    void Bind(ConfigurationHooks& config,ChallengeRuntimeHooks& challenge); // 실제 reload/회원·IP/자동ban 저장/길드 조회 장치를 같은 owner에 연결한다.
    void Reload(); //00477570 선두의 blacklist/ipban/chatlist/guild/guildreq/filter 순서를 수행한다.
    void CheckNews(); //00478ca0의 변경 시각/이전 뉴스 반납·전체 읽기/mtime checksum을 현재 messages에 반영한다.
    void CheckBlacklist(); //00478dd0의 변경 시각 감지 때만 현재 표를 비우고 LF별 sscanf로 다시 읽는다.
    bool BlockedSubscriber(std::uint32_t subscriber) const;bool MarkedSubscriber(std::uint32_t subscriber) const; // 서로 다른 현재 숫자 표를 조회한다.
    bool BlockedAddress(std::string_view address) const; //004576c0: 입력을 새로 정규화하지 않고 현재 표/원문 축소 strlen 반복으로 비교한다.
    void BanAndSave(std::uint32_t subscriber); // 중복 없이 현재 표에 삽입한 뒤 삽입 여부와 무관하게 전체 현재 목록을 저장한다.
    void MarkSubscriber(std::uint32_t subscriber,bool marked); //87h SHUTUP의 현재 chatlist 삽입/첫삭제만 수행하며 실제 참가자 flag/저장은 명령 부모가 순서대로 호출한다.
    void SaveMarkedSubscribers(); //458860의 현재 전체 chatlist를 실제 사용자 홈에 저장한다.
    void BanAddressAndSave(std::string_view address); //457630의 현재 주소정규화/중복비교·삽입 뒤458060 전체저장이다.
    std::optional<std::string> GuildName(std::uint32_t subscriber) const; //0045d7e0의 첫 회원 일치 길드 이름이다.
    const SharedFileTables& Tables() const { return tables_; } // 실제 길드/요청·관리 표를 읽기만 빌린다. 변경/재로드 뒤 레코드 참조를 재사용하지 않는다.
    std::optional<std::size_t> GuildFor(std::uint32_t subscriber) const; //0045d7e0의 현재 순서상 첫 소속 길드 번호다. 회원0/중복도 별도 필터 없이 읽는다.
    std::optional<std::size_t> GuildIndex(std::string_view name) const; //0045c720의 대소문자 구분 이름 첫 일치이며 실패는 NULL이다.
    std::uint32_t CreateGuild(std::string_view name,std::string_view password,std::uint32_t subscriber,GuildNameSource source=GuildNameSource::CopiedString); //0045c9d0의 기존회원2/중복이름3/원문빈리터럴4/생성·저장1을 반환한다. 빈 내용 자체를 금지하지 않는다.
    void RemoveGuild(std::size_t index); //0045c7a0의 현재 길드 한 행 제거→전체저장이다. 가입요청을 함께 제거하지 않는다.
    void AddGuildMember(std::size_t index,std::uint32_t subscriber); //0045cd60은 중복 검사 없이 실제 회원 목록 끝에 추가→저장한다. 원문 실제 caller0인 보조 몸체다.
    void LeaveGuild(std::size_t index,std::uint32_t subscriber); //0045d3e0은 첫 회원만 제거하고 마지막이면 길드제거/저장,미일치면 아무 쓰기도 하지 않는다.
    void ToggleGuildRecruiting(std::size_t index); //0045d730의 정확히1→0/그외→1 쓰기 뒤 전체 길드를 저장한다.
    void SetGuildPassword(std::size_t index,std::string_view password); //0045d7a0의 현재 비밀번호 교체→전체저장이다. 원문에 없는 길이/권한 관문을 추가하지 않는다.
    std::optional<std::size_t> GuildLogin(std::string_view name,std::string_view password,std::uint32_t subscriber) const; //0045d960은 요청자가 어느 길드든 회원이면 지정 이름/비밀번호를 비교한다. 같은 길드 소속 관문으로 강화하지 않는다.
    std::uint32_t RequestGuildJoin(std::string_view name,std::string_view playerName,std::uint32_t subscriber); //0045dfe0의 미존재0/모집아님2/기존회원1/중복요청3/추가·저장4를 원문 순서로 반환한다.
    bool ApproveGuildRequest(std::string_view name,std::uint32_t subscriber); //87h GUILDREQAPROVE의 첫 길드명/구독자 일치 요청에 승인1→전체저장하며 소속/권한 판단은 실제 부모가 수행한다.
    void RemoveGuildRequest(std::size_t index); //0045e1b0의 현재 요청 한행 제거→전체저장이다.
    void RemoveGuildRequestsFor(std::uint32_t subscriber); //0045e210의 뒤에서부터 index>0만 검사/각삭제마다저장한다. 원문의0번 누락과 caller0을 유지한다.
private:
    SharedFileTables& tables_;ChallengeMessages& messages_;SharedFileStorage& storage_; // 현재 표·CCh 뉴스와 같은 실제 파일 장치다.
    std::function<void(std::int32_t,std::string_view)> log_; // 선택된 실제 원문 level/완성된 로그 장치다.
    void AppendSubscribers(std::string_view name,std::vector<std::uint32_t>& values); //00479df0/79f00의 signed DWORD 추출·중복 삽입이며 기존 표를 지우지 않는다.
    void LoadAddresses(); //00479c30의 공백 구분 string 추출/현재 표에 정규화·중복 제외 삽입이다.
    void LoadGuilds();void LoadGuildRequests(); //0045cda0/5da40의 현재 목록 초기화·파일 행/회원/유효 요청 필터다.
    void LoadFilter(); //0047a010의32BYTE char 토큰과 '/' 연결 결과로 현재 문자열을 교체한다.
};
}
