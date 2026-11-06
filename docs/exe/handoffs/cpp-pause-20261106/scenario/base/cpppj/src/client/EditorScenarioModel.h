// 원문 설정 소비자의 상속값과 사용자 편집값을 구별하는 시나리오 문서 모델이다.
#pragma once
#include "client/ScenarioDocument.h"
#include <array>
#include <functional>
#include <map>

namespace netstorm::client { // 실제 게임 상태를 변경하지 않는 편집 미리보기다.
// 미리보기 값이 기본값/공통값/owner 값 중 어디에서 왔는지 표시한다.
enum class ScenarioValueOrigin { Default, Document, CommonAi, OwnerAi, CombinedAi };
// 원시 미지정·명시 빈 문자열·명시 0과 실제 상속 결과를 함께 보관한다.
struct ScenarioFieldView {
    std::optional<std::string> raw; // 이 키의 첫 정의이며 없는 키와 빈 값을 구별한다.
    std::optional<std::string> commonRaw; // AI 뷰에서 ai<suffix>의 첫 정의다.
    std::string effective; // 실제 Config 치환 및 소비자 계산을 거친 표시값이다.
    ScenarioValueOrigin origin{ScenarioValueOrigin::Default}; // 실제 값을 제공한 층이다.
    bool dynamic{}; // 원문에 치환식이 있어 외부 문맥 없이 확정값이라고 주장하지 않는다.
};
// 로컬 owner는 미션 문서가 아닌 현재 세션 입력이며 편집용 고정 SP와 구별한다.
struct EditorScenarioPlayerView {
    std::uint32_t owner{}; // 호출자가 지정한 현재 로컬 owner 1~8이다.
    ScenarioFieldView money; // myStartMoney의 원시/치환값이다.
    ScenarioFieldView technology; // myTech의 순서를 유지한 실제 목록값이다.
    float startingMoney{}; // RawMissionStart와 같은 ParseLong→float이며 editor 194162를 적용하지 않는다.
};
// 기존 AI 설정 몸체의 결과만 보여 준다. AI 생성/행동/동맹표 쓰기는 하지 않는다.
struct EditorScenarioAiView {
    std::uint32_t owner{}; // ai<owner> 키를 적용한 1~8 owner다.
    std::map<std::string,ScenarioFieldView> fields; // suffix별 값과 원문/상속 출처다.
    std::array<std::uint8_t,600> settings{}; // 독립 설정 계산에만 쓰는 실제 600BYTE 사본이다.
    std::uint32_t allyMask{}; // 공통/owner의 앞10항목이 가산된 설정 DWORD다.
    std::uint32_t abilityMask{}; // 실제13개 능력과 원문 미등록 index13 처리 결과다.
    std::vector<std::string> diagnostics; // 기존 미등록 능력 등을 숨기지 않고 표시한다.
};
// 모델 바깥 현재 Config 문맥과 실제 타입 이름 검색은 호출자가 명시 공급한다.
struct EditorScenarioContext {
    std::uint32_t localOwner{1}; // 기본은 사용자 커스텀 맵의 로컬 owner1이다.
    std::int32_t startingSp{2500}; // 기존 AI 기본 StartMoney가 더하는 현재 전역이다.
    std::int32_t stuffRefreshRate{3}; // 기존 AI 기본 float에 변환할 현재 전역 정수다.
    // 있으면 staged 문서와 현재 전역을 함께 고려해 해당 mission 키의 치환값을 반환한다.
    // 없으면 독립 mission ConfigRegistry만 사용하며 외부 참조를 I/O로 해결하지 않는다.
    std::function<std::string(const ScenarioDocument&,std::string_view)> resolve;
    std::function<bool(std::string_view)> knownTechnology; // 변경한 기술 이름만 실제 RiftTypeTable로 검사한다.
};
// 지원 필드만 명시 변경하고 원문 나머지를 ScenarioDocument에 보존한다.
class EditorScenarioModel {
public:
    // 문서를 값으로 소유한다. 문맥 콜백은 모델보다 오래 살아 있는 대상을 빌려야 한다.
    explicit EditorScenarioModel(ScenarioDocument document,EditorScenarioContext context={});
    // 일반 지원키의 첫 원시값과 현재 치환값을 읽는다. 원문은 변경하지 않는다.
    ScenarioFieldView Field(std::string_view key) const;
    // 시작 SP/기술의 실제 소비자 뷰이며 Editor 현재 SP를 저장값으로 오인하지 않는다.
    EditorScenarioPlayerView Player() const;
    // 기본값→공통 ai*→owner aiN*를 기존 RawMissionAiSettings에 적용한다.
    EditorScenarioAiView Ai(std::uint32_t owner) const;
    // 지원키·입력 범위·실제 이름을 검증한 뒤 해당 키만 바꾼다. 기존 미지원 입력은 보존된다.
    bool Set(std::string_view key,std::string_view value);
    // owner의 명시값을 빈 문자열로 만든다. 원문 소비자처럼 공통값/기본값이 계속 적용된다.
    // AllyList/Ability는 가산이므로 이 함수로 공통 관계나 능력을 제거하지 않는다.
    bool InheritAi(std::uint32_t owner,std::string_view suffix);
    // UI 목록/검증에 사용하는 실제 AI 색/능력 이름표다.
    static std::span<const std::string_view> AiColors();
    static std::span<const std::string_view> AiAbilities();
    // 파일 쓰기 없이 현재 문서를 확인/직렬화한다. 저장 성공 후 재생성은 파일 owner가 담당한다.
    const ScenarioDocument& Document() const;
private:
    // 독립 문맥 또는 호출자의 staged 문맥에서 해당 키를 매번 다시 해석한다.
    std::string Resolve(std::string_view key) const;
    ScenarioDocument document_; // 원시 주석/본문/미지 키/반복 대본의 소유자다.
    EditorScenarioContext context_; // 실제 owner/전역/타입·치환 경계의 입력이다.
};
}
