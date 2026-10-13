// 원본 자연 반환·현재 목록 재조회·local PRNG와 성공/실패 횟수의 정확한 반복 계약이다.
#include "o/RawFortFinishOwner.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 레지던스 genus와 raw 타입/소유자 위치, 각 AddRandom의 최대 시도 횟수다.
constexpr std::uint32_t kResidence=0x40000000,kAttempts=1000;
constexpr std::size_t kType=10,kPatchOwner=34,kCdOwner=32;
// 시각 seed의 원본 double 배율과 DWORD 계수다.
constexpr double kTimeScale=1013.0;
constexpr std::uint32_t kSeedFactor=33;
// __ftol의64비트 절삭 하위 DWORD다. 비유한/불확정 정수는 원본 하위DWORD0으로 처리한다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// limit<1은 local PRNG를 소비하지 않는 원본 Random helper의 분기다.
std::uint32_t Next(GameRandom& random,std::int32_t limit) { return limit<1 ? 0U : random.Next(static_cast<std::uint32_t>(limit)); }
}
// 같은 판본의 현재 타입/실제 레지던스 목록과 모든 외부 효과를 요구한다.
RawFortFinishOwner::RawFortFinishOwner(const SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopList& residences,
    FortFinishOwnerState& state,FortFinishOwnerHooks hooks)
    :pool_(pool),types_(types),residences_(residences),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        !hooks_.regionBounds || !hooks_.wallSeconds || !hooks_.place || !hooks_.invalidType) throw std::invalid_argument("Fort owner 후처리의 실제 자료/경계 누락");
}
// SAR의 음수 반올림과 DWORD 감김을 그대로 유지한다. 글로벌 GameRandom seed를 변경하지 않는다.
std::uint32_t RawFortFinishOwner::TimeSeed(double wallSeconds) {
    const auto value=Truncate(wallSeconds*kTimeScale);
    return static_cast<std::uint32_t>(value>>4)+static_cast<std::uint32_t>(value)*kSeedFactor;
}
// 처음 정한 region은 반복 중 모드/Player가 바뀌어도 다시 정하지 않는다. 현재 residences/type은 매 순회 읽는다.
void RawFortFinishOwner::Finish(std::uint32_t owner) const {
    if (state_.chaffMode || state_.special) return;
    if (owner>=state_.playerRegions.size()) throw std::out_of_range("Fort owner 기본 지역 범위 오류");
    const auto region=state_.fort ? 0 : state_.playerRegions[owner];if (region==state_.invalidRegion) return;
    const auto ownerOffset=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchOwner : kCdOwner;
    // 원본은 실패1000회 뒤에도 다음 region/시각 seed를 구해 계속한다. 가짜 외부 타이머/성공을 넣지 않는다.
    for (;;) {
        bool found=false;
        // 전체 목록을 끝까지 읽은 뒤 일치 여부를 판단한다. free/dead/void를 추가로 제외하지 않는다.
        for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(residences_.count);++index) {
            if (index>=residences_.entries.size()) throw std::out_of_range("Fort residence 현재 목록 저장소 부족");
            const auto number=residences_.entries[index];if (number>=pool_.Capacity()) throw std::out_of_range("Fort residence SID 범위 오류");
            const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(number)});if (raw[kType]>=types_.size()) throw std::out_of_range("Fort residence 현재 타입 오류");
            if ((types_[raw[kType]].flags2&kResidence) && raw[ownerOffset]==owner) found=true;
        }
        if (found) return;
        const auto rectangle=hooks_.regionBounds(region);GameRandom random(TimeSeed(hooks_.wallSeconds()));
        PlaceRandom(random,rectangle,region,1,state_.residenceType,0);
    }
}
// 원본이 전달한 rectangle/value 인자는 snapshot이다. placement는 같은 local random을 소비/변경할 수 있다.
std::uint32_t RawFortFinishOwner::PlaceRandom(GameRandom& random,const FortRandomRectangle& rectangle,std::int32_t region,
    std::uint32_t quantity,std::uint32_t type,std::uint32_t force) const {
    if (!type || type==0xffffffffU) { hooks_.invalidType();return 0; }
    const auto area=rectangle; // 원본 by-value 인자를 보존하여 callback의 외부 사각형 변경과 분리한다.
    const auto height=Truncate(static_cast<double>(area.bottom)-area.top),width=Truncate(static_cast<double>(area.right)-area.left);
    std::uint32_t result=0;
    // 각 성공에서만 quantity를 감소시키고 현재 last result를 반환한다. quantity0도 첫 시도는 실행한다.
    for (std::uint32_t attempt=0;attempt<kAttempts;++attempt) {
        const auto dy=Next(random,height),dx=Next(random,width);
        const float x=static_cast<float>(static_cast<double>(dx)+area.left),y=static_cast<float>(static_cast<double>(dy)+area.top);
        result=hooks_.place(random,{type,x,y,region,0,force});if (result) --quantity;if (!quantity) break;
    }
    return result;
}
// 실제 같은 월드 참조다.
const SidPool& RawFortFinishOwner::Pool() const { return pool_; }
// 누락된 finishOwner를 자연 반환/반복의 실제 몸체로만 교체한다.
FortObjectLoadHooks MakeFortFinishOwnerHooks(const SidPool& pool,const RawFortFinishOwner& finish,FortObjectLoadHooks hooks) {
    if (&pool!=&finish.Pool()) throw std::invalid_argument("Fort loader/owner 후처리 SID 풀이 다릅니다");
    hooks.finishOwner=[&finish](std::uint32_t owner) { finish.Finish(owner); };return hooks;
}
}
