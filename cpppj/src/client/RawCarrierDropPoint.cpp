// 원본 후보 순서와 현재 타입 발자국 재조회·signed64 절삭의 하위32비트를 보존한다.
#include "client/RawCarrierDropPoint.h"
#include "client/RawCarrierMenu.h"
#include "o/TerrainBuilder.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr float kSnapBias=0.9999899864196777F; // PE00502310의 실제 단정도 값이며 반올림/ceil로 바꾸지 않는다.
// 원본 ftol의 signed64 하위 DWORD를 signed32로 해석하고 fild32처럼 float에 저장한다.
float SnapCoordinate(float coordinate) {
    const double value=static_cast<double>(coordinate)+kSnapBias;std::int64_t integer=0;
    if (std::isfinite(value) && value>=-0x1p63 && value<0x1p63) integer=static_cast<std::int64_t>(value);
    return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(integer)));
}
// 원본 fild32+fadd float 뒤 한 번만 단정도로 저장한다.
float Offset(float coordinate,std::int32_t offset) { return static_cast<float>(static_cast<double>(coordinate)+offset); }
// 비정렬 raw float는 호스트 포인터 캐스트 없이 little endian DWORD로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 필요한 네 바이트만 낮은 바이트부터 조립한다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8);
    return std::bit_cast<float>(bits);
}
// 실제 도달한 필수 외부 몸체 누락을 조용한 실패점으로 바꾸지 않는다.
template<class F> const F& Required(const F& hook) { if (!hook) throw std::logic_error("운반물 위치의 실제 경계 미연결");return hook; }
}
// PATCH의 실제 슬롯과 현재 타입 표만 사용한다.
RawCarrierDropPoint::RawCarrierDropPoint(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
    const CarrierDropPointState& state,CarrierDropPointHooks hooks):pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.empty()) throw std::invalid_argument("운반물 위치 Pool/TYPE 판본 오류");
}
// 타입 범위 오류를 호스트 메모리 접근으로 진행하지 않는다.
const o::RiftTypeRecord& RawCarrierDropPoint::Type(std::uint32_t type) const {
    if (type>=types_.size()) throw std::out_of_range("운반물 위치 TYPE");return types_[type];
}
// 소유자는 BYTE를 부호 확장하지 않고 DWORD에 넣는다.
std::uint32_t RawCarrierDropPoint::Owner(o::Sid carrier) const { return pool_.Slot(carrier)[34]; }
// 원본은 후보의 두 축이 모두0 초과이며 현재 지도 한계 미만인 경우에만 채택한다.
bool RawCarrierDropPoint::Valid(std::array<float,2> point) const {
    const float limit=static_cast<float>(state_.mapLimit);return 0.0F<point[0] && point[0]<limit && 0.0F<point[1] && point[1]<limit;
}
// 두 축은 각각 ftol을 거친다. NaN/무한대/범위 밖은 CRT 불확정 정수의 하위0이다.
std::array<float,2> RawCarrierDropPoint::Snap(std::array<float,2> point) { return {SnapCoordinate(point[0]),SnapCoordinate(point[1])}; }
// 외부 배치 판정 뒤 바뀐 현재 footX/Y를 반복 조건에서 다시 읽는다.
std::array<float,2> RawCarrierDropPoint::FindFootprint(std::uint32_t type,std::uint32_t argument,float x,float y,
    std::uint32_t flags,std::uint32_t owner) const {
    const auto& record=Type(type);
    // 각 열의 첫 행부터 검사하며 실패한 열을 건너뛰는 최적화를 넣지 않는다.
    for (std::int32_t column=0;column<record.footX;++column) {
        // 원본 helper는 소유자 인자를 이 호출 동안 유지하고 mode는 항상0으로 전달한다.
        for (std::int32_t row=0;row<record.footY;++row) {
            if (Required(hooks_.mayPlace)({type,argument,Offset(x,column),Offset(y,row),flags,owner,0}))
                return {Offset(x,column),Offset(y,row)};
        }
    }
    return state_.invalidPoint;
}
// 원본은 즉시 허용점의 유효성을 다시 검사하지 않는다. 주변 helper 반환점만 별도 판정한다.
std::array<float,2> RawCarrierDropPoint::Find(o::Sid carrier,std::uint32_t type) const {
    const auto raw=pool_.Slot(carrier);auto point=Snap({Coordinate(raw,14),Coordinate(raw,18)});const auto& record=Type(type);
    if (record.flags2&0x100U) return Snap(Required(hooks_.center)(carrier));
    if (Required(hooks_.mayPlace)({type,0,point[0],point[1],0,Owner(carrier),0})) return point;
    // 짝수 방향 네 개를 먼저 검사하고 홀수 방향 네 개를 이어 검사한다.
    for (std::size_t phase=0;phase<2;++phase) {
        // 각 방향의 helper 호출 직전에 현재 소유자를 읽고 같은 위치 기준점을 유지한다.
        for (std::size_t index=0;index<4;++index) {
            const auto [dx,dy]=o::kNeighborCells[phase+index*2];
            const auto candidate=FindFootprint(type,0,Offset(point[0],dx),Offset(point[1],dy),0,Owner(carrier));
            if (Valid(candidate)) return candidate;
        }
    }
    return state_.invalidPoint;
}
// 어댑터가 다른 세션의 풀을 혼합하지 않도록 실제 참조를 돌려준다.
const o::SidPool& RawCarrierDropPoint::Pool() const { return pool_; }
// 같은 전체 파이프라인의 실제 MayPlace와 기존 중심 조회를 참조한다.
CarrierDropPointHooks MakeCarrierDropPointHooks(const o::SidPool& pool,o::RawCanonPlacementPipeline& placement,
    const o::RawGunTarget& centers) {
    if (&pool!=&placement.Pool() || &pool!=&centers.Pool()) throw std::invalid_argument("운반물 위치 월드 Pool 연결 오류");
    return {[&placement](const auto& query) { return placement.MayPlace(query); },[&centers](o::Sid sid) { return centers.Center(sid); }};
}
// 기존 전체 Carrier 메뉴가 이 위치 몸체를 호출하도록 연결한다.
void BindCarrierMenuDropPoint(CarrierMenuHooks& hooks,const o::SidPool& pool,const RawCarrierDropPoint& point) {
    if (&pool!=&point.Pool()) throw std::invalid_argument("운반물 메뉴 위치 Pool 연결 오류");
    hooks.dropPoint=[&point](o::Sid carrier,std::uint32_t type) { return point.Find(carrier,type); };
}
}
