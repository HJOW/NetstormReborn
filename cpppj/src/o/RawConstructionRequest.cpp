// 요청의 원본 분기와 float 좌표 비교를 유지하고 외부 효과만 호출자가 연결한다.
#include "o/RawConstructionRequest.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본의 raw 타입·좌표, 판본별 소유자/abstract 바이트 위치다.
constexpr std::size_t kType=10,kX=14,kY=18,kPatchOwner=34,kCdOwner=32,kPatchExtra=40,kCdExtra=35;
// 받침 검사 대상의 flags1 비트, 실패 정리 플래그, 표면 조회의 좌표 편향(00500edc / 00506958)이다.
constexpr std::uint32_t kNeedsBase=0x400,kCancelFlag=0x1000;
constexpr float kSurfaceBias=0.9998999834060669F;
// 원본 __ftol의 64비트 절삭 결과 하위 DWORD다. 유한하지 않거나 64비트 범위 밖이면 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 정렬되지 않은 raw 좌표 DWORD를 little endian으로 읽어 float로 복원한다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 각 바이트를 낮은 자리부터 합친다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(value);
}
// 원본 정수의 32비트 순환 연산을 보존한 발자국 오프셋이다. C++ signed overflow를 피한다.
std::int32_t Offset(int foot) { return std::bit_cast<std::int32_t>(1U-static_cast<std::uint32_t>(foot)); }
// 0049ae80 / 004449d0: 발자국만큼 물린 점을 단정도로 저장·1~255로 자르고 유효한 원래 좌표와 잇는다.
PriestPlacementRect Footprint(float x,float y,int width,int height) {
    const float pointX=std::clamp(static_cast<float>(static_cast<double>(x)+Offset(width)),1.0F,255.0F);
    const float pointY=std::clamp(static_cast<float>(static_cast<double>(y)+Offset(height)),1.0F,255.0F);
    if (!(x>0.0F && y>0.0F && x<256.0F && y<256.0F)) return {pointX,pointY,pointX,pointY};
    return {std::min(x,pointX),std::min(y,pointY),std::max(x,pointX),std::max(y,pointY)};
}
}
// 경계와 지도 표 크기를 검증한다. 같은 효과를 두 판본에 잘못 연결하지 않는다.
RawConstructionRequest::RawConstructionRequest(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,std::span<const std::uint8_t> spots,const ConstructionRequestState& state,ConstructionRequestHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),spots_(spots),state_(state),hooks_(std::move(hooks)) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (spots_.size()<kSpotCount || !hooks_.mayPlace || !hooks_.confirm || !hooks_.cancel ||
        (patch ? !hooks_.displace : !hooks_.displaceCd)) throw std::invalid_argument("건설 요청 자료/효과 연결 오류");
}
// spot 표의 발자국 사각형을 x 우선, y 안쪽 순서로 훑는다(원본 helper 순서).
bool RawConstructionRequest::NoSurface(const PriestPlacementRect& rectangle) const {
    const int left=static_cast<int>(rectangle.left),top=static_cast<int>(rectangle.top);
    const int right=static_cast<int>(rectangle.right),bottom=static_cast<int>(rectangle.bottom);
    // 모서리는 유한 요청 좌표와 1~255로 자른 점에서 나오므로 항상 표 안이다.
    for (int x=left;x<=right;++x) {
        // 한 열에서 bit 2가 있는 첫 칸을 만나면 false다.
        for (int y=top;y<=bottom;++y) if (spots_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]&2) return false;
    }
    return true;
}
// 실제 표면 해시 0단계의 WORD 머리를 조회한다. 정수 범위 밖에서 cast하지 않는다.
Sid RawConstructionRequest::Surface(float x,float y) const {
    const auto cellX=Ftol(static_cast<double>(x)+kSurfaceBias),cellY=Ftol(static_cast<double>(y)+kSurfaceBias);
    if (cellX<0 || cellY<0 || cellX>=256 || cellY>=256) return Sid{};
    return Sid{hash_.Cell(0,cellX,cellY)};
}
// 후보는 서버 번호도 검사한다. noIsland는 소유자/타입/좌표가 달라도 충돌 검사에서 제외한다.
bool RawConstructionRequest::Request(ConstructionConfirmRequest request) const {
    if (request.type>=types_.size() || request.type>=frames_.size() || request.type>0xff ||
        state_.patternTypes[2]>=types_.size() || state_.patternTypes[3]>=types_.size() ||
        !std::isfinite(request.x) || !std::isfinite(request.y)) throw std::out_of_range("건설 요청 타입/좌표 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto& record=types_[request.type];const auto& meta=frames_[request.type];
    if (state_.editor) request.player=state_.localPlayer;
    if (!hooks_.mayPlace({request.type,request.argument,request.x,request.y,request.direction,request.player,0})) return false;
    auto decoder=DecodeCanonType(meta.frames,meta.defaultFrame,state_.patternTypes,{request.type,std::bit_cast<int>(request.argument),
        std::bit_cast<int>(request.direction),request.x,request.y,false},pool_.Edition());
    const std::size_t extra=patch ? kPatchExtra : kCdExtra,owner=patch ? kPatchOwner : kCdOwner;
    // decoder의 모든 유효 칸에서 abstract 예측이 같은 플레이어·타입·정확한 좌표인지 검사한다.
    for (;decoder.Valid();decoder.Advance()) {
        const auto area=RawConstructionClear::CellArea(decoder.X(),decoder.Y(),record.footX,record.footY);
        if (hooks_.scan) hooks_.scan(area);
        RawSquidFinder finder(pool_,hash_,types_);
        // 일반 finder의 순서를 유지하고 첫 충돌에서 효과 없이 종료한다.
        for (Sid current=finder.Begin(area);current.value!=0;current=finder.Next()) {
            const auto raw=pool_.Slot(current);
            if (!(raw[extra]&1) || raw[kType]==state_.patternTypes[3]) continue;
            if (static_cast<std::uint32_t>(raw[owner])!=request.player || static_cast<std::uint32_t>(raw[kType])!=request.type ||
                Coordinate(raw,kX)!=decoder.X() || Coordinate(raw,kY)!=decoder.Y()) return false;
        }
    }
    if (record.flags2&state_.displaceGenus) {
        if (patch) { if (!hooks_.displace(request.x,request.y,request.type,request.player)) return false; }
        else hooks_.displaceCd(request.x,request.y,request.type);
    }
    const auto& island=types_[state_.patternTypes[2]];
    const float baseX=static_cast<float>(static_cast<double>(request.x)+Offset(island.footX));
    const float baseY=static_cast<float>(static_cast<double>(request.y)+Offset(island.footY));
    const auto rectangle=Footprint(request.x,request.y,record.footX,record.footY);
    if (record.flags1&kNeedsBase) {
        // 원본은 먼저 표면을 조회한다. spot에 bit 2가 없거나 표면이 abstract이면 받침을 선행 확정한다.
        const auto surface=pool_.Slot(Surface(request.x,request.y));
        if (NoSurface(rectangle) || (surface[extra]&1))
            hooks_.confirm({state_.patternTypes[3],baseX,baseY,request.player,0,0,0,0,0,0});
    }
    if (hooks_.confirm(request)) return true;
    // 확정 경계가 타입 플래그를 바꿀 수 있으므로 실패 후 현재 flags1을 다시 읽는다.
    if (record.flags1&kNeedsBase) hooks_.cancel(request.x,request.y,kCancelFlag,request.player);
    return false;
}
// 연결 검증에 실제 풀 참조를 돌려준다.
const SidPool& RawConstructionRequest::Pool() const { return pool_; }
// 객체를 캡처하는 경계의 수명은 호출자가 보장한다.
ConstructionRequestHooks MakeConstructionRequestHooks(const SidPool& pool,RawCanonPlacementPipeline& placement,
    const RawConstructionConfirm& confirm,ConstructionRequestHooks hooks) {
    if (&pool!=&placement.Pool() || &pool!=&confirm.Pool()) throw std::invalid_argument("건설 요청 모듈 풀 불일치");
    // 배치와 확정의 원본 인자를 실제 모듈에 그대로 전달한다.
    hooks.mayPlace=[&placement](const CanonPlacementQuery& query) { return placement.MayPlace(query); };
    hooks.confirm=[&confirm](const ConstructionConfirmRequest& request) { return confirm.Confirm(request); };
    return hooks;
}
}
