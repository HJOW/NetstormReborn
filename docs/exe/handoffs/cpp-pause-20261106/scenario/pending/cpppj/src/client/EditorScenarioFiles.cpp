// 파일/설정의 실제 owner를 공유하고 원문 부재/사용자 삭제/손상을 서로 바꾸지 않는다.
#include "client/EditorScenarioFiles.h"
#include "client/EditorMapCatalogFiles.h"
#include "platform/Utf8Path.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 시나리오 편집 파일은 게임 전역 상태와 분리한다.
namespace { // 실제 Config 스냅샷과 경로 identity만 처리한다.
// 현재 Config의 등록 순서/이름/본문을 독립 Registry에 복제하는 짧은 계산 수명이다.
struct Snapshot {
    o::ConfigRegistry registry; // 실제 Registry를 변경하지 않는 독립 등록표다.
    std::vector<std::unique_ptr<o::Config>> objects; // 각 설정 본문/Prefix를 현재 순서대로 소유한다.
    o::Config* configuration{}; // 원래 ConfigInterface::Configuration에 대응하는 사본이다.
    // 외부 mission 파일을 새로 읽지 않으며 현재 메모리의 이름/본문/로컬 환경 정책만 복제한다.
    explicit Snapshot(o::ConfigInterface& source) {
        registry.environment=source.Registry().environment;registry.registryValue=source.Registry().registryValue;
        for (const auto* original:source.Registry().Objects()) { // 같은 순서여야 bare/이름 붙은 조회의 우선순위가 보존된다.
            auto name=original->Prefix();if (!name.empty()) name.pop_back();auto copy=std::make_unique<o::Config>(registry,name);
            if (original->IsValid()) copy->Assign(original->Text());copy->environmentEnabled=original->environmentEnabled;
            if (original==&source.Configuration()) configuration=copy.get();objects.push_back(std::move(copy));
        }
        if (!configuration) throw std::logic_error("시나리오 경로의 실제 Configuration 사본 누락");
    }
};
// 같은 namespace의 case만 다른 경로는 한 파일로 판정한다.
bool Same(std::string_view first,std::string_view second) { auto a=o::AsciiLower(first),b=o::AsciiLower(second);std::replace(a.begin(),a.end(),'\\','/');std::replace(b.begin(),b.end(),'\\','/');return a==b; }
// 새 기본 문서/파일 이름은 원문의 ASCII 정책을 유지한다.
void CheckMap(std::string_view name) {
    if (name.empty()) throw std::invalid_argument("시나리오의 현재 맵 이름이 비었습니다");
    for (const auto byte:name) if (!((byte>='A' && byte<='Z') || (byte>='a' && byte<='z') || (byte>='0' && byte<='9') || byte=='_' || byte==' ')) throw std::invalid_argument("시나리오 파일명은 기존 ASCII 영숫자/밑줄/공백 정책을 사용합니다"); // 표시 제목은 별도 UTF-8 필드다.
}
}
// local.1과 언어만 독립 사본에 넣어 간접 missionSpec/사용자 override도 현재 문법으로 계산한다.
EditorScenarioPaths ResolveEditorScenarioPaths(o::ConfigInterface& config,std::string_view map) {
    CheckMap(map);Snapshot snapshot(config);o::Config local(snapshot.registry,"local");auto literal=ScenarioDocument::FromUtf8("");literal.SetLiteral("1",map);local.Assign(literal.Text());
    EditorScenarioPaths result;result.requested=snapshot.configuration->Get("missionSpec").value_or("");snapshot.configuration->Set("currentLanguage","english");result.english=snapshot.configuration->Get("missionSpec").value_or("");
    if (result.requested.empty() || result.english.empty()) throw std::invalid_argument("시나리오 missionSpec 경로가 비었습니다");return result;
}
// 마지막 mission 사본이 staged 문서여야 변경한 값의 실제 상속/치환을 미리 볼 수 있다.
std::string ResolveEditorScenarioField(o::ConfigInterface& config,const ScenarioDocument& document,std::string_view key) { Snapshot snapshot(config);o::Config mission(snapshot.registry,"mission");mission.Assign(document.Text());return mission.Get("mission."+std::string(key)).value_or(""); }
EditorScenarioFiles::EditorScenarioFiles(EditorScenarioFileHooks hooks):hooks_(std::move(hooks)) { if (!hooks_.read || !hooks_.deleted || !hooks_.write || !hooks_.paths) throw std::invalid_argument("시나리오 파일 장치 누락"); } // 선택 전에는 I/O를 수행하지 않는다.
// 기존 장부와 원본 보호 검사를 우회하는 별도 reader/writer를 만들지 않는다.
EditorScenarioFiles::EditorScenarioFiles(std::shared_ptr<EditorCatalogWorkspace> workspace,o::ConfigInterface& config):EditorScenarioFiles(EditorScenarioFileHooks{
    [workspace](auto path) { if (!workspace) throw std::logic_error("시나리오 workspace 누락");auto bytes=workspace->Read(path);if (!bytes && std::filesystem::exists(workspace->Resolve(path))) throw std::runtime_error("기존 시나리오 파일 읽기 실패");return bytes; },
    [workspace](auto path) { return workspace->Deleted(path); },
    [workspace](auto path,auto bytes) { workspace->Files().WriteAtomic(platform::PathToUtf8(workspace->Resolve(path)),bytes); },
    [&config](auto map) { return ResolveEditorScenarioPaths(config,map); }}) {}
// 기본 생성은 이름/경로 계산 뒤 기존 요청 언어를 덮지 않고 English 한 개만 작성한다.
bool EditorScenarioFiles::EnsureEnglish(std::string_view map) const {
    CheckMap(map);const auto paths=hooks_.paths(map);if (hooks_.read(paths.english)) return true;if (hooks_.deleted(paths.english)) return false;
    const auto bytes=CreateDefaultEditorScenario(map).Serialize();hooks_.write(paths.english,bytes);return true;
}
// 원문 손상은 FromBytes가 거부하며 빈/손상 파일을 신규 부재로 바꾸지 않는다.
EditorScenarioOpened EditorScenarioFiles::Open(std::string_view map) const {
    CheckMap(map);const auto paths=hooks_.paths(map);if (auto bytes=hooks_.read(paths.requested)) return {std::string(map),paths.requested,ScenarioDocument::FromBytes(*bytes),std::move(bytes),false};
    if (!Same(paths.requested,paths.english)) if (auto bytes=hooks_.read(paths.english)) return {std::string(map),paths.english,ScenarioDocument::FromBytes(*bytes),std::move(bytes),false};
    const bool deleted=hooks_.deleted(paths.english);return {std::string(map),paths.english,CreateDefaultEditorScenario(map),std::nullopt,deleted};
}
// atomic 실패/동시 수정은 opened의 baseline/dirty를 바꾸지 않는다.
void EditorScenarioFiles::Save(EditorScenarioOpened& opened,const ScenarioDocument& document,bool explicitSave) const {
    if (opened.deleted && !explicitSave) throw std::runtime_error("삭제한 시나리오는 Scenario의 Save로 명시 재작성해야 합니다");
    const auto current=hooks_.read(opened.path);if (current!=opened.baseline) throw std::runtime_error("시나리오 파일이 편집 중 다른 곳에서 변경되거나 삭제되었습니다");
    const auto bytes=document.Serialize();if (current && *current==bytes) return;hooks_.write(opened.path,bytes);opened.baseline=bytes;opened.document=ScenarioDocument::FromBytes(bytes);opened.deleted=false;
}
// 서로 다른 파일 저장을 한 트랜잭션이라고 가장하지 않는다. 대상 충돌은 쓰기 전에 모두 확인한다.
void EditorScenarioFiles::CopyAs(std::string_view previous,std::string_view next) const {
    CheckMap(previous);CheckMap(next);if (Same(previous,next)) return;const auto old=hooks_.paths(previous),target=hooks_.paths(next);
    std::vector<std::pair<std::string,std::vector<std::uint8_t>>> copies;
    for (const auto& paths:std::array<std::pair<std::string,std::string>,2>{{{old.english,target.english},{old.requested,target.requested}}}) { // English와 현재 언어만 복사하며 임의 대본 전체 치환은 하지 않는다.
        if (std::any_of(copies.begin(),copies.end(),[&](const auto& item) { return Same(item.first,paths.second); })) continue;
        if (const auto bytes=hooks_.read(paths.first)) { const auto existing=hooks_.read(paths.second);if (existing && *existing!=*bytes) throw std::runtime_error("Save as 대상 시나리오가 이미 있습니다");if (!existing) copies.emplace_back(paths.second,*bytes); }
    }
    for (const auto& [path,bytes]:copies) hooks_.write(path,bytes); // 첫 성공/뒤 실패의 부분 효과는 재시도할 때 같은 바이트인지 확인한다.
}
// 실제 미션은 현재 언어 override를 먼저 읽고 해당 요청이 부재일 때만 같은 workspace의 English를 사용한다.
std::optional<std::vector<std::uint8_t>> EditorScenarioFiles::ReadForMission(std::string_view map,std::string_view requested) const { const auto paths=hooks_.paths(map);if (auto bytes=hooks_.read(requested)) return bytes;if (Same(requested,paths.requested) && !Same(paths.requested,paths.english)) return hooks_.read(paths.english);return std::nullopt; }
}
