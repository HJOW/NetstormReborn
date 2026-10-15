// 관찰 callback으로 게임 계산을 대신하지 않고 기존 원본 몸체의 실제 Session 소비자를 공급한다.
#include "client/RawCampaignMissionStartBindings.h"
#include "o/OriginalText.h"
#include <array>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 모든 외부 소비자를 명시 요구한다. 현재 미선택 경계도 자동 GUI 통합 때 빈 성공으로 남기지 않는다.
RawCampaignMissionStartBindings::RawCampaignMissionStartBindings(RawCampaignSession& session,MissionScript& script,
    o::MissionFrameGateState& gate,o::MissionStartState& state,o::MissionStartGlobals& globals,
    o::MissionAiSettingsState& aiGlobals,CampaignMissionStartEffects effects)
    :session_(session),script_(script),gate_(gate),aiGlobals_(aiGlobals),effects_(std::move(effects)),
     priests_(session.Core().pool,session.Priests().Priests()),settings_(o::OriginalEdition::Patch1078,aiGlobals_,AiHooks()),
     start_(o::OriginalEdition::Patch1078,gate,state,globals,Hooks()) {
    if (!script_.Loaded() || !effects_.settingsBytes || !effects_.gameSeconds || !effects_.refreshSp ||
        !effects_.refreshCommandGump || !effects_.randomGeysers || !effects_.resetCamera)
        throw std::invalid_argument("실제 MissionStart의 Script/설정초기입력/시계/UI/랜덤간헐천 경계 누락");
    start_.BindCurrentInputs({[this] { return session_.LocalOwner(); },[this] { return session_.PlayerKnown(); },[this] { return session_.Editor(); }});
}
// 원본 atof의 좁은 문자열 변환과 현재 미션 문자열의 Windows-1252 경계를 사용한다.
o::MissionAiSettingsHooks RawCampaignMissionStartBindings::AiHooks() {
    o::MissionAiSettingsHooks hooks;
    hooks.get=[this](std::string_view key) { const auto bytes=o::EncodeOriginalText(script_.Get(key).value_or(std::string{}));return std::string(bytes.begin(),bytes.end()); };
    hooks.parseFloat=[](std::string_view value) { const std::string text(value);return std::strtod(text.c_str(),nullptr); };
    hooks.unknownAbilityAssertion=[this](std::string_view name) { if (!effects_.unknownAbilityAssertion) throw std::logic_error("미션 AI의 실제 능력 이름 진단 미연결");effects_.unknownAbilityAssertion(name); };
    hooks.unknownAbilityLog=[this](std::string_view name) { if (!effects_.unknownAbilityLog) throw std::logic_error("미션 AI의 실제 능력 이름 기록 미연결");effects_.unknownAbilityLog(name); };
    return hooks;
}
// 같은 현재 Script/Session과 원본 getter를 읽기 위치 그대로 연결한다.
o::MissionStartHooks RawCampaignMissionStartBindings::Hooks() {
    o::MissionStartHooks hooks;
    hooks.get=[this](std::string_view key) { return script_.Get(key).value_or(std::string{}); };
    hooks.sectionNames=[this](std::string_view prefix) { return script_.SectionNames(prefix); };
    hooks.set=[this](std::string_view key,std::string_view value) { static_cast<void>(script_.Set(key,value)); };
    hooks.gameSeconds=[this] { return effects_.gameSeconds(); };
    hooks.setSp=[this](std::uint32_t owner,float money) { session_.SetSp(owner,money); };
    hooks.refreshSp=[this] { effects_.refreshSp(); };
    hooks.clearTechnology=[this](std::uint32_t owner) { session_.ClearTechnology(static_cast<std::int32_t>(owner)); };
    hooks.addTechnology=[this](std::uint32_t owner,std::string_view list) { session_.AddTechnology(static_cast<std::int32_t>(owner),list); };
    hooks.hasPriest=[this](std::uint32_t owner) { return FindLivePriest(owner)!=0; };
    hooks.createAi=[this](std::uint32_t owner) { static_cast<void>(CreateAi(owner)); };
    hooks.refreshCommandGump=[this] { effects_.refreshCommandGump(); };
    hooks.randomGeysers=[this] { effects_.randomGeysers(); };
    hooks.resetCamera=[this] { effects_.resetCamera(); };
    return hooks;
}
// 같은 게이트의 원본 시작 표식만 사용하고 별도 성공 사본을 만들지 않는다.
void RawCampaignMissionStartBindings::Start() const { start_.Start(); }
// 호출자가 공급한 미초기화 영역에 기존 600BYTE 설정 초기화 순서를 한 번 적용한다.
std::uint32_t RawCampaignMissionStartBindings::CreateAi(std::uint32_t owner) const {
    if (owner<1 || owner>8) throw std::out_of_range("미션 AI 소유자 범위 오류");
    std::array<std::uint8_t,o::RawMissionAiSettings::kSize> bytes;
    effects_.settingsBytes(owner,bytes);settings_.Prepare(bytes,static_cast<std::int32_t>(owner));return session_.CreateAi(owner,bytes);
}
// 현재 프레임 소비자에게 같은 Start의 주소만 빌려준다.
const o::RawMissionStart& RawCampaignMissionStartBindings::StartBody() const { return start_; }
// 현재 프레임/Guide의 장기 상태를 복사하지 않는다.
const o::MissionFrameGateState& RawCampaignMissionStartBindings::Gate() const { return gate_; }
// 실제 사제 목록의 현재 owner/dead를 읽어 포획 상태를 추정하지 않는다.
std::uint32_t RawCampaignMissionStartBindings::FindLivePriest(std::uint32_t owner) const { return priests_.Find(owner); }
}
