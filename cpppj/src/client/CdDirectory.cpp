// ANSI/MAX_PATH 대신 Windows10/11 W API로 같은 CD-ROM/항목 순서와 깊이2를 처리한다.
#define NOMINMAX
#include "client/CdDirectory.h"
#include "o/RawBaseFileReadPolicy.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <windows.h>

namespace netstorm::client { // 같은 파일 탐색 판단을 실제 장치와 원문 독립 관측이 공유한다.
namespace { // 원본의 선택된 하위만 요구하는 내부 구현이다.
constexpr std::string_view kMarker="cd.netstorm.titanic.activision"; // 원본00532544의 CD 표시 파일 이름이다.
template<class F> const F& NeedCd(const F& callback) { if (!callback) throw std::logic_error("CD 탐색의 선택된 장치 미연결");return callback; } // 미선택 장치는 요구하지 않는다.
bool Marker(std::string_view name) { // 표시 이름은 ASCII이므로 원본 _stricmp의 해당 비교를 보존한다.
    if (name.size()!=kMarker.size()) return false;
    for (std::size_t i=0;i<name.size();++i) { const auto ch=static_cast<unsigned char>(name[i]);if ((ch>='A' && ch<='Z' ? ch+32 : ch)!=static_cast<unsigned char>(kMarker[i])) return false; } // 원래 각 이름 BYTE를 비교한다.
    return true;
}
int SearchCd(std::string directory,int depth,std::span<char> destination,const CdDirectoryHooks& h) { //004dd340의 깊이 우선·현재 항목 순서를 보존한다.
    if (!directory.empty() && directory.back()=='\\') directory.pop_back();const auto found=NeedCd(h.first)(directory+"\\*");if (!found) return 0;auto entry=std::optional<CdDirectoryEntry>(found->entry);
    while (entry) { // 다음 항목을 미리 모으지 않고 현재 항목/재귀가 끝난 뒤 FindNextFile을 호출한다.
        if (NeedCd(h.cancelled)()) return -1; // 원문은 이 가지에서 현재 핸들을 닫지 않고-1을 반환한다. 상위는 비영 성공으로 취급한다.
        if (Marker(entry->name)) { std::fill(destination.begin(),destination.end(),'\0');std::copy_n(directory.begin(),std::min(directory.size(),destination.size()-1),destination.begin());NeedCd(h.close)(found->handle);return 1; }
        if (!entry->name.empty() && entry->name[0]!='.' && entry->directory && depth>0) if (SearchCd(directory+"\\"+entry->name,depth-1,destination,h)!=0) { NeedCd(h.close)(found->handle);return 1; }
        entry=NeedCd(h.next)(found->handle);
    }
    NeedCd(h.close)(found->handle);return 0;
}
std::filesystem::path CdWide(std::string_view text) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()),text.size())); } // Windows10/11의 UTF-8 CD 경로를 손실 없이 W API에 전달한다.
std::string CdUtf8(const std::filesystem::path& path) { const auto value=path.u8string();return {value.begin(),value.end()}; } // 설치 언어와 무관하게 Config의 UTF-8 경로를 만든다.
CdDirectoryEntry CdEntry(const WIN32_FIND_DATAW& data) { return {CdUtf8(data.cFileName),(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0}; } // 실제 OS 이름/속성만 변환한다.
struct CdMediaErrorMode { // Windows10/11 호환성 확장: 빈 CD 장치의 시스템 오류 창으로 게임을 막지 않고 이 스레드의 이전 모드를 복구한다.
    DWORD previous{}; // 현재 스레드에서 실제로 교체한 이전 오류 모드다.
    bool changed{}; // 실패한 SetThreadErrorMode 뒤에 임의 모드를 복원하지 않는다.
    CdMediaErrorMode() { changed=SetThreadErrorMode(GetThreadErrorMode()|SEM_FAILCRITICALERRORS,&previous)!=FALSE; } // 원본에는 없는 현대 비대화식 장치 조회 경계다.
    ~CdMediaErrorMode() { if (changed) SetThreadErrorMode(previous,nullptr); } // 정상/예외 이탈 모두 현재 스레드에만 원래 설정을 돌려준다.
};
}
bool FindCdDirectory(std::span<char> destination,const CdDirectoryHooks& h) { // 원문 루트 열거가 모두 끝난 뒤에만 각 CD의 파일 탐색을 시작한다.
    if (destination.empty()) throw std::invalid_argument("CD 목적지 양수 길이 필요");std::vector<std::string> roots;
    for (auto& root:NeedCd(h.drives)()) if (NeedCd(h.driveType)(root)==5U) roots.push_back(std::move(root)); // 원래 GetLogicalDriveStrings 순서/종류5 필터다.
    for (const auto& root:roots) if (SearchCd(root,2,destination,h)!=0) return true; // 첫 비영 결과에서 이후 드라이브를 검사하지 않는다.
    return false;
}
std::string FindWindowsCdDirectory() { // 원본의 CD 자동 탐색은 유지하고 ANSI/고정 경로 버퍼만 현대 Windows용으로 확장한다.
    CdDirectoryHooks h;h.drives=[] { std::array<wchar_t,128> buffer{};const auto length=GetLogicalDriveStringsW(static_cast<DWORD>(buffer.size()),buffer.data());std::vector<std::string> roots;if (!length || length>=buffer.size()) return roots;
        for (const auto* root=buffer.data();*root;root+=std::char_traits<wchar_t>::length(root)+1) roots.push_back(CdUtf8(root));return roots; // 원본과 같은 최대26개 루트의 이중 NUL 순서다.
    };h.driveType=[](auto root) { return static_cast<std::uint32_t>(GetDriveTypeW(CdWide(root).c_str())); };
    h.first=[](auto pattern) -> std::optional<CdDirectoryFirst> { WIN32_FIND_DATAW data{};const auto handle=FindFirstFileW(CdWide(pattern).c_str(),&data);if (handle==INVALID_HANDLE_VALUE) return {};return CdDirectoryFirst{reinterpret_cast<std::uintptr_t>(handle),CdEntry(data)}; };
    h.next=[](auto handle) -> std::optional<CdDirectoryEntry> { WIN32_FIND_DATAW data{};if (!FindNextFileW(reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(handle)),&data)) return {};return CdEntry(data); };
    h.close=[](auto handle) { FindClose(reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(handle))); };h.cancelled=[] { return false; }; // 시작 설정 단계에는 아직 중단 입력 처리기가 없다.
    std::vector<char> output(4U*32768U); // 원본255BYTE 대신 Windows10/11의 긴 Unicode 경로가 UTF-8 중간에서 잘리지 않도록 충분히 확보한다.
    return FindCdDirectory(output,h) ? std::string(output.data()) : std::string();
}
// 루트는 ASCII 드라이브 글자이므로 원문 A 열거를 그대로 쓰고 파일 경로만 현대 Windows의 W API로 전달한다.
CdPresenceHooks MakeWindowsCdPresenceHooks() {
    CdPresenceHooks hooks;hooks.path=[](auto root,auto marker) { return o::JoinBaseFileRoot(root,marker); };
    hooks.exists=[](auto path) { CdMediaErrorMode mode;return GetFileAttributesW(CdWide(path).c_str())!=INVALID_FILE_ATTRIBUTES; }; // 원본 ANSI와 다른 Windows10/11 UTF-8/native 경로 호환이며 디렉터리 비영도 유지한다.
    hooks.roots=[](std::span<char,128> output) {
        CdDriveRootsHooks roots;roots.logicalDrives=[](std::span<char,128> buffer) { return static_cast<std::uint32_t>(GetLogicalDriveStringsA(128,buffer.data())); };roots.driveType=[](auto root) { const std::string terminated(root);return static_cast<std::uint32_t>(GetDriveTypeA(terminated.c_str())); };
        std::array<char,128> scratch;return CollectCdDriveRoots(128,output,scratch,roots);
    };
    return hooks;
}
}
