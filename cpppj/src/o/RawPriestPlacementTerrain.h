// 일반 사제 배치의 후보별 다리·섬·지역 배열과 모양 종료 효과를 복원한다.
#pragma once
#include "o/RawPriestPlacementGeometry.h"

namespace netstorm::o {
struct PriestPlacementTerrainState {
    std::array<std::uint8_t,144> regions{}; // 원본 열 간격 12, 후보 쓰기 범위는 8×8이다.
    std::uint32_t emptyRegion{127},noIslandType{}; // 현재 지역 sentinel DWORD와 noIsland 타입 전역이다.
    bool bridgeOverlap{},noIslandOnly{true},groundComplete{true},permission{true},canPlaceGround{};
};
class RawPriestPlacementTerrain {
public:
    // 지형 SID 지도는 미리보기의 표면 SID 지도와 구분하여 256×256으로 제공한다.
    RawPriestPlacementTerrain(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,PriestPlacementTerrainState& state);
    // 일반 사제의 초기 permission과 모양 사이에 유지하는 누적 상태를 초기화한다. 지역 배열은 유지한다.
    void Begin(const PriestPlacementQuery& query);
    // finder 탐색 직전에 배열을 sentinel 하위 BYTE로 채우고 발자국 원점/지면 마스크를 캡처한다.
    void BeginShape(const PriestPlacementQuery& query,float x,float y);
    // 충돌 검사를 통과한 모든 후보에 적용한다. 무시된 후보도 다리/섬 효과를 받는다.
    void Candidate(const PriestPlacementQuery& query,Sid candidate);
    // 첫 signed BYTE와 sentinel DWORD 비교 및 발자국 전체의 동일 지역 여부를 누적한다.
    bool EndShape(const PriestPlacementQuery& query);
    // 일반 사제는 최종 지면/관계 조건의 거부를 우회하지만 해당 계산 효과는 보존한다.
    bool Finish(const PriestPlacementQuery& query);
    // 실제 RegionAt/지역 getter: 지도 SID 0 또는 범위 밖 좌표는 127, 다리는 현재 sentinel이다.
    std::uint32_t RegionAt(int x,int y) const;
    // 같은 풀을 사용하는 모양/충돌 어댑터만 연결한다.
    const SidPool& Pool() const;
private:
    // bridge/특수 지역 제외 타입 조합과 배열 밖 발자국은 지원 범위 밖으로 진단한다.
    const RiftTypeRecord& Placing(const PriestPlacementQuery& query) const;
    // 순회 시작 및 모양 시작 순서를 확인한다.
    void Require(const PriestPlacementQuery& query,bool shape) const;
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    std::span<const std::uint16_t> islands_;
    PriestPlacementTerrainState& state_;
    std::int64_t originX_{},originY_{};
    std::uint32_t groundMask_{},activeType_{};
    bool active_{},shape_{};
};
// 실제 지역 초기화/모양 시작/모양 종료/최종 계산을 geometry에 연결한다.
PriestPlacementGeometryHooks MakePriestTerrainGeometryHooks(const SidPool& pool,RawPriestPlacementTerrain& terrain);
// 후보 지형 효과만 대체하고 기존 모양 순회를 유지한다.
PriestPlacementCollisionHooks MakePriestTerrainCollisionHooks(const SidPool& pool,RawPriestPlacementTerrain& terrain,
    PriestPlacementCollisionHooks hooks);
}
