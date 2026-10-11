// 화면 픽셀/카메라 환산은 개별 Draw 몸체가 수행하며 수집/정렬은 월드 좌표를 사용한다.
#include "client/ConstructionVisualRenderer.h"
#include "client/DrawSort.h"
#include <limits>
#include <stdexcept>

namespace netstorm::client {
// 클라이언트의 원본 비교 키를 그대로 사용한다. 수집 순서 보존 옵션은 정렬 전역이 꺼진 상태에 해당한다.
std::int32_t DrawVisibleConstruction(o::ConstructionVisualSystem& system,o::RenderWorldRect view,
    std::span<o::RenderCollectionEntry> work,bool sort) {
    if (work.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::length_error("건설 표시 작업 버퍼 크기 오류");
    const auto count=system.Collect(view,work,static_cast<std::int32_t>(work.size()));auto entries=work.first(static_cast<std::size_t>(count));
    if (sort) SortDrawEntries(entries,system.Pool().Edition());
    system.DrawCollected(entries);return count;
}
}
