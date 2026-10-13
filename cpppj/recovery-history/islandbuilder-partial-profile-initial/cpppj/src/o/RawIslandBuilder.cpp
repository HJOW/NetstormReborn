// 실제 패턴의 유효 칸만 현재 청크에 쓰고 world에서는 Runtime.Add와Bounds setter를 재사용한다.
#include "o/RawIslandBuilder.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 Territory 레코드와 실제20개 레코드 저장소의 바이트 수다.
constexpr std::size_t kRawTerritoryRecord=6,kRawTerritoryBytes=120;
// 원본 DWORD감김16배를float로 저장하며 signed shift의 호스트 미정의 동작을 피한다.
float CellBoundary(std::int32_t value) { return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)<<4)); }
// 원본 시작 좌표의 DWORD감김 덧셈과 float32 저장이다.
float Origin(std::int32_t value,std::uint32_t offset) { return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+offset)); }
// decoder float의 실제 _ftol 하위DWORD를 사용하여INT_MAX 부근의 float반올림도 보존한다.
std::int32_t ToChunkCoordinate(float value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); }
}
// 기존 Runtime/CanonDecoder가 조회하는 판본별 타입 수와 필수 진단만 확인한다.
RawIslandBuilder::RawIslandBuilder(ChunkMap& target,const ChunkMap& world,RawIslandRuntime& islands,
    std::span<const RiftTypeFrames> frames,std::uint32_t& puzzlePieceType,RawIslandBuilderState& state,
    std::function<void(RawIslandBuilderAssertion)> diagnostic)
    :target_(target),world_(world),islands_(islands),frames_(frames),puzzlePieceType_(puzzlePieceType),state_(state),diagnostic_(std::move(diagnostic)) {
    if (frames_.size()!=(Pool().Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !diagnostic_)
        throw std::invalid_argument("실제 IslandBuilder 판본/진단 연결 오류");
}
// 원본 inWorld는 대상 지도 주소와 world 전역 지도 주소만 비교한다.
bool RawIslandBuilder::InWorld() const { return &target_==&world_; }
// patch의한combined 좌표 진단과CD 무진단·기존getter 끝clamp를 구분한다.
ChunkEntry& RawIslandBuilder::ChunkAt(std::int32_t x,std::int32_t y) const {
    if (Pool().Edition()==OriginalEdition::Patch1078 && (x<0 || y<0 || x>=target_.XLength() || y>=target_.YLength()))
        diagnostic_(RawIslandBuilderAssertion::ChunkRange);
    return target_.At(x,y);
}
// 원본 Place는 record.high2를그대로전달한다. 패턴Count의high2×2를 이 몸체에 적용하지 않는다.
std::uint32_t RawIslandBuilder::Place(std::span<const std::uint8_t,6> record,std::uint32_t territory,bool ignorePosition) const {
    if (state_.bounds.left==-1 || state_.bounds.top==-1 || state_.bounds.right==-1 || state_.bounds.bottom==-1)
        diagnostic_(RawIslandBuilderAssertion::InvalidBounds);
    const auto offsetX=ignorePosition ? 0U : record[2]&15U,offsetY=ignorePosition ? 0U : record[2]>>4;
    auto region=territory;
    if (InWorld()) region=islands_.Add(state_.owner,static_cast<std::uint8_t>(territory));
    const auto x=Origin(state_.bounds.left,offsetX),y=Origin(state_.bounds.top,offsetY);
    const auto type=puzzlePieceType_,pattern=static_cast<std::uint32_t>(record[0]&63U);
    const auto direction=static_cast<std::int32_t>(record[0]>>6);
    if (type>=frames_.size()) throw std::out_of_range("실제 IslandBuilder 현재 puzzlePiece 타입 범위 오류");
    if (Pool().Edition()==OriginalEdition::Cd1072 && (direction&1)) diagnostic_(RawIslandBuilderAssertion::OddDirection);
    const auto patterns=TerritoryPatterns();if (pattern>=patterns.size()) throw std::out_of_range("실제 IslandBuilder 영역 패턴 범위 오류");
    CanonDecoder decoder(frames_[type],patterns[pattern],direction,x,y,
        [this] { diagnostic_(RawIslandBuilderAssertion::MissingFrame); });std::int32_t ordinal=0;
    // 실제 decoder의 최초유효칸을 포함하고 프레임이없는칸은 완료된Advance가 건너뛴다.
    for (;decoder.Valid();decoder.Advance(),++ordinal) {
        auto& entry=ChunkAt(ToChunkCoordinate(decoder.X()),ToChunkCoordinate(decoder.Y()));
        entry.island=static_cast<std::uint8_t>(region);
        // 원본 record6BYTE만 복사하며 방향/ordinal은 별도 필드에 기록한다.
        for (std::size_t byte=0;byte<kRawTerritoryRecord;++byte) entry.record[byte]=record[byte];
        entry.ordinal=ordinal;entry.side=decoder.Side();
    }
    if (InWorld()) {
        const auto number=std::bit_cast<std::int32_t>(region);
        if (!islands_.Exists(number)) diagnostic_(RawIslandBuilderAssertion::BoundsMissingIsland);
        const auto bounds=target_.Bounds(number);
        if (Pool().Edition()==OriginalEdition::Cd1072 && bounds.left==16 && bounds.top==16)
            diagnostic_(RawIslandBuilderAssertion::BoundsNotFound);
        islands_.Bounds(number,{CellBoundary(bounds.left),CellBoundary(bounds.top),CellBoundary(bounds.right),CellBoundary(bounds.bottom)});
    }
    return region;
}
// 물리20레코드 밖의 손상 메모리는 읽지 않고 그 안의 원본빈/역순범위는 그대로 처리한다.
void RawIslandBuilder::PlaceAll(std::span<const std::uint8_t> records,std::int32_t start,std::int32_t end) const {
    if (records.size()<kRawTerritoryBytes || start<0 || end<0 || start>20 || end>20)
        throw std::out_of_range("실제 IslandBuilder Territory 물리20레코드 범위 오류");
    if (start==end) diagnostic_(RawIslandBuilderAssertion::EqualRange);
    // 원본은 각레코드의현재flag를 다시읽고 이름/지역순서로 정렬하지 않는다.
    for (auto territory=start;territory<end;++territory) {
        const auto record=records.subspan(static_cast<std::size_t>(territory)*kRawTerritoryRecord,kRawTerritoryRecord);
        if ((record[1]&1) && !(record[1]&2)) Place(std::span<const std::uint8_t,6>(record.data(),kRawTerritoryRecord),static_cast<std::uint32_t>(territory),false);
    }
}
// 새 풀/섬 vector가 아닌 완료 Runtime의 실제 풀 참조다.
const SidPool& RawIslandBuilder::Pool() const { return islands_.Pool(); }
}
