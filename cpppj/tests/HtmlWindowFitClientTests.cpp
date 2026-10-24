// 실제 Client/글꼴/StyleText/돌 버튼/메뉴/공통 트리·픽셀에 원문 전체 HTML Fit을 접합한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawHtmlWindowContext.h"
#include "client/MenuGumpWidgets.h"
#include "client/MenuGumpControls.h"
#include "client/PaletteShade.h"
#include "platform/Bitmap.h"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <sstream>
using namespace netstorm::client;
namespace { // 실제 소유자/외부 화면 입력과 순수 관측만 준비한다.
ClientOptions FitOptions() { ClientOptions value;value.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";value.noAudio=true;return value; } // 게임 루프와 창을 만들지 않는다.
std::string FitLongBody() { std::string result;for (int index=0;index<120;++index) result+="Storm islands and bridges require careful planning. ";result+='\n';return result; } // 원문8192BYTE 이내의 실제 긴 문장 입력이다.
struct FitScene { // callback보다 오래 살아 있는 같은 실제 자산/Client/화면·공통 UI다.
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client{FitOptions()};FontStore fonts{assets.files,"Arial"}; // 보호 자산/장기 전역/실제 글꼴이다.
    int width,height;ScreenRect map;std::array<std::uint32_t,256> logical{};PaletteColorTable palette;double now{123};InputQueue input; // 현재 실제 표면·지도 뷰/팔레트/원시 입력이다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[this] { return now; }}};RawMenuControls* menus{}; // 같은 실제 등록표와 파생 분배다.
    explicit FitScene(int w=640,int h=480):width(w),height(h),map{20,30,w-20,h-30} { // 명시 화면 입력을 공통 루트와 같은 크기로 만든다.
        for (std::size_t index=0;index<logical.size();++index) { const auto c=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 실제 원문 색 채널 순서다.
        palette=BuildPaletteColorTable(logical);RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(palette);const auto& asset=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(asset,"A01",0);const auto size=assets.game.Shapes().SquidMetrics(asset.block,frame);buttons.SetControls({}, {},width,height,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),size.width,size.height,{}});
    }
    ~FitScene() { if (menus) menus->CloseHtmlWindows(); } // 현재 뷰/시각/큐를 파괴하기 전에 실제 UI를 반납한다.
    RawHtmlWindowContext Context() { // 같은 현재 화면/지도와 실제 Client 상태를 원문 주소별로 조회한다.
        RawHtmlWindowContext result;result.globals=client.HtmlWindowGlobals();result.wallSeconds=[this] { return now; };result.queue=[this](auto code,auto x,auto y) { input.Push(code,std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)); };result.link=[](auto) { throw std::logic_error("전체 Fit 검사 실제 링크 실행 미연결"); };
        result.read=[this](auto address) { switch (address) { case 0x5ca9b4:return static_cast<std::uint32_t>(map.right-map.left);case 0x531860:case 0x5c78f0:return static_cast<std::uint32_t>(width);case 0x531864:case 0x5c78f4:return static_cast<std::uint32_t>(height);case 0x5ca9cc:return static_cast<std::uint32_t>(map.left);case 0x5ca9d4:return static_cast<std::uint32_t>(map.right);case 0x5ca9d8:return static_cast<std::uint32_t>(map.bottom);case 0x5ca9d0:return static_cast<std::uint32_t>(map.top);default:throw std::logic_error("HTML Fit 예상 밖 실제 전역"); } };return result;
    }
    std::uint32_t Create(std::uint32_t requestedWidth=400,std::uint32_t requestedHeight=0) { return menus->CreateHtmlWindow(Context(),requestedWidth,requestedHeight); } // 실제 전체 부모를 만든다.
    HtmlWindowGumpNode& Root(std::uint32_t token) { return *static_cast<HtmlWindowGumpNode*>(menus->Node(token)); } // 현재 같은 부모 주소다.
    ScrollingTextGumpNode& Body(std::uint32_t root) { return *static_cast<ScrollingTextGumpNode*>(menus->Node(Root(root).body)); } // 현재 실제 본문이다.
    StoneButtonGumpNode& Button(std::uint32_t root,std::size_t index) { return *static_cast<StoneButtonGumpNode*>(menus->Node(Root(root).controls[index].child)); } // 같은 실제 버튼/측정이다.
    void Attach(std::uint32_t root) { menus->Attach(root,901,-77,buttons.RootToken(),0); } // Native가 무시하는 명시 x/y와 실제 부모/모드를 보낸다.
    std::uint32_t Count() const { std::istringstream report(buttons.Report());std::string name;std::uint32_t controls{},count{};report>>name>>controls>>count;CHECK(name=="RawButtons");return count; } // 실제 등록 수만 관측한다.
    std::vector<std::uint8_t> Draw(std::optional<ScreenRect> clip={}) { std::vector<std::uint8_t> pixels((width+7)*height,0x5a);menus->Draw(pixels,width+7,height,palette,BuildGumpShadeMaps(logical),clip);return pixels; } // 같은 실제 공통 정렬/클립/배경/글리프다.
};
}

// 실제 자연 본문/두 버튼을 원문400폭과 공통 깊이·현재 지도 중앙에 배치한다.
TEST_CASE(HtmlWindowFitClient_ActualShortBodyUniformButtonsCenterAndDepth) {
    FitScene scene;const auto before=scene.Count(),root=scene.Create();scene.menus->HtmlBackend()->AddButton(root,"Okay",0x02000095,1,1);scene.menus->HtmlBackend()->AddButton(root,"Cancel",0x02000096,2,0);scene.menus->HtmlBackend()->SetBody(root,"Starting Mission...\n");scene.Attach(root);const auto& node=scene.Root(root);const auto& text=scene.Body(root);const auto& first=scene.Button(root,0);const auto& second=scene.Button(root,1);
    const float maximum=static_cast<float>(std::max(first.textWidth,second.textWidth)+12);const float span=2*maximum+15;CHECK(node.baseWidth==400 && node.baseHeight==text.baseHeight+65 && node.width==node.baseWidth && node.height==node.baseHeight && node.parent==scene.buttons.RootToken() && !(node.flags&8) && node.depth==0xffffffb0);
    CHECK(node.x==(scene.map.right-scene.map.left-node.baseWidth)*0.5F+scene.map.left && node.y==(scene.map.bottom-scene.map.top-node.baseHeight)*0.5F+scene.map.top && first.baseWidth==maximum && second.baseWidth==maximum && first.x==static_cast<int>((400-span)*0.5F) && second.x==first.x+maximum+15 && first.y==static_cast<int>(text.baseHeight+30) && first.y==second.y);
    CHECK(text.parent==root && text.y==20 && text.x==static_cast<int>(node.baseWidth-text.baseWidth)/2 && text.depth==node.depth-1 && first.depth==node.depth-3 && scene.client.HtmlWindowGlobals()->window.instances==1);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);
}

// 실제 긴 문장의 화면 제한 분기는 현재320×240 표면에서 Slider/scroll을 켜고 본문 높이를 제한한다.
TEST_CASE(HtmlWindowFitClient_ActualLongBodyScreenLimitAndScrolling) {
    FitScene scene(320,240);scene.map={0,0,320,240};const auto before=scene.Count(),root=scene.Create();scene.menus->HtmlBackend()->SetBody(root,FitLongBody());scene.Attach(root);auto& node=scene.Root(root);auto& text=scene.Body(root);
    CHECK(node.baseWidth==320 && node.baseHeight==205 && text.baseWidth==280 && text.baseHeight==140 && (text.textFlags&6)==6 && (scene.menus->Node(text.slider)->flags&1) && text.scrollRequest==0 && node.x==0 && node.y==17.5F);const auto pixels=scene.Draw();CHECK(pixels.size()==327*240 && text.scroll==0);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.client.HtmlWindowGlobals()->window.instances==0);
}

// 실제 controlled 두 메뉴의 현재 스택 pop과 부모·본문·버튼 배치를 함께 확인한다.
TEST_CASE(HtmlWindowFitClient_ActualTwoMenuColumnsAndStackRetirement) {
    FitScene scene;const auto before=scene.Count(),root=scene.Create();scene.menus->HtmlBackend()->SetBody(root,"Choose a route.\n");scene.menus->HtmlBackend()->AddButton(root,"Okay",0x02000095,1,1);std::uint32_t first{},second{};
    scene.menus->OpenAttached([&](const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menus) { widgets.BeginVertical(0);controls.AddTitle("Left route");controls.AddText("Island A",0x02000095,1,true,true,false);first=menus.Current();widgets.BeginVertical(0);controls.AddTitle("Right route");controls.AddText("Island B",0x02000095,2,true,true,false);controls.AddChecked(true,"Remember choice",0x02000096,3,true,true);second=menus.Current();scene.Root(root).menuColumns=2;scene.Attach(root);CHECK(menus.Current()==0);return 0U; },false);
    const auto& node=scene.Root(root);const auto* left=scene.menus->Node(first);const auto* right=scene.menus->Node(second);CHECK(left && right && left->parent==root && right->parent==root && left->x>right->x && left->y==right->y && node.menuColumns==0 && scene.menus->MenuInstances()==2 && node.baseHeight>scene.Body(root).baseHeight+65 && scene.Button(root,0).y>=right->y+right->baseHeight);
    scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before && scene.menus->MenuInstances()==0 && !scene.menus->Node(first) && !scene.menus->Node(second));
}

// 이미 부착된 부모에서 실제 새 본문을 만들고 현재 뷰/요청 폭으로 재부착한다.
TEST_CASE(HtmlWindowFitClient_ActualAttachedBodyReplacementAndCurrentView) {
    FitScene scene;const auto before=scene.Count(),root=scene.Create();scene.menus->HtmlBackend()->SetBody(root,"Short body\n");scene.Attach(root);const auto old=scene.Root(root).body;scene.menus->HtmlBackend()->SetBody(root,FitLongBody());const auto current=scene.Root(root).body;CHECK(current!=old && (scene.menus->Node(old)->flags&0x10000) && scene.Body(root).parent==root && scene.Root(root).parent==scene.buttons.RootToken() && !(scene.Root(root).flags&8) && scene.Root(root).depth==0xffffffb0 && scene.client.HtmlWindowGlobals()->window.instances==1);scene.buttons.CleanupGumps();CHECK(!scene.menus->Node(old));
    scene.map={100,40,620,430};scene.Root(root).requestedWidth=320;scene.menus->HtmlBackend()->SetBody(root,"Changed body\n");scene.buttons.CleanupGumps();const auto& node=scene.Root(root);CHECK(node.baseWidth==320 && node.x==(520-node.baseWidth)*0.5F+100 && node.y==(390-node.baseHeight)*0.5F+40 && node.depth==0xffffffb0);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before);
}

// 현재 보조 serial 생존 여부가 전체 화면/지도 중앙 선택을 바꾸며 오래된 위치를 재사용하지 않는다.
TEST_CASE(HtmlWindowFitClient_ActualAuxiliaryReferenceSelectsCurrentCenter) {
    FitScene scene;scene.map={140,10,620,460};const auto before=scene.Count(),auxiliary=scene.Create(),root=scene.Create();scene.menus->HtmlBackend()->SetBody(root,"Centered body\n");scene.Root(root).auxiliarySerial=scene.Root(auxiliary).serial;scene.Attach(root);CHECK(scene.Root(root).x==(640-scene.Root(root).baseWidth)*0.5F && scene.Root(root).y==(480-scene.Root(root).baseHeight)*0.5F);
    scene.buttons.KillGump(auxiliary);scene.buttons.CleanupGumps();CHECK(!scene.menus->Node(auxiliary) && scene.client.HtmlWindowGlobals()->window.instances==1);scene.menus->HtmlBackend()->SetBody(root,"Map center again\n");CHECK(scene.Root(root).x==(480-scene.Root(root).baseWidth)*0.5F+140 && scene.Root(root).y==(450-scene.Root(root).baseHeight)*0.5F+10);scene.menus->CloseHtmlWindows();CHECK(scene.Count()==before);
}

// 같은 실제 배경/모서리/글리프/버튼 출력의 클립과7BYTE 행 padding을 확인하고 명시 QA 표면을 기록한다.
TEST_CASE(HtmlWindowFitClient_ActualPixelsClipsAndPadding) {
    FitScene scene;const auto root=scene.Create();scene.menus->HtmlBackend()->SetBody(root,"~rOriginal ~gcurrent ~wbody\nSecond line\n");scene.menus->HtmlBackend()->AddButton(root,"Okay",0x02000095,1,1);scene.menus->HtmlBackend()->AddButton(root,"Cancel",0x02000096,2,0);scene.Attach(root);const auto full=scene.Draw();const ScreenRect clip{170,190,390,290};const auto clipped=scene.Draw(clip);std::size_t differences{},changed{};
    for (int y=0;y<scene.height;++y) for (int x=0;x<scene.width+7;++x) { const auto inside=x>=clip.left && x<clip.right && y>=clip.top && y<clip.bottom;const auto at=y*(scene.width+7)+x;differences+=clipped[at]!=(inside ? full[at] : 0x5a);changed+=full[at]!=0x5a;if (x>=scene.width) differences+=full[at]!=0x5a; } // 출력 내용은 클립과 독립이며 행 끝은 보존한다.
    CHECK(differences==0 && changed>1000);
    if (const auto* output=std::getenv("NETSTORM_HTML_FIT_QA")) { std::vector<std::uint8_t> rgba(scene.width*scene.height*4);for (int y=0;y<scene.height;++y) for (int x=0;x<scene.width;++x) { const auto c=scene.assets.game.Palette().Color(full[y*(scene.width+7)+x]);const auto at=(y*scene.width+x)*4;rgba[at]=c.red;rgba[at+1]=c.green;rgba[at+2]=c.blue;rgba[at+3]=255; }netstorm::platform::WriteBitmap(output,scene.width,scene.height,rgba); } // 게임 창 없이 같은 실제 표면을 저장한다.
    scene.menus->CloseHtmlWindows();
}
