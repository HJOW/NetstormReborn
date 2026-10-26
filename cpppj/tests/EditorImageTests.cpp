// 실제 Image 변환/인코딩/복사/소멸을 독립 원본의 전체 shape 바이트와 비교한다.
#include "RawSceneSupport.h"
#include "client/RawImage.h"
#include "client/VFXDraw.h"
#include <algorithm>
#include <cstdio>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 원본 관측 입력/기대 바이트만 해석하는 검사 도구다.
// 출력 shape를 재계산하지 않고 TSV의 명시 hex를 바이트 배열로 읽는다.
std::vector<std::uint8_t> EditorImageBytes(const std::string& text) {
    std::vector<std::uint8_t> result;
    for (std::size_t offset=0;offset<text.size();offset+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(offset,2),nullptr,16))); // 각 바이트는 독립16진수다.
    return result;
}
}

// x87 두 모드·삼색 전 조합·긴 run·실제 지형 Bitmap을 같은 Native 행으로 검증한다.
TEST_CASE(EditorImage_ReplaysNativeCropEncoderCopyAndDestroy) {
    const auto fixture=LoadFixture(NETSTORM_EDITOR_IMAGE_FIXTURE);std::uint32_t checked{};
    for (const auto& row:fixture.rows) { // Native 기대 shape를 직접 읽으며 제품 인코더로 기대값을 생성하지 않는다.
        if (row[0]=="name") continue;
        const auto pixels=EditorImageBytes(row[4]);const auto width=static_cast<std::int32_t>(Number(row[1])),height=static_cast<std::int32_t>(Number(row[2]));
        RawImage image;image.Convert({width,height,pixels},Number(row[3])!=0);
        CHECK(image.Width()==static_cast<std::int32_t>(Number(row[6])) && image.Height()==static_cast<std::int32_t>(Number(row[7])));
        const auto expected=EditorImageBytes(row[9]);if (Hex(image.Shape())!=row[9]) std::fprintf(stderr,"Image Native행%u %s shape 불일치\n",checked,row[0].c_str());
        CHECK(image.ShapeSize()==Number(row[8]) && std::equal(image.Shape().begin(),image.Shape().end(),expected.begin(),expected.end()) && image.Bits().empty());
        RawImage copy;copy.CopyFrom(image);const auto expectedCopy=EditorImageBytes(row[10]);
        CHECK(copy.Shape().data()!=image.Shape().data() && std::equal(copy.Shape().begin(),copy.Shape().end(),expectedCopy.begin(),expectedCopy.end()));
        CHECK(copy.Width()==image.Width() && copy.Height()==image.Height() && copy.ShapeSize()==image.ShapeSize());
        ShapeDatabase database(expected);std::vector<std::uint8_t> actual(static_cast<std::size_t>(image.Width())*image.Height());
        const auto status=DrawShape(actual,image.Width(),image.Height(),database,0,0,image.Width()-1,image.Height()-1,{0,0,image.Width()-1,image.Height()-1});
        if (database.Blocks()[0].frames[0].IsSpecial()) CHECK(std::all_of(pixels.begin(),pixels.end(),[](auto value) { return value==0; }));
        else {
            CHECK(status==0);
            for (std::int32_t y=0;y<image.Height();++y) { // 실제 기존 RLE renderer가 crop 영역을 원래 픽셀로 그리는지도 검사한다.
                const auto first=static_cast<std::size_t>(y)*width;CHECK(std::equal(actual.begin()+static_cast<std::ptrdiff_t>(y)*image.Width(),actual.begin()+static_cast<std::ptrdiff_t>(y+1)*image.Width(),pixels.begin()+static_cast<std::ptrdiff_t>(first)));
            }
        }
        image.Destroy();image.Destroy();copy.Destroy();copy.Destroy();
        CHECK(image.Width()==0 && image.Height()==0 && image.Shape().empty() && image.Bits().empty() && image.ShapeSize()==Number(row[8]));
        CHECK(copy.Width()==0 && copy.Height()==0 && copy.Shape().empty() && copy.ShapeSize()==Number(row[8]));++checked;
    }
    CHECK(checked==1570);
}
