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
    CHECK(rejected && scene.buttons.Production() && scene.Count()==before+2);scene.buttons.DisableProduction();CHECK(scene.Count()==before && scene.menus->Report().find("raw_gump_help_strings\t0\n")!=std::string::npos);
}

// 실제 같은 Subscriber 참조와 단일 대여를 유지하며 소멸한 Player/커서 상태만 무효화한다.
TEST_CASE(ProductionInitialContext_GameplayGlobalsKeepRealAliasesAndRejectDuplicate) {
    InitialProductionScene scene;auto globals=scene.client.GameplayGlobals();auto subscribers=scene.client.SubscriberGlobals();CHECK(&globals->world.mySubscriber==&subscribers->MySubscriber() && &globals->world.loginSubscriber==&subscribers->LoginSubscriber());
    subscribers->MySubscriber()=71;subscribers->LoginSubscriber()=93;auto use=globals->Acquire();globals->player.playerKnown=1;globals->cursor.inputFlags=0x41;bool rejected{};
    try { globals->Acquire(); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && globals->Borrowed() && globals->player.playerKnown==1 && globals->cursor.inputFlags==0x41);
    use.reset();CHECK(!globals->Borrowed() && globals->player.playerKnown==0 && globals->cursor.inputFlags==0 && globals->cursor.lockDepth==1 && globals->world.mySubscriber==71 && globals->world.loginSubscriber==93);
    bool missing{};try { globals->RequireSession(); } catch (const std::logic_error&) { missing=true; }CHECK(missing); // 게임 효과를 가짜 Session 성공으로 바꾸지 않는다.
}
