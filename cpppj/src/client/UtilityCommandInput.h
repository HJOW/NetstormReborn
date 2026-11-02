// 원문 일반입력의 화면저장/URL별칭/Run/강조/명시SID보기만 기존 부모에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client { // Guide/Gump/뗌 뒤이며 일반 Console/정지 관문보다 앞이다.
inline constexpr std::uint32_t kTakeScreenShotCommand=0x0200001cU,kUrlAliasCommand=0x0200005aU,kRunCommand=0x020000daU; //542768/2860/2a5c의 기존 원문 등록명령이다.
inline constexpr std::uint32_t kMoveIslandCommand=0x02000006U,kLookAtSidCommand=0x020000aaU; //542714는 고정모드6이며54299c는 param2의 전체DWORD SID를 사용한다.
std::uint32_t UtilityCommandScriptCode(std::string_view name); //원문 다섯등록이름만ASCII대소문자를무시하고조회한다.
bool IsUtilityCommand(std::uint32_t code); //같은이름/숫자/버튼허용과일반분배가동일명령표를쓴다.
struct UtilityCommandHooks { //선택한명령이실제로도달한효과만필수로요구한다.
    std::function<std::string_view(std::uint32_t)> text; //현재ScriptStringRing의param2문자열을소비직전에조회한다.
    std::function<void()> screenshot; //실제4db140의기존번호/Config/잠금/GIF저장전체부모다.
    std::function<void(std::string_view)> tell; //스크린샷뒤4cf960의일반Tell이며새캡처active를바꾸지않는다.
    std::function<bool(std::string_view,std::string&)> readString; //441300의현재별칭키를조회하며실패때URL을호출하지않는다.
    std::function<void(std::string_view)> url; //성공한설정값을같은4a00c0링크정책으로소비한다.
    std::function<void(std::string_view,std::uint32_t)> run; //cf970→cf730의mode1이며일반Tell창열기로대체하지않는다.
    std::function<void(std::uint32_t)> highlight; //004d76f8의 고정모드6을 같은004d5bb0에 정적으로 전달한다. 새 MP 검사는 생략한다.
    std::function<void(std::uint32_t)> center; //현재DWORD SID를같은00433ba0/실제Session카메라몸체에전달한다.
};
bool DispatchUtilityCommandInput(InputEvent event,const UtilityCommandHooks& hooks); //다른명령은false,대상뗌은효과없이true다. 정지/모드를새로조회하지않는다.
}
