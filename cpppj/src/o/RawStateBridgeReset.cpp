// 실제 PE가 먼저 계산한 기한과 뒤따르는 고정 폭 쓰기 순서를 보존한다.
#include "o/RawStateBridgeReset.h"
#include <stdexcept>

namespace netstorm::o { //큰 State 부모나 프레임 순회를 새로 실행하지 않는다.
void ResetStateOwnerColors(std::span<std::int32_t> colors) {
    if (colors.size()<9) throw std::invalid_argument("State 소유자 색 표의 실제9DWORD 부족");
    for (std::int32_t owner=0;owner<9;++owner) colors[static_cast<std::size_t>(owner)]=owner; //원문은 뒤의 열째 칸을 포함하지 않는다.
}
void ResetStateBridgeDecayScan(BridgeDecayScanState& scan,float period,double now) {
    const double next=static_cast<double>(period)+now; //원문 fld/fadd는 첫 DWORD store보다 앞이며 now를 float로 줄이지 않는다.
    scan.first=15000;scan.last=23001;scan.cursor=15000;scan.next=next;
}
}
