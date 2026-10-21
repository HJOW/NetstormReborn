// float 발자국·판본별 탐색 거리·관계 판정·섬 정리 순서를 실제 원본 관찰대로 보존한다.
#include "o/RawConstructionEffects.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 큰 모서리의 원본 거의 올림 상수와 float 발자국 축소 조건/거리다.
constexpr double kRoundUpBias=0.9999899864196777;
constexpr float kShrinkWidth=3.0F,kShrink=1.0F;
// 섬 정리 메시지의 좌표 배율(00501ad0 / CD 005046b8)이다. 소수부를 보존하는 16.16 DWORD로 전송한다.
constexpr double kWireScale=65536.0;
// 이동 대상 genus 마스크와 가운데 우선 탐색/적대 이동 허용 비트다.
constexpr std::uint32_t kPatchDisplace=0x218000,kCdDisplace=0x210000,kCenterFirst=0x200000,kEnemyAllowed=0x8000;
// 섬 삭제에서 추가하는 bit 3/6과 추락 표시/소리 억제, 전파 억제, 후속 알림 비트다.
constexpr std::uint32_t kPieceFlags=0x48,kQuietFall=0x1000,kNoBroadcast=0x50,kNotifyGenus=0x10000;
// raw 타입/좌표와 판본별 소유자 위치다.
constexpr std::size_t kType=10,kX=14,kY=18,kPatchOwner=34,kCdOwner=32;
// 원본 __ftol의 하위 signed DWORD를 반환한다. 비유한/64비트 범위 밖은 하위 DWORD가 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 정렬되지 않은 raw float 좌표를 little endian으로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 낮은 바이트부터 DWORD로 합친다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(value);
}
// 원본 발자국 helper의 float 사각형이다. 시작점이 지도 밖이면 자른 반대 점 하나로 축소한다.
std::array<float,4> Footprint(float x,float y,int width,int height) {
    const float px=std::clamp(static_cast<float>(static_cast<double>(x)+1-width),1.0F,255.0F);
    const float py=std::clamp(static_cast<float>(static_cast<double>(y)+1-height),1.0F,255.0F);
    if (!(x>0 && y>0 && x<256 && y<256)) return {px,py,px,py};
    return {std::min(x,px),std::min(y,py),std::max(x,px),std::max(y,py)};
}
// finder에 전달할 모서리를 작은 쪽 절삭/큰 쪽 거의 올림으로 정수화한다. 지도 범위 보정은 finder가 수행한다.
SquidSearchArea Area(const std::array<float,4>& rectangle) {
    return {Ftol(rectangle[0]),Ftol(rectangle[1]),Ftol(static_cast<double>(rectangle[2])+kRoundUpBias),
        Ftol(static_cast<double>(rectangle[3])+kRoundUpBias)};
}
// 유한 좌표와 비음수 발자국을 검증한다. 원본49ae80은 Dude의 foot0도 x..x+1 사각형으로 펼친다.
void Validate(float x,float y,const RiftTypeRecord& type) {
    if (!std::isfinite(x) || !std::isfinite(y) || type.footX<0 || type.footY<0 || type.footX>256 || type.footY>256)
        throw std::out_of_range("건설 효과 좌표/발자국 오류");
}
}
RawConstructionEffects::RawConstructionEffects(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> spots,const ConstructionEffectsState& state,ConstructionEffectsHooks hooks)
    :pool_(pool),hash_(hash),types_(types),spots_(spots),state_(state),hooks_(std::move(hooks)) {
    if (spots.size()<RawConstructionRequest::kSpotCount || !hooks_.pathType || !hooks_.clearPath || !hooks_.unpop || !hooks_.pop ||
        !hooks_.fall || !hooks_.sound || !hooks_.destroy || !hooks_.notify || !hooks_.broadcast)
        throw std::invalid_argument("건설 효과 자료/경계 연결 오류");
}
std::uint32_t RawConstructionEffects::Genus(Sid sid) const {
    const auto type=pool_.Slot(sid)[kType];
    if (type>=types_.size()) throw std::out_of_range("건설 효과 후보 타입 오류");
    return types_[type].flags2;
}
bool RawConstructionEffects::Open(std::int64_t x,std::int64_t y) const {
    if (x<0 || y<0 || x>=256 || y>=256) throw std::out_of_range("건설 밀어내기 지도 조회 오류");
    const auto spot=spots_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)];
    return !(spot&0x10) && (spot&6)!=0;
}
bool RawConstructionEffects::Displace(float x,float y,std::uint32_t type,std::uint32_t player) const {
    if (type>=types_.size()) throw std::out_of_range("건설 밀어내기 타입 오류");
    const auto& record=types_[type];Validate(x,y,record);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    auto rectangle=Footprint(x,y,record.footX,record.footY);
    if (static_cast<double>(rectangle[2])-rectangle[0]>=kShrinkWidth) {
        rectangle[0]+=kShrink;rectangle[1]+=kShrink;rectangle[2]-=kShrink;rectangle[3]-=kShrink;
    }
    const auto area=Area(rectangle);if (hooks_.scan) hooks_.scan(area);
    RawSquidFinder finder(pool_,hash_,types_);
    // 현재 finder 순서대로 옮긴다. 뒤의 적대 후보에서 false여도 앞선 이동은 유지한다.
    for (Sid current=finder.Begin(area);current.value;current=finder.Next()) {
        if (!(Genus(current)&(patch ? kPatchDisplace : kCdDisplace))) continue;
        bool found=false;int targetX=0,targetY=0;
        if (Genus(current)&kCenterFirst) {
            const int cx=Ftol(static_cast<double>(x)-(record.footX-1)/2),cy=Ftol(static_cast<double>(y)+1);
            if (Open(cx,cy)) { found=true;targetX=cx;targetY=cy; }
        }
        // 원본 DWORD 좌표의 지도 밖 입력을 거부할 때도 C++ signed overflow가 나지 않도록 반복 산술만 넓힌다.
        const std::int64_t baseY=Ftol(y),baseX=Ftol(x);const int distance=patch ? 3 : 2;
        // 아래쪽 줄을 가까운 순서로, 각 줄의 x는 오른쪽부터 발자국 폭만큼 역순으로 찾는다.
        for (std::int64_t row=baseY+1;!found && row<=baseY+distance;++row) {
            // 폭+1개의 spot을 확인한다. 발자국의 왼쪽 바로 바깥 칸까지 포함한다.
            for (std::int64_t column=baseX;column>=baseX-record.footX;--column) {
                if (Open(column,row)) { found=true;targetX=static_cast<int>(column);targetY=static_cast<int>(row);break; }
            }
        }
        // 아래에서 못 찾으면 오른쪽 열을 가까운 순서로, y는 아래부터 역순으로 찾는다.
        for (std::int64_t column=baseX+1;!found && column<=baseX+distance;++column) {
            // 높이+1개의 spot을 확인한다.
            for (std::int64_t row=baseY;row>=baseY-record.footY;--row) {
                if (Open(column,row)) { found=true;targetX=static_cast<int>(column);targetY=static_cast<int>(row);break; }
            }
        }
        if (!found) continue;
        const auto owner=pool_.Slot(current)[patch ? kPatchOwner : kCdOwner];
        if (patch && owner!=player) {
            if (owner>=9 || player>=9) throw std::out_of_range("건설 밀어내기 관계 표 범위 오류");
            if (!state_.alliances[player*9+owner] && !(Genus(current)&kEnemyAllowed)) return false;
        }
        const auto pathType=hooks_.pathType(pool_.Slot(current)[kType]);
        hooks_.clearPath(current,pathType);hooks_.unpop(current,0);
        // 원본은 Unpop 후 genus를 재조회한다. 이 읽기의 유효성도 보존한다.
        (void)Genus(current);hooks_.pop(current,static_cast<float>(targetX),static_cast<float>(targetY),0);
    }
    return true;
}
void RawConstructionEffects::DropIsland(float x,float y,std::uint32_t flags,std::uint32_t player) const {
    if (state_.blocked) return;
    if (state_.islandType>=types_.size()) throw std::out_of_range("섬 정리 타입 오류");
    Validate(x,y,types_[state_.islandType]);
    if (state_.server && !(flags&kNoBroadcast))
        hooks_.broadcast(std::bit_cast<std::uint32_t>(Ftol(static_cast<double>(x)*kWireScale)),
            std::bit_cast<std::uint32_t>(Ftol(static_cast<double>(y)*kWireScale)),flags,player);
    if (state_.islandType>=types_.size()) throw std::out_of_range("전파 뒤 섬 정리 타입 오류");
    Validate(x,y,types_[state_.islandType]);
    const auto& island=types_[state_.islandType];
    const auto area=Area({static_cast<float>(static_cast<double>(x)-(island.footX-1)),
        static_cast<float>(static_cast<double>(y)-(island.footY-1)),x,y});
    if (hooks_.scan) hooks_.scan(area);RawSquidFinder finder(pool_,hash_,types_);
    int noIslands=0,islands=0,stalags=0;
    // next를 이미 저장한 finder로 삭제 중에도 같은 탐색 순서를 유지한다. 슬롯 1부터 유효한 원본 범위를 따른다.
    for (Sid current=finder.Begin(area);current.value;current=finder.Next()) {
        const auto raw=pool_.Slot(current);if (raw[pool_.Edition()==OriginalEdition::Patch1078 ? kPatchOwner : kCdOwner]!=player) continue;
        const std::uint32_t type=raw[kType];
        if (type==state_.noIslandType || type==state_.stalagType) {
            if (!(flags&kQuietFall)) {
                hooks_.fall(current);const auto after=pool_.Slot(current);hooks_.sound(Coordinate(after,kX),Coordinate(after,kY));
            }
            hooks_.destroy(current,flags|kPieceFlags);
            if (type==state_.noIslandType) ++noIslands;else ++stalags;
        }
        if (type==state_.stumpType) hooks_.destroy(current,flags|kPieceFlags);
        if (type==state_.islandType) { hooks_.destroy(current,flags);++islands; }
        if (Genus(current)&kNotifyGenus) hooks_.notify(current);
    }
    if (noIslands || islands || stalags) {
        if (noIslands!=9 && hooks_.assertion) hooks_.assertion("noIslandTypeCount == 9",0x7b);
        if (islands!=1 && hooks_.assertion) hooks_.assertion("islandTypeCount == 1",0x7c);
        if (stalags!=1 && hooks_.assertion) hooks_.assertion("islandStalagTypeCount == 1",0x7d);
    }
}
const SidPool& RawConstructionEffects::Pool() const { return pool_; }
ConstructionRequestHooks MakeConstructionEffectsRequestHooks(const SidPool& pool,const RawConstructionEffects& effects,ConstructionRequestHooks hooks) {
    if (&pool!=&effects.Pool()) throw std::invalid_argument("건설 효과 모듈 풀 불일치");
    // 원본 판본별 인자 수/반환 사용을 유지한 채 요청에 실제 몸체를 연결한다.
    hooks.displace=[&effects](float x,float y,std::uint32_t type,std::uint32_t player) { return effects.Displace(x,y,type,player); };
    hooks.displaceCd=[&effects](float x,float y,std::uint32_t type) { (void)effects.Displace(x,y,type,0); };
    hooks.cancel=[&effects](float x,float y,std::uint32_t flags,std::uint32_t player) { effects.DropIsland(x,y,flags,player); };
    return hooks;
}
}
