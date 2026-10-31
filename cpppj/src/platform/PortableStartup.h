// 휴대 배포본의 기본 GUI 진입에 필요한 실행 경로와 콘솔 처리를 제공한다.
#pragma once
#include <filesystem>
#include <string_view>

namespace netstorm::platform { // 원본 WinMain 대신 사용하는 Windows 10/11 호환 장치다.
// 작업 디렉토리와 무관하게 현재 실행 파일의 유니코드 절대 경로를 얻는다.
std::filesystem::path CurrentExecutableFile();
// 배포본의 game/을 먼저, 실행 파일 옆 원본 배치를 다음으로 찾는다. 현재 디렉토리는 검색하지 않는다.
std::filesystem::path PortableGameDirectory(const std::filesystem::path& executable);
// 탐색기에서 새로 만든 이 프로세스 전용 콘솔만 분리하며 기존 터미널은 유지한다.
void DetachOwnGuiConsole();
// 무인자 GUI 실행 실패를 콘솔 유무와 무관하게 사용자에게 표시한다.
void ShowGuiStartupError(std::string_view message);
}
