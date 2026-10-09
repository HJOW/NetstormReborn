// 일반 타입과 특수 패턴의 배치 모양 순회/실제 finder 사각형을 공유한다.
#pragma once
#include "o/CanonTypeDecoder.h"
#include "o/RawPriestPlacementGeometry.h"

namespace netstorm::o {
struct CanonPlacementCell {
    int frame{},label{}; // 현재 decoder 프레임과 패턴의 셀 라벨이다.
    std::uint8_t side{}; // 조회 가능한 실제 프레임 코드의 방향 문자다. 메타 코드가 없으면 0이다.
    float x{},y{},snappedX{},snappedY{}; // 원래 좌표와 지형 원점에 쓰는 절삭한 float 좌표다.
    SquidSearchArea area; // 현재 타입 발자국으로 계산한 finder의 네 정수 인자다.
};
struct CanonPlacementGeometryHooks {
    // decoder 생성/첫 프레임 선택 뒤 한 번 호출하는 지역 초기화 경계다.
    std::function<void(const CanonTypeQuery&)> beginRegions;
    // finder 탐색 전에 현재 모양의 배열/지형 원점을 준비하는 선택 경계다.
    std::function<void(const CanonTypeQuery&,const CanonPlacementCell&)> beginShape;
    // 후보 소진 뒤의 모양별 지형/관계 판정이다. false이면 후속 칸을 실행하지 않는다.
    std::function<bool(const CanonTypeQuery&,const CanonPlacementCell&)> endShape;
    // 모든 유효 칸이 끝났을 때의 최종 지역/관계 판정이다. 빈 decoder에도 호출한다.
    std::function<bool(const CanonTypeQuery&)> finishRegions;
};
class RawCanonPlacementGeometry {
public:
    // 패턴 전역/프레임/타입 자료의 수명은 호출자가 보장한다. 지형 정책은 Inspect에 명시한다.
    RawCanonPlacementGeometry(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,PriestPlacementGeometryState& state);
    // 패턴 번호/명시 프레임을 별도로 받아 첫 현재 좌표와 전체 순회 개수로 범위를 구한다.
    SquidSearchArea Bounds(CanonTypeQuery query) const;
    // 모든 칸에 준비→finder/후보→지형 종료→Advance를 수행한다. 거부 뒤 최종 판정을 생략한다.
    bool Inspect(CanonTypeQuery query,const std::function<bool(const CanonPlacementCell&)>& scan,
        const CanonPlacementGeometryHooks& hooks) const;
    // 0049b8c0..0049b95a / CD 004455ab..00445641의 실제 finder 인자 산술이다.
    static SquidSearchArea CollisionArea(float x,float y,int footX,int footY);
    // 동일 풀/판본 연결 확인을 위해 원래 풀을 제공한다.
    const SidPool& Pool() const;
private:
    // 현재 타입/메타 자료와 타입 전역의 우선순위로 decoder를 구성한다.
    CanonDecoder Decode(CanonTypeQuery query) const;
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    PriestPlacementGeometryState& state_;
};
}
