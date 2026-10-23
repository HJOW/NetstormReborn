// 원본메뉴명령의현재전역·진단뒤계속·외부효과순서를보존한다.
#include "client/GameMenuActions.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::client {
namespace {
// 이번에분리한실제원문명령만조회하며MenuJustClosed는부모Gump에남긴다.
constexpr std::array<GameMenuActionCommand,9> kActions{GameMenuActionCommand::QuitApp,GameMenuActionCommand::RequestQuitApp,
    GameMenuActionCommand::SoundVolume,GameMenuActionCommand::MusicVolume,GameMenuActionCommand::ReloadTheams,
    GameMenuActionCommand::ViewHome,GameMenuActionCommand::ViewYourPriest,GameMenuActionCommand::Pause,GameMenuActionCommand::HideBuildings};
// 도달한외부효과를미연결성공으로처리하지않는다.
template<class T> const T& Required(const T& hook) { if (!hook) throw std::runtime_error("현재게임메뉴의실제효과미연결");return hook; }
// 정렬되지않은Player/Form원문DWORD를현재바이트에서읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("현재게임메뉴의실제기록범위");return bytes[offset]|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24);
}
}
// 명령값조회는순수현재장치이며나머지는선택한분기에서요구한다.
GameMenuActions::GameMenuActions(GameMenuActionGlobals globals,GameMenuActionHooks hooks):globals_(std::move(globals)),hooks_(std::move(hooks)) { Required(globals_.read);Required(hooks_.command); }
// 전체UserInput전처리가소비하지않은일반명령의효과만실행한다.
std::optional<std::uint32_t> GameMenuActions::Handle(std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    static_cast<void>(first);const auto code=event&0x03ffffffU;std::optional<GameMenuActionCommand> selected;
    for (auto action:kActions) if (code==hooks_.command(action)) { selected=action;break; } // 정상원문이름표의서로다른현재ID중대상만고른다.
    if (!selected) return {};if (event&0x80000000U) return 0;
    switch (*selected) {
    case GameMenuActionCommand::QuitApp:
        if (second!=99) Required(hooks_.assertion)(993);Required(hooks_.sound)("selectTab.wav",0,0,0,0,0);return 1;
    case GameMenuActionCommand::RequestQuitApp:
        if (globals_.read(0x005c85a4)) Required(hooks_.tell)("QuitEditMode");
        else if (!globals_.read(0x00594fb8) && !globals_.read(0x00594fc8)) Required(hooks_.tell)("QuitAbandon");
        else Required(hooks_.queue)(hooks_.command(GameMenuActionCommand::QuitApp),0,99);
        break;
    case GameMenuActionCommand::SoundVolume:
    case GameMenuActionCommand::MusicVolume: {
        const auto assignment=std::string(*selected==GameMenuActionCommand::SoundVolume ? "soundVolume=" : "musicVolume=")+std::to_string(std::bit_cast<std::int32_t>(second)); // 원문%d는signedDWORD를그대로전달한다.
        Required(hooks_.config)(assignment);Required(hooks_.sound)("priestmove.wav",0,0,0,0,0);break;
    }
    case GameMenuActionCommand::Pause:
        if (globals_.read(0x00594fa4) || globals_.read(0x005c85a4)) {
            const bool paused=Required(hooks_.paused)();Required(hooks_.status)(11,0,paused ? 0U : 1U,0,"");
            if (paused) { Required(hooks_.resumeClock)();Required(hooks_.resumeSound)(); }
            else { Required(hooks_.cancelCursor)();Required(hooks_.pauseClock)();Required(hooks_.muteSound)(); }
        }
        break;
    case GameMenuActionCommand::ViewHome:
        if (globals_.read(0x00595344) && !globals_.read(0x00594fc8)) {
            if (!globals_.read(0x00540c70)) Required(hooks_.assertion)(1895);const auto player=Required(hooks_.player)(globals_.read(0x00540c70));
            if (!player.empty()) { const auto firstSid=Read(player,0x7c);if (firstSid) Required(hooks_.centerObject)(firstSid);else { const auto secondSid=Read(player,0x84);if (secondSid) Required(hooks_.centerObject)(secondSid); } }
        }
        break;
    case GameMenuActionCommand::ViewYourPriest:
        Required(hooks_.viewPriest)(second);break;
    case GameMenuActionCommand::HideBuildings:
        globals_.redraw=1;globals_.hideBuildings=globals_.hideBuildings==0;break;
    case GameMenuActionCommand::ReloadTheams:
        if (!globals_.read(0x00594fc8)) {
            Required(hooks_.reloadCount);Required(hooks_.reloadSid);Required(hooks_.form);Required(hooks_.reloadAt);
            for (auto list:{GameMenuReloadList::First,GameMenuReloadList::Second}) { // 원문은첫목록을끝낸뒤현재둘째목록을조회한다.
                for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<hooks_.reloadCount(list);++index) { // 콜백뒤현재개수를다시읽고다음SID/슬롯도현재주소에서읽는다.
                    const auto sid=hooks_.reloadSid(list,index);const auto form=hooks_.form(sid);const auto x=Read(form,0x0e),y=Read(form,0x12);hooks_.reloadAt(x,y,0);
                }
            }
        }
        break;
    }
    return 0;
}
}
