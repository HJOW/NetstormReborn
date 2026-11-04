// 원문 0041f870/0041f920/0041fab0의 중복 조회·할당·난수·float 저장 순서다.
#include "o/RawBombPostPop.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 호스트 주소를 raw 가상 함수 포인터로 호출하지 않는다.
namespace { // 세 PE 주소와 선택된 장치 요구를 한곳에 보관한다.
constexpr std::uint32_t kGravity=0x502c38U,kHeal=0x502cf0U,kMeteor=0x502db0U; // 각각 실제 +20=41f870/41f920/41fab0이다.
template<class F> F BombPopNeed(const F& hook) { if (!hook) throw std::logic_error("주문 효과 Pop의 선택 장치 미연결");return hook; } // 도달한 콜백만 수명 보존해 호출한다.
std::uint32_t BombPopVtable(const SidPool& pool,Sid parent) { // 현재 슬롯의 첫 DWORD만 little endian으로 읽는다.
    const auto raw=pool.Slot(parent);std::uint32_t value{}; // TYPE/상태에 추가 제약을 두지 않는 원문 주소다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[index])<<(8*index); // 네 바이트만 조합한다.
    return value;
}
}
RawBombPostPop::RawBombPostPop(const SidPool& pool,GameRandom& random,BombPostPopHooks hooks):pool_(pool),random_(random),hooks_(std::move(hooks)) { // PATCH 전용 장치의 판본을 확인한다.
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("주문 효과 Pop은10.78 전용");
}
bool RawBombPostPop::Handles(Sid parent) const { const auto table=BombPopVtable(pool_,parent);return table==kGravity || table==kHeal || table==kMeteor; } // 현재 TYPE/상태를 지원 주소와 혼동하지 않는다.
void RawBombPostPop::Prefix(Sid parent,std::uint32_t flags) const { // 선택 가상 몸체는 외부 조회가 VT를 바꿔도 이번 호출 동안 유지한다.
    const auto table=BombPopVtable(pool_,parent);if (table!=kGravity && table!=kHeal && table!=kMeteor) throw std::logic_error("미지원 주문 효과 Pop 가상 표");
    if (!(flags&1U) || BombPopNeed(hooks_.hasRegular)(parent,1) || !BombPopNeed(hooks_.reserveRegular)()) return;
    float delay=0.05F; // 그라비테이션의 원문 인자는 float 3d4ccccd다.
    if (table!=kGravity) {
        const float low=table==kHeal ? 0.0F : 0.01F,high=0.05F; // 455930의 두 입력은 원문 float 폭이다.
        const double selected=double(random_.Next(65536))*(1.0/65536.0)*std::abs(double(high)-double(low))+double(low); // 할당 실패/중복이면 공유 난수를 소비하지 않는다.
        delay=static_cast<float>(selected+(table==kHeal ? 0.05 : 0.0)); // 힐은 PE00502da8의 double0.05를 더한 뒤 한 번 float로 저장한다.
    }
    BombPopNeed(hooks_.addRegular)(parent,1,delay);
}
const SidPool& RawBombPostPop::Pool() const { return pool_; } // 같은 실제 풀의 명시 연결 확인용이다.
}
