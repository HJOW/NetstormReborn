// 독립 원본 색표의 전체 바이트와 초기 격자의0번 부모 조회를 검사한다.
#include "ConstructionSupport.h"
#include "client/ProductionColorMaps.h"
#include "o/RawContainedFinder.h"
using namespace netstorm::client;
using namespace netstorm::test::construction;
// 세 원본 팔레트의 두 출력 표를 재계산한 기대값 없이 읽는다.
TEST_CASE(ProductionColorMaps_NativeAllEntries) {
    std::size_t count{};
    // 실제 관측의 모든 입력/출력을 순서대로 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_PRODUCTION_COLORS_FIXTURE).rows) {
        CHECK(row.size()==5);std::array<std::uint32_t,256> logical{};std::array<std::array<std::uint8_t,256>,2> expected{};
        // DWORD 입력은 원문 리틀 엔디언 바이트로 복원한다.
        for (std::size_t i=0;i<logical.size();++i) for (std::size_t b=0;b<4;++b) logical[i]|=static_cast<std::uint32_t>(std::stoul(row[2].substr((i*4+b)*2,2),nullptr,16))<<(b*8);
        // 출력 두 표는 원본의256바이트를 그대로 읽는다.
        for (std::size_t t=0;t<2;++t) for (std::size_t i=0;i<256;++i) expected[t][i]=static_cast<std::uint8_t>(std::stoul(row[t+3].substr(i*2,2),nullptr,16));
        const auto maps=BuildProductionColorMaps(logical);CHECK(maps.disabled==expected[0]);CHECK(maps.underlay==expected[1]);++count;
    }
    CHECK(count==3);
}
// 부모0을 가짜 빈 목록으로 처리하지 않고 실제0번 슬롯의 head/다음/타입을 읽는다.
TEST_CASE(ContainedFinder_ZeroParentReadsActualHead) {
    netstorm::o::SidPool pool(netstorm::o::OriginalEdition::Patch1078,65535);netstorm::o::ContainedFinderState state;
    netstorm::o::RawContainedFinder finder(pool,state);CHECK(finder.Begin({}).value==0);
    const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);raw[10]=6;raw[4]=raw[5]=0;
    auto* zero=const_cast<std::uint8_t*>(pool.Slot({}).data());zero[6]=static_cast<std::uint8_t>(sid.value);zero[7]=static_cast<std::uint8_t>(sid.value>>8);
    CHECK(finder.Begin({})==sid);CHECK(finder.Next().value==0);raw[10]=3;CHECK(finder.Begin({}).value==0);
}
