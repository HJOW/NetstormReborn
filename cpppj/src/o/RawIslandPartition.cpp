// 좌표/프레임을 snapshot으로 대체하지 않고 원문 순회와 지역 쓰기 효과를 유지한다.
#include "o/RawIslandPartition.h"
#include "o/TerrainBuilder.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector> // Native 재귀 순서를 보존하는 호스트 작업 스택이다.

namespace netstorm::o { // 주소 산술의 호스트 물리 범위만 명시적으로 보호한다.
namespace { // 현재 비정렬 필드와 원문 _ftol의 낮은 DWORD를 읽는다.
std::uint32_t PartitionRead(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { std::uint32_t value{};for (std::size_t index=0;index<width;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);return value; } // 각 필드의 낮은 바이트부터 지정 폭만 결합한다.
std::int32_t PartitionCoordinate(float value) {
    constexpr double bound=9223372036854775808.0; // x86 _ftol의 signed64 유효 경계다. 무효 정수의 낮은 DWORD는0이다.
    if (!std::isfinite(value) || value < -bound || value>=bound) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
} // 원문은 스캔 칸이 아닌 현재 지면의 raw X/Y를 절삭해 flood 시작점으로 사용한다.
}
RawIslandPartition::RawIslandPartition(const SidPool& pool,std::span<const std::uint16_t> surfaces,std::span<const RiftTypeFrames> frames,const std::uint32_t& terrainType,RawIslandRuntime& islands,std::uint32_t& worldRedraw,std::function<std::uint32_t(Sid)> getRegion,std::function<void(Sid,std::uint32_t)> setRegion)
    :pool_(pool),surfaces_(surfaces),frames_(frames),terrainType_(terrainType),islands_(islands),worldRedraw_(worldRedraw),getRegion_(std::move(getRegion)),setRegion_(std::move(setRegion)) { if (pool.Edition()!=OriginalEdition::Patch1078 || &islands.Pool()!=&pool || surfaces.size()!=65536 || !getRegion_ || !setRegion_) throw std::invalid_argument("군도 분리의 실제10.78 Pool/표면/지역 장치 불일치"); }
// 원문 정수 좌표 관문 뒤 같은 hash0 WORD를 읽는다.
Sid RawIslandPartition::Surface(std::int32_t x,std::int32_t y) const { return x<0 || y<0 || x>=256 || y>=256 ? Sid{} : Sid{surfaces_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]}; }
// 원문 네 방향의 깊이 우선 순서를 역순 push로 보존하며 C++ 호출 스택 고갈을 피한다.
FortRandomRectangle RawIslandPartition::Flood(std::int32_t x,std::int32_t y,std::uint32_t region,std::span<const FrameCode> codes) const {
    FortRandomRectangle bounds{257,257,-1,-1};std::vector<std::array<std::int32_t,2>> work{{x,y}};
    while (!work.empty()) { // 같은 component의 현재 연결/지역만 읽으며 방문 기록은 실제 raw 지역 WORD다.
        const auto point=work.back();work.pop_back();const auto sid=Surface(point[0],point[1]);if (!sid.value) continue;auto raw=pool_.Slot(sid);
        if (raw[10]!=terrainType_ || getRegion_(sid)==region) continue;setRegion_(sid,region);
        bounds.left=std::min(bounds.left,static_cast<float>(point[0]));bounds.top=std::min(bounds.top,static_cast<float>(point[1]));bounds.right=std::max(bounds.right,static_cast<float>(point[0]));bounds.bottom=std::max(bounds.bottom,static_cast<float>(point[1]));
        raw=pool_.Slot(sid);const auto frame=std::bit_cast<std::int32_t>(PartitionRead(raw,36));if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("군도 flood의 실제 프레임 코드 범위");
        const auto mask=TerrainBuilder::Connection(static_cast<char>(codes[static_cast<std::size_t>(frame)].side));
        if (mask&8) work.push_back({point[0]-1,point[1]});if (mask&4) work.push_back({point[0],point[1]+1});if (mask&2) work.push_back({point[0]+1,point[1]});if (mask&1) work.push_back({point[0],point[1]-1});
    }++worldRedraw_;return bounds;
}
// 원문은 새 기록의 Temple/padding을 초기화하지 않고 Add와 동일한 필드만 쓴다.
std::uint32_t RawIslandPartition::Split() const {
    std::uint32_t count{};
    for (std::int32_t y=0;y<256;++y) { // 원문 바깥 루프는 hash0의 각256칸 행이다.
        for (std::int32_t x=0;x<256;++x) { // 같은 행을 왼쪽부터 읽으며 TYPE 일치/현재 지역0만 새 component 후보로 삼는다.
            const auto sid=Surface(x,y);if (!sid.value) continue;const auto raw=pool_.Slot(sid);if (raw[10]!=terrainType_ || getRegion_(sid)!=0) continue;
            if (terrainType_>=frames_.size()) throw std::out_of_range("군도 분리의 현재 지면 TYPE 범위");const auto codes=frames_[terrainType_].Codes();const auto region=islands_.Add(0,0xff);
            const auto bounds=Flood(PartitionCoordinate(std::bit_cast<float>(PartitionRead(raw,14))),PartitionCoordinate(std::bit_cast<float>(PartitionRead(raw,18))),region,codes);islands_.Bounds(static_cast<std::int32_t>(region),bounds);++count;
        }
    }return count;
}
}
