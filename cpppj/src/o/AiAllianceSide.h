// AI 생성 시점의 아군/적 판정이다. 원문 AI 생성자(00416860)는 설정 +23c 동맹 목록을 생성자 안에서야 Player 동맹표에 기록하므로,
// 생성자보다 먼저 난이도·시작 자금을 정하는 클론 추가 기능(11차 난이도, 테스트 전용 자동 플레이)은 설정의 동맹 목록도 함께 읽어야 한다.
// (2026-11-14 신규: 동맹 AI가 생성 시점에 적으로 분류되어 적 난이도를 받던 11차 판정을 바로잡기 위해 추가)
#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace netstorm::o { // 게임 규칙 계산만 두며 UI·설정 파일·Session에 의존하지 않는다.
// 600BYTE AI 설정에서 동맹 owner 비트 마스크(ai%sAllyList, 1<<owner의 합)가 있는 오프셋이다(RawMissionAiSettings::Apply가 쓰는 위치).
inline constexpr std::size_t kAiSettingsAllyMaskOffset = 0x23c;

// owner의 AI가 로컬 플레이어의 적인지 돌려준다. 로컬 자신, 로컬 동맹 마스크(Player+54)에 이미 있는 owner,
// 설정 동맹 목록에 로컬 owner가 들어 있는 AI는 적이 아니다. settings는 600BYTE 이상이어야 하며 owner/localOwner는 0..8이다.
// 사용법: RawAiCreate의 난이도 조회와 테스트 전용 적 시작 SP 지정이 AI 생성 직전에 같은 판정을 쓴다(2026-11-14 신규).
inline bool IsAiEnemyOfLocal(std::uint32_t owner, std::uint32_t localOwner, std::uint32_t localAllies, std::span<const std::uint8_t> settings) {
    if (settings.size() < kAiSettingsAllyMaskOffset + 4) throw std::invalid_argument("AI 아군/적 판정: 설정 동맹 목록 범위 누락");
    if (owner > 8 || localOwner > 8) throw std::out_of_range("AI 아군/적 판정: owner 범위 오류");
    if (owner == localOwner) return false; // 로컬 플레이어 자신의 AI(데모·테스트 자동 플레이)는 아군이다.
    if ((localAllies >> owner) & 1U) return false; // 로컬 쪽이 이미 동맹으로 기록한 owner다.
    std::uint32_t declared{}; // 설정의 동맹 목록이며 비정렬 little endian DWORD다.
    // 네 BYTE를 낮은 비트부터 합친다.
    for (std::size_t index = 0; index < 4; ++index) declared |= static_cast<std::uint32_t>(settings[kAiSettingsAllyMaskOffset + index]) << (8 * index);
    return ((declared >> localOwner) & 1U) == 0; // AI가 로컬을 동맹으로 선언하면 생성자가 양방향 동맹을 기록하므로 아군이다.
}
}
