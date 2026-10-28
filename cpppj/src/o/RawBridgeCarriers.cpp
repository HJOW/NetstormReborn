//일반 탐색의 현재 raw 체인과 float 최소값 저장을 원문 순서로 유지한다.
#include "o/RawBridgeCarriers.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //다른 이동체의 회피 정책을 이 원문 분기에 추가하지 않는다.
namespace { //CRT 절삭과 큰 효과의 연결만 공통화한다.
//도달한 실제 효과가 없으면 앞선 실제 변경을 유지한 채 오류를 전달한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("다리 이동체 대피의 실제 효과 미연결");return value; }
//비정렬 DWORD를 원본 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) { std::uint32_t value{};for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value; } //네 바이트의 위치만 복원한다.
//signed64 _ftol의 하위DWORD이며 비유한/범위 밖 indefinite의 하위값은0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); }
//반경 finder는 먼저 거의 올림→정수→float 저장을 거친다. 한 칸 finder의 단순 절삭과 구별한다.
std::int32_t Center(float value) { return Ftol(static_cast<float>(Ftol(static_cast<double>(value)+static_cast<double>(0.99999F)))); }
//원문 DWORD 덧셈의 감김을 C++ signed overflow 없이 보존한다.
std::int32_t Add(std::int32_t value,std::int32_t delta) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+std::bit_cast<std::uint32_t>(delta)); }
}
//PATCH의 raw 폭을 검증하며 구성 자체는 현재 전역/슬롯을 쓰지 않는다.
RawBridgeCarriers::RawBridgeCarriers(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    const bool& boss,const bool& checking,const std::uint32_t& priest,const std::uint32_t& rainWalker,const std::int32_t& avoidLife,BridgeCarrierHooks hooks)
    :pool_(pool),hash_(hash),types_(types),boss_(boss),checking_(checking),priest_(priest),rainWalker_(rainWalker),avoidLife_(avoidLife),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("다리 이동체 대피의 PATCH 풀/TYPE 오류");
}
//현재 위치는 함수 진입 snapshot으로 고정하지 않는다.
RawPathPoint RawBridgeCarriers::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
//현재 타입 범위 밖 읽기는 해당 스캔 칸의 예외 경계로 전달한다.
const RiftTypeRecord& RawBridgeCarriers::Type(Sid sid) const { const auto type=pool_.Slot(sid)[10];if (type>=types_.size()) throw std::out_of_range("다리 이동체 대피의 현재 TYPE 범위");return types_[type]; }
//원문 포인터 동일성만 필요하므로254는0, 나머지 번호는+1로 구별한다.
std::uint32_t RawBridgeCarriers::Graph(Sid sid) const { const auto number=Need(hooks_.graphFor)(sid);return number==254 ? 0U : number+1U; }
//일반 surface도 수명이0이면 정상이며 다리가 아닌 후보는 이 조건으로 제외하지 않는다.
bool RawBridgeCarriers::Dangerous(Sid sid) const { if (!sid.value || !(Type(sid).flags2&4U)) return false;const auto life=static_cast<std::int32_t>((pool_.Slot(sid)[12]&0x78U)>>3);return life && life<avoidLife_; }
//기존 이동 Form은 body/type이 아니라 원문의 종류 선택 뒤 같은 현재 내용물에서 조회한다.
void RawBridgeCarriers::Check(Sid bridge) const {
    const auto point=Position(bridge);const auto x=Ftol(point.x),y=Ftol(point.y);RawSquidFinder finder(pool_,hash_,types_);
    const SquidSearchArea area{x,y,x,y};if (hooks_.scan) hooks_.scan(area);
    for (auto sid=finder.Begin(area,0);sid.value;sid=finder.Next()) { //바깥 finder는 중첩 대피의 별도 finder와 독립적으로 다음 링크를 유지한다.
        const auto kind=pool_.Slot(sid)[10];if (kind!=priest_ && kind!=rainWalker_) continue;
        if (!(Type(sid).flags2&0x30000U)) Need(hooks_.assertion)(0x2d8);
        const auto movement=Need(hooks_.movementType)(pool_.Slot(sid)[10]);if (!Need(hooks_.movementForm)(sid,movement).value) Retreat(sid);
    }
}
//선택한 최소 거리만float로 저장한다. 동률은 앞서 선택한 현재 후보를 유지한다.
void RawBridgeCarriers::Retreat(Sid parent) const {
    if (!boss_ || !Graph(parent)) return;
    const auto kind=pool_.Slot(parent)[10];if (checking_ && !Need(hooks_.wontWalkOffEdge)(kind)) Need(hooks_.assertion)(0x893);
    float best=100.0F;RawPathPoint chosen{-10000.0F,-10000.0F};const auto point=Position(parent);const auto x=Center(point.x),y=Center(point.y);RawSquidFinder finder(pool_,hash_,types_);
    const SquidSearchArea area{Add(x,-5),Add(y,-5),Add(x,5),Add(y,5)};if (hooks_.scan) hooks_.scan(area);
    for (auto sid=finder.Begin(area,0);sid.value;sid=finder.Next()) { //현재 표면·수명·Graph 조건을 통과한 순서대로 거리를 비교한다.
        if (!(Type(sid).flags1&0x800U) || Dangerous(sid)) continue;
        const auto mine=Graph(parent),other=Graph(sid);if (mine!=other) continue;
        const auto destination=Position(sid),origin=Position(parent);const double dx=static_cast<double>(destination.x)-origin.x,dy=static_cast<double>(destination.y)-origin.y;
        const double distance=std::sqrt(dy*dy+dx*dx);if (distance>=1.0 && distance<static_cast<double>(best)) { best=static_cast<float>(distance);chosen=Position(sid); } //원문의 비교는 하한과 같아도 채택한다.
    }
    if (Need(hooks_.valid)(chosen) && !Need(hooks_.start)(parent,chosen) && checking_) Need(hooks_.assertion)(0x8b0);
}
}
