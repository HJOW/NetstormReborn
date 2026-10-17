// 탐색 실패를 성공 경로로 바꾸지 않고 원본1001 반환과 깊이·시각 저장 순서를 보존한다.
#include "o/RawCarrierEscape.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr float kSpotBias=0.9999F,kSnapBias=0.99999F; // 두 실제 PE 상수의 서로 다른 좌표 편향이다.
constexpr double kInt64Boundary=9223372036854775808.0; // 실제ftol의signed64 불확정값 경계다.
// 도달한 큰 하위의 누락은 원문 부작용을 보존한 채 거부한다.
template<class F> const F& Need(const F& f) { if (!f) throw std::logic_error("캐리어 회피 실제 하위 미연결");return f; }
// 비정렬 부모 raw DWORD의 원래 비트를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) { return std::uint32_t(raw[offset])|(std::uint32_t(raw[offset+1])<<8)|(std::uint32_t(raw[offset+2])<<16)|(std::uint32_t(raw[offset+3])<<24); }
// signed64 절삭 뒤 EAX 하위signed32만 사용하는 실제 변환이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value>=kInt64Boundary || value< -kInt64Boundary) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value)))); }
// 유효한signed 좌표 계산도 DWORD 감김으로 수행한다.
std::int32_t Add(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)+std::bit_cast<std::uint32_t>(second)); }
//422230은41d7a0의정수→float 저장 뒤 다시ftol하여중심을만든다.
std::int32_t Center(float value) { return Ftol(static_cast<float>(Ftol(static_cast<double>(value)+kSnapBias))); }
}
// PATCH의 실제 물리 자료만 받으며 CD의 다른 회피를 이 몸체로 대신하지 않는다.
RawCarrierEscape::RawCarrierEscape(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
    const double& now,const bool& checking,std::span<const std::uint32_t> cardinalTypes,CarrierEscapeState& state,CarrierEscapeHooks hooks)
    :pool_(pool),hash_(hash),types_(types),spots_(spots),now_(now),checking_(checking),cardinalTypes_(cardinalTypes),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || spots.size()!=65536 || cardinalTypes.size()!=4) throw std::invalid_argument("캐리어 회피 판본/물리 자료 오류");
}
// 최초 보관한 깊이는 진단 두 조건에 사용하고 최종 반환은 현재 같은Runtime에서 다시 읽는다.
std::int32_t RawCarrierEscape::Depth(Sid parent) const {
    const auto* path=Need(hooks_.path)(parent);if (!path) return 0;
    if (checking_) { const auto depth=path->MoveAwayDepth();if (depth<0) Need(hooks_.assertion)(0x87b);if (checking_ && depth>3) Need(hooks_.assertion)(0x87c); }return path->MoveAwayDepth();
}
// 초기부모 위치와 후보의 효과 전후 위치를 별개 조회한다.
RawPathPoint RawCarrierEscape::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// Native의 현재 타입 조회를 사용하며 호스트 표 밖은 역참조 전에 거부한다.
const RiftTypeRecord& RawCarrierEscape::Type(Sid sid) const { const auto kind=pool_.Slot(sid)[10];if (kind>=types_.size()) throw std::out_of_range("캐리어 회피 TYPE 범위 오류");return types_[kind]; }
// x/y를 각각 보드로clamp하지 않고 원래 선형 DWORD 주소를 계산한다.
bool RawCarrierEscape::Blocked(RawPathPoint point) const {
    const auto x=Ftol(static_cast<double>(point.x)+kSpotBias),y=Ftol(static_cast<double>(point.y)+kSpotBias);const auto index=std::bit_cast<std::uint32_t>(y)*256U+std::bit_cast<std::uint32_t>(x);
    if (index>=spots_.size()) throw std::out_of_range("캐리어 회피 spot 주소 범위 오류");return (spots_[index]&0x10U)!=0;
}
// 이전 경로가 지워져도 반환된 새 Runtime 이외에는 접근하지 않는다.
bool RawCarrierEscape::Start(Sid parent,RawPathPoint destination,std::int32_t depth) const { if (auto* path=Need(hooks_.start)(parent,destination)) { path->SetMoveAwayDepth(depth);return true; }return false; }
// event1da1a는네방향을건너뛰지만난수는먼저소비하며, 실패후에도1001을반환한다.
std::uint32_t RawCarrierEscape::Run(Sid parent,float distance,std::uint32_t event) const {
    const auto nextDepth=Add(Depth(parent),1);if (nextDepth>3 || (distance==1.0F && (Type(parent).flags2&0x20000U))) return 1000;
    const auto origin=Position(parent);const bool searchAround=event==0x1da1a;std::int32_t direction=std::bit_cast<std::int32_t>(event)/2;
    if (searchAround || event==0xffffffffU) direction=Need(hooks_.random)(0,5000)/1000;
    if (!searchAround) {
        // 첫 방향4를 그대로 읽은 다음 signed 나머지로 회전한다.
        for (int attempt=0;attempt<4;++attempt) {
            if (direction<0 || direction>4) throw std::out_of_range("캐리어 회피 물리 방향 범위 오류");const auto i=static_cast<std::size_t>(direction)*2;
            const RawPathPoint destination{static_cast<float>(static_cast<double>(state_.directions[i])*distance+origin.x),static_cast<float>(static_cast<double>(state_.directions[i+8])*distance+origin.y)};
            if (!Blocked(destination) && Start(parent,destination,nextDepth)) return 1001;direction=Add(direction,1)%4;
        }
        return 1001;
    }
    const auto x=Center(origin.x),y=Center(origin.y);RawSquidFinder finder(pool_,hash_,types_);float best=10000.0F;RawPathPoint chosen{-10000.0F,-10000.0F}; // 원본의 최소값과 미선택 좌표다.
    const SquidSearchArea area{Add(x,-5),Add(y,-5),Add(x,5),Add(y,5)};if (hooks_.scan) hooks_.scan(area);
    // 실제일반finder의현재체인과표면후보를원래순서대로평가한다.
    for (auto candidate=finder.Begin(area,0);candidate.value;candidate=finder.Next()) {
        if (!(Type(candidate).flags1&0x800U)) continue;const auto point=Position(candidate);
        if (Blocked(point) || Need(hooks_.blocker)(point).value) continue;RawPathRuntime temporary(OriginalEdition::Patch1078);const auto destination=Position(candidate);const auto kind=pool_.Slot(parent)[10];
        const auto cardinal=std::find(cardinalTypes_.begin(),cardinalTypes_.end(),kind)!=cardinalTypes_.end();Need(hooks_.search)(temporary,parent,origin,destination,cardinal ? 1U : 0U);
        if (temporary.Count()) { const auto movement=Need(hooks_.movement)(parent);const auto now=now_;temporary.Prepare(now,now,movement[0],movement[1],0);const auto metric=temporary.Time(temporary.Count()-1)-now_;if (metric<static_cast<double>(best)) { best=static_cast<float>(metric);chosen=Position(candidate); } }
    }
    if (Need(hooks_.valid)(chosen)) static_cast<void>(Start(parent,chosen,nextDepth));return 1001;
}
// 현재 실제 경로/공간의 같은풀이다.
const SidPool& RawCarrierEscape::Pool() const { return pool_; }
}
