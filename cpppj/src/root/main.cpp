// 원본 R의 정확한 lan 인자와 현재 작업 폴더를 새 Windows10/11 실행 owner에 전달한다.
#include "root/Server.h"
#include <cstdio>
#include <exception>
#include <vector>

int main(int argc,char** argv) { // UTF-8 manifest의 실제 CRT 인자를 그대로 보관하며 서버/게임 실행은 이 진입점에서만 시작한다.
    try {
        std::vector<std::string> arguments;arguments.reserve(static_cast<std::size_t>(argc));
        for (int index=0;index<argc;++index) arguments.emplace_back(argv[index]); // argv0도 포함한 실제 개수/대소문자를 보존한다. 새 해석/추가 설정 인자를 만들지 않는다.
        netstorm::root::Server server(std::filesystem::current_path(),netstorm::platform::UserDataRoot::Resolve());return server.Run(arguments);
    } catch (const std::exception& error) { std::fprintf(stderr,"Netstorm Root Server 11.01: %s\n",error.what());return 1; } // 호스트의 실제 구성/물리·파일 예외만 보고하고 성공한 서버 상태를 합성하지 않는다.
}
