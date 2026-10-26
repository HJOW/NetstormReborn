// 편집기의 임시 청크 지도에서 영역 패턴을 놓을 수 있는지 원본 순서로 검사한다.
#pragma once
#include "o/RawIslandBuilder.h"

namespace netstorm::o { // 원본 Islandbuilder의 공용 지도 검사이며 Client UI 저장소는 소유하지 않는다.
// 10.78 0046de50 / CD 00490a50: 현재 bounds·TYPE 프레임·청크를 읽고 쓰기 없이 0/1을 반환한다.
// target/state/frames/puzzlePieceType은 호출 동안 살아 있어야 하며 진단은 기존 builder와 같은 경계를 사용한다.
bool CanPlaceIsland(OriginalEdition edition,const ChunkMap& target,std::span<const RiftTypeFrames> frames,
    const std::uint32_t& puzzlePieceType,const RawIslandBuilderState& state,std::span<const std::uint8_t,6> record,
    const std::function<void(RawIslandBuilderAssertion)>& diagnostic);
}
