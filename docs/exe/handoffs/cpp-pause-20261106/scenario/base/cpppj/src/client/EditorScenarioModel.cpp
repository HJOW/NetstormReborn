// 기존 Config/AI 설정 몸체를 독립 문서에 적용하며 게임 상태나 파일은 변경하지 않는다.
#include "client/EditorScenarioModel.h"
#include "o/Config.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include "o/RawMissionAiSettings.h"
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 화면과 저장 장치를 붙이기 전의 순수 시나리오 편집 모델이다.
namespace { // 지원 필드 검사와 실제 설정 사본 읽기만 담당한다.
static_assert(o::RawMissionAiSettings::kSize==600); // 공개 미리보기 사본이 실제 원문 설정 폭과 같아야 한다.
constexpr std::array<std::string_view,9> kColors{{"none","blue","red","white","green","purple","yellow","lightblue","orange"}}; // RawMissionAiSettings의 실제 색 순서다.
constexpr std::array<std::string_view,13> kAbilities{{"FOCUS_ON_THINGS_THAT_ATTACK_BUILDINGS","BUILD_BLOCKERS_WHEN_BUILDINGS_HARMED","ATTACH_TO_ENEMY_ISLAND","STOP_COLLECTING_FROM_DANGEROUS_GEYSERS","COLLECT_STORM_POWER","ENCIRCLE_ENEMY_ISLAND","CONCENTRATE_FIRE","REPEAT_SUCCESSFUL_DROPS","CAPTURE_ENEMY_PRIEST","BUILD_DAIS","SACRIFICE_ENEMY_PRIEST","USE_PRIEST_TO_COLLECT","BUILD_BRIDGES"}}; // UI에서 새 이름을 만들지 않는 실제13개 표다.
constexpr std::array<std::string_view,15> kAiSuffixes{{"Name","Tech","BridgeDrawRate","StuffRefreshRate","TimeBetweenMoves","StartMoney","GeyserAttachments","MaxInHand","Collectors","Enemy","NoTemple","Off","Color","AllyList","Ability"}}; // 실제 Apply가 읽는 suffix 전체다.
constexpr std::array<std::string_view,12> kOrdinaryKeys{{"title","missionType","missionNumber","tutorialNumber","myStartMoney","myTech","techAllowed","moreGeysers","denySalvage","denyAscend","allowAnyCapture","randGeysers"}}; // 첫 편집 UI가 지원하는 원문 소비 키다.
// 지원키를 확인한 결과이며 AI owner0은 공통 ai* 키를 뜻한다.
struct FieldKind {
    std::string canonical; // 실제 철자의 전체 키다.
    std::string suffix; // AI 키면 실제 suffix, 일반 키면 빈 값이다.
    std::uint32_t owner{}; // 공통0 또는 owner1~8이다.
};
// ASCII 원문 이름의 대소문자만 무시한다.
bool Equal(std::string_view left,std::string_view right) { return o::AsciiLower(left)==o::AsciiLower(right); }
// 1~8만 실제 owner로 인정하며 이름 문자열에서 owner 존재를 추측하지 않는다.
void CheckOwner(std::uint32_t owner) { if (owner<1 || owner>8) throw std::out_of_range("시나리오 owner는 1~8이어야 합니다"); }
// 임의 미지 키는 문서에 남기되 이 지원 필드 편집 API로 새 의미를 부여하지 않는다.
FieldKind Describe(std::string_view key) {
    for (const auto ordinary:kOrdinaryKeys) { // 일반 키는 원문 철자를 돌려준다.
        if (Equal(key,ordinary)) return {std::string(ordinary),{},0};
    }
    if (key.size()>=2 && Equal(key.substr(0,2),"ai")) {
        auto suffix=key.substr(2);std::uint32_t owner=0;
        if (!suffix.empty() && suffix.front()>='0' && suffix.front()<='9') { owner=static_cast<std::uint32_t>(suffix.front()-'0');CheckOwner(owner);suffix.remove_prefix(1); }
        for (const auto supported:kAiSuffixes) { // ai01 등 원문이 만들지 않는 alias는 일치하지 않는다.
            if (Equal(suffix,supported)) return {"ai"+(owner ? std::to_string(owner) : "")+std::string(supported),std::string(supported),owner};
        }
    }
    throw std::invalid_argument("편집을 지원하지 않는 시나리오 키입니다: "+std::string(key));
}
// 백틱으로 보호되지 않은 중괄호만 동적 식으로 표시한다. 실제 치환은 Config가 수행한다.
bool Dynamic(std::string_view value) {
    for (std::size_t index=0;index<value.size();++index) { // escape 다음 한 글자는 동적 문법으로 보지 않는다.
        if (value[index]=='`' && index+1<value.size()) ++index;else if (value[index]=='{') return true;
    }
    return false;
}
// 새 입력만 완전한 signed32 숫자로 검사한다. 기존 원문의 atol prefix/wrap은 수정하지 않는다.
void CheckInteger(std::string_view value) {
    if (!value.empty() && value.front()=='+') { value.remove_prefix(1);if (!value.empty() && (value.front()=='+' || value.front()=='-')) throw std::invalid_argument("시나리오 정수의 부호가 중복되었습니다"); }
    std::int32_t result{};const auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    if (value.empty() || parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size()) throw std::invalid_argument("새 시나리오 수치는 signed32 정수여야 합니다");
}
// 새 rate만 전체가 유한 float 범위인지 검사한다. 기존 atof 해석은 AI 뷰가 그대로 사용한다.
void CheckRate(std::string_view value) {
    const std::string text(value);char* end=nullptr;const auto result=std::strtod(text.c_str(),&end);
    if (end==text.c_str() || end!=text.c_str()+text.size() || !std::isfinite(result) || std::abs(result)>std::numeric_limits<float>::max())
        throw std::invalid_argument("새 AI rate는 유한 float 범위의 숫자여야 합니다");
}
// 기존 목록의 항목 순서를 바꾸지 않고 실제 이름을 검증한다.
void CheckTechnology(std::string_view value,bool allowed,const EditorScenarioContext& context) {
    if (o::EncodeOriginalText(value).size()>=512) throw std::length_error("새 기술 목록은 원문 임시 버퍼 512BYTE 미만이어야 합니다");
    const auto separators=static_cast<int>(std::count(value.begin(),value.end(),';')); // 검증할 전체 새 입력의 구분자 수다.
    for (int index=0;index<=separators;++index) { // 소비자가 빈 항목 뒤를 읽지 않으므로 그런 숨은 새 입력도 거부한다.
        const auto item=o::ConfigListItem(value,index);
        if (item.empty()) throw std::invalid_argument("새 기술 목록에 빈 항목이 있습니다");
        const auto lower=o::AsciiLower(item);
        if (lower!="all" && !(allowed && (lower=="allow" || lower=="deny")) && (!context.knownTechnology || !context.knownTechnology(item)))
            throw std::invalid_argument("실제 타입 표에서 확인되지 않은 새 기술 이름입니다: "+item);
    }
}
// 설정 DWORD의 실제 little endian 값을 읽는다.
std::uint32_t Read(const std::array<std::uint8_t,600>& bytes,std::size_t offset) {
    std::uint32_t result=0;
    for (std::size_t index=0;index<4;++index) result|=static_cast<std::uint32_t>(bytes[offset+index])<<(index*8); // 정확히 네 바이트만 합친다.
    return result;
}
// 설정의 NUL 종료 CP1252 문자열만 UTF-8 표시값으로 읽고 원시 문서는 바꾸지 않는다.
std::string ReadText(const std::array<std::uint8_t,600>& bytes,std::size_t offset) {
    const auto begin=bytes.begin()+static_cast<std::ptrdiff_t>(offset),end=std::find(begin,bytes.end(),std::uint8_t{});
    if (end==bytes.end()) throw std::invalid_argument("시나리오 AI 설정 문자열의 NUL이 없습니다");
    return o::DecodeOriginalText({&*begin,static_cast<std::size_t>(end-begin)});
}
// 색 none/미등록은 이전 값 유지이므로 실제로 덮어쓰는 유효색만 판별한다.
bool AssignsColor(std::string_view value) {
    return std::any_of(kColors.begin()+1,kColors.end(),[&](auto color) { return Equal(value,color); }); // 원문 index1~8과 비교한다.
}
}

// 현재 owner를 즉시 검사하되 문서를 열었다는 이유로 legacy 필드 값을 정규화하지 않는다.
EditorScenarioModel::EditorScenarioModel(ScenarioDocument document,EditorScenarioContext context):document_(std::move(document)),context_(std::move(context)) { CheckOwner(context_.localOwner); }
// 외부 파일/환경/레지스트리를 자동 탐색하지 않는 독립 Config 문맥이다.
std::string EditorScenarioModel::Resolve(std::string_view key) const {
    if (context_.resolve) return context_.resolve(document_,key);
    o::ConfigRegistry registry;o::Config mission(registry,"mission");mission.Assign(document_.Text());return mission.Get("mission."+std::string(key)).value_or("");
}
// 동적 입력은 원문과 표시값을 함께 노출하여 현재 전역 문맥 부족을 감추지 않는다.
ScenarioFieldView EditorScenarioModel::Field(std::string_view key) const {
    const auto kind=Describe(key);ScenarioFieldView view;view.raw=document_.Raw(kind.canonical);view.effective=Resolve(kind.canonical);
    view.origin=view.raw ? ScenarioValueOrigin::Document : ScenarioValueOrigin::Default;view.dynamic=view.raw && Dynamic(*view.raw);return view;
}
// Editor 194162 강제값과 AI startingSp+4000을 로컬 시작 SP에 섞지 않는다.
EditorScenarioPlayerView EditorScenarioModel::Player() const {
    EditorScenarioPlayerView view;view.owner=context_.localOwner;view.money=Field("myStartMoney");view.technology=Field("myTech");view.startingMoney=static_cast<float>(o::ConfigParseLong(view.money.effective));return view;
}
// 독립 600BYTE에 실제 Apply를 실행하며 MP/참가자 동맹표/AI 프로세스는 생성하지 않는다.
EditorScenarioAiView EditorScenarioModel::Ai(std::uint32_t owner) const {
    CheckOwner(owner);EditorScenarioAiView view;view.owner=owner;
    o::MissionAiSettingsState state;state.startingSp=context_.startingSp;state.stuffRefreshRate=context_.stuffRefreshRate;
    o::MissionAiSettingsHooks hooks;
    hooks.get=[this](auto key) { const auto bytes=o::EncodeOriginalText(Resolve(key));return std::string(bytes.begin(),bytes.end()); }; // 실제 시작 연결과 같은 CP1252 경계다.
    hooks.parseFloat=[](auto value) { const std::string text(value);return std::strtod(text.c_str(),nullptr); }; // 현재 제품의 atof 경계와 같다.
    hooks.unknownAbilityLog=[&view](auto value) { view.diagnostics.push_back("기존 원문의 미등록 AI 능력: "+std::string(value)); }; // 기존 문서는 지우거나 성공으로 숨기지 않는다.
    o::RawMissionAiSettings settings(o::OriginalEdition::Patch1078,state,std::move(hooks));settings.Prepare(view.settings,static_cast<std::int32_t>(owner));
    for (const auto suffix:kAiSuffixes) { // 필드 출처는 원문 저장 형식을 유지하면서 별도로 표시한다.
        const auto key="ai"+std::to_string(owner)+std::string(suffix),commonKey="ai"+std::string(suffix);
        ScenarioFieldView field;field.raw=document_.Raw(key);field.commonRaw=document_.Raw(commonKey);
        const auto own=Resolve(key),common=Resolve(commonKey);const bool ownApplies=suffix=="Color" ? AssignsColor(own) : !own.empty();const bool commonApplies=suffix=="Color" ? AssignsColor(common) : !common.empty();
        field.origin=ownApplies ? ScenarioValueOrigin::OwnerAi : commonApplies ? ScenarioValueOrigin::CommonAi : ScenarioValueOrigin::Default;
        if ((suffix=="AllyList" || suffix=="Ability") && ownApplies && commonApplies) field.origin=ScenarioValueOrigin::CombinedAi;
        field.dynamic=(field.raw && Dynamic(*field.raw)) || (field.commonRaw && Dynamic(*field.commonRaw));view.fields.emplace(suffix,std::move(field));
    }
    view.fields.at("Name").effective=ReadText(view.settings,0);view.fields.at("Tech").effective=ReadText(view.settings,0x1e);
    constexpr std::array<std::pair<std::string_view,std::size_t>,3> rates{{{"BridgeDrawRate",0x220},{"StuffRefreshRate",0x224},{"TimeBetweenMoves",0x228}}}; // 실제 float 필드 세 개다.
    for (const auto& [suffix,offset]:rates) view.fields.at(std::string(suffix)).effective=std::to_string(std::bit_cast<float>(Read(view.settings,offset))); // 정수 비트를 float로 해석한다.
    constexpr std::array<std::pair<std::string_view,std::size_t>,7> integers{{{"StartMoney",0x22c},{"GeyserAttachments",0x230},{"MaxInHand",0x238},{"Collectors",0x234},{"Enemy",0x240},{"NoTemple",0x244},{"Off",0x254}}}; // 실제 atol 필드 일곱 개다.
    for (const auto& [suffix,offset]:integers) view.fields.at(std::string(suffix)).effective=std::to_string(std::bit_cast<std::int32_t>(Read(view.settings,offset))); // signed32 표시를 유지한다.
    view.allyMask=Read(view.settings,0x23c);view.abilityMask=Read(view.settings,0x248);view.fields.at("AllyList").effective=std::to_string(view.allyMask);view.fields.at("Ability").effective=std::to_string(view.abilityMask);
    const auto color=Read(view.settings,0x250);view.fields.at("Color").effective=std::string(kColors.at(color));return view;
}
// 변경한 필드만 검증한다. 미지 키/기존 불규칙 값은 열기/다른 키 수정으로 손상되지 않는다.
bool EditorScenarioModel::Set(std::string_view key,std::string_view value) {
    const auto kind=Describe(key);ScenarioDocument::ValidateUtf8(value);const auto name=kind.suffix.empty() ? kind.canonical : kind.suffix;
    if (!value.empty()) {
        if (name=="myTech" || name=="Tech" || name=="techAllowed") CheckTechnology(value,name=="techAllowed",context_);
        else if (name=="Color") { if (std::none_of(kColors.begin(),kColors.end(),[&](auto color) { return Equal(value,color); })) throw std::invalid_argument("등록되지 않은 새 AI 색입니다"); }
        else if (name=="Ability" || name=="AllyList") {
            if (std::count(value.begin(),value.end(),';')>=10) throw std::length_error("새 AI 목록은 원문 소비 상한인 10항목 이하여야 합니다");
            for (int index=0;index<=static_cast<int>(std::count(value.begin(),value.end(),';'));++index) { // 새 목록의 빈 항목도 조용히 무시하지 않는다.
                auto item=o::ConfigListItem(value,index);if (item.empty()) throw std::invalid_argument("새 AI 목록에 빈 항목이 있습니다");
                if (name=="AllyList") { CheckInteger(item);const auto owner=o::ConfigParseLong(item);if (owner<1 || owner>8) throw std::out_of_range("새 동맹 owner는 1~8이어야 합니다"); }
                else { if (item.front()=='!') item.erase(item.begin());if (std::none_of(kAbilities.begin(),kAbilities.end(),[&](auto ability) { return Equal(item,ability); })) throw std::invalid_argument("등록되지 않은 새 AI 능력입니다"); }
            }
        }
        else if (name=="Name") { if (o::EncodeOriginalText(value).size()>29) throw std::length_error("새 AI 이름은 원문 Tech 영역 앞의 29BYTE 이하여야 합니다"); }
        else if (name=="BridgeDrawRate" || name=="StuffRefreshRate" || name=="TimeBetweenMoves") CheckRate(value);
        else if (name=="missionType") { if (!Equal(value,"Tutorial") && !Equal(value,"Battle")) throw std::invalid_argument("새 미션 유형은 실제 Tutorial/Battle이어야 합니다"); }
        else if (name!="title") CheckInteger(value);
    }
    return document_.SetLiteral(kind.canonical,value);
}
// 빈 owner override도 명시 상태로 남기므로 중복 키 삭제나 공통값 펼치기를 하지 않는다.
bool EditorScenarioModel::InheritAi(std::uint32_t owner,std::string_view suffix) { CheckOwner(owner);return Set("ai"+std::to_string(owner)+std::string(suffix),""); }
// UI 선택 목록도 실제 이름 순서를 유지한다.
std::span<const std::string_view> EditorScenarioModel::AiColors() { return kColors; }
// 미등록 능력을 만들어 원문 index13 오류 경로를 새 입력으로 사용하지 않는다.
std::span<const std::string_view> EditorScenarioModel::AiAbilities() { return kAbilities; }
// 원문과 바이트 직렬화는 문서 소유자에게 맡긴다.
const ScenarioDocument& EditorScenarioModel::Document() const { return document_; }
}
