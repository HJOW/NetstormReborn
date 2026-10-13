// 원본 자산에서 사제 배치에 필요한 프레임·기준점·SHP 자료를 함께 소유한다.
#pragma once
#include "client/GameAssets.h"
#include "o/RawPriestPlacementShape.h"

namespace netstorm::client {
// 사제 배치 검사에 사용하는 프레임·핫스폿·셰이프·기하 자료이다.
struct PriestPlacementAssets {
    // 내부 배열을 소유하여 배치 모듈의 span이 임시 메타 자료를 참조하지 않게 한다.
    explicit PriestPlacementAssets(const GameAssets& assets);
    // frames: 타입별 프레임 자료.
    std::vector<o::PriestPlainCanonType> frames;
    // hotspots: 프레임 기준점을 찾는 타입별 핫스폿 자료.
    std::vector<o::PriestTypeHotspot> hotspots;
    // shapes: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    std::vector<o::SquidDisplayShape> shapes;
    // geometry: 프레임 발자국·사각형·칸 순회를 계산하는 기하 처리기 또는 상태.
    o::PriestPlacementGeometryState geometry;
};
}
