// 원본 소스: ClientMain.cpp (클라이언트 폴더) — 프로그램 진입점과 메인 루프
//
// 메인 루프(원본 WinMain = FUN_00438dc0)는 아직 옮기지 않았다.
// 설정의 프레임 제한 간격과 1ms 시계 기준 양자화 계산을 제공한다.
// 분석 문서: docs/exe/main-loop.md
#pragma once
#include <cstdint>

namespace netstorm::client {

// 설정 키 "maxFPS" 의 코드 기본값. 화면 루프를 초당 몇 번까지 돌릴지 정한다.
// 원본: FUN_00435220 @ 00435220 의 `DAT_005318d8 = 0x4b`
inline constexpr int kDefaultMaxFps = 75;

// 프레임 제한 간격(초)을 구한다. maxFps 가 0 이하면 0(제한 없음)이다.
double FrameIntervalSeconds(int maxFps);

// 1ms 눈금에서 원본 대기가 끝나는 최소 간격. 75fps이면 ceil(1000/75) = 14ms다.
// 원본 자체는 실수 간격의 바쁜 대기다. 여기서는 그 눈금의 결과만 계산한다.
std::uint32_t QuantizedFrameMilliseconds(int maxFps);

}  // namespace netstorm::client
