#include "client/PaletteShade.h"
#include <algorithm>
#include <cmath>

namespace netstorm::client {
namespace {
// 원본 ColorSort 생성자에 저장된 double 정규화 계수다. 각 RGB 곱은 double로 저장한 뒤 사용한다.
constexpr double kNormalize=1.0/255.0;
// 원본 명암 생성자의 double 값이다. 어두움 계수는 float 0.6을 double로 승격한 값이다.
constexpr double kBright=1.5,kDark=0.6000000238418579;
}
// 각 소스 색의 목표 RGB를 double로 저장하고 후보를 원본 번호 순서로 비교한다.
GumpShadeMaps BuildGumpShadeMaps(std::span<const std::uint32_t,256> logical) {
    std::array<std::array<double,3>,256> rgb{};GumpShadeMaps result;
    // 예약 바이트를 제외한 원본 R/G/B를 ColorSort와 같은 double 값으로 저장한다.
    for (std::size_t i=0;i<rgb.size();++i)
        rgb[i]={(logical[i]&255)*kNormalize,((logical[i]>>8)&255)*kNormalize,((logical[i]>>16)&255)*kNormalize};
    // 소스 번호마다 두 독립 목표 색을 검색한다.
    for (std::size_t i=0;i<rgb.size();++i) {
        const std::array<double,3> bright{std::min(1.0,rgb[i][0]*kBright),std::min(1.0,rgb[i][1]*kBright),std::min(1.0,rgb[i][2]*kBright)};
        const std::array<double,3> dark{rgb[i][0]*kDark,rgb[i][1]*kDark,rgb[i][2]*kDark};
        float brightMinimum=1e6f,darkMinimum=1e6f;
        // 거리 자체는 double로 비교하고 최소 거리만 float로 좁힌다. 동일 RGB도 저장 반올림에 따라 뒤 번호를 고를 수 있다.
        for (std::size_t candidate=0;candidate<rgb.size();++candidate) {
            const double brightDistance=std::abs(bright[0]-rgb[candidate][0])+std::abs(bright[1]-rgb[candidate][1])+std::abs(bright[2]-rgb[candidate][2]);
            const double darkDistance=std::abs(dark[0]-rgb[candidate][0])+std::abs(dark[1]-rgb[candidate][1])+std::abs(dark[2]-rgb[candidate][2]);
            if (brightDistance<brightMinimum && (candidate<229 || candidate>245)) {
                brightMinimum=static_cast<float>(brightDistance);result.bright[i]=static_cast<std::uint8_t>(candidate);
            }
            if (darkDistance<darkMinimum) { darkMinimum=static_cast<float>(darkDistance);result.dark[i]=static_cast<std::uint8_t>(candidate); }
        }
    }
    return result;
}
}
