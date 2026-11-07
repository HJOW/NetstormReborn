// 원본 서버 대신 현재 클론의 재빌드된 로컬 R.exe를 확인하고 실제 Windows 자식으로 실행한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace netstorm::client { // Native37의 검색/실행 부모와 Windows10/11 파일·프로세스 경계다.
bool IsRebuiltLocalServer(std::string_view program); // 실제 파일의11.01 제품/파일버전·R.exe 식별 리소스를 읽는다. 원본10.71 R/게임EXE/아카이브는 실행 후보로 채택하지 않으며 코드를 로드/실행하지 않는다.
bool StartRebuiltLocalServer(std::string_view program,std::string_view arguments,std::uint32_t wait,const std::filesystem::path& workingDirectory); //37의정확한lan/wait0만 같은실제CreateProcessW로비대기실행한다. 한글·공백경로/자식전용콘솔·게임자료CWD와핸들반납을보존하며성공상태/알림을합성하지않는다.
}
