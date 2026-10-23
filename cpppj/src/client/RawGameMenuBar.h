// 원본10.78의 화면 위 가로 Game 메뉴막대004cc210을 같은 controlled 메뉴에 작성한다.
#pragma once
#include "client/MenuGumpControls.h"
#include "client/MenuGumpWidgets.h"
#include <functional>

namespace netstorm::client {
// 부모가 각 접근 시점에 다시 읽는 서로 다른 원본 전역이다.
enum class GameMenuBarValue { Editor,Battle,Cheat,Mission,Width,Left,Root };
// 각 하위 부모는 실제 controlled 메뉴에서 자신의 완성 토큰을 꺼내 반환한다.
enum class GameMenuBarChild { Game,Edit,View,Options,Players,Cheat };
// 원본0054d4e0의 검산 DWORD와005cad24의 실제 생성순번을 같은 Client 수명에 보관한다.
struct GameMenuBarState {
    explicit GameMenuBarState(std::int32_t& tracking):checksum(tracking) {} // 같은SpStore/사제0054d4e0소유주소를필수로빌린다.
    std::int32_t& checksum; // DWORD감김뒤signed>9600일때만9600을한번뺀다. 별도값복사를소유하지않는다.
    std::uint32_t menuSerial{}; // 토큰/등록배열칸id(+1c)와구별하는Gump+2c의현재생성순번이다.
    std::uint32_t helpCommand{0x02000017U},technicalCommand{0x02000049U},tellCommand{0x02000095U}; //00542754/0054281c/00542948의 현재 명령 전역이다.
};
// 큰 하위 부모와 실제 GUI·번역·현재 전역은 명시 경계다. 누락을 빈 성공으로 바꾸지 않는다.
struct GameMenuBarHooks {
    std::function<std::uint32_t(GameMenuBarValue)> read; // 각 원문 조회 시점의 현재 값을 반환한다.
    std::function<void()> prepare; //00491680의 현재 Player/검산 준비를 먼저 실행한다.
    std::function<std::string(std::string_view)> translate; // 원본 바이트 번역이며 하위 작성보다 먼저 호출한다.
    std::function<std::string()> about; //0054db38이 가리키는 현재 About 제목이며 별도 재번역하지 않는다.
    std::function<std::uint32_t(GameMenuBarChild)> child; // 실제 Game/Edit/View/Options/Players와 선택 Cheat 생성이다.
    std::function<std::uint32_t()> horizontal,take,technicalHelp; //00477a10·00475c00·00435ef0의 현재 반환값이다.
    std::function<void()> vertical,separator; // About의 세로 메뉴 생성/구분선이다.
    std::function<void(std::string_view,std::uint32_t)> submenu; // enabled1/checked0으로 현재 부모에 붙인다.
    std::function<void(std::string_view,std::uint32_t,std::string_view,std::uint32_t)> item; // label/code/문자열인자/include이며 enabled1/force0이다.
    std::function<void(std::uint32_t,float)> width; // 원본 가상 폭 설정이며 이후 Left를 다시 읽는다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 토큰/x/y/실제root/mode1이다.
    std::function<std::uint32_t(std::uint32_t)> serial; // 부착 후 현재 Gump+2c의생성순번을읽는다.
};
// 원본 부모 전체의 메뉴 순서·조건·조회·폭·부착·생성순번 쓰기를 복원한다.
class RawGameMenuBar {
public:
    RawGameMenuBar(GameMenuBarState& state,GameMenuBarHooks hooks); // 같은 실제 상태와 명시 경계를 빌린다.
    std::uint32_t Open() const; // 완성한 실제 가로 토큰을 반환하며 controlled 스택은 비운다.
private:
    GameMenuBarState& state_; // 현재 검산/메뉴ID의 단일 소유 저장소다.
    GameMenuBarHooks hooks_; // 도달한 경계만 호출 시점에 요구한다.
};
// GUI 작성/폭/부착만 실제 공유 처리기에 연결한다. 문자열 토큰은 같은 ScriptStringRing 공급자가 맡는다.
void BindGameMenuBarControls(GameMenuBarHooks& hooks,const MenuGumpWidgets& widgets,
    const MenuGumpControls& controls,const MenuGump& menus,
    std::function<const GumpCreateNode*(std::uint32_t)> node,
    std::function<std::uint32_t(std::string_view)> storeText);
}
