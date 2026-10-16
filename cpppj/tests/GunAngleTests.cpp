// 원본 Gunanim의 모든 유효 인덱스·상한·signed 나눗셈을 독립 반환값과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGunAngle.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 음수-4..-1과0..512 및INT_MAX는 모두 원본의 유효 표 범위에 도달한다.
TEST_CASE(GunAngle_Native518) {
    const auto fixture=LoadFixture(NETSTORM_GUN_ANGLE_FIXTURE);CHECK(fixture.rows.size()==519 && fixture.rows.front()[0]=="frame");
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 각 입력의 signed 값과 실제 원문 반환을 그대로 읽는다.
        const auto& row=fixture.rows[index];CHECK(row.size()==2);CHECK(PatchGunGraphFromFrame(static_cast<std::int32_t>(std::stol(row[0])))==Number(row[1]));
    }
}
// 원문이 진단 뒤 표 밖 메모리를 읽는 음수 인덱스는 정상 방향0으로 가장하지 않는다.
TEST_CASE(GunAngle_InvalidNegativeIndex) { CHECK(Throws([] { static_cast<void>(PatchGunGraphFromFrame(-5)); })); }
