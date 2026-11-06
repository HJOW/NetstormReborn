// 원본00486bc0의 스펠 소비·SP 주기·일반 간헐천 보충 프레임이다.
#pragma once
#include "o/RawGameAuxState.h"
#include "o/RawWorldGeyser.h"

namespace netstorm::o { // 실제 장기 게임 전역과 현재 Player/옵션 물리를 직접 빌린다.
struct BattleResourceFrameGlobals {
    RawGameAuxState& aux;const double& loginEnd;const double& now; // 같은 보조 전역과 로그인/프레임 고정 게임시각이다.
    const bool& boss;const bool& battle;const bool& single;const std::uint32_t& editor;const std::uint32_t& owner; // 각 Native 모드/현재owner를 효과 뒤 다시 읽는다.
    std::span<const std::uint8_t> options;std::function<std::span<const std::uint8_t>()> players; //87BYTE와 현재9행의 장기 소유자는 caller다.
};
struct BattleResourceFrameHooks {
    std::function<void()> replenish;std::function<void(std::uint32_t,std::uint32_t,std::int32_t)> income; // 실제485e40과6ch/종류2/서버9 송신이다.
    std::function<bool()> predictable;std::function<double()> wallSeconds; // IntEquals(predictableGeysers,1)과 지역 TimeSeed다.
    std::function<std::int32_t()> budget,density,spacing; // 현재 원문 옵션 getter이며 미리 캡처한 기본값을 쓰지 않는다.
    std::function<FortRandomRectangle()> whole;std::function<FortRandomRectangle(std::int32_t)> partition; // 원문 전체/3×3 분할 범위다.
    std::function<std::uint32_t(FortRandomRectangle)> count;std::function<std::uint32_t()> geyserCount;std::function<float()> typeCost; // flags1 finder의 현재TYPE 수·실제 목록count/TYPE+c4다.
    std::function<std::uint32_t(std::uint32_t)> choose; // 후보가2개 이상일 때만 원문 공유 난수를 한 번 소비한다.
    std::function<void(std::int32_t,GameRandom&,FortRandomRectangle,GeyserPlacementOptions)> populate; // 같은 실제 Pool/MayPlace/현재 자원/최근 목록 생성이다.
};
void RunBattleResourceFrame(BattleResourceFrameGlobals globals,const BattleResourceFrameHooks& hooks); // 로그인 중 자연 반환하며 한 만료당 한 번만 소비/생성한다.
}
