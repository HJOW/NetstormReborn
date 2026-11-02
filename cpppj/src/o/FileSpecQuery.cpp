// 원문459610/4595f0의 경로 작성과 검색 핸들 수명을 실제 Windows 장치에 연결한다.
#include "o/FileSpecQuery.h"
#include "platform/Utf8Path.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <stdexcept>

namespace netstorm::o { // 원문 경로 BYTE 규칙은 유지하고 OS 입력만 Unicode로 변환한다.
namespace { // 새로 여는 파일이나 디렉터리는 없으며 FindFirst/Close만 연결한다.
FileSpecFindResult WindowsFind(std::string_view name) { // 원본 ANSI와 다른 Windows10/11 호환: 한글/보충 문자 패턴을 W API로 전달한다.
    WIN32_FIND_DATAW data{};const auto handle=::FindFirstFileW(platform::PathFromUtf8(name).c_str(),&data);
    try { return {reinterpret_cast<std::uintptr_t>(handle),handle==INVALID_HANDLE_VALUE ? std::string{}:platform::PathToUtf8(data.cFileName)}; }
    catch (...) { static_cast<void>(::FindClose(handle));throw; } // Unicode 이름 변환/할당 실패에서도 방금 얻은 핸들을 잃지 않는다.
}
void WindowsClose(std::uintptr_t handle) { static_cast<void>(::FindClose(reinterpret_cast<HANDLE>(handle))); } // 원문은 실패 핸들/FindClose 반환값도 별도로 처리하지 않는다.
}
FileSpecQuery::Search::~Search() { if (handle) close(handle); } // 현재 호출이 실제 소유한 비NULL 검색 핸들만 한 번 반납한다.
FileSpecQuery::FileSpecQuery(std::string_view name,const FileSpecQueryHooks& hooks) { // 원문은 패턴이 있으면 정규화한 값 대신 원래 입력으로 다시 검색한다.
    name=name.substr(0,name.find('\0'));value_=name;
    if (name.find_first_of("\\:")==name.npos) value_=".\\"+value_;std::replace(value_.begin(),value_.end(),'/','\\');
    if (name.find_first_of("*?")==name.npos) return;
    if (static_cast<bool>(hooks.find)!=static_cast<bool>(hooks.close)) throw std::logic_error("FileSpec 검색/반납 장치는 함께 필요");
    search_.close=hooks.close ? hooks.close:WindowsClose;const auto found=hooks.find ? hooks.find(name):WindowsFind(name);search_.handle=found.handle;
    if (found.handle==reinterpret_cast<std::uintptr_t>(INVALID_HANDLE_VALUE)) { value_.clear();return; }
    const auto slash=name.find_last_of('\\');value_=slash==name.npos ? std::string{}:std::string(name.substr(0,slash+1));value_+=std::string_view(found.name).substr(0,found.name.find('\0'));
}
std::string_view FileSpecQuery::Text() const { return value_; } // 새 고리나 OS 조회를 만들지 않는다.
}
