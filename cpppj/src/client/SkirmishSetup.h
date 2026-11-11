// 12차 스커미시: Campaign 메뉴의 Skirmish 설정 화면(AI 수·AI별 난이도/동맹·금지 유닛·시작/취소)과 시작 대본을 만드는 순수 로직이다.
// 원본 10.78에는 스커미시가 없으므로 화면은 원본 HTML 창(Tell)과 같은 부품으로 만들고, 시작은 생성한 미션 대본 + 기존 다인용 fort로 한다.
#pragma once
#include "o/AiDifficulty.h"
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client { // 게임 규칙·UI 문구·설정 저장만 두며 실제 창/월드는 UberGump가 소유한다.
// 생성 미션 이름이다. BeginMission이 이 이름이면 파일 대신 생성한 대본/fort를 읽는다.
inline constexpr std::string_view kSkirmishMissionName = "Skirmish";
// 설정 화면이 열 수 있는 AI 수의 최대값이다(소유자 2..8).
inline constexpr int kSkirmishMaxAi = 7;
// 금지 유닛의 최대 개수다(AGENTS 12차: 최대 5개).
inline constexpr std::size_t kSkirmishMaxBans = 5;
// 한 화면에 보이는 금지 유닛 항목 수다(창 높이 제한으로 쪽을 나눈다).
inline constexpr std::size_t kSkirmishBansPerPage = 12;
// 스커미시 Tell 대상의 객체 접두(제품 전용 Patch1101 객체)다.
inline constexpr std::string_view kSkirmishTargetPrefix = "patch1101.skirmish";
// 금지 유닛 후보 한 칸이다.
struct SkirmishBannable {
    std::string_view type;  // 미션 techAllowed에 쓰는 내부 타입 이름이다.
    std::string_view label; // 화면에 보이는 영어 이름(번역 키)이다.
};
// 금지 유닛으로 고를 수 있는 모든 건물/유닛(Outpost 포함)이다.
std::span<const SkirmishBannable> SkirmishBannableTypes();
// AI 한 명의 설정이다.
struct SkirmishAi {
    o::AiDifficulty difficulty{o::AiDifficulty::Normal}; // 이 AI에만 적용하는 난이도다(옵션 난이도 대신).
    bool ally{};                                         // 참이면 플레이어와 같은 편이다.
};
// 설정 화면 전체의 값이다. 설정 파일(options.cfg)에 저장해 다음 실행에도 유지한다.
struct SkirmishSettings {
    int aiCount{1};                                      // AI 수(1..7)이며 소유자 2..(1+aiCount)가 쓴다.
    std::array<SkirmishAi, kSkirmishMaxAi> ai{};         // 소유자 2..8의 AI 설정이다.
    std::vector<std::string> bans;                       // 금지한 내부 타입 이름이며 최대 5개다.
};
// 설정 값 한 칸을 읽는 함수(키 → 값 문자열, 없으면 빈 문자열)다. 실제로는 Client의 설정 객체를 연결한다.
using SkirmishConfigGet = std::function<std::string(std::string_view)>;
// 설정 값 한 칸을 쓰는 함수(키, 값)다.
using SkirmishConfigSet = std::function<void(std::string_view, std::string_view)>;
// 설정에서 읽는다. 값이 없거나 범위 밖이면 기본값(AI 1명·보통·적·금지 없음)이다.
SkirmishSettings LoadSkirmishSettings(const SkirmishConfigGet& get);
// 설정에 쓴다(options.cfg에는 설정 객체가 변경 알림으로 저장한다).
void SaveSkirmishSettings(const SkirmishConfigSet& set, const SkirmishSettings& settings);
// 스커미시 Tell 대상의 종류다.
enum class SkirmishScreen { None, Main, Count, Ai, Bans, Set, Start };
// 해석한 Tell 대상이다. args는 괄호 안 인자(쉼표 구분)다.
struct SkirmishTarget {
    SkirmishScreen kind{SkirmishScreen::None};
    std::vector<std::string> args;
};
// "Patch1101.SkirmishX(a,b)" 형식을 해석한다. 스커미시 대상이 아니면 kind가 None이다.
SkirmishTarget ParseSkirmishTarget(std::string_view target);
// Set 대상(count/diff/ally/ban/clearbans)을 설정에 적용하고 다음에 보일 화면의 Tell 대상을 돌려준다. 알 수 없는 동작은 Main이다.
std::string ApplySkirmishAction(SkirmishSettings& settings, const std::vector<std::string>& args);
// 화면 한 장의 Tell 본문을 만든다. language는 원문 언어 번호(0/1 영어, 5 일본어, 7 한국어)이고 translate는 유닛 이름 번역이다.
std::string BuildSkirmishScreen(const SkirmishTarget& target, const SkirmishSettings& settings, std::uint32_t language,
    const std::function<std::string(std::string_view)>& translate);
// 시작 섬 한 칸이다: 원본 fort 파일 하나의 영역(Terr) 하나를 한 참가자의 전투섬으로 쓴다(MP에서 참가자가 자기 fort의 전투 영역을 가져오는 것과 같다).
struct SkirmishStartIsland {
    std::string_view fort; // d 디렉터리의 원본 fort 이름(확장자 없음, 예: "school5")이다.
    int territory{};       // 그 fort 안의 영역 번호(0..19)다. 이 영역의 객체(사제)가 이 참가자 소유로 만들어진다.
};
// 시작에 쓰는 생성 미션이다. 월드는 fort 파일이 아니라 MP 시작 규칙(참가자별 전투섬 + 무작위 중앙섬 + 무작위 간헐천)으로 만든다.
struct SkirmishLaunch {
    std::string script;                       // 생성한 미션 대본(english)이다.
    std::vector<SkirmishStartIsland> islands; // 소유자 1..(AI 수+1) 순서의 시작 섬이다(MP 방향표대로 링에 놓인다).
    std::array<std::uint8_t, 6> central{};    // 무작위 중앙섬 영역 기록(패턴|회전, 플래그, 위치 0, 지형 씨앗 세 바이트)이다.
    int mapSize{};                            // 맵 크기 옵션(0..3, 링 간격 3/4/5/6칸)이다. 참가자 수가 많을수록 크다.
};
// 설정과 난수로 시작 자료를 만든다. 시작 섬은 후보 표에서 서로 다른 것을 뽑고 중앙섬은 무작위 패턴/회전/씨앗이다. 후보가 모자라면 islands가 비어 있다.
SkirmishLaunch BuildSkirmishLaunch(const SkirmishSettings& settings, std::uint32_t randomValue);
// 월드 생성 단계가 쓰는 시작 계획이다: 시작 섬마다 원본 fort 바이트를 함께 들고 있어 세션이 파일 시스템 없이 지역 Template로 읽을 수 있다.
struct SkirmishWorldPlan {
    struct Island {
        std::string fort;                 // 원본 fort 이름이다(제목/서명 확인용).
        int territory{};                  // 그 fort의 전투 영역 번호다.
        std::vector<std::uint8_t> bytes;  // 그 fort 파일 전체 내용이다.
    };
    std::vector<Island> islands;          // 소유자 1..N+1 순서의 시작 섬이다.
    std::array<std::uint8_t, 6> central{}; // 중앙섬 영역 기록이다.
    int mapSize{};                        // 맵 크기 옵션(0..3)이다.
};
// 시작 섬 후보 표 전체다(자동 생성 표).
std::span<const SkirmishStartIsland> SkirmishStartIslandPool();
// 스커미시 월드를 가진 가상 미션 fort를 만든다: 기준 fort에서 영역 표/Chaff/Terr 객체를 모두 비워(섬 없는 빈 월드) 기술/덱/돈 같은 나머지 섹션만 남긴다.
// 비어 있거나 형식이 틀린 입력이면 빈 벡터를 돌려준다.
std::vector<std::uint8_t> MakeSkirmishEmptyWorldFort(std::span<const std::uint8_t> baseFort);
// 이 이름이 생성 스커미시 미션인지 판정한다(대소문자 무시).
bool IsSkirmishMissionName(std::string_view name);
}
