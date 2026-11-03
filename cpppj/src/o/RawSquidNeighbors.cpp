// 연결 이웃: 일반 탐색의 동적 next/버킷 순서에 원본 파생 필터를 연결한다.
#include "o/RawSquidNeighbors.h"
#include "o/Bridge.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>

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
RawSquidNeighbors::Object RawSquidNeighbors::ReadObject(Sid sid,bool readFrame) const { return ReadObject(Source(sid),readFrame,false); }
// 실제 슬롯의 정보만 포착하며 프레임 코드나 후보를 아직 읽지 않는다.
SquidNeighborSource RawSquidNeighbors::Source(Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("raw 이웃 SID 오류");
    const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    return {std::bit_cast<float>(Read(raw,kX)),std::bit_cast<float>(Read(raw,kY)),raw[kType],Read(raw,patch ? 36 : 34,patch ? 4 : 1)};
}
// 값 원천의 보정 발자국/중심을 만들고 필요한 프레임만 판독한다. 후보의 같은 타입 자료는 바꾸지 않는다.
RawSquidNeighbors::Object RawSquidNeighbors::ReadObject(const SquidNeighborSource& source,bool readFrame,bool lazyFrame) const {
    const auto number=source.type;
    if (number<kFirstAssetTypeNumber || number>=types_.size()) throw std::out_of_range("raw 이웃 자산 타입 오류");
    const auto& type=types_[number];const float x=source.x,y=source.y;
    if (!std::isfinite(x) || !std::isfinite(y) ||
        type.footX<1 || type.footY<1 || type.footX>kWorldCells || type.footY>kWorldCells)
        throw std::out_of_range("raw 이웃 좌표/발자국 오류 TYPE="+std::to_string(number)+" x="+std::to_string(x)+" y="+std::to_string(y)+" foot="+std::to_string(type.footX)+","+std::to_string(type.footY));
    // 실제49ae80의 모서리 확장은 원래 두 좌표가 모두(0,256) 안일 때만 이전 점을 보존한다.
    const float left=std::clamp(x-static_cast<float>(type.footX-1),kFirstCell,kLastCell),top=std::clamp(y-static_cast<float>(type.footY-1),kFirstCell,kLastCell);
    const bool valid=x>0 && x<kWorldCells && y>0 && y<kWorldCells; // 초기(0,0) 미리보기는 보정된 한 점이 된다.
    Object object{valid ? std::min(x,left) : left,valid ? std::min(y,top) : top,valid ? std::max(x,left) : left,valid ? std::max(y,top) : top,
        x-static_cast<float>(type.footX)*0.5f,y-static_cast<float>(type.footY)*0.5f,number,{}};
    if (readFrame) {
        const auto codes=Frames(number).Codes();
        if (source.frame>=codes.size()) {
            if (!lazyFrame) throw std::out_of_range("raw 이웃 프레임 오류");
        } else object.frame=codes[source.frame];
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
    const int left=std::clamp(static_cast<int>(source.left),1,255),right=std::clamp(static_cast<int>(source.right),1,255),top=std::clamp(static_cast<int>(source.top),1,255),bottom=std::clamp(static_cast<int>(source.bottom),1,255); // 원본4ab880은 네 정수 모서리를 각각 보정한다.
    // 정수 절삭/보정 후 원본 y/x 순서로 발자국을 읽는다.
    for (int y=top;y<=bottom;++y)
        // 오른쪽 아래 값이255.x여도 실제 spot 칸은255다.
        for (int x=left;x<=right;++x)
            bits=static_cast<std::uint8_t>(bits&spots_[static_cast<std::size_t>(y*kWorldCells+x)]);
    return (bits&kInterior)!=0;
}
// 004b1e80 ↔ CD 004ec040: 일반 finder의 기하 교차와 flags8 지도 후보를 구별하고 현재 프레임/상태/spot을 검사한다.
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
    if ((flags&NeighborFlag::kSurfaceMap)==0 &&
        intersects(object.left-1.0f,object.top,object.right+1.0f,object.bottom)==
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
    // 후보의 실제 좌표를 사용한다. (0,y)/(x,0)의 보정된 발자국 모서리로 바꾸면 원문의 내부 spot 필터가 달라진다.
    const auto index=static_cast<std::size_t>(BiasedCell(std::bit_cast<float>(Read(raw,kY)))*kWorldCells+
        BiasedCell(std::bit_cast<float>(Read(raw,kX))));
    if (index>=spots_.size()) throw std::out_of_range("raw 이웃 spot 칸 범위 오류");
    return (spots_[index]&kInterior)==0;
}
// 첫 반환 뒤의 버킷/슬롯은 읽지 않는다. 별도 일반 finder를 사용하므로 다른 삭제 탐색 커서를 덮지 않는다.
Sid RawSquidNeighbors::First(Sid source) const {
    return RawSquidNeighborWalk(*this,source,0).Current();
}
// 004b23e0 → 004b1d70: 원천을 한 번 보관하고 내부 발자국이면 끝낸다. flags8은 현재 지도, 나머지는 현재 일반 해시에서 찾는다.
RawSquidNeighborWalk::RawSquidNeighborWalk(const RawSquidNeighbors& neighbors,Sid source,std::uint32_t flags)
    :RawSquidNeighborWalk(neighbors,neighbors.Source(source),flags) {}
// 삭제 값 인자와 일반 SID 생성자가 같은 현재 후보 필터/커서를 사용한다.
RawSquidNeighborWalk::RawSquidNeighborWalk(const RawSquidNeighbors& neighbors,const SquidNeighborSource& source,std::uint32_t flags,bool lazyFrame)
    :finder_(neighbors.pool_,neighbors.hash_,neighbors.types_) {
    if ((flags&~NeighborFlag::kSupported)!=0) throw std::invalid_argument("raw 이웃 flags 범위 오류");
    const auto object=neighbors.ReadObject(source,true,lazyFrame);
    if (neighbors.Interior(object)) return;
    // 필터는 생성 때 읽은 source 값과 flags를 값으로 잡는다. 이 객체를 옮겨도 유효하다.
    const auto* owner=&neighbors;
    if ((flags&NeighborFlag::kSurfaceMap)!=0) {
        // 원문 _ftol로 저장하는 확장 사각형의 정수 네 모서리다. 지도 밖 셀은 Next에서0으로 취급한다.
        const int left=static_cast<int>(object.left-1.0f),top=static_cast<int>(object.top-1.0f);
        const int right=static_cast<int>(object.right+1.0f),bottom=static_cast<int>(object.bottom+1.0f);
        // 원천/flags/사각형은 생성 때 보관한다. 커서와 SHORT 캐시는 각 Walk가 독립적으로 소유한다.
        surfaceNext_=[owner,object,flags,left,top,right,bottom,x=left,y=top,seen=std::vector<std::int16_t>{}]() mutable -> Sid {
            // 원문004b1c20의 선증가와 모서리 생략을 유지하며 현재 셀/후보를 매 Next마다 읽는다.
            for (;;) {
                ++x;
                if (y==top || y==bottom) {
                    if (x>=right) { ++y;if (y>bottom) return {};x=left; }
                } else if (x>right) { ++y;x=left+(y==bottom ? 1 : 0); }
                if (x<0 || y<0 || x>=kWorldCells || y>=kWorldCells) continue;
                const Sid candidate{owner->hash_.Cell(0,x,y)};if (!candidate.value) continue;
                const auto raw=owner->pool_.Slot(candidate);
                const float cx=std::bit_cast<float>(Read(raw,kX)),cy=std::bit_cast<float>(Read(raw,kY));
                if (object.left<=cx && cx<=object.right && object.top<=cy && cy<=object.bottom) continue;
                if (!owner->Accept(candidate,object,flags)) continue;
                // 원문 캐시는 signed short를 int로 확장해 DWORD SID와 비교한다. 별도 자기 번호 제외는 없다.
                if (std::any_of(seen.begin(),seen.end(),[candidate](auto prior) { return static_cast<std::int32_t>(prior)==candidate.value; })) continue;
                seen.push_back(std::bit_cast<std::int16_t>(candidate.value));return candidate;
            }
        };
        current_=surfaceNext_();return;
    }
    current_=finder_.Begin({static_cast<int>(object.left-1.0f),static_cast<int>(object.top-1.0f),
        static_cast<int>(object.right+1.0f),static_cast<int>(object.bottom+1.0f)},0,
        [owner,object,flags](Sid candidate) { return owner->Accept(candidate,object,flags); });
}
// 현재 번호를 읽기만 한다.
Sid RawSquidNeighborWalk::Current() const { return current_; }
// 끝난 뒤(또는 내부 발자국이라 시작하지 않은 경우)에는 일반 탐색을 다시 부르지 않는다.
Sid RawSquidNeighborWalk::Next() {
    if (current_.value!=0) current_=surfaceNext_ ? surfaceNext_() : finder_.Next();
    return current_;
}
}
