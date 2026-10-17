// 실제 캠페인 사제의 Start/BFS/Halt/Arrival/cleanup을 단일 월드에 구성한다.
#pragma once
#include "client/RawCampaignPathBindings.h"

namespace netstorm::client {
// 실제 몸체가 매 호출 읽는 mutable 모드를 상위 캠페인이 소유한다.
struct RawCampaignMotionState {
    o::CanonPlacementTerrainState terrain; // 중립 도착의 현재 지면 getter 상태다.
    o::PriestHaltState halt; // 실제 전투/권한/운반 타입의 현재 값이다.
    o::CarrierArrivalState carrier; // 실제 동맹/AI 존재/운반 권한의 현재 값이다.
    o::PriestArrivalState priest; // 실제 loading/중립 귀환 억제의 현재 값이다.
    o::PathCleanupState cleanup; // 의식/보조 건설물/너겟 정리의 현재 타입과 권한이다.
    o::RawPathSearchState search; // 원본 BFS가 직접 읽는 현재 Graph/정책 전역이다.
};
// 큰 운반·의식·경제·겹침/음향 효과는 실제 소비자를 공급한다. 누락을 성공으로 대체하지 않는다.
struct RawCampaignMotionEffects {
    o::PathCleanupHooks cleanup; // 의식/환불/할당의 실제 경계이며 contained/Graph/예약은 내부 같은 몸체로 연결한다.
    o::PriestHaltHooks halt; // 포획/너겟 전달이며 공간 Pop/Unpop은 같은 월드에 연결한다.
    o::CarrierArrivalHooks carrier; // 운반/포획/회피/조회/전송의 실제 외부 하위다.
    o::PriestArrivalHooks priest; // 실제 중립 귀환이며 자기삭제는 같은 월드에 연결한다.
    RawCampaignPathEffects path; // 같은 TYPE 메타/소리/겹침/재탐색의 실제 경계다.
    std::function<void(o::Sid)> carrierGoHome; // 428fd0의 실제 귀환 요청이며4276a0과 같은 함수로 대체하지 않는다.
};
// 첫 명령 전 구성하고 Kernel을 먼저 비운 뒤 해제한다. 별도 좌표/경로/지면 사본을 만들지 않는다.
class RawCampaignMotionBindings {
public:
    // 동일 Pool의 사제/섬/내용물/배치 Graph getter와 현재 원본 Start/Process 시계를 공유한다.
    RawCampaignMotionBindings(RawSurfaceWorld& world,const RawPriestWorldBindings& priests,
        const o::RawIslandRuntime& islands,const o::RawContainedCreate& contained,const o::RawPlayerPlacementAnchor& anchor,
        const o::SquidReward& hp,std::span<const o::PriestPlainCanonType> frames,RawCampaignMotionState& state,
        o::RawPathStartState& start,o::RawPathProcessState& process,RawCampaignMotionEffects effects);
    // 내부 훅이 참조하는 모듈의 고정 주소를 유지한다.
    ~RawCampaignMotionBindings();
    RawCampaignMotionBindings(const RawCampaignMotionBindings&)=delete;
    RawCampaignMotionBindings& operator=(const RawCampaignMotionBindings&)=delete;
    // 실제 사제 명령/현재 Path/직렬화를 기존 동일 Host 연결부로 실행한다.
    RawCampaignPathBindings& Paths() const;
    // 후속 운반/전투 모듈도 같은 현재 도착/cleanup 몸체를 재사용한다.
    const o::RawCarrierArrival& CarrierArrival() const;
    std::uint32_t CommonHalt(o::Sid parent) const; // 같은42a5f0을187의 주기 wrapper에서도 재사용한다.
    const o::RawPathCleanup& Cleanup() const;
    const o::SidPool& Pool() const;
private:
    struct Impl; // 실제 모듈과 단일 Core의 참조를 고정 주소에 보관한다.
    std::unique_ptr<Impl> impl_; // 외부 저장소와 Kernel은 해제 순서를 보존해야 한다.
};
}
