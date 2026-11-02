// 원본과 다른 game/ 휴대 배치와 Windows 10/11 유니코드 실행 경로를 지원한다.
#include "platform/PortableStartup.h"
#include "platform/Utf8Path.h"
#include "o/FileSpecQuery.h"
#include "o/RawFileName.h"
#include <stdexcept>
#include <vector>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace netstorm::platform { // 게임 규칙과 구별되는 새 OS 실행 어댑터다.
std::filesystem::path CurrentExecutableFile() {
#if defined(_WIN32)
    std::vector<wchar_t> buffer(512); // 긴 한글 배포 경로도 잘리지 않게 필요할 때 확장한다.
    for (;;) { // Win32의 실제 반환 길이가 버퍼보다 작을 때만 경로를 확정한다.
        const auto length=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(buffer.size())); // 현재 모듈의 UTF-16 경로 길이다.
        if (!length) throw std::runtime_error("Unable to locate Netstorm.exe");
        if (length<buffer.size()) return std::filesystem::path(std::wstring(buffer.data(),length));
        if (buffer.size()>=32768) throw std::runtime_error("Netstorm.exe path is too long");
        buffer.resize(buffer.size()*2);
    }
#else
    throw std::runtime_error("The cpppj GUI executable requires Windows");
#endif
}

std::string ModuleDirectory(std::string_view executable,o::RawFileName& names) { // 모듈 조회 OS 경계는 호출자가 담당하고 나머지 원문 부모의 고리 수명은 그대로 따른다.
    o::FileSpecQuery spec(executable);const auto directory=names.Directory(spec.Text());const auto device=names.Device(spec.Text());
    const std::array<o::FileNamePart,2> parts{{{-1,device},{-2,directory}}};return std::string(names.Format(parts));
}

std::filesystem::path PortableGameDirectory(const std::filesystem::path& executable,o::RawFileName* names) {
    const auto absolute=std::filesystem::absolute(executable);const auto text=PathToUtf8(absolute);o::RawFileName localNames;
    auto directory=absolute.parent_path(); // 호출자의 cwd 대신 실제 EXE가 소유한 자료만 선택한다.
    if (text.size()<256) {
        directory=PathFromUtf8(ModuleDirectory(text,names ? *names:localNames)).parent_path();
        if (directory!=absolute.parent_path()) directory=absolute.parent_path(); // 원본과 다른 Windows10/11 호환: 드라이브 루트의 EXE도 파일명을 디렉터리로 오인하지 않는다. 원문 세 칸의 내용은 유지한다.
    } // 원본과 다른 Windows10/11 호환: UTF-8로256BYTE 이상인 Unicode 설치 경로는 원문 고정 배열의 범위를 넘기므로 OS 경로를 보존한다.
    for (const auto& candidate:{directory/"game",directory}) { // 배포본의 하위 자료를 같은 이름의 옆 자료보다 우선한다.
        std::error_code error; // 누락/접근 불가 자료로 다른 cwd나 원본 폴더에 자동 진입하지 않는다.
        if (std::filesystem::is_regular_file(candidate/"netstorm.tarc",error)) return candidate;
    }
    throw std::runtime_error("Game data missing. Extract the whole archive beside Netstorm.exe (game/netstorm.tarc), or use --run <game-dir>.");
}

void DetachOwnGuiConsole() {
#if defined(_WIN32)
    DWORD processes[2]{}; // 두 개 이상이면 사용자가 이미 연 터미널이므로 분리하지 않는다.
    if (GetConsoleProcessList(processes,2)==1 && processes[0]==GetCurrentProcessId()) FreeConsole();
#endif
}

void ShowGuiStartupError(std::string_view message) {
#if defined(_WIN32)
    const auto length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,message.data(),static_cast<int>(message.size()),nullptr,0); // UTF-8 오류를 OS 언어와 무관하게 표시한다.
    std::wstring text(static_cast<std::size_t>(length),L'\0'); // 변환 실패 때는 아래의 고정 오류문을 사용한다.
    if (length) MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,message.data(),static_cast<int>(message.size()),text.data(),length);
    MessageBoxW(nullptr,length ? text.c_str() : L"Unable to start Netstorm. Check the extracted game data.",L"Netstorm",MB_OK|MB_ICONERROR);
#else
    static_cast<void>(message);
#endif
}
}
