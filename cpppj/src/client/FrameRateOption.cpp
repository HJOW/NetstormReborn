// 후순위 F1: 프레임 상한 옵션 값 해석의 구현이다.
#include "client/FrameRateOption.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>

namespace netstorm::client { // 값 해석의 구현이다.
// 0/범위 밖은 원본 기본으로 본다. 1~1000 사이의 임의 값도 그대로 비교한다(수동 설정).
int NormalizeFrameRate(int configured) { return configured >= 1 && configured <= 1000 ? configured : kOriginalFrameRate; }
// 접두 "patch1101.setframerate("와 닫는 괄호 사이의 정수가 선택지에 있을 때만 값이다.
std::optional<int> ParseFrameRateTarget(std::string_view target) {
    std::string lower(target);
    for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); // 대소문자를 무시한다.
    constexpr std::string_view prefix = "patch1101.setframerate(";
    if (lower.rfind(prefix, 0) != 0 || lower.empty() || lower.back() != ')') return std::nullopt;
    const std::string_view digits = std::string_view(lower).substr(prefix.size(), lower.size() - prefix.size() - 1);
    int value{};
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (error != std::errc{} || end != digits.data() + digits.size()) return std::nullopt;
    if (std::find(kFrameRateChoices.begin(), kFrameRateChoices.end(), value) == kFrameRateChoices.end()) return std::nullopt; // 선택지 밖은 거부한다.
    return value;
}
}
