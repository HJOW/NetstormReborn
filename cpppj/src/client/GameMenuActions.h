// 원본 UserInput의 실제 메뉴 일반명령 부분을 같은 현재 세션/Client 효과에 연결한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace netstorm::client {
// 숫자는원문명령전역의주소식별자이며현재명령ID는command훅에서읽는다.
enum class GameMenuActionCommand : std::uint32_t {
    QuitApp=0x00542700,RequestQuitApp=0x00542944,SoundVolume=0x00542a8c,MusicVolume=0x00542a90,
    Pause=0x00542a30,ViewHome=0x005429ac,ViewYourPriest=0x005429d4,HideBuildings=0x00542ab8,ReloadTheams=0x00542b54
};
// 두실제SID목록은원문0055a70c/714와005e189c/18a4다.
enum class GameMenuReloadList { First,Second };
// bool보유세션도복제하지않도록읽기는현재주소getter,쓰기는실제DWORD참조로연결한다.
struct GameMenuActionGlobals {
    std::function<std::uint32_t(std::uint32_t)> read; //005c85a4/594fb8/594fc8/594fa4/595344/540c70을호출위치에서현재조회한다.
    std::uint32_t& hideBuildings; //0059a8ac의현재건물숨김값이다.
    std::uint32_t& redraw; //0059a8b0의현재월드갱신요청이다.
};
// 해당분기에서선택한효과만필수이며미연결효과를성공으로대체하지않는다.
struct GameMenuActionHooks {
    std::function<std::uint32_t(GameMenuActionCommand)> command; // 실제명령ID의순수현재조회다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue; // 실제PushCommand의동일큐다.
    std::function<void(std::string_view)> tell; //004cf960의실제Tell파서/설치다.
    std::function<void(std::string_view)> config; //00441580의실제설정문해석/적용이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> sound; //004a9cb0의원문6인자재생이다.
    std::function<bool()> paused; //00460de0의현재signed깊이양수질의다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::string_view)> status; //004d3090의실제상태안내생성이다.
    std::function<void()> cancelCursor,pauseClock,resumeClock,muteSound,resumeSound; //446e70/460df0/460e30/4aa900/4aa970의현재효과다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> player; // 현재Player+7c/+84를읽는동일기록이며null포인터는빈span이다.
    std::function<void(std::uint32_t)> centerObject,viewPriest; //433ba0의명시SID와4950d0의명시owner다.
    std::function<std::int32_t(GameMenuReloadList)> reloadCount; // 각목록의현재signed개수다.
    std::function<std::uint32_t(GameMenuReloadList,std::uint32_t)> reloadSid; // 현재목록주소의현재SID다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> form; // 같은SID의최소0x16BYTE원문을빌린다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> reloadAt; //455380의현재X/Y float비트와0이다.
    std::function<void(std::uint32_t)> assertion; // QuitApp line993/ViewHome line1895의실제진단이다.복귀시계속한다.
};
// Guide/Gump가소비하지않은UserInput일반명령몸체에연결한다.전체4d62b0의OS/폴링대체가아니다.
class GameMenuActions {
public:
    // 실제전역/장치수명은이처리기와명령콜백보다길어야한다.
    GameMenuActions(GameMenuActionGlobals globals,GameMenuActionHooks hooks);
    // 미대상은nullopt,처리된일반명령은0,QuitApp의원문즉시반환은1이다.
    // release는대상명령의효과를실행하지않고0이다.MenuJustClosed는공통Gump부모가처리하므로미대상이다.
    std::optional<std::uint32_t> Handle(std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
private:
    GameMenuActionGlobals globals_; // 실제현재전역의참조만보관한다.
    GameMenuActionHooks hooks_; // 같은Client/Session/큐의명시효과다.
};
}
