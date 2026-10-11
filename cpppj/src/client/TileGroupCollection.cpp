// 00433410 ↔ CD 004884a0의 실제 행 묶음/빈 칸/깊이와 정상 종료 시 해시 선택을 옮긴다.
#include "client/TileGroupCollection.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
// 실제 풀/해시 참조와 토큰 영역을 검증하고 모든 월드 칸을 담을 안정된 멤버 저장소를 확보한다.
TileGroupCollection::TileGroupCollection(o::SquidHash& hash,const o::SidPool& pool,TileGroupCollectionState state)
    :hash_(hash),pool_(pool),state_(state) {
    if (state.screenHeight<0 || state.screenHeight>std::numeric_limits<std::int32_t>::max()-66)
        throw std::invalid_argument("Tile group screen height");
    const auto end=static_cast<std::uint64_t>(state.objectTokenBase)+static_cast<std::uint64_t>(pool.Capacity())*pool.Layout().stride;
    const auto groupEnd=static_cast<std::uint64_t>(state.groupTokenBase)+o::kWorldCells*8;
    if (end>0x100000000ULL || groupEnd>0x100000000ULL ||
        (state.objectTokenBase<groupEnd && state.groupTokenBase<end)) throw std::invalid_argument("Tile group token ranges");
    entries_.reserve(o::kWorldCells*o::kWorldCells);groups_.reserve(o::kWorldCells);
}
// 좌표 보조의 +0.5와 CRT 64비트 절삭을 거쳐 확장 전 signed 셀 키를 반환한다.
std::int32_t TileGroupCollection::Cell(float coordinate) {
    // NaN/범위 밖 FISTP의 정수 indefinite는 하위 DWORD 0이다. 큰 유한 수도 하위 DWORD를 보존한다.
    const double value=static_cast<double>(coordinate)+0.5;std::int64_t integer=0;
    if (std::isfinite(value) && value>=-0x1p63 && value<0x1p63) integer=static_cast<std::int64_t>(value);
    const auto cell=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(integer));
    return std::min(cell,o::kWorldCells-1);
}
// 0단계 머리/현재 슬롯에서 행별 원본 키를 만들고 정상 반환 때 2단계 선택과 배경 결과를 제출한다.
std::span<const DrawGroupView> TileGroupCollection::Collect(o::RenderWorldRect view,std::int32_t rowLimit,bool& backgroundCovered) {
    entries_.clear();groups_.clear();hash_.Select(0);bool covered=true;
    // 원본 DWORD 확장의 넘침을 C++ signed overflow 없이 재현하고 각 끝을 독립적으로 클램프한다.
    const auto expand=[](std::int32_t cell,std::uint32_t step) {
        return std::clamp(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(cell)+step),0,o::kWorldCells-1);
    };
    const int left=expand(Cell(view.left),0xffffffffU),top=expand(Cell(view.top),0xffffffffU);
    const int right=expand(Cell(view.right),1),bottom=expand(Cell(view.bottom),1);
    const int maxRows=pool_.Edition()==o::OriginalEdition::Patch1078 ? state_.screenHeight/11+6 : 49;
    // 빈 행은 헤더를 만들지 않으며 뒤 행은 같은 연속 멤버 배열의 다음 위치에 기록한다.
    for (int y=top;y<=bottom;++y) {
        const auto start=entries_.size();std::int32_t count=0;
        // 원본은 한 칸의 머리만 읽는다. next/type/state/실제 프레임 크기는 이 수집기의 필터가 아니다.
        for (int x=left;x<=right;++x) {
            const o::Sid sid{hash_.At(x,y)};
            if (!sid.value) { covered=false;continue; }
            const auto raw=pool_.Slot(sid);o::RenderCollectionEntry entry;
            std::memcpy(&entry.x,raw.data()+14,4);std::memcpy(&entry.y,raw.data()+18,4);
            entry.token=state_.objectTokenBase+sid.value*pool_.Layout().stride;
            const auto depthOffset=pool_.Edition()==o::OriginalEdition::Patch1078 ? 35 : 33;
            entry.depth=std::bit_cast<std::int8_t>(raw[depthOffset]);entry.kind=1;entry.tail=0;
            entries_.push_back(entry);++count;
            if (count>=rowLimit) throw std::length_error("Tile group count < rowLimit");
        }
        if (count>0) {
            counts_[groups_.size()]=count;
            groups_.push_back({state_.groupTokenBase+static_cast<std::uint32_t>(groups_.size())*8,
                std::span<const o::RenderCollectionEntry>(entries_).subspan(start,static_cast<std::size_t>(count)),&counts_[groups_.size()]});
            if (static_cast<int>(groups_.size())>=maxRows) throw std::length_error("Tile group listCount < MAX_TILE_ROWS");
        }
    }
    hash_.Select(2);backgroundCovered=covered;return groups_;
}
// kind 1이 받은 헤더 토큰을 현재 프레임의 멤버 span과 signed 개수로 해석한다.
DrawGroupView TileGroupCollection::Group(std::uint32_t token) const {
    if (token<state_.groupTokenBase || (token-state_.groupTokenBase)%8) throw std::out_of_range("Tile group token");
    return groups_.at((token-state_.groupTokenBase)/8);
}
// raw 객체 토큰의 기준/stride/범위를 검사한 뒤 sprite 경계가 사용할 실제 SID를 반환한다.
o::Sid TileGroupCollection::Object(std::uint32_t token) const {
    if (token<state_.objectTokenBase) throw std::out_of_range("Tile object token");
    const auto offset=token-state_.objectTokenBase,stride=pool_.Layout().stride;
    if (offset%stride || offset/stride==0 || offset/stride>=pool_.Capacity()) throw std::out_of_range("Tile object SID");
    return {static_cast<std::uint16_t>(offset/stride)};
}
// 현재 프레임의 연속 멤버 배열을 복사 없이 읽는다. 다음 수집 때는 이전 내용을 사용할 수 없다.
std::span<const o::RenderCollectionEntry> TileGroupCollection::Entries() const { return entries_; }
// 수집/분배 양쪽이 같은 헤더/멤버 저장소를 사용하게 연결한다. 호출자는 collector 수명을 유지한다.
void BindTileGroupCollection(DrawCollectionHooks& collection,DrawDispatchHooks& dispatch,TileGroupCollection& collector) {
    // 가시 월드 범위/행별 제한/배경 관찰을 실제 해시와 풀로 전달한다.
    collection.groups=[&collector](auto view,auto rowLimit,bool& covered) { return collector.Collect(view,rowLimit,covered); };
    // 정렬된 kind 1은 같은 수집기의 현재 헤더와 내부 멤버를 사용한다.
    dispatch.group=[&collector](auto token) { return collector.Group(token); };
}
}
