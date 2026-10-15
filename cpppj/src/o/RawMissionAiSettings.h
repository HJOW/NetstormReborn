// PATCH MissionStage3가 실제 AI 생성자에 넘기는 600바이트 설정의 기본값·공통/owner 덮어쓰기를 복원한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::o {
// 기본값 생성에서 읽는 실제 정수 전역과 능력 이름 진단 모드다.
struct MissionAiSettingsState {
    std::int32_t stuffRefreshRate{3},startingSp{2500}; // 005318d4/0052f3c8이며 float 비트를 정수로 해석하지 않는다.
    bool checkingAsserts{}; // 005e4794가 켜져 있으면 알 수 없는 능력 이름을 먼저 진단한다.
};
// 문자열 조회는 같은 현재 미션의 mission.<key>를 읽는다. atof는 원본 CRT 변환의 명시 경계다.
struct MissionAiSettingsHooks {
    std::function<std::string(std::string_view)> get; // 00482c90/00482d50의 현재 미션 키 조회다.
    std::function<double(std::string_view)> parseFloat; // 004e4359의 실제 atof 결과이며 비어 있지 않은 세 rate에만 필요하다.
    std::function<void(std::string_view)> unknownAbilityAssertion,unknownAbilityLog; // 004e0620→004c2630(29)의 순서를 보존한다.
};
// 초기화하지 않은 나머지 바이트는 caller가 소유한다. 전체 storage를 지우거나 문자열 공백을 보충하지 않는다.
class RawMissionAiSettings {
public:
    static constexpr std::size_t kSize=600; // 0040ef80/00416860에서 쓰는 실제 설정 크기다.
    // PATCH전용이며 현재 전역 참조와 실제 설정/CRT/진단 효과를 빌린다.
    RawMissionAiSettings(OriginalEdition edition,MissionAiSettingsState& state,MissionAiSettingsHooks hooks);
    // 0040ef80: 실제 기본값이 쓰는 바이트만 바꾸고 나머지 caller 바이트를 보존한다.
    void Initialize(std::span<std::uint8_t> storage) const;
    // 004c2d90: owner<=0은 aiName, owner>0은 ai<owner>Name 꼴이며 빈 값은 이전 필드를 유지한다.
    void Apply(std::span<std::uint8_t> storage,std::int32_t owner) const;
    // 004c2d40의 실제 순서인 기본값→공통(-1)→owner를 실행한다. AI 생성 자체는 호출자 경계다.
    void Prepare(std::span<std::uint8_t> storage,std::int32_t owner) const;
    // 0040f030: ASCII 대소문자를 무시하고 원본 13개 이름 또는 미등록 index13의 비트를 수정한다.
    void SetAbility(std::span<std::uint8_t> storage,std::string_view name,bool enabled) const;
private:
    MissionAiSettingsState& state_; // 같은 실제 기본 전역과 진단 모드다.
    MissionAiSettingsHooks hooks_; // 매 조회마다 현재 값을 반환하는 실제 경계다.
};
}
