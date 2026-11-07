// 현재 로컬 서버의 실제 파일 식별과 Unicode 프로세스 실행을 분리한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Windows 구 선언 충돌을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 함수와 Windows 매크로 충돌을 피한다.
#endif
#include <windows.h>
#include <winver.h>
#include "client/LocalServerProcess.h"
#include "client/ProductVersion.h"
#include "platform/Utf8Path.h"
#include <stdexcept>
#include <string>
#include <vector>

namespace netstorm::client { // 사용자 데이터 폴더의 원본 R.exe를 새서버로 오인해 실행하지 않는다.
bool IsRebuiltLocalServer(std::string_view program) {
    program=program.substr(0,program.find('\0'));if (program.empty()) return false;const auto path=platform::PathFromUtf8(program);DWORD ignored{};const auto size=GetFileVersionInfoSizeW(path.c_str(),&ignored);if (!size) return false;std::vector<std::uint8_t> bytes(size);if (!GetFileVersionInfoW(path.c_str(),0,size,bytes.data())) return false;
    void* raw{};UINT length{};if (!VerQueryValueW(bytes.data(),L"\\",&raw,&length) || length<sizeof(VS_FIXEDFILEINFO)) return false;const auto& info=*static_cast<const VS_FIXEDFILEINFO*>(raw);constexpr DWORD version=(NETSTORM_PRODUCT_VERSION_MAJOR<<16U)|NETSTORM_PRODUCT_VERSION_MINOR; // 실제 두 실행 파일과 같은11.01 고정 리소스 값이다.
    if (info.dwSignature!=0xfeef04bdU || info.dwFileType!=VFT_APP || info.dwFileVersionMS!=version || info.dwFileVersionLS || info.dwProductVersionMS!=version || info.dwProductVersionLS) return false;
    if (!VerQueryValueW(bytes.data(),L"\\StringFileInfo\\040904b0\\OriginalFilename",&raw,&length) || length!=6) return false;const auto* name=static_cast<const wchar_t*>(raw);return name[5]==L'\0' && CompareStringOrdinal(name,5,L"R.exe",5,TRUE)==CSTR_EQUAL; // 게임Netstorm.exe와 이름이 바뀐 원본 R도 버전/원문이름 계약을 통과하지 않는다.
}
bool StartRebuiltLocalServer(std::string_view program,std::string_view arguments,std::uint32_t wait,const std::filesystem::path& workingDirectory) {
    arguments=arguments.substr(0,arguments.find('\0'));if (arguments!="lan" || wait) throw std::invalid_argument("로컬 서버37의 실제 lan/wait0 계약");if (!IsRebuiltLocalServer(program)) return false;
    const auto path=std::filesystem::absolute(platform::PathFromUtf8(program.substr(0,program.find('\0')))).lexically_normal();const auto directory=std::filesystem::absolute(workingDirectory).lexically_normal();auto line=L"\""+path.native()+L"\" lan";if (line.size()>=32767) throw std::length_error("로컬 서버 Windows 명령줄의 실제 길이 범위");STARTUPINFOW startup{};startup.cb=static_cast<DWORD>(sizeof(startup));startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION process{};
    const bool created=CreateProcessW(path.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NEW_CONSOLE,nullptr,directory.c_str(),&startup,&process)!=FALSE;
    if (created) { CloseHandle(process.hThread);CloseHandle(process.hProcess); } // 부모콘솔을바꾸지않는실제자식전용콘솔/최초숨김이다. 원문기본lan본문이자기창을최소화하며핸들반납은자식을종료하지않는다.
    return created; // 원문 void launch 부모가 무시하는 실제 시작 결과이며 서버 준비/접속/500알림을 대신 만들지 않는다.
}
}
