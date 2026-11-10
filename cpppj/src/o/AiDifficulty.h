// 11차 난이도 조절: 적 인공지능 설정(600BYTE)에 쉬움/어려움 배율을 적용하는 순수 계산이다.
// 원본 10.78에는 난이도 개념이 없으므로 보통(기본)은 설정을 한 비트도 바꾸지 않고, 쉬움/어려움만 클론이 추가한 배율을 곱한다.
#pragma once
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace netstorm::o { // 게임 규칙 계산만 두며 UI·설정 파일·Session에 의존하지 않는다.
// 난이도 단계다. 값은 설정 파일(options.cfg)의 difficulty 값과 같고, 값이 없거나 범위 밖이면 보통이다(11차).
enum class AiDifficulty : std::uint32_t {
    Easy = 1,   // 쉬움: AI의 다리 건설·유닛 생성 주기가 4배로 늘고 채취 유닛 수가 절반(최소 1)으로 준다.
    Normal = 2, // 보통(기본값): 원본과 동일하다.
    Hard = 3    // 어려움: AI의 다리 건설·유닛 생성 주기가 2배 빨라진다(주기가 절반).
};
// 설정 파일에 저장하는 난이도 키 이름이다(옵션 메뉴와 Skirmish 화면이 공유한다).
inline constexpr std::string_view kDifficultyConfigKey = "difficulty";
// 쉬움에서 주기에 곱하는 배율(느려짐)이다.
inline constexpr float kEasyCycleFactor = 4.0F;
// 어려움에서 주기에 곱하는 배율(빨라짐)이다.
inline constexpr float kHardCycleFactor = 0.5F;
// 저장된 정수(0/누락 포함)를 난이도로 바꾼다. 1은 쉬움, 3은 어려움이고 그 밖의 모든 값은 보통이다.
AiDifficulty AiDifficultyFromValue(std::uint32_t value);
// 난이도를 설정 파일에 저장하는 정수로 바꾼다(AiDifficultyFromValue의 역함수).
std::uint32_t AiDifficultyToValue(AiDifficulty level);
// 600BYTE AI 설정의 복사본에 난이도를 적용해 돌려준다. 입력은 바꾸지 않는다.
// 보통이면 입력과 같은 바이트의 복사본이다. 쉬움/어려움은 +0x220 다리 그리기 주기·+0x224 물건 갱신 주기·+0x228 이동 간격(float)에 배율을 곱하고,
// 쉬움은 +0x234 채취 유닛 수가 1 이상일 때만 절반(내림, 최소 1)으로 줄인다. 비유한/0 이하 주기는 그대로 둔다.
std::vector<std::uint8_t> ApplyAiDifficulty(std::span<const std::uint8_t> settings, AiDifficulty level);
}
