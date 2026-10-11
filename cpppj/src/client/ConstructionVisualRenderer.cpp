// 화면 픽셀/카메라 환산은 개별 Draw 몸체가 수행하며 수집/정렬은 월드 좌표를 사용한다.
#include "client/ConstructionVisualRenderer.h"
#include "client/Renderer.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
// 클라이언트의 원본 비교 키를 그대로 사용한다. 수집 순서 보존 옵션은 정렬 전역이 꺼진 상태에 해당한다.
std::int32_t DrawVisibleConstruction(o::ConstructionVisualSystem& system,o::RenderWorldRect view,
    std::span<o::RenderCollectionEntry> work,bool sort) {
    if (work.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::length_error("건설 표시 작업 버퍼 크기 오류");
    const auto count=system.Collect(view,work,static_cast<std::int32_t>(work.size()));auto entries=work.first(static_cast<std::size_t>(count));
    if (sort) {
        // NaN 비교의 순환 관계를 표준 정렬기에 넘기지 않으며 출력 전에 거부한다.
        for (const auto& entry:entries) if (!std::isfinite(entry.x) || !std::isfinite(entry.y)) throw std::invalid_argument("건설 표시 정렬 좌표 오류");
        std::stable_sort(entries.begin(),entries.end(),[](const o::RenderCollectionEntry& a,const o::RenderCollectionEntry& b) {
            return CompareDrawOrder({a.x,a.y,a.depth},{b.x,b.y,b.depth})<0;
        });
    }
    system.DrawCollected(entries);return count;
}
}
