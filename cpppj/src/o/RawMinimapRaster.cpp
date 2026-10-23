// 지면과 해시 체인을 별도 객체 목록이나 일반 아이콘으로 대체하지 않는다.
#include "o/RawMinimapRaster.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 미니맵 행 폭과 셀 수이며 월드256칸을 두 칸당 한 픽셀로 그린다.
constexpr std::int32_t kWidth=128,kCells=64;
}
// 지면 Hash0의 alias와 나머지 해시가 같은 인스턴스임을 참조로 보장한다.
RawMinimapRaster::RawMinimapRaster(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,const MinimapRasterState& state)
    :pool_(pool),hash_(hash),types_(types),state_(state) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.empty()) throw std::invalid_argument("미니맵 셀 그림의 실제10.78 Pool/TYPE가 필요합니다");
}
// 원본의 색 억제는 검정이 아닌 현재 프레임 하위BYTE를 쓴다.
std::uint8_t RawMinimapRaster::OwnerColor(std::uint8_t owner,std::uint32_t fallback) const {
    if (state_.suppressColors) return static_cast<std::uint8_t>(state_.frame);
    return static_cast<std::uint8_t>(owner>=1 && owner<=8 ? state_.owners[owner-1] : fallback);
}
// 원본 x87 ftol의 integer-indefinite 하위DWORD는0이다.
std::int32_t RawMinimapRaster::Coordinate(std::span<const std::uint8_t> raw,std::size_t at) {
    std::uint32_t bits{}; // 정렬되지 않은 현재 좌표의 실제 네BYTE다.
    // 이웃 필드를 읽지 않고 little endian DWORD만 조합한다.
    for (std::size_t i=0;i<4;++i) bits|=std::uint32_t(raw[at+i])<<(i*8);
    const auto value=static_cast<double>(std::bit_cast<float>(bits)); // ftol은 현재float 좌표를 정확히 확장한다.
    if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 원본은 유효 월드 입력을 전제한다. 손상 입력이 호스트 메모리를 쓰지 않도록 명시 거부한다.
void RawMinimapRaster::Pixel(std::span<std::uint8_t> bitmap,std::int32_t x,std::int32_t y,std::uint8_t color) {
    if (x<0 || x>=kWidth || y<0 || y>=kWidth) throw std::out_of_range("미니맵 픽셀 좌표 범위 오류");
    bitmap[static_cast<std::size_t>(y)*kWidth+x]=color;
}
// 일반 모드의 다리는 한 점, Challenge의 두 TYPE만 짝수 정렬된 네 점이다.
void RawMinimapRaster::Chain(std::span<std::uint8_t> bitmap,std::uint16_t head,bool challengeMarks) const {
    std::uint32_t visited{}; // 손상된 raw next 순환만 거부하며 정상 체인 방문 순서는 바꾸지 않는다.
    // next는 같은 슬롯의+4 WORD이며 지면/상태/소유자 추가 필터를 넣지 않는다.
    for (auto sid=head;sid;) {
        if (++visited>pool_.Capacity()) throw std::logic_error("미니맵 해시 체인의 순환");
        const auto raw=pool_.Slot({sid}); // 효과 직전 현재 슬롯이며 사본 목록을 사용하지 않는다.
        if (!state_.challenge) {
            if (raw[10]>=types_.size()) throw std::out_of_range("미니맵 해시의 TYPE 범위 오류");
            const auto flags=types_[raw[10]].flags2; //004ac200의 현재 getter 결과다.
            if ((flags&6)!=0) {
                const auto x=Coordinate(raw,14),y=Coordinate(raw,18); // 원본은 island에서도 먼저 좌표를 절삭한다.
                if (flags&4) Pixel(bitmap,x/2,y/2,OwnerColor(raw[34],246));
            }
        } else if (challengeMarks && (raw[10]==state_.battleIslandType || raw[10]==state_.playerIslandType)) {
            const auto x=(Coordinate(raw,14)/2)&~1,y=(Coordinate(raw,18)/2)&~1; // signed /2 뒤 각축 bit0만 지운다.
            const auto color=static_cast<std::uint8_t>(state_.suppressColors ? state_.frame : raw[10]==state_.battleIslandType ? state_.challengeGreen : state_.owners[4]); // playerIsland는 자홍 전역을 쓴다.
            Pixel(bitmap,x,y,color);Pixel(bitmap,x+1,y,color);Pixel(bitmap,x,y+1,color);Pixel(bitmap,x+1,y+1,color);
        }
        sid=static_cast<std::uint16_t>(raw[4]|(std::uint32_t(raw[5])<<8));
    }
}
// 전체 Refresh는 이 몸체를 행 우선으로 부르며 한 셀 안의 원본 순서는 여기서 보존한다.
void RawMinimapRaster::DrawCell(std::span<std::uint8_t> bitmap,std::int32_t cellX,std::int32_t cellY) const {
    if (bitmap.size()!=kWidth*kWidth || cellX<0 || cellX>=kCells || cellY<0 || cellY>=kCells) throw std::out_of_range("미니맵 bitmap/셀 범위 오류");
    const auto px=cellX*2,py=cellY*2; // bitmap의 해당2×2 왼쪽 위다.
    const auto black=static_cast<std::uint8_t>(state_.black); // 초기화는 색 억제와 무관하게 검정 전역이다.
    Pixel(bitmap,px,py,black);Pixel(bitmap,px+1,py,black);Pixel(bitmap,px,py+1,black);Pixel(bitmap,px+1,py+1,black);
    // 지면은 Y 바깥/X 안쪽으로4×4를 읽어 마지막 지면의 색이 같은 픽셀을 덮게 한다.
    for (auto y=cellY*4;y<cellY*4+4;++y) {
        // 같은 Hash0의 현재 머리만 지면으로 읽으며 next를 따라가지 않는다.
        for (auto x=cellX*4;x<cellX*4+4;++x) if (const auto sid=hash_.Cell(0,x,y)) {
            const auto raw=pool_.Slot({sid}); // 실제 지면 WORD가 지시한 현재50BYTE 슬롯이다.
            if (raw[10]==state_.isleType || raw[10]==state_.noIslandType)
                Pixel(bitmap,x/2,y/2,OwnerColor(raw[34],raw[10]==state_.isleType ? state_.neutralIsle : state_.neutralNoIsland));
        }
    }
    Chain(bitmap,hash_.Cell(2,cellX,cellY),false);
    // 해시1은 X 바깥/Y 안쪽의2×2 버킷 순서다.
    for (auto x=px;x<px+2;++x) {
        // 현재 버킷의 체인을 끝낸 뒤 다음 Y로 이동한다.
        for (auto y=py;y<py+2;++y) Chain(bitmap,hash_.Cell(1,x,y),true);
    }
    // 해시0은 X 바깥/Y 안쪽의4×4 버킷 순서다. 지면 전용 순서와 다르다.
    for (auto x=cellX*4;x<cellX*4+4;++x) {
        // 이미 그린 지면도 원본과 같은 일반 flags 검사를 거친다.
        for (auto y=cellY*4;y<cellY*4+4;++y) Chain(bitmap,hash_.Cell(0,x,y),true);
    }
}
}
