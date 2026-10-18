// 선행 표는 원본 memset의0이며 비활성은 빨강만2.1배 한 제곱 거리다.
#include "client/ProductionColorMaps.h"
#include <algorithm>
namespace netstorm::client {
ProductionColorMaps BuildProductionColorMaps(std::span<const std::uint32_t,256> logical) {
    ProductionColorMaps result;std::array<std::array<double,3>,256> rgb{}; // 원본 ColorSort의 정규화 성분이다.
    constexpr double normalize=1.0/255.0; // 실제 원본 double 정규화다.
    // 후보 번호는 정렬하지 않고 팔레트 순서대로 유지한다.
    for (std::size_t i=0;i<rgb.size();++i) rgb[i]={(logical[i]&255)*normalize,((logical[i]>>8)&255)*normalize,((logical[i]>>16)&255)*normalize};
    // 원본은 소스 빨강만 확대한 뒤1로 제한한다.
    for (std::size_t i=0;i<rgb.size();++i) {
        const double target=std::min(1.0,rgb[i][0]*2.1);float minimum=1e6f;
        // blue→green→red 제곱 합을 float 최소값과 엄격 비교한다.
        for (std::size_t candidate=0;candidate<rgb.size();++candidate) {
            const double blue=rgb[i][2]-rgb[candidate][2],green=rgb[i][1]-rgb[candidate][1],red=target-rgb[candidate][0];
            const double distance=blue*blue+green*green+red*red;
            if (distance<minimum) { minimum=static_cast<float>(distance);result.disabled[i]=static_cast<std::uint8_t>(candidate); }
        }
    }
    return result;
}
}
