// 실제 공통 트리의 메뉴 입력/하위 메뉴/제목·픽셀/삭제와 기존 버튼 복귀를 검사한다.
#include "RawSceneSupport.h"
#include "client/RawMenuControls.h"
#include "client/RawButtonControls.h"
#include "client/RawObjectMenu.h"
#include "client/RawCarrierMenu.h"
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
    // 게임/창을 열지 않고 실제 보호 자산과 모든 복원 모듈을 조립한다.
    explicit MenuClient(OriginalEdition edition=OriginalEdition::Patch1078):root(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"),files(root),assets(Register(),edition),fonts(files,"Arial"),
        buttons(edition,{[] { return 0.0; },[this](auto) { return poll; },[] {}}) {
        RawMenuHooks hooks;hooks.command=[this](auto command,auto id,auto argument) { commands.push_back({command,id,argument}); };
        hooks.readInt=[](std::string_view,std::int32_t& value) { value=0; };hooks.sound=[this](std::string_view name) { sounds.emplace_back(name); };
        hooks.poll=[this](auto) { return poll; };hooks.cursor=[this](auto value) { cursor=value; };
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
        const auto& asset=assets.Find("fortGump");const auto frame=assets.FrameIndex(asset,"A00",0);const auto metrics=assets.Shapes().SquidMetrics(asset.block,frame);
        const std::array<GumpControl,1> controls{{{0,{10,10,90,32},false,true,false}}};const std::array<std::string,1> labels{{"Under"}};
        buttons.SetControls(controls,labels,width,height,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(Palette())});
    }
    // 실제 현재 트리의 hover와 누름을 통과시킨다. 명령 callback을 직접 호출하지 않는다.
    void Click(ScreenPoint point) { poll={InputCode::kLeftButton,point.x,point.y};buttons.Poll(poll);buttons.Event(poll); }
};
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
    // 실제 Config.Set의 global. 경로는 현재 기본 설정의 salvageSid 키에 쓴다.
    CHECK(config.GetInt("salvageSid")==50);CHECK(client.commands.size()==2);if (client.commands.size()==2) CHECK(client.commands[1][0]==state.salvageCommand && client.commands[1][2]==50);
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
    objects.format=hooks.format;objects.count=[](Sid) { return 1U; };objects.salvageRefund=[](Sid,std::uint32_t,std::uint32_t) { return 0; };objects.playerName=[](std::uint32_t) { return "Player1"; };
    const auto open=[&] { client.poll={InputCode::kRelease,0,0};client.buttons.Poll(client.poll);client.menus->Open({160,120},[&](const auto& widgets,const auto& controls,const auto& menus) {
        widgets.BeginVertical();BindObjectMenuControls(objects,controls,menus,config,[](std::string_view)->std::uint32_t { throw std::logic_error("검사 미등록 정적 문자열"); });
        // Player 전체 몸체는 이 검사의 명시 경계다. 하위 노드/등록은 실제 builder를 쓴다.
        objects.playerMenu=[&](std::string_view text,bool first,bool second) { CHECK(first && !second);widgets.BeginVertical();controls.AddHeader(text);return menus.TakeCurrent(); };
        RawObjectMenu base(pool,client.assets.TypeTable().Types(),common,objects);BindCarrierMenuControls(hooks,controls);BindCarrierMenuBase(hooks,base);
        RawCarrierMenu carrier(pool,client.assets.TypeTable().Types(),finder,state,hooks);PriestMenuHooks priest;BindPriestMenuCarrier(priest,carrier);priest.baseMenu(Sid{50});
    }); };open();
    const auto regions=client.menus->Controls();CHECK(regions.size()>=4);if (regions.empty()) return;
    client.Click({(regions[0].rect.left+regions[0].rect.right)/2,(regions[0].rect.top+regions[0].rect.bottom)/2});
    CHECK(client.commands.size()==1);if (client.commands.size()==1) CHECK(client.commands[0][0]==state.castCommand && client.commands[0][2]==50);
    // 실제 명령 선택은 메뉴를 닫으므로 About 선택 전 새 수명으로 다시 연다.
    client.menus->Close();open();const auto about=client.menus->Point("About Temple");CHECK(about.has_value());if (about) client.Click(*about);
    CHECK(client.commands.size()==2);if (client.commands.size()==2) CHECK(client.commands[1][0]==common.helpCommand && client.commands[1][2]==123);
    client.menus->Close();CHECK(!client.menus->HasMenus());
}
