// 11차 난이도: AI 설정 600BYTE에 쉬움/어려움 배율을 적용하는 순수 계산과 RawAiCreate 연결을 게임 실행 없이 검사한다.
#include "TestSupport.h"
#include "o/AiDifficulty.h"
#include "o/RawAiCreate.h"
#include <bit>
#include <cstring>
#include <stdexcept>

using namespace netstorm::o;

namespace { // 이 파일의 검사 도우미다.
// 지정 위치에 float 값을 쓴다.
void PutFloat(std::vector<std::uint8_t>& bytes, std::size_t offset, float value) { const auto bits = std::bit_cast<std::uint32_t>(value); std::memcpy(bytes.data() + offset, &bits, 4); }
// 지정 위치에 signed 정수를 쓴다.
void PutInt(std::vector<std::uint8_t>& bytes, std::size_t offset, std::int32_t value) { std::memcpy(bytes.data() + offset, &value, 4); }
// 지정 위치의 float 값을 읽는다.
float GetFloat(const std::vector<std::uint8_t>& bytes, std::size_t offset) { std::uint32_t bits{}; std::memcpy(&bits, bytes.data() + offset, 4); return std::bit_cast<float>(bits); }
// 지정 위치의 signed 정수를 읽는다.
std::int32_t GetInt(const std::vector<std::uint8_t>& bytes, std::size_t offset) { std::int32_t value{}; std::memcpy(&value, bytes.data() + offset, 4); return value; }
// 원본 AI 기본 설정과 비슷한 600BYTE를 만든다(다리 0, 갱신 1.0, 이동 2.0, 지질 3, 채취 6). 다른 바이트는 구별 가능한 무늬로 채운다.
std::vector<std::uint8_t> SampleSettings() {
    std::vector<std::uint8_t> bytes(600);
    // 모든 칸을 위치 기반 무늬로 채워 배율이 다른 필드를 건드리는지 비교할 수 있게 한다.
    for (std::size_t index = 0; index < bytes.size(); ++index) bytes[index] = static_cast<std::uint8_t>(index * 7 + 1);
    PutFloat(bytes, 0x220, 0.0F); PutFloat(bytes, 0x224, 1.0F); PutFloat(bytes, 0x228, 2.0F); PutInt(bytes, 0x230, 3); PutInt(bytes, 0x234, 6);
    return bytes;
}
}

// 난이도 값 해석: 1쉬움/3어려움 외에는 모두 보통이며 왕복이 맞는다.
TEST_CASE(AiDifficulty_ValueMapping) {
    CHECK(AiDifficultyFromValue(0) == AiDifficulty::Normal && AiDifficultyFromValue(2) == AiDifficulty::Normal && AiDifficultyFromValue(99) == AiDifficulty::Normal); // 값 없음/보통/범위 밖은 보통이다.
    CHECK(AiDifficultyFromValue(1) == AiDifficulty::Easy && AiDifficultyFromValue(3) == AiDifficulty::Hard);
    // 세 난이도의 저장값 왕복을 확인한다.
    for (const auto level : {AiDifficulty::Easy, AiDifficulty::Normal, AiDifficulty::Hard}) CHECK(AiDifficultyFromValue(AiDifficultyToValue(level)) == level);
}

// NB10: 강제 적 번호가 범위 밖이거나 자기 자신이면 0으로 정리하고, 정상 값과 자동 선택(0)은 그대로 둔다.
TEST_CASE(AiDifficulty_SanitizeForcedEnemy) {
    auto settings = SampleSettings();
    PutInt(settings, 0x240, 0); // 무늬로 채운 바이트를 자동 선택(0)으로 맞춘다.
    CHECK(SanitizeAiForcedEnemy(settings, 2).empty()); // 0(자동 선택)은 그대로다.
    PutInt(settings, 0x240, 3);
    CHECK(SanitizeAiForcedEnemy(settings, 2).empty() && !SanitizeAiForcedEnemy(settings, 3).empty()); // 자기 자신(3)은 정리한다.
    CHECK(GetInt(SanitizeAiForcedEnemy(settings, 3), 0x240) == 0);
    for (const std::int32_t bad : {9, 100, -1, 0x7fffffff}) { PutInt(settings, 0x240, bad); CHECK(GetInt(SanitizeAiForcedEnemy(settings, 2), 0x240) == 0); } // 소유자 범위(1..8) 밖이다.
    PutInt(settings, 0x240, 8);
    CHECK(SanitizeAiForcedEnemy(settings, 2).empty()); // 경계 8은 정상이다.
    const auto cleaned = SanitizeAiForcedEnemy(settings, 8);
    for (std::size_t index = 0; index < settings.size(); ++index) if (index < 0x240 || index >= 0x244) CHECK(cleaned[index] == settings[index]); // 다른 바이트는 그대로다.
}

// 보통은 설정을 한 비트도 바꾸지 않는다(현행과 동일).
TEST_CASE(AiDifficulty_NormalKeepsEveryByte) {
    const auto settings = SampleSettings();
    CHECK(ApplyAiDifficulty(settings, AiDifficulty::Normal) == settings);
}

// 쉬움: 세 주기가 4배, 채취 유닛 수가 절반(내림·최소 1). 다른 필드는 그대로다. 주기 0은 0이다.
TEST_CASE(AiDifficulty_EasyScalesCyclesAndHalvesCollectors) {
    const auto settings = SampleSettings();
    const auto easy = ApplyAiDifficulty(settings, AiDifficulty::Easy);
    CHECK(GetFloat(easy, 0x220) == 0.0F); // 다리 주기 0은 배율로 바뀌지 않는다.
    CHECK(GetFloat(easy, 0x224) == 4.0F && GetFloat(easy, 0x228) == 8.0F);
    CHECK(GetInt(easy, 0x234) == 3 && GetInt(easy, 0x230) == 3); // 채취 6→3, 지질 연결 수는 그대로다.
    // 주기·채취 필드만 달라야 한다.
    for (std::size_t index = 0; index < settings.size(); ++index) {
        const bool changed = (index >= 0x224 && index < 0x22c) || (index >= 0x234 && index < 0x238);
        if (!changed) CHECK(easy[index] == settings[index]);
    }
    auto bridged = SampleSettings(); PutFloat(bridged, 0x220, 6.0F); PutInt(bridged, 0x234, 1);
    const auto easyBridged = ApplyAiDifficulty(bridged, AiDifficulty::Easy);
    CHECK(GetFloat(easyBridged, 0x220) == 24.0F && GetInt(easyBridged, 0x234) == 1); // 다리 6→24, 채취 1은 최소 1을 보장한다.
    auto none = SampleSettings(); PutInt(none, 0x234, 0);
    CHECK(GetInt(ApplyAiDifficulty(none, AiDifficulty::Easy), 0x234) == 0); // 원래 채취 유닛이 없는 설계는 늘리지 않는다.
    auto odd = SampleSettings(); PutInt(odd, 0x234, 5);
    CHECK(GetInt(ApplyAiDifficulty(odd, AiDifficulty::Easy), 0x234) == 2); // 5→2(내림)다.
}

// 어려움: 세 주기가 절반이 되고 채취 유닛 수는 그대로다.
TEST_CASE(AiDifficulty_HardHalvesCyclesOnly) {
    auto settings = SampleSettings(); PutFloat(settings, 0x220, 6.0F);
    const auto hard = ApplyAiDifficulty(settings, AiDifficulty::Hard);
    CHECK(GetFloat(hard, 0x220) == 3.0F && GetFloat(hard, 0x224) == 0.5F && GetFloat(hard, 0x228) == 1.0F);
    CHECK(GetInt(hard, 0x234) == 6);
    bool failed{};
    try { static_cast<void>(ApplyAiDifficulty(std::span<const std::uint8_t>(settings).first(100), AiDifficulty::Hard)); } catch (const std::invalid_argument&) { failed = true; }
    CHECK(failed); // 설정 크기가 600바이트가 아니면 명시 거부한다.
}
