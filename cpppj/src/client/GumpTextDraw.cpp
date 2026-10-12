// 측정 폭/ABC A와 출력 전진 폭을 구분하고 빈 문자열에서도 원본의 색 표 설치를 유지한다.
#include "client/GumpTextDraw.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// x86의 signed 좌표 덧셈을 하위 DWORD로 보존하며 C++ signed overflow를 피한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
// 실제 두 PE의 고정 외곽선 좌표 표 순서다. 위→오른쪽→아래→왼쪽으로 그린다.
constexpr std::array<ScreenPoint,4> kOutline{{{0,-1},{1,0},{0,1},{-1,0}}};
}
// 문자열 범위를 먼저 확인해 잘못된 호스트 입력에서 일부 색/픽셀만 변경되는 일을 피한다.
void DrawBitmapGlyphText(std::span<const char> bytes,const BitmapFont* font,ScreenPoint position,
    const GumpTextContext& context,const GumpGlyphHooks& hooks,std::int32_t length) {
    std::size_t count=0;
    if (length==-1) {
        const auto end=std::find(bytes.begin(),bytes.end(),'\0');
        if (end==bytes.end()) throw std::invalid_argument("Gump 글자 출력 범위에 NUL 종료 필요");
        count=static_cast<std::size_t>(end-bytes.begin());
    } else {
        if (length<0 || static_cast<std::size_t>(length)>bytes.size()) throw std::invalid_argument("Gump 글자 출력 길이 범위 오류");
        count=static_cast<std::size_t>(length);
    }
    if (!hooks.color || !hooks.glyph) throw std::invalid_argument("Gump 색/글리프 출력 경계 필요");
    // 패스마다 x를 초기화하고 색을 먼저 설치한다. 포인터가 없는 글리프는 폭도 전진하지 않는다.
    const auto pass=[&](ScreenPoint offset,std::uint32_t color) {
        hooks.color(static_cast<std::uint8_t>(color));
        ScreenPoint at{Add(position.x,offset.x),Add(position.y,offset.y)};
        // 양수 길이에는 코드 0과 128~255도 부호 없는 폭/포인터 표 색인으로 출력한다.
        for (std::size_t i=0;i<count;++i) {
            const auto code=static_cast<std::uint8_t>(static_cast<unsigned char>(bytes[i]));
            if (!font) continue;
            const auto& glyph=font->Glyph(code);
            if (!glyph.shape) continue;
            hooks.glyph(*font,code,at);at.x=Add(at.x,glyph.drawAdvance);
        }
    };
    if (context.flags&2U) pass({context.shadowX,context.shadowY},context.shadowColor);
    // 원본의 고정 네 방향 표를 순서대로 소비한다. 그림자와 외곽선은 독립 플래그다.
    if (context.flags&4U) for (const auto offset:kOutline) pass(offset,context.shadowColor);
    pass({},context.color);
}
// 글리프/색 경계는 빈 입력에서도 호출되므로 생성할 때 모두 확인한다.
GumpTextDraw::GumpTextDraw(FontStore& fonts,GumpGlyphHooks hooks,o::OriginalEdition edition)
    :fonts_(fonts),hooks_(std::move(hooks)),edition_(edition) {
    if (!hooks_.color || !hooks_.glyph) throw std::invalid_argument("Gump 색/글리프 출력 경계 필요");
}
// 글꼴 슬롯 assert의 안전 계약과 원본 스타일 로그/fallback을 별개로 처리한다.
void GumpTextDraw::Draw(std::span<const char> bytes,ScreenPoint position,const GumpTextContext& context,std::int32_t length) const {
    auto slot=std::bit_cast<std::int32_t>(context.font);FontStore::LogicalHeight(slot);
    const auto maximum=edition_==o::OriginalEdition::Patch1078 ? 4U : 2U;
    auto style=context.style;
    if (style>maximum) { if (hooks_.illegalStyle) hooks_.illegalStyle();slot=0;style=0; }
    const auto selected=static_cast<FontStyle>(style);
    const bool cdEmpty=edition_!=o::OriginalEdition::Patch1078 && slot==6 && selected==FontStyle::Bold;
    const auto* font=FontStore::Available(slot,selected) && !cdEmpty ? &fonts_.Get(slot,selected) : nullptr;
    DrawBitmapGlyphText(bytes,font,position,context,hooks_,length);
}
// Position은 같은 노드 캐시/월드 투영을 조회한다. 원본 wrapper는 항상 길이 -1로 위임한다.
void GumpTextDraw::DrawGump(const GumpCollection& collection,std::uint32_t token,std::span<const char> bytes,ScreenPoint offset,
    const std::array<std::uint32_t,8>& defaultContext,const GumpTextContext* context) const {
    const auto position=collection.Position(token);
    const auto selected=context ? *context : DecodeGumpTextContext(defaultContext);
    Draw(bytes,{Add(position.x,offset.x),Add(position.y,offset.y)},selected);
}
// 색 번호 100만 바꾸고 256바이트 전체를 설치한다. 그림자/외곽선 뒤 본문 표를 복구하지 않는다.
GumpGlyphHooks MakeGumpGlyphPixelHooks(const GumpPixelTarget& target,GumpGlyphColorState& state) {
    GumpGlyphHooks hooks;
    hooks.color=[&state](std::uint8_t color) { state.fontColors[100]=color;state.vfxColors=state.fontColors; };
    // 특별 빈 프레임은 실제 포인터/폭을 갖지만 출력 픽셀이 없다. 일반 프레임의 상대 영역만 합성한다.
    hooks.glyph=[&target,&state](const BitmapFont& font,std::uint8_t code,ScreenPoint at) {
        const auto& shape=*font.Glyph(code).shape;const auto& frame=shape.Blocks()[0].frames[0];
        if (!frame.IsSpecial()) target.Image(shape.Decode(0,0),{Add(at.x,frame.rect.left),Add(at.y,frame.rect.top)},&state.vfxColors);
    };
    return hooks;
}
}
