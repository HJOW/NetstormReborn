// 기존 건설 private setter의 원본 폭/쓰기 몸체를 그대로 공유한다.
#include "o/RawSquidHitPoints.h"
#include <bit>

namespace netstorm::o {
namespace {
constexpr std::size_t kHp=26; // 두 판본 공통 raw현재HP의 시작 오프셋이다.
}
// 일반 HP 저장은 공간/상태/회복/표시 효과를 추가하지 않는다.
void SetRawSquidHitPoints(SidPool& pool,Sid sid,std::int32_t value) {
    auto raw=pool.AllocatedBytes(sid);const std::size_t width=pool.Edition()==OriginalEdition::Patch1078 ? 4 : 2;
    // 낮은 바이트부터 판본의 HP 폭까지만 쓴다.
    for (std::size_t i=0;i<width;++i) raw[kHp+i]=static_cast<std::uint8_t>(std::bit_cast<std::uint32_t>(value)>>(8*i));
}
}
