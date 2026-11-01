// 기존 MenuClient의 실제 자산·등록표·입력으로 메인 팝업의 모달/위치/명령/정리를 확인한다.
#include "client/RawGameMenus.h"

// Intro가 존재하는 실제9버튼도 둘째 행5개를 따로 가운데에 놓으며 표시/입력의 실제 가족을 유지한다.
TEST_CASE(RawMenuControls_MainLayoutNineButtonsWithIntro) {
    MenuClient client;client.Resize(1024,768);const std::array<std::string,9> labels{{"Campaign","Multiplayer","Demo","Help","Edit","Intro","Credits","Options","Quit"}};std::array<GumpControl,9> controls{};
    for (std::size_t index=0;index<controls.size();++index) controls[index]={static_cast<int>(index),{10,10,20,20},false,true,false}; // 원문 순서의 실제9개 돌 버튼을 아직 겹친 임시 위치에 생성한다.
    const auto& asset=client.assets.Find("fortGump");const auto frame=client.assets.FrameIndex(asset,"A01",0);const auto metrics=client.assets.Shapes().SquidMetrics(asset.block,frame);
    client.buttons.SetControls(controls,labels,1024,768,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(client.Palette())},true,false,-83);client.buttons.LayoutMainMenuControls(controls);
    for (std::size_t index=0;index<controls.size();++index) { // 첫 행4개는356, 둘째 행5개는316.5에서 시작하며 공통 Bounds는 정수316을 반환한다.
        const auto x=index<4 ? 356+static_cast<int>(index)*79 : 316+static_cast<int>(index-4)*79;const auto y=index<4 ? 311 : 334;
        CHECK(controls[index].rect.left==x && controls[index].rect.top==y && controls[index].rect.right==x+75 && controls[index].rect.bottom==y+20);
        CHECK(client.buttons.PickGump({x+37,y+10})==client.buttons.ControlGump(static_cast<int>(index)));
    }
}

namespace { //실제 원본 메인 부모를 독립 메뉴 검사와 같은 자산/등록표에 구성한다.
std::array<GumpControl,8> InstallTestMain(MenuClient& client) { //8버튼의 실제 부모/모달/idle과 배치까지 생성한다.
    std::array<GumpControl,8> controls{};const std::array<std::string,8> labels{{"Campaign","Multiplayer","Demo","Help","Edit","Credits","Options","Quit"}};
    for (std::size_t index=0;index<controls.size();++index) controls[index]={static_cast<int>(index),{10,10,20,30},false,true,true}; //actual 부모 통지가 누름 즉시에 발생하는 원문 버튼을 공급한다.
    const auto& asset=client.assets.Find("fortGump");const auto frame=client.assets.FrameIndex(asset,"A01",0);const auto metrics=client.assets.Shapes().SquidMetrics(asset.block,frame);
    client.buttons.SetControls(controls,labels,client.screenWidth,client.screenHeight,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(client.Palette())},true,false,-83,true);client.buttons.LayoutMainMenuControls(controls);return controls;
}
}
//실제 모달 부모가 Help/HTML 뒤에서 보존되고 일반 버튼의 같은 callback 안에서 자식과 함께 Kill되는지 확인한다.
TEST_CASE(RawMenuControls_MainParentModalPreserveAndKill) {
    MenuClient client;auto controls=InstallTestMain(client);const auto parent=client.buttons.MainMenuToken(),button=client.buttons.ControlGump(0);CHECK(parent && client.buttons.CurrentDialog()==parent && client.buttons.DialogCount()==1 && client.buttons.GumpDepth(parent)==std::uint32_t(-80));
    client.Click({controls[3].rect.left+10,controls[3].rect.top+10});CHECK(client.buttons.MainMenuToken()==parent);client.buttons.Cancel();
    client.menus->OpenMainMenuPopup(client.buttons.ControlGump(3),false,[](const auto& widgets,const auto& controls,const auto& menu) { widgets.BeginVertical();controls.AddText("Item",0x2000017U,0);return menu.TakeCurrent(); });CHECK(client.buttons.DialogCount()==2 && client.buttons.MainMenuToken()==parent);client.buttons.RequestClosePopups();CHECK(client.buttons.CurrentDialog()==parent);client.buttons.CleanupGumps();
    client.EnableHelp();const auto help=client.menus->OpenHelp(client.Topic("F1Help"));CHECK(help && client.menus->HelpOpen() && client.buttons.MainMenuToken()==parent && client.buttons.CurrentDialog()==help);client.menus->CloseHelp();CHECK(client.buttons.CurrentDialog()==parent); //실제005cad28 serial 허용이 없으면 메인 모달에서 Help가 열리지 않는다.
    const auto html=client.menus->ShowMissionLoading(client.LoadingContext({0,0,640,480}),"Loading",[] { return "Cancel"; });CHECK(html && client.htmlOwner->window.instances==1 && client.buttons.CurrentDialog()!=parent);
    const std::vector<GumpControl> empty;const std::vector<std::string> labels;const auto& asset=client.assets.Find("fortGump");const auto frame=client.assets.FrameIndex(asset,"A01",0);const auto metrics=client.assets.Shapes().SquidMetrics(asset.block,frame);
    client.buttons.SetControls(empty,labels,640,480,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(client.Palette())},true,true);CHECK(client.buttons.MainMenuToken()==parent && client.buttons.ControlGump(0)==button);client.menus->CloseMissionLoading();client.buttons.CleanupGumps();CHECK(client.buttons.CurrentDialog()==parent && !client.htmlOwner->window.instances);
    controls=InstallTestMain(client);CHECK(client.buttons.MainMenuToken()==parent && client.buttons.ControlGump(0)==button);client.Click({controls[0].rect.left+10,controls[0].rect.top+10});CHECK(!client.buttons.MainMenuToken() && !client.buttons.CurrentDialog() && client.buttons.ControlGump(0)==button);client.buttons.CleanupGumps();CHECK(!client.buttons.ControlGump(0));
}
//실제 idle 표/루트 팝업/HTML 인스턴스/현재 포인터와45초 만료의 같은 큐/Kill 경로를 확인한다.
TEST_CASE(RawMenuControls_MainParentActivityRealIdleAndPopup) {
    std::uint32_t enabled=1;MenuClient client;int queues{};client.buttons.BindMainMenu({[&] { return enabled; },[&] { return std::bit_cast<std::int32_t>(client.htmlOwner->window.instances); },[&](auto code,auto a,auto b) { CHECK(code==0x20000c0U && a=="DemoStormRages" && b=="DemoStormRages");++queues; }});InstallTestMain(client);const auto parent=client.buttons.MainMenuToken();
    client.wallSeconds=44.99;client.buttons.IdleGumps();CHECK(!queues && client.buttons.CurrentDialog()==parent);
    client.buttons.NotifyPointer({InputCode::kRelease,30,40},0,false);client.wallSeconds=45;client.buttons.IdleGumps();CHECK(!queues && client.buttons.MainMenuReport().find("\t90\t30\t40")!=std::string::npos);
    client.menus->OpenMainMenuPopup(client.buttons.ControlGump(3),false,[](const auto& widgets,const auto& controls,const auto& menu) { widgets.BeginVertical();controls.AddText("Item",0x2000017U,0);return menu.TakeCurrent(); });client.wallSeconds=89;client.buttons.IdleGumps();CHECK(!queues && client.buttons.MainMenuReport().find("\t134\t30\t40")!=std::string::npos);client.buttons.RequestClosePopups();client.buttons.CleanupGumps();
    client.wallSeconds=133.99;client.buttons.IdleGumps();CHECK(!queues && client.buttons.MainMenuToken()==parent);client.wallSeconds=134;client.buttons.IdleGumps();CHECK(queues==1 && !client.buttons.MainMenuToken() && !client.buttons.CurrentDialog());client.buttons.IdleGumps();CHECK(queues==1);client.buttons.CleanupGumps();
}

// 원본 글꼴의 최대 맞춤 폭75와 004ce940에서 읽은 여섯 해상도의 위치를 실제 버튼/입력 경계에 대조한다.
TEST_CASE(RawMenuControls_MainLayoutMeasuredWidthAndSixResolutions) {
    const std::array<std::array<int,4>,6> cases{{{640,480,164,215},{800,600,244,255},{1024,768,356,311},{1280,720,484,295},{1280,800,484,321},{1366,768,527,311}}}; // 각 행은 화면 폭/높이와 첫 행의 원문 x/y다. font 파일의 Multiplayer 폭63+12=75다.
    const std::array<std::string,8> labels{{"Campaign","Multiplayer","Demo","Help","Edit","Credits","Options","Quit"}}; // Intro 없는 원본의 실제 표시 순서다.
    for (const auto& entry:cases) { // 창 없이 같은 실제 글꼴·트리로 각 해상도의 측정/부착/적중을 확인한다.
        MenuClient client;client.Resize(entry[0],entry[1]);std::array<GumpControl,8> controls{};
        for (std::size_t index=0;index<controls.size();++index) controls[index]={static_cast<int>(index),{10,10,20,20},false,true,false}; // 새 레이아웃이 초기 임시 폭10/위치10을 재사용하지 못하게 한다.
        const auto& asset=client.assets.Find("fortGump");const auto frame=client.assets.FrameIndex(asset,"A01",0);const auto metrics=client.assets.Shapes().SquidMetrics(asset.block,frame);
        client.buttons.SetControls(controls,labels,entry[0],entry[1],{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(client.Palette())},true,false,-83);
        client.buttons.LayoutMainMenuControls(controls);
        for (std::size_t index=0;index<controls.size();++index) { // 각 입력 영역이 원문 위치/공통 폭/개별 글자 높이이며 적중도 같은 실제 버튼이어야 한다.
            const auto& rect=controls[index].rect;const auto x=entry[2]+static_cast<int>(index%4)*79,y=entry[3]+static_cast<int>(index/4)*23;
            CHECK(rect.left==x && rect.top==y && rect.right==x+75 && rect.bottom==y+20);
            const auto token=client.buttons.ControlGump(static_cast<int>(index));CHECK(client.buttons.PickGump({x+37,y+10})==token && client.buttons.GumpDepth(token)==std::uint32_t(-83));
        }
    }
}

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
