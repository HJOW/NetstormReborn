// C++ 변경 전에 동결한 원본004dab90/0040351d의 전체 픽셀과 실제 Sprite 경로를 대조한다.
#include "ConstructionSupport.h"
#include "client/VFXTransform.h"
#include "client/SpriteDraw.h"

namespace { // 입력/기대 바이트만 읽으며 복원 측 계산으로 기대값을 만들지 않는다.
using namespace netstorm::test::construction;
using namespace netstorm::client;
std::vector<std::uint8_t> TranslatedBytes(const std::string& text) {
    std::vector<std::uint8_t> result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16))); // 원문16진수 두 글자가 한BYTE다.
    return result;
} // 소스 RLE와 원본 표면의 같은 직렬화 규칙이다.
std::int32_t TranslatedSigned(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 원문 signed DWORD를 보존한다.
void ReplayTranslated(bool sprite) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_TRANSLATED_PIXELS_FIXTURE).rows) { // 합성288과실제간헐천8개를생략없이같은순서로읽는다.
        CHECK(row.size()==9);if (row.size()!=9) continue;
        const auto dimensions=Inputs(row[2]),pane=Inputs(row[3]),position=Inputs(row[4]),scale=Inputs(row[5]);
        ShapeDatabase database(TranslatedBytes(row[1]));const auto image=database.Decode(0,0);const auto& frame=database.Blocks()[0].frames[0];
        const auto colorsBytes=TranslatedBytes(row[6]),background=TranslatedBytes(row[7]),expected=TranslatedBytes(row[8]);ColorMap colors{};
        CHECK(colorsBytes.size()==colors.size());std::copy(colorsBytes.begin(),colorsBytes.end(),colors.begin());
        const ScreenRect clip{TranslatedSigned(pane[0]),TranslatedSigned(pane[1]),TranslatedSigned(pane[2])+1,TranslatedSigned(pane[3])+1};
        const ScreenPoint point{TranslatedSigned(position[0]),TranslatedSigned(position[1])};
        const auto compare=[&](const auto& actual) { // 첫차이만진단하되기대표면전체를비교한다.
            const bool same=actual==expected;CHECK(same);
            if (!same) { std::size_t first{};while (first<actual.size() && first<expected.size() && actual[first]==expected[first]) ++first;std::fprintf(stderr,"Translated Pixels %s sprite=%d first=%zu\n",row[0].c_str(),sprite,first); }return same;
        };
        if (!sprite) {
            auto actual=background;DrawTranslatedVfxImage(actual,static_cast<int>(dimensions[0]),static_cast<int>(dimensions[1]),image,frame,point,TranslatedSigned(scale[0]),TranslatedSigned(scale[1]),clip,colors);
            if (!compare(actual)) return;
        } else {
            for (const auto mode:{SpriteShapeMode::Shadow,SpriteShapeMode::Dead,SpriteShapeMode::Abstract}) { // 동일004dab90을부르는세고정표모드를각각확인한다.
                auto actual=background;SpritePixelTarget target(actual,static_cast<int>(dimensions[0]),static_cast<int>(dimensions[1]),clip,
                    [&](auto type) { CHECK(type==70);return PreviewShapeBinding{&database,0}; },[&](auto requested,auto,auto) { CHECK(requested==mode);return colors; });
                auto hooks=target.Hooks();SpriteShapeRequest request{70,0,point,mode};request.scaled=true;request.scaleX=TranslatedSigned(scale[0]);request.scaleY=TranslatedSigned(scale[1]);hooks.shape(request);
                if (!compare(actual)) return;
            }
        }
        ++count;
    }
    CHECK(count==296);
} // 실제커널표면과소유자외세고정표의배포경로를별도로확인한다.
}
TEST_CASE(TranslatedPixels_Original296CompleteSurfaces) { ReplayTranslated(false); } // 두정밀도/재관측888회로동결한기대값이다.
TEST_CASE(TranslatedPixels_SpriteThreeModesUseOriginalDestinationKernel) { ReplayTranslated(true); } //888개실제Sprite출력도같은독립기대값을사용한다.
