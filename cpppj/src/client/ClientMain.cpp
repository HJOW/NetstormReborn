// 원본 소스: ClientMain.cpp (클라이언트 폴더) — 프로그램 진입점과 메인 루프
#include "client/ClientMain.h"

namespace netstorm::client {

// 프레임 제한 간격(초)을 구한다.
//
// 원본: FUN_00435220 @ 00435220 (패치판 10.78) [신뢰도 A] ↔ CD판 FUN_00484ac0 @ 00484ac0 (string/strong)
// 소속: 주소가 Clientdebug.cpp(00434a00)와 Clientmain.cpp(00435e40) 사이여서 ClientMain.cpp 로 추정한다.
// 범위: 설정을 읽는 함수 가운데 maxFPS 부분만 옮겼다.
//
//   DAT_005318d8 = 0x4b;                                      // 기본 75
//   FUN_00441270("maxFPS", &DAT_005318d8);                    // 설정 파일 값으로 덮어쓴다
//   _DAT_005318e0 = _DAT_00500460;                            // 0.0
//   if (0 < DAT_005318d8)
//       _DAT_005318e0 = _DAT_00501580 / (double)DAT_005318d8; // 1.0 / maxFPS
//
// 이 간격은 FUN_00436450 의 바쁜 대기(`벽시계 - 직전 그리기 시각 < 간격`)에서 쓰인다.
double FrameIntervalSeconds(int maxFps) {
    if (maxFps > 0) {
        return 1.0 / static_cast<double>(maxFps);
    }
    return 0.0;
}

}  // namespace netstorm::client
