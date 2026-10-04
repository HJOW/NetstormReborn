// 원본 소스: ClientMain.cpp (클라이언트 폴더) — 프로그램 진입점과 메인 루프
//
// 지금은 기초 구조만 있다. 메인 루프(원본 WinMain = FUN_00438dc0)는 아직 옮기지 않았고,
// 다시 만든 코드에 다는 주석 규칙의 견본으로 프레임 제한 간격 계산만 들어 있다.
// 분석 문서: docs/exe/main-loop.md
#pragma once

namespace netstorm::client {

// 설정 키 "maxFPS" 의 코드 기본값. 화면 루프를 초당 몇 번까지 돌릴지 정한다.
// 원본: FUN_00435220 @ 00435220 의 `DAT_005318d8 = 0x4b`
inline constexpr int kDefaultMaxFps = 75;

// 프레임 제한 간격(초)을 구한다. maxFps 가 0 이하면 0(제한 없음)이다.
double FrameIntervalSeconds(int maxFps);

}  // namespace netstorm::client
