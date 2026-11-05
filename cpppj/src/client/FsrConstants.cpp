// 7차 FSR 1.0: 공식 헤더의 CPU 경로를 한 번역 단위에 가둬 컴파일한다(헤더는 AMD 원본 그대로이며 경고 규칙이 달라 이 파일에서만 포함한다).
#include "client/FsrConstants.h"
#include <cmath>
#include <cstdint>
#include <stdexcept>

#if defined(_MSC_VER)
#pragma warning(push, 0) // 서드파티 헤더의 경고는 이 프로젝트 경고 규칙에서 제외한다.
#endif
#define A_CPU 1
#include "ffx_a.h"
#include "ffx_fsr1.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace netstorm::client { // FSR 상수 계산 구현이다.
// 공식 FsrEasuCon에 입력 전체를 뷰포트로 넘긴다. 입력 크기와 출력 크기가 양수가 아니면 0 나눗셈이므로 거부한다.
FsrEasuConstants ComputeFsrEasuConstants(float inputWidth, float inputHeight, float outputWidth, float outputHeight) {
    if (!(inputWidth > 0.0f) || !(inputHeight > 0.0f) || !(outputWidth > 0.0f) || !(outputHeight > 0.0f)) throw std::invalid_argument("FSR EASU 상수의 입력/출력 크기는 양수여야 한다");
    AU1 con0[4], con1[4], con2[4], con3[4]; // 공식 함수가 채우는 네 벡터다.
    FsrEasuCon(con0, con1, con2, con3, inputWidth, inputHeight, inputWidth, inputHeight, outputWidth, outputHeight); // 뷰포트 = 입력 이미지 전체다.
    FsrEasuConstants result;
    for (int index = 0; index < 4; ++index) { // 네 성분을 복사한다.
        result.con0[index] = con0[index];
        result.con1[index] = con1[index];
        result.con2[index] = con2[index];
        result.con3[index] = con3[index];
    }
    return result;
}
// 공식 FsrRcasCon으로 선명도(스톱 → 선형 값) 상수를 만든다.
std::array<std::uint32_t, 4> ComputeFsrRcasConstants(float sharpnessStops) {
    AU1 con[4]; // 공식 함수가 채우는 한 벡터다.
    FsrRcasCon(con, sharpnessStops);
    return {con[0], con[1], con[2], con[3]};
}
}
