// 테스트 전용 캠페인 자동 플레이 설정이다. 원본 10.78에 없는 개발/검증용 기능이며 일반 사용자 실행(무인자 Netstorm.exe)에서는 켤 수 없다.
// 명령줄(--run/--mcp 뒤의 --test-* 옵션)로만 켜지고 options.cfg·게임 내 메뉴에 저장/노출되지 않는다.
// 켜면 캠페인 미션과 스커미시에서 로컬 플레이어(아군)에도 원본 데모 분기와 같은 AI를 붙이고, 아군/적 AI 난이도와 시작 SP를 바꿀 수 있다.
// 편집 Test Battle·데모·LAN/MP에는 적용하지 않는다. 사용 안내: docs/testing/cpp-autoplay-test-guide.md
// (2026-11-14 사용자 요청: 우연성이 큰 캠페인 플레이 검증을 아군 AI 자동 플레이로 재현하기 위해 신규 추가, 같은 날 스커미시까지 확장)
#pragma once
#include "o/AiDifficulty.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 클라이언트 연결층이며 원문 복원 몸체(o)는 이 설정을 알지 못한다.
// 테스트 전용 시작 SP의 허용 범위다. 음수나 원본 float SP가 정수를 정확히 담지 못하는 큰 값은 거부한다(2026-11-14 신규).
inline constexpr std::int32_t kCampaignTestPlayMaxSp = 10000000;

// 명령줄로 지정한 테스트 전용 자동 플레이 설정이다. 값이 없는 항목은 일반 게임과 같다.
// 사용법: main이 ParseCampaignTestPlayOption으로 인자를 모은 뒤 FinalizeCampaignTestPlay→ConfigureCampaignTestPlay 순으로 한 번 지정한다.
// (2026-11-14 신규)
struct CampaignTestPlayOptions {
    bool autoplay{}; // --test-autoplay 프리셋 요청이다. Finalize에서 지정되지 않은 아군/적 난이도를 어려움/쉬움으로 채운다.
    std::optional<o::AiDifficulty> allyAi; // 값이 있으면 로컬 플레이어에도 AI를 붙이고, 로컬 AI와 로컬 동맹 AI의 난이도로 쓴다.
    std::optional<o::AiDifficulty> enemyAi; // 값이 있으면 옵션 메뉴 난이도 대신 적 AI 난이도로 쓴다.
    std::optional<std::int32_t> startSp; // 값이 있으면 미션의 myStartMoney 대신 로컬 플레이어 시작 SP로 쓴다(로컬 AI의 지갑도 같은 값에서 시작한다).
    std::optional<std::int32_t> enemyStartSp; // 값이 있으면 미션의 ai%sStartMoney 대신 적 AI의 시작 SP(지갑)로 쓴다.
    bool followBattle{}; // 관전 카메라: 주요 전투 위치, 사제 희생 의식 중에는 제단으로 화면을 옮긴다(2026-11-14 추가, --test-autoplay 프리셋에 포함).
    // 하나라도 지정되어 일반 게임과 달라지는지 돌려준다. autoplay 단독은 Finalize 전에는 활성으로 보지 않는다.
    bool Active() const { return allyAi.has_value() || enemyAi.has_value() || startSp.has_value() || enemyStartSp.has_value() || followBattle; }
};

// 명령줄 인자 하나가 테스트 전용 옵션이면 해석해 options에 반영하고 true를 돌려준다. 값이 필요한 옵션은 value()로 다음 인자를 소비한다.
// 지원: --test-autoplay, --test-ally-ai <easy|normal|hard|1|2|3>, --test-enemy-ai <같은 값>, --test-start-sp <0..10000000>, --test-enemy-start-sp <같은 범위>,
// --test-follow-battle(관전 카메라, 2026-11-14 추가).
// 잘못된 값은 std::invalid_argument로 거부하며 다른 인자는 false를 돌려준다(2026-11-14 신규).
bool ParseCampaignTestPlayOption(std::string_view argument,const std::function<std::string()>& value,CampaignTestPlayOptions& options);
// --test-autoplay 프리셋을 적용한다(아군 어려움·적 쉬움·관전 카메라). 명시한 --test-ally-ai/--test-enemy-ai 값은 덮어쓰지 않는다(2026-11-14 신규).
void FinalizeCampaignTestPlay(CampaignTestPlayOptions& options);
// 프로세스 전체에 테스트 설정을 지정한다. Client 생성 전 main에서 한 번만 부른다(2026-11-14 신규).
void ConfigureCampaignTestPlay(const CampaignTestPlayOptions& options);
// 현재 프로세스의 테스트 설정을 돌려준다. 지정하지 않았으면 비활성 기본값이다(2026-11-14 신규).
const CampaignTestPlayOptions& ConfiguredCampaignTestPlay();
// 로그/진단용 한 줄 요약이다. 예: "아군AI=어려움 적AI=쉬움 시작SP=30000 적시작SP=(미션값)"(2026-11-14 신규).
std::string DescribeCampaignTestPlay(const CampaignTestPlayOptions& options);

// owner와 적 여부를 받아 난이도 설정값(0/1/2/3)을 돌려주는 조회다. CampaignAiServices::difficulty와 같은 형태다.
using CampaignAiDifficultyQuery=std::function<std::uint32_t(std::uint32_t,bool)>;
// 테스트 설정으로 난이도 조회를 감싼다. 적은 enemyAi, 아군(로컬/동맹)은 allyAi가 있으면 그 값을, 없으면 base 결과를 돌려준다.
// 사용법: 캠페인 미션의 서비스 생성 직후 services.ai.difficulty를 이 반환값으로 교체한다(2026-11-14 신규).
CampaignAiDifficultyQuery WrapCampaignTestPlayDifficulty(const CampaignTestPlayOptions& options,CampaignAiDifficultyQuery base);

// 로컬 플레이어용 테스트 AI의 600BYTE 설정을 원문 MakeAi(00417fa0)의 owner 비영 분기와 같은 규칙으로 고친다.
// settings는 이미 원문 기본값(0040ef80)으로 초기화되어 있어야 하며, 이름=현재 로컬 이름(29BYTE까지), 추가 기술=없음(myTech 유지),
// 시작 자금=현재 로컬 SP, 동맹 마스크=현재 로컬 동맹, 강제 적/NoTemple/색/Off=0으로 둔다. 미션 공통 ai* 키는 읽지 않는다(2026-11-14 신규).
void PrepareLocalTestAiSettings(std::span<std::uint8_t> settings,std::span<const std::uint8_t> localName,float localSp,std::uint32_t localAllies);
// 600BYTE AI 설정의 시작 자금(+22c, ai%sStartMoney)을 바꾼다(2026-11-14 신규).
void SetAiSettingsStartMoney(std::span<std::uint8_t> settings,std::int32_t money);
}
