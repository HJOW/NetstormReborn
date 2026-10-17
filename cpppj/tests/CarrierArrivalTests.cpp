// 실제 Carrier 도착의 내용물/현재경로/깊이/다이스/끝 회피 검색을 두 PE에 대조한다.
#include "CarrierArrivalSupport.h"

using namespace netstorm::test::carrierarrival;
// 모든 기대값은 actual PE 반환/요청/부모전체raw이며 하위 큰효과는 명시 경계다.
TEST_CASE(rawCarrierArrival_native_queries_depth_authority_dais_and_obstacles) {
    const auto fixture=LoadFixture(NETSTORM_CARRIERARRIVAL_FIXTURE);std::size_t checked=0;
    // 두 판본의 각 고유 입력을 자체 실제 풀/hash/HP/contained/Graph helper에 연결한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==5);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;ArrivalScene scene(edition,row[1]);const auto result=scene.arrival->Arrival(Sid{50},0,0);CHECK(result==Number(row[2]));CHECK(scene.Effects()==row[3]);CHECK(Hex(scene.pool.Slot(Sid{50}))==row[4]);++checked;
    }
    CHECK(checked==406);
}
// 같은 원문에 현재 getter를 연결하고 반대 값의 예전 bool 표가 분기를 결정하지 않음을 확인한다.
TEST_CASE(CarrierChoice_ArrivalUsesCurrentOwnerAi) {
    std::size_t checked{},queries{}; // 원문 재생 수와 실제 현재 Player 조회 수다.
    // 생성 뒤 현재 값을 바꿔도 매 조회에서 같은 현재 저장소를 사용해야 한다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIERARRIVAL_FIXTURE).rows) {
        std::uint32_t brain{}; // 생성 시0이며 실제 도착 직전에 원문 Player 입력으로 바꾼다.
        ArrivalScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row[1],[&](std::uint32_t owner) { CHECK(owner>0 && owner<9);++queries;return brain!=0; });
        scene.state.controlledOwners[scene.Value("owner")]=scene.Value("controlled")==0;brain=scene.Value("controlled") ? 0x12345678U : 0U;
        CHECK(scene.arrival->Arrival(Sid{50},0,0)==Number(row[2]));CHECK(scene.Effects()==row[3]);CHECK(Hex(scene.pool.Slot(Sid{50}))==row[4]);++checked;
    }
    CHECK(checked==406 && queries>0);
}
