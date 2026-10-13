// MayPlace 후반의 현재 표면 소유 관계·특수 지역·최종 거부 조건을 복원한다.
#pragma once
#include "o/RawCanonPlacementPermission.h"

namespace netstorm::o {
// 일반 타입·패턴 배치 섬 영향을 받는 지역 정보이다.
struct CanonPlacementIslandRegion {
    std::uint32_t restricted{},exists{}; // IslandList +id*32+4/+8의 제한 값과 존재 값이다.
};
// 일반 타입·패턴 배치 관계 처리 중 조회·갱신하는 상태 묶음이다.
struct CanonPlacementRelationsState {
    std::uint32_t bypassGroundPermission{}; // 0059ab30 / CD 0051cc00: 지면/권한만 우회한다.
    std::uint32_t islandCount{}; // 10.78 IslandList 00568b60의 부호 있는 개수 DWORD다.
};
// 일반 타입·패턴 배치 관계 검사나 변경 뒤 반환하는 결과이다.
struct CanonPlacementRelationResult {
    bool allowed{},relationRejected{}; // 최종 반환과 표면 관계의 내부 거부값이다.
    std::uint32_t foreignRejected{},priorRejection{}; // 최종 소유자 거부값과 CD의 이전 거부 지역 변수 저장값이다.
};
// MayPlace 후반의 현재 표면 소유 관계·특수 지역·최종 거부 조건을 복원한다.
class RawCanonPlacementRelations {
public:
    // 현재 표면 지도는 지형 후보의 고정 섬 지도와 별개다. 자료/공유 상태는 호출자가 소유한다.
    RawCanonPlacementRelations(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const std::uint16_t> surfaces,std::span<const CanonPlacementIslandRegion> islands,
        const CanonPlacementRelationsState& state,PriestPlacementState& placement,
        const RawCanonPlacementPermission& permission);
    // 기존 Finish의 지면 누적 뒤 호출한다. priorRejection은 원본 함수 진입에서 0인 지역 변수다.
    CanonPlacementRelationResult Inspect(const CanonPlacementQuery& query,CanonPlacementTerrainState& terrain,
        std::uint32_t priorRejection=0) const;
    // 0040eaf0 / CD 최종 인라인: 원래 좌표+0.9999 절삭 후 현재 지도 WORD를 읽는다.
    Sid SurfaceAt(float x,float y) const;
    // 같은 raw 풀을 사용하는 지형 어댑터만 연결한다.
    const SidPool& Pool() const;
private:
    // 포인터 범위 검사는 free/dead/type 필드와 독립이며 번호 0은 무효다.
    bool Valid(Sid sid) const;
    // 실제 최종 표는 편집기/동일 owner/동맹 활성 조건 없이 방향 DWORD를 직접 읽는다.
    bool Alliance(std::uint32_t currentOwner,std::uint32_t owner) const;
    // 0046f260: 존재하는 유효 지역 번호의 현재 제한 값을 읽는다.
    std::uint32_t Restricted(std::int32_t region) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // surfaces_: 표면 객체의 공간·연결·수명 처리 자료.
    std::span<const std::uint16_t> surfaces_;
    // islands_: 현재 섬의 배치·소유권 자료.
    std::span<const CanonPlacementIslandRegion> islands_;
    // state_: 처리 중 조회·갱신하는 상태 (const CanonPlacementRelationsState).
    const CanonPlacementRelationsState& state_;
    // placement_: 최종 배치 허용 여부를 판정하는 처리기 또는 상태.
    PriestPlacementState& placement_;
    // permission_: 소유권·지역 관계에 따른 배치 허용 판정.
    const RawCanonPlacementPermission& permission_;
};
// 후보/주변 권한 정책을 유지하고 최종 관계 경계만 실제 구현으로 바꾼다.
CanonPlacementTerrainHooks MakeCanonRelationsTerrainHooks(const SidPool& pool,
    const RawCanonPlacementRelations& relations,CanonPlacementTerrainHooks hooks);
}
