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
std::optional<std::string> WindowsNext(std::uintptr_t handle) { WIN32_FIND_DATAW data{};if (!::FindNextFileW(reinterpret_cast<HANDLE>(handle),&data)) return {};return platform::PathToUtf8(data.cFileName); } // 원본A대신Windows10/11의Unicode이름을같은OS열거순서로받는다.
}
FileSpecQuery::Search::~Search() { if (handle) close(handle); } // 현재 호출이 실제 소유한 비NULL 검색 핸들만 한 번 반납한다.
FileSpecQuery::FileSpecQuery(std::string_view name,const FileSpecQueryHooks& hooks):hooks_(hooks) { Assign(name); } // 재대입과같은최초검색/부분실패수명을사용한다.
void FileSpecQuery::Assign(std::string_view name) { // 원문은패턴이있으면정규화한값대신원래입력으로다시검색한다.
    const auto input=std::string(name.substr(0,name.find('\0')));value_=input;
    if (input.find_first_of("\\:")==input.npos) value_=".\\"+value_;std::replace(value_.begin(),value_.end(),'/','\\');valid_=true;
    if (search_.handle) { search_.close(search_.handle);search_.handle=0; }
    if (input.find_first_of("*?")==input.npos) return;
    if (static_cast<bool>(hooks_.find)!=static_cast<bool>(hooks_.close)) throw std::logic_error("FileSpec 검색/반납 장치는 함께 필요");
    search_.close=hooks_.close ? hooks_.close:WindowsClose;const auto found=hooks_.find ? hooks_.find(input):WindowsFind(input);search_.handle=found.handle;
    valid_=found.handle!=reinterpret_cast<std::uintptr_t>(INVALID_HANDLE_VALUE);if (!valid_) { value_.clear();return; }
    const auto slash=input.find_last_of('\\');value_=slash==input.npos ? std::string{}:input.substr(0,slash+1);value_+=std::string_view(found.name).substr(0,found.name.find('\0'));
}
std::string_view FileSpecQuery::Text() const { return value_; } // 새 고리나 OS 조회를 만들지 않는다.
bool FileSpecQuery::Valid() const { return valid_; } // 유효성만조회하고다음파일을미리읽지않는다.
bool FileSpecQuery::Next() { // 실패한핸들도반납전까지그대로보존하며마지막이름을지우지않는다.
    if (!search_.handle) { valid_=false;return false; }if (!valid_) return false;
    if (hooks_.find && !hooks_.next) throw std::logic_error("FileSpec의명시다음검색장치없음");
    const auto found=hooks_.next ? hooks_.next(search_.handle):WindowsNext(search_.handle);valid_=found.has_value();if (!found) return false;
    const auto slash=value_.find_last_of('\\');value_.resize(slash==value_.npos ? 0:slash+1);value_+=std::string_view(*found).substr(0,found->find('\0'));return true;
}
}
