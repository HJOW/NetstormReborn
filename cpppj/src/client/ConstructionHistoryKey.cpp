// 실제 Point 유효성·첫 칸 복귀·카메라 뒤 현재 순번 갱신을 별도 북마크 기능으로 바꾸지 않는다.
#include "client/ConstructionHistoryKey.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 같은 값형 장부는 Session 교체 때 재생성하지 않는다.
void InitializeConstructionHistory(o::ConstructionHistoryState& history) {
    for (auto& point:history.points) point={-10000.0F,-10000.0F}; //원문 최초5회 복사이며 Tab 순번에는 쓰지 않는다.
}
bool IsConstructionHistoryPointValid(o::ConstructionNoticePoint point,std::int32_t worldCells) {
    const auto maximum=static_cast<float>(worldCells); //원문FILD32→단정도 저장의 같은 signed 상한이다.
    return 0.0F<point.x && point.x<maximum && 0.0F<point.y && point.y<maximum;
}
bool DispatchConstructionHistoryKey(InputEvent event,o::ConstructionHistoryState& history,const std::int32_t& worldCells,const ConstructionHistoryKeyHooks& hooks) {
    if ((event.code&InputCode::kRelease) || (event.code&0x03ffffffU)!=9) return false;
    if (!hooks.paused) throw std::logic_error("최근 건설 위치 키의 실제 정지 판정 미연결");if (hooks.paused()) return true;
    if (history.index>=history.points.size()) throw std::out_of_range("최근 건설 위치 순번의 물리5칸 범위");
    auto point=history.points[history.index]; //실제 두DWORD를 현재 순번에서 먼저 복사한다.
    if (!IsConstructionHistoryPointValid(point,worldCells)) { history.index=0;point=history.points[0]; }
    if (IsConstructionHistoryPointValid(point,worldCells)) {
        if (!hooks.center) throw std::logic_error("최근 건설 위치의 실제 즉시 카메라 미연결");hooks.center(point.x,point.y);
        history.index=std::bit_cast<std::uint32_t>(std::bit_cast<std::int32_t>(history.index+1U)%5); //카메라가 바꾼 현재 순번을 다시 읽고 INC의32비트 감김/signed IDIV 나머지를 보존한다.
    }
    return true;
}
}
