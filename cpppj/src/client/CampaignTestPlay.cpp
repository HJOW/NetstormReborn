// 테스트 전용 캠페인 자동 플레이 설정의 명령줄 해석·프로세스 전역 보관·난이도/AI 설정 보조 구현이다.
// (2026-11-14 사용자 요청으로 신규 추가. 일반 사용자 실행 경로에서는 지정되지 않는다.)
#include "client/CampaignTestPlay.h"
#include "o/AiAllianceSide.h"
#include "o/OriginalText.h"
#include "o/RawMissionAiSettings.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 테스트 전용 설정의 구현이다.
namespace { // 이 파일에서만 쓰는 상수와 작은 도우미다.
// 600BYTE AI 설정의 필드 오프셋이다. RawMissionAiSettings::Initialize/Apply가 쓰는 위치와 같다.
constexpr std::size_t kNameOffset = 0x00; // ai%sName(NUL 포함 30BYTE)
constexpr std::size_t kNameBytes = 0x1e; // 이름 칸 크기이며 바로 뒤가 기술 목록이다.
constexpr std::size_t kTechOffset = 0x1e; // ai%sTech 첫 BYTE이며 0이면 초기 갱신이 기술을 추가하지 않는다.
constexpr std::size_t kStartMoneyOffset = 0x22c; // ai%sStartMoney(signed DWORD)
constexpr std::size_t kForcedEnemyOffset = 0x240; // ai%sEnemy
constexpr std::size_t kNoTempleOffset = 0x244; // ai%sNoTemple
constexpr std::size_t kColorOffset = 0x250; // ai%sColor
constexpr std::size_t kOffOffset = 0x254; // ai%sOff(비영이면 AI가 첫 Tick에 스스로 종료)

// 프로세스 전체에서 공유하는 테스트 설정이다. main이 Client 생성 전에만 쓰고 이후에는 읽기만 한다.
CampaignTestPlayOptions& Storage() { static CampaignTestPlayOptions options; return options; }
// 지정 위치에 little endian DWORD를 쓴다.
void Put(std::span<std::uint8_t> bytes, std::size_t offset, std::uint32_t value) {
    if (offset > bytes.size() || bytes.size() - offset < 4) throw std::out_of_range("테스트 AI 설정 쓰기 범위 오류");
    // 네 BYTE만 낮은 비트부터 기록한다.
    for (std::size_t index = 0; index < 4; ++index) bytes[offset + index] = static_cast<std::uint8_t>(value >> (8 * index));
}
// 난이도 이름 또는 숫자를 해석한다. 영어 이름(대소문자 무시)과 옵션 파일 값 1/2/3을 받는다.
o::AiDifficulty ParseLevel(std::string_view option, std::string_view text) {
    const auto lower = o::AsciiLower(text);
    if (lower == "easy" || lower == "1") return o::AiDifficulty::Easy;
    if (lower == "normal" || lower == "2") return o::AiDifficulty::Normal;
    if (lower == "hard" || lower == "3") return o::AiDifficulty::Hard;
    throw std::invalid_argument(std::string(option) + " 값은 easy/normal/hard(1/2/3)여야 합니다: " + std::string(text));
}
// 시작 SP 정수를 해석한다. 숫자 외 문자나 허용 범위 밖은 거부한다.
std::int32_t ParseSp(std::string_view option, const std::string& text) {
    std::size_t used{}; long long value{};
    try { value = std::stoll(text, &used, 10); } catch (const std::exception&) { used = 0; }
    if (text.empty() || used != text.size() || value < 0 || value > kCampaignTestPlayMaxSp)
        throw std::invalid_argument(std::string(option) + " 값은 0.." + std::to_string(kCampaignTestPlayMaxSp) + " 정수여야 합니다: " + text);
    return static_cast<std::int32_t>(value);
}
// 로그용 난이도 이름이다.
const char* LevelName(o::AiDifficulty level) {
    switch (level) {
    case o::AiDifficulty::Easy: return "쉬움";
    case o::AiDifficulty::Hard: return "어려움";
    default: return "보통";
    }
}
}

// 지원하는 다섯 옵션만 처리하고 나머지는 호출자의 기존 오류 처리에 맡긴다.
bool ParseCampaignTestPlayOption(std::string_view argument, const std::function<std::string()>& value, CampaignTestPlayOptions& options) {
    if (argument == "--test-autoplay") { options.autoplay = true; return true; }
    if (argument == "--test-ally-ai") { options.allyAi = ParseLevel(argument, value()); return true; }
    if (argument == "--test-enemy-ai") { options.enemyAi = ParseLevel(argument, value()); return true; }
    if (argument == "--test-start-sp") { options.startSp = ParseSp(argument, value()); return true; }
    if (argument == "--test-enemy-start-sp") { options.enemyStartSp = ParseSp(argument, value()); return true; }
    if (argument == "--test-follow-battle") { options.followBattle = true; return true; } // 관전 카메라(2026-11-14 추가)다.
    return false;
}
// 프리셋은 명시 값을 보존하며 빈 항목만 채운다. 관전 카메라도 함께 켠다.
void FinalizeCampaignTestPlay(CampaignTestPlayOptions& options) {
    if (!options.autoplay) return;
    if (!options.allyAi) options.allyAi = o::AiDifficulty::Hard;
    if (!options.enemyAi) options.enemyAi = o::AiDifficulty::Easy;
    options.followBattle = true;
}
// 전역 보관소에 복사한다.
void ConfigureCampaignTestPlay(const CampaignTestPlayOptions& options) { Storage() = options; }
// 지정 전에는 모든 항목이 비어 있는 비활성 값이다.
const CampaignTestPlayOptions& ConfiguredCampaignTestPlay() { return Storage(); }
// 지정하지 않은 항목은 "(미션값)" 또는 "(옵션값)"으로 적는다.
std::string DescribeCampaignTestPlay(const CampaignTestPlayOptions& options) {
    std::string text = "아군AI=";
    text += options.allyAi ? LevelName(*options.allyAi) : "(없음)";
    text += " 적AI="; text += options.enemyAi ? LevelName(*options.enemyAi) : "(옵션값)";
    text += " 시작SP="; text += options.startSp ? std::to_string(*options.startSp) : "(미션값)";
    text += " 적시작SP="; text += options.enemyStartSp ? std::to_string(*options.enemyStartSp) : "(미션값)";
    text += " 관전카메라="; text += options.followBattle ? "켬" : "끔";
    return text;
}
// 적/아군 구분은 호출자(Session)가 이미 계산한 enemy를 그대로 쓴다.
CampaignAiDifficultyQuery WrapCampaignTestPlayDifficulty(const CampaignTestPlayOptions& options, CampaignAiDifficultyQuery base) {
    return [ally = options.allyAi, enemy = options.enemyAi, base = std::move(base)](std::uint32_t owner, bool isEnemy) -> std::uint32_t {
        if (isEnemy && enemy) return o::AiDifficultyToValue(*enemy);
        if (!isEnemy && ally) return o::AiDifficultyToValue(*ally);
        return base ? base(owner, isEnemy) : 0U; // 지정하지 않은 쪽은 기존 옵션/스커미시 규칙을 그대로 쓴다.
    };
}
// 원문 MakeAi의 owner 비영 분기(기술 추가 없음·현재 SP)를 따르고, 로컬 플레이어에 붙일 때 필요한 이름/동맹 보존을 더한다.
void PrepareLocalTestAiSettings(std::span<std::uint8_t> settings, std::span<const std::uint8_t> localName, float localSp, std::uint32_t localAllies) {
    if (settings.size() != o::RawMissionAiSettings::kSize) throw std::invalid_argument("테스트 AI 설정은 600BYTE여야 한다");
    const auto nul = std::find(localName.begin(), localName.end(), std::uint8_t{});
    const auto length = std::min<std::size_t>(static_cast<std::size_t>(nul - localName.begin()), kNameBytes - 1); // NUL 칸을 남긴다.
    std::fill(settings.begin() + kNameOffset, settings.begin() + kNameOffset + kNameBytes, std::uint8_t{}); // 이름 칸 전체를 비워 뒤 BYTE를 남기지 않는다.
    std::copy_n(localName.begin(), length, settings.begin() + kNameOffset); // 초기 갱신이 Player 이름을 이 값으로 다시 쓰므로 원래 이름을 유지한다.
    settings[kTechOffset] = 0; // 원문 MakeAi처럼 기술을 추가하지 않아 미션 myTech만으로 생산한다.
    const double money = std::isfinite(localSp) ? std::trunc(static_cast<double>(localSp)) : 0.0; // 원문 ftol 절삭이며 비유한 값은 0이다.
    const auto clamped = std::clamp(money, 0.0, static_cast<double>(std::numeric_limits<std::int32_t>::max()));
    Put(settings, kStartMoneyOffset, static_cast<std::uint32_t>(static_cast<std::int32_t>(clamped)));
    Put(settings, o::kAiSettingsAllyMaskOffset, localAllies); // 생성자의 SetAlliance가 로컬 동맹을 지우지 않게 현재 마스크를 그대로 준다.
    Put(settings, kForcedEnemyOffset, 0); Put(settings, kNoTempleOffset, 0); Put(settings, kColorOffset, 0); Put(settings, kOffOffset, 0); // 적 자동 선택·신전 유지·색 유지·동작 유지다.
}
// 시작 자금 DWORD만 바꾼다.
void SetAiSettingsStartMoney(std::span<std::uint8_t> settings, std::int32_t money) {
    if (settings.size() != o::RawMissionAiSettings::kSize) throw std::invalid_argument("AI 시작 자금 설정은 600BYTE여야 한다");
    Put(settings, kStartMoneyOffset, static_cast<std::uint32_t>(money));
}
}
