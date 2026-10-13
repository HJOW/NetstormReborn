// 사제 실제 +3c의 중립 회복/내용물 부착·삭제와 공통 CarrierArrival 위임을 복원한다.
#pragma once
#include "o/RawCarrierArrival.h"
#include "o/RawCanonPlacementTerrain.h"
#include "o/RawIslandRuntime.h"

namespace netstorm::o {
// 현재 권한/로딩/중립 재배치 억제는 하위 효과 뒤에도 다시 읽는다.
struct PriestArrivalState {
    bool boss{true},loading{},suppressNeutralReturn{}; //00540bc4/005c89b8/00594fa4 및 CD00540a2c/005178d4/0052e170이다.
};
// 선택된 부모 자기삭제/큰 귀환 몸체는 빈 효과로 완료 처리하지 않는다.
struct PriestArrivalHooks {
    std::function<void(Sid,std::uint32_t)> destroy; // 실제 사제 가상+10의 flags0 자기삭제다.
    std::function<void(Sid)> neutralReturn; //00491db0/CD0040d030의 속도/귀환/임의 위치 이동 몸체다.
};
// 기존 실제 현재HP/MaxHP/지역/섬Temple/발자국/내용물 생성/공통 도착을 같은 풀에 연결한다.
class RawPriestArrival {
public:
    // 참조된 월드/metadata/상태/하위 효과는 이 객체보다 오래 살아야 한다.
    RawPriestArrival(const SidPool& pool,const RawPriestState& priest,const SquidReward& hitPoints,
        const RawCanonPlacementTerrain& terrain,const RawIslandRuntime& islands,const RawSquidNeighbors& footprint,
        const RawContainedCreate& create,const RawCarrierArrival& carrier,PriestArrivalState& state,PriestArrivalHooks hooks);
    // 실제00491f70/CD0040d200 전체 부모 wrapper이며 일반 분기는 CarrierArrival을 직접 호출한다.
    std::uint32_t Arrival(Sid sid,std::uint32_t targetA,std::uint32_t targetB) const;
    //0046f950/CD004bd9f0: 현재 plain 좌표의 지역→현재 섬 Temple DWORD를 읽는다.
    Sid TempleAt(float x,float y) const;
    // 실제 PathProcess 어댑터가 같은 월드의 사제 Arrival만 연결하도록 확인한다.
    const SidPool& Pool() const;
private:
    // 현재 raw 좌표/owner는 하위 callback 뒤 다시 읽는 원본 저장소 입력이다.
    std::array<float,2> Position(Sid sid) const;
    std::uint32_t Owner(Sid sid) const;
    bool NeutralEligible(Sid sid) const; // CD0040c3d0과 패치 인라인 중립 조건이다.
    bool NearTemple(Sid sid,Sid temple) const; //0041d840/CD00440b20의 축별 signed거리와1.0 비교다.
    const SidPool& pool_; // 현재 사제/섬Temple SID가 속한 raw 풀이다.
    const RawPriestState& priest_; // 실제 현재 signed DWORD/WORD HP다.
    const SquidReward& hitPoints_; // 현재 최대HP/모드의 기존 몸체다.
    const RawCanonPlacementTerrain& terrain_; // 실제 현재 지면 WORD/genus/지역 getter를 재사용한다.
    const RawIslandRuntime& islands_; // 같은 현재 섬목록 Temple getter를 재사용한다.
    const RawSquidNeighbors& footprint_; // 기존 실제 float 발자국 사각형이다.
    const RawContainedCreate& create_; // 기존 실제 내용물 Factory/Form 부착이다.
    const RawCarrierArrival& carrier_; // 일반 부모 도착의 전체 현재 body다.
    PriestArrivalState& state_; // 효과 뒤 현재 권한/모드를 다시 읽는다.
    PriestArrivalHooks hooks_; // 선택된 실제 자기삭제/중립 귀환 효과다.
};
}
