// 원문 진단→현재 boss→지역 seed→현재 TYPE/실제 배치의 순서를 보존한다.
#include "o/RawBattleNuggetPlacement.h"
#include "o/SpStore.h" // 실제 지역 GameRandom을 값으로 만들고 공유 난수와 구별한다.
#include <stdexcept>

namespace netstorm::o { // 별도 성공/객체/전역 난수 소비를 합성하지 않는다.
namespace { // 선택되지 않은 지역/시계 장치는 자연히 요구하지 않는다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("전투 오벨리스크 배치의 실제 장치 미연결");return effect; } // 실제 선택된 큰 부모만 연결을 확인한다.
}
void PlaceBattleNuggets(FortRandomRectangle rectangle,std::uint32_t quantity,BattleNuggetPlacementState state,const BattleNuggetPlacementHooks& hooks) {
    if (state.single) Need(hooks.singleAssertion)();GameRandom random(0);
    if (state.boss) random.SetState(RawFortFinishOwner::TimeSeed(Need(hooks.wallSeconds)()));
    else { const auto record=Need(hooks.regionZero)();random.SetState(std::uint32_t(record[3])*103U); }
    Need(hooks.place)(random,rectangle,-1,quantity,state.buriedType,1); // 원문의 미사용 반환을 새 성공 분기로 해석하지 않는다.
}
}
