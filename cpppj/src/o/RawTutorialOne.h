// Early Mission1의 실제 단계 조건을 공유 미션 게이트의 Advance에 전달한다.
#pragma once
#include "o/SquidPostPop.h"

namespace netstorm::o {
// 원본 mutable 전역을 참조하여 callback 뒤에도 같은 현재 상태를 읽는다.
struct TutorialOneState {
    std::int32_t& stage; // 원본005ca8e8의 signed char 단계이며 미션 게이트와 같은 저장소다.
    std::uint32_t& localPlayer; // 원본00540c70의 현재 플레이어 번호다.
    std::uint32_t& battleShown; // 원본00595344의 SP 조회 허용 전역이다.
    std::uint32_t& homeCommand; // 원본005429ac의 F4 명령 토큰이다.
    std::uint32_t& reclaimCommand; // 원본0054271c의 A단계에서 소비하는 명령 토큰이다.
};
// 단계 판단에 필요한 실제 게임 저장소와 생성 효과의 연결점이다.
struct TutorialOneHooks {
    std::function<std::int32_t(std::uint32_t)> builtForGenus; // 004c2500(4): 같은 실제 Totalmade 두 번째 로컬 집계다.
    std::function<std::uint32_t(std::uint32_t)> graphFor; // 00462bc0: 현재 신전/간헐천 SID의 실제 Graph다.
    std::function<float(std::uint32_t)> money; // 0040ec50: 현재 플레이어 SP의 단정도 결과다.
    std::function<void()> advance; // 004c33f0: 정지/잠금 확인 뒤 공유 단계를 증가시킨다.
    std::function<void()> spawnGeysers; // 00486360: C조건에서 Advance 반환과 무관하게 실행하는 실제 생성 몸체다.
};
// PATCH004c3a20의 전체 분기다. UI 프레임 게이트·건설·수확 몸체 자체는 호출자가 연결한다.
class RawTutorialOne {
public:
    // players는 같은9×0xb4 raw Player 표이며 geysers는00595094/98의 최근 생성 최대5개 목록이다.
    // AssetPostPopState의 전체 간헐천 목록과 다르므로 그 전체 목록으로 대체하지 않는다.
    RawTutorialOne(TutorialOneState state,std::span<const std::uint8_t> players,const SquidPostPopList& geysers,TutorialOneHooks hooks);
    // 원본은 A에서 reclaim 명령만1로 소비하고 나머지 조건/단계는0을 반환한다.
    std::uint32_t Handle(std::uint32_t event) const;
private:
    TutorialOneState state_; // 미션 게이트/클라이언트의 현재 전역 참조다.
    std::span<const std::uint8_t> players_; // 신전SID를 같은 Player+7c에서 직접 읽는다.
    const SquidPostPopList& geysers_; // 최근 생성표의 count/현재항목을 다시 읽는다. 전체 간헐천 장부가 아니다.
    TutorialOneHooks hooks_; // 실제 집계/Graph/SP/단계/생성 경계다.
};
}
