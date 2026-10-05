#include "o/SquidSpatial.h"
#include "o/Squid.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// DAT_005424b8/CD 005395cc의 실제 건물군 마스크다.
constexpr std::uint32_t kBuildingMask=0x50444200;
// 원본 spot 쓰기 대상에는 건물군 외에 이 genus 비트들도 포함된다.
constexpr std::uint32_t kSpotMask=kBuildingMask|0x400ff;
// 타입 flags1에서 표면임을 뜻한다.
constexpr std::uint32_t kSurface=0x800;
// 상태 바이트의 해제/포함/전송 비트다. dead=2는 Unpop을 막지 않는다.
constexpr std::uint8_t kFree=1,kVoid=4,kContained=8,kTransmitting=16;
// 부가 바이트에서 매몰과 첫 등록 완료를 뜻한다. 상태 바이트의 bit 8과 구분한다.
constexpr std::uint8_t kBuried=8,kFirstPopped=0x80;
// 원본 SetIsland의 잘못된 섬 번호 표식은 ushort 최대값이 아니라 127이다.
constexpr std::uint16_t kInvalidIsland=127;
// 원본 화면 좌표 캐시의 축별 배율과 반올림 오프셋이다.
constexpr double kScreenScaleX=16,kScreenScaleY=11,kScreenBias=0.5;
// 음수가 아닌 short 번호로 격리된 입력 객체를 식별한다. 실제 SID 세대 형식은 후속이다.
constexpr std::uint16_t kMaxInputId=32767;
}

// 256×256 바이트를 소유하며 빈 지도 또는 정확한 크기의 초기 지도만 받는다.
SquidSpatial::SquidSpatial(SpatialEdition edition,SpatialDependencies dependencies,std::span<const std::uint8_t> initialSpots)
    :edition_(edition),dependencies_(std::move(dependencies)),spots_(kWorldCells*kWorldCells,0) {
    if (!initialSpots.empty()) {
        if (initialSpots.size()!=spots_.size()) throw std::invalid_argument("Spatial spot dimensions");
        std::copy(initialSpots.begin(),initialSpots.end(),spots_.begin());
    }
}
// 등록 입력은 할당된 void 상태만 받는다. Add 자체가 원본 메모리 풀 할당을 복원하지는 않는다.
void SquidSpatial::Add(SpatialObject object) {
    if (!object.id || object.id>kMaxInputId || objects_.contains(object.id) ||
        (object.state & (kFree|kVoid|kContained|kTransmitting))!=kVoid || object.next || object.level ||
        object.width<1 || object.height<1 || object.width>kWorldCells || object.height>kWorldCells ||
        !std::isfinite(object.frameWidth) || !std::isfinite(object.frameHeight) || object.frameWidth<0 || object.frameHeight<0)
        throw std::invalid_argument("Spatial allocated void object");
    objects_.emplace(object.id,std::move(object));
}
// 보존한 객체 번호로 공간 필드를 읽는다.
const SpatialObject& SquidSpatial::Object(std::uint16_t id) const { return objects_.at(id); }
// 수명 전이는 클래스 내부에서만 수행한다.
SpatialObject& SquidSpatial::Mutable(std::uint16_t id) { return objects_.at(id); }
// 읽기 전용 해시로 표면 머리와 다른 단계의 체인을 확인한다.
const SquidHash& SquidSpatial::Hash() const { return hash_; }
// 실제 OR/AND 결과를 외부 탐색기에 전달한다.
std::span<const std::uint8_t> SquidSpatial::Spots() const { return spots_; }
// 원본 타입 선택식과 buried/contained 필터를 그대로 보존한다.
bool SquidSpatial::WritesSpots(const SpatialObject& object) {
    return ((object.flags2 & kSpotMask)!=0 || (object.flags1 & kSurface)!=0) &&
        (object.extra & kBuried)==0 && (object.state & kContained)==0;
}
// 원본은 잘못된 유한 좌표를 (10,10)으로 바꾸고 화면 캐시를 별도 배율로 계산한다.
void SquidSpatial::Coordinates(SpatialObject& object,float x,float y) {
    if (!std::isfinite(x) || !std::isfinite(y)) throw std::invalid_argument("Spatial finite coordinates");
    if (x<=0 || y<=0 || x>=kWorldCells || y>=kWorldCells) x=y=10;
    object.x=x; object.y=y;
    object.screenX=static_cast<std::int16_t>(static_cast<double>(x)*kScreenScaleX+kScreenBias);
    object.screenY=static_cast<std::int16_t>(static_cast<double>(y)*kScreenScaleY+kScreenBias);
}
// Pop/Unpop은 float→int→+0.9999→int 순서다. Genus 내부의 float+0.9999와 구분한다.
SquidSpatial::Bounds SquidSpatial::Footprint(const SpatialObject& object) {
    const int right=static_cast<int>(object.x),bottom=static_cast<int>(object.y);
    const Bounds bounds{right-object.width+1,bottom-object.height+1,right,bottom};
    if (bounds.left<1 || bounds.top<1 || bounds.right>=kWorldCells || bounds.bottom>=kWorldCells)
        throw std::out_of_range("Spatial footprint on board");
    return bounds;
}
// 좌우 변의 중간 칸에 있는 0단계 머리만 읽는다. 전체 발자국을 SID로 채우지 않는다.
void SquidSpatial::AttachedSurface(const SpatialObject& object,int x,int y,bool pop,SpatialChange& change) {
    if ((object.flags2 & kBuildingMask)==0) return;
    const auto bounds=Footprint(object);
    if ((x!=bounds.left && x!=bounds.right) || y==bounds.top || y==bounds.bottom) return;
    const auto id=hash_.Cell(0,x,y);
    const auto found=objects_.find(id);
    if (found==objects_.end()) return;
    auto& surface=found->second;
    if (pop) {
        if ((surface.flags1 & kSurface)==0) return;
        if (!dependencies_.regionSupported) throw std::logic_error("Spatial region dependency missing");
        const bool supported=dependencies_.regionSupported(object.x,object.y);
        change.events.push_back({SpatialEventKind::RegionQuery,static_cast<std::uint32_t>(supported)});
        surface.surfaceWord=supported ? static_cast<std::uint16_t>((surface.surfaceWord & 0xfffb)|2) :
            static_cast<std::uint16_t>(surface.surfaceWord & 0xfff9);
    } else {
        surface.surfaceWord=static_cast<std::uint16_t>(surface.surfaceWord & 0xfff9);
    }
    change.events.push_back({SpatialEventKind::SurfaceChanged,id});
}
// 원본 공간 갱신과 비전투 활성화만 옮긴다. 사건 목록은 아직 미복원 효과의 호출 계약이다.
SpatialChange SquidSpatial::Pop(std::uint16_t id,float x,float y,std::uint32_t flags) {
    auto& object=Mutable(id); SpatialChange change;
    if ((object.state & kVoid)==0 || (object.state & kContained)!=0) {
        if (edition_==SpatialEdition::CD1072) throw std::logic_error("CD Pop requires void object");
        return change;
    }
    // 원본의 잘못된 발자국 assert 이후 배열 밖 쓰기는 새 코드에서는 사전에 거부한다.
    auto positioned=object; Coordinates(positioned,x,y);
    if (WritesSpots(positioned)) static_cast<void>(Footprint(positioned));
    object=positioned;
    if (WritesSpots(object)) {
        const auto bounds=Footprint(object);
        // 행 우선 처리이므로 overlap 조기 반환 때 앞 칸의 OR와 표면 갱신은 이미 남는다.
        for (int cy=bounds.top;cy<=bounds.bottom;++cy) {
            // 오른쪽 아래 기준점까지 각 칸을 갱신한다.
            for (int cx=bounds.left;cx<=bounds.right;++cx) {
                const auto genus=static_cast<std::uint8_t>(Squid::EffectiveGenus(object.flags2,object.x,object.y,object.width,object.height,cx,cy));
                auto& spot=spots_[static_cast<std::size_t>(cy*kWorldCells+cx)];
                if (genus) {
                    if (edition_==SpatialEdition::Patch1078 && (spot & genus)!=0) { change.result=SpatialResult::Overlap; return change; }
                    spot=static_cast<std::uint8_t>(spot|genus);
                }
                AttachedSurface(object,cx,cy,true,change);
            }
        }
    }
    object.level=static_cast<std::uint8_t>(SquidHash::ObjectLevel(object.flags2,object.frameWidth,object.frameHeight));
    auto& head=hash_.Bucket(object.level,object.x,object.y);
    if (head==id) throw std::logic_error("Spatial next would point to itself");
    object.next=head; head=id;
    if (object.level==0 && (object.flags2 & 4)==0) object.island=kInvalidIsland;
    change.events.push_back({(flags & 0x2000)!=0 ? SpatialEventKind::Update8c : SpatialEventKind::Update88,0});
    if ((object.extra & kFirstPopped)==0) {
        const auto first=dependencies_.firstPopFlags ? dependencies_.firstPopFlags(id) : 0;
        change.events.push_back({SpatialEventKind::FirstPop,first}); flags|=first;
        object.extra=static_cast<std::uint8_t>(object.extra|kFirstPopped);
    }
    object.state=static_cast<std::uint8_t>(object.state & ~kVoid);
    if ((flags & 8)==0) flags|=0x10;
    change.events.push_back({SpatialEventKind::PostPop,flags});
    change.result=SpatialResult::Registered;
    return change;
}
// 원본은 제거 후 next를 0으로 지우지 않고 머리 또는 이전 객체의 next만 연결한다.
SpatialChange SquidSpatial::Unpop(std::uint16_t id,std::uint32_t flags) {
    auto& object=Mutable(id); SpatialChange change;
    if ((object.state & kContained)!=0) throw std::logic_error("Spatial Unpop of contained object");
    if ((object.state & (kFree|kVoid))!=0) return change;
    if (WritesSpots(object)) static_cast<void>(Footprint(object));
    object.state=static_cast<std::uint8_t>(object.state|kVoid);
    if (WritesSpots(object)) {
        const auto bounds=Footprint(object);
        // 원본과 같은 행/열 순서로 spot을 AND 해제한다. 겹친 다른 객체의 bit도 지울 수 있다.
        for (int cy=bounds.top;cy<=bounds.bottom;++cy) {
            // 실제 genus의 low byte만 지운다.
            for (int cx=bounds.left;cx<=bounds.right;++cx) {
                const auto genus=static_cast<std::uint8_t>(Squid::EffectiveGenus(object.flags2,object.x,object.y,object.width,object.height,cx,cy));
                auto& spot=spots_[static_cast<std::size_t>(cy*kWorldCells+cx)];
                spot=static_cast<std::uint8_t>(spot & ~genus);
                AttachedSurface(object,cx,cy,false,change);
            }
        }
    }
    bool removed=false;
    // 원본은 저장한 level 바이트를 신뢰하지 않고 0~3단계를 순서대로 검색한다.
    for (int level=0;level<4 && !removed;++level) {
        auto& head=hash_.Bucket(level,object.x,object.y);
        std::uint16_t previous=0,current=head; std::size_t visited=0;
        // 새 입력 보호: 손상된 체인이 무한 순환하거나 없는 SID를 참조하면 실패한다.
        while (current) {
            if (++visited>objects_.size()) throw std::logic_error("Spatial cyclic chain");
            const auto next=Object(current).next;
            if (current==id) {
                if (!previous) head=next; else Mutable(previous).next=next;
                removed=true; break;
            }
            previous=current; current=next;
        }
    }
    if (!removed && edition_==SpatialEdition::Patch1078) throw std::logic_error("Spatial object missing from hash");
    change.events.push_back({(flags & 0x2000)!=0 ? SpatialEventKind::Update8c : SpatialEventKind::Update88,0});
    change.result=SpatialResult::Removed;
    return change;
}
// 원본 정수 표면 조회 범위에서 실제 체인 머리와 spot을 함께 전달한다.
SurfaceFinder SquidSpatial::Surfaces() const {
    std::vector<SurfaceObject> surfaces;
    // void/free 객체는 지도에 없으며 dead는 기존 탐색기의 필터 입력으로 남긴다.
    for (const auto& [id,object]:objects_) {
        if ((object.state & (kFree|kVoid))!=0) continue;
        if (std::trunc(object.x)!=object.x || std::trunc(object.y)!=object.y)
            throw std::logic_error("Surface snapshot requires integer anchors");
        surfaces.push_back({id,static_cast<int>(object.x),static_cast<int>(object.y),object.width,object.height,
            object.flags1,object.flags2,object.frame,(object.state & 2)!=0,(object.extra & kBuried)!=0});
    }
    return SurfaceFinder(surfaces,hash_.Entries(0),spots_);
}
}
