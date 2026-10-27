// Windows10/11 호환성 확장: 원본 DirectDraw 대신 DIB 전체화면의 화면비/입력/변경 영역을 계산한다.
#include "client/Screen.h"
#include "client/ScreenPresentation.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace netstorm::client { // 원본 화면 크기를 유지하는 호환 출력 경계다.
// 원본 DIB처럼 양수 카메라 이동은 원본 오른쪽/아래 픽셀을 왼쪽/위로 복사한다. 화면 밖 읽기는 호스트에서 거부한다.
void ScrollScreenPixels(std::span<std::uint8_t> pixels,int pitch,int width,int height,int x,int y,int regionWidth,int regionHeight,int dx,int dy) {
    if (regionWidth<=0 || regionHeight<=0 || std::abs(std::int64_t(dx))>=regionWidth || std::abs(std::int64_t(dy))>=regionHeight || (!dx && !dy)) return;
    if (width<=0 || height<=0 || pitch<width || x<0 || y<0 || std::int64_t(x)+regionWidth>width || std::int64_t(y)+regionHeight>height || pixels.size()<static_cast<std::size_t>(pitch)*height) throw std::out_of_range("DIB 화면 이동의 실제 픽셀 범위 오류");
    const int sourceX=x+std::max(dx,0),destinationX=x+std::max(-dx,0),rows=regionHeight-static_cast<int>(std::abs(std::int64_t(dy))),columns=regionWidth-static_cast<int>(std::abs(std::int64_t(dx)));
    for (int index=0;index<rows;++index) { // 아래로 복사할 때는 뒤쪽 행부터 옮겨 겹친 원본을 보존한다.
        const int row=dy>0 ? index : rows-1-index,sourceY=y+std::max(dy,0)+row,destinationY=y+std::max(-dy,0)+row;
        std::memmove(pixels.data()+static_cast<std::size_t>(destinationY)*pitch+destinationX,pixels.data()+static_cast<std::size_t>(sourceY)*pitch+sourceX,static_cast<std::size_t>(columns));
    }
}
// 원본과 다른 Windows10/11 경로는8비트 DIB를 쓰며 실제로 없는 DirectDraw flip 비트를 공개하지 않는다.
std::uint32_t ModernScreenMode(std::uint32_t requested) {
    requested|=ScreenMode::kModeSet;
    if (requested&ScreenMode::kFullScreen) requested=(requested|ScreenMode::kBackBufferDib)&~ScreenMode::kFlipping;
    else requested=(requested&~(ScreenMode::kSoftwareMouse|ScreenMode::kFlipping|ScreenMode::kParallax))|ScreenMode::kBackBufferDib; // 원본00435ba0의 창 모드 비트 정리도 유지한다.
    return requested;
}
// 원본과 다른 호환 전체화면은 논리 크기를 바꾸지 않고 현재 클라이언트 영역에 비율대로 가운데 표시한다.
ScreenRect FitScreenPresentation(int width,int height,int clientWidth,int clientHeight) {
    if (width<=0 || height<=0 || clientWidth<=0 || clientHeight<=0) throw std::invalid_argument("전체화면 출력의 양수 크기 필요");
    int fittedWidth=clientWidth,fittedHeight=clientHeight;
    if (std::int64_t(clientWidth)*height<=std::int64_t(clientHeight)*width) fittedHeight=std::max(1,static_cast<int>(std::int64_t(clientWidth)*height/width));
    else fittedWidth=std::max(1,static_cast<int>(std::int64_t(clientHeight)*width/height));
    const int left=(clientWidth-fittedWidth)/2,top=(clientHeight-fittedHeight)/2;return {left,top,left+fittedWidth,top+fittedHeight};
}
// 호환 출력의 실제 마우스를 같은 논리 화면에 되돌리고 원본 Poll의 끝 포함 범위로 제한한다.
ScreenPoint MapScreenPresentationPoint(ScreenPoint point,ScreenRect output,int width,int height) {
    if (width<=0 || height<=0 || output.right<=output.left || output.bottom<=output.top) throw std::invalid_argument("전체화면 입력 변환의 실제 출력 범위 필요");
    const auto x=(std::int64_t(point.x)-output.left)*width/(std::int64_t(output.right)-output.left),y=(std::int64_t(point.y)-output.top)*height/(std::int64_t(output.bottom)-output.top);
    return {static_cast<int>(std::clamp<std::int64_t>(x,0,width)),static_cast<int>(std::clamp<std::int64_t>(y,0,height))};
}
// 전체 원본 표면을 같은 비율로 내보내되 변경 영역은 바깥쪽으로 반올림해 확대 경계의 잔상을 막는다.
ScreenRect MapScreenPresentationRect(ScreenRect rect,ScreenRect output,int width,int height) {
    if (width<=0 || height<=0 || output.right<=output.left || output.bottom<=output.top) throw std::invalid_argument("전체화면 dirty 변환의 실제 출력 범위 필요");
    rect=ClipScreenRect(rect,{0,0,width,height});const auto x=std::int64_t(output.right)-output.left,y=std::int64_t(output.bottom)-output.top;
    return {output.left+static_cast<int>(std::int64_t(rect.left)*x/width),output.top+static_cast<int>(std::int64_t(rect.top)*y/height),
        output.left+static_cast<int>((std::int64_t(rect.right)*x+width-1)/width),output.top+static_cast<int>((std::int64_t(rect.bottom)*y+height-1)/height)};
}
// 원본 DirectDraw와 다른 호환 확대는 장치별 StretchDIBits 반올림 대신 같은 픽셀 중심 표본을 사용한다.
void ScaleScreenPresentation(std::span<const std::uint8_t> source,int sourcePitch,int sourceWidth,int sourceHeight,std::span<std::uint8_t> destination,int destinationPitch,int destinationWidth,int destinationHeight) {
    if (sourceWidth<=0 || sourceHeight<=0 || destinationWidth<=0 || destinationHeight<=0 || sourcePitch<sourceWidth || destinationPitch<destinationWidth) throw std::invalid_argument("전체화면 확대의 양수 크기/행 간격 필요");
    if (source.size()<static_cast<std::size_t>(sourceHeight-1)*sourcePitch+sourceWidth || destination.size()<static_cast<std::size_t>(destinationHeight-1)*destinationPitch+destinationWidth) throw std::out_of_range("전체화면 확대의 실제 픽셀 범위 부족");
    std::vector<int> columns(static_cast<std::size_t>(destinationWidth));
    for (int x=0;x<destinationWidth;++x) columns[x]=static_cast<int>((2*std::int64_t(x)+1)*sourceWidth/(2*std::int64_t(destinationWidth))); // 각 열의 최근접 선택을 한 번 계산한다.
    for (int y=0;y<destinationHeight;++y) {
        const auto row=static_cast<std::size_t>((2*std::int64_t(y)+1)*sourceHeight/(2*std::int64_t(destinationHeight)))*sourcePitch;const auto offset=static_cast<std::size_t>(y)*destinationPitch;
        for (int x=0;x<destinationWidth;++x) destination[offset+x]=source[row+columns[x]]; // padding/화면비를 바꾸지 않고8비트 원본 번호만 복사한다.
    }
}
}
