// 원본 편집 테스트 명령의 저장/Guide 교체와 편집 복귀 예약 순서를 제공한다.
#pragma once
#include "client/GuideMissionTransitions.h"

namespace netstorm::client { // 원본 일반 입력의 c2 분기만 담당하며 실제 월드 설치는 State에 남긴다.
inline constexpr std::uint32_t kEditorTestBattleCommand=0x020000c2U; //005429fc에 저장된 원본 명령이다.
// 현재 이름은 저장/해제가 끝난 위치에서 빌리고, 각 큰 하위의 정상 반환 뒤에만 다음 효과를 실행한다.
struct EditorTestBattleHooks {
    std::function<void()> snapshot; //004bfb50의 현재 Template 저장 부모다.
    std::function<std::string_view()> currentFortName; //0055b490의 현재 이름 주소를 NUL 포함 물리 범위로 빌린다. Guide 해제 중 내용이 바뀔 수 있다.
    GuideMissionTransitionHooks guide; //실제 Begin의 finish/release/create와 Close의 조건부 release, 새 현재의 +64 쓰기다.
    std::function<void(std::string_view)> pending; //00460bb0의 pending 이름 복사다. current와 별개 저장소를 사용한다.
    std::function<void()> requestEdit; //004b3560 전체 부모다. next1 쓰기와 선택된 Battle14 보존 하위를 포함한다.
};
// 일반 입력의 뗌을 효과 없이 소비하며 y==0만 테스트 시작, 그 외 DWORD는 편집 복귀다.
bool DispatchEditorTestBattle(const InputEvent& event,std::uint32_t& selectionMode,const EditorTestBattleHooks& hooks);
}
