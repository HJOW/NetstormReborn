// 일반 타입/패턴 배치의 초기화·즉시 허용·픽셀 여백·지도 경계 접두를 복원한다.
#pragma once
#include "o/RawPriestPlacement.h"

namespace netstorm::o {
class RawCanonPixelShape;
struct CanonPlacementQuery {
    std::uint32_t type{},argument{}; // 타입 번호와 패턴 번호/첫 모양 인자를 별도로 보존한다.
    float x{},y{}; // 원본 스택으로 전달되는 지도 좌표다.
    std::uint32_t flags{},owner{},mode{}; // 방향·부호 BYTE 소유자·충돌 모드를 원본 값으로 전달한다.
};
struct CanonPlacementHooks {
    // 픽셀 getter의 (타입 번호, 별도 첫 인자, 방향)을 받는다. 명시 프레임은 항상 꺼진다.
    std::function<PriestPlacementRect(std::uint32_t,std::uint32_t,std::uint32_t)> shape;
    // 미리보기/충돌/지형/관계 전체는 별도 경계다. 접두의 성공만으로 배치를 허용하지 않는다.
    std::function<bool(const CanonPlacementQuery&,bool)> inspectGeometry;
};
class RawCanonPlacement {
public:
    // 모든 유효 자산 타입의 접두를 공유한다. 자료/상태/경계의 수명은 호출자가 보장한다.
    RawCanonPlacement(const SidPool& pool,std::span<const RiftTypeRecord> types,
        PriestPlacementState& state,CanonPlacementHooks hooks);
    // 0049b510 / CD 00445200: 현재 genus의 강제 허용 뒤 별도 argument로 모양을 조회한다.
    bool MayPlace(CanonPlacementQuery query) const;
    // 같은 풀에 실제 픽셀 getter를 연결하는지 검사한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    PriestPlacementState& state_;
    CanonPlacementHooks hooks_;
};
// 공통 모양 경계만 실제 전체 픽셀 getter로 교체한다. 후반 정책은 호출자가 공급해야 한다.
CanonPlacementHooks MakeCanonShapePlacementHooks(const SidPool& pool,const RawCanonPixelShape& shape,CanonPlacementHooks hooks);
}
