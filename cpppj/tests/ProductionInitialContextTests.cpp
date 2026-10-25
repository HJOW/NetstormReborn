// 실제 Client 전역과 공통 UI로 Session 이전의 생산 전체 생성/자식/문자열/정리를 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/RawButtonControls.h"
#include "client/RawProductionControls.h"
#include "client/InventoryGump.h"
#include "client/HudDock.h"
#include "client/PaletteColors.h"
#include "client/PaletteShade.h"
#include "client/RawMinimapControls.h"
#include "client/RawUberRootControls.h"
#include "client/RawNsInfoContext.h"
#include "client/RawChatWindowContext.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include <sstream>

using namespace netstorm::client;
using namespace netstorm::o;
namespace {
// 창/장치를 열지 않고 실제 보호 파일을 사용하는 Client를 만든다.
ClientOptions ProductionInitialOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; }
// 실제 자산/Client/UI를 사용하며 Player나 Session의 대체 객체를 만들지 않는다.
struct InitialProductionScene {
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 보호 자산을 읽기만 한다.
    Client client{ProductionInitialOptions()};FontStore fonts{assets.files,"Arial"}; // 모든 물리/글꼴 소비자보다 오래 유지한다.
    std::shared_ptr<RawSurfaceStorage> storage=MakeRawSurfaceStorage(assets.game);std::array<std::uint8_t,87> options{}; // 실제 초기 물리와 명시 옵션 BYTE 입력이다.
    double wall{123.25},game{};std::uint32_t configReads{},timeReads{};std::vector<std::string> translations; // 실제 선택된 외부 읽기만 관측한다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[this] { return wall; }}};RawMenuControls* menus{}; // 같은 공통 등록/트리/수명이다.
    // 초기 화면과 실제 메뉴/폰트를 연결한다. Session/Player 초기화는 수행하지 않는다.
    InitialProductionScene() {
        RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};hooks.gameSeconds=[this] { return game; };
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));const auto& type=assets.game.Find("fortGump");const auto metrics=assets.game.Shapes().SquidMetrics(type.block,0); // 같은 실제 배경 자산이다.
        const std::array<GumpControl,0> controls{};const std::array<std::string,0> labels{};buttons.SetControls(controls,labels,640,480,{static_cast<std::uint32_t>(type.block+70),0,metrics.width,metrics.height,{}});
    }
    // 실제 Client 소유자의 같은 현재 전역과 아직 미할당인 Player owner를 제공한다.
    ProductionControlContext Context() { return {storage,client.GameplayGlobals(),client.PlayerRecords(),options}; }
    // 읽기 전용 관측/영어 번역을 제외한 게임 효과는 실제 도달 시 실패해야 한다.
    RawProductionHooks Hooks(bool translated=true) {
        RawProductionHooks hooks;hooks.now=[this] { ++timeReads;return wall; };hooks.sound=[](auto) { throw std::logic_error("초기 생산 생성에 음향 도달"); };hooks.tell=[](auto) { throw std::logic_error("초기 생산 생성에 Tell 도달"); };
        hooks.configInt=[this](auto,std::int32_t fallback) { ++configReads;return fallback; };if (translated) hooks.translate=[this](std::string_view text) { translations.emplace_back(text);return std::string(text); };hooks.gameSeconds=[this] { return game; };return hooks;
    }
    // 현재 공통 등록표에서 파생 가상표가 같은 실제 생산 노드를 찾는다.
    std::uint32_t Token(std::uint32_t table) {
        for (std::uint32_t id=1;id<1000;++id) { const auto token=buttons.RegisteredGump(id);if (auto* node=buttons.Production()->Node(token);node && node->vtable==table) return token; } // 별도 노드/그림 목록을 만들지 않는다.
        return 0;
    }
    // 공통 등록 개수와 생산 자체 수명을 구별한다.
    std::uint32_t Count() { std::istringstream input(buttons.Report());std::string name;std::uint32_t controls{},count{};input>>name>>controls>>count;CHECK(name=="RawButtons");return count; }
};
}

// 실제 원문 생성만 호출했을 때8노드/40폭/원래높이/구분선/도움말과 지연 Runtime이 맞는다.
TEST_CASE(ProductionInitialContext_ConstructsAllNativeChildrenWithoutSession) {
    InitialProductionScene scene;scene.options[1]=3;const auto before=scene.Count();auto& panel=scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());
    CHECK(scene.Count()==before+8 && scene.client.PlayerRecords()->Base()==0 && !scene.client.GameplayGlobals()->Borrowed());CHECK(scene.timeReads==1 && scene.configReads==0);
    CHECK((scene.translations==std::vector<std::string>{"Storm Power Available","Production Window","Production Window","Production Window"}));
    const auto root=scene.Token(0x5069c0),budget=scene.Token(0x506948),divider=scene.Token(0x514fd8);CHECK(root && budget && divider);auto* node=panel.Node(root);CHECK(node && !node->parent && node->baseWidth==40 && node->baseHeight==480 && node->width==40 && node->height==480);
    const auto* money=panel.Node(budget);CHECK(money && money->textPointer && money->alternateTextPointer && (money->alignFlags&4));const auto* line=static_cast<HudDividerNode*>(panel.Background(divider));CHECK(line && line->axis==0 && line->baseWidth==40 && line->baseHeight==5 && line->y==21);
    std::uint32_t grids{}; // 두 실제 격자의 Native 최소값/현재 크기를 검사한다.
    for (std::uint32_t id=1;id<1000;++id) if (auto* item=panel.Node(scene.buttons.RegisteredGump(id));item && item->vtable==0x506800) {
        const auto* grid=static_cast<InventoryGridNode*>(item);++grids;CHECK(grid->object==0);
        if (grid->contentMask==0x40) CHECK(grid->minimumColumns==2 && grid->minimumRows==4 && grid->fixed==1 && grid->baseHeight==144 && grid->y==26 && grid->textPointer && grid->alternateTextPointer);
        else CHECK(grid->contentMask==8 && grid->minimumColumns==1 && grid->minimumRows==6 && grid->baseHeight==250 && grid->y==170);
    }
    CHECK(grids==2 && scene.menus->Report().find("raw_gump_help_strings\t8\n")!=std::string::npos);scene.buttons.IdleGumps();CHECK(panel.Report().find("raw_production_gump\t")!=std::string::npos);
    std::array<ScreenColor,256> palette{};std::array<std::uint32_t,256> logical{}; // 실제 보호 팔레트의 같은 RGB와 명암표 입력이다.
    for (std::size_t index=0;index<palette.size();++index) { const auto color=scene.assets.game.Palette().Color(static_cast<std::uint8_t>(index));palette[index]={color.red,color.green,color.blue,0};logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 파일 색을 조회할 뿐 게임 상태를 주입하지 않는다.
    panel.SetPalette(palette);panel.Attach(root,0,0,scene.buttons.RootToken(),0);constexpr int pitch=647,height=480;std::vector<std::uint8_t> pixels(pitch*height,0x5a); // 명시 부착 뒤 실제 전체 출력과 행 padding을 구별한다.
    scene.menus->Draw(pixels,pitch,height,BuildPaletteColorTable(logical),BuildGumpShadeMaps(logical));CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0x5a; }));
    for (int y=0;y<height;++y) for (int x=640;x<pitch;++x) CHECK(pixels[static_cast<std::size_t>(y)*pitch+x]==0x5a); // 생산 자식은 실제 화면 클립 밖 padding을 쓰지 않는다.
    panel.Close();CHECK(scene.Count()==before && scene.menus->Report().find("raw_gump_help_strings\t0\n")!=std::string::npos);scene.buttons.DisableProduction();CHECK(!scene.buttons.Production());
}

// 미부착 부모/자식도 같은 실제 공통 수명에서 반납하며 새 생성에 죽은 토큰을 반환하지 않는다.
TEST_CASE(ProductionInitialContext_RepeatedConstructionAndGlobalPageRetirement) {
    InitialProductionScene scene;const auto before=scene.Count();std::vector<std::uint32_t> old;
    for (int generation=0;generation<3;++generation) { // 한 Client/물리에서 세 실제 생성자를 순서대로 실행한다.
        auto& panel=scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());const auto token=scene.Token(0x5069c0);old.push_back(token);
        for (const auto retired:old) if (retired!=token) CHECK(!panel.Node(retired)); // 실제 반납한 이전 주소를 되살리지 않는다.
        scene.buttons.DisableProduction();CHECK(scene.Count()==before && scene.menus->Report().find("raw_gump_help_strings\t0\n")!=std::string::npos);
    }
    scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());const auto& type=scene.assets.game.Find("fortGump");const auto metrics=scene.assets.game.Shapes().SquidMetrics(type.block,0); // 기존 페이지 전체 교체도 미부착 생산을 먼저 닫아야 한다.
    scene.buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(type.block+70),0,metrics.width,metrics.height,{}});CHECK(!scene.buttons.Production() && scene.Count()==before);
}

// 선택 번역 누락 뒤 부분 등록을 그대로 관측하고 실제 Close로 원래 개수를 복구한다.
TEST_CASE(ProductionInitialContext_MissingTranslationKeepsAndRetiresPartialEffects) {
    InitialProductionScene scene;const auto before=scene.Count();bool rejected{};try { scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks(false)); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && !scene.buttons.Production() && scene.Count()==before+2);scene.buttons.DisableProduction();CHECK(scene.Count()==before && scene.menus->Report().find("raw_gump_help_strings\t0\n")!=std::string::npos); // 실패한생성자는현재serial을쓰지않지만부분등록2개는실제Close까지남는다.
}

// 실제 같은 Subscriber 참조와 단일 대여를 유지하며 소멸한 Player/커서 상태만 무효화한다.
TEST_CASE(ProductionInitialContext_GameplayGlobalsKeepRealAliasesAndRejectDuplicate) {
    InitialProductionScene scene;auto globals=scene.client.GameplayGlobals();auto subscribers=scene.client.SubscriberGlobals();CHECK(&globals->world.mySubscriber==&subscribers->MySubscriber() && &globals->world.loginSubscriber==&subscribers->LoginSubscriber());
    subscribers->MySubscriber()=71;subscribers->LoginSubscriber()=93;auto use=globals->Acquire();globals->player.playerKnown=1;globals->cursor.inputFlags=0x41;bool rejected{};
    try { globals->Acquire(); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && globals->Borrowed() && globals->player.playerKnown==1 && globals->cursor.inputFlags==0x41);
    use.reset();CHECK(!globals->Borrowed() && globals->player.playerKnown==0 && globals->cursor.inputFlags==0 && globals->cursor.lockDepth==1 && globals->world.mySubscriber==71 && globals->world.loginSubscriber==93);
    bool missing{};try { globals->RequireSession(); } catch (const std::logic_error&) { missing=true; }CHECK(missing); // 게임 효과를 가짜 Session 성공으로 바꾸지 않는다.
}

namespace {
// 실제 파일의 RGB만 공통 Draw와 미니맵 색 해석에 공급한다.
std::array<std::uint32_t,256> InitialHudLogical(InitialProductionScene& scene) {
    std::array<std::uint32_t,256> result{};
    for (std::size_t index=0;index<result.size();++index) { const auto color=scene.assets.game.Palette().Color(static_cast<std::uint8_t>(index));result[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 팔레트의 원래 번호 순서를 유지한다.
    return result;
}
// 실제 메뉴 수집/정렬/클립이 새/이전 HUD를 모두 그리는지 같은 화면에서 검사한다.
std::vector<std::uint8_t> InitialHudPixels(InitialProductionScene& scene) {
    const auto logical=InitialHudLogical(scene);std::vector<std::uint8_t> result(640*480,0x5a);scene.menus->Draw(result,640,480,BuildPaletteColorTable(logical),BuildGumpShadeMaps(logical));return result;
}
// 두 서로 떨어진 실제 패널 영역에서 배경/글자 픽셀을 비교한다.
std::vector<std::uint8_t> InitialHudRegion(const std::vector<std::uint8_t>& image,int left,int top,int width,int height) {
    std::vector<std::uint8_t> result;
    for (int y=top;y<top+height;++y) result.insert(result.end(),image.begin()+y*640+left,image.begin()+y*640+left+width); // 행 padding이나 다른 HUD를 포함하지 않는다.
    return result;
}
// 같은 초기 물리에서 현재 카메라 장치만 제공하며 Session/게임 계산을 대체하지 않는다.
RawMinimapHooks InitialMinimapHooks() {
    RawMinimapHooks hooks;hooks.view=[] { return MinimapGeometryView{{},{85,0,640,400},16,11}; };hooks.center=[](float,float) {};hooks.move=[](float,float) {};hooks.inputMode=[] { return 0U; };hooks.poll=[] { return InputEvent{InputCode::kRelease,0,0}; };return hooks;
}
}

// 이전 생산을 지우기 전에 새 전체 생산을 만들고 실제 두 출력이 동시에 살아 있는지 확인한다.
TEST_CASE(ProductionInitialContext_OverlappingParentsRetireOnlyOldDraw) {
    InitialProductionScene scene;const auto before=scene.Count();scene.menus->SetExternalGumps({{},[](auto) { return false; },[](auto,auto&,auto&,auto&) { throw std::logic_error("비소유 고정 슬롯 출력에 도달"); }});
    auto& old=scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());const auto oldToken=old.RootToken();old.Attach(oldToken,0,0,scene.buttons.RootToken(),0);
    auto& current=scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());const auto currentToken=current.RootToken();current.Attach(currentToken,200,0,scene.buttons.RootToken(),0);
    CHECK(scene.buttons.Production()==&current && scene.Count()==before+16 && old.Node(oldToken) && current.Node(currentToken));
    const auto both=InitialHudPixels(scene);const auto oldPixels=InitialHudRegion(both,0,0,40,480),currentPixels=InitialHudRegion(both,200,0,40,480);
    CHECK(std::any_of(oldPixels.begin(),oldPixels.end(),[](auto value) { return value!=0x5a; }) && std::any_of(currentPixels.begin(),currentPixels.end(),[](auto value) { return value!=0x5a; }));
    scene.buttons.KillGump(oldToken);CHECK(scene.Count()==before+16 && scene.buttons.Production()==&current);scene.buttons.CleanupGumps();CHECK(scene.Count()==before+8 && scene.buttons.Production()==&current);
    const auto after=InitialHudPixels(scene),cleared=InitialHudRegion(after,0,0,40,480);CHECK(std::all_of(cleared.begin(),cleared.end(),[](auto value) { return value==0x5a; }) && InitialHudRegion(after,200,0,40,480)==currentPixels);
    CHECK(scene.menus->Report().find("raw_external_gumps\t1\t1\n")!=std::string::npos);scene.buttons.DisableProduction();CHECK(scene.Count()==before && scene.menus->Report().find("raw_external_gumps\t1\t0\n")!=std::string::npos);scene.menus->SetExternalGumps({});
}

// 최신serial의 대상이 죽었을 때 아직 살아 있는 이전 생산을 현재로 되살리지 않는다.
TEST_CASE(ProductionInitialContext_LatestKilledDoesNotSelectOlderParent) {
    InitialProductionScene scene;const auto before=scene.Count();auto& old=scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());const auto oldToken=old.RootToken();
    auto& current=scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());scene.buttons.KillGump(current.RootToken());CHECK(!scene.buttons.Production());scene.buttons.CleanupGumps();CHECK(!scene.buttons.Production() && old.Node(oldToken) && scene.Count()==before+8);
    auto& next=scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());CHECK(scene.buttons.Production()==&next && old.Node(oldToken));scene.buttons.DisableProduction();CHECK(scene.Count()==before);
}

// 실패한 새 생성의 부분 노드와 이전 현재serial은 독립적으로 보존된다.
TEST_CASE(ProductionInitialContext_PartialNewConstructorKeepsPreviousSerial) {
    InitialProductionScene scene;const auto before=scene.Count();auto& old=scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks());bool rejected{};
    try { scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks(false)); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && scene.buttons.Production()==&old && scene.Count()==before+10);scene.buttons.CleanupGumps();CHECK(scene.buttons.Production()==&old && scene.Count()==before+10);scene.buttons.DisableProduction();CHECK(scene.Count()==before);
}

// 원문 생성자는 최초bitmap폭0과부모0을보존하며bitmap/도킹작업을선행하지않는다.
TEST_CASE(ProductionInitialContext_MinimapFactoryUsesCurrentWidthWithoutRefresh) {
    InitialProductionScene scene;const auto before=scene.Count();auto& mini=scene.buttons.ConstructMinimap(scene.storage,InitialMinimapHooks());const auto* node=mini.Node(mini.RootToken());
    CHECK(scene.buttons.Minimap()==&mini && scene.Count()==before+1 && node && !node->parent && node->baseWidth==0 && node->baseHeight==0 && !(node->flags&0x20000U));
    CHECK(mini.Report().ends_with("\t0\t0\t0\t2166136261") && scene.menus->Report().find("raw_gump_help_strings\t2\n")!=std::string::npos);scene.buttons.DisableMinimap();CHECK(!scene.buttons.Minimap() && scene.Count()==before && scene.menus->Report().find("raw_gump_help_strings\t0\n")!=std::string::npos);
}

// 새현재표시는공유bitmap을사용하고이전표시소멸뒤에도자기출력을유지한다.
TEST_CASE(ProductionInitialContext_OverlappingMinimapsKeepCurrentBitmapAndDraw) {
    InitialProductionScene scene;const auto before=scene.Count();const auto palette=BuildPaletteColorTable(InitialHudLogical(scene));auto& old=scene.buttons.EnableMinimap(scene.storage,InitialMinimapHooks());old.SetPalette(palette);old.Resize(480);old.Refresh();const auto oldToken=old.RootToken();
    auto& current=scene.buttons.ConstructMinimap(scene.storage,InitialMinimapHooks());const auto token=current.RootToken();auto* node=current.Node(token);CHECK(node && !node->parent && node->baseWidth==128 && node->baseHeight==128);node->baseWidth=node->width=80;node->baseHeight=node->height=75;current.Attach(token,160,405,scene.buttons.RootToken(),0);current.Refresh();
    CHECK(scene.buttons.Minimap()==&current && scene.Count()==before+2);const auto report=current.Report();CHECK(!old.Refresh() && current.Report()==report);const auto both=InitialHudPixels(scene),newPixels=InitialHudRegion(both,160,405,80,75),oldPixels=InitialHudRegion(both,0,405,80,75);CHECK(std::any_of(oldPixels.begin(),oldPixels.end(),[](auto value) { return value!=0x5a; }) && std::any_of(newPixels.begin(),newPixels.end(),[](auto value) { return value!=0x5a; }));
    scene.buttons.KillGump(oldToken);scene.buttons.CleanupGumps();CHECK(scene.buttons.Minimap()==&current && scene.Count()==before+1);const auto after=InitialHudPixels(scene),cleared=InitialHudRegion(after,0,405,80,75);CHECK(std::all_of(cleared.begin(),cleared.end(),[](auto value) { return value==0x5a; }) && InitialHudRegion(after,160,405,80,75)==newPixels);
    CHECK(scene.menus->Report().find("raw_external_gumps\t0\t1\n")!=std::string::npos);scene.buttons.DisableMinimap();CHECK(scene.Count()==before && scene.menus->Report().find("raw_external_gumps\t0\t0\n")!=std::string::npos);
}

// 포인터기반현재표시를삭제해도등록표의이전노드로fallback하지않는다.
TEST_CASE(ProductionInitialContext_LatestMinimapKilledDoesNotSelectOlder) {
    InitialProductionScene scene;const auto before=scene.Count();auto& old=scene.buttons.ConstructMinimap(scene.storage,InitialMinimapHooks());const auto oldToken=old.RootToken();auto& current=scene.buttons.ConstructMinimap(scene.storage,InitialMinimapHooks());scene.buttons.KillGump(current.RootToken());scene.buttons.CleanupGumps();
    CHECK(!scene.buttons.Minimap() && old.Node(oldToken) && !old.Refresh() && scene.Count()==before+1);scene.buttons.DisableMinimap();CHECK(scene.Count()==before);
}

namespace { // 실제 자산/장기 Client와 전체 공통 HUD의 결합 입력만 준비한다.
struct InitialUberGlobals {
    std::int32_t width{640},height{480},left{},top{},mapWidth{},mapHeight{}; // 같은 실제 도킹이 쓰는 지도/화면 전역이다.
    std::uint32_t shown{1},refresh{},resizes{};float chatY{}; // 명시 기본 표시와 실제 후처리 관측이다.
    std::array<float,4> floats{};ScreenRect bottom{};std::array<char,80> title{}; // 정수 지도는 실제 GumpTree를 빌리고 메시지/float/제외 영역은 BSS 입력이다.
    std::array<std::uint8_t,256> rankOutput,rankTemporary; // 실제 원문 임시의 미작성 입력을 보존한다.
    PaletteColorTable palette; // 편집 자식도 현재 실제 갈색/흰색 장치를 사용한다.
};
// 네 자식 모두 기존 실제 공장으로 만들며 가짜 기본 노드/빈 성공으로 대체하지 않는다.
RawUberRootContext InitialUberContext(InitialProductionScene& scene,InitialUberGlobals& state) {
    std::array<std::uint32_t,256> logical{}; // 실제 보호 색 표의 입력이며 임의 색 번호를 공급하지 않는다.
    for (std::size_t index=0;index<logical.size();++index) { const auto color=scene.assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 파일의 원래 RGB를 읽는다.
    state.palette=BuildPaletteColorTable(logical);scene.menus->SetPalette(state.palette);
    RawUberRootContext context{{state.width,state.height,state.shown,state.left,state.top,state.mapWidth,state.mapHeight,state.floats,scene.buttons.MapViewRect(),state.bottom,state.refresh,state.chatY},{},{},{},{},{}};
    context.poll=[](auto) { return InputEvent{InputCode::kRelease,300,200}; };context.cursor=[](auto) {};
    context.root.read=[&scene,&state](auto address) -> std::uint32_t {
        if (address==0x5411c8U) return static_cast<std::uint32_t>(scene.assets.game.Find("fortGump").block+netstorm::o::kFirstAssetTypeNumber);
        if (address==0x565e70U) return static_cast<std::uint8_t>(state.title[0]);throw std::out_of_range("초기 전체 HUD의 미선언 전역 조회");
    };
    context.root.type=[&scene](auto type) { CHECK(type<scene.storage->types.size());return 0x100000U+type; };
    context.root.createMinimap=[&scene] { return scene.buttons.ConstructMinimap(scene.storage,InitialMinimapHooks()).RootToken(); };
    context.root.createProduction=[&scene] { return scene.buttons.ConstructProduction(scene.Context(),scene.assets.game,scene.Hooks()).RootToken(); };
    context.root.createKnowledge=[&scene,&state] {
        RawNsInfoContext data;data.globals=scene.client.NsInfoGlobals();data.gameplay=scene.client.GameplayGlobals();data.storage=scene.storage;
        data.read=[&scene,&state](auto address) -> std::uint32_t { if (address==0x5411c8U) return static_cast<std::uint32_t>(scene.assets.game.Find("fortGump").block+70);if (address==0x5bec3cU) return state.palette.basic[7];throw std::out_of_range("초기 NSInfo 미선언 장치"); };
        data.translate=[](auto text) { return std::string(text); };data.storeText=[&scene](auto text) { return scene.client.ChatWindowGlobals()->Store(text); };
        data.text=[&scene](auto token) -> std::string_view { return scene.client.ChatWindowGlobals()->strings.at(token); };return scene.buttons.ConstructNsInfo(std::move(data));
    };
    context.root.createChat=[&scene] {
        RawChatWindowContext data;data.globals=scene.client.ChatWindowGlobals();data.commands=scene.client.DotCommandGlobals();data.editor.now=[&scene] { return scene.wall; };
        data.translate=[](auto text) { return std::string(text); };data.text=[&scene](auto token) -> std::string_view { return scene.client.ChatWindowGlobals()->strings.at(token); };
        data.read=[&scene](auto address) -> std::uint32_t { if (address==0x594fc8U) return scene.storage->ownerMode.challenge;throw std::out_of_range("초기 Chat 생성의 미선언 동적 전역"); };return scene.menus->CreateChatWindow(std::move(data));
    };
    context.root.refreshTechnology=[&scene,&state] { auto hooks=MakeClientChatManagerHooks(scene.client.ChatManagerGlobals());netstorm::o::RefreshChatManagerRankPrefix({scene.client.ChatManagerGlobals()->Current(),scene.client.CurrentSubscriber(),scene.client.CoreDataGlobals()->Active()},hooks,state.rankOutput,state.rankTemporary); };
    context.root.translate=[&scene](auto text) { return scene.client.ChatWindowGlobals()->Store(text); };
    context.root.help=[&scene](auto token,auto text) { scene.menus->SetHelpText(token,scene.client.ChatWindowGlobals()->strings.at(text)); };
    context.root.helpWord=[&scene](auto token,auto text) { scene.menus->SetHelpWord(token,text); };
    context.dock.invalidate=[](auto,auto,auto,auto,auto) {};context.dock.finishResize=[&state] { ++state.resizes; };
    context.dock.chatVisible=[&scene](bool visible) { scene.menus->ChatGeometryBackend()->SetHelperVisible(scene.client.ChatWindowGlobals()->window.current,visible); };
    context.dock.chatInput=[&scene](bool visible) { scene.menus->ChatGeometryBackend()->SetInputVisible(scene.client.ChatWindowGlobals()->window.current,visible); };
    return context;
}
}

// 실제 전체 자식/세 구분선·공통 입력·지도 결과와 동일 실제 Draw를 함께 확인한다.
TEST_CASE(ProductionInitialContext_FullUberUsesActualChildrenAndDock) {
    InitialUberGlobals globals;InitialProductionScene scene;auto& uber=scene.buttons.EnableUberRoot(InitialUberContext(scene,globals));const auto oldRoot=scene.buttons.RootToken();uber.Replace();
    const auto root=scene.buttons.RootToken();CHECK(root!=oldRoot && uber.Current() && !uber.Node(oldRoot));const auto* node=static_cast<UberRootNode*>(uber.Node(root));CHECK(node && node->command==54 && node->vtable==0x51503c);
    CHECK(node->minimap==scene.buttons.Minimap()->RootToken() && node->production==scene.buttons.Production()->RootToken() && node->knowledge==scene.menus->NsInfoRoot() && node->chat==scene.menus->ChatWindowRoot());
    const auto* vertical=static_cast<HudDividerNode*>(uber.Background(node->vertical));CHECK(vertical && vertical->command==55 && vertical->slaves[0].first && vertical->slaves[5].first && !vertical->slaves[6].first);
    scene.buttons.HandleGump(root,0x0200003dU,3,0);const auto& view=scene.buttons.MapViewRect();CHECK(view.left==85 && view.top==0 && view.right==640 && view.bottom==384 && globals.resizes==1 && globals.refresh==1);
    const auto* mini=scene.buttons.Minimap()->Node(node->minimap);const auto* panel=scene.buttons.Production()->Node(node->production);CHECK(mini && panel && mini->parent==root && mini->baseWidth==80 && mini->baseHeight==75 && panel->baseWidth==80 && panel->baseHeight==400);
    scene.buttons.BubbleGump(node->chat,0x02000088U,0,0);CHECK(view.bottom==480); // 실제 미션 Start의 현재 Chat→부모 숨김 명령이 지도 높이를 복원한다.
    const auto pixels=InitialHudPixels(scene);CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto color) { return color!=0x5a; }));CHECK(scene.menus->Report().find("raw_external_gumps\t0\t3\n")!=std::string::npos);
}

// 원문 교체가 실제 새 자식을 먼저 만든 뒤 이전 직계 자식을 이동하고 이전 루트만 반납하는지 확인한다.
TEST_CASE(ProductionInitialContext_FullUberReplacementMovesOldChildren) {
    InitialUberGlobals globals;InitialProductionScene scene;auto& uber=scene.buttons.EnableUberRoot(InitialUberContext(scene,globals));uber.Replace();const auto first=scene.buttons.RootToken();const auto* old=static_cast<UberRootNode*>(uber.Node(first));
    const auto oldProduction=old->production,oldMinimap=old->minimap;auto* previousProduction=scene.buttons.Production();auto* previousMinimap=scene.buttons.Minimap();const auto count=scene.Count();uber.Replace();const auto next=scene.buttons.RootToken();const auto* current=static_cast<UberRootNode*>(uber.Node(next)); // 이전 두 실제 자식은 원문 이동으로 생존해야 한다.
    CHECK(next!=first && uber.Current() && !uber.Node(first) && scene.Count()>count);CHECK(current->production!=oldProduction && current->minimap!=oldMinimap && scene.buttons.Production()->RootToken()==current->production && scene.buttons.Minimap()->RootToken()==current->minimap);
    CHECK(previousProduction->Node(oldProduction) && previousProduction->Node(oldProduction)->parent==next);CHECK(previousMinimap->Node(oldMinimap) && previousMinimap->Node(oldMinimap)->parent==next); // 이전 루트의 두 실제 자식 모두 새 루트의 같은 주소로 이동한다.
    CHECK(scene.buttons.Report().find("raw_hud_owners\t2\t2\t")!=std::string::npos);CHECK(scene.menus->Report().find("raw_external_gumps\t0\t5\n")!=std::string::npos); // 현재 대상을 이전 세대로 되돌려 조회하지 않는다.
}

// 새 화면 장치는 기존 Native 구분선/자식을 유지하며 현재 하단 거리와 지도/그림 크기를 갱신해야 한다.
TEST_CASE(ProductionInitialContext_FullUberReflowsExistingScreen) {
    InitialUberGlobals globals;InitialProductionScene scene;auto& uber=scene.buttons.EnableUberRoot(InitialUberContext(scene,globals));uber.Replace();scene.buttons.HandleGump(scene.buttons.RootToken(),0x0200003dU,3,0);
    const auto root=scene.buttons.RootToken(),count=scene.Count();const auto* node=static_cast<UberRootNode*>(uber.Node(root));const auto mini=node->minimap,production=node->production;
    globals.width=800;globals.height=600;const auto& asset=scene.assets.game.Find("fortGump");const auto metrics=scene.assets.game.Shapes().SquidMetrics(asset.block,0); // 기존 화면 설치와 같은 실제 헤더다.
    scene.buttons.SetControls({}, {},800,600,{static_cast<std::uint32_t>(asset.block+70),0,metrics.width,metrics.height,{}},true);uber.Reflow();
    CHECK(scene.buttons.RootToken()==root && scene.Count()==count && node->minimap==mini && node->production==production);const auto& view=scene.buttons.MapViewRect();CHECK(view.left==85 && view.right==800 && view.bottom==504);
    CHECK(uber.Background(node->vertical)->baseHeight==600 && scene.buttons.Minimap()->Node(mini)->y==525 && scene.buttons.Minimap()->Node(mini)->baseHeight==75 && scene.buttons.Production()->Node(production)->baseHeight==520);
    scene.buttons.BubbleGump(node->chat,0x02000088U,0,0);CHECK(view.bottom==600); // 같은 원문 숨김은 새 전체 화면 높이를 사용한다.
}
