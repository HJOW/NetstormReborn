// 실제 NSInfo 전체 자식/문자/Slider/공통 수명과 Client 전역을 창 없이 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawNsInfoContext.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/HudNsInfoCreate.h"
#include "client/ScriptCommand.h"
#include "client/PaletteColors.h"
#include "client/PaletteShade.h"
#include <set>
#include <sstream>
using namespace netstorm::client;
namespace {
// 실제 Client 소유자를 만들며 Run/창/오디오 장치를 실행하지 않는다.
ClientOptions NsOptions() { ClientOptions value;value.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";value.noAudio=true;return value; }
// 같은 실제 자산과 Client 전역으로 공통 Gump를 구성한다. 게임 값/성공 결과의 대체 객체는 없다.
struct NsScene {
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client{NsOptions()}; // 모든 전역/물리 소비자보다 오래 유지한다.
    FontStore fonts{assets.files,"Arial"};std::shared_ptr<RawSurfaceStorage> storage=MakeRawSurfaceStorage(assets.game);ScriptStringRing strings; // 같은 실제 글꼴/물리/80칸 고리다.
    std::array<std::uint32_t,256> logical{};PaletteColorTable palette;bool failTranslation{};std::vector<std::string> translated; // 현재 보호 팔레트와 명시 외부 중단 입력이다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[] { return 0.0; }}};RawMenuControls* menus{}; // 같은 등록표와 Native 수명/입력 처리기다.
    // 같은 초기 메뉴 배경과 실제 보호 색을 연결한다.
    NsScene() {
        for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 원문 DWORD 채널 순서다.
        palette=BuildPaletteColorTable(logical);RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(palette);Resize();client.NsInfoGlobals()->serverSelection=-1;
    }
    // 페이지 교체도 실제 등록표 정리를 통해 새 빈 화면을 설치한다.
    void Resize() { const auto& asset=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(asset,"A01",0);const auto metrics=assets.game.Shapes().SquidMetrics(asset.block,frame);buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,{}}); }
    // 실제 Client 고정 주소와 같은 문자열 고리를 원문 생성자에 제공한다.
    RawNsInfoContext Context() {
        RawNsInfoContext value;value.globals=client.NsInfoGlobals();value.gameplay=client.GameplayGlobals();value.storage=storage;
        value.read=[this](auto address) { if (address==0x5411c8U) return static_cast<std::uint32_t>(assets.game.Find("fortGump").block+70);if (address==0x5bec3cU) return static_cast<std::uint32_t>(palette.basic[7]);throw std::logic_error("검사의 예상 밖 원본 전역 조회"); };
        value.translate=[this](std::string_view text) { translated.emplace_back(text);if (failTranslation && text=="Start Battle") throw std::runtime_error("명시 번역 장치 중단");return std::string(text); };
        value.storeText=[this](auto text) { return 0x70000000U|strings.Store(text); };value.text=[this](auto token) { return strings.Resolve(token&0x0fffffffU); };return value;
    }
    // 실제 공통 등록 개수를 읽으며 메뉴 노드 사본을 만들지 않는다.
    std::uint32_t Count() const { std::istringstream input(buttons.Report());std::string name;std::uint32_t controls{},count{};input>>name>>controls>>count;CHECK(name=="RawButtons");return count; }
    // 전체 실제 픽셀 출력을 수행하며 화면 밖 행 padding을 보존해야 한다.
    std::vector<std::uint8_t> Draw() { std::vector<std::uint8_t> pixels(647*480,0x5a);menus->Draw(pixels,647,480,palette,BuildGumpShadeMaps(logical));return pixels; }
};
}

// Session 없이130실제 노드/12버퍼·Slider를 만들고 현재 파싱된 제목과 숨김 전체 출력을 검사한다.
TEST_CASE(NsInfoClient_FullRealChildrenBuffersAndHiddenPixelsBeforeSession) {
    NsScene scene;const auto before=scene.Count();const auto pixels=scene.Draw();const auto token=scene.buttons.ConstructNsInfo(scene.Context());auto* root=static_cast<HudNsInfoNode*>(scene.menus->Node(token));
    CHECK(root && root->vtable==0x50d074 && root->command==6 && !(root->flags&1) && scene.menus->NsInfoRoot()==token && scene.client.NsInfoGlobals()->current==token);
    CHECK(scene.Count()==before+130 && !scene.client.GameplayGlobals()->Borrowed() && scene.client.PlayerRecords()->Base()==0 && scene.strings.Index()==1);CHECK(scene.menus->Report().find("raw_nsinfo_owned\t50\n")!=std::string::npos);
    CHECK(scene.menus->Node(root->background)->parent==token && !(scene.menus->Node(root->roster)->flags&1) && !(scene.menus->Node(root->battle)->flags&1));std::set<std::uint32_t> buffers,sliders;std::uint32_t texts{},masters{},special{}; // 실제 등록표의 파생 종류와 별도 소유 버퍼를 구별한다.
    for (std::uint32_t id=1;id<1000;++id) if (auto* node=scene.menus->Node(scene.buttons.RegisteredGump(id))) { // 같은 고정 주소의 실제 노드만 읽는다.
        if (node->vtable==0x51454c) { const auto* text=static_cast<ScrollingTextGumpNode*>(node);++texts;CHECK(text->buffer && text->ownsBuffer && text->slider);buffers.insert(text->buffer);sliders.insert(text->slider);CHECK(scene.menus->Node(text->slider)->parent==scene.buttons.RegisteredGump(id)); }
        if (node->vtable==0x5144d4) ++masters;if (node->vtable==0x5038e8) ++special;
    }
    CHECK(texts==12 && buffers.size()==12 && sliders.size()==12 && masters==9 && special==8);
    for (const auto player:root->players) { const auto* row=static_cast<HudPlayerInfoNode*>(scene.menus->Node(player));CHECK(row && row->vtable==0x50cd64 && row->command==37 && row->player==0 && row->text && row->master && row->button && row->start);const auto* button=static_cast<HudPlayerButtonNode*>(scene.menus->BaseButton(row->button));CHECK(button && button->entry==0 && button->command==56 && button->baseWidth==16 && button->baseHeight==15 && (button->buttonFlags&6)==6);CHECK(std::string(scene.menus->Button(row->start)->label.data())=="Start Battle"); } // 실제8필수 자식을 확인한다.
    root->baseWidth=root->width=640;root->baseHeight=root->height=480;scene.menus->Attach(token,0,0,scene.buttons.RootToken(),0);CHECK(scene.menus->Node(root->background)->baseWidth==640 && scene.menus->Node(root->background)->baseHeight==480 && scene.Draw()==pixels);
    const auto report=scene.menus->Report();CHECK(report.find("LAN Server")!=std::string::npos && report.find("Players in Zone 0")!=std::string::npos);scene.menus->CloseNsInfo();CHECK(scene.Count()==before && !scene.menus->NsInfoRoot());CHECK(scene.client.NsInfoGlobals()->current==token); // 원문이 보존하는 죽은 current 전역과 실제 생존을 구별한다.
}

// 새/기존 NSInfo를 동시에 만드는 Native 교체 경계와 전체 페이지 정리를 같은 실제 수명으로 검사한다.
TEST_CASE(NsInfoClient_TwoRootsThenRealRetirementAndPageCleanup) {
    NsScene scene;const auto before=scene.Count();const auto first=scene.buttons.ConstructNsInfo(scene.Context()),second=scene.buttons.ConstructNsInfo(scene.Context());CHECK(first!=second && scene.menus->NsInfoRoot()==second && scene.Count()==before+260 && scene.strings.Index()==2);
    scene.buttons.KillGump(first);scene.buttons.CleanupGumps();CHECK(!scene.menus->Node(first) && scene.menus->NsInfoRoot()==second && scene.Count()==before+130);scene.Resize();CHECK(scene.Count()==before && !scene.menus->NsInfoRoot());
    const auto third=scene.buttons.ConstructNsInfo(scene.Context());CHECK(third!=first && third!=second);scene.menus->CloseNsInfo();CHECK(scene.Count()==before);
}

// 번역 장치 중단/실제 메타맵 관문 뒤 부분 효과를 숨기지 않고 공통 Close로 반납한다.
TEST_CASE(NsInfoClient_PartialTranslationAndSelectedMetamapAreNotFakeSuccess) {
    NsScene scene;const auto before=scene.Count();scene.failTranslation=true;bool rejected{};try { scene.buttons.ConstructNsInfo(scene.Context()); } catch (const std::runtime_error&) { rejected=true; }
    CHECK(rejected && scene.Count()>before && scene.client.NsInfoGlobals()->current==0);scene.menus->CloseNsInfo();CHECK(scene.Count()==before && scene.menus->Report().find("raw_nsinfo_owned\t0\n")!=std::string::npos);scene.failTranslation=false;
    scene.storage->ownerMode.challenge=true;scene.client.GameplayGlobals()->player.playerKnown=1;rejected=false;try { scene.buttons.ConstructNsInfo(scene.Context()); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && scene.Count()>before && scene.client.NsInfoGlobals()->current==0);scene.menus->CloseNsInfo();CHECK(scene.Count()==before && scene.menus->Report().find("raw_nsinfo_owned\t0\n")!=std::string::npos);scene.storage->ownerMode.challenge=false;scene.client.GameplayGlobals()->player.playerKnown=0;
    CHECK(scene.buttons.ConstructNsInfo(scene.Context())!=0);scene.menus->CloseNsInfo();CHECK(scene.Count()==before);
}

// 같은 Client 옵션 DWORD와 현재80BYTE 버퍼의 LAN/유효/무효 항목 동작을 검사한다.
TEST_CASE(NsInfoClient_CurrentServerConfigAndNativeNameBufferRules) {
    NsScene scene;auto globals=scene.client.NsInfoGlobals();globals->LoadServerNames(scene.assets.files.Read("d/!rootservers.dat"));CHECK(globals->servers.size()==2);scene.client.Configuration().Set("curServer","0");CHECK(globals->serverSelection==0);
    CHECK(globals->ResolveServerName({})=="Main Server" && globals->serverName.back()==0);scene.client.Configuration().Set("curServer","1");CHECK(globals->ResolveServerName({})=="Backup Server");
    const auto saved=globals->serverName;globals->serverSelection=100;CHECK(globals->ResolveServerName({})=="Backup Server" && globals->serverName==saved);globals->serverSelection=-2;CHECK(globals->ResolveServerName({})=="Backup Server" && globals->serverName==saved);
    globals->serverName.fill('Q');scene.client.Configuration().Set("curServer","-1");CHECK(globals->serverSelection==-1);CHECK(globals->ResolveServerName([](auto text) { CHECK(text=="LAN Server");return std::string("LAN name"); })=="LAN name" && globals->serverName.back()=='Q');
    scene.client.Configuration().Set("otherOption","9");CHECK(globals->serverSelection==-1); // 누락 ReadInt는 현재 선택 값을 보존한다.
}
