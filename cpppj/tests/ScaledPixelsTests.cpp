// 실제 원본 전체 VFX 변환/실제 RLE의 독립 최종 픽셀을 현재 배율 커널과 대조한다.
#include "ConstructionSupport.h"
#include "client/VFXTransform.h"

namespace { // 원문 입력/표면을 읽으며 복원 알고리즘으로 기대값을 만들지 않는다.
using namespace netstorm::test::construction; // 기존 fixture와 DWORD/바이트 도구다.
std::vector<std::uint8_t> Bytes(const std::string& text) { // 압축 입력/완성 원본 표면을 같은 바이트로 읽는다.
    std::vector<std::uint8_t> result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16))); // 두16진수 글자가 한BYTE다.
    return result;
}
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 원문 좌표/배율의 signed DWORD 해석이다.
}

TEST_CASE(ScaledPixels_Original240FullVfxKernelSurfaces) { // pane 원점/clip·RLE투명/255·단위/분수 배율의 전체 표면이다.
    const auto fixture=LoadFixture(NETSTORM_SCALED_PIXELS_FIXTURE).rows;std::size_t count{};
    for (const auto& row:fixture) { // 각 입력은 독립 실제 SHP 해제와 같은 크기의 현재 픽셀 표면을 사용한다.
        CHECK(row.size()==8);if (row.size()!=8) continue;const auto dimensions=Inputs(row[2]),pane=Inputs(row[3]),position=Inputs(row[4]),scale=Inputs(row[5]);
        netstorm::client::ShapeDatabase database(Bytes(row[1]));const auto image=database.Decode(0,0);const auto& frame=database.Blocks()[0].frames[0];std::vector<std::uint8_t> actual(dimensions[0]*dimensions[1],static_cast<std::uint8_t>(Number(row[6])));
        const netstorm::client::ScreenRect clip{Signed(pane[0]),Signed(pane[1]),Signed(pane[2])+1,Signed(pane[3])+1}; // 원문 inclusive pane의 같은 half-open 표면 경계다.
        netstorm::client::DrawScaledVfxImage(actual,static_cast<int>(dimensions[0]),static_cast<int>(dimensions[1]),image,frame,{Signed(position[0]),Signed(position[1])},Signed(scale[0]),Signed(scale[1]),clip);
        const auto expected=Bytes(row[7]);const bool same=actual==expected;CHECK(same);
        if (!same) { std::size_t first{};while (first<actual.size() && first<expected.size() && actual[first]==expected[first]) ++first;std::fprintf(stderr,"Scaled Pixels %s first difference %zu\n",row[0].c_str(),first);break; }++count; // 같은 접두 BYTE를 건너뛰는 while은 첫 차이 위치만 보고한다.
    }
    CHECK(count==240);
}
