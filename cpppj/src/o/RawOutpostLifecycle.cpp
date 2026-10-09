// 원본 Array의 용량·순서·중복·비활성 꼬리와 지역 통지 전후 호출 순서를 보존한다.
#include "o/RawOutpostLifecycle.h"
#include "o/RawDamageablePreDestroy.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본의 공통 타입/상태/좌표와 abstract/buried·free/contained 비트다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18;
constexpr std::uint8_t kBlocked=9;
// 비정렬 raw 좌표를 읽어 NaN/부호 있는 0의 비트도 그대로 효과에 넘긴다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 원본 little endian DWORD의 낮은 바이트부터 조립한다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
// 가득 찼으면 조회/추가를 생략하고, 남은 용량에서는 기존 항목이 있을 때 추가하지 않는다.
void Add(SquidPostPopList& list,Sid sid) {
    if (list.count>=list.entries.size()) return;
    const auto items=list.Items();if (std::find(items.begin(),items.end(),sid.value)!=items.end()) return;
    list.entries[list.count++]=sid.value;
}
// 활성 중복 전부를 제거하되 남은 항목의 순서와 비활성 꼬리를 보존한다.
void Remove(SquidPostPopList& list,Sid sid) {
    std::uint32_t removed=0;
    // 원본은 남는 항목만 앞쪽으로 복사하며 새 꼬리를 지우지 않는다.
    for (std::uint32_t i=0;i<list.count;++i) {
        const auto value=list.entries[i];if (value==sid.value) ++removed;else list.entries[i-removed]=value;
    }
    list.count-=removed;
}
}
// 빠진 지역/부모 경계와 별도 전역 목록의 잘못된 연결을 생성 시 거부한다.
RawOutpostLifecycle::RawOutpostLifecycle(SidPool& pool,SquidPostPopList& additional,SquidPostPopList& nearest,OutpostLifecycleHooks hooks)
    :pool_(pool),additional_(additional),nearest_(nearest),hooks_(std::move(hooks)) {
    if (&additional==&nearest || !hooks_.regionChanged || !hooks_.validatePostPop || !hooks_.postPop || !hooks_.validatePreDestroy || !hooks_.preDestroy)
        throw std::invalid_argument("outpost 별도 목록/필수 효과 연결 오류");
}
// 파생 가상 메서드의 호출자 계약을 확인하고 원본의 extra 분기는 각 진입에서 따로 읽는다.
std::span<const std::uint8_t> RawOutpostLifecycle::Object(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto count=pool_.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (sid.value<5 || (raw[kState]&kBlocked) || raw[kType]<kFirstAssetTypeNumber || raw[kType]>=count)
        throw std::invalid_argument("outpost raw 자산 상태/타입 오류");
    return raw;
}
// 목록 쓰기와 지역 효과가 끝난 뒤 작업장 base를 같은 flags로 반드시 호출한다.
void RawOutpostLifecycle::PostPop(Sid sid,std::uint32_t flags) const {
    const auto raw=Object(sid);const bool ordinary=!(raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kBlocked);
    hooks_.validatePostPop(sid,flags);
    if ((flags&1) && ordinary) {
        // 두 목록을 먼저 검사하여 두 번째 손상이 첫 번째만 갱신하는 상황을 막는다.
        static_cast<void>(additional_.Items());static_cast<void>(nearest_.Items());Add(additional_,sid);Add(nearest_,sid);
        hooks_.regionChanged(Coordinate(pool_.Slot(sid),kX),Coordinate(pool_.Slot(sid),kY));
    }
    hooks_.postPop(sid,flags);
}
// abstract/buried는 목록/지역만 생략하며 Damageable base 호출은 생략하지 않는다.
void RawOutpostLifecycle::PreDestroy(Sid sid,std::uint32_t flags) const {
    const auto raw=Object(sid);const bool ordinary=!(raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kBlocked);
    hooks_.validatePreDestroy(sid,flags);
    if (ordinary) {
        static_cast<void>(additional_.Items());static_cast<void>(nearest_.Items());Remove(additional_,sid);Remove(nearest_,sid);
        hooks_.regionChanged(Coordinate(pool_.Slot(sid),kX),Coordinate(pool_.Slot(sid),kY));
    }
    hooks_.preDestroy(sid,flags);
}
// 외부 연결 시 동일한 실제 풀 참조를 제공한다.
const SidPool& RawOutpostLifecycle::Pool() const { return pool_; }
// 공간이나 원본 주소를 추측하지 않고 같은 풀의 복원 모듈에 삭제 준비를 위임한다.
OutpostLifecycleHooks MakeOutpostDamageablePreDestroyHooks(const SidPool& pool,const RawDamageablePreDestroy& damageable,OutpostLifecycleHooks hooks) {
    if (&pool!=&damageable.Pool()) throw std::invalid_argument("outpost/Damageable SID 풀이 다릅니다");
    hooks.validatePreDestroy=[&damageable](Sid sid,std::uint32_t flags) { damageable.Validate(sid,flags); };
    hooks.preDestroy=[&damageable](Sid sid,std::uint32_t flags) { damageable.PreDestroy(sid,flags); };return hooks;
}
}
