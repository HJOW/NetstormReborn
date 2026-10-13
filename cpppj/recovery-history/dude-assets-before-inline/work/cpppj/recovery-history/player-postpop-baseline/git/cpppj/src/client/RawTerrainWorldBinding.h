// 초기 지형 도장의 실제 생성·소유자·공간·지역 쓰기를 같은 raw 월드에 연결한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "o/RawSquidRegion.h"
#include "o/RawTerrainStamp.h"

namespace netstorm::client {
// 원본 ChunkMap 조회와 두 지형 assertion은 호출자가 공급한다. 나머지 네 훅은 실제 월드 몸체로 교체한다.
// world/state/진단 대상은 반환한 훅과 RawTerrainStamp보다 오래 살아야 한다.
o::TerrainStampHooks MakeTerrainStampWorldHooks(RawSurfaceWorld& world,const o::TerrainStampState& state,
    std::function<void(o::SquidRegionAssert)> regionDiagnostic,o::TerrainStampHooks effects);
}
