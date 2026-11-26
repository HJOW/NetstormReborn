// 초기 미션의 현재 다음 단계 조건을 읽고 기존 공통 글자 노드로 표시한다.
#pragma once
#include "client/BitmapFont.h"
#include "o/RawMissionFrameGate.h"

namespace netstorm::client { // 게임 상태를 바꾸지 않는 튜토리얼 안내와 화면 배치를 제공한다.
class RawMenuControls; // 기존 등록표·글꼴·수명·부분 갱신을 빌리는 공통 UI 처리기다.
// 공식 초기 미션의 현재 단계와 영어 번역 키를 연결하는 읽기 전용 항목이다.
struct TutorialObjectiveEntry {
    std::uint32_t tutorial{}; // 공식 초기 미션 번호1~6이다.
    char stage{}; // 기존 gate의 A부터 시작하는 진행 단계다.
    std::string_view english; // 기존 Translate가 사용할 영어 안내문 겸 번역 키다.
};
// 완료 단계를 제외한 전체40단계 목록을 빌린다. 검사와 실제 안내가 같은 목록을 사용한다.
std::span<const TutorialObjectiveEntry> TutorialObjectives();
// 공식 미션 이름·번호와 실제 현재 단계/예약을 읽는다. 데모·편집·완료·미시작은 빈 안내다.
std::string_view TutorialObjectiveKey(std::string_view mission,const o::MissionFrameGateState& gate,std::uint32_t editing);
// 표시 바이트의 확장 문자 경계를 보존하며 실제 글꼴 폭으로 줄바꿈한다.
std::vector<std::string> WrapTutorialObjective(std::string_view bytes,int width,const std::function<int(std::string_view)>& measure);
// 배경이나 입력 영역 없이 기존 TextGump 노드만 소유 식별한다. 실제 메모리는 공통 등록표가 관리한다.
class TutorialObjectiveGump {
public:
    // 현재 안내/지도 영역이 바뀔 때만 기존 글자를 지우고 새 줄을 부착한다. menus는 호출 동안 살아 있어야 한다.
    void Sync(RawMenuControls& menus,std::uint32_t root,const BitmapFont& font,std::string bytes,ScreenRect viewport,std::uint8_t color,std::uint8_t shadow);
    // 실제 serial이 일치하는 자기 글자만 Kill하고 표시 식별자를 비운다. 다른 노드의 재사용 번호는 건드리지 않는다.
    void Clear(RawMenuControls& menus);
    // 실제 화면에 제출한 현재 표시 원문을 빌린다. 미션/게임 상태와 별도의 UI 관측용이다.
    std::string_view Text() const;
private:
    std::vector<std::array<std::uint32_t,2>> nodes_; // 각 글자 줄의 공통 토큰과 생성 serial이다.
    std::string text_; // 현재 제출한 표시 바이트다. 같은 안내의 매 프레임 재생성을 막는다.
    ScreenRect viewport_{}; // 마지막 지도 영역이다. 해상도와 HUD 도킹 변경을 감지한다.
    std::uint32_t root_{}; // 실제 현재 루트 토큰이다. UI 루트 교체 뒤 새 부모에 부착한다.
    const BitmapFont* font_{}; // 마지막 실제 글꼴 주소다. 캐시 재생성 뒤 새 폭을 계산한다.
    std::uint8_t color_{},shadow_{}; // 마지막 팔레트의 본문·그림자 색 번호다.
};
}
