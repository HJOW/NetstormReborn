// 패턴 없는 사제 CanonDecoder의 실제 한 칸 순회/미리보기 범위/충돌 사각형을 연결한다.
#pragma once
#include "o/CanonDecoder.h"
#include "o/RawPriestPlacementCollision.h"

namespace netstorm::o {
struct PriestPlainCanonType {
    RiftTypeFrames frames{std::vector<FrameCode>{}}; // 타입 +0x114/+0x124의 실제 프레임 코드 표다.
    int defaultFrame{}; // 타입 +0x118: -1이면 유효 칸을 찾지 못한다.
};
struct PriestPlacementGeometryState {
    // CanonDecoder 생성에서 패턴으로 분기하는 네 특수 타입과 네 추가 타입의 현재 번호다.
    std::array<std::uint32_t,8> patternTypes{};
};
struct PriestPlacementGeometryHooks {
    // decoder 생성 뒤의 지역 판정 초기화 경계다. 실제 후보/지형 전역의 수명은 호출자가 연결한다.
    std::function<void(const PriestPlacementQuery&)> beginRegions;
    // 한 모양의 finder/후보 소진 뒤 지형/주변 관계 처리 경계다. false이면 즉시 거부한다.
    std::function<bool(const PriestPlacementQuery&,int,float,float)> endShape;
    // 모든 모양 진행 뒤 최종 지역/관계 판정 경계다. 일반 자산 몸체와 구분한다.
    std::function<bool(const PriestPlacementQuery&)> finishRegions;
    // finder 탐색 직전 모양별 지역 배열/발자국 원점을 준비한다. 기존 외부 경계에는 선택 사항이다.
    std::function<void(const PriestPlacementQuery&,int,float,float)> beginShape;
};
class RawPriestPlacementGeometry {
public:
    // 사제 genus·현재 패턴 타입 전역·프레임 메타 자료로 지원하는 비패턴 경로를 확인한다.
    RawPriestPlacementGeometry(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,PriestPlacementGeometryState& state,PriestPlacementGeometryHooks hooks);
    // 실제 비패턴 생성/진행과 현재 타입 발자국으로 미리보기 범위를 계산한다. frame=-1도 계산한다.
    SquidSearchArea Bounds(PriestPlacementQuery query) const;
    // 실제 한 칸 순회와 모양별 정수 사각형을 scan에 공급하며 거부 이후 진행/후처리를 호출하지 않는다.
    bool Inspect(PriestPlacementQuery query,const std::function<bool(SquidSearchArea)>& scan) const;
    // 0049b8c0..0049b95a / CD 004455ab..00445641: 원본 모양 좌표와 현재 발자국의 finder 인자다.
    static SquidSearchArea CollisionArea(float x,float y,int footX,int footY);
    // 어댑터는 같은 풀/판본에만 연결한다.
    const SidPool& Pool() const;
private:
    // 현재 genus/패턴 전역을 검사하고 실제 decoder를 구성한다.
    CanonDecoder Decode(const PriestPlacementQuery& query) const;
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    PriestPlacementGeometryState& state_;
    PriestPlacementGeometryHooks hooks_;
};
// 미리보기 bounds만 실제 비패턴 decoder에 연결하고 후보 충돌 경계는 유지한다.
PriestPlacementPreviewHooks MakePriestGeometryPreviewHooks(const SidPool& pool,
    const RawPriestPlacementGeometry& geometry,PriestPlacementPreviewHooks hooks);
// 후보 검사기의 모양 순회를 실제 decoder로 연결하고 후보별 지형 효과 경계는 유지한다.
PriestPlacementCollisionHooks MakePriestGeometryCollisionHooks(const SidPool& pool,
    const RawPriestPlacementGeometry& geometry,PriestPlacementCollisionHooks hooks);
}
