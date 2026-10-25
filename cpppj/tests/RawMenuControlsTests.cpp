// 실제 공통 트리의 메뉴 입력/하위 메뉴/제목·픽셀/삭제와 기존 버튼 복귀를 검사한다.
#include "RawSceneSupport.h"
#include "client/RawMenuControls.h"
#include "client/RawHtmlWindowContext.h"
#include "client/RawButtonControls.h"
#include "client/RawObjectMenu.h"
#include "client/RawCarrierMenu.h"
#include "client/RawPlayerMenu.h"
#include "client/QuickHelpWindow.h"
#include "client/ScriptCommand.h"
#include "client/HtmlConditionEffect.h"
#include "client/ImageGump.h"
#include "client/RawMinimapControls.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawProductionControls.h"
#include "client/ClientGameplayGlobals.h"
#include <filesystem>
#include <sstream>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 보호 자료는 읽기만 하며 실제 공통 Gump 처리기에 자산/글꼴을 공급한다.
struct MenuClient {
    std::filesystem::path root; // 현재 저장소의 보호10.78 자료 경로다.
    BaseFileSystem files; // 같은 TAFF 읽기다.
    GameAssets assets; // 실제 TYPE/SHP/팔레트다.
    FontStore fonts; // 같은 글꼴 캐시다.
    RawButtonControls buttons; // 같은 실제 공통 등록표/트리/입력/수명이다.
    RawMenuControls* menus{}; // buttons가 소유하는 확장 계층을 빌린다.
    InputEvent poll{InputCode::kRelease,0,0}; // 실제 공통 폴링 경계의 현재 입력이다.
    std::vector<std::array<std::uint32_t,3>> commands; // 원본 입력이 전달한 전체 command/id/parameter다.
    std::vector<std::string> sounds; // 실제 원본 입력 경로의 이름별 음향 요청이다.
    std::uint32_t cursor{}; // 제목 입력이 선택한 실제 커서 번호다.
    double gameSeconds{}; // 비용 표시는 벽시계 대신 이 현재 게임 시각을 읽는다.
    HtmlTextParserGlobals helpGlobals;ScriptStringRing helpStrings;std::unique_ptr<ConfigInterface> helpConfig; // 창 조립에 쓰는 실제 HTML 전역/80칸 고리/조건 설정이다.
    std::shared_ptr<ClientHtmlWindowGlobals> htmlOwner=std::make_shared<ClientHtmlWindowGlobals>();ScreenRect htmlView{};int screenWidth{},screenHeight{}; // 실제 장기 HTML 상태와 현재 출력/지도 경계다.
    ~MenuClient() { menus->CloseHtmlWindows(); } // 외부 관측/Config 수명보다 먼저 같은 부모를 반납한다.
    RawHtmlWindowContext LoadingContext(ScreenRect view) { // 실제 전체 공장/Config/원시 큐를 기존 공통 트리에 연결한다.
        htmlView=view;if (!helpConfig) helpConfig=std::make_unique<ConfigInterface>([this](auto path) { return files.TryRead(path); });helpConfig->Set("OkButton","$Button=Okay,LeaveBattle,3");RawHtmlWindowContext c;c.globals=htmlOwner;c.wallSeconds=[] { return 0.0; };c.queue=[this](auto command,auto first,auto second) { commands.push_back({command,first,second}); };c.metadata.strings=&helpStrings;c.metadata.commands.lookup=[](auto name) { return name=="LeaveBattle" ? 0x020000c5U : 0xffffffffU; };c.metadata.commands.unknown=[](auto) { throw std::logic_error("안내 검사 미등록 명령"); };
        c.factory.globals=&helpGlobals;BindHtmlPictureText(c.factory.html);BindHtmlConditionConfig(c.factory.html,*helpConfig,assets.Edition());c.factory.substitute=[this](auto text,auto) { return helpConfig->SubstituteLocal(text); };c.factory.fallback=[this] { return helpConfig->PathSpec("OkButton"); };
        c.read=[this](auto address)->std::uint32_t { switch (address) { case 0x531860:case 0x5c78f0:return screenWidth;case 0x531864:case 0x5c78f4:return screenHeight;case 0x5ca9b4:return htmlView.right-htmlView.left;case 0x5ca9cc:return htmlView.left;case 0x5ca9d0:return htmlView.top;case 0x5ca9d4:return htmlView.right;case 0x5ca9d8:return htmlView.bottom;case 0x563e38:return buttons.RootToken();default:throw std::logic_error("안내 검사 미등록 전역"); } };return c;
    }
    // 게임/창을 열지 않고 실제 보호 자산과 모든 복원 모듈을 조립한다.
    explicit MenuClient(OriginalEdition edition=OriginalEdition::Patch1078,std::uint32_t* sharedFullRefresh=nullptr):root(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"),files(root),assets(Register(),edition),fonts(files,"Arial"),
        buttons(edition,{[] { return 0.0; },[this](auto) { return poll; },[] {}},sharedFullRefresh) {
        RawMenuHooks hooks;hooks.command=[this](auto command,auto id,auto argument) { commands.push_back({command,id,argument}); };
        hooks.readInt=[](std::string_view,std::int32_t& value) { value=0; };hooks.sound=[this](std::string_view name) { sounds.emplace_back(name); };
        hooks.poll=[this](auto) { return poll; };hooks.cursor=[this](auto value) { cursor=value; };
        hooks.gameSeconds=[this] { return gameSeconds; };
        menus=&buttons.EnableMenus(assets,fonts,std::move(hooks));menus->SetPalette(Palette());Resize(640,480);
    }
    // GameAssets 생성보다 먼저 실제 archive를 등록한다.
    const BaseFileSystem& Register() { files.RegisterArchive(root/"netstorm.tarc");return files; }
    // 원본 파일 팔레트의 실제 기본/명암 색을 계산한다.
    PaletteColorTable Palette() const {
        std::array<std::uint32_t,256> logical{};
        // RGB 채널을 원본 DWORD 순서로 옮긴다.
        for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(static_cast<std::uint32_t>(color.green)<<8)|(static_cast<std::uint32_t>(color.blue)<<16); }
        return BuildPaletteColorTable(logical,assets.Edition());
    }
    // 공통 트리 크기와 실제 돌 버튼을 같은 SetControls로 갱신한다.
    void Resize(int width,int height) {
        screenWidth=width;screenHeight=height;
        const auto& asset=assets.Find("fortGump");const auto frame=assets.FrameIndex(asset,"A00",0);const auto metrics=assets.Shapes().SquidMetrics(asset.block,frame);
        const std::array<GumpControl,1> controls{{{0,{10,10,90,32},false,true,false}}};const std::array<std::string,1> labels{{"Under"}};
        buttons.SetControls(controls,labels,width,height,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(Palette())});
    }
    // 실제 현재 트리의 hover와 누름을 통과시킨다. 명령 callback을 직접 호출하지 않는다.
    void Click(ScreenPoint point) { poll={InputCode::kLeftButton,point.x,point.y};buttons.Poll(poll);buttons.Event(poll); }
    // 실제 보호 Help/TYPE/자산과 기존 공통 트리를 연결한다. 메인 메뉴와 같은 월드 설치 전 경계다.
    void EnableHelp() {
        RawHelpHooks h;h.globals=&helpGlobals;BindHtmlPictureText(h.html);if (!helpConfig) helpConfig=std::make_unique<ConfigInterface>([this](auto path) { return files.TryRead(path); });helpConfig->SetInt("global.inMission",0);BindHtmlConditionConfig(h.html,*helpConfig,assets.Edition());h.translate=[](auto text) { return std::string(text); };h.config=[](auto key) { return key=="Quick Help Spec" ? std::string("d/help.english") : std::string(key); };h.substitute=[](auto text) { return std::string(text); };
        h.load=[this](auto path) -> std::optional<std::string> { const auto bytes=files.TryRead(path);if (!bytes) return std::nullopt;return std::string(bytes->begin(),bytes->end()); };
        h.text=[this](auto token) { return std::string(helpStrings.Resolve(token&0x0fffffffU)); };h.storeText=[this](auto text) { return Topic(text); };
        h.lookup=[this](auto name) { return assets.TypeTable().Find(name); };h.typeName=[this](auto type) { return assets.TypeTable().Types()[type].name; };
        h.type=[this](auto type) { return ReadAssetQuickHelpType(assets,type); };h.description=[this](auto type) { return assets.TypeTable().Types()[type].description; };h.hitPoints=[this](auto type) { return assets.TypeTable().Types()[type].maxHitPoints; };h.helpFrame=[this](auto type) { return type<70 || type-70>=assets.Types().size() ? -1 : assets.Types()[type-70].definition.specialFrames.helpFrame; };
        h.nearestColor=[this](auto r,auto g,auto b) { std::array<std::uint32_t,256> logical{}; // 현재 보호 팔레트의 실제 RGB 검색을 공급한다.
            // 원본 논리 DWORD의 채널 순서를 유지한다.
            for (std::size_t i=0;i<logical.size();++i) { const auto c=assets.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); }return FindPaletteColor(logical,r,g,b);
        };
        h.queue=[this](auto command,auto,auto) { commands.push_back({command,0,0}); };h.script=[](auto) { throw std::logic_error("이 조립 검사의 cmd 링크 미사용"); };h.external=[](auto) { throw std::logic_error("이 조립 검사의 외부 링크 미사용"); };h.learn=[](auto) { throw std::logic_error("일반 Help OK는 학습을 호출하지 않는다"); };
        h.scripted=h.battle=h.challenge=[] { return false; };h.scriptName=[] { return std::string{}; };menus->EnableHelp(std::move(h));
    }
    // 실제80칸 고리의 같은 수명에 TYPE 관문 밖 별칭을 준다.
    std::uint32_t Topic(std::string_view text) { return 0x70000000U|helpStrings.Store(text); }
};
}
// 실제 전용 부모/자식/공통 등록/스크롤 요청/픽셀과 다음 프레임 지연 삭제를 검사한다. 실행은 후속 통합 단계로 이월한다.
TEST_CASE(RawMenuControls_HelpActualTreePixelsAndOutsideInput) {
    MenuClient client;client.EnableHelp();const auto root=client.buttons.RegisteredGump(0);const auto token=client.menus->OpenHelp(0x61000001U);
    CHECK(client.menus->HelpOpen() && client.buttons.RegisteredGump(0)==root);auto* window=static_cast<QuickHelpContentNode*>(client.menus->Node(token));CHECK(window && window->vtable==0x514ec4 && window->parent==root);if (!window) return;
    // 실제 클라이언트 기본 전역은 TYPE92 fortGump의 프레임0이며 단색 배경과 구별한다.
    for (auto child:{window->pictureBackground,window->bodyBackground}) { const auto* background=client.menus->Background(child);CHECK(background && background->type==92 && background->frame==0); }
    auto* body=static_cast<HtmlBodyGumpNode*>(client.menus->Node(window->body));CHECK(body && body->vtable==0x50bd38 && body->text);if (!body) return;CHECK(body->body[0] && client.menus->Node(body->text)->parent==window->body);
    auto* text=static_cast<ScrollingTextGumpNode*>(client.menus->Node(body->text));CHECK(text->buffer && text->slider);CHECK((text->textFlags&6)==6 && text->scrollRequest==0);
    client.buttons.Event({0x00280000U,0,0});CHECK(text->scrollRequest==10); // 원본 Down의 상대 스크롤 요청을 같은 표시 자식에서 읽는다.
    const auto reused=client.menus->OpenHelp(client.Topic("unitHelp"));CHECK(reused==token && window->body);CHECK(client.menus->Report().find("raw_help_body\t")!=std::string::npos);
    constexpr int pitch=647,height=480; // 행 padding도 실제 Help 클립 밖으로 보존한다.
    std::vector<std::uint8_t> pixels(pitch*height,0x5a);std::array<std::uint32_t,256> logical{};
    // 같은 실제 보호 팔레트로 음영 표를 만든다.
    for (std::size_t i=0;i<logical.size();++i) { const auto c=client.assets.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); }
    client.menus->Draw(pixels,pitch,height,client.Palette(),BuildGumpShadeMaps(logical));CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0x5a; }));
    // 실제 출력의 행 padding에는 전용 창을 그리지 않는다.
    for (int y=0;y<height;++y) for (int x=640;x<pitch;++x) CHECK(pixels[static_cast<std::size_t>(y)*pitch+x]==0x5a);
    CHECK(!client.buttons.Event({InputCode::kLeftButton,20,20}));CHECK(!client.menus->HelpOpen() && client.menus->Node(token) && (window->flags&0x10000));
    const auto child=body->text;client.menus->CleanupFrame();CHECK(!client.menus->Node(token) && !client.menus->Node(child)); // 아래 돌 버튼으로 클릭을 재전달하지 않는다.
}

// 공통 소유 문자열의 교체/우클릭 제목·주제·본문과 실제 scalar 정리를 함께 검사한다.
TEST_CASE(RawMenuControls_OwnedHelpStringsDriveRealMenuAndRelease) {
    std::array<std::uint8_t,87> options{};MenuClient client;client.EnableHelp();RawProductionHooks hooks;hooks.now=[] { return 0.0; };hooks.sound=hooks.tell=[](auto) {};hooks.configInt=[](auto,auto fallback) { return fallback; };hooks.translate=[](auto text) { return std::string(text); };hooks.gameSeconds=[] { return 0.0; }; // Session 없이 실제 원본 생성자에 명시 외부 입력을 제공한다.
    auto& panel=client.buttons.ConstructProduction({MakeRawSurfaceStorage(client.assets),std::make_shared<ClientGameplayGlobals>(),std::make_shared<ClientPlayerRecords>(),options},client.assets,std::move(hooks));std::uint32_t token{}; // 실제 생산 격자의 부모와 같은 공통 등록표다.
    for (std::uint32_t id=1;id<1000;++id) { const auto current=client.buttons.RegisteredGump(id);const auto* node=panel.Node(current);if (node && node->vtable==0x506800 && node->textPointer) { token=current;break; } } // 생성한 다리 격자만 그 실제 포인터로 선택한다.
    CHECK(token!=0);if (!token) return;
    client.menus->SetHelpText(token,"Old owned title");client.menus->SetHelpWord(token,"oldOwned");const auto before=client.menus->Report();CHECK(before.find("raw_gump_help_strings\t8\n")!=std::string::npos);
    client.menus->SetHelpText(token,"Production Window");client.menus->SetHelpWord(token,"teleportView");CHECK(client.menus->Report().find("raw_gump_help_strings\t8\n")!=std::string::npos);
    CHECK(panel.Handle(token,InputCode::kRightButton,300,200)==1);const auto help=client.menus->Point("Help");CHECK(client.menus->Point("Production Window").has_value() && help.has_value());
    CHECK(client.sounds==std::vector<std::string>{"opengump.wav"});if (help) client.Click(*help);CHECK(client.commands.size()==1);
    if (client.commands.size()==1) CHECK(client.commands[0][0]==0x02000017U && client.helpStrings.Resolve(client.commands[0][2]&0x0fffffffU)=="teleportViewHelp"); // 실제 클릭의 주제 토큰을 외부 문자열 고리에서 관측한다.
    client.buttons.KillGump(token);client.buttons.CleanupGumps();CHECK(client.menus->Report().find("raw_gump_help_strings\t6\n")!=std::string::npos);client.buttons.DisableProduction();CHECK(client.menus->Report().find("raw_gump_help_strings\t0\n")!=std::string::npos);
}
// Back/OK는 실제 돌 버튼 serial과 같은 파생 Handle을 통과하고 소멸 뒤 이력/표식을 정리한다.
TEST_CASE(RawMenuControls_HelpActualHistoryButtonsAndReopen) {
    MenuClient client;client.EnableHelp();const auto token=client.menus->OpenHelp(0x61000001U);auto* window=static_cast<QuickHelpContentNode*>(client.menus->Node(token));CHECK(window);if (!window) return;
    client.menus->OpenHelp(client.Topic("unitHelp"));const auto back=client.menus->Node(window->backButton)->serial;
    CHECK(client.buttons.HandleGump(token,0x0200002cU,back,0)==1);const auto* body=static_cast<HtmlBodyGumpNode*>(client.menus->Node(window->body));CHECK(body && std::string(body->body.data()).find("~5~w")!=std::string::npos);
    const auto* ok=client.menus->Node(window->learnButton);const ScreenPoint point{static_cast<int>(window->x+ok->x+ok->width/2),static_cast<int>(window->y+ok->y+ok->height/2)}; // 실제 부착 좌표로 누름/뗌을 보낸다.
    client.Click(point);CHECK(client.buttons.HasCapture());client.poll={InputCode::kLeftButton|InputCode::kRelease,point.x,point.y};client.buttons.Event(client.poll);CHECK(!client.menus->HelpOpen());client.menus->CleanupFrame();CHECK(!client.menus->Node(token) && !client.menus->PopupState().command);
    const auto reopened=client.menus->OpenHelp(0x61000001U);CHECK(reopened && reopened!=token && client.menus->HelpOpen());client.menus->CloseHelp();CHECK(!client.menus->HelpOpen() && !client.menus->Node(reopened));
}
// 실제 Sun Cannon 정보/도움말 전용 프레임이 메인 메뉴에서도 같은 트리/픽셀 처리기를 사용한다.
TEST_CASE(RawMenuControls_HelpActualTypeInfoAndImage) {
    MenuClient client;client.EnableHelp();const auto type=static_cast<std::uint32_t>(client.assets.TypeTable().Find("SunCannon"));const auto info=ReadAssetQuickHelpType(client.assets,type);CHECK(info.range==20 && info.damage==16 && info.cost==400 && info.mana=="s");
    const auto token=client.menus->OpenHelp(type);const auto* window=static_cast<QuickHelpContentNode*>(client.menus->Node(token));CHECK(window && window->picture && window->information);if (!window) return;
    const auto* image=static_cast<ImageGumpNode*>(client.menus->Node(window->picture));CHECK(image && image->type==type && image->mode==8);const auto* pane=static_cast<ScrollingTextGumpNode*>(client.menus->Node(window->information));CHECK(pane && pane->buffer && (pane->flags&1));
    std::array<std::uint32_t,256> logical{};
    // 그림/본문도 같은 실제 보호 팔레트로 출력한다.
    for (std::size_t i=0;i<logical.size();++i) { const auto c=client.assets.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); }
    std::vector<std::uint8_t> pixels(640*480,0x5a);client.menus->Draw(pixels,640,480,client.Palette(),BuildGumpShadeMaps(logical));CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0x5a; }));client.menus->CloseHelp();
}
// 같은 실제 트리/원본 StyleText/아이콘/클립/idle와 다음 프레임 지연 해제를 검사한다.
TEST_CASE(RawMenuControls_FloatingSpActualPixelsAndLifetime) {
    MenuClient client;client.gameSeconds=3.25;
    const auto token=client.menus->ShowSp(800,200,200);const auto* node=client.menus->Node(token);
    CHECK(node && node->vtable==0x50688c && node->depth==0xffffffd8U && node->parent==client.buttons.RegisteredGump(0));
    CHECK(!client.menus->HasMenus() && client.menus->HasOverlay());if (!node) return;
    CHECK(node->baseWidth>0 && node->baseHeight>0 && node->width==node->baseWidth && node->height==node->baseHeight);
    constexpr int height=480,pitch=647; // 실제 픽셀과 행 padding을 분리한다.
    std::vector<std::uint8_t> pixels(pitch*height,0x5a);std::array<std::uint32_t,256> logical{};
    // 보호 팔레트 그대로 그림/글리프 명암 표를 만든다.
    for (std::size_t index=0;index<logical.size();++index) { const auto color=client.assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(static_cast<std::uint32_t>(color.green)<<8)|(static_cast<std::uint32_t>(color.blue)<<16); }
    client.menus->Draw(pixels,pitch,height,client.Palette(),BuildGumpShadeMaps(logical));
    CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0x5a; }));
    // 실제 원본 클립은 비용 표시 경계 밖과 모든 행 padding을 보존한다.
    for (int y=0;y<height;++y) for (int x=0;x<pitch;++x) if (x<200 || y<200 || x>=200+node->width || y>=200+node->height) CHECK(pixels[static_cast<std::size_t>(y)*pitch+x]==0x5a);
    client.gameSeconds=3.5;CHECK(client.buttons.Poll({InputCode::kRelease,0,0}));CHECK(node->y==200);
    client.gameSeconds=3.75;client.buttons.Poll({InputCode::kRelease,0,0});CHECK(node->y==199.5F);
    // 원본은 위치가 음수가 될 때 Kill만 하고 다음 비정지 프레임 정리에서 실제 저장소를 반납한다.
    for (int frame=0;frame<100 && client.menus->HasOverlay();++frame) { client.gameSeconds+=0.25;client.buttons.Poll({InputCode::kRelease,0,0}); }
    CHECK(!client.menus->HasOverlay() && client.menus->Node(token));client.menus->CleanupFrame();CHECK(!client.menus->Node(token));
    CHECK(client.menus->Report().find("raw_sp_gump\t")==std::string::npos);
}
// 같은 등록표에서 실제 원본 메뉴 명령/전체 DWORD·비활성 행을 검사한다.
TEST_CASE(RawMenuControls_CommonTreeInputAndDisabledItem) {
    MenuClient client;const auto root=client.buttons.RegisteredGump(0);client.menus->Open({600,450},[](const auto& widgets,const auto& controls,const auto&) {
        widgets.BeginVertical();controls.AddChecked(true,"Checked",0x020000a0,0x89abcdef);controls.AddText("Forbidden",0x02000060,99,true,false);
    });
    CHECK(client.buttons.RegisteredGump(0)==root && client.menus->HasMenus());const auto regions=client.menus->Controls();CHECK(regions.size()==2);
    CHECK(regions[0].rect.right<=640 && regions[1].rect.bottom<=480 && !regions[1].enabled);
    client.Click({(regions[1].rect.left+regions[1].rect.right)/2,(regions[1].rect.top+regions[1].rect.bottom)/2});CHECK(client.commands.empty());
    client.Click({(regions[0].rect.left+regions[0].rect.right)/2,(regions[0].rect.top+regions[0].rect.bottom)/2});
    CHECK(client.commands.size()==1);if (!client.commands.empty()) CHECK(client.commands[0][0]==0x020000a0 && client.commands[0][1]!=0 && client.commands[0][2]==0x89abcdef);
    client.menus->Close();CHECK(!client.menus->HasMenus() && client.buttons.RegisteredGump(0)==root);
    client.buttons.Event({InputCode::kLeftButton,30,20});const auto activated=client.buttons.Event({InputCode::kLeftButton|InputCode::kRelease,30,20});CHECK(activated && *activated==0);
}
// 실제 fancy 생성/제목/숨긴 Slider·하위 확장/픽셀/공통 삭제를 한 수명에서 검사한다.
// 실제 가로 메뉴의 열 중심·클릭·루트 부착의 원본 지연 교체를 같은 등록표에서 확인한다.
TEST_CASE(RawMenuControls_HorizontalPointAndDeferredReplace) {
    MenuClient client;std::uint32_t priorToken{};client.menus->Open({100,100},[&](const auto& widgets,const auto& controls,const auto&) {
        priorToken=widgets.BeginVertical();controls.AddText("Prior",0x02000060,7);
    });
    const auto prior=client.menus->Point("Prior");CHECK(prior);const auto count=client.menus->MenuInstances();
    std::uint32_t token{};
    client.menus->OpenAttached([&](const auto& widgets,const auto& controls,const auto& menu) {
        token=widgets.BeginHorizontal();controls.AddText("Game",0x020000a0,11);controls.AddText("View",0x020000a0,22);controls.AddText("Options",0x020000a0,33);
        menu.AttachCurrent(85,0,client.buttons.RootToken());return token;
    },false);
    // 호스트 선행 Cleanup은 생략하지만 실제 루트 Attach의 replace 비트는 이전 팝업을 Kill한다.
    CHECK(client.menus->MenuInstances()==count+1);CHECK(!client.menus->Point("Prior"));
    CHECK(client.menus->Node(priorToken) && (client.menus->Node(priorToken)->flags&0x10000U));
    const auto* node=static_cast<MenuGumpNode*>(client.menus->Node(token));CHECK(node && node->id!=node->serial);if (!node) return;
    CHECK(client.buttons.SerialGump(node->serial)==token && client.buttons.SerialGump(node->id)!=token);
    const auto regions=client.menus->Controls();CHECK(regions.size()==3);
    std::optional<ScreenPoint> previous;
    // 현재 세 열은 서로 다른 실제 X와 같은 행 내부 Y를 사용해야 한다.
    for (const auto label:{"Game","View","Options"}) {
        const auto point=client.menus->Point(label);CHECK(point);if (!point) continue;
        CHECK(point->y>=0 && point->y<node->height);if (previous) CHECK(point->x>previous->x);previous=point;
    }
    const auto options=client.menus->Point("Options");CHECK(options);if (!options) return;
    CHECK(GumpControlInput::Contains(regions[2].rect,*options));client.Click(*options);
    CHECK(client.commands.size()==1);if (!client.commands.empty()) CHECK(client.commands[0][0]==0x020000a0 && client.commands[0][2]==33);
    CHECK(client.buttons.SerialGump(node->serial)==0);client.menus->CleanupFrame();CHECK(!client.menus->HasMenus());
}
// 실제 fancy 생성/제목/숨긴 Slider·하위 확장/픽셀/공통 삭제를 한 수명에서 검사한다.
TEST_CASE(RawMenuControls_FancySubmenuPixelsAndCleanup) {
    MenuClient client;client.Resize(320,240);client.menus->Open({290,210},[](const auto& widgets,const auto& controls,const auto& menus) {
        widgets.BeginFancy();controls.AddTitle("Priest\n");widgets.BeginVertical();controls.AddText("Temple",0x02000060,0x007b0032);const auto child=menus.TakeCurrent();
        controls.AddSubmenu("Construct",child);controls.AddSeparator();controls.AddText("About",0x02000027,158);
    });
    const std::map<std::uint32_t,std::uint32_t> kinds{{0x50c6f0,26},{0x50ae7c,57},{0x51454c,46},{0x5116f8,39},{0x5116a8,1},{0x511668,51}}; // 실제 VT와 PE 종류 BYTE다.
    std::map<std::uint32_t,bool> seen;
    const auto palette=client.Palette();const auto dim=std::find_if(palette.named.begin(),palette.named.end(),[](const auto& color) { return color.red==47 && color.green==24 && color.blue==17; }); // 실제 원본 메뉴 shadow/disabled 전역이다.
    CHECK(dim!=palette.named.end());
    // 공통 등록표가 반환한 실제 토큰에서 +20 종류와 +28 위치 정렬을 구별한다.
    for (std::uint32_t id=1;id<kGumpRegistryCapacity;++id) {
        const auto token=client.buttons.RegisteredGump(id);const auto* node=token ? client.menus->Node(token) : nullptr;if (!node) continue;
        const auto expected=kinds.find(node->vtable);CHECK(expected!=kinds.end());if (expected!=kinds.end()) { CHECK(node->command==expected->second && node->alignFlags==0);seen[node->command]=true; }
        if (node->command==26 && dim!=palette.named.end()) CHECK(node->textContext[5]==dim->index);
    }
    CHECK(seen.size()==6);
    const auto point=client.menus->Point("Construct");CHECK(point.has_value());if (!point) return;client.Click(*point);
    // 실제 확장 프레임 두 번으로 목표 크기/expanding을 원본 순서로 갱신한다.
    for (int frame=0;frame<2;++frame) client.buttons.Poll({InputCode::kRelease,point->x,point->y});
    const auto child=client.menus->Point("Temple");CHECK(child.has_value());if (child) CHECK(client.menus->Contains(*child));else std::printf("메뉴 입력 지점 %d,%d\n%s\n%s",point->x,point->y,client.buttons.Report().c_str(),client.menus->Report().c_str());
    constexpr int width=320,height=240,pitch=327; // 실제 화면과 행 padding을 구분하는 입력이다.
    std::vector<std::uint8_t> pixels(pitch*height,0x5a);std::array<std::uint32_t,256> logical{};
    // 실제 보호 팔레트의 명암 표를 같은 픽셀 출력에 공급한다.
    for (std::size_t index=0;index<logical.size();++index) { const auto color=client.assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(static_cast<std::uint32_t>(color.green)<<8)|(static_cast<std::uint32_t>(color.blue)<<16); }
    client.menus->Draw(pixels,pitch,height,client.Palette(),BuildGumpShadeMaps(logical));CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0x5a; }));
    // 원본 클립 밖의 모든 padding 바이트를 보존한다.
    for (int y=0;y<height;++y) for (int x=width;x<pitch;++x) CHECK(pixels[static_cast<std::size_t>(y)*pitch+x]==0x5a);
    if (child) client.Click(*child);CHECK(client.commands.size()==1);if (!client.commands.empty()) CHECK(client.commands[0][0]==0x02000060 && client.commands[0][2]==0x007b0032);
    client.menus->Close();CHECK(!client.menus->HasMenus());CHECK(client.menus->Report().starts_with("raw_menus\t0\t0\t0\t0\t"));
}
// 실제 공통 입력을 통해 객체 메뉴 문자열 토큰과 현재 설정 쓰기가 같은 소비자에 도달한다.
TEST_CASE(RawMenuControls_ObjectMenuCommandsAndCurrentConfig) {
    MenuClient client;SidPool pool(OriginalEdition::Patch1078,24000,true);
    // 원문 객체50을 실제 클라이언트 영역에 확보한다.
    for (std::uint32_t number=5;number<=50;++number) CHECK(pool.Allocate(2).value==number);
    auto raw=pool.AllocatedBytes(Sid{50});raw[10]=150;raw[34]=1;
    ConfigInterface config([](std::string_view)->std::optional<std::vector<std::uint8_t>> { return std::nullopt; });
    config.Configuration().Assign("objectMenuInput=1"); //실제 실행처럼 루트 설정 입력을 두어 원문 Get의 this 버퍼 관문을 유지한다.
    ObjectMenuState state;state.denySalvage=1;ObjectMenuHooks hooks;
    hooks.salvageRefund=[](Sid sid,std::uint32_t type,std::uint32_t count) { CHECK(sid.value==50 && type==150 && count==3);return 45; };
    hooks.format=[](std::string_view key,std::span<const PriestMenuArgument> arguments) { std::string text(key);CHECK(arguments.size()==1);text.replace(text.find("%d"),2,std::to_string(std::get<std::int32_t>(arguments[0])));return text; };
    constexpr std::uint32_t denyToken=0xee000001; // 검사 명령 소비자가 함께 쓰는 정적 상수 토큰이다.
    const auto encode=[](std::string_view text) { if (text=="DenySalvage") return denyToken;throw std::logic_error("검사 미등록 상수 문자열"); };
    const auto open=[&] {
        client.poll={InputCode::kRelease,0,0};client.buttons.Poll(client.poll);
        client.menus->Open({120,110},[&](const auto& widgets,const auto& controls,const auto& menus) {
            widgets.BeginVertical();BindObjectMenuControls(hooks,controls,menus,config,encode);
            RawObjectMenu menu(pool,client.assets.TypeTable().Types(),state,hooks);menu.AddSalvage(Sid{50},150,3);
        });
        const auto regions=client.menus->Controls();CHECK(regions.size()==1);if (regions.size()==1) client.Click({(regions[0].rect.left+regions[0].rect.right)/2,(regions[0].rect.top+regions[0].rect.bottom)/2});
    };
    open();CHECK(client.commands.size()==1);if (client.commands.size()==1) CHECK(client.commands[0][0]==state.tellCommand && client.commands[0][2]==denyToken);
    CHECK(!config.Configuration().GetRaw("salvageSid"));client.menus->Close();state.denySalvage=0;open();
    // 실제 Config.Set의 global. 경로는 장기 global 객체에 쓰고 루트에 같은 키를 만들지 않는다.
    CHECK(config.GetInt("global.salvageSid")==50 && !config.Configuration().GetRaw("salvageSid"));
    const auto* global=config.Registry().FindByName("global"); //원본 CRT에서 생성된 실제 저장 객체도 함께 확인한다.
    CHECK(global && global->GetRaw("salvageSid")=="50");CHECK(client.commands.size()==2);if (client.commands.size()==2) CHECK(client.commands[1][0]==state.salvageCommand && client.commands[1][2]==50);
    client.menus->Close();CHECK(!client.menus->HasMenus());
}
// 사제 기본 호출→Carrier→회수/객체 메뉴를 같은 실제 트리에서 조립하고 Cast 입력을 전달한다.
TEST_CASE(RawMenuControls_CarrierAndObjectMenuChain) {
    MenuClient client;SidPool pool(OriginalEdition::Patch1078,24000,true);
    // 실제 부모50과 내용물60을 같은 클라이언트 영역에 확보한다.
    for (std::uint32_t number=5;number<=60;++number) CHECK(pool.Allocate(2).value==number);
    auto parent=pool.AllocatedBytes(Sid{50});parent[10]=158;parent[34]=1;Put(parent,6,60,2);
    auto content=pool.AllocatedBytes(Sid{60});content[10]=6;Put(content,18,123,2);Put(content,20,1,2);
    ContainedFinderState finder;ObjectMenuState common;common.helpWord="ObjectHelp";std::int32_t limit=256;float cdSp=0;
    CarrierMenuState state{common.localOwner,common.editor,limit,cdSp,common.helpCommand};CarrierMenuHooks hooks;ObjectMenuHooks objects;
    ConfigInterface config([](std::string_view)->std::optional<std::vector<std::uint8_t>> { return std::nullopt; });
    hooks.groundImmobile=[](Sid) { return false; };hooks.movementType=[](std::uint32_t) { return 54U; };hooks.findPath=[](Sid,std::uint32_t) { return Sid{}; };
    hooks.dropPoint=[](Sid,std::uint32_t) { return std::array<float,2>{40,50}; };hooks.patchCost=[](std::uint32_t) { return 400; };hooks.money=[](std::uint32_t) { return 1000.0F; };hooks.hasEvent=[](Sid,std::uint32_t) { return false; };hooks.description=[](std::uint32_t) { return "Temple"; };
    hooks.format=[](std::string_view key,std::span<const PriestMenuArgument> arguments) { std::string result;std::size_t argument=0;
        // 고정 원본 서식에 대한 검사 번역 경계이며 제품 서식 계산을 대체하지 않는다.
        for (std::size_t index=0;index<key.size();++index) { if (key[index]!='%') { result+=key[index];continue; }++index;if (key[index]=='s') result+=std::get<std::string>(arguments[argument++]);else result+=std::to_string(std::get<std::int32_t>(arguments[argument++])); }CHECK(argument==arguments.size());return result;
    };
    objects.format=hooks.format;objects.count=[](Sid) { return 1U; };objects.salvageRefund=[](Sid,std::uint32_t,std::uint32_t) { return 0; };
    std::array<std::uint8_t,9*180> players{};std::array<std::uint8_t,100*48> chat{};PlayerIdentityReads reads;
    reads.localOwner=[&] { return common.localOwner; };reads.subscriber=[] { return 0xfedcba98U; };reads.address=[] { return 0x12345678U; };reads.name=[] { return std::string_view("Player1"); };
    reads.battle=[] { return true; };reads.editor=[&] { return common.editor; };reads.metamap=[] { return false; };reads.unknownName=[] { return std::string_view("SOMEONE"); };
    RawPlayerIdentity identity(OriginalEdition::Patch1078,players,{},chat,reads);BindObjectMenuIdentity(objects,identity);
    Put(players,180+44,0xfedcba98);std::copy_n("Player1",8,players.begin()+180+12);players[180+80]=0x23;
    std::array<std::uint32_t,81> alliances{};std::array<std::uint8_t,9> hidden{};std::array<std::uint32_t,9> priestStates{};
    PlayerMenuState playerState;PlayerMenuHooks playerHooks;playerHooks.mission=[] { return true; };playerHooks.fort=[] { return false; };playerHooks.ignored=[](std::uint32_t) { return false; };playerHooks.whispered=[](std::uint32_t) { return std::uint8_t{}; };
    playerHooks.translate=[](std::string_view key) { return std::string(key); };playerHooks.print=hooks.format;
    const auto open=[&] { client.poll={InputCode::kRelease,0,0};client.buttons.Poll(client.poll);client.menus->Open({160,120},[&](const auto& widgets,const auto& controls,const auto& menus) {
        widgets.BeginVertical();BindObjectMenuControls(objects,controls,menus,config,[](std::string_view)->std::uint32_t { throw std::logic_error("검사 미등록 정적 문자열"); });
        // 같은 실제 Player/사제/동맹 상태와 실제 Player 메뉴 몸체를 사용한다. 목록 조회는 명시 검사 입력이다.
        BindPlayerMenuControls(playerHooks,widgets,controls,menus);RawPlayerMenu playerMenu(identity,alliances,hidden,priestStates,playerState,playerHooks);BindObjectPlayerMenu(objects,playerMenu);
        RawObjectMenu base(pool,client.assets.TypeTable().Types(),common,objects);BindCarrierMenuControls(hooks,controls);BindCarrierMenuBase(hooks,base);
        RawCarrierMenu carrier(pool,client.assets.TypeTable().Types(),finder,state,hooks);PriestMenuHooks priest;BindPriestMenuCarrier(priest,carrier);priest.baseMenu(Sid{50});
    }); };open();
    const auto regions=client.menus->Controls();CHECK(regions.size()>=4);if (regions.empty()) return;
    client.Click({(regions[0].rect.left+regions[0].rect.right)/2,(regions[0].rect.top+regions[0].rect.bottom)/2});
    CHECK(client.commands.size()==1);if (client.commands.size()==1) CHECK(client.commands[0][0]==state.castCommand && client.commands[0][2]==50);
    // 실제 명령 선택은 메뉴를 닫으므로 About 선택 전 새 수명으로 다시 연다.
    client.menus->Close();open();const auto about=client.menus->Point("About Temple");CHECK(about.has_value());if (about) client.Click(*about);
    CHECK(client.commands.size()==2);if (client.commands.size()==2) CHECK(client.commands[1][0]==common.helpCommand && client.commands[1][2]==123);
    client.menus->Close();open();const auto player=client.menus->Point("Player");CHECK(player.has_value());if (player) {
        client.Click(*player);
        // 실제 확장 프레임으로 Player 하위 메뉴를 표시한다.
        for (int frame=0;frame<2;++frame) client.buttons.Poll({InputCode::kRelease,player->x,player->y});
        const auto find=client.menus->Point("Find Priest!");CHECK(find.has_value());if (find) client.Click(*find);
    }
    CHECK(client.commands.size()==3);if (client.commands.size()==3) CHECK(client.commands[2][0]==playerState.findPriest && client.commands[2][2]==1);
    client.menus->Close();CHECK(!client.menus->HasMenus());
}

// 실제 시작 안내의 serial 재사용/돌 버튼 즉시 명령과 부모 자식 정리를 함께 검증한다.
TEST_CASE(RawMenuControls_MissionLoadingUsesSharedTreeAndCancel) {
    MenuClient client;client.Resize(1024,768);
    const auto serial=client.menus->ShowMissionLoading(client.LoadingContext({85,0,1024,768}),"Starting Mission...",[] { return std::string("Cancel"); });
    const auto bounds=client.menus->MissionLoadingBounds();CHECK(bounds.has_value());
    if (!bounds) return;
    CHECK(bounds->right-bounds->left==400);CHECK(bounds->left==354);CHECK(bounds->top>=344 && bounds->top<=345);
    CHECK(client.buttons.SerialGump(serial)!=0);const auto point=client.menus->Point("Cancel");CHECK(point.has_value());
    // 본문 상대 깊이와 초기 부착을 검사하여 잘못된 정렬·void·중복 부착을 함께 잡는다.
    const auto parent=client.buttons.SerialGump(serial);std::size_t textChildren{};
    for (auto child=client.menus->Node(parent)->child;child;child=client.menus->Node(child)->next) {
        const auto* node=client.menus->Node(child);CHECK(node->parent==parent && !(node->flags&8));
        if (node->vtable==0x51454cU && !(node->flags&0x10000U)) { ++textChildren;CHECK(node->secondaryDepth==0xffffffffU && node->alignFlags==0); }
    }
    CHECK(textChildren==1);
    CHECK(client.menus->ShowMissionLoading(client.LoadingContext({85,0,1024,768}),"Starting Mission...",[] { return std::string("Cancel"); })==serial);
    // 실제 원본 자산을 모든 자식과 함께 출력하여 Draw가 메타/사본에 의존하지 않는지 확인한다.
    std::vector<std::uint8_t> pixels(1024*768);std::array<std::uint32_t,256> logical{};
    // 실제 팔레트의 RGB를 원본 명암 표 입력으로 옮긴다.
    for (std::size_t index=0;index<logical.size();++index) { const auto color=client.assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); }
    const auto maps=BuildGumpShadeMaps(logical);
    client.menus->Draw(pixels,1024,768,client.Palette(),maps);
    if (point) client.Click(*point);
    CHECK(client.commands.size()==1);
    if (!client.commands.empty()) CHECK((client.commands[0]==std::array<std::uint32_t,3>{0x020000c5U,3,3}));
    CHECK(!client.menus->MissionLoadingBounds());CHECK(client.buttons.SerialGump(serial)==0);
    client.buttons.CleanupGumps();
    const auto next=client.menus->ShowMissionLoading(client.LoadingContext({85,0,1024,768}),"Starting Mission...",[] { return std::string("Cancel"); });CHECK(next!=serial);
    client.menus->CloseMissionLoading();client.buttons.CleanupGumps();CHECK(!client.menus->Point("Cancel"));
    CHECK(client.menus->Report().find("raw_mission_loading\t")==std::string::npos);
}

// Session 없이 생성한 실제 미니맵이 같은 물리의 월드 교체와 Pop 통지를 안전하게 따라간다.
TEST_CASE(RawMenuControls_MinimapBeforeSessionAndAcrossWorldBindings) {
    MenuClient client;auto storage=MakeRawSurfaceStorage(client.assets);RawMinimapHooks hooks; // 실제 물리는 Session보다 먼저 존재한다.
    hooks.view=[] { return MinimapGeometryView{{},{85,0,640,400},16,11}; };hooks.center=[](float,float) {};hooks.move=[](float,float) {};hooks.inputMode=[] { return 2U; };hooks.poll=[&] { return client.poll; };
    auto& mini=client.buttons.EnableMinimap(storage,std::move(hooks));mini.SetPalette(client.Palette());mini.Resize(480);mini.Refresh();CHECK(!storage->Borrowed() && mini.Point("minimap:40,35"));
    const auto bitmap=[&] { const auto text=mini.Report();return text.substr(text.rfind('\t')+1); };const auto empty=bitmap(); // 같은 실제128×128 bitmap 전체의 읽기 전용 해시다.
    const auto links=[] { BridgeConnectState value;value.bridgeType=82;value.islandType=94;value.stalagType=95;value.connectorType=155;value.noIslandType=157;value.battle=true;return value; }; // 현재 실제 표면 종류다.
    const auto make=[&](const std::shared_ptr<RawSurfaceStorage>& data) { return std::make_unique<RawSurfaceWorld>(OriginalEdition::Patch1078,data->types,data->frames,data->shapes,links(),97,nullptr,data); }; // 기존 전체 월드 모듈을 같은 물리에 설치한다.
    auto world=make(storage);mini.BindWorld(world->Core());const auto sid=world->Create(157);world->Core().owner.Set(sid,1);world->Pop(sid,128,128,0);mini.Refresh();CHECK(bitmap()!=empty); // 원문 일반 모드가 실제로 그리는 noIsland 지면157이다.
    bool duplicate{};try { mini.BindWorld(world->Core()); } catch (const std::invalid_argument&) { duplicate=true; }CHECK(duplicate); // 이미 붙은 현재 통지를 덮어쓰지 않는다.
    mini.UnbindWorld();world.reset();CHECK(!storage->Borrowed());mini.Refresh(); // 더 이상 존재하지 않는 Pop/Unpop을 다시 읽지 않는다.
    auto other=MakeRawSurfaceStorage(client.assets);auto foreign=make(other);bool rejected{};try { mini.BindWorld(foreign->Core()); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected); // 같은 자산이라도 다른 물리면 거부한다.
    world=make(storage);mini.BindWorld(world->Core());mini.Refresh();CHECK(storage->Generation()==2 && bitmap()==empty);
    std::array<std::uint32_t,256> logical{}; // 실제 공통 픽셀 출력에 파일 RGB를 제공한다.
    for (std::size_t i=0;i<logical.size();++i) { const auto color=client.assets.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 기존 명암표와 동일한 RGB 포맷이다.
    std::vector<std::uint8_t> pixels(640*480,0x5a);client.menus->Draw(pixels,640,480,client.Palette(),BuildGumpShadeMaps(logical));CHECK(std::any_of(pixels.begin()+405*640,pixels.end(),[](auto byte) { return byte!=0x5a; }));
    client.buttons.DisableMinimap();CHECK(!client.buttons.Minimap());world.reset();CHECK(!storage->Borrowed());
}

// 실제 Button 공장에 빌린 Client 갱신값을 팔레트 변경이 증가시키고 같은 미니맵 Refresh가 소비한다.
TEST_CASE(RawMenuControls_MinimapPaletteUsesSharedClientRevision) {
    ClientGameplayGlobals gameplay;gameplay.MapRevision()=23;MenuClient client(OriginalEdition::Patch1078,&gameplay.MapRevision());CHECK(gameplay.MapRevision()==23); // 공통 Button 생성자가 준비된 Client 값을 초기화하지 않는다.
    auto storage=MakeRawSurfaceStorage(client.assets);RawMinimapHooks hooks;hooks.view=[] { return MinimapGeometryView{{},{85,0,640,400},16,11}; };hooks.center=[](float,float) {};hooks.move=[](float,float) {};hooks.inputMode=[] { return 0U; };hooks.poll=[&] { return client.poll; };
    auto& mini=client.buttons.EnableMinimap(storage,std::move(hooks));auto palette=client.Palette();mini.SetPalette(palette);mini.Resize(480);CHECK(mini.Refresh() && gameplay.MapRevision()==0 && !storage->Borrowed());
    const auto bitmap=[&] { const auto report=mini.Report();return report.substr(report.rfind('\t')+1); };const auto first=bitmap(); // 실제128×128 bitmap 전체의 읽기 전용 해시다.
    palette.basic[0]=(palette.basic[0]+1U)&255U;gameplay.MapRevision()=7;mini.SetPalette(palette);CHECK(gameplay.MapRevision()==8);mini.SetPalette(palette);CHECK(gameplay.MapRevision()==8); // 실제 미니맵 black 색이 변한 한 번만 동일 DWORD를 증가시킨다.
    CHECK(mini.Refresh() && gameplay.MapRevision()==0 && bitmap()!=first);const auto changed=bitmap();CHECK(!mini.Refresh() && gameplay.MapRevision()==0 && bitmap()==changed); // 같은 view/색/빈 큐의 다음 호출은 실제 셀 그림을 반복하지 않는다.
    gameplay.MapRevision()=11;mini.SetPalette(palette);CHECK(gameplay.MapRevision()==11);CHECK(mini.Refresh() && gameplay.MapRevision()==0 && bitmap()==changed); // 팔레트와 별개로 원래 owner의 현재값을 직접 바꾸어도 같은 Refresh가 읽는다.
    client.buttons.DisableMinimap();CHECK(!client.buttons.Minimap() && !storage->Borrowed());
}

// 이전표시의소멸이새표시의실제Pop/Unpop연결을지우면두번째표면의셀그림변화가실패해야한다.
TEST_CASE(RawMenuControls_MinimapReplacementKeepsCurrentWorldNotifications) {
    MenuClient client;auto storage=MakeRawSurfaceStorage(client.assets);RawMinimapHooks hooks;hooks.view=[] { return MinimapGeometryView{{},{85,0,640,400},16,11}; };hooks.center=[](float,float) {};hooks.move=[](float,float) {};hooks.inputMode=[] { return 0U; };hooks.poll=[&] { return client.poll; };
    auto& old=client.buttons.EnableMinimap(storage,hooks);old.SetPalette(client.Palette());old.Resize(480);old.Refresh();const auto oldToken=old.RootToken();
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;links.battle=true;auto world=std::make_unique<RawSurfaceWorld>(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,links,97,nullptr,storage);old.BindWorld(world->Core());
    auto& current=client.buttons.ConstructMinimap(storage,hooks);current.BindWorld(world->Core());current.Attach(current.RootToken(),0,0,client.buttons.RootToken(),0);current.Resize(480);current.Refresh();const auto bitmap=[&] { const auto text=current.Report();return text.substr(text.rfind('\t')+1); };const auto empty=bitmap(); // 미부착공장뒤의기존도킹Resize는원문처럼부착후에호출한다.
    client.buttons.KillGump(oldToken);client.buttons.CleanupGumps();CHECK(client.buttons.Minimap()==&current);const auto sid=world->Create(157);world->Core().owner.Set(sid,1);world->Pop(sid,128,128,0);current.Refresh();CHECK(bitmap()!=empty);
    CHECK(world->Destroy(sid));current.Refresh();CHECK(bitmap()==empty);client.buttons.DisableMinimap();world.reset();CHECK(!storage->Borrowed());
}
