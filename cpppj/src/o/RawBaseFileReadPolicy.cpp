// 실제 FileSpec 장치 판정과 대소문자를 구분하는 바이트 접두 절삭을 보존한다.
#include "o/RawBaseFileReadPolicy.h"
#include <algorithm>
#include <stdexcept>

// 같은 조회 함수는 UI와 독립적으로 현재 VFS/명시 Native 재생 장치를 사용할 수 있다.
namespace netstorm::o {
// 원문 C문자열과 FileSpec의 선택된 판정만 공유한다.
namespace {
// 원문 strcpy/strchr는 첫 NUL까지만 입력으로 사용한다.
std::string_view Text(std::string_view value) { return value.substr(0,value.find('\0')); }
// 원문 FileSpec의 현재 기본 구분자 backslash로 변환한다.
std::string Slashes(std::string_view value) { std::string result(Text(value));std::replace(result.begin(),result.end(),'/','\\');return result; }
//459610은 원래 입력에 backslash/colon이 모두 없으면 .\를 먼저 붙인 뒤 슬래시를 바꾼다.
bool Device(std::string_view name) {
    if (name.find(':')!=name.npos) return true;
    return name.find('\\')!=name.npos && Slashes(name).starts_with("\\\\");
}
// 실제로 방문한 조회 장치만 요구한다.
const BaseFileReadLookup& Need(const BaseFileReadLookup& lookup) { if (!lookup) throw std::logic_error("BaseFile 읽기의 실제 조회 장치 미연결");return lookup; }
}
// 이 조합은 전체 경로를 정규화하지 않고 원문의 끝 구분자 한 개 제거/앞 구분자 제거만 적용한다.
std::string JoinBaseFileRoot(std::string_view root,std::string_view name) {
    auto result=Slashes(root);if (result.find_first_of("\\:")==result.npos) result=".\\"+result;
    if (result.size()>1 && result.back()=='\\') result.pop_back();if (result.empty() || result.back()!='\\') result+='\\';
    auto part=Slashes(name);part.erase(0,part.find_first_not_of('\\'));if (part.size()>1 && part.back()=='\\') part.pop_back();result+=part;return result;
}
// 장치 실패는 전체 root 접두인지 확인하지 않고 첫 불일치 BYTE까지의 공통 부분만 잘라낸다.
std::optional<std::vector<std::uint8_t>> ReadBaseFileByPolicy(std::string_view name,std::string_view primaryRoot,const BaseFileReadLookup& lookup) {
    name=Text(name);primaryRoot=Text(primaryRoot);
    if (Device(name)) {
        if (auto bytes=Need(lookup)(BaseFileReadSite::DirectDisk,name)) return bytes;
        std::size_t common{}; //원문 두 char 포인터가 함께 증가한 BYTE 수다.
        while (common<name.size() && common<primaryRoot.size() && name[common]==primaryRoot[common]) ++common; //디렉터리 경계/대소문자 정규화 관문을 추가하지 않는다.
        name.remove_prefix(common);
    }else if (auto bytes=Need(lookup)(BaseFileReadSite::PrimaryDisk,name)) return bytes;
    if (auto bytes=Need(lookup)(BaseFileReadSite::PrimaryArchive,name)) return bytes;
    if (auto bytes=Need(lookup)(BaseFileReadSite::SecondaryDisk,name)) return bytes;
    return Need(lookup)(BaseFileReadSite::SecondaryArchive,name);
}
}
