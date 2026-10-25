// Pool 바이트 초기화를 복제하지 않고 두 판본의 기존 실제 본문을 재사용한다.
#include "o/RawSquidPoolReset.h"
#include "o/SidPool.h"
#include "o/RawPathRuntime.h"
#include <bit>

namespace netstorm::o { //실패점은 손실 위치·미사일 이전점 등 다른 -10000 필드와 별개다.
void FinishSquidPoolReset(SidPool& pool,RawPathPoint& invalid) {
    pool.ClearDeletionHistory();
    invalid.y=std::bit_cast<float>(0xc61c4000U);invalid.x=std::bit_cast<float>(0xc61c4000U); //Native는 Y 다음 X의 두 DWORD만 쓴다.
}
void ResetSquidWorldPool(SidPool& pool,RawPathPoint& invalid,std::int32_t capacity) {
    pool.Reset(capacity); //실패하면 기존 기록·실패점의 뒤 효과를 실행하지 않는다.
    FinishSquidPoolReset(pool,invalid);
}
}
