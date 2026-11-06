// 원본0040e050의 보충용 오벨리스크 배치를 현재 모드/청크 seed/실제 AddRandom에 연결한다.
#pragma once
#include "o/RawFortFinishOwner.h"

namespace netstorm::o { // 공유 게임 난수와 별도의 원문 지역 난수를 사용한다.
struct BattleNuggetPlacementState {
    const bool& single;const bool& boss;const std::uint32_t& buriedType; //00594fa4/00540bc4/00541218의 현재값을 각 원문 위치에서 읽는다.
};
struct BattleNuggetPlacementHooks {
    std::function<void()> singleAssertion;std::function<double()> wallSeconds; // 원문 !inSingle 진단 복귀와 현재 TimeSeed다.
    std::function<std::array<std::uint8_t,6>()> regionZero; // 현재 ChunkMap의 지역0 첫 일치6BYTE이며 주소/소유자 값이 아니다.
    std::function<void(GameRandom&,FortRandomRectangle,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> place; // 실제0040ded0이며 지역-1/force1과 현재 타입을 그대로 전달한다.
};
void PlaceBattleNuggets(FortRandomRectangle rectangle,std::uint32_t quantity,BattleNuggetPlacementState state,const BattleNuggetPlacementHooks& hooks); // boss는 실제 wall seed, 비boss는 지역0 기록의byte3*103으로 배치한다.
}
