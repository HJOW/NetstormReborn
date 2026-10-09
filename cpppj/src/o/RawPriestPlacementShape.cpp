// 패치/CD의 x87 중간 저장 위치·signed WORD와 DWORD 차·빈 범위를 보존한다.
#include "o/RawPriestPlacementShape.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 frameCheck의 이중 프레임 플래그와 해제된 타입의 표식이다.
constexpr std::uint32_t kDoubleFrames=0x440000;
constexpr std::int32_t kFreedA=-0x22222223,kFreedB=-0x32323233;
// 원본 sub의 low DWORD를 보존한다. C++ signed 뺄셈의 넘침을 피한다.
std::int32_t Difference(std::int16_t header,std::int32_t type) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int32_t>(header))-static_cast<std::uint32_t>(type));
}
}
// 같은 타입 번호 체계로 프레임 표·SHP·기준점을 받는다. 실제 참조 수명은 호출자가 보장한다.
RawPriestPlacementShape::RawPriestPlacementShape(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,std::span<const SquidDisplayShape> shapes,
    std::span<const PriestTypeHotspot> hotspots,PriestPlacementGeometryState& geometryState,PriestPlacementShapeState& state)
    :pool_(pool),types_(types),frames_(frames),shapes_(shapes),hotspots_(hotspots),geometryState_(geometryState),state_(state) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        frames.size()!=types.size() || shapes.size()!=types.size() || hotspots.size()!=types.size())
        throw std::invalid_argument("사제 픽셀 모양 타입/메타/SHP/기준점 자료 누락");
}
// 패치는 논리 프레임 및 해제 표식을 검사한다. CD는 실제 접근 가능한 물리 프레임만 확인한다.
const SquidDisplayFrame& RawPriestPlacementShape::Header(std::uint32_t type,int frame) const {
    const auto& shape=shapes_[type];const auto& record=types_[type];
    if (!shape.loaded || frame<0 || static_cast<std::size_t>(frame)>=shape.frames.size())
        throw std::out_of_range("사제 픽셀 모양 SHP 물리 프레임 조회 실패");
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        const auto count=static_cast<std::uint64_t>(frames_[type].frames.Codes().size())*((record.flags1&kDoubleFrames) ? 2U : 1U);
        if (static_cast<std::uint64_t>(frame)>=count || record.maxHitPoints==kFreedA || record.maxHitPoints==kFreedB)
            throw std::out_of_range("사제 픽셀 모양 frameCheck 실패(00419850)");
    }
    return shape.frames[static_cast<std::size_t>(frame)];
}
// 기본 프레임을 생성 시 캡처한 후 실제 Valid/Advance 순서로 SHP 헤더를 처리한다.
PriestPlacementPixelShape RawPriestPlacementShape::Measure(std::uint32_t type,std::uint32_t argument,std::uint32_t direction,bool explicitFrame) const {
    if (type>=types_.size() || !(types_[type].flags2&0x200000) ||
        std::find(geometryState_.patternTypes.begin(),geometryState_.patternTypes.end(),type)!=geometryState_.patternTypes.end())
        throw std::invalid_argument("사제 픽셀 모양 비패턴 타입 계약 오류");
    const auto& meta=frames_[type];CanonDecoder decoder(meta.frames,meta.defaultFrame,std::bit_cast<std::int32_t>(argument),std::bit_cast<std::int32_t>(direction),0,0,explicitFrame,pool_.Edition());
    PriestPlacementPixelShape result{{1000,1000,0,0},state_.scaleX,state_.scaleY};
    // 패턴 없는 사제는 한 프레임만 유효하지만 원본 순회 종료를 그대로 유지한다.
    while (decoder.Valid()) {
        if (!std::isfinite(state_.scaleX) || !std::isfinite(state_.scaleY)) throw std::invalid_argument("사제 픽셀 모양 배율의 비유한 값");
        const auto& header=Header(type,decoder.Frame());const auto& hot=hotspots_[type];
        const auto dx=Difference(header.hotspotX,hot.x),dy=Difference(header.hotspotY,hot.y);
        const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
        const double offsetX=patch ? static_cast<double>(dx) : static_cast<double>(static_cast<float>(dx));
        const float offsetY=static_cast<float>(dy);
        const double pixelX=patch ? static_cast<double>(decoder.X())*state_.scaleX : static_cast<float>(static_cast<double>(decoder.X())*state_.scaleX);
        const float pixelY=static_cast<float>(static_cast<double>(decoder.Y())*state_.scaleY);
        const double leftWide=pixelX-offsetX,topWide=static_cast<double>(pixelY)-offsetY;
        const float left=static_cast<float>(leftWide),top=static_cast<float>(topWide);
        if (leftWide<result.bounds.left) result.anchorX=static_cast<float>(dx);
        if (topWide<result.bounds.top) result.anchorY=offsetY;
        if (left<=result.bounds.left) result.bounds.left=left;
        if (top<=result.bounds.top) result.bounds.top=top;
        const double right=patch ? static_cast<double>(left)+header.width : (static_cast<double>(header.width)-offsetX)+pixelX;
        const double bottom=patch ? topWide+header.height : (static_cast<double>(header.height)-offsetY)+pixelY;
        // 패치는 같은 최댓값도 저장하지만 CD는 기존 값이 엄격히 작을 때만 저장한다(부호 있는 0 포함).
        if (patch ? result.bounds.right<=right : result.bounds.right<right) result.bounds.right=static_cast<float>(right);
        if (patch ? result.bounds.bottom<=bottom : result.bounds.bottom<bottom) result.bounds.bottom=static_cast<float>(bottom);
        decoder.Advance();
    }
    return result;
}
// 같은 풀에 연결하는 어댑터의 구성 검사에 사용한다.
const SidPool& RawPriestPlacementShape::Pool() const { return pool_; }
// 배치 경로는 명시 프레임을 켜지 않고 타입 번호를 첫 인자로 그대로 전달한다.
PriestPlacementHooks MakePriestShapePlacementHooks(const SidPool& pool,const RawPriestPlacementShape& shape,PriestPlacementHooks hooks) {
    if (&pool!=&shape.Pool()) throw std::invalid_argument("사제 픽셀 모양/배치의 SID 풀이 다릅니다");
    hooks.shape=[&shape](std::uint32_t type,std::uint32_t argument,std::uint32_t direction) { return shape.Measure(type,argument,direction).bounds; };return hooks;
}
}
