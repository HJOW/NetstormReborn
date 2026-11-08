// 원문 Config 레지스트리 문자열의 기본값 조회·진단 복귀와 실제 ACP/UTF-8 경계를 구현한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Windows 구형 선언을 제외하고 실제 레지스트리/NLS API만 빌린다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // Windows 매크로가 표준 알고리즘 이름을 바꾸지 않게 한다.
#endif
#include <windows.h>
#include "platform/LegacyRegistryValue.h"
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace netstorm::platform { // 레지스트리 쓰기나 폐기 검증 복구 없이 현재 Config 문자열 조회만 제공한다.
namespace { // 원문 고정 버퍼와 Windows NLS의 실제 문자열 변환/핸들 반납을 공유한다.
constexpr std::size_t kRegistryPathBytes=1024; // 원문 strcpy 지역 버퍼의 NUL을 포함한 실제 BYTE 용량이다.
constexpr std::size_t kRegistryValueBytes=80; // RegQueryValueA에 넘기는 원문 LONG 0x50 한도다.

// 지정한 코드 페이지의 첫 NUL 이전 바이트를 손실 없이 UTF-16으로 읽는다.
std::wstring RegistryWide(std::string_view text,UINT page) {
    if (text.empty()) return {};if (text.size()>static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("레지스트리 문자열 Win32 길이 범위");
    const auto count=MultiByteToWideChar(page,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0); // 잘못된 ACP/UTF-8을 U+FFFD 성공으로 바꾸지 않는다.
    if (!count) throw std::invalid_argument("레지스트리 문자열의 코드 페이지 바이트 형식");std::wstring result(static_cast<std::size_t>(count),L'\0');
    if (MultiByteToWideChar(page,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),count)!=count) throw std::runtime_error("레지스트리 문자열 UTF-16 변환 실패");return result;
}
// UTF-16을 현재 ACP 또는 UTF-8로 바꾸며 best-fit/물음표 대체를 거부한다.
std::string RegistryNarrow(std::wstring_view text,UINT page) {
    if (text.empty()) return {};if (text.size()>static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("레지스트리 UTF-16 문자열 Win32 길이 범위");
    const bool unicode=page==CP_UTF8 || page==54936; // UTF-8/GB18030은 NLS의 별도 flags/default 포인터 계약을 따른다.
    const DWORD flags=unicode ? WC_ERR_INVALID_CHARS:WC_NO_BEST_FIT_CHARS; // 기존 글꼴 이름 경계와 같은 무손실 ACP 정책이다.
    BOOL substituted=FALSE;const auto count=WideCharToMultiByte(page,flags,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,unicode ? nullptr:&substituted);
    if (!count || substituted) throw std::invalid_argument("레지스트리 문자열의 현재 코드 페이지 표현 불가");std::string result(static_cast<std::size_t>(count),'\0');substituted=FALSE;
    if (WideCharToMultiByte(page,flags,text.data(),static_cast<int>(text.size()),result.data(),count,nullptr,unicode ? nullptr:&substituted)!=count || substituted) throw std::invalid_argument("레지스트리 문자열 코드 페이지의 손실 변환");return result;
}
struct RegistryRoot { // 원문의 순서대로 HKEY 이름과 실제 Windows 사전 정의 핸들을 대응시킨다.
    std::string_view name; // 원문 _strnicmp의 비교 길이와 이름 바이트다.
    HKEY handle; // 64비트 Windows에서도 포인터 폭을 보존한 실제 HKEY다.
};
struct RegistryKey { // 성공한 RegOpenKeyA의 단일 핸들을 현재 함수 복귀 때 반납한다.
    HKEY handle; // 이 owner가 소유하는 열린 키이며 사전 정의 root 선택과 구별한다.
    ~RegistryKey() { RegCloseKey(handle); } // 원문 누락 반납을 현대 호스트에서 보완하며 조회 결과는 바꾸지 않는다.
};
}
std::optional<std::string> ReadLegacyRegistryValue(std::string_view path,const std::function<void()>& invalidRoot) {
    path=path.substr(0,path.find('\0'));const auto page=GetACP(); // Config는 UTF-8이지만 실제 A API의 코드 페이지는 프로세스 manifest/OS에 따른다.
    auto bytes=RegistryNarrow(RegistryWide(path,CP_UTF8),page);if (bytes.size()>=kRegistryPathBytes) throw std::length_error("원문 레지스트리 경로의 실제1024BYTE 범위");
    const auto slash=bytes.find('\\'),hash=bytes.find('#');if (slash==bytes.npos || hash==bytes.npos) return std::nullopt; // 원문은 두 구분자를 먼저 찾고 없으면 OS 조회/진단 없이0으로 복귀한다.
    bytes[slash]=0;bytes[hash]=0;HKEY selected{}; // 두 실제 첫 구분자 위치를 NUL로 바꿔 역순 구분자 입력의 C문자열 의미도 유지한다.
    const std::array<RegistryRoot,6> roots{{{"HKEY_CLASSES_ROOT",HKEY_CLASSES_ROOT},{"HKEY_CURRENT_USER",HKEY_CURRENT_USER},{"HKEY_LOCAL_MACHINE",HKEY_LOCAL_MACHINE},{"HKEY_USERS",HKEY_USERS},{"HKEY_CURRENT_CONFIG",HKEY_CURRENT_CONFIG},{"HKEY_DYN_DATA",HKEY_DYN_DATA}}}; // 원문의 여섯 root 우선순위이며 현재 Windows의 미지원 root도 API 실패로 남긴다.
    for (const auto& root:roots) { // 원문은 root 이름 길이만 대소문자 무시 비교하므로 뒤에 붙은 문자도 허용한다.
        if (_strnicmp(bytes.c_str(),root.name.data(),root.name.size())==0) { selected=root.handle;break; }
    }
    if (!selected) { if (!invalidRoot) throw std::logic_error("레지스트리 keyValue의 실제 진단 장치 미연결");invalidRoot(); } // 진단이 정상 복귀하면 원문처럼 NULL root로 RegOpenKeyA를 계속 호출한다.
    HKEY opened{};if (RegOpenKeyA(selected,bytes.data()+slash+1,&opened)!=ERROR_SUCCESS) return std::nullopt;RegistryKey key{opened}; // 원문 API/호스트 기본 registry view를 유지하고 성공한 핸들만 소유한다.
    std::array<char,kRegistryValueBytes> value;LONG length=static_cast<LONG>(value.size()); // 성공한 API가 기록한 BYTE만 아래에서 읽으며 미작성 지역 바이트를 합성하지 않는다.
    if (RegQueryValueA(key.handle,bytes.data()+hash+1,value.data(),&length)!=ERROR_SUCCESS) return std::nullopt; // '#' 뒤는 named value가 아니라 열린 키 아래 하위키의 기본값이다.
    if (length<=0 || static_cast<std::size_t>(length)>value.size()) throw std::out_of_range("레지스트리 기본값의 실제80BYTE 반환 범위");
    const std::string_view result(value.data(),static_cast<std::size_t>(length));const auto nul=result.find('\0');if (nul==result.npos) throw std::out_of_range("레지스트리 기본값의 원문 끝NUL 없음"); // 원문 strlen의 범위 밖 읽기는 호스트에서 명시 거부한다.
    return RegistryNarrow(RegistryWide(result.substr(0,nul),page),CP_UTF8); // 원문 Config 치환/괄호 이스케이프에는 현재 ACP 바이트를 UTF-8로 변환해 반환한다.
}
}
