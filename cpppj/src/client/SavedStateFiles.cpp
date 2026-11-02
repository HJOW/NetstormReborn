// 같은 현재 경로를 사용하고 실패 시 원문 CRT의 doserrno/errno 쓰기를 유지한다.
#include "client/SavedStateFiles.h"
#include "o/Win32FileError.h"
#include "o/RawFileName.h"
#include <array>
#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <windows.h>

namespace netstorm::client { // 게임 설치 폴더를 프로세스 CWD 변경 없이 사용할 실제 장치다.
// 원본은 게임 폴더를 CWD로 설정한다. cpppj는 같은 파일을 현재 명시 설치 경로에서 연다.
std::filesystem::path SavedStateFilePath(const std::filesystem::path& gameDirectory,o::RawFileName& formatter) {
    const std::array<o::FileNamePart,3> parts{{{-2,""},{-2,"d"},{-3,"!state.dat"}}};const auto path=formatter.Format(parts); // 실제004b5470의 root/d/서식 결과를 같은 고리에 작성한다.
    if (!path.starts_with(".\\")) throw std::logic_error("원문 저장 상태 이름의 상대 접두 불일치");
    return std::filesystem::absolute(gameDirectory/std::filesystem::path(path.substr(2))); // 원본 chdir 대신 명시 설치 폴더를 사용하며 W 경로의 한글 설치 이름을 그대로 보존한다.
}
// 원본 ANSI DeleteFileA와 달리 Windows10/11 한글 설치 경로를 위해 W API를 사용한다. 디렉터리/readonly를 지우거나 속성을 바꾸지 않는다.
std::int32_t DeleteSavedStateFile(const std::filesystem::path& gameDirectory,o::RawFileName& formatter) {
    const auto path=SavedStateFilePath(gameDirectory,formatter);
    if (::DeleteFileW(path.c_str())) return 0;
    const auto error=::GetLastError();if (!error) return 0; // 원문도 실패BOOL 뒤 OS오류0이면 매핑하지 않고0을 돌려준다.
    static_cast<void>(::_set_doserrno(error));errno=o::Win32FileErrorCode(error);return -1;
}
}
