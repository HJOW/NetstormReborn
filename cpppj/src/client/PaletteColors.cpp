#include "client/PaletteColors.h"
#include "client/Screen.h"

namespace netstorm::client {
namespace {
// 004a4850의 58개 검색 인자 순서다. 주석 주소는 결과를 저장하는 10.78 전역이며 용도 미확정 항목의 추적에 쓴다.
constexpr std::array<std::array<std::int32_t,3>,kPatchPaletteColorCount> kNamedRgb{{
    {0,0,0},         // 기본 검정: 005a4058.
    {255,22,22},     // 기본 빨강: 005b5e58.
    {22,255,22},     // 기본 초록: 005acd20.
    {22,22,255},     // 기본 파랑: 005ad128.
    {255,22,255},    // 기본 자홍: 005b5da0.
    {255,255,22},    // 기본 노랑: 0059afd8.
    {240,61,0},      // 기본 주황: 005bec34.
    {255,255,255},   // 기본 흰색: 005bec2c.
    {128,128,128},   // 기본 회색: 005bec3c.
    {181,140,111},   // 해 원소의 갈색: 005acd24.
    {95,188,92},     // 목표 RGB 결과: 005b5dc4.
    {151,215,210},   // 목표 RGB 결과: 005a4060.
    {0,255,102},     // 10.78 추가 목표 RGB: 0059af80.
    {255,102,204},   // 10.78 추가 목표 RGB: 005b5e6c.
    {255,153,0},     // 10.78 추가 목표 RGB: 005b5dcc.
    {51,255,102},    // 10.78 추가 목표 RGB: 0059afd4.
    {255,0,153},     // 10.78 추가 목표 RGB: 005b5dd0.
    {99,66,255},     // 10.78 추가 목표 RGB: 005b5e54.
    {0,128,0},       // 10.78 추가 목표 RGB: 005acd1c.
    {184,147,20},    // 10.78 추가 목표 RGB: 0059afd0.
    {233,214,186},   // 목표 RGB 결과: 005a4064.
    {200,175,144},   // 목표 RGB 결과: 005b5e5c.
    {249,233,145},   // 목표 RGB 결과: 005bec30.
    {219,213,203},   // 목표 RGB 결과: 005b5dc8.
    {215,207,119},   // 목표 RGB 결과: 0059b3e0.
    {45,62,32},      // 목표 RGB 결과: 005b5df0.
    {17,66,60},      // 목표 RGB 결과: 005b5e68.
    {17,66,60},      // 같은 RGB의 별도 전역: 005b5dbc.
    {108,57,54},     // 목표 RGB 결과: 0059b3e4.
    {74,58,56},      // 목표 RGB 결과: 005b5da4.
    {114,106,103},   // 목표 RGB 결과: 005b5e60.
    {250,248,238},   // 목표 RGB 결과: 005b5dd4.
    {84,80,81},      // 목표 RGB 결과: 005b5dc0.
    {206,193,178},   // 목표 RGB 결과: 005b5dd8.
    {255,255,255},   // 기본 흰색과 같은 RGB의 별도 전역: 005a405c.
    {128,128,128},   // 기본 회색과 같은 RGB의 별도 전역: 005b5e64.
    {47,24,17},      // 목표 RGB 결과: 005c78d0.
    {55,51,54},      // 돌 버튼 부모의 외곽선: 005bec38(CD 0055287c).
    {88,121,67},     // 목표 RGB 결과: 005b5e44.
    {111,146,48},    // 목표 RGB 결과: 005b5e48.
    {153,179,55},    // 목표 RGB 결과: 005b5e4c.
    {216,205,57},    // 목표 RGB 결과: 005b5e50.
    {54,100,130},    // 목표 RGB 결과: 005accfc.
    {191,75,71},     // 목표 RGB 결과: 005acd00.
    {204,193,180},   // 목표 RGB 결과: 005acd04.
    {91,129,50},     // 목표 RGB 결과: 005acd08.
    {127,113,182},   // 목표 RGB 결과: 005acd0c.
    {226,185,82},    // 목표 RGB 결과: 005acd10.
    {53,122,112},    // 목표 RGB 결과: 005acd14.
    {238,126,40},    // 목표 RGB 결과: 005acd18.
    {146,168,186},   // 목표 RGB 결과: 005c78b0.
    {223,133,116},   // 목표 RGB 결과: 005c78b4.
    {249,247,239},   // 목표 RGB 결과: 005c78b8.
    {118,173,105},   // 목표 RGB 결과: 005c78bc.
    {178,172,215},   // 목표 RGB 결과: 005c78c0.
    {223,206,108},   // 목표 RGB 결과: 005c78c4.
    {129,192,185},   // 목표 RGB 결과: 005c78c8.
    {238,144,41}     // 목표 RGB 결과: 005c78cc.
}};
}
// 독립 x86으로 대조한 기존 검색 몸체를 사용하고 여기서는 원본 인자와 별칭 대입 순서를 유지한다.
PaletteColorTable BuildPaletteColorTable(std::span<const std::uint32_t,256> logical,o::OriginalEdition edition) {
    PaletteColorTable result;result.named.reserve(edition==o::OriginalEdition::Patch1078 ? kPatchPaletteColorCount : kCdPaletteColorCount);
    // CD판에 없는 12~19번 추가 색만 건너뛰고 원본 호출 순서를 유지한다.
    for (std::size_t i=0;i<kNamedRgb.size();++i) {
        if (edition!=o::OriginalEdition::Patch1078 && i>=12 && i<20) continue;
        const auto& rgb=kNamedRgb[i];const auto index=FindPaletteColor(logical,rgb[0],rgb[1],rgb[2]);
        result.named.push_back({static_cast<std::uint8_t>(rgb[0]),static_cast<std::uint8_t>(rgb[1]),static_cast<std::uint8_t>(rgb[2]),index});
        if (i<result.basic.size()) result.basic[i]=index;
    }
    result.weather={result.basic[5],result.basic[3],result.basic[1],result.named[9].index};return result;
}
}
