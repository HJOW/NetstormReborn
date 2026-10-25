// outpost·작업장 지역 투표와 판본별 도장 요청 순서를 복원한다.
#pragma once
#include "o/RawOutpostLifecycle.h"

namespace netstorm::o {
class RawCanonPlacementTerrain;
// 소유권 영향받은 영향을 받는 지역 정보이다.
struct OwnershipAffectedRegion {
    float x{},y{}; // 패치의 12바이트 누적 기록에서 좌표는 투표에 사용하지 않는다.
    // region: 지역.
    std::uint32_t region{};
};
// 지역 소유권 처리 중 조회·갱신하는 상태 묶음이다.
struct RegionOwnershipState {
    // affected: 소유권 변경의 영향을 받아 갱신할 지역 목록.
    std::vector<OwnershipAffectedRegion> affected;
    std::uint32_t revision{},dirty{}; // 독립 입력의 패치0059a8b0 증가/005949d4의1쓰기다. 지형 도장의 내부 증가는 같은 원본 주소에 별도 수행한다.
    std::uint32_t* sharedWorldRedraw{};std::uint32_t* sharedMapRevision{}; // 패치0059a8b0 증가와005949d4의1쓰기를 서로 다른 실제 owner에서 빌린다.
    std::uint32_t& WorldRedraw() { return sharedWorldRedraw ? *sharedWorldRedraw : revision; } // 투표 효과 뒤 현재 월드 다시 그리기 DWORD를 직접 증가시킨다.
    const std::uint32_t& WorldRedraw() const { return sharedWorldRedraw ? *sharedWorldRedraw : revision; } // 독립 입력만 기존 revision 필드를 읽는다.
    std::uint32_t& MapRevision() { return sharedMapRevision ? *sharedMapRevision : dirty; } // 같은 미니맵 DWORD에는 증가 대신 원문1을 쓴다.
    const std::uint32_t& MapRevision() const { return sharedMapRevision ? *sharedMapRevision : dirty; } // 미니맵 Refresh가0으로 소비하는 현재값이다.
};
// 지역 소유권 표시에 적용할 소유자·테마 정보이다.
struct RegionOwnershipPaint {
    int x{},y{}; // 원래 query float를 편향 없이 절삭한다.
    // owner: 객체나 지역을 소유한 플레이어 번호.
    // theme: 테마.
    std::uint32_t owner{},theme{};
};
// 지역 소유권 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct RegionOwnershipHooks {
    // 실제 좌표 snap/지역 getter는 아래 terrain 어댑터로 연결한다.
    std::function<std::uint32_t(float,float)> regionAt;
    // 00470900 / CD 004bd2d0의 flood/지형 도장은 필수 외부 경계다.
    std::function<void(const RegionOwnershipPaint&)> paint;
    // 패치의 theammode 정수 조회다. 일치하는 작업장마다 새로 조회한다.
    std::function<std::uint32_t()> themeMode;
};
// outpost·작업장 지역 투표와 판본별 도장 요청 순서를 복원한다.
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
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // additional_: 추가.
    SquidPostPopList& additional_;
    // workshops_: 워크샾.
    SquidPostPopList& workshops_;
    // themes_: 테마.
    std::span<const std::uint32_t> themes_;
    // state_: 처리 중 조회·갱신하는 상태 (RegionOwnershipState).
    RegionOwnershipState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (RegionOwnershipHooks).
    RegionOwnershipHooks hooks_;
};
// 기존 WORD SID/genus getter 앞에 원본 0.99999 float 편향 snap을 연결한다.
RegionOwnershipHooks MakeTerrainRegionOwnershipHooks(const SidPool& pool,const RawCanonPlacementTerrain& terrain,
    RegionOwnershipHooks hooks);
// outpost 등록/삭제 뒤의 필수 지역 통지를 실제 투표 모듈로 교체한다.
OutpostLifecycleHooks MakeOutpostRegionOwnershipHooks(const SidPool& pool,const RawRegionOwnership& ownership,
    OutpostLifecycleHooks hooks);
}
