// 일반 자산의 명암은 L1, 그림자는 제곱 거리로 소스 팔레트 번호를 변환한다.
#include "client/SpriteColorMaps.h"
#include "client/PaletteShade.h"
namespace netstorm::client {
namespace {
constexpr double kNormalize=1.0/255.0,kShadow=0.6000000238418579; // 원본 정규화 double과 승격한float0.6이다.
}
// 기존 원본 명암 두 표를 재사용하고 독립 그림자 표를 같은 RGB로 계산한다.
SpriteColorMaps BuildSpriteColorMaps(std::span<const std::uint32_t,256> logical) {
    const auto shade=BuildGumpShadeMaps(logical);SpriteColorMaps result{shade.bright,shade.dark,{}};
    std::array<std::array<double,3>,256> rgb{}; // ColorSort가 저장한 성분별double이다.
    // 예약 BYTE를 제외하고 실제 원본 번호 순서로 정규화한다.
    for (std::size_t i=0;i<rgb.size();++i) rgb[i]={(logical[i]&255)*kNormalize,((logical[i]>>8)&255)*kNormalize,((logical[i]>>16)&255)*kNormalize};
    // 각각의 소스 RGB를 어둡게 만든 뒤 모든 후보를 원본 순서로 검사한다.
    for (std::size_t i=0;i<rgb.size();++i) {
        const std::array<double,3> target{rgb[i][0]*kShadow,rgb[i][1]*kShadow,rgb[i][2]*kShadow};float minimum=1e6f;
        // 원본 blue→green→red 합과 엄격한 비교 뒤 float 저장을 유지한다.
        for (std::size_t candidate=0;candidate<rgb.size();++candidate) {
            const double blue=target[2]-rgb[candidate][2],green=target[1]-rgb[candidate][1],red=target[0]-rgb[candidate][0];
            const double distance=blue*blue+green*green+red*red;
            if (distance<minimum) { minimum=static_cast<float>(distance);result.shadow[i]=static_cast<std::uint8_t>(candidate); }
        }
    }
    // 원본43c010 끝에서18회에 걸쳐228..245를 항등으로 덮는다. 증분 전 종료값을 사용한다.
    for (std::size_t i=228;i<=245;++i) result.shadow[i]=static_cast<std::uint8_t>(i);
    result.shadow[254]=254;result.shadow[255]=255; // 마지막 두 예약 번호도 원본의 명시 대입이다.
    return result;
}
}
