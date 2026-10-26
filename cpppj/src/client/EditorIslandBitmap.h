// 섬 미리보기의 실제 Context/Growth/Bitmap과 보호 SHP를 조립한다.
#pragma once
#include "client/GameAssets.h"
#include "o/RawTerrainBitmap.h"
#include "o/RawTerrainContext.h"

namespace netstorm::client { // 전용 Gump의 이미지 수명은 호출자가 원본 순서로 소유한다.
o::TerrainBitmapTexture ReadEditorIslandTexture(const GameAssets& assets,std::uint32_t type); // SHP0을 원본200×200 문맥과 같은 메모리에 그린다.
o::TerrainBitmapImage BuildEditorIslandBitmap(const GameAssets& assets,const o::ChunkMap& chunks,std::uint32_t owner,o::ChunkRect bounds,o::TerrainContextGlobals& globals,o::TerrainBitmapState& state,std::function<std::uint32_t()> green); //0046e410→004c1cc0의 같은 실제 지형/로컬 난수/출력이다.
}
