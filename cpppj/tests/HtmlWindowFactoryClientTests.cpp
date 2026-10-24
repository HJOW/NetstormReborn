// 실제 보호 자산/Config/두 번 파서·링크/공통 UI에 전체 HTML 공장을 연결한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawHtmlWindowContext.h"
#include "client/MenuGumpWidgets.h"
#include "client/TellMapMenus.h"
#include "client/HtmlConditionEffect.h"
#include "client/PaletteShade.h"
#include "platform/Bitmap.h"
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <bit>
#include <sstream>
using namespace netstorm::client;
namespace { // 실제 장기 소유자와 명시 외부 화면 입력만 준비한다.
ClientOptions FactoryOptions() { ClientOptions value;value.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";value.noAudio=true;return value; } // 게임 루프/창을 실행하지 않는다.
template<class F> bool FactoryRejected(F action) { try { action(); }catch (const std::logic_error&) { return true; }return false; } // 선택된 누락 장치를 빈 성공과 구분한다.
struct FactoryScene { // UI callback이 참조하는 모든 실제 장치를 UI보다 오래 유지한다.
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client{FactoryOptions()};FontStore fonts{assets.files,"Arial"}; // 보호 자료와 같은 Client 전역이다.
    ScriptStringRing strings;InputQueue input;double now{100.5};std::uint32_t enableAll{},editor{};std::vector<std::string> diagnostics; // 같은80칸/원시 큐와 명시 현재 전역이다.
    HtmlTextParserGlobals parserGlobals;netstorm::o::ConfigInterface config{{}};std::uint32_t wallCalls{},fallbackCalls{};std::vector<std::string> links; // 같은 파서 전역/Config·선택 효과의 실제 관측이다.
    std::uint32_t screenWidth{640},screenHeight{480};ScreenRect mapView{0,0,640,480}; // 재사용한 콜백도 현재 출력과 지도 영역을 다시 읽는다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[this] { return now; }}};RawMenuControls* menus{}; // 같은 공통 등록표/수명/실제 파생 UI다.
    FactoryScene() { // 실제 팔레트/원문 글꼴/공통 루트를 준비한다.
        std::array<std::uint32_t,256> logical{};for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 실제 색 채널을 원문 순서로 공급한다.
        config.Set("OkButton","$Button=Okay,Config,0 ");RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(BuildPaletteColorTable(logical));
        const auto& fort=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(fort,"A01",0);const auto size=assets.game.Shapes().SquidMetrics(fort.block,frame);buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(fort.block+70),static_cast<std::uint32_t>(frame),size.width,size.height,{}});
    }
    ~FactoryScene() { if (menus) menus->CloseHtmlWindows(); } // 관측/시각/고리의 반납 전에 실제 부모를 먼저 닫는다.
    RawHtmlWindowContext Context() { // 같은 실제 장치와 명시 현재 화면을 원문 주소에 연결한다.
        RawHtmlWindowContext context;context.globals=client.HtmlWindowGlobals();context.wallSeconds=[this] { ++wallCalls;return now; };context.queue=[this](auto command,auto first,auto second) { input.Push(command,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };context.metadata.strings=&strings;
        context.metadata.commands.lookup=[](auto name) { if (name=="Config") return kScriptConfig;if (name=="Warning") return kScriptWarning;if (name=="LeaveBattle") return 0x020000c5U;return 0xffffffffU; };context.metadata.commands.unknown=[this](auto text) { diagnostics.emplace_back(text); };
        context.read=[this](auto address)->std::uint32_t { switch (address) { case 0x5ca9b4:return mapView.right-mapView.left;case 0x531860:case 0x5c78f0:return screenWidth;case 0x531864:case 0x5c78f4:return screenHeight;case 0x5ca9cc:return mapView.left;case 0x5ca9d0:return mapView.top;case 0x5ca9d4:return mapView.right;case 0x5ca9d8:return mapView.bottom;case 0x5cab30:return enableAll;case 0x55a480:return editor;case 0x5429ec:return 0x020000be;case 0x563e38:return buttons.RootToken();default:throw std::logic_error("메타 검사 예상 밖 실제 전역"); } };context.link=[this](auto token) { links.push_back(menus->ResolveHtmlWindowLink(token)); };context.factory.globals=&parserGlobals;BindHtmlPictureText(context.factory.html);BindHtmlConditionConfig(context.factory.html,config,OriginalEdition::Patch1078);context.factory.substitute=[this](auto text,auto capacity) { CHECK(capacity==8192);return config.SubstituteLocal(text); };context.factory.fallback=[this] { ++fallbackCalls;return config.PathSpec("OkButton"); };return context;
    }
    HtmlWindowGumpNode& Root(std::uint32_t token) { return *static_cast<HtmlWindowGumpNode*>(menus->Node(token)); } // 같은 실제 파생 주소다.
    std::uint32_t Count() const { std::istringstream report(buttons.Report());std::string name;std::uint32_t controls{},count{};report>>name>>controls>>count;CHECK(name=="RawButtons");return count; } // 실제 등록 수만 읽는다.
};
}

// 실제 Config 치환·기본OkButton·전체 본문/Fit/모달 입력과 반납을 함께 확인한다.
TEST_CASE(HtmlWindowFactoryClient_ActualConfigFallbackBodyModalAndQueue) {
    FactoryScene scene;scene.config.Set("caption","Storm Islands");const auto before=scene.Count();const auto serial=scene.menus->ShowHtmlWindow(scene.Context(),"{caption}<br>Choose a route.",0,400,0);const auto root=scene.buttons.SerialGump(serial);CHECK(serial && root && scene.client.HtmlWindowGlobals()->window.createdSerial==serial && scene.client.HtmlWindowGlobals()->window.lastCloseTime==100.5 && scene.wallCalls==1 && scene.fallbackCalls==1 && scene.Root(root).controlCount==1 && scene.Root(root).body && scene.Root(root).parent==scene.buttons.RootToken());
    CHECK(scene.menus->Report().find("Storm Islands")!=std::string::npos && scene.Root(root).controls[0].command==kScriptConfig && scene.Root(root).controls[0].parameter==0);scene.buttons.RouteInput({0x02000038,0,0});CHECK(scene.Root(root).flags&0x10000);const auto selected=scene.input.Pop(false);CHECK(selected.code==kScriptConfig && selected.x==0 && selected.y==0);scene.buttons.CleanupGumps();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0 && scene.wallCalls==2);
}

// 첫 링크 효과 직후 명령이 같은 부모 바이트를 읽고 두 번째 변환/삭제 요청 뒤에도 보존된다.
TEST_CASE(HtmlWindowFactoryClient_ActualImmediateLinksTwoPassAndKillBeforeCleanup) {
    FactoryScene scene;auto context=scene.Context();std::uint32_t token{},probes{};context.factory.html.command=[&](auto& parser,auto) { ++probes;const auto at=parser.output.rfind("~[T");CHECK(at!=std::string::npos);if (at==std::string::npos) return;token=static_cast<std::uint32_t>(std::stoul(parser.output.substr(at+3)));CHECK(scene.menus->ResolveHtmlWindowLink(token)=="route"); };const auto before=scene.Count();const auto serial=scene.menus->ShowHtmlWindow(std::move(context),"<a href=\"#route\"><$probe>Travel</a><br>",0,400,0);const auto root=scene.buttons.SerialGump(serial);CHECK(root && token && probes==1 && std::string(scene.Root(root).html.data())=="route" && scene.menus->ResolveHtmlWindowLink(token)=="route");
    CHECK(scene.buttons.HandleGump(root,0x02000025,0,token)==1 && (scene.Root(root).flags&0x10000) && scene.links==std::vector<std::string>{"route"} && scene.menus->ResolveHtmlWindowLink(token)=="route");scene.buttons.CleanupGumps();CHECK(scene.Count()==before && FactoryRejected([&] { scene.menus->ResolveHtmlWindowLink(token); }));
}

// 첫 변환에서 닫히지 않은 조건 상태를 두 번째 변환에 그대로 전달한다.
TEST_CASE(HtmlWindowFactoryClient_ActualSameParserHiddenStateAcrossPasses) {
    FactoryScene scene;const auto before=scene.Count();const auto serial=scene.menus->ShowHtmlWindow(scene.Context(),"visible<?0>hidden",0,400,0);const auto root=scene.buttons.SerialGump(serial);CHECK(root && scene.fallbackCalls==1 && scene.Root(root).body && scene.Root(root).controlCount==1);const auto report=scene.menus->Report();CHECK(report.find("visible")==std::string::npos && report.find("hidden")==std::string::npos);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before);
}

// 빈 출력은 원문xxx 자식을 부착/Kill하며 창 생략은 첫 파서 효과만 실행한다.
TEST_CASE(HtmlWindowFactoryClient_ActualEmptyDummyAndSuppressedParserEffects) {
    for (const bool suppress:{false,true}) { // 같은 실제 파서/명령을 빈 본문과 창 없는 경로로 나눠 관측한다.
        FactoryScene scene;auto context=scene.Context();const auto before=scene.Count();std::uint32_t root{},probes{};context.factory.html.command=[&](auto&,auto) { ++probes;root=scene.buttons.SerialGump(scene.client.HtmlWindowGlobals()->window.createdSerial); };const auto serial=scene.menus->ShowHtmlWindow(std::move(context),"<$probe>!?",suppress ? 1U : 0U,400,30);CHECK(serial==0 && probes==1 && scene.client.HtmlWindowGlobals()->window.createdSerial==0 && scene.wallCalls==0 && scene.fallbackCalls==0);
        if (suppress) CHECK(root==0 && scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);else { CHECK(root && (scene.Root(root).flags&0x10000) && scene.Root(root).body && scene.menus->Report().find("xxx")!=std::string::npos && scene.client.HtmlWindowGlobals()->window.instances==1);scene.buttons.CleanupGumps();CHECK(scene.Count()==before && scene.wallCalls==1 && scene.client.HtmlWindowGlobals()->window.instances==0); }
    }
}

// 새 모달의 깊이는 이전 부모뿐 아니라 실제 표시 중인 버튼까지 내려간 최소 깊이를 따른다.
TEST_CASE(HtmlWindowFactoryClient_ActualNestedModalDepthAndInputRestoration) {
    FactoryScene scene;const auto before=scene.Count();const auto first=scene.buttons.SerialGump(scene.menus->ShowHtmlWindow(scene.Context(),"First window",0,400,0));const auto firstDepth=scene.Root(first).depth;const auto buttonDepth=scene.menus->Node(scene.Root(first).controls[0].child)->depth;const auto second=scene.buttons.SerialGump(scene.menus->ShowHtmlWindow(scene.Context(),"Second window",0,400,0));
    std::printf("actual factory modal first=%u second=%u firstDepth=%u buttonDepth=%u secondDepth=%u\n",first,second,firstDepth,buttonDepth,scene.Root(second).depth); // 실제 표시 플래그/자식 최솟값을 Native에 공급할 관측이다.
    if (const auto* path=std::getenv("NETSTORM_HTML_FACTORY_MODAL_QA")) { std::ofstream file(path,std::ios::binary);file<<"actual_modal_target\t"<<first<<'\t'<<second<<'\n'<<scene.menus->Report(); } // 원본에 기대 결과 대신 실제 이전 창 트리를 공급한다.
    CHECK(first && second && first!=second && scene.Root(first).depth==firstDepth && scene.Root(second).depth==buttonDepth-3U && scene.client.HtmlWindowGlobals()->window.instances==2);
    scene.buttons.KillGump(second);scene.buttons.CleanupGumps();CHECK(scene.client.HtmlWindowGlobals()->window.instances==1 && scene.buttons.SerialGump(scene.Root(first).serial)==first);scene.buttons.RouteInput({0x02000038,0,0});const auto selected=scene.input.Pop(false);CHECK(selected.code==kScriptConfig && (scene.Root(first).flags&0x10000));scene.buttons.CleanupGumps();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);
}

// 실제 긴 본문·기본 버튼/스크롤·픽셀과 행padding을 검사하고 명시 QA 표면을 기록한다.
TEST_CASE(HtmlWindowFactoryClient_ActualLongBodyScrollingAndPixels) {
    FactoryScene scene;std::string source;for (int index=0;index<120;++index) source+="Storm islands and bridges require careful planning. "; // 원문8192BYTE 안에서 실제 글꼴 줄바꿈/화면 제한을 선택한다.
    const auto before=scene.Count(),root=scene.buttons.SerialGump(scene.menus->ShowHtmlWindow(scene.Context(),source,0,400,0));const auto* body=static_cast<ScrollingTextGumpNode*>(scene.menus->Node(scene.Root(root).body));CHECK(body && (body->textFlags&6)==6 && scene.Root(root).baseHeight<=480 && body->baseHeight<480);
    std::array<std::uint32_t,256> logical{};for (std::size_t index=0;index<logical.size();++index) { const auto color=scene.assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 같은 보호 팔레트의 실제 색 채널이다.
    std::vector<std::uint8_t> pixels(647*480,0x5a);scene.menus->Draw(pixels,647,480,BuildPaletteColorTable(logical),BuildGumpShadeMaps(logical));std::size_t changed{},padding{};for (int y=0;y<480;++y) for (int x=0;x<647;++x) { if (x<640) changed+=pixels[y*647+x]!=0x5a;else padding+=pixels[y*647+x]!=0x5a; } // 전체 창이 실제 표면에 그려지고7BYTE 행 끝을 덮지 않아야 한다.
    CHECK(changed>1000 && padding==0);if (const auto* path=std::getenv("NETSTORM_HTML_FACTORY_QA")) { std::vector<std::uint8_t> rgba(640*480*4);for (int y=0;y<480;++y) for (int x=0;x<640;++x) { const auto color=scene.assets.game.Palette().Color(pixels[y*647+x]);const auto at=(y*640+x)*4;rgba[at]=color.red;rgba[at+1]=color.green;rgba[at+2]=color.blue;rgba[at+3]=255; }netstorm::platform::WriteBitmap(path,640,480,rgba); } // 게임 창 없이 같은 실제 표면을 저장한다.
    scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before);
}

// 선택된 실제 Config/파서 경계가 없으면 생성 후 부분 효과를 보존하고 공통 수명으로 반납한다.
TEST_CASE(HtmlWindowFactoryClient_SelectedMissingServicesRejectAndCleanup) {
    for (const int missing:{0,1,2}) { // 치환/파서 전역/기본OkButton 조회를 각각 누락한다.
        FactoryScene scene;const auto before=scene.Count();auto context=scene.Context();if (missing==0) context.factory.substitute={};else if (missing==1) context.factory.globals=nullptr;else context.factory.fallback={};CHECK(FactoryRejected([&] { scene.menus->ShowHtmlWindow(std::move(context),"Visible body",0,400,0); }) && scene.client.HtmlWindowGlobals()->window.instances==1);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);
    }
}

// 신규 공장 뒤의 두 번째 메타/본문 갱신과 재사용·즉시 Cancel도 같은 실제 전역/노드로 수행한다.
TEST_CASE(MissionLoadingClient_ActualOwnerFactoryReuseQueueAndCleanup) {
    FactoryScene scene;auto& state=scene.client.HtmlWindowGlobals()->window;const auto before=scene.Count();std::uint32_t translated{};const auto cancel=[&] { ++translated;return std::string("Cancel"); };
    const auto serial=scene.menus->ShowMissionLoading(scene.Context(),"Starting Mission...",cancel),root=scene.buttons.SerialGump(serial);CHECK(root && state.transientSerial==serial && state.createdSerial==serial && state.instances==1 && translated==1 && scene.fallbackCalls==0 && scene.Root(root).updateDepth==0 && scene.Root(root).controlCount==1);
    const auto oldBody=scene.Root(root).body,oldButton=scene.Root(root).controls[0].child;CHECK(scene.Root(root).controls[0].command==0x020000c5U && scene.Root(root).controls[0].parameter==3);
    CHECK(scene.menus->ShowMissionLoading(scene.Context(),"Preparing islands...",cancel)==serial && translated==2 && state.instances==1 && scene.Root(root).body!=oldBody && scene.Root(root).controls[0].child!=oldButton && (scene.menus->Node(oldBody)->flags&0x10000) && (scene.menus->Node(oldButton)->flags&0x10000));
    scene.buttons.CleanupGumps();CHECK(scene.menus->Report().find("Preparing islands...")!=std::string::npos);scene.buttons.RouteInput({0x02000038,0,0});const auto event=scene.input.Pop(false);CHECK(event.code==0x020000c5U && event.x==3 && event.y==3 && state.instances==1 && scene.buttons.SerialGump(serial)==0);
    scene.buttons.CleanupGumps();CHECK(state.instances==0 && scene.Count()==before);const auto next=scene.menus->ShowMissionLoading(scene.Context(),"Starting Mission...",cancel);CHECK(next!=serial && state.transientSerial==next && state.createdSerial==next && state.instances==1);scene.menus->CloseMissionLoading();scene.buttons.CleanupGumps();CHECK(state.instances==0 && scene.Count()==before);
}

// 원문 사용자 메타와 '~.'는 기본 Cancel 조회를 생략하고 실제 전체 부모를 갱신한다.
TEST_CASE(MissionLoadingClient_CustomMetadataAndNoButtonPrefix) {
    FactoryScene scene;const auto serial=scene.menus->ShowMissionLoading(scene.Context(),"Custom route\n$ButtonInstant=Proceed,Config,7",{}),root=scene.buttons.SerialGump(serial);CHECK(root && scene.Root(root).controlCount==1 && scene.Root(root).controls[0].command==kScriptConfig && scene.Root(root).controls[0].parameter==7 && scene.menus->Point("Proceed"));
    CHECK(scene.menus->ShowMissionLoading(scene.Context(),"~.No automatic button",{})==serial && scene.Root(root).controlCount==0 && !scene.menus->Point("Proceed") && scene.Root(root).updateDepth==0 && scene.fallbackCalls==0);
    std::string source;for (int index=0;index<100;++index) source+="Islands "; // 논리1024BYTE 안에서 실제 줄바꿈과 전체 Fit을 선택한다.
    CHECK(scene.menus->ShowMissionLoading(scene.Context(),source,[] { return std::string("Cancel"); })==serial && scene.Root(root).body && scene.Root(root).baseWidth<=640 && scene.Root(root).baseHeight<=480 && scene.Root(root).controlCount==1);
}

// 같은 콜백/부모를 재사용해도 현재 뷰와 실제 공통 루트의 변경 크기를 따른다.
TEST_CASE(MissionLoadingClient_ReusedParentReadsCurrentView) {
    FactoryScene scene;const auto serial=scene.menus->ShowMissionLoading(scene.Context(),"Starting Mission...",[] { return std::string("Cancel"); });const auto first=scene.menus->MissionLoadingBounds();CHECK(first && first->left==120);
    scene.mapView={85,0,640,480};CHECK(scene.menus->ShowMissionLoading(scene.Context(),"Starting Mission...",[] { return std::string("Cancel"); })==serial);const auto moved=scene.menus->MissionLoadingBounds();CHECK(moved && moved->left==162 && moved->right-moved->left==400);
    scene.screenWidth=1024;scene.screenHeight=768;scene.mapView={85,0,1024,768};const auto& fort=scene.assets.game.Find("fortGump");const auto frame=scene.assets.game.FrameIndex(fort,"A01",0);const auto size=scene.assets.game.Shapes().SquidMetrics(fort.block,frame);scene.buttons.SetControls({}, {},1024,768,{static_cast<std::uint32_t>(fort.block+70),static_cast<std::uint32_t>(frame),size.width,size.height,{}});
    CHECK(scene.buttons.SerialGump(serial)==0);const auto replaced=scene.menus->ShowMissionLoading(scene.Context(),"Starting Mission...",[] { return std::string("Cancel"); });CHECK(replaced!=serial && scene.client.HtmlWindowGlobals()->window.transientSerial==replaced);const auto next=scene.menus->MissionLoadingBounds();CHECK(next && next->left==354 && next->right-next->left==400 && next->top>=344 && next->top<=345);
}

// 공장의 반환 serial과 파서 효과가 변경한 현재 createdSerial을 복제/혼동하지 않는다.
TEST_CASE(MissionLoadingClient_FactoryEffectChangesCurrentCreatedOwner) {
    FactoryScene scene;auto context=scene.Context();std::uint32_t older{};context.factory.html.command=[&](auto&,auto) { scene.client.HtmlWindowGlobals()->window.createdSerial=older; };older=scene.menus->ShowHtmlWindow(std::move(context),"Older window",0,400,30);const auto oldRoot=scene.buttons.SerialGump(older),oldBody=scene.Root(oldRoot).body;
    const auto current=scene.menus->ShowMissionLoading(scene.Context(),"<$switch>New route\n$ButtonInstant=Travel,Warning,9",{}),newRoot=scene.buttons.SerialGump(current);const auto& state=scene.client.HtmlWindowGlobals()->window;
    CHECK(newRoot && newRoot!=oldRoot && state.createdSerial==older && state.transientSerial==current && state.instances==2 && scene.Root(oldRoot).body!=oldBody && scene.Root(oldRoot).controls[0].command==kScriptWarning && scene.Root(newRoot).controls[0].command==kScriptWarning && scene.Root(oldRoot).updateDepth==0 && scene.Root(newRoot).updateDepth==0);
}
