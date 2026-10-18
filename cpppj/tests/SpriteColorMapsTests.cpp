// 원본 전체 팔레트 생성기의 독립 세 출력을 모든256번호에 대조한다.
#include "ConstructionSupport.h"
#include "client/SpriteColorMaps.h"
using namespace netstorm::client;
using namespace netstorm::test::construction;
namespace {
// 원본 관측 바이트를 순서대로 읽으며 색 번호를 재계산하지 않는다.
std::vector<std::uint8_t> Bytes(const std::string& hex) {
    std::vector<std::uint8_t> result(hex.size()/2);
    // 각 두 자리의 원본 BYTE를 독립 해석한다.
    for (std::size_t i=0;i<result.size();++i) result[i]=static_cast<std::uint8_t>(std::stoul(hex.substr(i*2,2),nullptr,16));return result;
}
}
// 실제 GIFCLOUD/혼합 RGB/중복 회색에서 밝음·건설·그림자 모두 원본과 같아야 한다.
TEST_CASE(SpriteColorMaps_WholeNativeOutputs) {
    std::size_t count{},different{};
    // 세 입력의 색256개×세 표를 동결한 원문 그대로 대조한다.
    for (const auto& row:LoadFixture(NETSTORM_SPRITE_COLORS_FIXTURE).rows) {
        CHECK(row.size()==6);const auto raw=Bytes(row[2]);std::array<std::uint32_t,256> logical{};
        // RGB/예약 DWORD를 리틀 엔디언 원문에서 읽는다.
        for (std::size_t i=0;i<logical.size();++i) logical[i]=Get(raw,i*4);
        const auto maps=BuildSpriteColorMaps(logical);CHECK(std::ranges::equal(maps.bright,Bytes(row[3])));CHECK(std::ranges::equal(maps.dead,Bytes(row[4])));CHECK(std::ranges::equal(maps.shadow,Bytes(row[5])));
        different+=maps.dead!=maps.shadow;++count;
    }
    CHECK(count==3 && different>=2);
}
