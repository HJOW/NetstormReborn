// 원본의 첫 record/현재 PlayerId/미션 패턴과 DWORD 청크 산술을 그대로 연결한다.
#include "o/RawPlayerGeyserBounds.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kPlayerBytes=0xb4,kFlags=0x50,kRegion=0x78; // PATCH Player 원문 폭과 현재 입력 필드다.
constexpr float kExpansion=4.0F; // ROM00502314의 실제 칸 단위 양끝 확장값이다.
// 원본의 DWORD 감김을 보존한 뒤 signed 결과로 읽는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 비정렬 필드의 정확 네 BYTE를 읽는다.
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result=0;
    // 낮은 바이트부터 읽으며 Player의 주변 필드를 섞지 않는다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[offset+index])<<(index*8);return result;
}
// 실제 정수 청크 좌표의 DWORD<<4를 signed로 해석한 뒤 float로 저장한다.
float Cell(std::int32_t value) { return static_cast<float>(Signed(static_cast<std::uint32_t>(value)<<4)); }
}
// 독립 값형 지도나 축약 IslandList를 받지 않으며 현재 판본과 물리 표 크기를 검증한다.
RawPlayerGeyserBounds::RawPlayerGeyserBounds(const SidPool& pool,const ChunkMap& chunks,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> players,PlayerGeyserBoundsState state,
    std::function<void(PlayerGeyserBoundsAssertion)> assertion)
    :pool_(pool),chunks_(chunks),types_(types),frames_(frames),players_(players),state_(state),assertion_(std::move(assertion)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188 || players.size()!=9*kPlayerBytes || !assertion_)
        throw std::invalid_argument("Player 간헐천 범위의 현재 원본 자료/진단 누락");
}
// 지도 전체의 원본 순서이며 첫 일치에서만 record6BYTE를 복사한다.
std::array<std::uint8_t,6> RawPlayerGeyserBounds::RegionRecord(std::int32_t region) const {
    // 원본은 먼저(0,0)의 유효 getter를 호출하지만 실제 ChunkMap은 생성부터 유효하며 최소1×1이다.
    for (std::int32_t y=0;y<chunks_.YLength();++y) {
        // 이 행의 x를 먼저 증가시키며 signed BYTE를 regionDWORD와 비교한다.
        for (std::int32_t x=0;x<chunks_.XLength();++x) {
            const auto& entry=chunks_.At(x,y);
            if (static_cast<std::int32_t>(std::bit_cast<std::int8_t>(entry.island))==region) return entry.record;
        }
    }
    return {};
}
// 실제 current TYPE와 기존 CanonDecoder가 수행하는 최초 프레임 검색/진단도 생략하지 않는다.
std::array<std::int32_t,2> RawPlayerGeyserBounds::Dimensions(std::span<const std::uint8_t,6> record) const {
    const auto type=state_.puzzlePieceType;const auto patterns=TerritoryPatterns(); // 호출 시점의 실제 패턴 타입과 ROM 표다.
    if (type>=types_.size() || (record[0]&63U)>=patterns.size()) throw std::out_of_range("Player 범위의 현재 영역 타입/패턴 물리 범위");
    CanonDecoder decoder(frames_[type],patterns[record[0]&63U],static_cast<int>(record[0]>>6)*2,0,0,
        [this] { assertion_(PlayerGeyserBoundsAssertion::MissingFrame); });
    const auto bounds=decoder.Bounds(types_[type].footX,types_[type].footY); // 원본004bfd70→00425b90의 같은 발자국 기반 결과다.
    return {Signed(static_cast<std::uint32_t>(bounds[2])-static_cast<std::uint32_t>(bounds[0])),
        Signed(static_cast<std::uint32_t>(bounds[3])-static_cast<std::uint32_t>(bounds[1]))};
}
// 원본 helper0041ca10은 매 호출 현재 BYTE로 로컬[3,4,5,6]을 고른다.
std::uint32_t RawPlayerGeyserBounds::Spacing() const {
    constexpr std::array<std::uint32_t,4> spacing{3,4,5,6}; // 원본 스택 배열에 쓰는 네 정수다.
    if (state_.mapSize>=spacing.size()) throw std::out_of_range("Player 배치의 현재 mapSize 물리 범위");return spacing[state_.mapSize];
}
// 진단이 현재 모드/방향표를 바꾸더라도 그 뒤의 실제 조회는 다시 읽는다.
ChunkRect RawPlayerGeyserBounds::Placement(std::int32_t player,std::span<const std::uint8_t,6> record) const {
    if (!state_.battle) assertion_(PlayerGeyserBoundsAssertion::NotBattle);
    if (player<1 || player>8) assertion_(PlayerGeyserBoundsAssertion::InvalidPlayer);
    if (state_.mission) {
        const auto size=Dimensions(record);const auto x=std::uint32_t(record[2]&15U)+1U,y=std::uint32_t(record[2]>>4)+1U; // 현재 위치 nibble은 원본 기본left/top1에 더한다.
        return {Signed(x),Signed(y),Signed(x+static_cast<std::uint32_t>(size[0])),Signed(y+static_cast<std::uint32_t>(size[1]))};
    }
    if (player<0 || static_cast<std::size_t>(player)>=state_.playerDirections.size()) throw std::out_of_range("Player 배치의 방향표 물리 범위");
    const auto direction=state_.playerDirections[static_cast<std::size_t>(player)],even=direction&~1U; // 현재 방향의 lowbit는 대각선 추가 이동을 고른다.
    if (even>=kNeighborCells.size()) throw std::out_of_range("Player 배치의 현재 방향 물리 범위");
    const auto [dx,dy]=kNeighborCells[even]; // ROM0052f83c/52f85c의 같은 북부터 시계 방향 이웃이다.
    auto x=9U+Spacing()*static_cast<std::uint32_t>(dx),y=9U+Spacing()*static_cast<std::uint32_t>(dy);
    if (direction&1U) { x-=Spacing()*static_cast<std::uint32_t>(dy);y+=Spacing()*static_cast<std::uint32_t>(dx); }
    return {Signed(x-2U),Signed(y-2U),Signed(x),Signed(y)};
}
// flags진단→현재region record복사→현재PlayerId 진단→배치→좌표저장/확장 순서다.
FortRandomRectangle RawPlayerGeyserBounds::Bounds(std::uint32_t player) const {
    if (player>=9) throw std::out_of_range("Player 간헐천 범위의 원문9기록 경계");
    const auto offset=player*kPlayerBytes; // 유효한 동일 Player 원문의 시작 위치다.
    if (!(players_[offset+kFlags]&0x22U)) assertion_(PlayerGeyserBoundsAssertion::InactivePlayer);
    const auto record=RegionRecord(Signed(Word(players_,offset+kRegion))); // PlayerId 진단 전에 원본처럼 record를 값으로 보존한다.
    if (state_.metamap) assertion_(PlayerGeyserBoundsAssertion::Metamap);
    const auto rectangle=Placement(static_cast<std::int32_t>(player),record); // 유효한 원문 주소의 PlayerId는 같은0~8인덱스다.
    return {static_cast<float>(static_cast<double>(Cell(rectangle.left))-kExpansion),static_cast<float>(static_cast<double>(Cell(rectangle.top))-kExpansion),
        static_cast<float>(static_cast<double>(Cell(rectangle.right))+kExpansion),static_cast<float>(static_cast<double>(Cell(rectangle.bottom))+kExpansion)};
}
// 실제 풀과 지도의 참조만 노출하며 사본을 생성하지 않는다.
const SidPool& RawPlayerGeyserBounds::Pool() const { return pool_; }
const ChunkMap& RawPlayerGeyserBounds::Map() const { return chunks_; }
}
