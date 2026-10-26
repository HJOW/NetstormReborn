// 원본 현재 TYPE0/헤더/RLE와 지형 몸체만 연결하며 미니맵 출력으로 대체하지 않는다.
#include "client/EditorIslandBitmap.h"
#include "o/RawTerrainGrowth.h"
#include <memory>
#include <stdexcept>

namespace netstorm::client { // 현재 bitmap은 원본 팔레트 인덱스 그대로 반환한다.
o::TerrainBitmapTexture ReadEditorIslandTexture(const GameAssets& assets,std::uint32_t type) {
    if (type<70 || type-70>=assets.Types().size()) throw std::out_of_range("섬 bitmap 현재 texture TYPE 범위");
    const auto& asset=assets.Types()[type-70];const auto& shape=assets.Shapes();const auto header=shape.SquidMetrics(asset.block,0);
    o::TerrainBitmapTexture texture;texture.width=header.width;texture.height=header.height;texture.pixels.fill(255);
    if (DrawShape(texture.pixels,200,200,shape,asset.block,0,header.hotspotX,header.hotspotY,{0,0,199,199})!=0) throw std::logic_error("섬 bitmap의 실제 SHP0 출력 실패");
    return texture;
}
// 원문 상위는 별도 context와 owner*4323 로컬 seed를 쓰며 공유 게임 난수를 사용하지 않는다.
o::TerrainBitmapImage BuildEditorIslandBitmap(const GameAssets& assets,const o::ChunkMap& chunks,std::uint32_t owner,o::ChunkRect bounds,o::TerrainContextGlobals& globals,o::TerrainBitmapState& state,std::function<std::uint32_t()> green) {
    if (bounds.left<0 || bounds.top<0 || bounds.right>16 || bounds.bottom>16 || bounds.left>bounds.right || bounds.top>bounds.bottom) throw std::out_of_range("섬 bitmap의 유효 청크 bounds");
    auto context=std::make_unique<o::RawTerrainContext>(assets.Edition(),owner,globals.noRegion,bounds,globals,[](auto) { throw std::logic_error("섬 bitmap 원문 context 진단"); });
    o::RawTerrainGrowth growth(*context,chunks,globals);growth.RunNormal();context->MarkEdges('x','=');
    o::GameRandom random(owner*0x10e3U);o::RawTerrainBitmap body(state,globals.islandType,{std::move(green),[&assets](auto type) { return ReadEditorIslandTexture(assets,type); }});
    return body.Render(std::span<const std::uint8_t,65536>(context->Codes().data(),65536),(bounds.right-bounds.left)*16,(bounds.bottom-bounds.top)*16,random);
}
}
