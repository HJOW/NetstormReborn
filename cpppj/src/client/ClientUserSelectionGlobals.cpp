// 실제 다중 선택 목록의 고정 용량/중복/안정된 제거를 원문 헤더 규칙으로 처리한다.
#include "client/ClientUserSelectionGlobals.h"
#include <stdexcept>

namespace netstorm::client { //게임 입력과 State cleanup이 같은 실제 배열/개수를 공유한다.
// 원문은 용량 관문을 중복 검색보다 먼저 검사하며 가득 차면 표시 통지의 선행 효과도 유지한다.
void ClientUserSelectionGlobals::AppendMultiple(std::uint32_t sid) {
    if (multipleCount>=static_cast<std::int32_t>(multiple.size())) return;if (multipleCount<0) throw std::out_of_range("다중 선택 추가의 음수 물리 인덱스");
    for (std::int32_t index=0;index<multipleCount;++index) if (multiple[static_cast<std::size_t>(index)]==sid) return; //같은 SID는 현재 개수의 앞부분에서만 찾는다.
    multiple[static_cast<std::size_t>(multipleCount)]=sid;++multipleCount;
}
// 제거 뒤의 꼬리 바이트는 지우지 않는다. 현재 count 앞의 안정된 압축만 수행한다.
void ClientUserSelectionGlobals::RemoveMultiple(std::uint32_t sid) {
    if (multipleCount>static_cast<std::int32_t>(multiple.size())) throw std::out_of_range("다중 선택 제거의 현재 배열 물리 범위");std::int32_t removed{};
    for (std::int32_t index=0;index<multipleCount;++index) { //원문은 음수 count이면 방문 없이 그 값을 그대로 보존한다.
        const auto value=multiple[static_cast<std::size_t>(index)];if (value==sid) ++removed;else multiple[static_cast<std::size_t>(index-removed)]=value;
    }
    multipleCount-=removed;
}
}
