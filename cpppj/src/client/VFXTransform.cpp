// 디컴파일의 다각형 행 순회를 직교 사각형으로 줄이되 정수 변환·끝점·표본 순서를 유지한다.
#include "client/VFXTransform.h"
#include "client/Renderer.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 배율 1과 텍스처 좌표의 한 픽셀은 같은 16.16 단위를 사용한다.
constexpr std::int32_t kFixedUnit=0x10000;
// 텍스처 좌표의 반 픽셀 편향이다. 꼭짓점 변환의 64비트 덧셈에는 같은 비트를 따로 쓴다.
constexpr std::uint32_t kTextureBias=0x8000;
// 변환 임시 표면은 255로 채워지며 색 치환 후 같은 값도 투명으로 남는다.
constexpr std::uint8_t kScratchTransparent=255;
// x86 고정소수점 좌표와 기울기의 정상 signed WORD 범위를 검사할 때 사용한다.
constexpr std::int32_t kWordMinimum=-32768,kWordMaximum=32767;

// 기준점 이동의 하위 DWORD를 보존한다. C++ signed overflow를 발생시키지 않는다.
std::int32_t Add(std::int32_t left,std::int32_t right) {
    return std::bit_cast<std::int32_t>(std::uint32_t(left)+std::uint32_t(right));
}

// 꼭짓점의 원본 IMUL/ADD 8000/ADC를 계산한다. 음수 몫은 아래 정수로 내린다.
// 반 픽셀을 더하는 일반 반올림과 달리, 편향은 64비트 곱의 하위 DWORD에만 더해진다.
std::int32_t ScaleCoordinate(std::int32_t coordinate,std::int32_t scale) {
    const auto shifted=std::bit_cast<std::int32_t>(std::uint32_t(coordinate)<<16);
    const auto biased=std::int64_t(shifted)*scale+kTextureBias;
    const auto divisor=std::int64_t(1)<<32;
    const auto quotient=biased/divisor-(biased%divisor<0 ? 1 : 0);
    return static_cast<std::int32_t>(quotient);
}

// 원본 signed IDIV의 절삭 기울기다. 분모는 양수이며 검증한 간격만 전달한다.
std::int32_t TextureStep(std::int32_t delta,std::int32_t span) {
    return static_cast<std::int32_t>(std::int64_t(delta)*kFixedUnit/span);
}

// 원본 좌표 누산의 DWORD 감김을 유지한다. 클리핑된 첫 픽셀까지도 같은 기울기로 전진한다.
std::uint32_t Advance(std::uint32_t value,std::int32_t step,std::int32_t count) {
    return value+std::uint32_t(step)*std::uint32_t(count);
}

// 픽셀 중심 좌표를 만든다. 호출자는 0~32767의 원본 픽셀 인덱스만 넘긴다.
std::uint32_t TextureStart(std::int32_t index) {
    return (std::uint32_t(index)<<16)+kTextureBias;
}

// 원본 WORD 좌표를 정규화 없이 해석할 수 있는 범위인지 검사한다.
bool IsWord(std::int32_t value) { return value>=kWordMinimum && value<=kWordMaximum; }
}

// 일반 VFX 해제 이미지에서 임시 255 표면과 같은 결과를 직접 샘플링한다.
void DrawScaledVfxImage(std::span<std::uint8_t> pixels,int pitch,int height,
    const IndexedImage& image,const ShapeFrame& frame,ScreenPoint position,
    std::int32_t scaleX,std::int32_t scaleY,ScreenRect clip,const ColorMap* colors) {
    if (pitch<1 || height<1 || std::uint64_t(pitch)*std::uint64_t(height)>pixels.size())
        throw std::invalid_argument("VFX 변환 표면 용량 오류");
    if (image.width<1 || image.height<1 || image.width>std::size_t(kWordMaximum)+1 ||
        image.height>std::size_t(kWordMaximum)+1 || image.width*image.height!=image.indices.size() ||
        image.opacity.size()!=image.indices.size()) throw std::invalid_argument("VFX 변환 이미지 용량 오류");
    const auto& rect=frame.rect;
    if (frame.IsSpecial() || std::int64_t(rect.right)-rect.left+1!=static_cast<std::int64_t>(image.width) ||
        std::int64_t(rect.bottom)-rect.top+1!=static_cast<std::int64_t>(image.height))
        throw std::invalid_argument("VFX 변환 실제 프레임 영역 오류");
    // 원본의 조기 일반 출력 분기에서는 255도 불투명 색으로 유지한다.
    if (scaleX==kFixedUnit && scaleY==kFixedUnit) {
        DrawIndexedImage(pixels,pitch,height,image,Add(position.x,rect.left),Add(position.y,rect.top),clip,colors);
        return;
    }
    if (!IsWord(rect.left) || !IsWord(rect.right) || !IsWord(rect.top) || !IsWord(rect.bottom))
        throw std::out_of_range("VFX 변환 프레임 WORD 좌표 범위 초과");
    const auto firstX=Add(position.x,ScaleCoordinate(rect.left,scaleX));
    const auto lastX=Add(position.x,ScaleCoordinate(rect.right,scaleX));
    const auto firstY=Add(position.y,ScaleCoordinate(rect.top,scaleY));
    const auto lastY=Add(position.y,ScaleCoordinate(rect.bottom,scaleY));
    if (!IsWord(firstX) || !IsWord(lastX) || !IsWord(firstY) || !IsWord(lastY))
        throw std::out_of_range("VFX 변환 화면 WORD 좌표 범위 초과");
    // 최저/최고 y가 같으면 원본은 행을 그리지 않는다. 한 행 그림의 비단위 변환도 같다.
    if (firstY==lastY) return;
    const auto left=std::min(firstX,lastX),right=std::max(firstX,lastX);
    const auto top=std::min(firstY,lastY),bottom=std::max(firstY,lastY);
    const auto spanX=right-left,spanY=bottom-top;
    if (spanX>kWordMaximum || spanY>kWordMaximum)
        throw std::out_of_range("VFX 변환 고정소수점 간격 범위 초과");
    const auto visible=ClipScreenRect({left,top,right+1,bottom+1},ClipScreenRect(clip,{0,0,pitch,height}));
    if (visible.left>=visible.right || visible.top>=visible.bottom) return;
    const auto sourceX=static_cast<std::int32_t>(image.width-1);
    const auto sourceY=static_cast<std::int32_t>(image.height-1);
    const auto stepX=spanX ? TextureStep(lastX>firstX ? sourceX : -sourceX,spanX) : 0;
    const auto stepY=TextureStep(lastY>firstY ? sourceY : -sourceY,spanY);
    // 동일 x의 두 변 중 오른쪽 변을 먼저 선택하는 <= 비교와 최저 y의 마지막 꼭짓점을 유지한다.
    const auto startX=spanX ? (lastX>firstX ? 0 : sourceX) : (lastY>firstY ? sourceX : 0);
    const auto firstU=Advance(TextureStart(startX),stepX,visible.left-left);
    auto v=Advance(TextureStart(lastY>firstY ? 0 : sourceY),stepY,visible.top-top);
    // 마지막 y도 포함한다. 잘린 행은 원본 기울기를 누적한 위치에서 시작한다.
    for (int y=visible.top;y<visible.bottom;++y) {
        const auto sourceRow=std::size_t(v>>16)*image.width;
        const auto targetRow=std::size_t(y)*std::size_t(pitch);
        auto u=firstU;
        // 마지막 x도 포함하고 투명/색 치환 후 255는 기존 대상 픽셀을 유지한다.
        for (int x=visible.left;x<visible.right;++x) {
            const auto input=sourceRow+std::size_t(u>>16);
            if (image.opacity[input]) {
                const auto ink=colors ? (*colors)[image.indices[input]] : image.indices[input];
                if (ink!=kScratchTransparent) pixels[targetRow+std::size_t(x)]=ink;
            }
            u=Advance(u,stepX,1);
        }
        v=Advance(v,stepY,1);
    }
}
}
