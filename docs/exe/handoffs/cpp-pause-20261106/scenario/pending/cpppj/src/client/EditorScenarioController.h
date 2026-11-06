// 실제 Editor 전용 패널의 페이지/편집/저장/취소를 파일 소유자에 연결한다.
#pragma once
#include "client/EditorScenarioFiles.h"

namespace netstorm::client { // 원문 명령과 겹치지 않는 명시 클론 확장 UI다.
inline constexpr std::uint32_t kEditorScenarioOpen=0x0300e100U; // Edit 메뉴의 Scenario 진입 명령이다.
inline constexpr std::uint32_t kEditorScenarioPage=0x0300e101U; // 인자 페이지로 별도 시나리오 패널을 연다.
inline constexpr std::uint32_t kEditorScenarioEdit=0x0300e102U; // 인자 필드의 실제 TextEntry 모달을 연다.
inline constexpr std::uint32_t kEditorScenarioSave=0x0300e103U; // 사용자 work에 명시 저장하고 패널을 닫는다.
inline constexpr std::uint32_t kEditorScenarioDiscard=0x0300e104U; // 저장하지 않은 변경을 버리고 닫는다.
inline constexpr std::uint32_t kEditorScenarioInherit=0x0300e105U; // AI override를 명시 빈 값으로 바꾼다.
// 메뉴 한 행의 표시 UTF-8와 원시 일반 명령이다.
struct EditorScenarioRow { std::string label;std::uint32_t command{},parameter{};bool enabled{true}; }; // label/code/인자는 순서대로 같은 Gump에 전달한다.
// 고정 필드 ID는 페이지가 바뀌어도 다른 키를 가리키지 않는다.
struct EditorScenarioEditRequest { std::string key,title,initial;std::int32_t maximum{512};bool multiline{}; }; // initial은 UTF-8이며 maximum은 NUL 포함 BYTE 상한이다.
class EditorScenarioController {
public:
    EditorScenarioController(std::shared_ptr<EditorScenarioFiles> files,std::string map,EditorScenarioContext context); // 현재 맵 문서만 열고 보류 변경을 소유한다.
    std::vector<EditorScenarioRow> Page(std::uint32_t page,const std::function<bool(std::uint32_t)>& hasPriest={}) const; // 0전체/1로컬/2일반/3브리핑/11~18AI이며 실제 사제 배치를 읽기만 한다.
    EditorScenarioEditRequest Edit(std::uint32_t field) const; // 고정 ID의 원문 raw값을 편집용으로 반환한다. 실효값으로 원문을 덮지 않는다.
    void Apply(std::uint32_t field,std::string_view value); // 수정한 필드만 모델 검증 후 반영한다.
    void Inherit(std::uint32_t field); // AI 공통값과 가산 관계를 그대로 유지한다.
    void Save(bool explicitSave); // atomic 정상 반환 뒤에만 dirty 기준을 새로 잡는다.
    bool Dirty() const; // 신규/수정/삭제 재작성 상태를 읽는다.
    const std::string& Map() const; // 맵 전환 뒤의 지연 callback을 거부하는 identity다.
    const std::string& Path() const; // 실제 선택 언어/English 저장 대상을 화면에 표시한다.
private:
    std::shared_ptr<EditorScenarioFiles> files_; // 현재 workspace와 동일한 파일 장치 수명이다.
    EditorScenarioOpened opened_; // 기준 바이트/대상 경로/삭제 재작성 상태다.
    EditorScenarioContext context_; // staged 문서를 실제 Config 스냅샷으로 해석한다.
    EditorScenarioModel model_; // 저장 전의 보류 문서/미리보기다.
};
}
