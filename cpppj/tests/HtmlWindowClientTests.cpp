// 실제 Client HTML 전역·20돌 버튼·ScrollingText/Slider/StyleText와 같은 공통 등록표 수명을 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawHtmlWindowContext.h"
#include "client/StoneButtonGump.h"
#include "client/PaletteShade.h"
#include <bit>
#include <sstream>
using namespace netstorm::client;
namespace { // 실제 소유자와 외부 입력 관측만 구성한다.
ClientOptions HtmlOptions() { ClientOptions value;value.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";value.noAudio=true;return value; } // 창/소리/게임 루프 없이 Client 소유자를 만든다.
struct HtmlScene { // 같은 보호 자산/글꼴/공통 노드/Client 전역을 유지한다.
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client{HtmlOptions()};FontStore fonts{assets.files,"Arial"}; // 실제 자산과 장기 소유자다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[] { return 0.0; }}};RawMenuControls* menus{};InputQueue input; // 같은 공통 UI와 실제40칸 raw 큐다.
    double now{123.25};std::vector<std::uint32_t> queueCounts,links;std::uint32_t auxiliaryOnExit{},observedOnExit{}; // 실제 callback 순서와 명시 외부 입력을 관측한다.
    std::uint32_t wallCalls{};std::function<void()> onClock; // 현재 clock 호출 수와 명시 경계의 현재 필드 변경 입력이다.
    HtmlScene() { // 실제 보호 팔레트/장식/버튼/본문을 준비하며 Win32 창은 만들지 않는다.
        std::array<std::uint32_t,256> logical{};for (std::size_t i=0;i<logical.size();++i) { const auto c=assets.game.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 같은 원문 색 채널 순서다.
        RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(BuildPaletteColorTable(logical));Resize();
    }
    ~HtmlScene() { if (menus) menus->CloseHtmlWindows(); } // callback이 참조하는 관측/clock 멤버를 파괴하기 전에 실제 UI를 먼저 반납한다.
    void Resize() { const auto& asset=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(asset,"A01",0);const auto size=assets.game.Shapes().SquidMetrics(asset.block,frame);buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),size.width,size.height,{}}); } // 실제 페이지 반납이다.
    RawHtmlWindowContext Context() { RawHtmlWindowContext value;value.globals=client.HtmlWindowGlobals();value.wallSeconds=[this] { ++wallCalls;if (onClock) onClock();return now; };value.queue=[this](auto event,auto first,auto second) { queueCounts.push_back(client.HtmlWindowGlobals()->window.instances);input.Push(event,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second));if (observedOnExit) { auto& root=Root(observedOnExit);CHECK(root.onExitCommand==event && root.onExitParameter==first && first==second);if (auxiliaryOnExit) root.auxiliarySerial=auxiliaryOnExit; } };value.link=[this](auto token) { links.push_back(token); };return value; } // 같은 현재 장기 상태와 raw 큐다.
    std::uint32_t Create(std::uint32_t width=400,std::uint32_t height=30) { return menus->CreateHtmlWindow(Context(),width,height); } // 원문 전체 저장소/생성자를 사용한다.
    HtmlWindowGumpNode& Root(std::uint32_t token) { return *static_cast<HtmlWindowGumpNode*>(menus->Node(token)); } // 현재 같은 파생 주소다.
    std::uint32_t Count() const { std::istringstream report(buttons.Report());std::string name;std::uint32_t controls{},count{};report>>name>>controls>>count;CHECK(name=="RawButtons");return count; } // 실제 등록표 수만 관측한다.
    InputEvent Pop() { return input.Pop(false); } // 실제 큐의 다음 사건이다.
};
}

// 실제 장식 부모·Client count와 두 부모 교체/페이지 반납에서 장기 전역을 확인한다.
TEST_CASE(HtmlWindowClient_ActualConstructorConcurrentRootsAndClientGlobals) {
    HtmlScene scene;const auto before=scene.Count(),first=scene.Create(),second=scene.Create(0xffffffff,0x80000000);auto globals=scene.client.HtmlWindowGlobals();auto& a=scene.Root(first);auto& b=scene.Root(second);
    CHECK(scene.Count()==before+2 && globals->window.instances==2 && a.vtable==0x515e7c && a.command==27 && a.flags==9 && a.body==0 && a.controlCount==0 && a.html[0]==0 && a.timeoutSeconds==0 && a.timeoutCommand==0 && a.onExitCommand==0 && a.cornerType && a.type);
    CHECK(a.requestedWidth==400 && a.requestedHeight==30 && b.requestedWidth==0xffffffff && b.requestedHeight==0x80000000);for (const auto& entry:a.controls) CHECK(entry.child==0 && entry.command==0 && entry.parameter==0); // 실제20행이 초기화된다.
    scene.buttons.KillGump(first);scene.buttons.CleanupGumps();CHECK(globals->window.instances==1 && globals->window.lastCloseTime==123.25 && !scene.menus->Node(first) && scene.menus->Node(second)==&b && scene.Count()==before+1);
    scene.now=456.5;scene.Resize();CHECK(globals->window.instances==0 && globals->window.lastCloseTime==456.5 && scene.Count()==before && !scene.menus->Node(second));const auto third=scene.Create();CHECK(globals==scene.client.HtmlWindowGlobals() && globals->window.instances==1 && globals->window.lastCloseTime==456.5 && third!=second);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before);
}

// 실제 돌 버튼/각 배경의20행 한도와 Kill 지연·Clear 후 같은 부모에서 재생성을 확인한다.
TEST_CASE(HtmlWindowClient_ActualTwentyStoneControlsAndDeferredClear) {
    HtmlScene scene;const auto before=scene.Count(),root=scene.Create();auto* body=scene.menus->HtmlBackend();std::vector<std::uint32_t> children;
    for (std::uint32_t index=0;index<20;++index) { body->AddButton(root,"Action",0xfedcba98+index,0x87654321+index,index&1 ? 4 : 0);const auto& entry=scene.Root(root).controls[index];const auto* button=static_cast<StoneButtonGumpNode*>(scene.menus->Node(entry.child));CHECK(button && button->parent==root && button->background && button->child==button->background && (button->buttonFlags&4U)==(index&1U ? 4U : 0U) && entry.command==0xfedcba98+index && entry.parameter==0x87654321+index);children.push_back(entry.child); } // 같은 실제 버튼/자식 배경을 매번 조회한다.
    CHECK(scene.Root(root).controlCount==20 && scene.Count()==before+41);body->AddButton(root,"Ignored",1,2,1);CHECK(scene.Root(root).controlCount==20 && scene.Count()==before+41);body->ClearButtons(root);CHECK(scene.Root(root).controlCount==0 && scene.Count()==before+41);
    for (const auto& entry:scene.Root(root).controls) CHECK(entry.child==0 && entry.command==0 && entry.parameter==0); // 원문20행 현재 값이다.
    scene.buttons.CleanupGumps();CHECK(scene.Count()==before+1);for (const auto token:children) CHECK(!scene.menus->Node(token)); // 실제 공통 scalar에서 자식들이 반납된다.
    body->AddButton(root,"Again",0xffffffff,0xabcdef01,1);CHECK(scene.Root(root).controlCount==1 && scene.Root(root).controls[0].command==0xffffffff);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);
}

// 실제 소유 ScrollingText/Slider/StyleText 본문과 LF보충·기존 본문 Kill/반납을 확인한다.
TEST_CASE(HtmlWindowClient_ActualScrollingBodyStyleAndReplacementLifetime) {
    HtmlScene scene;const auto before=scene.Count(),root=scene.Create();auto* body=scene.menus->HtmlBackend();body->SetBody(root,"~rStyled text");const auto first=scene.Root(root).body;const auto* text=static_cast<ScrollingTextGumpNode*>(scene.menus->Node(first));CHECK(text && text->parent==root && text->secondaryDepth==0xffffffff && text->buffer && text->ownsBuffer==1 && text->slider && !(text->textFlags&4) && scene.menus->Node(text->slider)->parent==first);
    CHECK(scene.menus->Report().find("\t1\tStyled text\n")!=std::string::npos);const auto count=scene.Count();body->SetBody(root,"First\nSecond\n");const auto second=scene.Root(root).body;const auto* next=static_cast<ScrollingTextGumpNode*>(scene.menus->Node(second));CHECK(second!=first && next->buffer!=text->buffer && (text->flags&0x10000) && next->parent==root && scene.Count()>count && scene.menus->Report().find("\t2\tFirstSecond\n")!=std::string::npos);
    scene.buttons.CleanupGumps();CHECK(!scene.menus->Node(first) && scene.menus->Node(second)==next && scene.Count()==count);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.menus->Report().find("raw_html_body\t")==std::string::npos && scene.client.HtmlWindowGlobals()->window.instances==0);
}

// 실제 raw 큐에서 선택 명령→Kill→소멸 OnExit의 순서·전체DWORD·count 감소 시점·필드 보존을 확인한다.
TEST_CASE(HtmlWindowClient_ActualInputQueueAndNativeOnExitOrder) {
    HtmlScene scene;const auto before=scene.Count(),root=scene.Create();scene.menus->HtmlBackend()->AddButton(root,"Action",0xffffffff,0x87654321,1);auto& node=scene.Root(root);const auto serial=scene.menus->Node(node.controls[0].child)->serial;node.onExitCommand=0x01001234;node.onExitParameter=0xabcdef01;
    CHECK(scene.buttons.HandleGump(root,0x8200002c,serial,0)==1 && scene.client.HtmlWindowGlobals()->window.instances==1 && (node.flags&0x10000));const auto selected=scene.Pop();CHECK(selected.code==0xffffffff && std::bit_cast<std::uint32_t>(selected.x)==0x87654321 && selected.x==selected.y && scene.input.Empty());
    scene.observedOnExit=root;scene.buttons.CleanupGumps();const auto exit=scene.Pop();CHECK(exit.code==0x01001234 && std::bit_cast<std::uint32_t>(exit.x)==0xabcdef01 && exit.x==exit.y && scene.input.Empty() && scene.queueCounts==std::vector<std::uint32_t>({1,0}) && scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);scene.observedOnExit=0;
}

// OnExit 뒤의 현재 보조 serial 재조회와 미연결 전체 Fit의 명시 거부·기존 본문 보존을 확인한다.
TEST_CASE(HtmlWindowClient_CurrentAuxiliarySerialAndExplicitFitBoundary) {
    HtmlScene scene;const auto before=scene.Count(),first=scene.Create(),second=scene.Create();auto& a=scene.Root(first);a.onExitCommand=0x02000095;a.onExitParameter=77;scene.observedOnExit=first;scene.auxiliaryOnExit=scene.Root(second).serial;scene.buttons.KillGump(first);scene.buttons.CleanupGumps();CHECK(!scene.menus->Node(first) && !scene.menus->Node(second) && scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);scene.observedOnExit=0;scene.auxiliaryOnExit=0;
    const auto third=scene.Create();scene.menus->HtmlBackend()->SetBody(third,"Old body");const auto old=scene.Root(third).body;bool rejected{};try { scene.menus->Attach(third,10,10,scene.buttons.RootToken(),0); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && scene.Root(third).parent==0 && (scene.Root(third).flags&8));
    rejected=false;try { scene.menus->HtmlBackend()->SetBody(third,std::string(1007,'A')); }catch (const std::length_error&) { rejected=true; }CHECK(rejected && scene.Root(third).body==old && !(scene.menus->Node(old)->flags&0x10000));scene.Resize();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);
}

// 실제 Idle 분배/현재 기한 재조회와 raw0 명령도 큐에 넣는 원문 시간 경계를 확인한다.
TEST_CASE(HtmlWindowClient_ActualIdleCurrentDeadlineAndRawZeroCommand) {
    HtmlScene scene;const auto before=scene.Count(),root=scene.Create();scene.menus->Idle(root);CHECK(scene.wallCalls==0 && scene.input.Empty());auto& node=scene.Root(root);node.timeoutSeconds=1;node.timeoutCommand=0xfedcba98;node.timeoutParameter=0x87654321;scene.now=0.5;scene.menus->Idle(root);CHECK(scene.wallCalls==1 && scene.input.Empty() && !(node.flags&0x10000));
    scene.now=1.5;scene.onClock=[&] { node.timeoutSeconds=2; };scene.menus->Idle(root);CHECK(scene.wallCalls==2 && scene.input.Empty() && !(node.flags&0x10000));scene.onClock={};scene.now=2;scene.menus->Idle(root);const auto timed=scene.Pop();CHECK(timed.code==0xfedcba98 && std::bit_cast<std::uint32_t>(timed.x)==0x87654321 && timed.x==timed.y && (node.flags&0x10000) && scene.client.HtmlWindowGlobals()->window.instances==1);scene.buttons.CleanupGumps();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);
    const auto second=scene.Create();auto& zero=scene.Root(second);zero.timeoutSeconds=-1;zero.timeoutCommand=0;zero.timeoutParameter=0xffffffff;scene.now=0;scene.menus->Idle(second);CHECK(!scene.input.Empty());const auto rawZero=scene.Pop();CHECK(rawZero.code==0 && rawZero.x==-1 && rawZero.y==-1 && (zero.flags&0x10000));scene.buttons.CleanupGumps();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);
}
