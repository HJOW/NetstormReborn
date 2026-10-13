// 일반 타입/패턴의 후보 지형·지역 배열·모양 종료와 최종 지면 누적을 복원한다.
#pragma once
#include "o/RawCanonPlacementGeometry.h"

namespace netstorm::o {
// 일반 타입·패턴 배치 지형 처리 중 조회·갱신하는 상태 묶음이다.
struct CanonPlacementTerrainState {
    std::array<std::uint8_t,144> regions{}; // 열 간격 12이며 후보는 8×8 안에만 쓴다.
    std::uint32_t emptyRegion{127},noIslandType{}; // 현재 sentinel DWORD와 noIsland 타입 전역이다.
    // bridgeOverlap: 다리 겹침 여부.
    // noIslandOnly: 없음 섬 전용 여부.
    // groundComplete: 지면 완료 여부.
    // permission: 소유권·지역 관계에 따른 배치 허용 판정.
    // canPlaceGround: 가능 배치 지면 여부.
    bool bridgeOverlap{},noIslandOnly{true},groundComplete{true},permission{},canPlaceGround{};
};
// 일반 타입·패턴 배치 지형 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct CanonPlacementTerrainHooks {
    // 00462cb0 / CD 0045bae0: 지면이 맞는 섬의 현재 SID와 원래 소유자로 권한을 조회한다.
    std::function<bool(const CanonPlacementQuery&,Sid)> candidatePermission;
    // permission이 false인 모양에서만 주변 소유 관계 탐색을 요청한다. true면 권한을 누적한다.
    std::function<bool(const CanonPlacementQuery&,const CanonPlacementCell&)> shapePermission;
    // 지면 누적 뒤의 표면/소유 관계·특수 지역·최종 거부 경계다. 전체 MayPlace 복원은 후속이다.
    std::function<bool(const CanonPlacementQuery&,CanonPlacementTerrainState&)> finishRelations;
};
// 일반 타입/패턴의 후보 지형·지역 배열·모양 종료와 최종 지면 누적을 복원한다.
class RawCanonPlacementTerrain {
public:
    // 지역 SID 지도와 실제 프레임 코드 표를 받고 후보/주변 권한·최종 관계 훅을 필수로 요구한다.
    RawCanonPlacementTerrain(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,
        CanonPlacementTerrainState& state,CanonPlacementTerrainHooks hooks);
    // 그룹 10/현재 genus/bridge에 따른 초기 권한과 모양 사이의 누적 상태를 준비한다.
    void Begin(const CanonPlacementQuery& query);
    // 현재 snapped 좌표로 원점/지면을 캡처하며 144바이트를 sentinel 하위 BYTE로 채운다.
    void BeginShape(const CanonPlacementQuery& query,const CanonPlacementCell& cell);
    // 충돌에서 무시한 후보도 다리·섬·지역 쓰기·조건부 권한 조회를 받는다.
    void Candidate(const CanonPlacementQuery& query,Sid candidate);
    // signed BYTE/sentinel 비교와 현재 발자국 검사를 누적한 뒤 필요한 주변 관계를 조회한다.
    bool EndShape(const CanonPlacementQuery& query);
    // bridge/허용 지면/지역/permission을 누적하고 필수 최종 관계 경계의 판정을 반환한다.
    bool Finish(const CanonPlacementQuery& query);
    // 실제 좌표별 지역 getter의 WORD SID·genus·sentinel 판본 차이를 유지한다.
    std::uint32_t RegionAt(int x,int y) const;
    // 같은 풀을 사용하는 어댑터만 연결한다.
    const SidPool& Pool() const;
private:
    // 0 발자국도 허용하되 원본 배열의 물리 범위를 넘는 입력은 진단한다.
    const RiftTypeRecord& Placing(const CanonPlacementQuery& query) const;
    // 활성 순회의 타입/모양 준비 순서를 확인한다.
    void Require(const CanonPlacementQuery& query,bool shape) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::span<const PriestPlainCanonType> frames_;
    // islands_: 현재 섬의 배치·소유권 자료.
    std::span<const std::uint16_t> islands_;
    // state_: 처리 중 조회·갱신하는 상태 (CanonPlacementTerrainState).
    CanonPlacementTerrainState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (CanonPlacementTerrainHooks).
    CanonPlacementTerrainHooks hooks_;
    // cell_: 현재 위치에 대응하는 격자 칸.
    CanonPlacementCell cell_;
    // originX_: 좌표 계산의 기준 X 원점.
    // originY_: 좌표 계산의 기준 Y 원점.
    std::int64_t originX_{},originY_{};
    // groundMask_: 검사할 지면 칸을 고르는 비트 마스크.
    // activeType_: 활성 타입.
    std::uint32_t groundMask_{},activeType_{};
    // active_: 현재 입력·실행 대상이 활성 상태인지.
    // shape_: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    bool active_{},shape_{};
};
// 원래 소유자/모드를 캡처하여 실제 decoder의 준비/모양 종료/최종 경계를 연결한다.
CanonPlacementGeometryHooks MakeCanonTerrainGeometryHooks(const SidPool& pool,RawCanonPlacementTerrain& terrain,
    CanonPlacementQuery query);
// 후보 지형 효과만 연결하고 기존 실제 geometry/finder 순회는 유지한다.
CanonPlacementCollisionHooks MakeCanonTerrainCollisionHooks(const SidPool& pool,RawCanonPlacementTerrain& terrain,
    CanonPlacementCollisionHooks hooks);
}
