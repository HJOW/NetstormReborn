// 현재 Editor 작업 namespace에서 시나리오 선택/기본 생성/충돌 검사/저장을 소유한다.
#pragma once
#include "client/EditorScenarioModel.h"
#include "o/ConfigInterface.h"
#include <functional>
#include <memory>

namespace netstorm::client { // 실제 파일 보호와 순수 문서 편집을 연결하는 클론 확장이다.
class EditorCatalogWorkspace; // 기존 이관 장부/TAFF/삭제 표식 owner를 공유한다.
// 동일 미션 이름의 현재 언어/English 경로다. 사용자 missionSpec를 그대로 해석한다.
struct EditorScenarioPaths {
    std::string requested; // 현재 선택 언어의 실제 missionSpec 결과다.
    std::string english; // 독립 Config에서 currentLanguage만 english로 해석한 결과다.
};
// 독립 장치 검사와 실제 workspace가 같은 생성/저장 정책을 사용한다.
struct EditorScenarioFileHooks {
    std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)> read; // 부재와 읽기 실패를 구별해야 한다.
    std::function<bool(std::string_view)> deleted; // 완료 이관 후 삭제된 경로는 자동 부활시키지 않는다.
    std::function<void(std::string_view,std::span<const std::uint8_t>)> write; // 사용자 work의 atomic 쓰기만 허용한다.
    std::function<EditorScenarioPaths(std::string_view)> paths; // 현재 Config/이름을 매번 해석한다.
};
// 편집 중인 파일 identity와 최초 읽기를 소유한다. 실제 현재 mission Config를 수정하지 않는다.
struct EditorScenarioOpened {
    std::string map; // Fort의 기존 ASCII 이름이며 UTF-8 파일명 확장을 하지 않는다.
    std::string path; // 현재 언어 override가 있으면 그 파일, 없으면 English다.
    ScenarioDocument document; // 보존형 원문과 사용자 수정의 소유자다.
    std::optional<std::vector<std::uint8_t>> baseline; // 동시 수정/삭제를 감지할 최초 바이트다.
    bool deleted{}; // 명시 삭제된 English를 사용자가 다시 작성하려는 임시 문서다.
};
class EditorScenarioFiles {
public:
    explicit EditorScenarioFiles(EditorScenarioFileHooks hooks); // 생성만으로 파일을 읽거나 쓰지 않는다.
    explicit EditorScenarioFiles(std::shared_ptr<EditorCatalogWorkspace> workspace,o::ConfigInterface& config); // 같은 실제 workspace/장기 Config를 빌린다.
    bool EnsureEnglish(std::string_view map) const; // 실제 부재만 기본 생성한다. tombstone이면 false로 자동 생성을 생략한다.
    EditorScenarioOpened Open(std::string_view map) const; // 요청 언어→English 선택이며 손상/접근 실패를 기본값으로 덮지 않는다.
    void Save(EditorScenarioOpened& opened,const ScenarioDocument& document,bool explicitSave) const; // 충돌 검사 뒤 atomic 저장한다. 삭제 재작성은 명시 Save에서만 허용한다.
    void CopyAs(std::string_view previous,std::string_view next) const; // Save as가 현재/English sidecar를 사용자 work에 복사한다. 기존 대상은 덮지 않는다.
    std::optional<std::vector<std::uint8_t>> ReadForMission(std::string_view map,std::string_view requested) const; // Fort/추가파일은 원래 read, 해당 script 요청만 English fallback이다.
private:
    EditorScenarioFileHooks hooks_; // 모든 실제 I/O와 경로 계산 장치는 이 owner보다 오래 살아야 한다.
};
// 현재 Config를 별도 Registry에 복제하여 전역 변경 없이 경로와 staged mission 실효값을 계산한다.
EditorScenarioPaths ResolveEditorScenarioPaths(o::ConfigInterface& config,std::string_view map);
std::string ResolveEditorScenarioField(o::ConfigInterface& config,const ScenarioDocument& document,std::string_view key);
}
