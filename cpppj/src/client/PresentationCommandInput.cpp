// 원문004d7051/004d7295의 선행 종료·현재 인자·재생성·서식 순서를 보존한다.
#include "client/PresentationCommandInput.h"
#include "client/BitmapFont.h"
#include "client/Renderer.h"
#include "o/OriginalText.h"
#include <stdexcept>
#include <string>

namespace netstorm::client { //호스트 UI 선택이나 게임 정지 관문을 일반 명령에 추가하지 않는다.
namespace { //도달한 분기의 누락만 명시 오류로 거부한다.
template<class F> const F& NeedPresentation(const F& hook) { if (!hook) throw std::logic_error("글꼴/음질 명령의 실제 경계 미연결");return hook; } //빈 성공으로 원문 효과를 건너뛰지 않는다.
}
// 기존 FontStore가 글리프를 파기하기 전에 host Renderer의 주소 캐시를 명시 반납한다. 원문에는 없는 호스트 표시 캐시의 수명 처리다.
void RebuildPresentationFonts(FontStore& fonts,Renderer& renderer,std::optional<std::string_view> face) {
    renderer.ClearDecodedImages();fonts.Rebuild(face);
}
// 두 원문 이름의 실제 명령값만 조회한다.
std::uint32_t PresentationCommandScriptCode(std::string_view name) {
    const auto key=o::AsciiLower(name);if (key=="changefontface") return kChangeFontFaceCommand;if (key=="changesoundquality") return kChangeSoundQualityCommand;return 0xffffffffU;
}
// 분류 전에 수식 비트를 제거한 일반 명령값 두 개를 구별한다.
bool IsPresentationCommand(std::uint32_t code) { return code==kChangeFontFaceCommand || code==kChangeSoundQualityCommand; }
// 공통 뗌과 선택 분기의 실패 순서를 보존해 현재 부모로 전달한다.
bool DispatchPresentationCommandInput(InputEvent event,const PresentationCommandHooks& hooks) {
    const auto code=event.code&0x03ffffffU;if (!IsPresentationCommand(code)) return false;if (event.code&InputCode::kRelease) return true;
    if (code==kChangeFontFaceCommand) {
        const auto face=NeedPresentation(hooks.fontFace)(event.y);NeedPresentation(hooks.rebuildFonts)(face);
        const auto height=NeedPresentation(hooks.height)(),width=NeedPresentation(hooks.width)(); //원문은 재생성 뒤 H를 먼저 읽고 W를 읽는다.
        NeedPresentation(hooks.invalidate)(0,0,width,height,0);
        const auto current=NeedPresentation(hooks.currentFontFace)();NeedPresentation(hooks.set)("fontFaceName",current);return true;
    }
    NeedPresentation(hooks.closeSound)();
    NeedPresentation(hooks.config)("soundQuality="+std::to_string(event.y)); //signed %d이며 장치만 허용0..3으로 낮추고 Config 원시 값은 보존한다.
    NeedPresentation(hooks.sound)("priestmove.wav",0,0,0,0,0);return true;
}
}
