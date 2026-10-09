// outpost·작업장 지역 투표와 판본별 도장 요청 순서를 복원한다.
#pragma once
#include "o/RawOutpostLifecycle.h"

namespace netstorm::o {
class RawCanonPlacementTerrain;
struct OwnershipAffectedRegion {
    float x{},y{}; // 패치의 12바이트 누적 기록에서 좌표는 투표에 사용하지 않는다.
    std::uint32_t region{};
};
struct RegionOwnershipState {
    std::vector<OwnershipAffectedRegion> affected;
    std::uint32_t revision{},dirty{}; // 투표 wrapper 자체의 DWORD 쓰기다. 지형 도장의 내부 증가는 별도다.
};
struct RegionOwnershipPaint {
    int x{},y{}; // 원래 query float를 편향 없이 절삭한다.
    std::uint32_t owner{},theme{};
};
struct RegionOwnershipHooks {
    // 실제 좌표 snap/지역 getter는 아래 terrain 어댑터로 연결한다.
    std::function<std::uint32_t(float,float)> regionAt;
    // 00470900 / CD 004bd2d0의 flood/지형 도장은 필수 외부 경계다.
    std::function<void(const RegionOwnershipPaint&)> paint;
    // 패치의 theammode 정수 조회다. 일치하는 작업장마다 새로 조회한다.
    std::function<std::uint32_t()> themeMode;
};
class RawRegionOwnership {
public:
    // 작업장 목록은 factories/nearest와 구별한다. theme은 타입 raw +0x98의 별도 입력 표다.
    RawRegionOwnership(const SidPool& pool,SquidPostPopList& additional,SquidPostPopList& workshops,
        std::span<const std::uint32_t> themes,RegionOwnershipState& state,RegionOwnershipHooks hooks);
    // 00455380 / CD 0047b3c0: 동률/중립과 패치 mode 10 누적 재투표를 처리한다.
    void Update(float x,float y,std::uint32_t mode=0) const;
    // 실제 outpost/terrain과 같은 풀을 사용하는지 검사하는 참조다.
    const SidPool& Pool() const;
private:
    // 현재 목록 순서로 지역과 소유자를 읽고 같은 표 배열에 누적한다.
    void Vote(SquidPostPopList& list,float x,float y,std::array<std::int32_t,9>& votes,
        const std::size_t* affectedIndex) const;
    const SidPool& pool_;
    SquidPostPopList& additional_;
    SquidPostPopList& workshops_;
    std::span<const std::uint32_t> themes_;
    RegionOwnershipState& state_;
    RegionOwnershipHooks hooks_;
};
// 기존 WORD SID/genus getter 앞에 원본 0.99999 float 편향 snap을 연결한다.
RegionOwnershipHooks MakeTerrainRegionOwnershipHooks(const SidPool& pool,const RawCanonPlacementTerrain& terrain,
    RegionOwnershipHooks hooks);
// outpost 등록/삭제 뒤의 필수 지역 통지를 실제 투표 모듈로 교체한다.
OutpostLifecycleHooks MakeOutpostRegionOwnershipHooks(const SidPool& pool,const RawRegionOwnership& ownership,
    OutpostLifecycleHooks hooks);
}
