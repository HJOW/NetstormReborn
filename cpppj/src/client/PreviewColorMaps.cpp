// 원본 전체 팔레트 생성의 별도 x86 출력과 대조하는 커서 전용 세 표다.
#include "client/PreviewColorMaps.h"
#include <algorithm>
namespace netstorm::client {
namespace {
constexpr double kNormalize=1.0/255.0,kIllegalRed=2.1; // 실제 ColorSort와506708의 double 상수다.
}
// 원본의 blue→green→red 제곱 합과 float 최소값 저장 순서를 보존한다.
PreviewColorMaps BuildPreviewColorMaps(std::span<const std::uint32_t,256> logical,std::uint32_t red) {
    PreviewColorMaps result;result.destroy.fill(static_cast<std::uint8_t>(red));result.labelShadow.fill(0);
    std::array<std::array<double,3>,256> rgb; // 실제 ColorSort가 저장한 정규 RGB를 원본 번호 순서로 보관한다.
    // 예약 BYTE를 제외하고 각 성분을 double로 저장한다.
    for (std::size_t index=0;index<rgb.size();++index) rgb[index]={(logical[index]&255)*kNormalize,((logical[index]>>8)&255)*kNormalize,((logical[index]>>16)&255)*kNormalize};
    // 각 원본 색의 빨강만 확대하고 나머지 두 성분은 유지한다.
    for (std::size_t index=0;index<rgb.size();++index) {
        const double target=std::min(1.0,rgb[index][0]*kIllegalRed);float minimum=1e6f;
        // 원본은 엄격한 double 비교 뒤 최소 거리만 float로 좁히므로 동률 선택을 단순화하지 않는다.
        for (std::size_t candidate=0;candidate<rgb.size();++candidate) {
            const double blue=rgb[index][2]-rgb[candidate][2],green=rgb[index][1]-rgb[candidate][1],difference=target-rgb[candidate][0];
            const double distance=blue*blue+green*green+difference*difference;
            if (distance<minimum) { minimum=static_cast<float>(distance);result.illegal[index]=static_cast<std::uint8_t>(candidate); }
        }
    }
    return result;
}
}
