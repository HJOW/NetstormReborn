// 실제 DWORD 감김·산술 시프트·두 구간의 do/while과 포함 경계 픽셀을 보존한다.
#include "client/VFXEllipse.h"
#include "client/VFXLine.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 다른 기하 라이브러리로 원본 타원 윤곽을 대체하지 않는다.
namespace { // 원문32비트 연산과 손상 입력의 호스트 반복 경계다.
constexpr std::uint64_t kIterationLimit=1000000; // 정상 작은 표시 반경과 구별되는 비수렴/과도한 호스트 입력 상한이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 원문 CMP/JL의 signed DWORD 해석이다.
std::uint32_t Sar(std::uint32_t value) { return (value>>1)|(value&0x80000000U); } // 원문 SAR1을 음수 오른쪽 시프트의 호스트 차이 없이 보존한다.
}
void DrawVfxEllipse(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,
    ScreenPoint center,std::int32_t radiusX,std::int32_t radiusY,std::uint32_t color) { // pane 원점 감산/재가산이 상쇄된 절대 중심을 받는다.
    if (pitch<1 || height<1 || std::uint64_t(pitch)*height>pixels.size()) throw std::invalid_argument("VFX 타원 표면 용량 오류");
    clip=ClipScreenRect(clip,{0,0,pitch,height});if (clip.left>=clip.right || clip.top>=clip.bottom) return;
    const auto cx=static_cast<std::uint32_t>(center.x),cy=static_cast<std::uint32_t>(center.y),rx=static_cast<std::uint32_t>(radiusX),ry=static_cast<std::uint32_t>(radiusY); // 좌표/반경의 실제 DWORD 비트다.
    if (!radiusX || !radiusY) { DrawVfxLine(pixels,pitch,height,clip,{Signed(cx-rx),Signed(cy-ry)},{Signed(cx+rx),Signed(cy+ry)},color);return; }
    const auto a=rx*rx,b=ry*ry;std::uint32_t x=0,y=ry,dx=0,dy=a*2U*ry,decision=(a>>2)+b-a*ry;std::uint64_t iterations=0; // 원문 제곱/차분/결정 변수의32비트 감김이다.
    const auto pixel=[&](std::uint32_t px,std::uint32_t py) { const auto sx=Signed(px),sy=Signed(py);if (sx>=clip.left && sx<clip.right && sy>=clip.top && sy<clip.bottom) pixels[std::size_t(sy)*pitch+sx]=static_cast<std::uint8_t>(color); }; // 현재 포함 경계 안의 한 BYTE만 쓴다.
    const auto plot=[&] { if (++iterations>kIterationLimit) throw std::out_of_range("VFX 타원 반복 범위 초과");pixel(cx+x,cy+y);pixel(cx+x,cy-y);pixel(cx-x,cy+y);pixel(cx-x,cy-y); }; // 원문 네 대칭점의 중복 쓰기와 순서를 유지한다.
    while (Signed(dx-dy)<0) { // 첫 구간은 signed 두 차분의 차가 음수일 때만 실행한다.
        plot();if (Signed(decision)>=0) { --y;dy-=a*2U;decision-=dy; }++x;dx+=b*2U;decision+=dx+b;
    }
    const auto difference=a-b;decision+=Sar(Sar(difference)+difference-dx-dy); // 원문 두 차례 SAR과 사이의 DWORD 감김이다.
    do { // 두 번째 구간은 음수 초기 반경이어도 한 번 실행한 뒤 현재 y를 판단한다.
        plot();if (Signed(decision)<0) { ++x;dx+=b*2U;decision+=dx; }--y;dy-=a*2U;decision-=dy-a;
    }while (Signed(y)>=0);
}
}
