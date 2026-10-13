// 원본의 방향·반 픽셀 편향·클립 시작/끝의 다른 역산과 분수 ADD carry를 유지한다.
#include "client/VFXLine.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 클립 분류 비트의 실제 우선 순서는 왼쪽→오른쪽→위→아래다. 반 픽셀은 0.32 분수의 bit 31이다.
constexpr std::uint32_t kLeft=8,kRight=4,kTop=2,kBottom=1,kHalf=0x80000000U;
// 정상 네 경계 교차 밖에서 수렴하지 않는 호스트 입력을 제한하는 반복 상한이다.
constexpr unsigned kClipIterations=32;
// 원본 DWORD 좌표 덧셈/부호 반전을 signed overflow 없이 수행한다.
std::int32_t Add(std::int32_t a,std::uint32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+b); }
// 제한한 단색 선의 현재 두 끝점과 원래 기준점/기울기다. pixel phase는 별도 순회에서 계산한다.
struct LineClip {
    // first: 구간 또는 연결 목록의 첫 항목.
    // last: 구간 또는 연결 목록의 마지막 항목.
    // origin: 좌표 또는 시각 계산의 기준 원점.
    ScreenPoint first,last,origin;
    // bounds: 화면 좌표의 사각형. right·bottom은 포함하지 않는다(원본 004a1800의 인자) (ScreenRect).
    ScreenRect bounds;
    // slope: 기울기.
    std::uint32_t slope{};
    // majorX: 주 X 여부.
    // negativeSlope: 음의 기울기 여부.
    bool majorX{},negativeSlope{};
};
// 반 평면 비교는 현재 inclusive 경계를 사용한다. 겹치는 비트가 있으면 선 전체를 버릴 수 있다.
std::uint32_t Outcode(ScreenPoint point,ScreenRect bounds) {
    return (point.x<bounds.left ? kLeft : 0U)|(point.x>bounds.right ? kRight : 0U)|
        (point.y<bounds.top ? kTop : 0U)|(point.y>bounds.bottom ? kBottom : 0U);
}
// 실제 클립 곱의 high DWORD와 low DWORD bit 31을 더해 반 픽셀을 반올림한다.
std::uint32_t RoundedProduct(std::uint32_t distance,std::uint32_t slope) {
    const auto product=std::uint64_t(distance)*slope;
    return static_cast<std::uint32_t>(product>>32)+(static_cast<std::uint32_t>(product)>=kHalf ? 1U : 0U);
}
// 원본 DIV는 32비트 몫만 받는다. 불가능한 DIV overflow를 호스트 예외로 거부한다.
std::uint32_t Inverse(std::uint32_t distance,std::uint32_t slope,bool start) {
    const auto numerator=(std::uint64_t(start ? distance-1U : distance)<<32)|kHalf;
    const auto quotient=numerator/slope,remainder=numerator%slope;
    if (quotient>std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("VFX 선 클립 DIV 범위 초과");
    return static_cast<std::uint32_t>(quotient)+(start ? 1U : 0U)-(remainder==0 ? 1U : 0U);
}
// 한 끝점을 원래 시작점 기준으로 교차한다. 현재 끝점에서 다시 기울기를 계산하지 않는다.
void Cut(LineClip& line,std::uint32_t code,bool start) {
    const bool xAxis=(code&(kLeft|kRight))!=0;
    const auto boundary=xAxis ? (code&kLeft ? line.bounds.left : line.bounds.right) :
        (code&kTop ? line.bounds.top : line.bounds.bottom);
    const auto origin=xAxis ? line.origin.x : line.origin.y;
    const auto delta=std::int64_t(boundary)-origin;
    const auto distance=static_cast<std::uint32_t>(delta<0 ? -delta : delta);
    const auto offset=xAxis==line.majorX ? RoundedProduct(distance,line.slope) : Inverse(distance,line.slope,start);
    const bool negative=(delta<0)!=line.negativeSlope;
    const auto other=Add(xAxis ? line.origin.y : line.origin.x,negative ? 0U-offset : offset);
    auto& endpoint=start ? line.first : line.last;
    if (xAxis) endpoint={boundary,other};else endpoint={other,boundary};
}
// 두 끝점의 바깥 분류를 반복한다. 정상 원본 좌표 밖에서 수렴하지 않는 입력만 거부한다.
bool Clip(LineClip& line) {
    // 정상 좌표의 교차/거부 순서를 유지하고 비정상 수렴 실패만 상한에서 거부한다.
    for (unsigned count=0;count<kClipIterations;++count) {
        const auto first=Outcode(line.first,line.bounds),last=Outcode(line.last,line.bounds);
        if (!(first|last)) return true;
        if (first&last) return false;
        Cut(line,first ? first : last,first!=0);
    }
    throw std::runtime_error("VFX 선 클립 수렴 실패");
}
}
// 실제 표면에만 쓰며 원본 pane 원점의 감산/재가산이 상쇄된 절대 좌표를 사용한다.
void DrawVfxLine(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,
    ScreenPoint first,ScreenPoint last,std::uint32_t color) {
    if (pitch<1 || height<1 || std::uint64_t(pitch)*height>pixels.size()) throw std::invalid_argument("VFX 선 표면 용량 오류");
    clip=ClipScreenRect(clip,{0,0,pitch,height});if (clip.left>=clip.right || clip.top>=clip.bottom) return;
    const ScreenRect bounds{clip.left,clip.top,clip.right-1,clip.bottom-1};
    const auto dx=std::int64_t(last.x)-first.x,dy=std::int64_t(last.y)-first.y;
    const auto ax=static_cast<std::uint64_t>(dx<0 ? -dx : dx),ay=static_cast<std::uint64_t>(dy<0 ? -dy : dy);
    const auto ink=static_cast<std::uint8_t>(color);
    if (!dx || !dy) {
        const auto left=std::max<std::int64_t>(clip.left,std::min(first.x,last.x)),right=std::min<std::int64_t>(clip.right,std::int64_t(std::max(first.x,last.x))+1);
        const auto top=std::max<std::int64_t>(clip.top,std::min(first.y,last.y)),bottom=std::min<std::int64_t>(clip.bottom,std::int64_t(std::max(first.y,last.y))+1);
        if (left>=right || top>=bottom) return;
        // 직교 선은 기존 현재 클립에서 양 끝 포함으로 채운다. 한 점도 같은 경로다.
        for (auto y=top;y<bottom;++y) std::fill(pixels.begin()+y*pitch+left,pixels.begin()+y*pitch+right,ink);
        return;
    }
    if (ax>std::numeric_limits<std::int32_t>::max() || ay>std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("VFX 선 signed 거리 한도 초과");
    const auto slope=static_cast<std::uint32_t>(ax==ay ? 0xffffffffULL : (std::min(ax,ay)<<32)/std::max(ax,ay));
    LineClip line{first,last,first,bounds,slope,ax>=ay,(dx<0)!=(dy<0)};
    if (!Clip(line)) return;
    const auto startMajor=line.majorX ? line.first.x : line.first.y,endMajor=line.majorX ? line.last.x : line.last.y;
    const auto originMajor=line.majorX ? first.x : first.y;
    const auto distance=std::int64_t(startMajor)-originMajor;
    auto phase=static_cast<std::uint32_t>(static_cast<std::uint64_t>(distance<0 ? -distance : distance)*slope)+kHalf;
    auto point=line.first;const int sx=dx<0 ? -1 : 1,sy=dy<0 ? -1 : 1;
    const auto span=std::int64_t(endMajor)-startMajor;
    const auto count=static_cast<std::uint64_t>(span<0 ? -span : span)+1;
    // 원래 방향과 분수 위상을 유지하며 잘린 시작/끝 사이의 실제 주축 픽셀만 방문한다.
    for (std::uint64_t index=0;index<count;++index) {
        if (!Outcode(point,bounds)) pixels[std::size_t(point.y)*pitch+point.x]=ink;
        const auto next=std::uint64_t(phase)+slope;const bool carry=next>0xffffffffULL;phase=static_cast<std::uint32_t>(next);
        if (line.majorX) { point.x=Add(point.x,std::uint32_t(sx));if (ax==ay || carry) point.y=Add(point.y,std::uint32_t(sy)); }
        else { point.y=Add(point.y,std::uint32_t(sy));if (carry) point.x=Add(point.x,std::uint32_t(sx)); }
    }
}
}
