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
