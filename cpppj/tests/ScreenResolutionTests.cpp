// 4차 와이드 해상도 선택 목록(원본 4:3 세 개 + 호환 16:9/16:10 다섯 개, 최대 1920x1080)의 계약을 게임/GUI 없이 확인한다.
#include "TestSupport.h"
#include "client/ScreenResolutions.h"

using namespace netstorm::client;

// 목록은 원본 세 해상도 뒤에 호환 다섯 해상도가 오고 모두 1920x1080 이하이며 16:9 항목은 정확한 비율이다.
TEST_CASE(ScreenResolutions_ListKeepsOriginalThreeThenCompatibilityFiveUpTo1080p) {
    CHECK(kScreenResolutionChoices.size() == 8 && kOriginalScreenResolutions.size() == 3 && kCompatibilityScreenResolutions.size() == 5);
    CHECK(kOriginalScreenResolutions[0][0] == 640 && kOriginalScreenResolutions[1][0] == 800 && kOriginalScreenResolutions[2][0] == 1024); // 원본 순서다.
    for (const auto& size : kOriginalScreenResolutions) CHECK(size[0] * 3 == size[1] * 4); // 원본 세 개는 4:3이다.
    bool has1080 = false, has900 = false;
    for (const auto& size : kScreenResolutionChoices) { // 모든 선택은 최대 1920x1080 이내여야 한다.
        CHECK(size[0] <= 1920 && size[1] <= 1080);
        has1080 = has1080 || (size[0] == 1920 && size[1] == 1080);
        has900 = has900 || (size[0] == 1600 && size[1] == 900);
        if (size[1] == 720 || size[1] == 900 || size[1] == 1080) CHECK(size[0] * 9 == size[1] * 16); // 이름 붙은 16:9 행은 정확한 비율이다.
    }
    CHECK(has1080 && has900);
    CHECK(kCompatibilityScreenResolutions.data() == kScreenResolutionChoices.data() + 3); // 호환 행은 같은 표의 뒷부분이다.
}

// 허용 판정은 목록의 폭/높이 쌍만 받고 목록 밖의 임의 크기·섞인 쌍은 거부한다.
TEST_CASE(ScreenResolutions_SupportCheckAcceptsOnlyListedPairs) {
    for (const auto& size : kScreenResolutionChoices) CHECK(IsSupportedScreenResolution(size[0], size[1]));
    CHECK(!IsSupportedScreenResolution(1920, 1200)); // 최대 1920x1080을 넘는 16:10이다.
    CHECK(!IsSupportedScreenResolution(3840, 2160) && !IsSupportedScreenResolution(2560, 1440)); // FSR 해상도는 FSR을 쓸 수 있다고 알릴 때만 허용된다(7차).
    CHECK(!IsSupportedScreenResolution(1920, 1081) && !IsSupportedScreenResolution(1024, 769) && !IsSupportedScreenResolution(0, 0) && !IsSupportedScreenResolution(-1, -1));
    CHECK(!IsSupportedScreenResolution(1600, 768) && !IsSupportedScreenResolution(1280, 1080)); // 폭과 높이가 다른 행에서 섞인 쌍이다.
}

// 7차 FSR: 2560x1440과 3840x2160은 FSR 사용 가능일 때만 허용되고 실질 논리 크기는 1920x1080이며 나머지 해상도의 논리 크기는 그대로다.
TEST_CASE(ScreenResolutions_FsrResolutionsNeedFsrAndRenderAt1080p) {
    CHECK(kFsrScreenResolutionChoices.size() == 2 && kFsrScreenResolutionChoices[0][0] == 2560 && kFsrScreenResolutionChoices[1][1] == 2160);
    for (const auto& size : kFsrScreenResolutionChoices) { // 두 FSR 해상도는 정확한 16:9이고 논리 1080p보다 크다.
        CHECK(size[0] * 9 == size[1] * 16 && size[0] > kFsrLogicalWidth && size[1] > kFsrLogicalHeight);
        CHECK(IsFsrScreenResolution(size[0], size[1]) && !IsSupportedScreenResolution(size[0], size[1]) && IsSupportedScreenResolution(size[0], size[1], true));
        const auto logical = LogicalScreenSize(size[0], size[1]);
        CHECK(logical[0] == 1920 && logical[1] == 1080);
    }
    CHECK(!IsFsrScreenResolution(1920, 1080) && !IsFsrScreenResolution(3840, 2161) && !IsFsrScreenResolution(2560, 1080)); // 일반 해상도와 섞인 쌍은 FSR이 아니다.
    for (const auto& size : kScreenResolutionChoices) { // 기존 여덟 해상도는 FSR 허용 여부와 상관없이 지원되고 논리 크기가 그대로다.
        CHECK(IsSupportedScreenResolution(size[0], size[1]) && IsSupportedScreenResolution(size[0], size[1], true));
        const auto logical = LogicalScreenSize(size[0], size[1]);
        CHECK(logical[0] == size[0] && logical[1] == size[1]);
    }
    CHECK(!IsSupportedScreenResolution(3840, 2161, true) && !IsSupportedScreenResolution(1920, 1200, true)); // FSR 허용도 목록 밖 크기는 받지 않는다.
}
