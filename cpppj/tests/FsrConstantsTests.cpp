// 7차 FSR 1.0: AMD 공식 헤더 CPU 경로로 만든 EASU/RCAS 상수가 공식 문서의 정의와 일치하는지 게임/GPU 없이 확인한다.
#include "TestSupport.h"
#include "client/FsrConstants.h"
#include <bit>
#include <cmath>
#include <stdexcept>

using namespace netstorm::client;

namespace { // 상수 비교용 보조다.
// 부동소수점 비트 패턴을 값으로 되돌린다.
float Value(std::uint32_t bits) { return std::bit_cast<float>(bits); }
}

// 1920x1080을 3840x2160으로 늘리는 EASU 상수는 공식 FsrEasuCon의 정의(출력 한 칸 = 입력 0.5칸, 입력 크기의 역수 등)와 같다.
TEST_CASE(FsrConstants_EasuMatchesOfficialDefinitionFor2xUpscale) {
    const auto c = ComputeFsrEasuConstants(1920.0f, 1080.0f, 3840.0f, 2160.0f);
    CHECK_NEAR(Value(c.con0[0]), 0.5f, 1e-7f); // 입력 폭 / 출력 폭이다.
    CHECK_NEAR(Value(c.con0[1]), 0.5f, 1e-7f);
    CHECK_NEAR(Value(c.con0[2]), 0.5f * 0.5f - 0.5f, 1e-7f); // 출력 픽셀 중심을 입력 위치로 옮기는 오프셋이다.
    CHECK_NEAR(Value(c.con0[3]), 0.5f * 0.5f - 0.5f, 1e-7f);
    CHECK_NEAR(Value(c.con1[0]), 1.0f / 1920.0f, 1e-9f); // 입력 크기의 역수다.
    CHECK_NEAR(Value(c.con1[1]), 1.0f / 1080.0f, 1e-9f);
    CHECK_NEAR(Value(c.con1[2]), 1.0f / 1920.0f, 1e-9f);
    CHECK_NEAR(Value(c.con1[3]), -1.0f / 1080.0f, 1e-9f);
    CHECK_NEAR(Value(c.con2[0]), -1.0f / 1920.0f, 1e-9f);
    CHECK_NEAR(Value(c.con2[1]), 2.0f / 1080.0f, 1e-9f);
    CHECK_NEAR(Value(c.con2[2]), 1.0f / 1920.0f, 1e-9f);
    CHECK_NEAR(Value(c.con2[3]), 2.0f / 1080.0f, 1e-9f);
    CHECK_NEAR(Value(c.con3[0]), 0.0f, 1e-9f);
    CHECK_NEAR(Value(c.con3[1]), 4.0f / 1080.0f, 1e-9f);
    CHECK(c.con3[2] == 0 && c.con3[3] == 0);
}

// 1.5배(1920x1080 → 2560x1440)는 입력/출력 비율이 3/4이며 잘못된 크기는 거부한다.
TEST_CASE(FsrConstants_EasuRatioForQhdAndInvalidSizesAreRejected) {
    const auto c = ComputeFsrEasuConstants(1920.0f, 1080.0f, 2560.0f, 1440.0f);
    CHECK_NEAR(Value(c.con0[0]), 0.75f, 1e-6f);
    CHECK_NEAR(Value(c.con0[1]), 0.75f, 1e-6f);
    CHECK_NEAR(Value(c.con0[2]), 0.5f * 0.75f - 0.5f, 1e-6f);
    bool rejected = false;
    try { static_cast<void>(ComputeFsrEasuConstants(0.0f, 1080.0f, 3840.0f, 2160.0f)); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    rejected = false;
    try { static_cast<void>(ComputeFsrEasuConstants(1920.0f, 1080.0f, 3840.0f, -1.0f)); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
}

// RCAS 선명도는 스톱 값을 2^-스톱으로 바꾸며 0이 최대(1.0), 클수록 절반씩 줄고 두 반정밀도 칸에 같은 값이 들어간다.
TEST_CASE(FsrConstants_RcasConvertsStopsToLinearSharpness) {
    const auto maximum = ComputeFsrRcasConstants(0.0f);
    CHECK_NEAR(Value(maximum[0]), 1.0f, 1e-7f);
    const auto half = ComputeFsrRcasConstants(1.0f);
    CHECK_NEAR(Value(half[0]), 0.5f, 1e-7f);
    const auto recommended = ComputeFsrRcasConstants(0.2f);
    CHECK_NEAR(Value(recommended[0]), std::exp2(-0.2f), 1e-6f);
    CHECK((recommended[1] & 0xffffu) == (recommended[1] >> 16) && recommended[1] != 0); // 두 반정밀도 값이 같다.
    CHECK(recommended[2] == 0 && recommended[3] == 0);
}
