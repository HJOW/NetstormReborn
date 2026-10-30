// 실제 Windows 외부 실행의 Unicode 명령을 기록하고 창 없이 끝나는 검사 전용 GUI 도우미다.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <string>
namespace { // 검사 프로세스의 원시 Windows 명령만 UTF-8로 보존한다.
std::string Utf8(const wchar_t* value) { // 시스템 ANSI 코드 페이지와 무관하게 원래 UTF-16 전체를 기록한다.
    const auto count=WideCharToMultiByte(CP_UTF8,0,value,-1,nullptr,0,nullptr,nullptr);if (count<=0) return {};std::string result(static_cast<std::size_t>(count),'\0');
    if (WideCharToMultiByte(CP_UTF8,0,value,-1,result.data(),count,nullptr,nullptr)!=count) return {};result.pop_back();return result;
}
}
// 새 검사 경로에만 CREATE_NEW로 기록한다. 외부 통신/게임/사용자 창은 만들지 않는다.
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    std::array<wchar_t,32768> path{};const auto length=GetEnvironmentVariableW(L"NETSTORM_HELP_PROOF_PATH",path.data(),static_cast<DWORD>(path.size()));if (!length || length>=path.size()) return 2;
    const auto value=std::to_string(GetCurrentProcessId())+'\n'+Utf8(GetCommandLineW());const auto file=CreateFileW(path.data(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if (file==INVALID_HANDLE_VALUE) return 3;
    DWORD written{};const bool success=WriteFile(file,value.data(),static_cast<DWORD>(value.size()),&written,nullptr)!=FALSE && written==value.size();FlushFileBuffers(file);CloseHandle(file);return success ? 0 : 4;
}
