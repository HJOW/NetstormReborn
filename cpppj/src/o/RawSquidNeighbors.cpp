// 연결 이웃: 일반 탐색의 동적 next/버킷 순서에 원본 파생 필터를 연결한다.
#include "o/RawSquidNeighbors.h"
#include "o/Bridge.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 두 판본의 공통 raw 타입·상태·좌표 위치와 dead/abstract/섬 내부 비트다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18;
constexpr std::uint8_t kDead=2,kAbstract=1,kInterior=8;
// 원본 발자국 보정이 쓰는 지도 좌표 양 끝이다.
constexpr float kFirstCell=1.0f,kLastCell=255.0f;
// 00500edc ↔ CD 00507484: 후보 기준점의 spot 칸을 고를 때 더하는 값이다(더한 뒤 0 방향으로 자른다).
constexpr float kSpotBias=0.9999f;
// 정렬되지 않은 little endian 필드를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 필드 폭 안에서만 낮은 바이트부터 합친다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// x87 덧셈 뒤 _ftol과 같이 중간값을 float로 좁히지 않고 0 방향으로 자른 칸 번호다. 좌표는 ReadObject가 지도 안으로 확인했다.
int BiasedCell(float value) { return static_cast<int>(static_cast<double>(value)+static_cast<double>(kSpotBias)); }
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
// 다른 계층이 같은 풀을 쓰는지 확인할 수 있게 한다.
const SidPool& RawSquidNeighbors::Pool() const { return pool_; }
// 일반 사각형 탐색을 같은 해시에 연결할 때 쓴다.
const SquidHash& RawSquidNeighbors::Hash() const { return hash_; }
// 복사해 둔 타입 표다. 이 객체가 살아 있는 동안 유효하다.
std::span<const RiftTypeRecord> RawSquidNeighbors::Types() const { return types_; }
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
// 발자국만 필요한 호출자에게 사각형을 그대로 준다.
SquidFootprint RawSquidNeighbors::Footprint(Sid sid) const {
    const auto object=ReadObject(sid,false);
    return {object.left,object.top,object.right,object.bottom};
}
// 중심은 x87에서 계산한 뒤 단정도로 저장한 값이다.
std::array<float,2> RawSquidNeighbors::Center(Sid sid) const {
    const auto object=ReadObject(sid,false);
    return {object.centerX,object.centerY};
}
// 0041ce90의 flag 1 분기. 가로 차이는 스택에 남은 값(두 단정도의 차이라 배정도에서 정확하다)이고
// 세로 차이는 단정도로 저장했다가 다시 읽는다. |세로| < |가로|가 아니면(같음·NaN 포함) 세로 방향이다.
int RawSquidNeighbors::Direction(float fromX,float fromY,float toX,float toY) {
    const double dx=static_cast<double>(toX)-static_cast<double>(fromX);
    const float dy=static_cast<float>(static_cast<double>(toY)-static_cast<double>(fromY));
    if (!(std::abs(static_cast<double>(dy))<std::abs(dx))) return dy>0 ? 4 : 0;
    return dx>0 ? 2 : 6;
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
// 004b1e80 ↔ CD 004ec040: 일반 finder가 이미 buried·정수 사각형 교차를 검사한 후보만 받는다.
bool RawSquidNeighbors::Accept(Sid candidate,const Object& source,std::uint32_t flags) const {
    const auto raw=pool_.Slot(candidate); const auto number=raw[kType];
    if ((types_.at(number).flags1&TypeFlag1::kSurface)==0 && (flags&NeighborFlag::kAnyType)==0) return false;
    const auto object=ReadObject(candidate,false);
    // 0041d9b0 ↔ CD 0043fe10: 후보 사각형을 축별로 한 칸 넓혀 source와 교차하며 정확히 한 축만 겹쳐야 한다.
    const auto intersects=[&source](float left,float top,float right,float bottom) {
        const float x1=std::max(source.left,left),y1=std::max(source.top,top);
        const float x2=std::min(source.right,right),y2=std::min(source.bottom,bottom);
        return x1>=kFirstCell && y1>=kFirstCell && x2<kWorldCells && y2<kWorldCells && x1<=x2 && y1<=y2;
    };
    if (intersects(object.left-1.0f,object.top,object.right+1.0f,object.bottom)==
        intersects(object.left,object.top-1.0f,object.right,object.bottom+1.0f)) return false;
    if ((flags&NeighborFlag::kNoJoint)==0) {
        const auto framed=ReadObject(candidate,true);
        // 후보 중심에서 source 중심을 보는 방향으로 두 프레임의 접합을 검사한다.
        const int direction=Direction(framed.centerX,framed.centerY,source.centerX,source.centerY);
        if (!Bridge::Connects(framed.frame,types_[number].flags2,source.frame,types_[source.type].flags2,direction)) return false;
    }
    if (raw[kState]&kDead) return false;
    if ((flags&NeighborFlag::kSkipAbstract)!=0 && (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kAbstract)!=0) return false;
    // 후보 기준점의 spot은 좌표에 0.9999를 더해 자른 칸에서 읽는다. 소수 좌표의 객체는 다음 칸을 본다.
    // 원본은 y*256+x의 선형 주소를 읽으므로 x가 256이 되면 다음 줄의 첫 칸이다. 배열 밖만 거부한다.
    const auto index=static_cast<std::size_t>(BiasedCell(object.bottom)*kWorldCells+BiasedCell(object.right));
    if (index>=spots_.size()) throw std::out_of_range("raw 이웃 spot 칸 범위 오류");
    return (spots_[index]&kInterior)==0;
}
// 첫 반환 뒤의 버킷/슬롯은 읽지 않는다. 별도 일반 finder를 사용하므로 다른 삭제 탐색 커서를 덮지 않는다.
Sid RawSquidNeighbors::First(Sid source) const {
    return RawSquidNeighborWalk(*this,source,0).Current();
}
// 004b23e0 → 004b1d70: source 발자국이 섬 내부이면 탐색하지 않고, 아니면 한 칸 넓힌 정수 사각형으로 일반 탐색을 시작한다.
RawSquidNeighborWalk::RawSquidNeighborWalk(const RawSquidNeighbors& neighbors,Sid source,std::uint32_t flags)
    :finder_(neighbors.pool_,neighbors.hash_,neighbors.types_) {
    if ((flags&~NeighborFlag::kSupported)!=0) throw std::invalid_argument("raw 이웃 flags(표면 지도 순회는 미지원)");
    const auto object=neighbors.ReadObject(source,true);
    if (neighbors.Interior(object)) return;
    // 필터는 생성 때 읽은 source 값과 flags를 값으로 잡는다. 이 객체를 옮겨도 유효하다.
    const auto* owner=&neighbors;
    current_=finder_.Begin({static_cast<int>(object.left-1.0f),static_cast<int>(object.top-1.0f),
        static_cast<int>(object.right+1.0f),static_cast<int>(object.bottom+1.0f)},0,
        [owner,object,flags](Sid candidate) { return owner->Accept(candidate,object,flags); });
}
// 현재 번호를 읽기만 한다.
Sid RawSquidNeighborWalk::Current() const { return current_; }
// 끝난 뒤(또는 내부 발자국이라 시작하지 않은 경우)에는 일반 탐색을 다시 부르지 않는다.
Sid RawSquidNeighborWalk::Next() {
    if (current_.value!=0) current_=finder_.Next();
    return current_;
}
}
