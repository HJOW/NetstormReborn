// 7차 FSR 1.0: AMD FidelityFX 공식 헤더(third_party/fsr1)의 CPU 경로로 EASU/RCAS 셰이더 상수를 계산한다.
// 셰이더 쪽 알고리즘은 같은 헤더의 GPU 경로이며 이 모듈은 상수 버퍼에 넣을 값만 만든다.
#pragma once
#include <array>
#include <cstdint>

namespace netstorm::client { // 그래픽 장치와 독립인 순수 계산이라 게임/GPU 없이 검사할 수 있다.
// EASU(공간 업스케일) 셰이더 상수 네 벡터다. 각 값은 부동소수점 비트 패턴을 uint32로 담는다.
struct FsrEasuConstants {
    std::array<std::uint32_t, 4> con0{}, con1{}, con2{}, con3{}; // 공식 FsrEasuCon의 con0..con3다.
};
// 입력(논리) 화면 전체를 뷰포트로 보고 출력 크기로 늘리는 EASU 상수를 계산한다.
// inputWidth/Height는 원본 논리 화면 크기, outputWidth/Height는 실제 표시 크기이며 모두 0보다 커야 한다.
FsrEasuConstants ComputeFsrEasuConstants(float inputWidth, float inputHeight, float outputWidth, float outputHeight);
// RCAS(선명도) 상수 네 값을 계산한다. sharpnessStops는 0이 최대 선명도이고 클수록 절반씩 줄어드는 '스톱' 값이다(공식 권장 기본 0.2).
std::array<std::uint32_t, 4> ComputeFsrRcasConstants(float sharpnessStops);
}
