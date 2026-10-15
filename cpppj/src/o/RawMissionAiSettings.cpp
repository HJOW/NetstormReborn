// AI 설정의 원본 필드 폭·조회 순서·비어 있는 override와 가산 동맹을 보존한다.
#include "o/RawMissionAiSettings.h"
#include "o/ConfigInterface.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::string_view kDefaultTech="sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher"; // 00501000의 NUL 포함75바이트다.
constexpr std::array<std::string_view,13> kAbilities{{"FOCUS_ON_THINGS_THAT_ATTACK_BUILDINGS","BUILD_BLOCKERS_WHEN_BUILDINGS_HARMED","ATTACH_TO_ENEMY_ISLAND","STOP_COLLECTING_FROM_DANGEROUS_GEYSERS","COLLECT_STORM_POWER","ENCIRCLE_ENEMY_ISLAND","CONCENTRATE_FIRE","REPEAT_SUCCESSFUL_DROPS","CAPTURE_ENEMY_PRIEST","BUILD_DAIS","SACRIFICE_ENEMY_PRIEST","USE_PRIEST_TO_COLLECT","BUILD_BRIDGES"}}; // 0052f3fc의 실제 포인터 순서다.
constexpr std::array<std::string_view,9> kColors{{"none","blue","red","white","green","purple","yellow","lightblue","orange"}}; // 00542660의 실제 슬래시 목록 순서다.
// 이 모듈의 실제 600바이트 계약을 확인한다.
void Check(std::span<const std::uint8_t> raw) { if (raw.size()!=RawMissionAiSettings::kSize) throw std::invalid_argument("AI설정은실제600바이트여야합니다"); }
// 실제 DWORD를 little endian 순서로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t result=0;
    // 원본 필드의 네 바이트만 합친다.
    for (std::size_t index=0;index<4;++index) result|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return result;
}
// 실제 DWORD를 little endian 순서로 쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 주변의 미초기화 바이트는 보존한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 원본 strcpy처럼 NUL까지 쓰며 호스트 storage 밖의 손상은 명시적으로 거부한다.
void Copy(std::span<std::uint8_t> raw,std::size_t offset,std::string_view value) {
    value=value.substr(0,value.find('\0'));
    if (value.size()>=raw.size()-offset) throw std::out_of_range("AI설정문자열이실제저장소를벗어납니다");
    std::copy(value.begin(),value.end(),raw.begin()+static_cast<std::ptrdiff_t>(offset));raw[offset+value.size()]=0;
}
// 원본의 ASCII 설정 이름 __stricmp 비교다.
bool Equal(std::string_view left,std::string_view right) {
    if (left.size()!=right.size()) return false;
    const auto lower=[](unsigned char value) { return value>='A' && value<='Z' ? value+32U : value; }; // 언어 환경과 무관한 실제 이름의 ASCII 소문자다.
    // 첫 차이에서 중단한다.
    for (std::size_t index=0;index<left.size();++index) if (lower(static_cast<unsigned char>(left[index]))!=lower(static_cast<unsigned char>(right[index]))) return false;return true;
}
}

// 실제 설정 조회가 없으면 준비를 시작하지 않는다.
RawMissionAiSettings::RawMissionAiSettings(OriginalEdition edition,MissionAiSettingsState& state,MissionAiSettingsHooks hooks):state_(state),hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078 || !hooks_.get) throw std::invalid_argument("MissionAI는PATCH실제설정조회가필요합니다");
}

// CRT ftol 앞의 FILD startingSp +4000.0을 정확한 정수 합으로 유지하고 반환 하위 DWORD를 보존한다.
void RawMissionAiSettings::Initialize(std::span<std::uint8_t> raw) const {
    Check(raw);raw[0]=0;Copy(raw,0x1e,kDefaultTech);Put(raw,0x220,std::bit_cast<std::uint32_t>(6.0F));Put(raw,0x224,std::bit_cast<std::uint32_t>(static_cast<float>(state_.stuffRefreshRate)));Put(raw,0x228,std::bit_cast<std::uint32_t>(3.0F));
    Put(raw,0x22c,static_cast<std::uint32_t>(static_cast<std::int64_t>(state_.startingSp)+4000));Put(raw,0x230,3);Put(raw,0x234,3);Put(raw,0x238,4);Put(raw,0x23c,0);Put(raw,0x240,0);Put(raw,0x244,0);Put(raw,0x24c,0);Put(raw,0x250,0);Put(raw,0x254,0);Put(raw,0x248,0xffff);
}

// 원본은 미등록 이름을 보고한 뒤 index13 비트도 수정한다. 임의로 무시하지 않는다.
void RawMissionAiSettings::SetAbility(std::span<std::uint8_t> raw,std::string_view name,bool enabled) const {
    Check(raw);name=name.substr(0,name.find('\0'));if (name.empty()) return;
    std::size_t index=0;
    // 실제 NULL 종료 이름 표에서 일치하거나 마지막 NULL 위치까지 찾는다.
    while (index<kAbilities.size() && !Equal(name,kAbilities[index])) ++index;
    if (index==kAbilities.size()) {
        if (state_.checkingAsserts) { if (!hooks_.unknownAbilityAssertion) throw std::logic_error("AI능력이름assert경계누락");hooks_.unknownAbilityAssertion(name); }
        if (!hooks_.unknownAbilityLog) throw std::logic_error("AI능력이름log경계누락");hooks_.unknownAbilityLog(name);
    }
    const auto bit=1U<<(index&31U);Put(raw,0x248,enabled ? Read(raw,0x248)|bit : Read(raw,0x248)&~bit);
}

// 공통과 owner 적용은 매 번 현재 설정을 조회한다. 목록 열 개를 하나로 합치지 않는다.
void RawMissionAiSettings::Apply(std::span<std::uint8_t> raw,std::int32_t owner) const {
    Check(raw);const auto prefix=std::string("ai")+(owner>0 ? std::to_string(owner) : "");
    const auto get=[&](std::string_view suffix) { return hooks_.get(prefix+std::string(suffix)); }; // 원본 형식 문자열이 만든 현재 키다.
    auto value=get("Name");if (!value.empty()) Copy(raw,0,value);value=get("Tech");if (!value.empty()) Copy(raw,0x1e,value);
    constexpr std::array<std::pair<std::string_view,std::size_t>,3> rates{{{"BridgeDrawRate",0x220},{"StuffRefreshRate",0x224},{"TimeBetweenMoves",0x228}}}; // 세 atof→float 필드의 실제 순서다.
    // 비어 있지 않은 값에만 원본 atof 경계를 호출한다.
    for (const auto& [key,offset]:rates) { value=get(key);if (!value.empty()) { if (!hooks_.parseFloat) throw std::logic_error("AI설정atof경계누락");Put(raw,offset,std::bit_cast<std::uint32_t>(static_cast<float>(hooks_.parseFloat(value)))); } }
    constexpr std::array<std::pair<std::string_view,std::size_t>,7> integers{{{"StartMoney",0x22c},{"GeyserAttachments",0x230},{"MaxInHand",0x238},{"Collectors",0x234},{"Enemy",0x240},{"NoTemple",0x244},{"Off",0x254}}}; // 일곱 atol 필드의 실제 순서다.
    // 원본의 일곱 atol 필드를 같은 순서로 적용한다.
    for (const auto& [key,offset]:integers) { value=get(key);if (!value.empty()) Put(raw,offset,static_cast<std::uint32_t>(ConfigParseLong(value))); }
    value=get("Color");
    // none/index0과 미등록 값은 기존 색을 지우지 않는다.
    for (std::size_t color=1;color<kColors.size();++color) if (Equal(value,kColors[color])) { Put(raw,0x250,static_cast<std::uint32_t>(color));break; }
    // 공통/owner 목록은 원본대로 가산하며8을 넘는 양수도 shift의 하위5비트로 처리한다.
    for (int index=0;index<10;++index) { const auto ally=ConfigParseLong(ConfigListItem(get("AllyList"),index));if (ally>0) Put(raw,0x23c,Read(raw,0x23c)|(1U<<(static_cast<std::uint32_t>(ally)&31U))); }
    // 느낌표는 선두 한 글자만 제거하며 빈 이름은 아무 것도 하지 않는다.
    for (int index=0;index<10;++index) { value=ConfigListItem(get("Ability"),index);const bool enabled=value.empty() || value.front()!='!';SetAbility(raw,enabled ? std::string_view(value) : std::string_view(value).substr(1),enabled); }
}

// 같은 storage에 누적하여 공통의 동맹/능력이 owner 값에 보존되게 한다.
void RawMissionAiSettings::Prepare(std::span<std::uint8_t> raw,std::int32_t owner) const { Initialize(raw);Apply(raw,-1);Apply(raw,owner); }
}
