// 관찰 callback으로 게임 계산을 대신하지 않고 기존 원본 몸체의 실제 Session 소비자를 공급한다.
#include "client/RawCampaignMissionStartBindings.h"
#include "client/DisplayTextInput.h" // 기존 한국어 표시 토큰의 유효성을 공유한다.
#include "o/OriginalText.h"
#include "o/AiAllianceSide.h" // 테스트 전용 적 시작 SP의 아군/적 판정이다(2026-11-14 추가).
#include <array>
#include <cstdlib>
#include <iostream>
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
    hooks.get=[this](std::string_view key) { const auto name=o::AsciiLower(key);const bool display=name.size()>=4 && name.ends_with("name");const auto text=script_.Get(key).value_or(std::string{});const auto bytes=display ? o::EncodeDisplayText(text) : o::EncodeOriginalText(text);const std::string value(bytes.begin(),bytes.end());if (display && HasExtendedDisplayText(value) && bytes.size()>29) throw std::length_error("확장 AI 이름은 한글 표시토큰을 포함해29BYTE 이하여야 합니다");return value; }; // 새 확장 Name만 제한하고 기존 CP1252 대본의 원문 복사 규칙은 보존한다.
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
    hooks.setSp=[this](std::uint32_t owner,float money) {
        // 테스트 전용: --test-start-sp가 있으면 myStartMoney 대신 지정한 로컬 시작 SP를 쓴다(2026-11-14 추가). 일반 실행은 원본 값 그대로다.
        if (const auto* test=TestPlay();test && test->startSp && owner==session_.LocalOwner()) money=static_cast<float>(*test->startSp);
        session_.SetSp(owner,money);
    };
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
// 12차 스커미시: 일반 미션은 prepareWorld가 비어 있어 기존과 같고, 스커미시는 Start가 사제를 찾기 전에 MP 규칙 섬 생성을 먼저 실행한다.
// 테스트 전용 자동 플레이(allyAi)는 원본 Start가 모두 끝난 뒤 로컬 AI를 붙인다. 원본 Start 순서/효과는 바꾸지 않는다(2026-11-14 추가).
void RawCampaignMissionStartBindings::Start() const {
    if (effects_.prepareWorld) effects_.prepareWorld();start_.Start();
    if (const auto* test=TestPlay();test && test->allyAi) {
        const auto token=CreateLocalTestAi(); // 사제가 없거나 이미 AI가 있으면 0이며 미션은 그대로 진행한다.
        std::clog<<"테스트 자동 플레이: 로컬 owner "<<session_.LocalOwner()<<(token ? " AI 생성 완료 (" : " AI 생성 생략 (")<<DescribeCampaignTestPlay(*test)<<")\n";
    }
}
// 호출자가 공급한 미초기화 영역에 기존 600BYTE 설정 초기화 순서를 한 번 적용한다.
std::uint32_t RawCampaignMissionStartBindings::CreateAi(std::uint32_t owner) const {
    if (owner<1 || owner>8) throw std::out_of_range("미션 AI 소유자 범위 오류");
    std::array<std::uint8_t,o::RawMissionAiSettings::kSize> bytes;
    effects_.settingsBytes(owner,bytes);settings_.Prepare(bytes,static_cast<std::int32_t>(owner));
    // 테스트 전용: --test-enemy-start-sp가 있으면 적 AI의 시작 자금(ai%sStartMoney)만 바꾼다. 동맹 AI는 미션 값을 유지한다(2026-11-14 추가).
    if (const auto* test=TestPlay();test && test->enemyStartSp) {
        const auto local=session_.LocalOwner();
        if (local<9 && o::IsAiEnemyOfLocal(owner,local,ReadPlayerDword(local,0x54),bytes)) SetAiSettingsStartMoney(bytes,*test->enemyStartSp);
    }
    return session_.CreateAi(owner,bytes);
}
// 데모는 원본이 이미 로컬 AI를 붙이고 난이도도 보통으로 고정하므로 테스트 설정을 적용하지 않는다. 편집 모드도 제외한다.
const CampaignTestPlayOptions* RawCampaignMissionStartBindings::TestPlay() const {
    if (!effects_.testPlay || !effects_.testPlay->Active() || gate_.demo!=0 || session_.Editor()!=0) return nullptr;
    return &*effects_.testPlay;
}
// 원문 MakeAi(00417fa0) owner 비영 분기처럼 기본 설정(0040ef80)만 쓰고 미션 공통 ai* 키는 읽지 않는다.
// 공통 키(예: finalconfrontation의 aiAllyList="2;3;4;5")를 로컬에 적용하면 적과 동맹이 되는 등 미션이 깨지기 때문이다.
std::uint32_t RawCampaignMissionStartBindings::CreateLocalTestAi() const {
    const auto owner=session_.LocalOwner();
    if (owner<1 || owner>8 || !FindLivePriest(owner) || session_.Participation().Brain(owner)!=0) return 0; // 원본 데모 분기도 살아 있는 사제가 있는 owner만 AI를 만든다.
    std::array<std::uint8_t,o::RawMissionAiSettings::kSize> bytes;
    effects_.settingsBytes(owner,bytes);settings_.Initialize(bytes);
    const auto records=session_.PlayerRecords();const auto nameAt=static_cast<std::size_t>(owner)*0xb4+12; // Player+0c의 현재 이름이며 다음 필드(+2c) 전까지 32BYTE다.
    PrepareLocalTestAiSettings(bytes,records.subspan(nameAt,0x20),session_.Sp(owner),ReadPlayerDword(owner,0x54));
    return session_.CreateAi(owner,bytes);
}
// 현재 Player 기록의 비정렬 DWORD를 읽는다. 9명 범위 밖은 거부한다.
std::uint32_t RawCampaignMissionStartBindings::ReadPlayerDword(std::uint32_t owner,std::size_t offset) const {
    const auto records=session_.PlayerRecords();const auto at=static_cast<std::size_t>(owner)*0xb4+offset;
    if (owner>=9 || at+4>records.size()) throw std::out_of_range("테스트 자동 플레이 Player 기록 범위 오류");
    std::uint32_t value{};
    // 네 BYTE를 낮은 비트부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(records[at+index])<<(8*index);
    return value;
}
// 현재 프레임 소비자에게 같은 Start의 주소만 빌려준다.
const o::RawMissionStart& RawCampaignMissionStartBindings::StartBody() const { return start_; }
// 현재 프레임/Guide의 장기 상태를 복사하지 않는다.
const o::MissionFrameGateState& RawCampaignMissionStartBindings::Gate() const { return gate_; }
// 실제 사제 목록의 현재 owner/dead를 읽어 포획 상태를 추정하지 않는다.
std::uint32_t RawCampaignMissionStartBindings::FindLivePriest(std::uint32_t owner) const { return priests_.Find(owner); }
}
