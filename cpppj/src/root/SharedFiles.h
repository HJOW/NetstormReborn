// R의 현재 관리 표/길드·뉴스와 실제 사용자 홈 파일 장치를 연결한다.
#pragma once
#include "root/ChallengeRuntime.h"
#include "platform/UserDataRoot.h"
#include <filesystem>
#include <fstream>
#include <vector>

namespace netstorm::root { // 원문 관리 파일의 현재 저장소와 프로세스 장치다.
struct Guild { //0045cae0의 현재 길드 레코드이며 문자열/회원은 호스트가 소유한다.
    std::uint32_t id{}; // 원문 첫 DWORD 길드 식별자다.
    std::string name,contact; //+4/+c의 두 C문자열 값이며 파일 입력은 각각32BYTE 지역을 거친다.
    std::vector<std::uint32_t> members; //+14의 현재 회원 DWORD 목록이며 중복/원래 순서를 보존한다.
};
struct GuildRequest { //0045dd00의 현재 길드 가입 요청이다.
    std::string guild,contact; // 두40BYTE 지역 입력에서 복사한 현재 C문자열 값이다.
    std::uint32_t subscriber{},value{}; //+10/+14의 입력 두 DWORD다.
};
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
    std::ifstream OpenWorking(std::string_view name) const; // 사용자 Saves 우선, 그 외 원문 CWD의 실제 텍스트 입력을 연다.
    std::filesystem::path DataFile(std::string_view name) const; // 사용자 Saves 우선, 그 외 현재 dataPrefix의 실제 전체 읽기/시각 경로다.
    std::filesystem::path WorkingFile(const std::filesystem::path& relative) const; // 사용자 Saves의 같은 파일명 우선,그 외 현재 workingDirectory와 원문 상대 경로를 결합한다.
    std::filesystem::path WritableFile(std::string_view name) const; // 현재 사용자 Saves를 만들고 단일 생성 파일의 실제 wide 경로만 반환한다.
    void SaveBlacklist(std::span<const std::uint32_t> subscribers) const; //00458770의 현재 순서/정수·endl을 사용자 Saves/blacklist.txt에 쓴다.
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
    std::optional<std::string> GuildName(std::uint32_t subscriber) const; //0045d7e0의 첫 회원 일치 길드 이름이다.
private:
    SharedFileTables& tables_;ChallengeMessages& messages_;SharedFileStorage& storage_; // 현재 표·CCh 뉴스와 같은 실제 파일 장치다.
    std::function<void(std::int32_t,std::string_view)> log_; // 선택된 실제 원문 level/완성된 로그 장치다.
    void AppendSubscribers(std::string_view name,std::vector<std::uint32_t>& values); //00479df0/79f00의 signed DWORD 추출·중복 삽입이며 기존 표를 지우지 않는다.
    void LoadAddresses(); //00479c30의 공백 구분 string 추출/현재 표에 정규화·중복 제외 삽입이다.
    void LoadGuilds();void LoadGuildRequests(); //0045cda0/5da40의 현재 목록 초기화·파일 행/회원/유효 요청 필터다.
    void LoadFilter(); //0047a010의32BYTE char 토큰과 '/' 연결 결과로 현재 문자열을 교체한다.
    std::optional<std::size_t> GuildFor(std::uint32_t subscriber) const; // 현재 순서의 첫 소속 길드 번호다.
};
}
