// 배치 decoder의 현재 모양에서 flag 8 표면 지도를 동적으로 순회하고 주변 권한을 누적한다.
#pragma once
#include "o/RawCanonPlacementPermission.h"
#include "o/RawSquidNeighbors.h"

namespace netstorm::o {
struct CanonSurfaceCursor {
    int x{},y{},left{},top{},right{},bottom{}; // 원본 +68..+7c의 정수 커서와 확장 모서리다.
    Sid current{};
    std::vector<std::uint16_t> returned; // 원본 signed WORD 중복 캐시의 반환 순서다.
};
class RawCanonSurfaceWalk {
public:
    // 현재 모양의 float 발자국/중심/프레임 번호는 생성 때 고정하고 지도/raw/코드 표는 매 Next에서 읽는다.
    RawCanonSurfaceWalk(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,
        std::span<const std::uint8_t> spots,const CanonPlacementQuery& query,const CanonPlacementCell& cell);
    // 생성 직후 첫 결과와 이후 결과를 원본처럼 노출한다. 0 반환 뒤에는 종료 상태를 유지한다.
    Sid Current() const;
    Sid Next();
    // 기계어 관찰과 지도 변경 검증에 사용할 실제 커서다.
    const CanonSurfaceCursor& State() const;
private:
    // 표면 타입→접합→죽음/기준점 spot 순서로 flag 8 가상 필터를 적용한다.
    bool Accept(Sid candidate) const;
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    std::span<const std::uint16_t> islands_;
    std::span<const std::uint8_t> spots_;
    std::uint32_t sourceType_{};
    int sourceFrame_{}; // 원본 +44에 저장한 프레임 번호이며 코드 자체는 필터 때 읽는다.
    float left_{},top_{},right_{},bottom_{},centerX_{},centerY_{};
    CanonSurfaceCursor state_;
    bool ended_{};
};
class RawCanonPlacementSurrounding {
public:
    // 실제 후보 권한 helper와 같은 풀을 요구하며 지도/타입/프레임 자료는 호출자 소유다.
    RawCanonPlacementSurrounding(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,
        std::span<const std::uint8_t> spots,const RawCanonPlacementPermission& permission);
    // 권한을 얻어도 남은 후보를 계속 읽으며 요청 owner의 하위 BYTE만 부호 확장한다.
    bool Inspect(const CanonPlacementQuery& query,const CanonPlacementCell& cell) const;
    // 같은 raw 풀을 사용하는 지형 어댑터만 허용한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    std::span<const std::uint16_t> islands_;
    std::span<const std::uint8_t> spots_;
    const RawCanonPlacementPermission& permission_;
};
// 기존 후보/최종 정책을 유지하고 모양 종료의 주변 권한 경계만 실제 flag 8 순회로 바꾼다.
CanonPlacementTerrainHooks MakeCanonSurroundingTerrainHooks(const SidPool& pool,
    const RawCanonPlacementSurrounding& surrounding,CanonPlacementTerrainHooks hooks);
}
