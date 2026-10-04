// 새로 쓰는 코드: Windows CRT의 줄바꿈 변환 없이 바이너리 검증 결과를 출력한다.
#pragma once
namespace netstorm::platform {
// 표준 출력을 바이너리 모드로 전환한다. POSIX에서는 아무 작업도 하지 않는다.
void UseBinaryStdout();
}
