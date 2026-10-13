// 종속 객체 삭제 뒤의 현재 링크와 DWORD 타입/순번·번호별 권한을 보존한다.
#include "o/RawContainedLookup.h"
#include "o/RawConstructionProcess.h"
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본이 공유하는 next/head WORD·타입 BYTE·상태 BYTE 위치와 dead 비트다.
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11;
constexpr std::uint8_t kDead=2;
// 일반 client 번호의 첫 SID다. 서버 권한이 없으면 이 번호부터 serverFirst 전까지만 삭제한다.
constexpr std::uint16_t kFirst=5;
// 정렬되지 않은 링크 WORD를 little endian으로 읽는다.
Sid Link(std::span<const std::uint8_t> raw,std::size_t offset) {
    return {static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8))};
}
}
// 필수 삭제 경계를 확보하고 같은 풀을 계속 참조한다.
RawContainedLookup::RawContainedLookup(SidPool& pool,ContainedDestroy destroy):pool_(pool),destroy_(std::move(destroy)) {
    if (!destroy_) throw std::invalid_argument("contained 삭제 경계 누락");
}
// 타입 일치와 dead 제외를 통과한 항목만 순번을 소비한다. 선택 후에도 부모 head에서 다시 시작하지 않는다.
Sid RawContainedLookup::Find(Sid parent,std::uint32_t type,std::uint32_t ordinal,bool remove) const {
    Sid current=Link(pool_.Slot(parent),kHead);std::unordered_set<std::uint16_t> visited;
    // 삭제가 슬롯을 반납하거나 next를 바꿀 수 있으므로 다음 링크는 효과가 끝난 뒤 읽는다.
    while (current.value) {
        if (current.value>=pool_.Capacity() || !visited.insert(current.value).second)
            throw std::logic_error("contained 조회 종속 체인 손상");
        const auto raw=pool_.Slot(current);
        if (raw[kType]==type && !(raw[kState]&kDead)) {
            if (ordinal==0) {
                if (!remove) return current;
                if (pool_.IsServer() || (current.value>=kFirst && current.value<pool_.Layout().serverFirst))
                    destroy_(current,0);
            }
            --ordinal;
        }
        current=Link(pool_.Slot(current),kNext);
    }
    return {};
}
// 어댑터 연결 확인용 읽기 전용 참조다.
const SidPool& RawContainedLookup::Pool() const { return pool_; }
// 완료 시 진행음 프로세스를 실제 종속 체인에서 찾아 지우도록 연결한다.
ConstructionProcessHooks MakeContainedConstructionHooks(const SidPool& pool,const RawContainedLookup& lookup,ConstructionProcessHooks hooks) {
    if (&pool!=&lookup.Pool()) throw std::invalid_argument("contained 건설 어댑터 풀 불일치");
    hooks.removeContained=[&lookup](Sid parent,std::uint32_t type) { static_cast<void>(lookup.Find(parent,type,0,true)); };
    return hooks;
}
}
