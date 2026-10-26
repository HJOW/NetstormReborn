// 원본의 별도200 stride 텍스처와 두 번의 정확한 순회/난수를 사용한다.
#include "o/RawTerrainBitmap.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 원문 픽셀 출력에 월드나 공유 GameRandom 효과를 추가하지 않는다.
namespace { // 선택 장치와 저장 범위 진단만 지역화한다.
// 비정상 텍스처의 원문 나눗셈/잘못된 주소를 정상 이미지로 가장하지 않는다.
void ValidateTexture(const TerrainBitmapTexture& texture) {
    if (texture.width<=1 || texture.height<=1 || texture.width>200 || texture.height>200) throw std::out_of_range("지형 bitmap의 실제 texture 헤더 범위");
}
}
// 생성자는 분류 캐시나 난수를 변경하지 않는다.
RawTerrainBitmap::RawTerrainBitmap(TerrainBitmapState& state,const std::uint32_t& islandType,TerrainBitmapHooks hooks)
    :state_(state),islandType_(islandType),hooks_(std::move(hooks)) {}
// 입력은 물리 X*256+Y다. 출력의 빈0은 텍스처의 투명255와 별개로 보존한다.
TerrainBitmapImage RawTerrainBitmap::Render(std::span<const std::uint8_t,65536> codes,std::int32_t width,std::int32_t height,GameRandom& random) const {
    if (width<0 || height<0 || width>256 || height>256) throw std::out_of_range("지형 bitmap의 실제 문자 저장소 범위");
    if (!state_.initialized) {
        state_.classified.fill(0);
        if (!hooks_.green) throw std::logic_error("지형 bitmap 현재 녹색 조회 미연결");
        state_.classified['x']=static_cast<std::uint8_t>(hooks_.green());state_.initialized=1;
    }
    if (!hooks_.texture) throw std::logic_error("지형 bitmap 실제 texture 조회 미연결");
    const auto fringe=hooks_.texture(state_.fringeType);ValidateTexture(fringe);
    TerrainBitmapImage result{width,height,std::vector<std::uint8_t>(static_cast<std::size_t>(width)*height)};
    for (std::int32_t x=0;x<width;++x) { // 원문은 각 X열의 Y를 아래에서 위로 진행한다.
        for (auto y=height-1;y>0;--y) { // 위 칸을 함께 읽으므로 실제 Y0은 첫 fringe 순회에서 제외한다.
            const auto code=codes[static_cast<std::size_t>(x)*256+y];
            if (code!='=') continue;
            const auto before=codes[static_cast<std::size_t>(x)*256+y-1];
            if (before=='x') {
                const auto length=random.Next(3)+3;
                for (std::uint32_t step=0;step<length && y+static_cast<std::int32_t>(step)<height;++step) { // 첫 샘플은 texture의1행이며 아래 끝은 출력 높이로 제한한다.
                    const auto sample=static_cast<std::size_t>((step+1)%(fringe.height-1))*200+x%(fringe.width-1);
                    result.pixels[static_cast<std::size_t>(y+step)*width+x]=fringe.pixels[sample];
                }
            }else if (before!='=') result.pixels[static_cast<std::size_t>(y)*width+x]=120;
        }
    }
    const auto ground=hooks_.texture(islandType_);ValidateTexture(ground);
    for (std::int32_t y=0;y<height;++y) { // 원본 두 번째 순회는 출력 행을 먼저 진행한다.
        for (std::int32_t x=0;x<width;++x) { // 각 행의 X를 처음부터 다시 읽어 지면 텍스처를 채운다.
            const auto code=codes[static_cast<std::size_t>(x)*256+y];
            if (code>=128) throw std::out_of_range("지형 bitmap의 원문 signed 문자 분류 범위");
            if (state_.classified[code]) result.pixels[static_cast<std::size_t>(y)*width+x]=ground.pixels[static_cast<std::size_t>(y%(ground.height-1))*200+x%(ground.width-1)];
        }
    }
    return result;
}
}
