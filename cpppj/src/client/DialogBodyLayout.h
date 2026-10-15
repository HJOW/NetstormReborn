// 원본 HTML/StyleText/다중 줄 버퍼를 같은 실제 글꼴과 팔레트로 연결하는 Tell 본문 어댑터다.
#pragma once
#include "client/HtmlTextParser.h"
#include "client/Renderer.h"
#include <memory>

namespace netstorm::client {
class GameAssets; // 실제 그림 타입/프레임/공유 SHP를 소유하는 자산 계층이다.
// 측정은 전체 본문을 대상으로 하며 각 출력 조각은 별도 내부 클립을 갖는다.
struct DialogBodyLayoutResult {
    std::vector<RenderText> text; // 현재 기준선 위치·글꼴·색·그림자·클립을 보관하는 Renderer 입력이다.
    std::int32_t maximumWidth{}; // 실제 StyleTextBuffer 표시 줄들의 signed 최대 폭이다.
    std::int32_t height{}; // 마지막 표시 줄의 y+높이다. TextGump FitHeight의 추가1은 포함하지 않는다.
    std::int32_t lineCount{}; // 자동 줄바꿈을 적용한 실제 표시 줄 수다.
    std::vector<RenderSprite> sprites; // 실제 Markup 그림의 SHP/물리 프레임·기준점·내부 클립 출력이다.
};
// 페이지 설정 때만 HTML 효과를 실행하고 Compose의 반복 배치는 이미 만든 소유 본문을 사용한다.
class DialogBodyLayout {
public:
    // fonts/palette는 본문보다 오래 살아야 한다. 팔레트는 페이지 설정 시 현재 색 번호를 읽는다.
    DialogBodyLayout(FontStore& fonts,const PaletteColorTable& palette,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 그림 타입/프레임/헤더/실제 SHP를 같은 GameAssets에서 공급한다. 자산은 본문/결과보다 오래 살아야 한다.
    DialogBodyLayout(FontStore& fonts,const PaletteColorTable& palette,const GameAssets& assets,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 입력/표시 복제와 그림 파생 표를 실제 StyleText/Markup의 scalar 반납 경계로 정리한다.
    ~DialogBodyLayout();
    // 같은 호출자의 HTML 상태/전역에 출력설정→입력설정→변환을 한 번 수행한다.
    // 새 본문 파싱이 실패하면 이전 성공 본문은 유지하지만 외부 callback 효과를 되돌리지는 않는다.
    void SetBody(std::string_view html,const std::array<std::uint32_t,8>& context,HtmlTextParserState& htmlState,
        HtmlTextParserGlobals& globals,HtmlTextParserHooks hooks={});
    // 양수 폭으로 원본 버퍼를 배치하고 기준선에 맞춘 글자를 반환한다. HTML/명령 효과는 재실행하지 않는다.
    // origin은 전체 본문의 좌상단이고 clip은 viewport 내부 영역이다. 전체 높이는 clip과 독립이다.
    DialogBodyLayoutResult Layout(std::int32_t width,ScreenPoint origin,ScreenRect clip) const;
private:
    struct Impl; // 실제 폰트/색 참조와 현재 본문 소유 저장소를 안정된 주소에 둔다.
    std::unique_ptr<Impl> impl_; // callback이 참조하는 현재 구현을 단독 소유한다.
};
}
