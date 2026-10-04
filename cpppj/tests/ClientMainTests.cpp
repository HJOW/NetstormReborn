// client/ClientMain 의 테스트
#include "TestSupport.h"
#include "client/ClientMain.h"

using netstorm::client::FrameIntervalSeconds;
using netstorm::client::kDefaultMaxFps;

// 실수 비교 허용오차
constexpr double kTolerance = 1e-12;

// 원본 기본값 75 에서 간격은 1/75초다 (docs/exe/main-loop.md 4절).
TEST_CASE(FrameInterval_DefaultMaxFps_IsOneSeventyFifth) {
    CHECK(kDefaultMaxFps == 75);
    CHECK_NEAR(FrameIntervalSeconds(kDefaultMaxFps), 1.0 / 75.0, kTolerance);
}

// maxFPS 가 0 이하면 간격이 0 이어서 제한이 없다.
TEST_CASE(FrameInterval_ZeroOrNegative_IsUnlimited) {
    CHECK(FrameIntervalSeconds(0) == 0.0);
    CHECK(FrameIntervalSeconds(-1) == 0.0);
}

// 원본 1ms 눈금의 75·60·120 제한은 14·17·9ms다. 정확한 60·120Hz 지원은 후속이다.
TEST_CASE(FrameInterval_OriginalClockQuantization_IsPreserved) {
    CHECK(netstorm::client::QuantizedFrameMilliseconds(75) == 14);
    CHECK(netstorm::client::QuantizedFrameMilliseconds(60) == 17);
    CHECK(netstorm::client::QuantizedFrameMilliseconds(120) == 9);
    CHECK(netstorm::client::QuantizedFrameMilliseconds(0) == 0);
}
