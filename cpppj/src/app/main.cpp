// 실행 파일 진입점 (새로 쓰는 코드 — 원본의 WinMain 을 대신한다)
//
// 지금은 기초 구조만 있어 창을 띄우지 않는다. 빌드 정보를 출력하고 끝난다.
// 메인 루프를 옮기면 여기서 플랫폼 계층을 초기화한 뒤 client/ClientMain 의 루프를 부른다.
#include <cstdio>

#include "client/ClientMain.h"

namespace {

// 다시 만드는 기준이 되는 원본 판본 (originals/Netstorm.exe)
constexpr const char* kTargetOriginalVersion = "10.78";

// 빌드 정보를 표준 출력에 쓴다. 콘솔 코드 페이지에 영향받지 않도록 ASCII 만 쓴다.
void PrintBuildInfo() {
    std::printf("NetstormCpp (C++ build, skeleton)\n");
    std::printf("  reconstructed from: NetStorm %s (decompiled)\n", kTargetOriginalVersion);
    std::printf("  default maxFPS: %d (frame interval %.6f s)\n",
                netstorm::client::kDefaultMaxFps,
                netstorm::client::FrameIntervalSeconds(netstorm::client::kDefaultMaxFps));
}

}  // namespace

// 프로그램 진입점. 지금은 빌드 정보만 출력하고 0 을 돌려준다.
int main() {
    PrintBuildInfo();
    return 0;
}
