// 현재값 재조회와 일반 입력 고리 예약을 보존하며 미연결 palette/UI를 빈 성공으로 채우지 않는다.
#include "client/ExistingEditorAfterCheckpoint.h"
#include <stdexcept>

namespace netstorm::client {
namespace {
// 이미 실행한 앞선 효과는 남기고 도달한 실제 장치 누락에서만 중단한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("기존 Editor checkpoint 이후 실제 장치 누락");return function;
}
}
// 외곽 checkpoint 뒤의 장치와 다음 상태 예약만 처리하며 State 확정이나 loaded를 앞당기지 않는다.
void ApplyExistingEditorAfterCheckpoint(ClientLocalStateGlobals& globals,InputQueue& input,const ExistingEditorAfterCheckpointEffects& effects) {
    auto state=globals.State(); //상태의 내용을 복사하지 않고 Client가 소유한 같은 다섯 블록을 빌린다.
    Need(effects.hud)(globals.HudModeCommand());
    globals.ActivityEnd()=Need(effects.wallSeconds)()+10.0;
    Need(effects.requestMusic)("ser22.mus");
    if (state.current[0]==6) { //음악 복귀 뒤의 현재 요청이며 진입 시 요청을 캐시하지 않는다.
        Need(effects.battleOver)();throw std::logic_error("기존 Editor 한정 tail의 BattleOver 분기");
    }
    Need(effects.fortPalette)();
    const auto current=state.current[0]; //palette 전체 부모 복귀 뒤의 현재 요청을 다시 읽는다.
    state.next[0]=4; //원문은 기존 next를 항상 덮어쓰며 나머지140BYTE는 유지한다.
    if (current==3) return;
    if (Need(effects.islandCount)()!=0) return;
    input.PushCommand(globals.AddIslandCommand(),0,0); //원래40칸 고리에만 예약하며 창/섬을 동기로 만들지 않는다.
}
}
