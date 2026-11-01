// 기존 MenuClient의 실제 자산·등록표·입력으로 메인 팝업의 모달/위치/명령/정리를 확인한다.
#include "client/RawGameMenus.h"

// Help 세 항목을 누를 수 있는 실제 메뉴로 작성하고, 바깥 클릭 뒤에도 원래 돌 버튼은 같은 수명으로 남아야 한다.
TEST_CASE(RawMenuControls_MainPopupHelpModalSelectionAndRetirement) {
    MenuClient client;const auto button=client.buttons.ControlGump(0);CHECK(button && !client.buttons.ControlGump(99));
    const auto popup=client.menus->OpenMainMenuPopup(button,false,[](const auto& widgets,const auto& controls,const auto& menu) {
        widgets.BeginVertical(0);controls.AddText("General Help - ~iF1~.",0x02000017U,0x70000065U);
        controls.AddText("Technical Help",0x02000049U,0,true,false);controls.AddText("Version",0x02000095U,101);return menu.TakeCurrent();
    });
    const auto* node=static_cast<const MenuGumpNode*>(client.menus->Node(popup));CHECK(node && node->parent==client.buttons.RootToken());
    CHECK(client.buttons.CurrentDialog()==popup && client.buttons.DialogCount()==1 && node->depth==std::uint32_t(-90));
    CHECK((node->menuFlags&0x04004000U)==0x04004000U && node->x==50 && node->y==10);
    const auto point=client.menus->Point("menu-command:2000017,70000065");CHECK(point.has_value());if (!point) return;client.Click(*point);
    CHECK(client.commands.size()==1 && client.commands[0][0]==0x02000017U && client.commands[0][2]==0x70000065U);
    CHECK(client.buttons.CurrentDialog()==0 && client.buttons.DialogCount()==0);client.buttons.CleanupGumps();
    CHECK(!client.menus->HasMenus() && client.buttons.ControlGump(0)==button);
}

// 원문 Options를 실제 하위 메뉴에 연결하고, 해상도 선택이 같은 문자열 고리/정수 명령으로 전달되는지 확인한다.
TEST_CASE(RawMenuControls_MainPopupOptionsNativeSubmenuAndOutsideClose) {
    MenuClient client;const auto button=client.buttons.ControlGump(0);ScriptStringRing strings;
    const auto open=[&] { // 매번 새 실제 메뉴만 만들며 기존 돌 버튼/입력 장치는 재생성하지 않는다.
        return client.menus->OpenMainMenuPopup(button,true,[&](const auto& widgets,const auto& controls,const auto& menu) {
            GameMenusHooks hooks;hooks.global=[](auto address)->std::uint32_t {
                if (const auto command=GameMenuCommandValue(address)) return *command;
                switch (address) { case 0x531860:return 640;case 0x531864:return 480;case 0x5c7910:return 1;case 0x531900:case 0x531904:return 3;
                case 0x5c78d4:case 0x5c7b14:case 0x594fbc:case 0x5c85a4:case 0x594fa4:case 0x594fb8:case 0x594fc8:return 0;
                default:throw std::logic_error("메인 Options 검사 미명시 전역"); }
            };
            hooks.config=[](auto) { return 0; };hooks.readInt=[](auto,std::int32_t&) {};hooks.translate=[](auto text) { return std::string(text); };
            BindGameMenusControls(hooks,widgets,controls,menu,[&](auto text) { return strings.Store(text); });return RawGameMenus(std::move(hooks)).Options();
        });
    };
    auto popup=open();const auto* node=static_cast<const MenuGumpNode*>(client.menus->Node(popup));
    CHECK(node && node->x==50 && node->y==0 && node->depth==std::uint32_t(-90)); // 원문 y=10-40을 실제 Attach가 화면 위쪽0으로 제한한다.
    CHECK(client.buttons.CurrentDialog()==popup && (node->menuFlags&0x04000000U));
    CHECK(client.menus->Report().find("Pass Server Diagnostic")==std::string::npos && client.menus->Report().find("Counterclockwise Piece Rotation")!=std::string::npos);
    const auto resolution=client.menus->Point("Resolution");CHECK(resolution.has_value());if (!resolution) return;client.Click(*resolution);
    for (int frame=0;frame<2;++frame) client.buttons.Poll({InputCode::kRelease,resolution->x,resolution->y}); // 원본 확장 프레임이 자식 크기를 확정하도록 한다.
    const auto size=client.menus->Point("1024 by 768");CHECK(size.has_value());if (!size) return;client.Click(*size);
    CHECK(client.commands.size()==1 && client.commands[0][0]==0x02000095U && strings.Resolve(client.commands[0][2])=="SETSIZE(1024,768)");
    client.buttons.CleanupGumps();CHECK(!client.menus->HasMenus() && !client.buttons.DialogCount());
    popup=open();client.Click({639,479});client.buttons.CleanupGumps();
    CHECK(client.commands.size()==1 && !client.menus->HasMenus() && !client.buttons.DialogCount() && client.buttons.ControlGump(0)==button);
}
