// 원본 mapped는 배경을 색 치환하지 않고 실제 불투명 RLE 소스 픽셀만 치환한다.
#include "client/VFXMappedDraw.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client {
// 평면8비트 장치에 원본 pane 원점/clip/투명 마스크를 적용한다.
int DrawMappedShape(std::span<std::uint8_t> destination,int width,int height,const ShapeDatabase& database,
    std::size_t block,std::size_t frame,int x,int y,ShapeRect pane,std::span<const std::uint8_t,256> colors) {
    if (width<=0 || height<=0) return -1;
    if (static_cast<std::size_t>(width)*static_cast<std::size_t>(height)>destination.size()) throw std::invalid_argument("mapped RLE 출력 버퍼 크기 오류");
    const auto left=std::max(pane.left,0),top=std::max(pane.top,0),right=std::min(pane.right,width-1),bottom=std::min(pane.bottom,height-1);if (right<left || bottom<top) return -2;
    if (block>=database.Blocks().size()) throw std::out_of_range("mapped RLE 블록 범위 오류");
    const auto& record=database.Blocks()[block].frames.at(frame);if (record.rect.right<record.rect.left || record.rect.bottom<record.rect.top) return -4;
    const auto originX=static_cast<std::int64_t>(x)+pane.left,originY=static_cast<std::int64_t>(y)+pane.top;
    const auto frameLeft=originX+record.rect.left,frameTop=originY+record.rect.top,frameRight=originX+record.rect.right,frameBottom=originY+record.rect.bottom;
    if (frameRight<left || frameBottom<top || frameLeft>right || frameTop>bottom) return -3;
    const auto image=database.Decode(block,frame);
    // 실제로 겹치는 행만 처리하며 다른 배경 픽셀을 유지한다.
    for (auto row=std::max<std::int64_t>(frameTop,top);row<=std::min<std::int64_t>(frameBottom,bottom);++row) {
        // 투명 건너뛰기는 색 번호0/255와 분리한다.
        for (auto column=std::max<std::int64_t>(frameLeft,left);column<=std::min<std::int64_t>(frameRight,right);++column) {
            const auto input=static_cast<std::size_t>(row-frameTop)*image.width+static_cast<std::size_t>(column-frameLeft);
            if (image.opacity[input]) destination[static_cast<std::size_t>(row)*static_cast<std::size_t>(width)+static_cast<std::size_t>(column)]=colors[image.indices[input]];
        }
    }
    return 0;
}
}
