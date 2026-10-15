// 실제 두 PE의 지면 소유자와 float snap/CRT 결과를 같은 raw Pool/hash0 입력으로 대조한다.
#include "ConstructionSupport.h"
#include "o/RawRegionOwner.h"
#include <fstream>
#include <sstream>

using namespace netstorm::o;
using namespace netstorm::test::construction;
// native 기계어가 고른 결과만 비교하며 정답 좌표/소유자를 테스트에서 다시 계산하지 않는다.
TEST_CASE(RegionOwner_IndependentPeFixture) {
    std::ifstream input(NETSTORM_REGION_OWNER_FIXTURE);CHECK(input.good());std::string line;std::size_t count=0;
    SidPool patch(OriginalEdition::Patch1078,32768),cd(OriginalEdition::Cd1072,32768);std::vector<std::uint16_t> surface(65536);
    // 원본 입력을 행 순서대로 재사용 가능한 같은 Pool에 전달한다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::string edition,operation;std::uint32_t x{},y{},sid{},terrain{},actual{},owner{},state{},expected{};
        row>>edition>>operation>>x>>y>>sid>>terrain>>actual>>owner>>state>>expected;CHECK(!row.fail());
        const bool isPatch=edition=="originals";auto& pool=isPatch ? patch : cd;auto raw=InputRaw(pool,Sid{5});
        raw[10]=static_cast<std::uint8_t>(actual);raw[11]=static_cast<std::uint8_t>(state);raw[isPatch ? 34U : 32U]=static_cast<std::uint8_t>(owner);
        std::fill(surface.begin(),surface.end(),static_cast<std::uint16_t>(sid));RawRegionOwner getter(pool,surface,terrain);
        const auto observed=operation=="Integer" ? getter.At(std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)) :
            getter.At(std::bit_cast<float>(x),std::bit_cast<float>(y));CHECK(observed==expected);++count;
    }
    CHECK(count==1632);
}
// 모든 상태와 type/owner/표면 교체가 복사본 없이 다음 실제 조회에 보이는지 검사한다.
TEST_CASE(RegionOwner_CurrentRawInputsAndBounds) {
    SidPool pool(OriginalEdition::Patch1078,32768);std::vector<std::uint16_t> surface(65536);std::uint32_t terrain=97;
    auto raw=InputRaw(pool,Sid{5});raw[10]=97;raw[11]=7;raw[34]=255;surface[0]=5;RawRegionOwner getter(pool,surface,terrain);
    CHECK(getter.At(0,0)==255 && &getter.Pool()==&pool);raw[34]=2;CHECK(getter.At(0.0F,0.0F)==2);
    terrain=98;CHECK(getter.At(0,0)==0);raw[10]=98;CHECK(getter.At(0,0)==2);surface[0]=0;CHECK(getter.At(0,0)==0);
    CHECK(getter.At(-1,0)==0 && getter.At(256,0)==0 && getter.At(0,256)==0);
    bool rejected=false;try { RawRegionOwner invalid(pool,std::span<const std::uint16_t>(surface).first(65535),terrain); }
    catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
