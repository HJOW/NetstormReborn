// No는 저장을 요구하지 않고 Yes의 부분 실패는 앞선 효과와 이전 next DWORD를 그대로 남긴다.
#include "client/EditorSaveGoMain.h"
#include <stdexcept>

namespace netstorm::client { // 원본 일반 명령의 두 작은 부모를 현재 조회/큰 하위와 분리한다.
namespace { // 선택한 실제 경계만 확인하는 공통 계약이다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("SaveGoMain의 선택된 실제 저장 경계 미연결");return function; } // 빈 callback을 저장 성공으로 바꾸지 않는다.
}
// 최초 owner는 EAX처럼 보관하고 Battle을 실행했을 때만 같은 위치에서 owner를 다시 읽는다.
void SnapshotCurrentMap(const CurrentMapSnapshotReads& reads,const CurrentMapSnapshotHooks& hooks) {
    auto owner=Need(reads.owner)();if (!Need(reads.manager)(owner)) return;
    if (Need(reads.battle)()) { Need(hooks.battle)(owner);owner=Need(reads.owner)(); }
    if (Need(reads.fort)()) Need(hooks.fort)(owner);
}
// HTML의 양 인자 복제에 의존하지 않고 원문의 사건 두 번째 정수 인자만 읽는다.
bool DispatchEditorSaveGoMain(const InputEvent& event,std::uint32_t& nextState,const std::function<void()>& saveCurrent) {
    if ((event.code&0x03ffffffU)!=kSaveGoMainCommand) return false;
    if (event.code&InputCode::kRelease) return true;
    if (event.y) Need(saveCurrent)();nextState=3;return true;
}
}
