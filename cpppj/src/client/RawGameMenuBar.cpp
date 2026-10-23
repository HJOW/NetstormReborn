// 원본004cc210의 실제 순서를 하위 부모/공유 Gump 처리기에 연결한다.
#include "client/RawGameMenuBar.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 도달한 명시 의존만 요구하여 실제 미복원 하위를 빈 메뉴로 숨기지 않는다.
template<class T> const T& Need(const T& value,const char* text) {
    if (!value) throw std::logic_error(text);return value;
}
}
// 상태는 원본 두 DWORD와 같은 Client 수명이며 하위 호출도 같은 현재 상태를 바꿀 수 있다.
RawGameMenuBar::RawGameMenuBar(GameMenuBarState& state,GameMenuBarHooks hooks):state_(state),hooks_(std::move(hooks)) {}
// 메뉴 생성 후 변경될 수 있는 모드/폭/생성순번은 원문 접근 순서대로 다시 읽는다.
std::uint32_t RawGameMenuBar::Open() const {
    Need(hooks_.prepare,"게임 메뉴의 현재 Player 준비 연결 누락")();
    const auto root=Need(hooks_.horizontal,"게임 메뉴의 실제 가로 생성 연결 누락")(); // 현재 가로 메뉴 토큰이다.
    const auto submenu=[&](std::string_view name,GameMenuBarChild kind) {
        const auto label=Need(hooks_.translate,"게임 메뉴의 실제 번역 연결 누락")(name); // 하위 생성보다 먼저 원본 문자열을 읽는다.
        const auto child=Need(hooks_.child,"게임 메뉴의 실제 하위 부모 연결 누락")(kind); // 같은 controlled 스택에서 꺼낸 하위다.
        Need(hooks_.submenu,"게임 메뉴의 실제 자식 작성 연결 누락")(label,child);
    }; // 원본의 번역→부모 몸체→자식 부착 순서를 공통으로 표현한다.
    submenu("Game",GameMenuBarChild::Game);
    if (Need(hooks_.read,"게임 메뉴의 현재 전역 조회 연결 누락")(GameMenuBarValue::Editor)) submenu("Edit",GameMenuBarChild::Edit);
    submenu("View",GameMenuBarChild::View);
    submenu("Options",GameMenuBarChild::Options);
    state_.checksum=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state_.checksum)+3U);
    if (state_.checksum>9600) state_.checksum=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state_.checksum)-9600U);
    if (hooks_.read(GameMenuBarValue::Battle)) submenu("Players",GameMenuBarChild::Players);
    Need(hooks_.vertical,"About의 실제 세로 생성 연결 누락")();
    auto label=hooks_.translate("General Help - ~iF1~."); // 원본 첫 행의 실제 바이트다.
    Need(hooks_.item,"About의 실제 행 작성 연결 누락")(label,state_.helpCommand,"F1Help",1);
    label=hooks_.translate("Technical Help");
    const auto technical=Need(hooks_.technicalHelp,"기술 도움말의 실제 가용성 조회 연결 누락")(); // enabled가 아니라 include 인자다.
    hooks_.item(label,state_.technicalCommand,{},technical);
    Need(hooks_.separator,"About의 실제 구분선 연결 누락")();
    label=hooks_.translate("Version");
    hooks_.item(label,state_.tellCommand,"About",1);
    auto child=Need(hooks_.take,"게임 메뉴의 실제 controlled 꺼내기 연결 누락")(); // About 하위를 먼저 꺼낸다.
    const auto about=Need(hooks_.about,"현재 About 제목 연결 누락")(); // 원본0054db38은 그대로 읽는다.
    hooks_.submenu(about,child);
    if (hooks_.read(GameMenuBarValue::Cheat) && hooks_.read(GameMenuBarValue::Mission)) {
        child=hooks_.child(GameMenuBarChild::Cheat); // 원본 직접 new/Construct 경계이며 allocation 실패0도 보존한다.
        label=hooks_.translate("Cheat");hooks_.submenu(label,child);
    }
    static_cast<void>(hooks_.take());
    const auto left=hooks_.read(GameMenuBarValue::Left),width=hooks_.read(GameMenuBarValue::Width); // 원본 signed 차이를 DWORD 감김으로 계산한다.
    Need(hooks_.width,"게임 메뉴의 실제 폭 설정 연결 누락")(root,static_cast<float>(std::bit_cast<std::int32_t>(width-left)));
    const auto x=static_cast<float>(std::bit_cast<std::int32_t>(hooks_.read(GameMenuBarValue::Left))); // 폭 설정 뒤 바뀔 수 있어 다시 읽는다.
    const auto parent=hooks_.read(GameMenuBarValue::Root); // 같은 등록표의 현재 루트다.
    Need(hooks_.attach,"게임 메뉴의 실제 루트 부착 연결 누락")(root,x,0,parent,1);
    state_.menuSerial=Need(hooks_.serial,"게임 메뉴의 현재 생성순번 조회 연결 누락")(root);
    return root;
}
// 이미 복원한 가로/세로/항목/크기/가상 부착의 본체를 그대로 공유한다.
void BindGameMenuBarControls(GameMenuBarHooks& hooks,const MenuGumpWidgets& widgets,
    const MenuGumpControls& controls,const MenuGump& menus,
    std::function<const GumpCreateNode*(std::uint32_t)> node,std::function<std::uint32_t(std::string_view)> storeText) {
    hooks.horizontal=[&widgets] { return widgets.BeginHorizontal(); };hooks.vertical=[&widgets] { static_cast<void>(widgets.BeginVertical(0)); };
    hooks.take=[&menus] { return menus.TakeCurrent(); };hooks.separator=[&controls] { controls.AddSeparator(); };
    hooks.submenu=[&controls](std::string_view text,std::uint32_t child) { controls.AddSubmenu(text,child,true,false); };
    hooks.item=[&controls,storeText=std::move(storeText)](std::string_view text,std::uint32_t command,std::string_view parameter,std::uint32_t include) {
        const auto value=parameter.empty() ? 0U : Need(storeText,"게임 메뉴의 실제 문자열 고리 연결 누락")(parameter); // 숫자0과 문자열 수명을 구별한다.
        controls.AddText(text,command,value,include!=0,true,false);
    };
    hooks.width=[&menus](std::uint32_t token,float width) { menus.SetWidth(token,width); };
    hooks.attach=[&widgets](std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) { widgets.Attach(token,x,y,parent,mode); };
    hooks.serial=[node=std::move(node)](std::uint32_t token) {
        const auto current=Need(node,"게임 메뉴의 실제 노드 조회 연결 누락")(token); // 같은토큰의부착후생성순번이며등록칸id와구별한다.
        if (!current) throw std::logic_error("게임 메뉴의 현재 노드 소멸");return current->serial;
    };
}
}
