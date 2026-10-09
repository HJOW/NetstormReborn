// 사제 배치의 후보 무시·로컬 발자국 표시·충돌 거부를 실제 사각형 탐색기에 연결한다.
#pragma once
#include "o/RawPriestPlacementPreview.h"
#include "o/RawCanonPlacementCollision.h"

namespace netstorm::o {
// 사제와 일반 타입은 같은 클라이언트/무시 전역을 공유한다.
using PriestPlacementCollisionState=CanonPlacementCollisionState;
// 0049ade0 / CD 00444900: 임의 타입의 원본 무시 함수다. 반환 비트값을 bool로 축소하지 않는다.
std::uint32_t PlacementIgnoreValue(OriginalEdition edition,const RiftTypeRecord& placing,
    const RiftTypeRecord& candidate,std::uint32_t mode,std::uint32_t ignoredGenus);
struct PriestPlacementCollisionHooks {
    // 미복원 CanonDecoder 모양 순회·지형/지역/관계 처리 경계다. 각 모양의 충돌 검사는 scan으로 요청한다.
    // scan이 false이면 즉시 중단해야 한다. true 이후에도 이 경계가 모양별 지형 결과와 최종 결과를 처리한다.
    std::function<bool(const PriestPlacementQuery&,const std::function<bool(SquidSearchArea)>&)> inspectRegions;
    // 0049bb1a / CD 004457f7 이후 후보별 지형/지역 효과 경계다. 충돌을 무시한 후보도 호출한 뒤 Next로 진행한다.
    std::function<void(const PriestPlacementQuery&,Sid)> inspectCandidateTerrain;
};
class RawPriestPlacementCollision {
public:
    // 실제 탐색기는 같은 풀/해시/현재 타입 표로 이 객체 내부에서 구성한다.
    RawPriestPlacementCollision(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        PriestPlacementCollisionState& state,PriestPlacementPreviewState& preview,PriestPlacementCollisionHooks hooks);
    // 원본 값 인자와 접두에서 캡처한 로컬 여부로 모양 경계를 호출한다.
    bool Inspect(PriestPlacementQuery query,bool localOwner) const;
    // 모양 사각형의 실제 finder Begin/Next를 사용한다. 첫 충돌 거부 뒤에는 후속 후보를 읽지 않는다.
    bool InspectArea(PriestPlacementQuery query,bool localOwner,SquidSearchArea area) const;
    // 0049b9c4..0049bb1a / CD 004456ad..004457f7의 한 후보를 검사한다. 지역 처리 이전까지다.
    bool InspectCandidate(PriestPlacementQuery query,bool localOwner,Sid candidate) const;
    // 미리보기와 같은 풀/판본인지 연결 시 확인한다.
    const SidPool& Pool() const;
private:
    // 사제 genus 및 안전한 raw/발자국 입력 계약을 검사한다.
    const RiftTypeRecord& Placing(const PriestPlacementQuery& query) const;
    const SidPool& pool_;
    const SquidHash& hash_;
    std::span<const RiftTypeRecord> types_;
    PriestPlacementCollisionHooks hooks_;
    RawCanonPlacementCollision collision_;
};
// 미리보기의 inspectCollisions만 실제 후보 검사/탐색기로 연결한다. 모양/지형 경계는 호출자가 제공한다.
PriestPlacementPreviewHooks MakePriestPlacementCollisionHooks(const SidPool& pool,
    const RawPriestPlacementCollision& collision,PriestPlacementPreviewHooks hooks);
}
