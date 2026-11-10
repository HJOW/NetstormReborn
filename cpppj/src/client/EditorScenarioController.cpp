// 원문 영어 키를 실제 저장 키로 쓰며 번역된 UI 문구를 설정 키로 사용하지 않는다.
#include "client/EditorScenarioController.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 파일을 직접 여는 UI 코드와 순수 검증 모델을 분리한다.
namespace { // 페이지/필드 ID와 표시용 값만 정의한다.
constexpr std::array<std::string_view,12> kGeneral{{"title","missionType","missionNumber","tutorialNumber","myStartMoney","myTech","techAllowed","moreGeysers","denySalvage","denyAscend","allowAnyCapture","randGeysers"}}; // 원문 일반 키의 고정 ID0..11이다.
constexpr std::array<std::string_view,15> kAi{{"Name","StartMoney","Tech","Off","Color","AllyList","Enemy","NoTemple","Ability","BridgeDrawRate","StuffRefreshRate","TimeBetweenMoves","GeyserAttachments","MaxInHand","Collectors"}}; // AI owner*32+index의 순서를 고정한다.
constexpr std::uint32_t kBriefing=0x1000U; // 첫 A. 본문의 필드 ID다.
// 현재 필드 ID를 원문 저장 키로 해석한다.
std::string Key(std::uint32_t field) { if (field<kGeneral.size()) return std::string(kGeneral[field]);if (field>=0x200U && field<0x200U+kAi.size()) return "ai"+std::string(kAi[field-0x200U]);const auto owner=field/32,index=field%32;if (owner>=1 && owner<=8 && index<kAi.size()) return "ai"+std::to_string(owner)+std::string(kAi[index]);if (field==kBriefing) return "[A.]";throw std::out_of_range("시나리오 UI 필드 ID 범위"); }
// 원문/실효 값을 구별하며 긴/표시 불가 문자열 때문에 패널 전체를 잃지 않는다.
std::string Preview(std::string_view text) {
    try { const auto bytes=o::EncodeDisplayText(text);std::size_t end{},count{};while (end<bytes.size() && count<24) { end+=o::DecodeExtendedSequence(bytes,end) ? 4U : 1U;++count; }return o::DecodeDisplayText(std::span<const std::uint8_t>(bytes).first(end))+(end<bytes.size() ? "..." : ""); } // 표시 인코딩 문자 경계에서만 자른다.
    catch (const std::invalid_argument&) { return "(표시 미지원 원문 보존)"; }
}
// 서식 제어가 있는 사용자 제목은 메뉴 마크업으로 재해석하지 않고 평문 값으로 안내한다.
std::string Safe(std::string value) { std::replace(value.begin(),value.end(),'~',' ');std::replace(value.begin(),value.end(),'\r',' ');std::replace(value.begin(),value.end(),'\n',' ');return value; }
}
EditorScenarioController::EditorScenarioController(std::shared_ptr<EditorScenarioFiles> files,std::string map,EditorScenarioContext context):files_(std::move(files)),opened_(files_ ? files_->Open(map) : throw std::invalid_argument("시나리오 UI 파일 owner 누락")),context_(std::move(context)),model_(opened_.document,context_) {} // 열기만으로 저장/AI생성을 하지 않는다.
// 각 페이지는40항목 메뉴 상한 안의 유한 목록이다.
std::vector<EditorScenarioRow> EditorScenarioController::Page(std::uint32_t page,const std::function<bool(std::uint32_t)>& hasPriest) const {
    std::vector<EditorScenarioRow> rows;
    if (!page) {
        rows.push_back({"Player start / 시작 SP·기술",kEditorScenarioPage,1});rows.push_back({"General / 제목·규칙",kEditorScenarioPage,2});rows.push_back({"Briefing / 브리핑",kEditorScenarioPage,3});
        rows.push_back({"File / 저장 대상: "+Safe(Preview(opened_.path)),0,0,false}); // 실제 현재 언어 override/English 선택을 읽기 전용으로 안내한다.
        rows.push_back({"AI common / 공통 상속값",kEditorScenarioPage,10});
        for (std::uint32_t owner=1;owner<=8;++owner) { const bool local=owner==context_.localOwner;rows.push_back({local ? "Player "+std::to_string(owner)+" / 로컬 시작값은 Player에서 편집" : "AI "+std::to_string(owner)+(hasPriest && hasPriest(owner) ? " / 사제 배치 있음" : " / 사제 배치 필요"),kEditorScenarioPage,10+owner,!local}); } // 일반 TestBattle은 로컬 owner의 AI를 만들지 않는다.
        rows.push_back({Dirty() ? "Save changes / 변경 저장" : "Save / 저장",kEditorScenarioSave});rows.push_back({"Discard / 취소",kEditorScenarioDiscard});return rows;
    }
    if (page==3) { rows.push_back({"Edit first [A.] briefing / 첫 브리핑 편집",kEditorScenarioEdit,kBriefing});rows.push_back({"다른 이벤트/승리 대본은 보존합니다",0,0,false}); }
    else if (page==1 || page==2) {
        for (std::uint32_t field=0;field<kGeneral.size();++field) { if ((page==1)!=(field==4 || field==5 || field==6)) continue;const auto view=model_.Field(kGeneral[field]);rows.push_back({Key(field)+" = "+Safe(Preview(view.effective))+(view.dynamic ? " [식]" : ""),kEditorScenarioEdit,field}); } // 실효값은 보기이고 수정 초기값은 raw를 쓴다.
    } else if (page==10) {
        for (std::uint32_t index=0;index<kAi.size();++index) { const auto id=0x200U+index;const auto field=model_.Field(Key(id));rows.push_back({Key(id)+" = "+Safe(Preview(field.effective)),kEditorScenarioEdit,id}); } // 공통 AllyList/Ability를 명시 편집해야 가산된 공통 관계를 제거할 수 있다.
        rows.push_back({"공통값은 모든 AI에 먼저 적용됩니다",0,0,false});
    } else if (page>=11 && page<=18) {
        const auto owner=page-10;const auto view=model_.Ai(owner);
        for (std::uint32_t index=0;index<kAi.size();++index) { const auto& field=view.fields.at(std::string(kAi[index]));const auto id=owner*32+index;rows.push_back({std::string(kAi[index])+" = "+Safe(Preview(field.effective))+(!field.raw || field.raw->empty() ? " [상속]" : ""),kEditorScenarioEdit,id}); } // 원문 목록/가산 결과를 자동 대칭화하지 않는다.
        rows.push_back({"AllyList: AI가 볼 동맹 owner; 1은 로컬",0,0,false});rows.push_back({"능력/적/테크는 기존 영문 키 목록입니다",0,0,false});
    } else throw std::out_of_range("시나리오 페이지 범위");
    rows.push_back({"Back / 이전",kEditorScenarioPage,0});return rows;
}
// 원시 값이 없으면 빈 입력으로 시작한다. 동적 식을 편집하면 명시 literal로 바뀜을 제목에 알린다.
EditorScenarioEditRequest EditorScenarioController::Edit(std::uint32_t field) const {
    EditorScenarioEditRequest result;result.key=Key(field);result.title=result.key+" / 빈 AI 값은 상속";
    if (field==kBriefing) { result.initial=model_.Document().Section("A.").value_or("");result.multiline=true;result.maximum=1000;result.title="Briefing [A.] / HTML·$Button 대본"; } // 첫 UI는999표시BYTE까지 편집하여 원문1024BYTE Draw 한 줄을 넘지 않는다.
    else { result.initial=model_.Document().Raw(result.key).value_or("");if (result.key.ends_with("Name")) result.maximum=30;else if (result.key=="title") result.maximum=240; }
    if (o::EncodeDisplayText(result.initial).size()>=static_cast<std::size_t>(result.maximum)) throw std::length_error("기존 시나리오 값이 이 입력창의 BYTE 상한을 넘습니다. 원문은 보존됩니다"); // 기존 긴 대본을 자동 절단하지 않는다.
    return result;
}
// 입력은 UTF-8로 해독된 사용자 문자다. 표시토큰/Config escape는 저장 경계가 각각 처리한다.
void EditorScenarioController::Apply(std::uint32_t field,std::string_view value) {
    const auto request=Edit(field);if (value==request.initial) return;const auto bytes=o::EncodeDisplayText(value);if (bytes.size()>=static_cast<std::size_t>(request.maximum)) throw std::length_error("시나리오 입력 BYTE 상한 초과"); // 기존 동적 식/escape를 그대로 OK한 경우 새 literal로 바꾸지 않는다.
    std::size_t column{};for (const auto byte:bytes) { if (byte=='\n') column=0;else if (++column>1000) throw std::length_error("브리핑 한 줄은1000BYTE 이하여야 합니다"); } // 실제 TextEntry Draw의1024BYTE 한 줄 저장소를 넘기지 않는다.
    if (field==kBriefing) model_.SetBriefing(value);else model_.Set(request.key,value);
}
void EditorScenarioController::Inherit(std::uint32_t field) { const auto key=Key(field);if (!key.starts_with("ai")) throw std::invalid_argument("AI override만 상속할 수 있습니다");model_.Set(key,""); } // 가산 공통값을 지우는 의미가 아니다.
void EditorScenarioController::Save(bool explicitSave) { files_->Save(opened_,model_.Document(),explicitSave);model_=EditorScenarioModel(opened_.document,context_); } // 파일 정상 완료 뒤에만 보류 문서 기준을 재설정한다.
bool EditorScenarioController::Dirty() const { return !opened_.baseline || model_.Document().Changed(); } // 신규 문서도 저장할 변경으로 표시한다.
const std::string& EditorScenarioController::Map() const { return opened_.map; } // 실제 Fort identity 비교용이다.
const std::string& EditorScenarioController::Path() const { return opened_.path; } // 쓰기 대상 안내용이다.
}
