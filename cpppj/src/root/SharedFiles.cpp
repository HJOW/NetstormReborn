// 실제 관리 표 재로드·IP 비교와 뉴스/blacklist 시각 감지의 현재 상태를 복원한다.
#include "root/SharedFiles.h"
#include "root/SharedFilesDetail.h"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace netstorm::root { // 현재 표/뉴스를 Config와 CCh가 같은 참조로 사용한다.
using namespace shared_files_detail; // 실제 스트림/숫자 표와 첫NUL 경계를 공유한다.
namespace { // 원문 주소/전체 파일 장치를 같은 현재 입력에 적용한다.
std::string Address(std::string_view address) { address=CString(address);if (address.size()>15) throw std::out_of_range("R IP 문자열의 실제16BYTE 지역 범위");return std::string(address); } //004576c0/57800/57a50의 strcpy 물리다.
bool AddressMatches(std::string_view first,std::string_view second) {
    if (first.size()!=second.size()) return false;
    for (std::size_t index=0;index<first.size()-index;++index) if (first[index]!=second[index] && first[index]!='*' && second[index]!='*') return false; //00457800은 포인터를 전진시키며 매회 줄어든 strlen과 index를 비교한다. 길이15면 앞8BYTE만 비교하는 실제 특이동작이다.
    return true;
} //00457970은 양쪽 어느 쪽의 '*'도 한 문자 일치로 처리한다.
std::string NormalizeAddress(std::string_view source) {
    const auto input=Address(source);std::string output;std::size_t first{};
    for (;;) { //00457a50처럼 빈 구간도000이며 각 '.' 구간을3문자로 왼쪽0 보충한다.
        const auto dot=input.find('.',first);const auto count=(dot==std::string::npos ? input.size():dot)-first;if (count>3) throw std::out_of_range("R IP 구간의 실제4BYTE 지역 범위");output.append(3-count,'0');output.append(input,first,count);if (dot==std::string::npos) break;output+='.';first=dot+1;
    }
    return Address(output);
} // 출력은 원문 현재16BYTE에 저장 가능한 실제 NUL 문자열이어야 한다.
std::optional<std::int32_t> ModificationTime(const std::filesystem::path& path) {
    struct __stat64 status{};if (::_wstat64(path.c_str(),&status)!=0) return std::nullopt;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(status.st_mtime));
} // 실제 CRT stat 초 단위 mtime을 원문32비트 저장 폭으로 전달한다.
std::optional<std::string> ReadWhole(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);if (!input) return std::nullopt;const auto length=input.tellg();if (length<0 || length>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("R 전체 파일 길이의 signed DWORD 범위");std::string bytes(static_cast<std::size_t>(length),'\0');input.seekg(0);if (!bytes.empty() && !input.read(bytes.data(),static_cast<std::streamsize>(bytes.size()))) return std::nullopt;return bytes;
} //00448860의 실제 파일 바이트/추가NUL은 호스트 string 소유권으로 보관한다.
}
SharedFiles::SharedFiles(SharedFileTables& tables,ChallengeMessages& messages,SharedFileStorage& storage,std::function<void(std::int32_t,std::string_view)> log)
    :tables_(tables),messages_(messages),storage_(storage),log_(std::move(log)) {}
void SharedFiles::Bind(ConfigurationHooks& config,ChallengeRuntimeHooks& challenge) {
    challenge.sharedFiles=this; //87h가 기존 로더/로그인 조회와 다른 길드 표나 저장 경로를 만들지 않도록 같은 실제 owner를 빌린다.
    config.reloadModeration=[this] { Reload(); };challenge.blockedSubscriber=[this](auto subscriber) { return BlockedSubscriber(subscriber); };challenge.markedSubscriber=[this](auto subscriber) { return MarkedSubscriber(subscriber); };challenge.banAndSave=[this](auto subscriber) { BanAndSave(subscriber); };challenge.blockedAddress=[this](auto address) { return BlockedAddress(address); };challenge.guildName=[this](auto subscriber) { return GuildName(subscriber); };
}
bool SharedFiles::BlockedSubscriber(std::uint32_t subscriber) const { return std::find(tables_.blockedSubscribers.begin(),tables_.blockedSubscribers.end(),subscriber)!=tables_.blockedSubscribers.end(); }
bool SharedFiles::MarkedSubscriber(std::uint32_t subscriber) const { return std::find(tables_.markedSubscribers.begin(),tables_.markedSubscribers.end(),subscriber)!=tables_.markedSubscribers.end(); }
bool SharedFiles::BlockedAddress(std::string_view address) const {
    const auto input=Address(address);
    for (const auto& current:tables_.blockedAddresses) if (AddressMatches(CString(std::string_view(current.data(),current.size())),input)) return true; // 원문은 현재 삽입 순서대로 조회하며 입력을 다시 정규화하지 않는다.
    return false;
}
void SharedFiles::AppendSubscribers(std::string_view name,std::vector<std::uint32_t>& values) { auto input=storage_.OpenWorking(name);std::int32_t value{};while (ReadInteger(input,value)) Insert(values,std::bit_cast<std::uint32_t>(value)); } // signed 추출 실패에서 멈추며 현재 표를 비우지 않는다.
void SharedFiles::LoadAddresses() {
    auto input=storage_.OpenWorking("ipbanlist.txt");std::string token;
    while (input>>token) { //0047a1f0은 getline이 아닌 공백 구분 std::string 추출이며 NUL 바이트도 문자열 입력에 들어갈 수 있다.
        const auto normalized=NormalizeAddress(token);if (BlockedAddress(normalized)) continue;std::array<char,16> bytes{};std::copy(normalized.begin(),normalized.end(),bytes.begin());tables_.blockedAddresses.push_back(bytes);
    }
}
void SharedFiles::LoadFilter() {
    auto input=storage_.OpenWorking("filter.txt");std::string result;bool first=true;
    while (const auto token=ReadChars(input,32)) { if (!first) result+='/';result+=*token;first=false; } // 순서/중복을 유지하고 파일이 없으면 빈 결과로 교체한다.
    tables_.filter=std::move(result);
}
void SharedFiles::Reload() { AppendSubscribers("blacklist.txt",tables_.blockedSubscribers);LoadAddresses();AppendSubscribers("chatlist.txt",tables_.markedSubscribers);LoadGuilds();LoadGuildRequests();LoadFilter(); }
void SharedFiles::BanAndSave(std::uint32_t subscriber) { Insert(tables_.blockedSubscribers,subscriber);storage_.SaveBlacklist(tables_.blockedSubscribers); }
void SharedFiles::CheckNews() {
    if (!log_) throw std::logic_error("R 뉴스의 실제 로그 장치 미연결");log_(29,"Checking for new news..\n");const auto path=storage_.DataFile("news.txt");const auto changed=ModificationTime(path);if (!changed || *changed==tables_.newsTime) return;log_(29,"New news!\n");tables_.newsTime=*changed;messages_.news.reset();messages_.news=ReadWhole(path);messages_.newsChecksum=std::bit_cast<std::uint32_t>(tables_.newsTime);if (!messages_.news) log_(9,"Failed to load the news file.\n");
}
void SharedFiles::CheckBlacklist() {
    const auto path=storage_.DataFile("blacklist.txt");const auto changed=ModificationTime(path);if (!changed || *changed==tables_.blacklistTime) return;if (!log_) throw std::logic_error("R blacklist의 실제 로그 장치 미연결");log_(29,"New blacklist!\n");tables_.blacklistTime=*changed;tables_.blockedSubscribers.clear();auto bytes=ReadWhole(path);if (!bytes) { log_(9,"Failed to load the blacklist file.\n");return; }std::size_t first{};
    while (first<bytes->size()) { //00478dd0의 LF별 sscanf는 각 행의 선두 정수 하나만 읽으며 embeddedNUL 뒤의 줄은 탐색하지 않는다.
        const auto current=bytes->c_str()+first;const auto newline=std::strchr(current,'\n');const auto line=newline ? std::string(current,static_cast<std::size_t>(newline-current)):std::string(current);int value{};if (std::sscanf(line.c_str()," %d",&value)==1) Insert(tables_.blockedSubscribers,std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(value)));if (!newline) break;first=static_cast<std::size_t>(newline-bytes->c_str())+1;
    }
}
}
