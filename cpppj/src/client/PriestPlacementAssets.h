// 원본 자산에서 사제 배치에 필요한 프레임·기준점·SHP 자료를 함께 소유한다.
#pragma once
#include "client/GameAssets.h"
#include "o/RawPriestPlacementShape.h"

namespace netstorm::client {
struct PriestPlacementAssets {
    // 내부 배열을 소유하여 배치 모듈의 span이 임시 메타 자료를 참조하지 않게 한다.
    explicit PriestPlacementAssets(const GameAssets& assets);
    std::vector<o::PriestPlainCanonType> frames;
    std::vector<o::PriestTypeHotspot> hotspots;
    std::vector<o::SquidDisplayShape> shapes;
    o::PriestPlacementGeometryState geometry;
};
}
