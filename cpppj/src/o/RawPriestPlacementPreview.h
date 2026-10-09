// 사제 배치의 기존 요청 계약을 일반 타입 미리보기에 연결한다.
#pragma once
#include "o/RawCanonPlacementPreview.h"

namespace netstorm::o {
// 두 경로는 원본의 같은 배열/관계 전역을 공유한다.
using PriestPlacementPreviewState=CanonPlacementPreviewState;
struct PriestPlacementPreviewHooks {
    // 기존 사제 비패턴 decoder의 범위 조회 경계다.
    std::function<SquidSearchArea(const PriestPlacementQuery&)> bounds;
    // 아직 복원하지 않은 충돌/지역 판정은 필수 경계로 유지한다.
    std::function<bool(const PriestPlacementQuery&,bool)> inspectCollisions;
};
class RawPriestPlacementPreview {
public:
    // 자료 수명과 지도/타입/판본 계약은 공통 미리보기에서 검사한다.
    RawPriestPlacementPreview(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const std::uint8_t> spots,std::span<const std::uint16_t> surfaceMap,
        PriestPlacementPreviewState& state,PriestPlacementPreviewHooks hooks);
    // 사제의 별도 첫 인자는 기존 원본 호출처럼 타입 번호다.
    bool Inspect(PriestPlacementQuery query,bool localOwner) const;
    // 연결 시 같은 SID 풀인지 확인한다.
    const SidPool& Pool() const;
private:
    RawCanonPlacementPreview preview_;
};
// 사제 접두의 후반 경계만 바꾸고 기존 모양 조회는 유지한다.
PriestPlacementHooks MakePriestPlacementPreviewHooks(const SidPool& pool,const RawPriestPlacementPreview& preview,PriestPlacementHooks hooks);
}
