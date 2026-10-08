// 원본 회복 분기와 가상 효과 뒤 재조회, 패치 전용 추적 필드 폭을 유지한다.
#include "o/RawPriestRegen.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 패치 추적 순환 상한과 측정 비교의 원본 float 리터럴이다.
constexpr std::int32_t kSequenceLimit=9600;
constexpr double kMeasurementLimit=7000.0;
// 정렬되지 않은 vtable DWORD를 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> raw) {
    std::uint32_t value=0;
    // 낮은 바이트부터 원본 주소 기록값을 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[i])<<(8*i);
    return value;
}
// 원본 signed ADD의 32비트 감기를 C++ 부호 오버플로 없이 표현한다.
std::int32_t Add(std::int32_t a,std::int32_t b) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));
}
}
// 미복원 효과를 빠뜨린 연결과 서로 다른 풀을 실행 전에 거부한다.
RawPriestRegen::RawPriestRegen(SidPool& pool,const SquidReward& hp,const RawPriestState& priest,
    const PriestHitPointMode& mode,PriestHitPointHooks space,GameRandom& random,PriestRegenState& state,PriestRegenHooks hooks)
    :pool_(pool),hp_(hp),priest_(priest),mode_(mode),space_(std::move(space)),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (&hp.Pool()!=&pool || &priest.Pool()!=&pool || !space_.unpop || !space_.repop || !hooks_.refresh ||
        !hooks_.damageable || !hooks_.shape || !hooks_.occupied || !hooks_.neutralAction ||
        (pool.Edition()==OriginalEdition::Patch1078 && (!hooks_.patchAudit || !hooks_.measurement)))
        throw std::invalid_argument("사제 회복 풀/외부 효과 연결 오류");
}
// 분배기의 동일 풀 보호에 쓴다.
const SidPool& RawPriestRegen::Pool() const { return pool_; }
// 00494580 / CD 0040ccd0의 회복 분기를 실행하며 다른 이벤트를 회복으로 오인하지 않는다.
float RawPriestRegen::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    (void)count;
    if (event!=kPriestRegenEvent) throw std::invalid_argument("미복원 사제 이벤트");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto current=priest_.CurrentHitPoints(sid);const auto raw=pool_.Slot(sid);
    if (raw[10]!=kPriestType || Vtable(raw)!=(patch ? kPatchPriestVtable : kCdPriestVtable))
        throw std::invalid_argument("사제 회복 타입/가상 표 오류");
    if (patch) {
        state_.patch.sample=random_.Next(9);
        state_.patch.sequence=Add(state_.patch.sequence,3);
        if (state_.patch.sequence>kSequenceLimit) state_.patch.sequence=Add(state_.patch.sequence,-kSequenceLimit);
    }
    if (current<hp_.MaxHitPoints(sid)) {
        const auto candidate=Add(priest_.CurrentHitPoints(sid),hp_.TypeHitPoints(kPriestType)/6);
        const auto value=candidate>hp_.MaxHitPoints(sid) ? hp_.MaxHitPoints(sid) :
            Add(priest_.CurrentHitPoints(sid),hp_.TypeHitPoints(kPriestType)/6);
        priest_.SetHitPoints(sid,value,mode_,space_);
        hooks_.refresh(sid);hooks_.damageable(sid);
    }
    if (patch) hooks_.patchAudit();
    // HP setter/외부 효과가 바꾼 현재 소유자·HP·전역을 이 시점에 다시 읽는다.
    const auto owner=pool_.Slot(sid)[patch ? 34 : 32];
    if (mode_.authority && !state_.neutralBlocked && owner==0 && !state_.modeBlocked &&
        priest_.CurrentHitPoints(sid)>=hp_.MaxHitPoints(sid)/2) {
        const auto shape=hooks_.shape(pool_.Slot(sid)[10]);
        if (!hooks_.occupied(sid,shape)) hooks_.neutralAction(sid);
    }
    if (patch) {
        auto& tracking=state_.patch;
        if (tracking.observedSequence!=tracking.sequence) tracking.changedSample=tracking.sample;
        if (tracking.remaining!=0) {
            if (hooks_.measurement()>kMeasurementLimit) tracking.changedSample=tracking.sample;
            --tracking.remaining;
        }
        if (tracking.gate==0 || tracking.sentinel==-1) tracking.changedSample=tracking.sample;
    }
    return payload;
}
// 다리 이벤트 분배기와 fallback으로 합성할 수 있다. 사제의 다른 사건은 명시적인 구현을 요구한다.
RegularHandler MakePriestRegenHandler(const SidPool& pool,const RawPriestRegen& regen,RegularHandler fallback) {
    if (&regen.Pool()!=&pool) throw std::invalid_argument("사제 회복 분배 풀 불일치");
    // 실제 vtable 기록값으로 구별하되 일반 Pop 지원 여부는 바꾸지 않는다.
    return [&pool,&regen,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        const auto vtable=Vtable(pool.Slot(sid));
        const bool priest=vtable==(pool.Edition()==OriginalEdition::Patch1078 ? kPatchPriestVtable : kCdPriestVtable);
        if (priest && event==kPriestRegenEvent) return regen.Handle(sid,event,count,payload);
        if (fallback) return fallback(sid,event,count,payload);
        if (priest) throw std::invalid_argument("미복원 사제 이벤트 분배");
        return payload;
    };
}
}
