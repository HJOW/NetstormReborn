// 원본 drag/drop의 판정 단계만 복원한다. 실제 지형·객체 생성과 저장은 원본 후속 부모가 담당한다.
#include "o/RawIslandPlacement.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 다른 원본 청크·패턴 소비자와 같은 물리 자료를 참조한다.
namespace { // 원문 DWORD 산술과 _ftol 좌표 변환만 지역 보조로 둔다.
// signed 덧셈의 DWORD 감김 뒤 원본 float32 저장을 보존한다.
float PlacementOrigin(std::int32_t origin,std::uint32_t offset) {
    return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(origin)+offset));
}
// 실제 decoder의 유한 청크 좌표를 원본 _ftol 하위 DWORD로 변환한다.
std::int32_t PlacementCoordinate(float value) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 원본은 좌측/상단 하한 검사를 추가하지 않으며 우측/하단은 배타 경계로 먼저 확인한다.
bool CanPlaceIsland(OriginalEdition edition,const ChunkMap& target,std::span<const RiftTypeFrames> frames,
    const std::uint32_t& puzzlePieceType,const RawIslandBuilderState& state,std::span<const std::uint8_t,6> record,
    const std::function<void(RawIslandBuilderAssertion)>& diagnostic) {
    const std::array<std::uint8_t,6> value{record[0],record[1],record[2],record[3],record[4],record[5]}; // 원문 stack의 값 전달이며 이후 진단이 외부 입력을 바꿔도 현재 인자는 유지한다.
    if (!diagnostic) throw std::invalid_argument("섬 배치 검사의 원본 진단 장치 누락");
    if (state.bounds.left==-1 || state.bounds.top==-1 || state.bounds.right==-1 || state.bounds.bottom==-1)
        diagnostic(RawIslandBuilderAssertion::InvalidBounds);
    const auto type=puzzlePieceType; // 진단 복귀 뒤 현재 TYPE을 읽고 decoder 생성 중 같은 값을 유지한다.
    if (type>=frames.size()) throw std::out_of_range("섬 배치 검사의 현재 puzzlePiece TYPE 범위");
    const auto direction=static_cast<std::int32_t>(value[0]>>6); // Place와 같이 high2를 그대로 전달한다.
    const auto x=PlacementOrigin(state.bounds.left,value[2]&15U),y=PlacementOrigin(state.bounds.top,value[2]>>4); // decoder 안의 홀수 방향 진단보다 앞서 원점 인자를 고정한다.
    if (edition==OriginalEdition::Cd1072 && (direction&1)) diagnostic(RawIslandBuilderAssertion::OddDirection);
    CanonDecoder decoder(frames[type],TerritoryPatterns()[value[0]&63U],direction,x,y,
        [&diagnostic] { diagnostic(RawIslandBuilderAssertion::MissingFrame); });
    // 첫 유효 패턴 칸을 포함하여 현재 우측/하단과 실제 청크를 매번 다시 검사한다.
    for (;decoder.Valid();decoder.Advance()) {
        const auto cx=PlacementCoordinate(decoder.X()),cy=PlacementCoordinate(decoder.Y());
        if (state.bounds.right<=cx || state.bounds.bottom<=cy) return false;
        if (edition==OriginalEdition::Patch1078 && (cx<0 || cy<0 || cx>=target.XLength() || cy>=target.YLength()))
            diagnostic(RawIslandBuilderAssertion::ChunkRange);
        if (target.At(cx,cy).island!=kNoIsland) return false;
    }
    return true;
}
}
