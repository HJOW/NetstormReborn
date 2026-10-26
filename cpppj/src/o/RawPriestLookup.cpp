// 원본 목록 검색의 조기 종료와 현재 필드 읽기를 복원한다.
#include "o/RawPriestLookup.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// raw 상태와 판본별 현재 소유자 BYTE의 위치다. 원래 소유자 WORD는 사용하지 않는다.
constexpr std::size_t kState=11,kPatchOwner=34,kCdOwner=32;
// 원본 조회가 제외하는 유일한 상태 비트다.
constexpr std::uint8_t kDead=2;
}
// 목록을 복사하지 않고 원본 전역과 같은 공유 상태를 참조한다.
RawPriestLookup::RawPriestLookup(const SidPool& pool,const PriestPostPopState& state):pool_(pool),state_(state) {}
// signed 개수 검사 뒤 활성 목록을 원래 순서로 읽고 첫 일치에서 멈춘다.
std::uint32_t RawPriestLookup::Find(std::uint32_t player) const {
    if (std::bit_cast<std::int32_t>(state_.Priests().count)<=0) return 0;
    const auto owner=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchOwner : kCdOwner;
    // 뒤의 손상된 항목도 첫 일치 뒤에는 읽지 않는다. free/void/타입/extra는 검사하지 않는다.
    for (const auto sid:state_.Priests().Items()) {
        if (sid>=pool_.Capacity()) throw std::out_of_range("소유 사제 조회 SID 범위 오류");
        const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(sid)});
        if (raw[owner]==player && !(raw[kState]&kDead)) return sid;
    }
    return 0;
}
// 어댑터가 참조 풀을 동일성으로 검사한다.
const SidPool& RawPriestLookup::Pool() const { return pool_; }
}
