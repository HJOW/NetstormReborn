// 새로 쓰는 코드. 원본 게임·화면을 시작하지 않는 콘솔 출력 지원.
#include "platform/Console.h"
#include <cstdio>
#include <stdexcept>
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif
namespace netstorm::platform {
// 원본 바이트를 검증기로 보낼 때 CRLF 변환이 끼어들지 않게 한다.
void UseBinaryStdout() {
#if defined(_WIN32)
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) throw std::runtime_error("Unable to set binary stdout");
#endif
}
}
