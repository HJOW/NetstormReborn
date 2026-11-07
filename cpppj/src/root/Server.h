// 새 R.exe가 복원된 실제 모듈들을 하나의 프로세스 수명으로 소유한다.
#pragma once
#include "platform/UserDataRoot.h"
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace netstorm::root { // 게임 Client/Player를 만들지 않는 원문 R 전용 실행 owner다.
class Server final { // Run 전체와 모든 수신/로그·파일 callback보다 오래 살아야 한다.
public:
    Server(std::filesystem::path workingDirectory,platform::UserDataRoot user); // 같은 가상CWD/사용자 경로와 실제 모듈을 구성한다. 생성은 설정/파일·시각/창/소켓을 실행하지 않는다.
    ~Server(); // 정상 Run은 원문 Shutdown을 이미 수행한다. 남은 호스트 소유권은 사용자 callback을 합성하지 않고 반납한다.
    Server(const Server&)=delete;Server& operator=(const Server&)=delete; // 같은 현재 표/모듈·등록 callback this를 복제하지 않는다.
    std::int32_t Run(std::span<const std::string> arguments); // 원문처럼 정확히argc2/argv1소문자lan일 때만LAN선택으로 같은 MainLoop.Run을 실제 호출한다. 나머지 argv는 새 설정 인자로 해석하지 않는다.
private:
    struct State;std::unique_ptr<State> state_; // 같은 실제 모든 프로세스 전역/모듈의 주소와 반납 순서를 구현 파일에서 소유한다.
};
}
