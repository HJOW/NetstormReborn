// 원본 Screen 글리프 반복과 기본 Gump 상대 좌표 출력을 실제 비트맵 글꼴에 연결한다.
#pragma once
#include "client/GumpText.h"
#include "client/GumpDraw.h"

namespace netstorm::client {
// color는 매 출력 패스의 VFX 색 표 설치, glyph는 실제 포인터가 있는 글리프의 출력 경계다.
// 콜백 실행 중 문자열/문맥/글꼴 입력은 바꾸지 않는다. 특별 빈 프레임도 glyph 호출과 폭 전진을 수행한다.
struct GumpGlyphHooks {
    std::function<void(std::uint8_t)> color;
    std::function<void(const BitmapFont&,std::uint8_t,ScreenPoint)> glyph;
    std::function<void()> illegalStyle; // 원본의 잘못된 스타일 로그 경계다. 생략해도 normal 슬롯 0으로 대체한다.
};
// 004a3f70/CD 004260f0 및 004a3430/CD 00426250의 출력 순서다. nullptr font는 빈 캐시 표다.
// 길이 -1은 범위 안 NUL 앞까지, 양수 길이는 중간 NUL도 그린다. 다른 음수는 호스트에서 거부한다.
// ABC A와 측정 폭은 쓰지 않으며 글리프가 있을 때만 세 번째 표의 폭을 DWORD 감김으로 더한다.
void DrawBitmapGlyphText(std::span<const char> bytes,const BitmapFont* font,ScreenPoint position,
    const GumpTextContext& context,const GumpGlyphHooks& hooks,std::int32_t length=-1);
// 원본 글꼴 표 선택과 Gump 위치 조회를 같은 FontStore/트리 저장소에 연결한다.
class GumpTextDraw {
public:
    // 글꼴/콜백이 참조하는 표면과 색 상태는 출력기보다 오래 살아야 한다.
    GumpTextDraw(FontStore& fonts,GumpGlyphHooks hooks,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 잘못된 스타일은 로그 후 normal 슬롯 0으로 대체한다. 원본이 만들지 않은 조합은 빈 표다.
    void Draw(std::span<const char> bytes,ScreenPoint position,const GumpTextContext& context,std::int32_t length=-1) const;
    // 00465050/CD 00493ed0: 기본/명시 문맥을 선택하고 현재 Position에 상대 오프셋을 더해 NUL 종료 출력한다.
    // 기본 문맥의 해석은 Position 콜백 뒤 수행하므로 투영이 바꾼 현재 문맥을 읽는다.
    void DrawGump(const GumpCollection& collection,std::uint32_t token,std::span<const char> bytes,ScreenPoint offset,
        const std::array<std::uint32_t,8>& defaultContext,const GumpTextContext* context=nullptr) const;
private:
    FontStore& fonts_;
    GumpGlyphHooks hooks_;
    o::OriginalEdition edition_;
};
// 원본 글꼴 전용 256색 표와 VFX에 마지막으로 설치한 표다. 클립 복구 후에도 본문 색을 유지한다.
struct GumpGlyphColorState { ColorMap fontColors{},vfxColors{}; };
// 글리프 해제/투명 합성을 현재 GumpPixelTarget 클립에 연결한다. 표면/색 상태는 훅보다 오래 살아야 한다.
GumpGlyphHooks MakeGumpGlyphPixelHooks(const GumpPixelTarget& target,GumpGlyphColorState& state);
}
