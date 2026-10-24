// 실제 보호 자산/Client/80칸 고리/메뉴/idle/원시 큐로 메타 본체의 연결을 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawHtmlWindowContext.h"
#include "client/MenuGumpWidgets.h"
#include "client/TellMapMenus.h"
#include "client/TellFileMenus.h"
#include <algorithm>
#include <bit>
#include <sstream>
using namespace netstorm::client;
namespace { // 실제 장기 소유자와 명시 외부 화면 입력만 준비한다.
ClientOptions MetadataOptions() { ClientOptions value;value.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";value.noAudio=true;return value; } // 게임 루프/창을 실행하지 않는다.
template<class F> bool MetadataRejected(F action) { try { action(); }catch (const std::logic_error&) { return true; }return false; } // 선택된 누락 장치를 빈 성공과 구분한다.
struct MetadataScene { // UI callback이 참조하는 모든 실제 장치를 UI보다 오래 유지한다.
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client{MetadataOptions()};FontStore fonts{assets.files,"Arial"}; // 보호 자료와 같은 Client 전역이다.
    ScriptStringRing strings;InputQueue input;double now{100.5};std::uint32_t enableAll{},editor{};std::vector<std::string> diagnostics; // 같은80칸/원시 큐와 명시 현재 전역이다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[this] { return now; }}};RawMenuControls* menus{}; // 같은 공통 등록표/수명/실제 파생 UI다.
    MetadataScene() { // 실제 팔레트/원문 글꼴/공통 루트를 준비한다.
        std::array<std::uint32_t,256> logical{};for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 실제 색 채널을 원문 순서로 공급한다.
        RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(BuildPaletteColorTable(logical));
        const auto& fort=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(fort,"A01",0);const auto size=assets.game.Shapes().SquidMetrics(fort.block,frame);buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(fort.block+70),static_cast<std::uint32_t>(frame),size.width,size.height,{}});
    }
    ~MetadataScene() { if (menus) menus->CloseHtmlWindows(); } // 관측/시각/고리의 반납 전에 실제 부모를 먼저 닫는다.
    RawHtmlWindowContext Context() { // 같은 실제 장치와 명시 현재 화면을 원문 주소에 연결한다.
        RawHtmlWindowContext context;context.globals=client.HtmlWindowGlobals();context.wallSeconds=[this] { return now; };context.queue=[this](auto command,auto first,auto second) { input.Push(command,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };context.metadata.strings=&strings;
        context.metadata.commands.lookup=[](auto name) { if (name=="Config") return kScriptConfig;if (name=="Warning") return kScriptWarning;return 0xffffffffU; };context.metadata.commands.unknown=[this](auto text) { diagnostics.emplace_back(text); };
        context.read=[this](auto address)->std::uint32_t { switch (address) { case 0x5ca9b4:case 0x531860:case 0x5c78f0:case 0x5ca9d4:return 640;case 0x531864:case 0x5c78f4:case 0x5ca9d8:return 480;case 0x5ca9cc:case 0x5ca9d0:return 0;case 0x5cab30:return enableAll;case 0x55a480:return editor;case 0x5429ec:return 0x020000be;default:throw std::logic_error("메타 검사 예상 밖 실제 전역"); } };return context;
    }
    std::uint32_t Create(RawHtmlWindowContext context) { return menus->CreateHtmlWindow(std::move(context),400,0); } // 실제 전체 부모 저장소를 생성한다.
    std::uint32_t Create() { return Create(Context()); } // 기본 명시 실제 장치 문맥이다.
    HtmlWindowGumpNode& Root(std::uint32_t token) { return *static_cast<HtmlWindowGumpNode*>(menus->Node(token)); } // 같은 실제 파생 주소다.
    std::uint32_t Count() const { std::istringstream report(buttons.Report());std::string name;std::uint32_t controls{},count{};report>>name>>controls>>count;CHECK(name=="RawButtons");return count; } // 실제 등록 수만 읽는다.
    std::uint32_t Process(std::uint32_t root,std::string_view source) { std::array<char,8192> buffer{};std::copy(source.begin(),source.end(),buffer.begin());return menus->ProcessHtmlMetadata(root,buffer); } // 원문8192BYTE 물리 버퍼를 사용한다.
    void BodyAndAttach(std::uint32_t root) { menus->HtmlBackend()->SetBody(root,"Choose an action.\n");menus->Attach(root,0,0,buttons.RootToken(),0); } // 같은 원문 본문과 전체 Fit에 부착한다.
    std::vector<std::uint32_t> MenuChildren(std::uint32_t root) { std::vector<std::uint32_t> result;for (auto child=Root(root).child;child;child=menus->Node(child)->next) if (menus->Node(child)->vtable==0x50c6f0) result.push_back(child);return result; } // 실제 메뉴 vtable과 현재 형제 링크만 관측한다.
};
}

// 처리 우선순위/원시 명령과20개 실제 돌 버튼 한도를 같은 등록표에서 확인한다.
TEST_CASE(HtmlWindowMetadataClient_ActualPriorityRawCapturesAndTwentyControls) {
    MetadataScene scene;const auto before=scene.Count(),root=scene.Create();std::string source="$OnExit=unused,Warning,9 \n$ButtonInstant=Fast,Config,2 \n";
    for (int index=0;index<24;++index) source+="$Button=Action"+std::to_string(index)+",Config,"+std::to_string(index+10)+" \n"; // 원문은24개를 처리하되 실제 버튼은20개다.
    source+="$Button=Raw, Config,7 \n";CHECK(scene.Process(root,source)==1 && scene.Root(root).controlCount==20 && scene.Root(root).onExitCommand==kScriptWarning && scene.Root(root).onExitParameter==9);
    for (std::size_t index=0;index<20;++index) { const auto& control=scene.Root(root).controls[index];const auto* button=static_cast<StoneButtonGumpNode*>(scene.menus->Node(control.child));CHECK(button && control.command==kScriptConfig && control.parameter==index+10 && !(button->buttonFlags&4)); } // ButtonInstant보다 일반 Button이 먼저20칸을 소비한다.
    CHECK(scene.strings.Index()==0 && scene.diagnostics.empty());scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before);const auto exit=scene.input.Pop(false);CHECK(exit.code==kScriptWarning && exit.x==9 && exit.y==9);
    const auto again=scene.Create();CHECK(scene.Process(again,"$Button=Raw, Config,7 \n")==1 && scene.Root(again).controls[0].command==0xffffffff && scene.diagnostics.empty());scene.menus->CloseHtmlWindows();
}

// 실제 일반/체크 메뉴 항목·disabled 비트와 전체 Fit의 스택 반납을 함께 확인한다.
TEST_CASE(HtmlWindowMetadataClient_ActualMenuCheckedFieldsFitAndCleanup) {
    MetadataScene scene;const auto before=scene.Count(),root=scene.Create();CHECK(scene.Process(root,"$Checked=Remember,Warning,8,1,0 \n$Menu=First,Config,7 \n")==1 && scene.Root(root).menuColumns==1);scene.BodyAndAttach(root);CHECK(scene.Root(root).menuColumns==0 && scene.menus->MenuInstances()==1);
    const auto children=scene.MenuChildren(root);CHECK(children.size()==1);if (children.size()!=1) return;const auto* menu=static_cast<MenuGumpNode*>(scene.menus->Node(children[0]));CHECK(menu->itemCount==2 && menu->items[0].command==kScriptConfig && menu->items[0].parameter==7 && menu->items[1].command==kScriptWarning && menu->items[1].parameter==8 && (menu->items[1].flags&0x204)==0x204);
    scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.menus->MenuInstances()==0);scene.menus->OpenAttached([](const auto&,const auto&,const auto& menu) { CHECK(menu.Current()==0);return 0U; },false);
}

// 원문 Timeout이 실제 idle 표에 등록되고 현재 시간에 raw 예약→Kill→OnExit가 실행된다.
TEST_CASE(HtmlWindowMetadataClient_ActualIdleDeadlineRawQueueAndOnExit) {
    MetadataScene scene;const auto before=scene.Count(),root=scene.Create();CHECK(scene.Process(root,"$Timeout=3,Config,12 \n$OnExit=unused,Warning,34 \n")==0 && scene.Root(root).timeoutSeconds==103.5);scene.buttons.IdleGumps();CHECK(scene.input.Empty() && !(scene.Root(root).flags&0x10000));scene.now=103.5;scene.buttons.IdleGumps();CHECK(scene.Root(root).flags&0x10000);
    const auto timeout=scene.input.Pop(false);CHECK(timeout.code==kScriptConfig && timeout.x==12 && timeout.y==12 && scene.input.Empty());scene.buttons.CleanupGumps();const auto exit=scene.input.Pop(false);CHECK(exit.code==kScriptWarning && exit.x==34 && exit.y==34 && scene.input.Empty() && !scene.menus->Node(root) && scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);scene.menus->CloseHtmlWindows();scene.buttons.IdleGumps();CHECK(scene.input.Empty());
}

// 보호 파일의 실제 SOLO/FORT 분류와 같은80칸 행 인자/선택을 실제 메뉴에서 대조한다.
TEST_CASE(HtmlWindowMetadataClient_ActualProtectedMapCatalogRowsAndFileSpec) {
    for (const bool solo:{false,true}) { // 두 원문 캐시 모드의 실제 디스크 파일과 키 복구를 각각 검사한다.
        MetadataScene scene;const auto before=scene.Count();netstorm::o::ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");config.Set("SOLOSpec","d/{local.1}.fort");config.SetInt("mayEditMissions",1);SoloMapFileSpec fileSpec;SoloMapCatalogEffects effects;effects.typeCount=[&] { return static_cast<std::int32_t>(scene.assets.game.TypeTable().Types().size()); };effects.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };effects.userName=[] { return std::string("metadata test"); };effects.translate=[](auto text) { return std::string(text); };effects.fileSpecKey=[&] { return fileSpec.Current(); };SoloMapCatalog catalog(scene.assets.files,config,std::move(effects));
        auto context=scene.Context();context.metadata.maps=&catalog;context.metadata.fileSpec=&fileSpec;context.metadata.currentMap=[] { return std::string_view("TEST01"); };context.metadata.fallbackMap=[] { return std::string_view{}; };const auto root=scene.Create(std::move(context));CHECK(scene.Process(root,solo ? "$Menu=*SOLOLIST*,Config,0 \n" : "$Menu=*FORTLIST*,Config,0 \n")==1);CHECK(fileSpec.Current()=="fortSpec" && bool(catalog.Cached().dirty)==solo);const auto count=catalog.Cached().count;CHECK(count>0);scene.BodyAndAttach(root);
        std::vector<std::string> names;for (const auto token:scene.MenuChildren(root)) { const auto* menu=static_cast<MenuGumpNode*>(scene.menus->Node(token));for (std::uint32_t index=0;index<menu->itemCount;++index) { const auto& item=menu->items[index];names.emplace_back(scene.strings.Resolve(item.parameter));CHECK(item.command==kReadFortCommand);const auto row=catalog.Row(static_cast<std::int32_t>(std::find(catalog.Cached().names.begin(),catalog.Cached().names.end(),names.back())-catalog.Cached().names.begin()),"TEST01","");CHECK(row && bool(item.flags&4)==row->checked && bool(item.flags&0x200)==!row->enabled); } } // 실제 항목을 같은 현재 캐시의 독립 행 조회와 대조한다.
        // 캐시의 실제 존재하는 문자열만 모아 열 분할 순서와 무관하게 전체 행 누락/중복을 확인한다.
        std::sort(names.begin(),names.end());std::vector<std::string> expected;for (const auto& name:catalog.Cached().names) if (name) expected.push_back(*name);std::sort(expected.begin(),expected.end());CHECK(names==expected);std::printf("metadata protected mode=%s rows=%d columns=%u\n",solo ? "SOLO" : "FORT",count,scene.menus->MenuInstances());scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.menus->MenuInstances()==0);
    }
}

// 미복원 선택 장치의 실패 뒤 미부착 메뉴/스택을 반납하고 같은 UI를 다시 쓸 수 있다.
TEST_CASE(HtmlWindowMetadataClient_SelectedBoundariesRetirePartialMenusAndStack) {
    for (const auto directive:{"$Menu=*TECHLIST*,Config,7 \n","$Menu=*THEMETECHLIST*,Config,7 \n","$Menu=@list,Config,7 \n","$Menu=#list,Config,7 \n","$Menu=%list,Config,7 \n","$Input=Name,40,empty \n","$Zsend=Name,Config,empty \n"}) { // 실제 하위 기능이 없는 모든 선택 분기를 빈 성공으로 숨기지 않는다.
        MetadataScene scene;const auto before=scene.Count(),root=scene.Create();CHECK(MetadataRejected([&] { scene.Process(root,std::string("$Menu=Existing,Config,1 \n")+directive); }));scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.menus->MenuInstances()==0);scene.menus->OpenAttached([](const auto&,const auto&,const auto& menu) { CHECK(menu.Current()==0);return 0U; },false);const auto next=scene.Create();CHECK(scene.Process(next,"$Button=Okay,Config,1 \n")==1);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before);
    }
}

// Key의 원문 무동작도 실제 고리를 소비하며80칸 이전 포인터는 해제된다.
TEST_CASE(HtmlWindowMetadataClient_ActualSharedRingKeyNoControlAndStaleTokens) {
    MetadataScene scene;const auto root=scene.Create();const auto stale=scene.strings.Store("old");std::string source;for (int index=0;index<82;++index) source+="$Key=K,Config,text \n"; // 원문 Key는 해석/고리 뒤 별도 컨트롤을 만들지 않는다.
    CHECK(scene.Process(root,source)==0 && scene.Root(root).controlCount==0 && scene.strings.Index()==3 && MetadataRejected([&] { scene.strings.Resolve(stale); }));CHECK(scene.Process(root,"$Button=Okay,Warning,last \n")==1);const auto token=scene.Root(root).controls[0].parameter;CHECK(scene.strings.Index()==4 && scene.strings.Resolve(token)=="last");scene.menus->CloseHtmlWindows();
}

// 명시 % 하위 반환을 실제 mutable 버퍼에 연결해 원문의 부분복사와 NUL 보존을 확인한다.
TEST_CASE(HtmlWindowMetadataClient_InlineLowerBoundaryPreservesPhysicalPartialCopy) {
    MetadataScene scene;auto context=scene.Context();std::uint32_t calls{};context.metadata.inlineList=[&](auto key,auto command,auto argument) { ++calls;CHECK(key=="list" && command=="Config" && argument=="7");return std::string("XY"); };const auto before=scene.Count(),root=scene.Create(std::move(context));std::array<char,8192> buffer;buffer.fill('!');const std::string source="HEAD $Menu=%list,Config,7 \nTAIL";std::copy(source.begin(),source.end(),buffer.begin());buffer[source.size()]=0;
    CHECK(scene.menus->ProcessHtmlMetadata(root,buffer)==1 && calls==1 && std::string(buffer.data())=="HEAD TAIL" && buffer[source.size()+1]=='!' && scene.Root(root).menuColumns==1);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.menus->MenuInstances()==0);
}

// 실제 @/# 본체가 같은 부모의 controlled 메뉴/구분선/80칸 고리에 직접 작성된다.
TEST_CASE(HtmlWindowMetadataClient_FileMenusActualConfigRowsAndSharedRing) {
    for (const bool fallback:{false,true}) { // 두 문법이 같은 메뉴 수명과 서로 다른 빈 제목 라벨을 사용한다.
        netstorm::o::ConfigInterface config([](auto path)->std::optional<std::vector<std::uint8_t>> { const std::string source=path=="d/a.english" ? "title=\"\"\n" : path=="d/b.english" ? "title=--divider\n" : "title=Cee\n";return std::vector<std::uint8_t>(source.begin(),source.end()); });
        config.Configuration().Assign("ready=1\n");MetadataScene scene;const auto before=scene.Count();auto files=std::make_shared<TellFileMenus>(config,[](auto pattern) { CHECK(pattern=="input");return std::vector<std::string>{"d/c.english","d/b.english","d/a.english"}; });auto context=scene.Context();context.metadata.fileMenus=files;const auto root=scene.Create(std::move(context));
        CHECK(scene.Process(root,fallback ? "$Menu=@input,Config,{cur.file} \n" : "$Menu=#input,Config,{cur.file} \n")==1 && scene.strings.Index()==3 && config.Registry().FindByName("cur")==nullptr);scene.BodyAndAttach(root);const auto children=scene.MenuChildren(root);CHECK(children.size()==1);
        if (children.size()==1) { const auto* menu=static_cast<MenuGumpNode*>(scene.menus->Node(children[0]));CHECK(menu->itemCount==3 && menu->items[0].command==kScriptConfig && scene.strings.Resolve(menu->items[0].parameter)=="a" && (menu->items[1].flags&0x10) && menu->items[2].command==kScriptConfig && scene.strings.Resolve(menu->items[2].parameter)=="c" && scene.menus->Point("Cee")); }
        scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.menus->MenuInstances()==0 && config.Registry().FindByName("a")!=nullptr); // 부모를 닫아도 적재 Config는 장기 목록의 같은 실제 수명이다.
    }
}
