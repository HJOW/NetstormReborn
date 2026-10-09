// 일반 타입/패턴 배치의 로컬 미리보기 배열·표면 조회·소유 관계 판정을 복원한다.
#pragma once
#include "o/RawCanonPlacement.h"
#include "o/RawSquidFinder.h"

namespace netstorm::o {
class RawCanonPlacementGeometry;
struct CanonPlacementPreviewState {
    std::array<std::uint8_t,144> blocked{}; // 0059a9f0 / CD 00565998: x 인덱스 먼저인 12×12 배열이다.
    std::uint32_t editor{}; // 005c85a4 / CD 00518904: 표면 소유 관계를 모두 허용한다.
    std::uint32_t useAlliances{}; // 00540cb0 / CD 0050f824: 소유자×9+요청자의 관계 표 사용 조건이다.
    std::array<std::uint32_t,81> alliances{}; // 00595200 / CD 0050f6e0: 행 소유자→열 요청자의 방향을 보존한다.
};
struct CanonPlacementPreviewHooks {
    // CanonDecoder 생성/정수 사각형 조회(00425c20·00425b90 / CD 0041fcf0·004202e0)의 경계다.
    std::function<SquidSearchArea(const CanonPlacementQuery&)> bounds;
    // 0049b825 이후 / CD 0044550e 이후: 아직 복원하지 않은 실제 충돌/지역 처리다.
    std::function<bool(const CanonPlacementQuery&,bool)> inspectCollisions;
};
class RawCanonPlacementPreview {
public:
    // 고정 표면 SID 지도·공간 BYTE·타입·상태 참조는 이 객체보다 오래 살아야 한다.
    RawCanonPlacementPreview(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const std::uint8_t> spots,std::span<const std::uint16_t> surfaceMap,
        CanonPlacementPreviewState& state,CanonPlacementPreviewHooks hooks);
    // 원본 접두가 캡처한 로컬 비교를 사용한다. 비로컬 요청은 배열을 지우거나 모양을 조회하지 않는다.
    bool Inspect(CanonPlacementQuery query,bool localOwner) const;
    // 배치 접두와 같은 풀/판본인지 검사한다.
    const SidPool& Pool() const;
private:
    // 현재 표면의 raw 타입/genus/소유자를 읽는다. 일반 자산의 free/dead 상태로 추가 필터링하지 않는다.
    bool CellBlocked(int x,int y,std::int32_t owner) const;
    // 편집기·동일 소유자·활성화된 방향 관계 표를 원본 순서로 판단한다.
    bool Related(std::uint8_t surfaceOwner,std::int32_t owner) const;
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const std::uint8_t> spots_;
    std::span<const std::uint16_t> surfaceMap_;
    CanonPlacementPreviewState& state_;
    CanonPlacementPreviewHooks hooks_;
};
// 배치 접두의 inspectGeometry만 실제 미리보기로 연결하고 모양 조회 훅은 유지한다.
CanonPlacementHooks MakeCanonPlacementPreviewHooks(const SidPool& pool,const RawCanonPlacementPreview& preview,CanonPlacementHooks hooks);
// 실제 타입/패턴 decoder의 정수 범위만 연결하고 충돌/지형 경계는 유지한다.
CanonPlacementPreviewHooks MakeCanonGeometryPreviewHooks(const SidPool& pool,const RawCanonPlacementGeometry& geometry,CanonPlacementPreviewHooks hooks);
}
