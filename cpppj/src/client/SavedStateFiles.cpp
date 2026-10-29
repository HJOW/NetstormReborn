// 같은 현재 경로를 사용하고 실패 시 원문 CRT의 doserrno/errno 쓰기를 유지한다.
#include "client/SavedStateFiles.h"
#include "o/Win32FileError.h"
#include <cerrno>
#include <cstdlib>
#include <windows.h>

namespace netstorm::client { // 게임 설치 폴더를 프로세스 CWD 변경 없이 사용할 실제 장치다.
// 원본은 게임 폴더를 CWD로 설정한다. cpppj는 같은 파일을 현재 명시 설치 경로에서 연다.
std::filesystem::path SavedStateFilePath(const std::filesystem::path& gameDirectory) {
    return std::filesystem::absolute(gameDirectory/"d"/"!state.dat");
}
// 원본 ANSI DeleteFileA와 달리 Windows10/11 한글 설치 경로를 위해 W API를 사용한다. 디렉터리/readonly를 지우거나 속성을 바꾸지 않는다.
std::int32_t DeleteSavedStateFile(const std::filesystem::path& gameDirectory) {
    const auto path=SavedStateFilePath(gameDirectory);
    if (::DeleteFileW(path.c_str())) return 0;
    const auto error=::GetLastError();if (!error) return 0; // 원문도 실패BOOL 뒤 OS오류0이면 매핑하지 않고0을 돌려준다.
    static_cast<void>(::_set_doserrno(error));errno=o::Win32FileErrorCode(error);return -1;
}
}
