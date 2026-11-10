// 후순위 F1: 옵션 메뉴의 Frame Rate(프레임 상한) 선택지와 Tell 대상 해석이다.
// 원본의 프레임 상한은 설정 maxFPS(기본 75)이며 여기서는 60/75/120 세 가지를 옵션 메뉴로 고르게 한다.
#pragma once
#include <array>
#include <optional>
#include <string_view>

namespace netstorm::client { // 게임 규칙·UI 없이 값 해석만 둔다.
// 옵션 메뉴가 제공하는 프레임 상한 선택지다(원본 기본 75 포함).
inline constexpr std::array<int, 3> kFrameRateChoices{60, 75, 120};
// 원본의 기본 프레임 상한이다(`DAT_005318d8 = 0x4b`).
inline constexpr int kOriginalFrameRate = 75;
// 설정 파일의 maxFPS 값을 선택 비교용 값으로 바꾼다. 값이 없거나(0) 범위 밖이면 원본 기본 75다.
int NormalizeFrameRate(int configured);
// "Patch1101.SetFrameRate(숫자)" 대상을 해석한다. 선택지 밖의 숫자나 다른 대상은 빈 값이다(대소문자 무시).
std::optional<int> ParseFrameRateTarget(std::string_view target);
}
