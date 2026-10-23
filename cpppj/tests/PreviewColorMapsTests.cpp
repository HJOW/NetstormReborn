// 원본 전체43c010이 남긴 세256바이트 출력을 복원 색표에 대조한다.
#include "ConstructionSupport.h"
#include "client/PreviewColorMaps.h"
#include "client/OwnerColorMaps.h"
#include "o/RawColorSortPrimary.h"
using namespace netstorm::client;
using namespace netstorm::test::construction;
namespace {
// 원본 관측의 실제 바이트 순서를 유지해 읽는다.
std::vector<std::uint8_t> ColorBytes(const std::string& hex) {
    std::vector<std::uint8_t> result(hex.size()/2);
    // 주변 바이트를 섞지 않고 각 원본 BYTE를 읽는다.
    for (std::size_t index=0;index<result.size();++index) result[index]=static_cast<std::uint8_t>(std::stoul(hex.substr(index*2,2),nullptr,16));return result;
}
}
// 실제 파일/혼합 RGB/중복 회색의 모든 출력 번호를 원본 fixture 그대로 비교한다.
TEST_CASE(PreviewColorMaps_ActualWholeNativeOutputs) {
    std::size_t count{};
    // 세 입력을 각각 독립 논리 팔레트로 재생한다.
    for (const auto& row:LoadFixture(NETSTORM_PREVIEW_COLORS_FIXTURE).rows) {
        CHECK(row.size()==6);const auto raw=ColorBytes(row[2]);std::array<std::uint32_t,256> logical;
        // 원본의 R/G/B/예약 DWORD를 리틀 엔디언으로 읽는다.
        for (std::size_t index=0;index<logical.size();++index) logical[index]=Get(raw,index*4);
        const auto maps=BuildPreviewColorMaps(logical,Number(row[1]));CHECK(std::ranges::equal(maps.destroy,ColorBytes(row[3])));CHECK(std::ranges::equal(maps.illegal,ColorBytes(row[4])));CHECK(std::ranges::equal(maps.labelShadow,ColorBytes(row[5])));++count;
    }
    CHECK(count==3);
}

// 실제 CRT가 선택한53비트 기본 출력에 다섯 전용 표/모든 소유자/모든 소스 번호를 대조한다.
TEST_CASE(PreviewColorMaps_SpecialOwnerMatchesActualDefaultPrecision) {
    std::size_t count{},different{};
    // 64비트 별도 관측은 보존하되 기본 출력의 기대값으로 혼합하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_OWNER_HSL_COLORS_FIXTURE).rows) {
        CHECK(row.size()==5);const auto raw=ColorBytes(row[2]);std::array<std::uint32_t,256> logical;
        // 현재 관측의 원본 R/G/B DWORD를 재생한다.
        for (std::size_t index=0;index<logical.size();++index) logical[index]=Get(raw,index*4);
        const auto maps=BuildSpecialOwnerColorMaps(logical,Number(row[1]));const auto expected=ColorBytes(row[3]);bool same=true;
        std::array<std::uint8_t,netstorm::o::kColorSortPrimaryBytes> allocation;allocation.fill(0xa7); // 동일 원문 RGBX를 raw constructor에 주며 미작성 padding은 별도 입력이다.
        netstorm::o::RawColorSortPrimary primary(allocation);primary.Construct(raw); // 최초 Native 15행을 현재 저장 L 경로에도 그대로 재생한다.
        const auto storedMaps=BuildSpecialOwnerColorMaps(primary,Number(row[1])); // RGB adapter 결과를 raw의 기대값으로 대체하지 않는다.
        // 중립0/소유자1..8의 전체2304바이트를 한 번씩 검사한다.
        for (std::size_t owner=0;owner<maps.size();++owner) { // 두 소비자의 전체 2304BYTE를 같은 독립 Native 출력에 각각 대조한다.
            same=same && std::equal(maps[owner].begin(),maps[owner].end(),expected.begin()+owner*256);
            CHECK(std::equal(storedMaps[owner].begin(),storedMaps[owner].end(),expected.begin()+owner*256));
        }
        CHECK(same);if (!same) std::printf("소유자 색표 입력 %s/%s 불일치\n",row[0].c_str(),row[1].c_str());different+=row[3]!=row[4];++count;
    }
    CHECK(count==15 && different>0);
}
