// 난이도 배율의 구현이다. AI 설정의 정확한 위치는 원문 AI 설정 파서(004c2f30대)가 쓰는 오프셋을 따른다.
#include "o/AiDifficulty.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace netstorm::o { // 난이도 계산의 구현이다.
namespace { // 이 파일에서만 쓰는 오프셋과 작은 접근자다.
// AI 설정(600BYTE)의 float 주기 필드 오프셋이다. ai%sBridgeDrawRate/StuffRefreshRate/TimeBetweenMoves에 해당한다.
constexpr std::array<std::size_t, 3> kCycleOffsets{0x220, 0x224, 0x228};
// AI 설정의 채취 유닛 수(ai%sCollectors, signed long) 오프셋이다.
constexpr std::size_t kCollectorsOffset = 0x234;
// AI 설정의 강제 적 소유자(ai%sEnemy, signed long) 오프셋이다.
constexpr std::size_t kForcedEnemyOffset = 0x240;
// 설정 전체 크기이며 RawMissionAiSettings::kSize와 같다.
constexpr std::size_t kSettingsBytes = 600;
// 지정 위치의 DWORD를 little endian으로 읽는다.
std::uint32_t ReadDword(std::span<const std::uint8_t> bytes, std::size_t offset) {
    std::uint32_t value{};
    std::memcpy(&value, bytes.data() + offset, sizeof value); // 호스트도 little endian이므로 그대로 복사한다.
    return value;
}
// 지정 위치에 DWORD를 쓴다.
void WriteDword(std::span<std::uint8_t> bytes, std::size_t offset, std::uint32_t value) {
    std::memcpy(bytes.data() + offset, &value, sizeof value);
}
}
// 1/3만 쉬움/어려움이며 0과 모르는 값은 모두 보통이다(설정 파일이 없을 때 현행과 같도록).
AiDifficulty AiDifficultyFromValue(std::uint32_t value) {
    if (value == static_cast<std::uint32_t>(AiDifficulty::Easy)) return AiDifficulty::Easy;
    if (value == static_cast<std::uint32_t>(AiDifficulty::Hard)) return AiDifficulty::Hard;
    return AiDifficulty::Normal;
}
// 열거값 그대로가 저장값이다.
std::uint32_t AiDifficultyToValue(AiDifficulty level) { return static_cast<std::uint32_t>(level); }
// 강제 적 번호가 1..8 밖이거나 자기 자신이면 0으로 정리한다. 0은 원래 "자동 선택"이라 그대로 둔다.
std::vector<std::uint8_t> SanitizeAiForcedEnemy(std::span<const std::uint8_t> settings, std::uint32_t owner) {
    if (settings.size() != kSettingsBytes) throw std::invalid_argument("AI 설정은 600바이트여야 한다");
    const auto forced = ReadDword(settings, kForcedEnemyOffset);
    if (forced == 0 || (forced <= 8 && forced != owner)) return {}; // 정상이거나 자동 선택이면 바꾸지 않는다.
    std::vector<std::uint8_t> result(settings.begin(), settings.end());
    WriteDword(result, kForcedEnemyOffset, 0);
    return result;
}
// 주기 필드 세 개와 채취 유닛 수에 난이도를 적용한다.
std::vector<std::uint8_t> ApplyAiDifficulty(std::span<const std::uint8_t> settings, AiDifficulty level) {
    if (settings.size() != kSettingsBytes) throw std::invalid_argument("AI 난이도: 설정은 600바이트여야 한다");
    std::vector<std::uint8_t> result(settings.begin(), settings.end());
    if (level == AiDifficulty::Normal) return result; // 보통은 원본 값 그대로다.
    const float factor = level == AiDifficulty::Easy ? kEasyCycleFactor : kHardCycleFactor;
    // 세 주기 필드를 차례로 곱한다. 0 이하/비유한 값(예: 다리 주기 0)은 배율로 바꿀 수 없어 그대로 둔다.
    for (const auto offset : kCycleOffsets) {
        const float value = std::bit_cast<float>(ReadDword(result, offset));
        if (std::isfinite(value) && value > 0.0F) WriteDword(result, offset, std::bit_cast<std::uint32_t>(value * factor));
    }
    if (level == AiDifficulty::Easy) { // 쉬움만 채취 유닛 수를 줄이며 원래 0이면(채취 유닛 없음 설계) 건드리지 않는다.
        const auto collectors = std::bit_cast<std::int32_t>(ReadDword(result, kCollectorsOffset));
        if (collectors > 0) WriteDword(result, kCollectorsOffset, std::bit_cast<std::uint32_t>(std::max<std::int32_t>(1, collectors / 2)));
    }
    return result;
}
}
