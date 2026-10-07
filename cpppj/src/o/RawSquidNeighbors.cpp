// flag 0 연결 이웃: 일반 탐색의 동적 next/버킷 순서에 원본 파생 필터를 연결한다.
#include "o/RawSquidNeighbors.h"
#include "o/Bridge.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 두 판본의 공통 raw 타입·상태·좌표 위치와 dead/섬 내부 비트다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18;
constexpr std::uint8_t kDead=2,kInterior=8;
// 원본 발자국 보정이 쓰는 지도 좌표 양 끝이다.
constexpr float kFirstCell=1.0f,kLastCell=255.0f;
// 정렬되지 않은 little endian 필드를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 필드 폭 안에서만 낮은 바이트부터 합친다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
}
// 손상된 표/지도 연결은 조회를 시작하기 전에 거부한다.
RawSquidNeighbors::RawSquidNeighbors(const SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
    std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames)
    :pool_(pool),hash_(hash),spots_(spots),types_(types.begin(),types.end()),frames_(frames.begin(),frames.end()) {
    if (spots.size()!=kWorldCells*kWorldCells || types.empty() || types.size()!=frames.size())
        throw std::invalid_argument("raw 이웃 타입/프레임/지도 크기 오류");
}
// 원본 타입 조회의 범위 밖 주소 산술을 예외로 바꾼다.
const RiftTypeFrames& RawSquidNeighbors::Frames(std::uint32_t type) const {
    return frames_.at(type);
}
// 004ade70/0049ae80의 발자국과 004adea0의 중심을 저장한 float 정밀도로 읽는다.
RawSquidNeighbors::Object RawSquidNeighbors::ReadObject(Sid sid,bool readFrame) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("raw 이웃 SID 오류");
    const auto raw=pool_.Slot(sid); const auto number=raw[kType];
    if (number<kFirstAssetTypeNumber || number>=types_.size()) throw std::out_of_range("raw 이웃 자산 타입 오류");
    const auto& type=types_[number];
    const float x=std::bit_cast<float>(Read(raw,kX)),y=std::bit_cast<float>(Read(raw,kY));
    if (!std::isfinite(x) || !std::isfinite(y) || x<kFirstCell || y<kFirstCell || x>=kWorldCells || y>=kWorldCells ||
        type.footX<1 || type.footY<1 || type.footX>kWorldCells || type.footY>kWorldCells)
        throw std::out_of_range("raw 이웃 좌표/발자국 오류");
    Object object{std::clamp(x-static_cast<float>(type.footX-1),kFirstCell,kLastCell),
        std::clamp(y-static_cast<float>(type.footY-1),kFirstCell,kLastCell),x,y,
        x-static_cast<float>(type.footX)*0.5f,y-static_cast<float>(type.footY)*0.5f,number,{}};
    if (readFrame) {
        const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
        const auto frame=Read(raw,patch ? 36 : 34,patch ? 4 : 1);
        const auto codes=Frames(number).Codes();
        if (frame>=codes.size()) throw std::out_of_range("raw 이웃 프레임 오류");
        object.frame=codes[frame];
    }
    return object;
}
// 004ab880 ↔ CD 00487810: 발자국 전체의 spot 비트를 AND한다.
bool RawSquidNeighbors::Interior(const Object& source) const {
    std::uint8_t bits=0xff;
    // 정수 절삭 후 y/x 순서로 발자국을 읽는다.
    for (int y=static_cast<int>(source.top);y<=static_cast<int>(source.bottom);++y)
        // 오른쪽 아래 값이 255.x여도 칸 번호는 255다.
        for (int x=static_cast<int>(source.left);x<=static_cast<int>(source.right);++x)
            bits=static_cast<std::uint8_t>(bits&spots_[static_cast<std::size_t>(y*kWorldCells+x)]);
    return (bits&kInterior)!=0;
}
// 004b1e80(flag 0): 일반 finder가 이미 buried·정수 사각형 교차를 검사한 후보만 받는다.
bool RawSquidNeighbors::Accept(Sid candidate,const Object& source) const {
    const auto raw=pool_.Slot(candidate); const auto number=raw[kType];
    if ((types_.at(number).flags1&TypeFlag1::kSurface)==0) return false;
    const auto object=ReadObject(candidate,false);
    // 0041d9b0 ↔ CD 0043fe10: 후보 사각형을 축별로 한 칸 넓혀 source와 교차하며 정확히 한 축만 겹쳐야 한다.
    const auto intersects=[&source](float left,float top,float right,float bottom) {
        const float x1=std::max(source.left,left),y1=std::max(source.top,top);
        const float x2=std::min(source.right,right),y2=std::min(source.bottom,bottom);
        return x1>=kFirstCell && y1>=kFirstCell && x2<kWorldCells && y2<kWorldCells && x1<=x2 && y1<=y2;
    };
    if (intersects(object.left-1.0f,object.top,object.right+1.0f,object.bottom)==
        intersects(object.left,object.top-1.0f,object.right,object.bottom+1.0f)) return false;
    const auto framed=ReadObject(candidate,true);
    // 0041ce90는 두 저장 중심 float의 차이를 다시 float로 저장한다. 동률은 세로 방향이다.
    const float dx=source.centerX-framed.centerX,dy=source.centerY-framed.centerY;
    const int direction=std::abs(dx)<=std::abs(dy) ? (dy>0 ? 4 : 0) : (dx<=0 ? 6 : 2);
    if (!Bridge::Connects(framed.frame,types_[number].flags2,source.frame,types_[source.type].flags2,direction)) return false;
    return !(raw[kState]&kDead) && !(spots_[static_cast<std::size_t>(static_cast<int>(object.bottom)*kWorldCells+
        static_cast<int>(object.right))]&kInterior);
}
// 첫 반환 뒤의 버킷/슬롯은 읽지 않는다. 별도 일반 finder를 사용하므로 다른 삭제 탐색 커서를 덮지 않는다.
Sid RawSquidNeighbors::First(Sid source) const {
    const auto object=ReadObject(source,true);
    if (Interior(object)) return {};
    RawSquidFinder finder(pool_,hash_,types_);
    return finder.Begin({static_cast<int>(object.left-1.0f),static_cast<int>(object.top-1.0f),
        static_cast<int>(object.right+1.0f),static_cast<int>(object.bottom+1.0f)},0,
        [this,object](Sid candidate) { return Accept(candidate,object); });
}
}
