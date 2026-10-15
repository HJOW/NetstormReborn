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
// 실제 Client의 메뉴 정지와 미션 정지가 서로의 깊이를 반납하지 않아야 한다. Run을 호출하지 않아 창/장치/파일쓰기는 없다.
TEST_CASE(ClientPause_UiAndMissionHaveSeparateOwnership) {
    netstorm::client::ClientOptions options;
    options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
    netstorm::client::Client client(std::move(options));
    CHECK(!client.Paused());client.PushGamePause();client.Pause(true);client.PopGamePause();
    CHECK(client.Paused());client.Pause(false);CHECK(!client.Paused());
    client.Pause(true);client.PushGamePause();client.Pause(false);CHECK(client.Paused());
    client.PopGamePause();CHECK(!client.Paused());
}
// 반복 메뉴 요청은 한 깊이만 보유하고, UI가 보유하지 않은 정지는 false 요청으로 풀리지 않아야 한다.
TEST_CASE(ClientPause_RepeatedUiDoesNotConsumeNestedMissionDepths) {
    netstorm::client::ClientOptions options;
    options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
    netstorm::client::Client client(std::move(options));
    client.PushGamePause();client.PushGamePause();client.Pause(false);CHECK(client.Paused());
    client.PopGamePause();CHECK(client.Paused());client.PopGamePause();CHECK(!client.Paused());
    client.Pause(true);client.Pause(true);client.PushGamePause();client.Pause(false);client.Pause(false);
    CHECK(client.Paused());client.PopGamePause();CHECK(!client.Paused());
}
