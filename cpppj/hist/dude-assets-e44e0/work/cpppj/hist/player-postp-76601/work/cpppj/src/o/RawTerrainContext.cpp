// 초기 지형의 code/region과 특수문자·bounds·이웃 분류를 원본 순서대로 구성한다.
#include "o/RawTerrainContext.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 초기 배열은 허공 문자 '.'와 현재 noRegion 하위BYTE로 채운다.
RawTerrainContext::RawTerrainContext(OriginalEdition edition,std::uint32_t owner,std::int32_t region,ChunkRect bounds,
    const TerrainContextGlobals& globals,std::function<void(TerrainContextAssertion)> diagnostic)
    :edition_(edition),owner_(owner),region_(region),diagnostic_(std::move(diagnostic)) {
    if (!diagnostic_) throw std::invalid_argument("초기 지형 context의 bounds 진단 연결 누락");
    codes_.fill('.');regions_.fill(static_cast<std::uint8_t>(globals.noRegion));
    mapping_['.']=0;mapping_['x']=static_cast<std::int32_t>(globals.islandType);
    mapping_['X']=static_cast<std::int32_t>(globals.edgeType);mapping_['o']=0;
    mapping_['=']=static_cast<std::int32_t>(globals.islandType);
    aliases_['x']='x';aliases_['X']='x';aliases_['o']='x';aliases_['=']='x';
    map_={codes_,regions_,mapping_,aliases_,{},1,{},globals.battle ? 0x10U : 0U};
    if (!owner_) map_.excludedFlags=0x18;
    SetBounds(bounds);
}
// 원본 DWORD 왼쪽 shift의 감김을 보존하며 음수 signed shift의 호스트 미정의 동작을 피한다.
void RawTerrainContext::SetBounds(ChunkRect bounds) {
    chunks_=bounds;
    const auto cells=[](std::int32_t value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)<<4); };
    map_.bounds={cells(bounds.left),cells(bounds.top),cells(bounds.right),cells(bounds.bottom)};
    if (edition_==OriginalEdition::Patch1078) {
        if (bounds.right>16) diagnostic_(TerrainContextAssertion::RightChunkLimit);
        if (chunks_.bottom>16) diagnostic_(TerrainContextAssertion::BottomChunkLimit);
    }
    if (chunks_.right>16) chunks_.right=16;
    if (chunks_.bottom>16) chunks_.bottom=16;
}
// 저장 범위를 넘으면 원본 자료 손상으로 진단한다. 가장자리로 끌어당기지 않는다.
std::size_t RawTerrainContext::Index(std::int32_t x,std::int32_t y) {
    if (x<0 || x>=256 || y<0 || y>=256) throw std::out_of_range("초기 지형 context 지도 범위 오류");
    return static_cast<std::size_t>(x)*256+static_cast<std::size_t>(y);
}
// signed char 음수 주소는 현재 ASCII 대응표의 정상 입력이 아니다.
std::size_t RawTerrainContext::Character(std::uint8_t value) {
    if (std::bit_cast<std::int8_t>(value)<0) throw std::out_of_range("초기 지형 context signed 문자 주소 범위 오류");
    return value;
}
// 이웃 판정은 반열린 순회 bounds와 달리 right/bottom 칸도 포함한다.
void RawTerrainContext::MarkEdges(std::int32_t code,std::uint8_t edge) {
    // 원본의 Y외부/X내부 순서로 앞에서 바뀐 문자도 alias를 통해 후행 판정에 반영한다.
    for (auto y=map_.bounds[1];y<map_.bounds[3];++y) {
        // 각 행의 현재 X범위를 읽는다.
        for (auto x=map_.bounds[0];x<map_.bounds[2];++x) {
            const auto index=Index(x,y);
            if (static_cast<std::int32_t>(std::bit_cast<std::int8_t>(codes_[index]))!=code) continue;
            // 북부터 시계방향 실제 여덟 이웃에서 첫 불일치만 처리한다.
            for (const auto [dx,dy]:kNeighborCells) {
                const auto nx=x+dx,ny=y+dy;
                if (nx<map_.bounds[0] || ny<map_.bounds[1] || nx>map_.bounds[2] || ny>map_.bounds[3]) {
                    codes_[index]=edge;break;
                }
                const auto neighbor=Index(nx,ny);
                if (aliases_[Character(codes_[neighbor])]!=static_cast<std::int32_t>(std::bit_cast<std::int8_t>(codes_[index])) ||
                    regions_[neighbor]!=regions_[index]) { codes_[index]=edge;break; }
            }
        }
    }
}
// 지역BYTE와 다른 문자/배열 밖 값은 변경하지 않는다.
void RawTerrainContext::NormalizeAfterStamp() {
    // 도장의 같은 반열린 Y외부/X내부 bounds를 순회한다.
    for (auto y=map_.bounds[1];y<map_.bounds[3];++y)
        // 원본의 현재 X범위를 각 행에서 다시 읽는다.
        for (auto x=map_.bounds[0];x<map_.bounds[2];++x) {
            auto& code=codes_[Index(x,y)];if (code=='=' || code=='o' || code=='X') code='x';
        }
}
// 복사한 지도 대신 현재 도장 context의 참조를 반환한다.
TerrainStampMap& RawTerrainContext::Map() { return map_; }
// 성장/경계 처리가 같은 X열 우선 배열에 쓴다.
std::span<std::uint8_t> RawTerrainContext::Codes() { return codes_; }
std::span<std::uint8_t> RawTerrainContext::Regions() { return regions_; }
// 문자 대응표를 별도 사본으로 만들지 않는다.
std::span<std::int32_t> RawTerrainContext::Aliases() { return aliases_; }
// 칸 bounds를 나누어 청크 범위를 새로 만들지 않는다.
ChunkRect RawTerrainContext::Chunks() const { return chunks_; }
// 원본 context의 현재 요청을 읽는다.
std::uint32_t RawTerrainContext::Owner() const { return owner_; }
std::int32_t RawTerrainContext::Region() const { return region_; }
}
