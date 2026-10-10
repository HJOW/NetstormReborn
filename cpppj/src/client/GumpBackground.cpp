#include "client/GumpBackground.h"
#include "client/Renderer.h"
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// x86 DWORD 계산의 감김을 보존하되 C++ signed overflow를 만들지 않는다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
// 원본 중간 폭/높이 계산에도 DWORD 뺄셈을 적용한다.
std::int32_t Sub(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b)); }
// 나눗셈 뒤 배수 정렬에 쓰는 원본 DWORD 곱이다.
std::int32_t Mul(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)*std::uint32_t(b)); }
// 안전 API의 정상 순서 입력인지 확인한다. 역전 경계는 원본 UI 계약에 포함하지 않는다.
bool Ordered(ScreenRect rect) { return rect.left<=rect.right && rect.top<=rect.bottom; }
// 전체 질감 함수 한 호출의 clip·명암·기준점 순서를 만든다. 지나치게 큰/감기는 반복은 안전 경계에서 거부한다.
GumpTexturePass TilePass(ScreenRect rect,ScreenRect clip,std::int16_t width,std::int16_t height,std::uint32_t flags) {
    GumpTexturePass result{rect,{},flags};const int dx=width-1,dy=height-1;
    if (dx<1 || dy<1) return result;
    if (!Ordered(rect)) throw std::invalid_argument("Wrapped GUI texture bounds");
    result.validTile=true;result.clip=ClipScreenRect(clip,rect);
    result.shade=(flags&GumpBackgroundFlags::kBright)!=0 ? 1U : (flags&GumpBackgroundFlags::kDark)!=0 ? 2U : 0U;
    const auto startX=Mul(rect.left/dx,dx),startY=Mul(rect.top/dy,dy);
    const auto endX=Mul(Add(Add(Sub(rect.right,rect.left)/dx,2),rect.left/dx),dx);
    const auto endY=Mul(Add(Add(Sub(rect.bottom,rect.top)/dy,2),rect.top/dy),dy);
    if (startX>endX || startY>endY) return result;
    // 호출 목록을 무한정 확장하거나 원본 루프의 정수 감김으로 멈추지 않는 입력을 실행하지 않는다.
    constexpr std::uint64_t kMaximumDraws=1000000;
    const auto nx=(std::int64_t(endX)-startX)/dx+1,ny=(std::int64_t(endY)-startY)/dy+1;
    if (std::uint64_t(nx)*std::uint64_t(ny)>kMaximumDraws || std::int64_t(endX)+dx>std::numeric_limits<int>::max() || std::int64_t(endY)+dy>std::numeric_limits<int>::max())
        throw std::length_error("GUI texture loop exceeds safe range");
    result.origins.reserve(static_cast<std::size_t>(nx*ny));
    // 원본처럼 위에서 아래로 끝 배수를 포함하여 여분 타일을 그린다. clip이 실제 출력 영역을 제한한다.
    for (std::int64_t y=startY;y<=endY;y+=dy) {
        // 같은 화면 기준 x 배수를 각 행에서 재사용한다. 버튼의 상대 원점에서 시작하지 않는다.
        for (std::int64_t x=startX;x<=endX;x+=dx) result.origins.push_back({static_cast<int>(x),static_cast<int>(y)});
    }
    return result;
}
}
// 바탕→왼쪽→오른쪽→위→아래 순서다. 들어온 명암 비트는 지우지 않고 원본처럼 OR한다.
std::vector<GumpTexturePass> PlanGumpBackground(ScreenRect rect,ScreenRect clip,std::int16_t width,std::int16_t height,std::uint32_t flags) {
    if (!Ordered(rect) || !Ordered(clip) || (flags&0x0c000000)!=0 || (flags&GumpBackgroundFlags::kTexture)==0)
        throw std::invalid_argument("Invalid GUI texture path");
    std::vector<GumpTexturePass> result;result.push_back(TilePass(rect,clip,width,height,flags));
    const auto nearShade=(flags&GumpBackgroundFlags::kPressed)!=0 ? GumpBackgroundFlags::kDark : GumpBackgroundFlags::kBright;
    const auto farShade=(flags&GumpBackgroundFlags::kPressed)!=0 ? GumpBackgroundFlags::kBright : GumpBackgroundFlags::kDark;
    if (flags&GumpBackgroundFlags::kLeft) result.push_back(TilePass(ClipScreenRect({rect.left,rect.top,Add(rect.left,2),rect.bottom},clip),clip,width,height,flags|nearShade));
    if (flags&GumpBackgroundFlags::kRight) result.push_back(TilePass(ClipScreenRect({Sub(rect.right,2),rect.top,rect.right,rect.bottom},clip),clip,width,height,flags|farShade));
    if (flags&GumpBackgroundFlags::kTop) result.push_back(TilePass(ClipScreenRect({rect.left,rect.top,rect.right,Add(rect.top,2)},clip),clip,width,height,flags|nearShade));
    if (flags&GumpBackgroundFlags::kBottom) result.push_back(TilePass(ClipScreenRect({rect.left,Sub(rect.bottom,2),rect.right,rect.bottom},clip),clip,width,height,flags|farShade));
    return result;
}
// 투명 질감은 이전 화면 색을 보존한다. SHP의 픽셀 영역 위치를 기준점에 더해 실제 VFX 원점을 재현한다.
void DrawGumpBackground(IndexedImage& canvas,const IndexedImage& texture,ScreenPoint frameOffset,
    std::span<const GumpTexturePass> passes,const GumpShadeMaps& maps) {
    // 원본의 질감 호출 순서와 겹치는 테두리 모서리의 최종 색을 유지한다.
    for (const auto& pass:passes) {
        if (!pass.validTile) continue;
        const ColorMap* map=pass.shade==1 ? &maps.bright : pass.shade==2 ? &maps.dark : nullptr;
        // 각 그림 기준점은 전체 화면 격자에 고정되어 여러 버튼이 같은 질감 위상을 공유한다.
        for (const auto origin:pass.origins) DrawIndexedImage(canvas.indices,static_cast<int>(canvas.width),static_cast<int>(canvas.height),texture,
            Add(origin.x,frameOffset.x),Add(origin.y,frameOffset.y),pass.clip,map);
    }
}
}
