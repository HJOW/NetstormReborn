#include "client/UtilityCommandInput.h"
#include "o/OriginalText.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { //기존부모의반환/실패를임의성공으로바꾸지않는일반명령분배다.
namespace { //원문등록표와도달한하위장치검사를외부이름과구별한다.
constexpr std::array<std::pair<std::string_view,std::uint32_t>,5> kCommands{{{"takescreenshot",kTakeScreenShotCommand},{"urlalias",kUrlAliasCommand},{"run",kRunCommand},{"moveisland",kMoveIslandCommand},{"lookatsid",kLookAtSidCommand}}}; //원문이름표의다섯쌍이며별칭을추측추가하지않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("일반유틸리티명령의실제효과미연결");return function; } //선택되지않은조회/효과는요구하지않는다.
std::string OriginalString(std::string_view text) { return std::string(text.substr(0,text.find('\0'))); } //원문strcpy의첫NUL까지를소유하며후속공유스크래치변경과구별한다.
}
std::uint32_t UtilityCommandScriptCode(std::string_view name) { //원문stricmp의ASCII이름비교와미등록ffffffff를유지한다.
    const auto lower=o::AsciiLower(name);for (const auto& [text,code]:kCommands) if (lower==text) return code; //실제등록순서의동일이름만조회한다.
    return 0xffffffffU;
}
bool IsUtilityCommand(std::uint32_t code) { //상위입력표시만제외하고명령번호전체를비교한다.
    code&=0x03ffffffU;for (const auto& [name,value]:kCommands) if (code==value) return true; //하위BYTE만같은문자/마우스는명령으로취급하지않는다.
    return false;
}
bool DispatchUtilityCommandInput(InputEvent event,const UtilityCommandHooks& hooks) { //원문의다섯일반명령에는추가정지/Editor/소유자관문이없다.
    const auto code=event.code&0x03ffffffU;if (!IsUtilityCommand(code)) return false;if (event.code&InputCode::kRelease) return true;
    const auto parameter=std::bit_cast<std::uint32_t>(event.y); //원문은두번째인자를전체DWORD로보존한다.
    if (code==kTakeScreenShotCommand) { Need(hooks.screenshot)();Need(hooks.tell)("TellGifWritten"); }
    else if (code==kUrlAliasCommand) {
        auto key=OriginalString(Need(hooks.text)(parameter));key+=" URL"; //설정조회전에원문별칭문자열과접미사를완성한다.
        if (key.size()>=256) throw std::length_error("URLAlias의원문256BYTE키범위"); //원문버퍼밖쓰기는Windows10/11호스트에서명시거부한다.
        std::string target;if (Need(hooks.readString)(key,target)) { target=OriginalString(target);if (target.size()>=256) throw std::length_error("URLAlias의원문256BYTE설정값범위");Need(hooks.url)(target); } //조회실패는기존링크정책도실행하지않는다.
    } else if (code==kRunCommand) { const auto text=OriginalString(Need(hooks.text)(parameter));Need(hooks.run)(text,1); } //cf730의mode1은첫HTML처리만실행한다.
    else if (code==kMoveIslandCommand) Need(hooks.highlight)(6); //004d76f8은 param2를 읽지 않고 고정 ChalSailPos 모드6을 전달한다. MP 전용 효과 검사는 생략한다.
    else Need(hooks.center)(parameter);
    return true;
}
}
