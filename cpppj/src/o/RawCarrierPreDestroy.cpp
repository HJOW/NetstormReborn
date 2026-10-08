// 종속의 next를 먼저 읽고 원본 DWORD 타입과 두 WORD 조건을 순서대로 평가한다.
#include "o/RawCarrierPreDestroy.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본에서 공통인 종속 링크·타입·상태와 조회 조건의 raw 위치다.
constexpr std::size_t kType=10,kState=11,kKind=18,kMask=20;
// free/contained는 직접 자산 호출에서 거부하고 dead는 원본 삭제 준비에서 허용한다.
constexpr std::uint8_t kFreeContained=9;
// 정렬되지 않은 little endian WORD를 읽는다.
std::uint16_t Word(std::span<const std::uint8_t> raw,std::size_t offset) {
    return static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8));
}
}
// 전체 Damageable와 전역 후처리 경계를 공급해야 직접 호출할 수 있다.
RawCarrierPreDestroy::RawCarrierPreDestroy(SidPool& pool,const CarrierPreDestroyState& state,CarrierPreDestroyHooks hooks)
    :pool_(pool),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.validateDamageable || !hooks_.damageablePre || !hooks_.dependentFound)
        throw std::invalid_argument("Carrier preDestroy 하위 효과 누락");
}
// 새로운 가상 지원이나 공간 해제 없이 순수 경계 검사만 수행한다.
void RawCarrierPreDestroy::Validate(Sid sid,std::uint32_t flags) const {
    const auto raw=pool_.AllocatedBytes(sid);
    const auto typeEnd=pool_.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if ((raw[kState]&kFreeContained) || raw[kType]<kFirstAssetTypeNumber || raw[kType]>=typeEnd)
        throw std::invalid_argument("Carrier preDestroy raw 자산 상태/타입 오류");
    hooks_.validateDamageable(sid,flags);
}
// Damageable에 전달한 flags는 종속 조회 인자에 영향을 주지 않는다.
void RawCarrierPreDestroy::PreDestroy(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);hooks_.damageablePre(sid,flags);
    if (state_.boss && FindContained(sid,1,0,0).value) hooks_.dependentFound();
}
// 실제 contained finder는 전체 종속 중 전역 타입과 같은 BYTE만 기본 true 필터에 넘긴다.
Sid RawCarrierPreDestroy::FindContained(Sid parent,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) const {
    RawContainedFinder finder(pool_,state_);
    // 공용 커서는 가장 최근 종속부터 진행하며 다음 링크를 먼저 저장한다.
    for (Sid current=finder.Begin(parent);current.value;current=finder.Next()) {
        const auto candidate=pool_.Slot(current);
        if ((!kind || Word(candidate,kKind)==kind)
            && (!mask || (Word(candidate,kMask)&mask))) {
            if (!index) return current;
            --index;
        }
    }
    return {};
}
// raw 참조 대신 풀 동일성만 공개한다.
const SidPool& RawCarrierPreDestroy::Pool() const { return pool_; }
// 하위 검사를 사제 목록 효과 전에 호출하도록 같은 풀의 두 경계를 교체한다.
PriestPreDestroyHooks MakeCarrierPreDestroyHooks(const SidPool& pool,const RawCarrierPreDestroy& carrier,PriestPreDestroyHooks hooks) {
    if (&carrier.Pool()!=&pool) throw std::invalid_argument("사제/Carrier preDestroy의 SID 풀이 다릅니다");
    hooks.validateCarrier=[&carrier](Sid sid,std::uint32_t flags) { carrier.Validate(sid,flags); };
    hooks.carrierPre=[&carrier](Sid sid,std::uint32_t flags) { carrier.PreDestroy(sid,flags); };
    return hooks;
}
}
