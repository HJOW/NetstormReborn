// 원본 메뉴 계층의 실제 처리기를 단일 Client 등록표·트리·입력·표면에 조립한다.
#include "client/RawMenuControls.h"
#include "client/DrawSort.h"
#include "client/GumpFormBinding.h"
#include "client/FloatingSpGump.h"
#include "client/RawKnowledgeMenu.h"
#include "client/RawCampaignSession.h"
#include "client/QuickHelpWindow.h"
#include "client/QuickHelpRequest.h"
#include "client/GumpHelp.h"
#include "client/SpriteColorMaps.h"
#include "client/ImageGump.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
#include <sstream>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 안정된 map의 파생 포인터만 반환한다. 기본 노드 복사본을 만들지 않는다.
template<class T> T* Find(const std::map<std::uint32_t,std::unique_ptr<T>>& nodes,std::uint32_t token) {
    const auto it=nodes.find(token);return it==nodes.end() ? nullptr : it->second.get();
}
}
// 실제 노드/조각·버퍼/처리기는 고정 주소에서 서로 참조한다.
struct RawMenuControls::Impl {
    RawMenuTree common; // Client 돌 버튼과 같은 공통 상태/처리기다.
    const GameAssets& assets; // 보호 TYPE/SHP를 읽는 같은 자산 저장소다.
    FontStore& fonts; // Client 글꼴 캐시를 공유한다.
    std::map<std::uint32_t,RawMenuExternalGumps> external; // 생산/미니맵의 독립 수명도 같은 실제 표면과 가상 Draw를 공유한다.
    RawMenuHooks hooks; // 실제 설정/명령/음향/마우스 경계다.
    PaletteColorTable palette; // 현재 파일 팔레트의 실제 색 번호다.
    GumpText metrics; // 기존 원본 일반 글자 측정기다.
    GumpDrawState clip; // 전체 메뉴 출력이 공유하는 실제 클립이다.
    GumpGlyphColorState colors; // 글리프/그림/질감이 공유하는 현재 VFX 색표다.
    ColorMap pictureColors{}; // 원본 그림 전용 치환표다.
    StyleTextState fragments; // 항목과 제목의 같은 실제 조각 소유 표다.
    MarkupState pictureState; // 같은 조각 토큰의 실제 그림 파생 자료다.
    StyleTextMarkupState markupState; // 현재 bracket 옵션/맞춤 전역이다.
    MenuGumpState menuState; // 실제 controlled20칸/인스턴스 수다.
    MenuGumpWidgetState widgetState; // 실제 현재 배경/색/객체 Form 전역이다.
    MenuGumpInputState inputState; // 실제 메뉴 식별/하위 요청/닫힘 사건이다.
    MenuGumpDrawState drawState; // 실제 색/배경/체크 그림 전역이다.
    GumpBackgroundDrawState backgroundState; // 실제 현재 배경 선/진단 색이다.
    GumpDecorationState decorationState; // 실제 장식 배경/모서리 전역이다.
    ScrollGumpState sliderState; // 실제 현재 Slider 사건들이다.
    ScrollGumpWidgetState sliderWidgetState; // 실제 Slider/화살표 정렬과 기본 배경이다.
    ScrollingTextGumpWidgetState titleState; // 실제 제목 생성/입력 사건 전역이다.
    std::int8_t backgroundColor{35}; // 원본 기본 배경 식별 BYTE다.
    std::uint32_t popup{}; // 마지막 루트 메뉴 토큰이며 원본 등록표의 생존 여부로 검사한다.
    std::vector<std::uint32_t> knowledge; // 같은 공통 트리에 실제 부착된 지식 카드 토큰이며 수명/열림은 노드에서 읽는다.
    QuickHelpPopupState popupState; // 005ca938/940의 마지막 팝업을 Help Back과 같은 저장소로 공유한다.
    RawHelpHooks helpHooks; // 실제 Client/현재 Session 경계이며 노드 수명과 별개다.
    QuickHelpWindowState helpState;HtmlBodyGumpState bodyState;QuickHelpHistory helpHistory; // 같은 캐시/전역/전역 이력이다.
    std::map<std::uint32_t,std::unique_ptr<QuickHelpContentNode>> helpWindows; // 실제0xd0 창이다.
    std::map<std::uint32_t,std::unique_ptr<HtmlBodyGumpNode>> helpBodies; // 실제 두8192바이트 부모다.
    std::map<std::uint32_t,std::unique_ptr<StoneButtonGumpNode>> helpButtons; // 공통 ButtonGump가 입력/배경을 처리한다.
    std::map<std::uint32_t,std::unique_ptr<ImageGumpNode>> helpImages; // 실제 ImageGump 자식이다.
    std::map<std::uint32_t,HtmlLinkTextState> helpLinks; // 각 HTML 부모와 같은 수명의 링크 영역이다.
    std::uint32_t nextLinkBase{0x62000000U},imageOwner{}; // 실제 주소 대신 별도 영역 토큰/Help의 비소유 그림 모드다.
    std::vector<std::uint32_t> imageOwners;ScreenPoint imageShadow{5,5}; // 모드8/배율1의 비소유자 표와 안정된 그림자 전역이다.
    std::unique_ptr<QuickHelpWindow> helpWindow;std::unique_ptr<HtmlBodyGump> helpBody;std::unique_ptr<ImageGump> helpImage; // 기존 전체 몸체다.
    ImageGumpHooks activeImage; // Draw 동안 같은 실제 표면에 연결한다.
    mutable std::array<std::uint32_t,4> lastInput{}; // 실제 파생 가상 입력의 마지막 토큰/사건/좌표를 진단 보고에만 보관한다.
    std::map<std::uint32_t,std::unique_ptr<MenuGumpNode>> menus; // 같은 기본 배경을 가진 실제 메뉴 노드다.
    std::map<std::uint32_t,std::unique_ptr<GumpDecorationNode>> decorations; // 실제 모서리 장식 노드다.
    std::map<std::uint32_t,std::unique_ptr<ScrollingTextGumpNode>> titles; // 실제 버퍼/Slider를 소유하는 제목 노드다.
    std::map<std::uint32_t,std::unique_ptr<ScrollGumpNode>> sliders; // 실제 제목 Slider 노드다.
    std::map<std::uint32_t,std::unique_ptr<ScrollGumpArrowNode>> arrows; // 공통 ButtonGump와 공유하는 화살표다.
    std::map<std::uint32_t,std::unique_ptr<ScrollGumpThumbNode>> thumbs; // 실제 배경 손잡이다.
    std::map<std::uint32_t,std::unique_ptr<GumpBackgroundNode>> backgrounds; // 화살표가 소유하는 실제 배경이다.
    std::map<std::uint32_t,std::unique_ptr<FloatingSpGumpNode>> costs; // 같은 실제 트리의 비용 표시 노드다.
    TextGumpState costTextState; // 한 줄 TextGump의 실제 종류 BYTE45다.
    std::unique_ptr<TextGump> costText; // 같은 조각/클립/기본 생성·수명을 사용하는 한 줄 표시기다.
    std::unique_ptr<FloatingSpGump> costGump; // 원본10.78 비용 표시의 생성/Idle/삭제다.
    GumpBackgroundAssets backgroundAssets; // 실제 모든 TYPE/SHP 헤더다.
    MarkupAssets markupAssets; // 실제 그림 조각 자산/기준점이다.
    GumpBackgroundDrawHooks activeBackground; // Draw 동안만 현재 실제 픽셀 대상에 연결된다.
    MarkupHooks activePicture; // Draw 동안만 현재 실제 그림 출력에 연결된다.
    const GumpTextDraw* activeText{}; // Draw 동안만 살아 있는 실제 글리프 출력기다.
    std::unique_ptr<GumpBackgroundCreate> backgroundCreate; // 공통 기본 생성자에 위임하는 배경 생성기다.
    std::unique_ptr<GumpBackgroundDraw> backgroundDraw; // 기존 실제 배경/질감 분배다.
    std::unique_ptr<GumpDecoration> decoration; // 기존 장식 생성/출력/수명이다.
    std::unique_ptr<Markup> pictures; // 기존 그림 조각 소유/측정/출력이다.
    std::unique_ptr<StyleTextMarkup> markup; // 기존 bracket 파서다.
    std::unique_ptr<StyleText> style; // 기존 문자열 파서/측정/출력/소멸이다.
    std::unique_ptr<StyleTextBufferStore> buffers; // 실제 제목 버퍼의 안정된 저장소다.
    std::unique_ptr<ScrollGump> slider; // 기존 Slider 값/가상 입력/부착이다.
    std::unique_ptr<ScrollGumpWidgets> sliderWidgets; // 기존 Slider/화살표/손잡이 생성/삭제다.
    std::unique_ptr<ScrollingTextGump> title; // 기존 실제 제목 줄 배치/출력이다.
    std::unique_ptr<ScrollingTextGumpWidgets> titleWidgets; // 기존 제목 생성/버퍼/입력/수명이다.
    std::unique_ptr<ScrollingTextGumpMarkup> titleMarkup; // 기존 제목 다중 줄 파서다.
    std::unique_ptr<MenuGump> menu; // 기존 메뉴 프레임/크기/controlled 스택이다.
    std::unique_ptr<MenuGumpLayout> layout; // 기존 메뉴 항목/전체 배치다.
    std::unique_ptr<MenuGumpWidgets> widgets; // 기존 실제 메뉴/장식/제목 생성이다.
    std::unique_ptr<MenuGumpControls> controls; // 기존 원본 항목/체크/제목/하위 메뉴 래퍼다.
    std::unique_ptr<MenuGumpInput> input; // 기존 선택/하위 메뉴/명령/닫기다.

    // 기존GumpFormBinding을그대로재사용하며각효과마다현재호스트/판본을검증한다.
    MenuGumpWidgetHooks FormHooks() {
        if (!hooks.objectForms) throw std::logic_error("현재메뉴 실제GumpFormHost미연결");
        MenuGumpWidgetHooks result;BindMenuGumpForms(common.create,widgetState,result,hooks.objectForms());return result;
    }

    // 모든 가상 훅은 실제 호출 때 현재 파생 저장소를 재조회한다.
    Impl(RawMenuTree supplied,const GameAssets& suppliedAssets,FontStore& suppliedFonts,RawMenuHooks suppliedHooks)
        :common(std::move(supplied)),assets(suppliedAssets),fonts(suppliedFonts),hooks(std::move(suppliedHooks)),metrics(fonts,suppliedAssets.Edition()),backgroundAssets(assets),markupAssets(assets) {
        if (common.create.collection.edition!=assets.Edition() || !common.token || !common.node || !common.attach || !common.changed || !hooks.command || !hooks.readInt || !hooks.sound || !hooks.poll || !hooks.cursor)
            throw std::invalid_argument("실제 메뉴 판본/공통 트리/Client 연결 오류");
        const auto& fort=assets.Find("fortGump");const auto backgroundType=static_cast<std::uint32_t>(fort.block+o::kFirstAssetTypeNumber);
        widgetState.backgroundType=backgroundType;widgetState.backgroundFrame=1;widgetState.objectFormType=3;
        helpState.backgroundType=backgroundType;helpState.backgroundFrame=0; //원본4c7160은5411c8의 fortGump TYPE와 프레임0을 Help의 공유 배경 전역에 쓴다.
        std::array<std::uint32_t,256> picturePalette{}; //원본557738 그림 치환표를 만드는 파일 팔레트 RGB다.
        // 일시 날씨 색 대신 이 자산 파일의 팔레트로 공유 그림자 표를 한 번 계산한다.
        for (std::size_t index=0;index<picturePalette.size();++index) { const auto color=assets.Palette().Color(static_cast<std::uint8_t>(index));picturePalette[index]=color.red|(static_cast<std::uint32_t>(color.green)<<8)|(static_cast<std::uint32_t>(color.blue)<<16); }
        pictureColors=BuildSpriteColorMaps(picturePalette).shadow;
        decorationState.backgroundType=backgroundType;decorationState.backgroundFrame=0;decorationState.cornerType=backgroundType;
        sliderWidgetState.defaultType=backgroundType;sliderWidgetState.defaultFrame=1;titleState.sliderType=backgroundType;titleState.sliderFrame=1;
        drawState.backgroundType=backgroundType;drawState.backgroundFrame=1;drawState.markerType=backgroundType;
        inputState.requestEvent=0x0200000d;inputState.closedEvent=0x020000be; // PE0054272c/005429ec의 초기 DWORD다.
        sliderState={0x0200002c,0x0200002f,0x0200002e,0x02000036}; // PE005427a8/b4/b0/d0의 현재 사건이다.
        titleState.changeEvent=0x02000036;titleState.stepEvent=0x0200002e;titleState.refreshEvent=0x0200001b;
        titleState.cursorEvent=0x0200003b;titleState.leftTagEvent=0x02000025;titleState.rightTagEvent=0x02000026; // PE005427e4/8c/90의 실제 사건이다.
        // 배경 생성/출력 모두 같은 실제 TYPE/SHP 공급을 사용한다.
        GumpBackgroundCreateHooks bg;bg.node=[this](auto token) { return Background(token); };
        GumpBackgroundDrawHooks bgDraw;bgDraw.node=bg.node;BindGumpBackgroundAssets(bg,bgDraw,backgroundAssets);
        bgDraw.clip=[this](auto rect) { SetGumpDrawClip(clip,rect); };bgDraw.shade=[this](auto amount) { activeBackground.shade(amount); };
        bgDraw.shape=[this](auto request) { activeBackground.shape(request); };bgDraw.fill=[this](auto rect,auto color,auto checker) { activeBackground.fill(rect,color,checker); };bgDraw.line=[this](auto line) { activeBackground.line(line); };
        backgroundCreate=std::make_unique<GumpBackgroundCreate>(common.create,common.factory,backgroundColor,std::move(bg));backgroundDraw=std::make_unique<GumpBackgroundDraw>(common.collection,clip,backgroundState,std::move(bgDraw));
        GumpDecorationHooks dh;dh.node=[this](auto token) { return Find(decorations,token); };dh.base=common.node;BindGumpDecorationAssets(dh,assets);
        dh.shape=[this](auto type,auto frame,auto point) { activeBackground.shape({type,frame,point,false}); };dh.invalidate=[this](auto) { common.changed(); };
        decoration=std::make_unique<GumpDecoration>(common.create,decorationState,*backgroundCreate,*backgroundDraw,common.collection,common.lifetime,std::move(dh));
        MarkupHooks ph;BindMarkupAssets(pictureState,ph,markupAssets);ph.color=[this] { if (!activePicture.color) throw std::logic_error("메뉴 그림 실제 Draw 색표 문맥 없음");activePicture.color(); };ph.shape=[this](auto request) { activePicture.shape(request); };pictures=std::make_unique<Markup>(fragments,pictureState,std::move(ph),assets.Edition());
        StyleTextMarkupHooks mh;BindStyleTextMarkupAssets(mh,assets);BindStyleTextMarkupPalette(mh,palette);mh.sound=hooks.sound;markup=std::make_unique<StyleTextMarkup>(fragments,*pictures,markupState,std::move(mh),assets.Edition());
        StyleTextHooks sh;BindStyleTextFonts(sh,metrics);BindStyleTextPalette(sh,palette);BindMarkup(sh,*pictures);sh=markup->Hooks(std::move(sh));
        sh.textDraw=[this](auto bytes,auto point,const auto& context) { if (!activeText) throw std::logic_error("메뉴 글자 실제 Draw 문맥 없음");activeText->Draw(bytes,point,DecodeGumpTextContext(context)); };
        style=std::make_unique<StyleText>(fragments,std::move(sh),assets.Edition());StyleTextBufferHooks bh;BindStyleTextBufferFonts(bh,metrics);BindStyleTextBufferMarkup(bh,*pictures);buffers=std::make_unique<StyleTextBufferStore>(fragments,*style,std::move(bh),[](auto) {});
        costText=std::make_unique<TextGump>(common.create,costTextState,common.factory,common.lifetime,common.collection,clip,*style,
            TextGumpHooks{[this](auto token) -> TextGumpNode* { return Find(costs,token) ? Find(costs,token) : ExternalText(token); },common.node});
        if (assets.Edition()==o::OriginalEdition::Patch1078) {
            FloatingSpGumpHooks fh;fh.node=[this](auto token) { return Find(costs,token); };
            fh.now=[this] { if (!hooks.gameSeconds) throw std::logic_error("비용 표시의 실제 게임 시계 미연결");return hooks.gameSeconds(); };
            fh.allocate=[this](auto bytes) { return Allocate(bytes,3); };fh.constructText=[this](auto token) { costText->Construct(token,0,0,0,0); };
            fh.clearText=[this](auto token) { costText->Clear(token); };fh.fitText=[this](auto token) { costText->Fit(token); };
            fh.styledText=[this](auto token,std::string_view text) { auto& node=*Find(costs,token);style->Parse(node.head,text,node.textContext); };
            fh.idle=[this](auto token,bool enabled) { common.lifetime.SetIdle(token,enabled); };fh.redraw=[this](auto,bool) { common.changed(); };
            fh.kill=[this](auto token) { common.lifetime.Kill(token); };fh.attach=common.attach;
            fh.reference=[](auto,auto) { throw std::logic_error("비용 표시00464540의 실제 객체 참조 미연결"); };
            fh.deleteText=[this](auto token,auto flags) { return costText->DeleteScalar(token,flags); };costGump=std::make_unique<FloatingSpGump>(common.create,std::move(fh));
        }
        ScrollGumpHooks sl;sl.node=[this](auto token) { return Find(sliders,token); };sl.thumbNode=[this](auto token) { return Find(thumbs,token); };sl.baseNode=common.node;sl.push=[this](auto token) { common.tree.Push(token); };sl.attach=common.attach;sl.redraw=[this](auto,bool) { common.changed(); };sl.poll=hooks.poll;
        slider=std::make_unique<ScrollGump>(common.create,sliderState,common.tree,common.collection,common.events,common.input,std::move(sl));
        ScrollGumpWidgetHooks sw;sw.node=[this](auto token) { return Find(sliders,token); };sw.arrow=[this](auto token) { return Find(arrows,token); };sw.thumb=[this](auto token) { return Find(thumbs,token); };sw.base=common.node;
        sw.allocate=[this](auto bytes) { return Allocate(bytes,2); };sw.attach=common.attach;sliderWidgets=std::make_unique<ScrollGumpWidgets>(common.create,sliderWidgetState,common.factory,common.stones,*backgroundCreate,*slider,common.lifetime,std::move(sw));
        ScrollingTextGumpHooks th;th.node=[this](auto token) { return Find(titles,token); };th.baseNode=common.node;th.redraw=[this](auto,bool) { common.changed(); };BindScrollingTextBuffers(th,*buffers);BindScrollingTextSlider(th,*slider);
        title=std::make_unique<ScrollingTextGump>(common.create,common.collection,clip,fragments,*style,std::move(th));
        ScrollingTextGumpWidgetHooks tw;tw.node=[this](auto token) { return Find(titles,token); };tw.slider=[this](auto token) { return Find(sliders,token); };tw.base=common.node;
        tw.allocate=[this](auto bytes) { return Allocate(bytes,1); };tw.push=[this](auto token) { common.tree.Push(token); };tw.attach=common.attach;tw.redraw=[this](auto,bool) { common.changed(); };tw.cursor=hooks.cursor;
        titleWidgets=std::make_unique<ScrollingTextGumpWidgets>(common.create,titleState,common.factory,common.tree,common.lifetime,common.events,common.input,fragments,*buffers,*title,*sliderWidgets,std::move(tw));
        titleMarkup=std::make_unique<ScrollingTextGumpMarkup>(*title,*style,*pictures,*markup,ScrollingTextGumpMarkupHooks{[this](auto token) { return Find(titles,token); },common.node});
        MenuGumpHooks gm;gm.node=[this](auto token) { return Find(menus,token); };gm.baseNode=common.node;gm.push=[this](auto token) { common.tree.Push(token); };gm.pop=[this](auto token) { const auto* node=common.node(token);common.attach(token,node->x,node->y,0,0); };gm.attach=common.attach;
        BindMenuGumpObjectAttach(gm,[this](std::uint32_t token) { return menus.contains(token) ? widgets.get() : nullptr; });
        gm.redraw=[this](auto token,auto erase) { Redraw(token,erase); };gm.baseRedraw=[this](auto,bool) { common.changed(); };menu=std::make_unique<MenuGump>(common.create,menuState,common.lifetime,std::move(gm));
        MenuGumpLayoutHooks lh;lh.node=[this](auto token) { return Find(menus,token); };lh.baseNode=common.node;lh.push=[this](auto token) { common.tree.Push(token); };lh.attach=common.attach;BindMenuLayoutText(lh,metrics);BindMenuLayoutStyleText(lh,*style);BindMenuTitleLayout(lh,*title);
        layout=std::make_unique<MenuGumpLayout>(common.create,*menu,std::move(lh));MenuGumpWidgetHooks mw;mw.node=[this](auto token) { return Find(menus,token); };mw.base=common.node;mw.allocate=[this](auto bytes) { return Allocate(bytes,0); };mw.attach=common.attach;
        mw.objectForm=[this](std::uint32_t type,std::uint32_t flags) { return FormHooks().objectForm(type,flags); };
        mw.formOwner=[this](std::uint32_t form,std::uint32_t owner) { FormHooks().formOwner(form,owner); };
        mw.formObject=[this](std::uint32_t form,std::uint32_t object,std::uint32_t flags) { FormHooks().formObject(form,object,flags); };
        mw.selectionNotice=[](auto,auto) { throw std::logic_error("메뉴 선택 외부 callback 미연결"); };widgets=std::make_unique<MenuGumpWidgets>(common.create,menuState,widgetState,*backgroundCreate,*decoration,*titleWidgets,*layout,*style,common.lifetime,common.collection,common.tree,std::move(mw));
        MenuGumpControlHooks ch;ch.node=[this](auto token) { return Find(menus,token); };ch.base=common.node;ch.readInt=hooks.readInt;controls=std::make_unique<MenuGumpControls>(common.create,menuState,*menu,*layout,*titleMarkup,std::move(ch));
        MenuGumpInputHooks ih;ih.node=[this](auto token) { return Find(menus,token); };ih.base=common.node;
        ih.broadcast=[this](auto token,auto event,auto first,auto second) { Broadcast(token,event,first,second); };ih.handle=[this](auto token,auto event,auto first,auto second) { return Handle(token,event,first,second); };ih.push=[this](auto token) { common.tree.Push(token); };ih.attach=common.attach;
        ih.redraw=[this](auto token,auto erase) { Redraw(token,erase); };ih.command=hooks.command;ih.sound=hooks.sound;ih.invalidate=[this](auto) { common.changed(); };input=std::make_unique<MenuGumpInput>(common.create,inputState,*menu,common.collection,common.events,common.lifetime,common.tree,std::move(ih));
    }
    // 현재 토큰을 소유한 외부 파생을 등록 순서대로 조회하며 다른 UI의 수명을 바꾸지 않는다.
    const RawMenuExternalGumps* External(std::uint32_t token) const {
        // 등록하지 않은 슬롯과 비소유 노드는 그대로 다음 처리기로 넘긴다.
        for (const auto& [slot,externalHooks]:external) if (externalHooks.owns && externalHooks.owns(token)) return &externalHooks;return nullptr;
    }
    // 외부 TextGump만 같은 한 줄 글자 처리기에 빌린다.
    TextGumpNode* ExternalText(std::uint32_t token) const {
        // 생산 글자와 다른 파생을 같은 노드로 잘못 분류하지 않는다.
        for (const auto& [slot,externalHooks]:external) if (externalHooks.text) if (auto* node=externalHooks.text(token)) return node;return nullptr;
    }
    // 같은0xd8 요청도 제목·Slider·손잡이는 호출 계층에 따라 구별한다.
    std::uint32_t Allocate(std::size_t bytes,int context) {
        const auto token=common.token();
        if (bytes==0xb90) menus.emplace(token,std::make_unique<MenuGumpNode>());
        else if (bytes==0xc4) decorations.emplace(token,std::make_unique<GumpDecorationNode>());
        else if (bytes==0xd8 && context==0) titles.emplace(token,std::make_unique<ScrollingTextGumpNode>());
        else if (bytes==0xd8 && context==1) sliders.emplace(token,std::make_unique<ScrollGumpNode>());
        else if ((bytes==0xd8 || bytes==0xd4) && context==2) thumbs.emplace(token,std::make_unique<ScrollGumpThumbNode>());
        else if (bytes==0x178) arrows.emplace(token,std::make_unique<ScrollGumpArrowNode>());
        else if (bytes==0x38 && context==1) return token;
        else if (bytes==0xb8 && context==3) costs.emplace(token,std::make_unique<FloatingSpGumpNode>());
        else throw std::logic_error("원본 메뉴 파생 할당 크기/문맥 오류");return token;
    }
    // 모든 공통 처리는 같은 실제 파생 기본 주소를 받는다.
    GumpCreateNode* Node(std::uint32_t token) const {
        if (auto* n=Find(helpWindows,token)) return n;if (auto* n=Find(helpBodies,token)) return n;if (auto* n=Find(helpButtons,token)) return n;if (auto* n=Find(helpImages,token)) return n;
        if (auto* n=Find(menus,token)) return n;if (auto* n=Find(decorations,token)) return n;if (auto* n=Find(titles,token)) return n;if (auto* n=Find(sliders,token)) return n;
        if (auto* n=Find(arrows,token)) return n;if (auto* n=Find(thumbs,token)) return n;if (auto* n=Find(costs,token)) return n;return Find(backgrounds,token);
    }
    // 화살표 배경처럼 공통 생성자가 소유한 실제 자식도 같은 출력 가지에 포함한다.
    bool OwnsBranch(std::uint32_t token) const {
        // 현재 실제 부모를 따라가며 메뉴 소유 조상을 찾는다.
        for (auto current=token;current;current=common.node(current)->parent) if (Node(current) || External(current)!=nullptr) return true;return false;
    }
    // 배경을 공유하는 파생 객체만 현재 배경 필드로 연결한다.
    GumpBackgroundNode* Background(std::uint32_t token) const {
        if (auto* n=Find(menus,token)) return n;if (auto* n=Find(decorations,token)) return n;if (auto* n=Find(thumbs,token)) return n;
        if (auto* n=Find(backgrounds,token)) return n;return common.background ? common.background(token) : nullptr;
    }
    // 각 실제 가상 Attach를 현재 분류로 호출한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
        if (helpWindows.contains(token)) helpWindow->Attach(token,x,y,parent,mode);else if (helpBodies.contains(token)) helpBody->Attach(token,x,y,parent,mode);else if (helpButtons.contains(token)) common.stones.Attach(token,x,y,parent,mode);
        else if (helpImages.contains(token)) helpImage->Attach(token,x,y,parent,mode);else if (menus.contains(token)) widgets->Attach(token,x,y,parent,mode);else if (titles.contains(token)) titleWidgets->Attach(token,x,y,parent,mode);
        else if (sliders.contains(token)) slider->Attach(token,x,y,parent,mode);else if (arrows.contains(token)) common.stones.Attach(token,x,y,parent,mode);else common.tree.Attach(token,x,y,parent,mode);
    }
    // 기존 표시 통지와 원본의 실제 메뉴 재그리기 요청을 유지한다.
    void Redraw(std::uint32_t token,bool erase) const { if (menus.contains(token)) menu->Redraw(token,erase);else if (decorations.contains(token)) decoration->Redraw(token,erase);else common.changed(); }
    // 부모가 전달한 원본 사건을 대상 자신에게 분배한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
        lastInput={token,event,first,second};
        if (helpWindows.contains(token)) return HelpEvent(token,{event,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)});
        if (menus.contains(token)) return input->Handle(token,event,first,second);if (titles.contains(token)) return titleWidgets->Handle(token,event,first,second);
        if (sliders.contains(token)) return slider->Handle(token,event,first,second);if (thumbs.contains(token)) return slider->ThumbHandle(token,event,first,second);return BaseHelp(token,event,first,second);
    }
    // 공통 Frame의 현재 마우스 좌표와 실제 파생 통지를 사용한다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
        if (menus.contains(token)) input->Broadcast(token,event,first,second);else if (titles.contains(token)) titleWidgets->Broadcast(token,event,first,second);else if (thumbs.contains(token)) slider->ThumbBroadcast(token,event,first,second);
    }
    // 전용 창의 serial은 등록 인덱스와 다르다. 죽은 노드는 원본464020처럼 제외한다.
    std::uint32_t Reference(std::uint32_t serial) const {
        if (!serial) return 0;
        // 루트 칸0을 제외한 현재 전체 등록표를 조회한다.
        for (std::size_t index=1;index<common.create.entries.size();++index) if (const auto token=common.create.entries[index]) { const auto* node=common.node(token);if (node && node->serial==serial && !(node->flags&0x10000)) return token; }return 0;
    }
    // 빈 요청/정적 themeHelp/현재 HTML 링크/입력 문자열 고리의 실제 수명을 구별한다.
    std::string HelpText(std::uint32_t token) const {
        if (!token) return {};if (token==0x61000001U) return "themeHelp";if (token==0x61000002U) return helpHooks.translate("Help");if (token==0x61000003U) return "F1Help";if (token==0x61000004U) return helpHooks.translate("Sky Overview");if (token==0x61000005U) return "minimapGump";
        // 링크 토큰은 소유 HTML 부모가 아직 살아 있는 경우에만 해석한다.
        for (const auto& [body,links]:helpLinks) if (!(common.node(body)->flags&0x10000) && token>=links.base && token-links.base<links.bytes.size()) return std::string(ResolveHtmlLink(links,token));
        return helpHooks.text(token);
    }
    // 실제 기본 GumpHelp의 우클릭 경로도 같은 controlled 메뉴/문자열/사운드를 사용한다.
    std::uint32_t BaseHelp(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
        if (!helpWindow) return 0;GumpHelpState state{0x61000002U,0x02000017U,{}};GumpHelpHooks h;h.node=common.node;h.text=[this](auto value) { return HelpText(value); };
        h.beginMenu=[this](auto flags) { widgets->BeginVertical(flags); };h.title=[this](auto value) { controls->AddTitle(HelpText(value)); };
        h.entry=[this](auto body,auto command,const auto& topic,auto,auto,auto) { controls->AddText(HelpText(body),command,helpHooks.storeText(topic),true,true,false); };
        h.attachMenu=[this](auto x,auto y,auto parent) { menu->AttachCurrent(x,y,parent); };h.sound=hooks.sound;
        return GumpHelp(common.create,state,std::move(h)).Handle(token,event,first,second);
    }
    // 생성 크기를 실제 종류로 할당한다. 돌 버튼의 배경은 공통 생성기가 별도로 소유한다.
    std::uint32_t AllocateHelp(std::size_t bytes) {
        const auto token=common.token();if (bytes==0xd0) helpWindows.emplace(token,std::make_unique<QuickHelpContentNode>());
        else if (bytes==0x40a4) { helpBodies.emplace(token,std::make_unique<HtmlBodyGumpNode>());auto& links=helpLinks[token];links.base=nextLinkBase;links.capacity=kHtmlBodyCapacity;if (nextLinkBase>0x6fffc000U) throw std::overflow_error("Help 링크 토큰 소진");nextLinkBase+=static_cast<std::uint32_t>(kHtmlBodyCapacity); }
        else if (bytes==0x170) helpButtons.emplace(token,std::make_unique<StoneButtonGumpNode>());else if (bytes==0xb4) helpImages.emplace(token,std::make_unique<ImageGumpNode>());
        else if (bytes==0xb8) backgrounds.emplace(token,std::make_unique<GumpBackgroundNode>());else if (bytes==0xc4) decorations.emplace(token,std::make_unique<GumpDecorationNode>());
        else if (bytes==0xd8) titles.emplace(token,std::make_unique<ScrollingTextGumpNode>());else throw std::logic_error("전용 Help 파생 크기 오류");return token;
    }
    // TYPE 정보와 본문이 같은 기본 HTML/링크·현재 정보 pane을 직접 사용한다.
    void RenderHelp(std::uint32_t body,std::uint32_t type,std::string_view topic,std::uint32_t pane,std::uint32_t stream) {
        if (stream) throw std::logic_error("Help 파일 기록 스트림 미연결");auto& node=*Find(helpBodies,body);auto& links=helpLinks.at(body);links.cursor=0;if (!links.bytes.empty()) links.bytes[0]=0;
        HtmlTextParserState state;auto base=helpHooks.html;BindHtmlLinkText(base,links);QuickHelpTypeInfo current;
        QuickHelpHtmlHooks h;h.type=[&]() -> const QuickHelpTypeInfo& { current=helpHooks.type(type);return current; };h.description=[&] { return helpHooks.description(type); };h.hitPoints=[&] { return helpHooks.hitPoints(type); };
        h.translate=helpHooks.translate;h.config=helpHooks.config;h.substitute=helpHooks.substitute;
        h.print=[](std::string_view pattern,std::span<const PriestMenuArgument> arguments) { // 원본 정보 서식의 %s/%d만 순서대로 쓴다.
            std::string result;std::size_t argument{};
            // 다른 서식은 이 경계의 실제 계약 위반이다.
            for (std::size_t index=0;index<pattern.size();++index) { if (pattern[index]!='%') { result+=pattern[index];continue; }if (++index==pattern.size()) throw std::logic_error("Help 서식 끝 오류");if (pattern[index]=='%') { result+='%';continue; }if (argument>=arguments.size()) throw std::logic_error("Help 서식 인자 부족");const auto& value=arguments[argument++];if (pattern[index]=='s') result+=std::get<std::string>(value);else if (pattern[index]=='d') result+=std::to_string(std::get<std::int32_t>(value));else throw std::logic_error("Help 미지원 서식"); }if (argument!=arguments.size()) throw std::logic_error("Help 서식 남은 인자");return result;
        };
        h.pane=[this,pane] { return common.node(pane); };h.paneText=[this,pane](auto text) { titleMarkup->Parse(pane,text); };h.paneScroll=[this,pane](auto y) { title->RequestScroll(pane,y); };
        h.panePush=[this,pane] { common.tree.Push(pane); };h.paneAttach=[this,pane] { const auto& n=*common.node(pane);common.attach(pane,n.x,n.y,0,0); };
        ManaPictureState mana;mana.type=static_cast<std::uint32_t>(assets.Find("mana").block+o::kFirstAssetTypeNumber);
        QuickHelpHtml parser(state,*helpHooks.globals,helpState.infoMode,type,topic,false,mana,std::move(h),std::move(base),&links);
        helpBody->Parse(body,{[] { return false; },[](auto,auto) {},[&] { const auto result=parser.Parse(helpState.helpData.value_or(std::string{}));std::copy(result.begin(),result.end(),node.body.begin());node.body[result.size()]=0; }});
        if (links.bytes.size()>node.links.size()) throw std::length_error("Help 링크8192바이트 초과");std::copy(links.bytes.begin(),links.bytes.end(),node.links.begin());
    }
    // 실제 전용 요청/이력/배치에 현재 입력을 전달한다. 화면 밖 클릭도 같은 원본 처리기가 판단한다.
    std::uint32_t HelpEvent(std::uint32_t token,InputEvent event) const {
        auto& window=*Find(helpWindows,token);QuickHelpEventHooks h;h.local=[this,token](auto e) { return common.collection.ScreenToLocal(token,{e.x,e.y}); };h.node=common.node;h.typeName=helpHooks.typeName;h.translate=helpHooks.translate;h.text=[this](auto value) { return HelpText(value); };
        h.request=[this,token,&window](auto type,auto topic) {
            QuickHelpRequestHooks request;request.lookup=helpHooks.lookup;request.print=[](auto pattern,auto value) { auto result=std::string(pattern);result.replace(result.find("%s"),2,value);return result; };
            request.history=[this](auto value) { const_cast<Impl*>(this)->helpHistory.Push(value); };request.show=[this,token](auto number,auto value) { helpWindow->Show(token,number,value); };request.push=[this,token] { common.tree.Push(token); };request.attach=[this,token](auto x,auto y,auto parent,auto mode) { common.attach(token,x,y,parent,mode); };QuickHelpRequest(window,std::move(request)).Open(type,topic);
        };
        const auto text=[this,&window] { return Find(helpBodies,window.body)->text; }; // 매 효과에서 현재 표시 자식을 읽는다.
        h.pageHeight=[this,text] { return common.node(text())->baseHeight; };h.scroll=[this,text](auto delta) { title->RequestRelativeScroll(text(),delta); };
        h.link=[this](auto value) { const_cast<Impl*>(this)->HelpLink(value); };h.kill=[this,token] { common.lifetime.Kill(token);common.changed(); };h.learn=helpHooks.learn;h.queue=helpHooks.queue;
        h.base=[this,token](auto e) { return BaseHelp(token,e.code,std::bit_cast<std::uint32_t>(e.x),std::bit_cast<std::uint32_t>(e.y)); };
        return DispatchQuickHelpWindow(window,event,const_cast<Impl*>(this)->helpHistory,const_cast<Impl*>(this)->popupState,h);
    }
    // 원본4a00c0의 내부 주제/cmd 분기다. 외부 OS 프로토콜은 현재 Client 경계로 명시 전달한다.
    void HelpLink(std::string_view value) {
        value=value.substr(0,value.find('\0'));const auto colon=value.find(':');const auto prefix=colon==std::string_view::npos ? std::string_view{} : value.substr(0,colon+1);const auto rest=colon==std::string_view::npos ? value : value.substr(colon+1);
        if (prefix.size()>=80 || rest.size()>=256) throw std::length_error("Help 원본 링크 버퍼 초과");
        if (o::AsciiLower(prefix)=="cmd:") helpHooks.script(rest);else if (prefix.empty() || (!rest.empty() && rest.front()=='#')) OpenHelp(helpHooks.storeText(rest),1,0);else helpHooks.external(value);
    }
    // 466600의 실제 dialog 등록/이전 창 깊이와 같은 목록을 사용한다.
    std::uint32_t OpenHelp(std::uint32_t topic,std::int32_t mode,std::uint32_t learnType) {
        if (!helpWindow) throw std::logic_error("전용 Help Client 경계 미설치");bodyState.screenWidth=common.create.screenWidth;bodyState.screenHeight=common.create.screenHeight;helpState.battle=helpHooks.battle();helpState.challenge=helpHooks.challenge();helpState.scriptName=helpHooks.scriptName();
        QuickHelpOpenHooks h;h.closePopups=[this] { input->ClosePopups(); };h.lookup=[this](auto serial) { return Reference(serial); };h.noModal=[this] { return common.life.currentDialog==0; };h.currentModal=[this] { return common.life.currentDialog; };h.construct=[this](auto type) { return helpWindow->Construct(AllocateHelp(0xd0),type); };h.node=common.node;h.attach=common.attach;h.addDepth=[this](auto token,auto delta) { common.tree.AddDepth(token,delta); };
        h.modal=[this](auto token,auto enabled) { if (!enabled) common.lifetime.RemoveDialog(token);else if (common.input.AddDialog(token) && common.life.dialogCount>1) { const auto prior=common.life.dialogs[common.life.dialogCount-2];common.tree.AddDepth(token,common.node(prior)->depth-common.node(token)->depth-3U); }common.changed(); };
        h.handle=[this](auto token,auto code,auto x,auto y) { return Handle(token,code,x,y); };
        return OpenQuickHelpWindow({helpState.id,0,common.create.collection.root,0,common.create.screenWidth},h,topic,mode,learnType);
    }
    // 같은 기존 클래스에 생성/색/파일·HTML/돌 버튼/그림 효과를 설치한다.
    void EnableHelp(RawHelpHooks supplied) {
        if (helpWindow || !supplied.globals || !supplied.translate || !supplied.config || !supplied.substitute || !supplied.load || !supplied.text || !supplied.storeText || !supplied.typeName || !supplied.type || !supplied.description || !supplied.hitPoints || !supplied.helpFrame || !supplied.lookup || !supplied.nearestColor || !supplied.queue || !supplied.script || !supplied.external || !supplied.learn || !supplied.scripted || !supplied.battle || !supplied.challenge || !supplied.scriptName) throw std::invalid_argument("전용 Help Client 연결 계약 오류");helpHooks=std::move(supplied);
        const auto construct=[this](auto token,auto type,float x,float y,auto flags) { common.factory.Construct(token,type,x,y,flags); }; // 같은 기본 등록 생성자다.
        HtmlBodyGumpHooks body;body.node=[this](auto token) { return Find(helpBodies,token); };body.base=common.node;body.constructBase=construct;body.allocate=[this](auto bytes) { return AllocateHelp(bytes); };body.constructText=[this](auto token,auto width,auto height,auto font,auto color) { titleWidgets->Construct(token,width,height,font,color); };
        body.kill=[this](auto token) { common.lifetime.Kill(token); };body.push=[this](auto token) { common.tree.Push(token); };body.attach=common.attach;body.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };body.styled=[this](auto token,auto text) { titleMarkup->Parse(token,text); };body.scroll=[this](auto token,auto y) { title->RequestScroll(token,y); };body.slider=[this](auto token,auto value) { titleWidgets->SetSlider(token,value); };body.scrollable=[this](auto token,auto value) { titleWidgets->SetScrollable(token,value); };helpBody=std::make_unique<HtmlBodyGump>(bodyState,std::move(body));
        ImageGumpHooks image;image.node=[this](auto token) { return Find(helpImages,token); };BindImageGumpAssets(image,assets);image.position=[this](auto token) { return common.collection.Position(token); };image.attach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };image.draw=[this](auto request) { activeImage.draw(request); };image.color=[this](auto token) { activeImage.color(token); };imageOwners.resize(assets.TypeTable().Types().size());helpImage=std::make_unique<ImageGump>(common.create,common.factory,ImageGumpState{imageOwner,imageOwners,imageShadow},std::move(image));
        QuickHelpWindowHooks window;window.node=[this](auto token) { return Find(helpWindows,token); };window.base=common.node;window.html=[this](auto token) { return Find(helpBodies,token); };window.constructBase=construct;window.allocate=[this](auto bytes) { return AllocateHelp(bytes); };
        window.constructDecoration=[this](auto token,float width,float height) { decoration->Construct(token,width,height); };window.constructBackground=[this](auto token,float width,float height,auto flags,auto kind,auto type,auto frame) { backgroundCreate->Construct(token,width,height,flags,kind,type,frame); };window.constructHtml=[this](auto token) { helpBody->Construct(token); };window.fitButton=[this](auto token) { common.stones.FitToText(token); };
        window.constructButton=[this](auto token,auto width,auto height,std::string_view label,auto unused,auto context) { if (context) throw std::logic_error("Help 돌 버튼의 비영 문맥 토큰");const std::string text(label);common.stones.Construct(token,width,height,text.c_str(),unused); };window.constructInformation=[this](auto token,auto width,auto height,auto font,auto color) { titleWidgets->Construct(token,width,height,font,color); };window.constructPicture=[this](auto token,auto type,auto frame,auto mode) { helpImage->Construct(token,type,frame,mode); };
        window.translate=helpHooks.translate;window.config=helpHooks.config;window.substitute=helpHooks.substitute;window.load=helpHooks.load;window.loadError=[](auto name) { throw std::runtime_error("Quick Help 파일 로드 실패: "+std::string(name)); };window.nearestColor=helpHooks.nearestColor;window.scripted=helpHooks.scripted;window.kill=[this](auto token) { common.lifetime.Kill(token); };window.push=[this](auto token) { common.tree.Push(token); };window.clearBody=[this](auto token) { helpBody->Clear(token); };window.attach=common.attach;window.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };
        window.lines=[this](auto token) { return buffers->Binding(Find(titles,token)->buffer).state->lineCount; };window.render=[this](auto bodyToken,auto type,auto topic,auto pane,auto stream) { RenderHelp(bodyToken,type,topic,pane,stream); };window.helpFrame=[this](auto type) -> std::optional<std::int32_t> { return helpHooks.helpFrame(type); };window.bodyText=[this](auto token,auto text) { helpBody->SetText(token,text); };
        helpState.noPictureA=109;helpState.noPictureB=161; // 실제 PE0054120c/5412dc의 TYPE 초기값이다.
        helpWindow=std::make_unique<QuickHelpWindow>(helpState,popupState,std::move(window));
    }
};
// 처리기가 참조하는 구현 주소를 고정한다.
RawMenuControls::RawMenuControls(RawMenuTree tree,const GameAssets& assets,FontStore& fonts,RawMenuHooks hooks):impl_(std::make_unique<Impl>(std::move(tree),assets,fonts,std::move(hooks))) {}
// 노드는 소유자의 공통 GumpLifetime으로 먼저 삭제한다.
RawMenuControls::~RawMenuControls()=default;
// 실제 생산 자식도 기존 파서/글리프/배경과 같은 현재 문맥을 사용한다.
void RawMenuControls::SetExternalGumps(RawMenuExternalGumps hooks) { SetExternalGumps(0,std::move(hooks)); }
// 빈 소유자 훅은 해당 슬롯만 반납한다. 다른 살아 있는 월드 UI는 유지한다.
void RawMenuControls::SetExternalGumps(std::uint32_t slot,RawMenuExternalGumps hooks) { if (hooks.owns) impl_->external[slot]=std::move(hooks);else impl_->external.erase(slot); }
// 기본 Help가 읽는 노드/문자열과 메뉴 생성기는 현재 공통 상태다.
std::uint32_t RawMenuControls::BaseHandle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { return impl_->BaseHelp(token,event,first,second); }
const TextGump& RawMenuControls::TextBackend() const { return *impl_->costText; } // 같은 한 줄 처리기다.
const StyleText& RawMenuControls::StyleBackend() const { return *impl_->style; } // 같은 조각 소유자다.
void RawMenuControls::MeasureText(std::span<const char> bytes,const std::array<std::uint32_t,8>& context,std::int32_t& width,std::int32_t& height,std::uint32_t limit) const { impl_->metrics.Measure(bytes,DecodeGumpTextContext(context),width,height,std::bit_cast<std::int32_t>(limit)); } // 같은 원본 문맥과 signed 길이다.
const GumpBackgroundCreate& RawMenuControls::BackgroundBackend() const { return *impl_->backgroundCreate; } // 같은 실제 자산이다.
QuickHelpPopupState& RawMenuControls::PopupState() const { return impl_->popupState; } // 현재 팝업 표식 사본을 만들지 않는다.
void RawMenuControls::EnableHelp(RawHelpHooks hooks) { impl_->EnableHelp(std::move(hooks)); } // 같은 실제 처리기를 설치한다.
std::uint32_t RawMenuControls::OpenHelp(std::uint32_t topic,std::int32_t mode,std::uint32_t learnType) { return impl_->OpenHelp(topic,mode,learnType); } // 원본 전용 재사용 경로다.
bool RawMenuControls::HelpOpen() const { const auto token=impl_->Reference(impl_->helpState.id);return token && (impl_->common.node(token)->flags&1) && impl_->common.node(token)->parent; } // 현재 등록/표시를 직접 읽는다.
void RawMenuControls::CloseHelp() { if (const auto token=impl_->Reference(impl_->helpState.id)) impl_->common.lifetime.Kill(token);impl_->common.lifetime.Cleanup();impl_->common.changed(); } // Session보다 먼저 삭제한다.
// 공통 생성과 분배는 별도 기본 노드 복사본을 만들지 않는다.
GumpCreateNode* RawMenuControls::Node(std::uint32_t token) const { return impl_->Node(token); }
// 스크롤 화살표도 같은 ButtonGump 상태를 사용한다.
StoneButtonGumpNode* RawMenuControls::Button(std::uint32_t token) const { if (auto* node=Find(impl_->helpButtons,token)) return node;return Find(impl_->arrows,token); }
// 메뉴·장식·손잡이의 실제 배경을 반환한다.
GumpBackgroundNode* RawMenuControls::Background(std::uint32_t token) const { return impl_->Background(token); }
// 현재 실제 파생 Attach를 선택한다.
void RawMenuControls::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const { impl_->Attach(token,x,y,parent,mode); }
// 직접 사건을 현재 대상 자신에게만 전달한다.
std::uint32_t RawMenuControls::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { return impl_->Handle(token,event,first,second); }
// 현재 대상의 실제 표시 통지를 실행한다.
void RawMenuControls::Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { impl_->Broadcast(token,event,first,second); }
// 원본 메뉴 확장 프레임을 진행한다.
void RawMenuControls::Idle(std::uint32_t token) const { if (impl_->menus.contains(token)) impl_->menu->Idle(token);else if (impl_->costs.contains(token)) impl_->costGump->Idle(token); }
// 같은 Client에 실제 재그리기 요청을 전달한다.
void RawMenuControls::Redraw(std::uint32_t token,bool erase) const { impl_->Redraw(token,erase); }
// 원본 파서·제목·메뉴가 소유한 자원을 각 소멸자로 반납한다.
void RawMenuControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) const {
    auto& s=*impl_;if (s.helpWindows.contains(token)) { s.helpHistory=QuickHelpHistory{};s.popupState.command=0;s.common.lifetime.DeleteScalar(token,flags); } // 실제4c6d40은 이력/명령만 지우고80바이트 인자는 보존한다.
    else if (s.helpButtons.contains(token)) s.common.stones.DeleteScalar(token,flags);else if (s.menus.contains(token)) s.widgets->DeleteScalar(token,flags);else if (s.titles.contains(token)) s.titleWidgets->DeleteScalar(token,flags);
    else if (s.decorations.contains(token)) s.decoration->DeleteScalar(token,flags);else if (s.costs.contains(token)) s.costGump->DeleteScalar(token,flags);else if (s.sliders.contains(token) || s.thumbs.contains(token) || s.arrows.contains(token)) s.sliderWidgets->DeleteScalar(token,flags);else s.common.lifetime.DeleteScalar(token,flags);
}
// 공통 scalar의 release만 실제 저장소를 지운다.
void RawMenuControls::Release(std::uint32_t token) { auto& s=*impl_;s.helpWindows.erase(token);s.helpBodies.erase(token);s.helpButtons.erase(token);s.helpImages.erase(token);s.helpLinks.erase(token);s.menus.erase(token);s.decorations.erase(token);s.titles.erase(token);s.sliders.erase(token);s.arrows.erase(token);s.thumbs.erase(token);s.backgrounds.erase(token);s.costs.erase(token); }
// controlled 스택의 실제 루트 메뉴를 공통 루트에 부착한다.
void RawMenuControls::Open(ScreenPoint point,const std::function<void(const MenuGumpWidgets&,const MenuGumpControls&,const MenuGump&)>& builder) {
    Close();auto& s=*impl_;if (!builder || s.menuState.index) throw std::invalid_argument("실제 메뉴 builder/controlled 스택 계약 오류");
    builder(*s.widgets,*s.controls,*s.menu);if (s.menuState.index!=1) throw std::logic_error("실제 메뉴 builder 루트 한 칸 필요");s.popup=s.menu->Current();s.menu->AttachCurrent(point.x,point.y,s.common.create.collection.root);s.common.changed();
}
// 원본객체Open이이미부착한같은메뉴를기록한다. 호스트의자동두번째부착은수행하지않는다.
void RawMenuControls::OpenAttached(const std::function<std::uint32_t(const MenuGumpWidgets&,const MenuGumpControls&,const MenuGump&)>& builder,bool closeExisting) {
    if (closeExisting) Close();auto& s=*impl_;if (!builder || s.menuState.index) throw std::invalid_argument("객체메뉴 builder/controlled 스택계약오류");
    const auto root=builder(*s.widgets,*s.controls,*s.menu);if (s.menuState.index) throw std::logic_error("객체메뉴 부착뒤controlled스택잔존");
    if (root && (!s.menus.contains(root) || s.common.node(root)->parent!=s.common.create.collection.root)) throw std::logic_error("객체메뉴 실제루트부착필요");
    s.popup=root;s.common.changed();
}
// 원본의 sticky 팝업 삭제를 같은 수명 경로로 실행한다.
void RawMenuControls::Close() { auto& s=*impl_;s.input->ClosePopups();s.common.lifetime.Cleanup();s.common.changed(); }
// 실제 지식 조회 뒤 첫 controlled 생성 직전에만 기존 팝업을 정리한다. 카드별 생성은 같은 원본 처리기다.
bool RawMenuControls::OpenKnowledge(RawCampaignSession& session,std::function<std::string(std::string_view)> translate,std::uint32_t language) {
    if (!translate) throw std::invalid_argument("지식 창 실제 번역 경계 누락");auto& s=*impl_;if (!session.PlayerKnown() || session.Core().ownerMode.challenge) return false;
    KnowledgeMenuState state;state.width=s.common.create.screenWidth;state.height=s.common.create.screenHeight;state.language=language;std::size_t created{};
    KnowledgeMenuHooks hooks;hooks.translate=translate;hooks.description=[&](std::uint32_t type) { return translate(session.Core().types[type].description); };
    hooks.begin=[&] { if (!created++) Close();return s.widgets->BeginVertical(0); };hooks.node=[&](std::uint32_t token)->MenuGumpNode& { const auto node=Find(s.menus,token);if (!node) throw std::logic_error("지식 카드 현재 노드 없음");return *node; };
    hooks.title=[&](std::string_view text) { s.controls->AddTitle(text); };hooks.separator=[&] { s.controls->AddSeparator(); };
    hooks.item=[&](std::string_view text,std::uint32_t command,KnowledgeParameter parameter,bool enabled) {
        std::uint32_t value{};
        if (std::holds_alternative<std::uint32_t>(parameter)) value=std::get<std::uint32_t>(parameter);
        else { if (std::get<std::string>(parameter)!="themeHelp") throw std::logic_error("지식 창 정적 도움말 문자열 계약");value=0x61000001U; } // 클론 호스트의 정적 themeHelp 수명 토큰이다. 원본 주소를 역참조하지 않는다.
        s.controls->AddText(text,command,value,true,enabled,false);
    };
    hooks.attach=[&](std::int32_t x,std::int32_t y) { s.menu->AttachCurrent(x,y,s.common.create.collection.root); };
    hooks.push=[&](std::uint32_t token) { s.common.tree.Push(token); };hooks.pop=[&](std::uint32_t token) { const auto node=s.common.node(token);s.common.attach(token,node->x,node->y,0,0); };
    hooks.width=[&](std::uint32_t token,float value) { s.menu->SetWidth(token,value); };hooks.height=[&](std::uint32_t token,float value) { s.menu->SetHeight(token,value); };
    hooks.position=[&](std::uint32_t token) { return s.common.collection.Position(token); };hooks.mark=[&](std::uint32_t command) { s.popupState.Mark(command); };
    auto tokens=session.ShowKnowledge(state,std::move(hooks));if (s.menuState.index) throw std::logic_error("지식 창 작성 뒤 controlled 스택 잔존");
    s.knowledge=std::move(tokens);s.popup=s.knowledge.empty() ? 0 : s.knowledge.front();s.common.changed();return !s.knowledge.empty();
}
// 지식 카드는 모두 같은 실제 루트의 팝업이며 다른 팝업이 닫은 뒤 stale 토큰은 열림으로 세지 않는다.
bool RawMenuControls::KnowledgeOpen() const {
    const auto& s=*impl_;
    // 살아 있는 실제 카드 하나라도 표시/부착되어 있으면 같은 창이 열려 있다.
    for (const auto token:s.knowledge) { const auto node=Find(s.menus,token);if (node && (node->flags&1) && !(node->flags&0x10000) && node->parent==s.common.create.collection.root) return true; }return false;
}
// 커서 Take 등의 실행 중 스택이 현재 메뉴를 참조할 수 있으므로 원본의 지연 삭제 시점을 보존한다.
void RawMenuControls::RequestClosePopups() { auto& s=*impl_;s.input->ClosePopups();s.common.changed(); }
// 파일 팔레트의 실제 기본색을 생성 전역과 Draw 전역에 함께 공급한다.
void RawMenuControls::SetPalette(const PaletteColorTable& palette) {
    const auto dim=std::find_if(palette.named.begin(),palette.named.end(),[](const NamedPaletteColor& color) { return color.red==47 && color.green==24 && color.blue==17; }); // 원본005c78d0/CD0055bad4의 실제 목표 RGB다.
    if (dim==palette.named.end()) throw std::invalid_argument("메뉴 원본 비활성/그림자 색 전역 없음");
    auto& s=*impl_;s.palette=palette;s.helpState.color=s.bodyState.color=palette.basic[7];s.widgetState.foreground=palette.basic[7];s.widgetState.shadow=dim->index;s.drawState.foreground=palette.basic[7];s.drawState.disabled=dim->index;SetGumpBackgroundColors(s.backgroundState,palette);
}
// 현재 노드를 원본 수집·정렬 순서로 같은8비트 표면에 출력한다.
void RawMenuControls::Draw(std::span<std::uint8_t> pixels,int pitch,int height,const PaletteColorTable& palette,const GumpShadeMaps& maps,std::optional<ScreenRect> clip) {
    auto& s=*impl_;if (s.activeText) throw std::logic_error("실제 메뉴 Draw 재진입 불가");SetPalette(palette);
    const auto region=clip.value_or(ScreenRect{0,0,s.common.create.screenWidth,height}); // 실제 Renderer 변경 영역 또는 독립 전체 표면이다.
    SetGumpDrawClip(s.clip,region);GumpPixelTarget target(pixels,pitch,height,s.clip);GumpTextDraw text(s.fonts,MakeGumpGlyphPixelHooks(target,s.colors),s.assets.Edition());GumpBackgroundPixelTarget bgPixels(target,s.backgroundAssets,maps,s.colors.vfxColors);MarkupPixelTarget picturePixels(target,s.markupAssets,s.colors.vfxColors,s.pictureColors);
    std::unique_ptr<ImageGumpPixelTarget> helpPictures; // 10.78 전용 Help를 설치한 판본에서만 해당 픽셀 처리기를 생성한다.
    if (s.helpImage) { helpPictures=std::make_unique<ImageGumpPixelTarget>(target,s.assets,s.pictureColors,[](auto) -> ColorMap { throw std::logic_error("Help 모드8 이외 소유자 색표 미연결"); });s.activeImage=helpPictures->Hooks({}); }
    s.activeText=&text;s.activeBackground=bgPixels.Hooks(s.clip);s.activePicture=picturePixels.Hooks({});GumpDrawHooks draw;
    // 예외 반환에서도 현재 표면을 참조한 일시 훅을 파기해 다음 출력에 댕글링 참조를 남기지 않는다.
    struct ResetDraw {
        Impl& state; // 현재 출력 동안만 연결된 실제 픽셀 문맥이다.
        ~ResetDraw() { state.activeText=nullptr;state.activeBackground={};state.activePicture={};state.activeImage={}; } // 정상/예외 반환의 같은 정리다.
    } reset{s};
    draw.draw=[&](auto token) {
        if (s.helpButtons.contains(token)) s.common.stones.Draw(token,s.common.collection,text,target);else if (s.helpImages.contains(token)) s.helpImage->Draw(token);else if (const auto* external=s.External(token)) external->draw(token,target,s.colors,*s.backgroundDraw);else if (s.costs.contains(token)) s.costText->Draw(token);else if (s.decorations.contains(token)) s.decoration->Draw(token);else if (s.titles.contains(token)) s.title->Draw(token);else if (s.arrows.contains(token)) s.sliderWidgets->DrawArrow(token,s.common.collection,text,target);
        else if (s.thumbs.contains(token)) s.sliderWidgets->DrawThumb(token,s.common.collection,*s.backgroundDraw);else if (s.Background(token)) s.backgroundDraw->Draw(token);
    };
    MenuGumpDrawHooks mh;mh.node=[&](auto token) { return Find(s.menus,token); };mh.base=s.common.node;GumpDecorationPixelTarget decorationPixels(target,s.assets);BindMenuGumpDrawAssets(mh,s.assets,decorationPixels);
    MenuGumpDraw menuDraw(s.common.create,s.clip,s.drawState,s.common.collection,*s.backgroundDraw,text,*s.style,*s.layout,std::move(mh));BindMenuGumpDraw(draw,[&](auto token)->const MenuGumpDraw* { return s.menus.contains(token) ? &menuDraw : nullptr; });GumpDraw dispatch(s.common.collection,s.clip,std::move(draw));
    std::array<o::RenderCollectionEntry,kDrawSubmissionCapacity> entries{};const auto count=s.common.collection.Collect({0,0,s.common.create.screenWidth,height},entries,kDrawSubmissionCapacity);auto used=std::span(entries).first(static_cast<std::size_t>(count));SortDrawEntries(used,s.assets.Edition());
    // 같은 등록표의 다른 버튼은 Client에서 이미 출력했으므로 메뉴 소유 노드만 제출한다.
    for (const auto& entry:used) if (s.OwnsBranch(entry.token)) dispatch.Draw(entry.token);
}
// 실제 입력/출력과 같은 현재 배치에서 자동 입력 지점을 읽는다.
std::optional<ScreenPoint> RawMenuControls::Point(std::string_view label) const {
    auto& s=*impl_;
    // 저장된 스타일 원문은 별도 라벨 사본으로 치환하지 않는다.
    for (const auto& [token,node]:s.menus) {
        bool visible=true;
        // 실제 항목과 모든 부모의 현재 visible/dead를 함께 검사한다.
        for (auto current=token;current;current=s.common.node(current)->parent) { const auto flags=s.common.node(current)->flags;if (!(flags&1) || (flags&0x10000)) { visible=false;break; } }
        if (!visible) continue;
        // 현재 보이는 메뉴의 원본 항목과 실제 StyleText를 조회한다.
        for (std::uint32_t index=0;index<node->itemCount;++index) {
        const auto& item=node->items[index];std::string value;
        if (item.styled) {
            // 실제 소유 체인의 모든 일반 문자열 조각을 순서대로 읽는다.
            for (auto part=item.styled;part;part=s.fragments.fragments.at(part).next) { const auto& bytes=s.fragments.fragments.at(part).text;value.append(bytes.begin(),std::find(bytes.begin(),bytes.end(),'\0')); }
        } else value=std::string(item.label.data(),std::find(item.label.begin(),item.label.end(),'\0')-item.label.begin());
        if (value==label && (node->flags&1)) {
            const auto p=s.common.collection.Position(token);
            // 가로 ItemPosition의 Y는 하위 메뉴 부착점이므로 실제 행 높이와 열 너비로 누를 곳을 구한다.
            if (node->menuFlags&0x40000U) return ScreenPoint{p.x+static_cast<int>((index+0.5F)*node->itemWidth),p.y+static_cast<int>(node->height/2)};
            const auto offset=s.menu->ItemPosition(token,static_cast<std::int32_t>(index));return ScreenPoint{p.x+static_cast<int>(node->width/2),p.y+static_cast<int>(offset[1]+node->heights[index]/2)};
        }
    }
    }
    return {};
}
// 현재 루트 메뉴의 실제 항목 영역을 보고/구형 라벨 어댑터에 빌려준다.
std::vector<GumpControl> RawMenuControls::Controls() const {
    auto& s=*impl_;std::vector<GumpControl> result;const auto* node=Find(s.menus,s.popup);if (!node) return result;const auto position=s.common.collection.Position(s.popup);
    // 원본 항목 크기/활성 flags를 직접 사용한다.
    for (std::uint32_t index=0;index<node->itemCount;++index) {
        // 가로 메뉴는 한 행의 실제 열 경계이며 ItemPosition의 하위 메뉴 부착 높이를 사용하지 않는다.
        if (node->menuFlags&0x40000U) {
            result.push_back({static_cast<int>(index),{position.x+static_cast<int>(index*node->itemWidth),position.y,position.x+static_cast<int>((index+1)*node->itemWidth),position.y+static_cast<int>(node->height)},true,(node->items[index].flags&0x200)==0,true});
        } else {
            const auto offset=s.menu->ItemPosition(s.popup,static_cast<std::int32_t>(index));const auto top=position.y+static_cast<int>(offset[1]);result.push_back({static_cast<int>(index),{position.x,top,position.x+static_cast<int>(node->width),top+static_cast<int>(node->heights[index])},true,(node->items[index].flags&0x200)==0,true});
        }
    }return result;
}
// 원본 생성/소멸 개수와 실제 현재 표시 여부를 구분해 인스턴스 생존을 반환한다.
bool RawMenuControls::HasMenus() const { return impl_->menuState.instances!=0; }
// 정지나 도움말 모달의 다른 카운터를 대신 읽지 않는다.
std::uint32_t RawMenuControls::MenuInstances() const { return impl_->menuState.instances; }
// 동일 원본 처리기를 통해 실제 비용 표시를 생성한다. 다른 판본을 추정하지 않는다.
std::uint32_t RawMenuControls::ShowSp(std::int32_t cost,std::int32_t x,std::int32_t y,std::uint32_t reference) {
    auto& s=*impl_;if (!s.costGump) throw std::logic_error("현재 판본의 비용 표시 미복원");const auto token=s.costGump->Show(cost,x,y,reference);s.common.changed();return token;
}
// 입력/메뉴 콜백이 반환한 뒤 동일 등록표의 가상 소멸/해제를 수행한다.
void RawMenuControls::CleanupFrame() { impl_->common.lifetime.Cleanup(); }
// 현재 실제 노드와 모든 부모의 visible/dead를 읽어 숨긴 메뉴·삭제 예약 표시를 제외한다.
bool RawMenuControls::HasOverlay() const {
    auto& s=*impl_;
    // 이 계층의 실제 각 노드가 공통 트리에서 표시되는지 조회한다.
    for (const auto token:s.common.create.entries) if (s.Node(token) || s.External(token)!=nullptr) {
        bool visible=true;
        // 숨긴 부모 아래 표시 노드를 출력 대상으로 만들지 않는다.
        for (auto current=token;current;current=s.common.node(current)->parent) { const auto flags=s.common.node(current)->flags;if (!(flags&1) || (flags&0x10000)) { visible=false;break; } }
        if (visible) return true;
    }
    return false;
}
// 원본 트리의 현재 visible/dead/부모 관계를 따라 실제 표시 메뉴의 경계를 읽는다.
bool RawMenuControls::Contains(ScreenPoint point) const {
    auto& s=*impl_;
    // 숨긴 하위 메뉴는 조상까지 확인하고 입력 영역에서 제외한다.
    for (const auto& [token,node]:s.menus) {
        bool visible=true;
        // 현재 노드와 모든 실제 부모의 visible/dead를 읽는다.
        for (auto current=token;current;current=s.common.node(current)->parent) { const auto flags=s.common.node(current)->flags;if (!(flags&1) || (flags&0x10000)) { visible=false;break; } }
        if (visible && GumpControlInput::Contains(s.common.collection.Bounds(token),point)) return true;
    }
    return false;
}
// 같은 실제 저장소의 개수/controlled 깊이를 진단 보고에 기록한다.
std::string RawMenuControls::Report() const {
    auto& s=*impl_;std::ostringstream out;out<<"raw_menus\t"<<s.menuState.instances<<'\t'<<s.menuState.index<<'\t'<<s.menus.size()<<'\t'<<s.titles.size()<<'\t'<<s.fragments.fragments.size()<<'\n';
    // 같은 전용 노드/자식/이력과 실제 링크 토큰을 후속 통합 플레이에서 읽는다.
    for (const auto& [token,node]:s.helpWindows) out<<"raw_help\t"<<token<<'\t'<<node->serial<<'\t'<<node->flags<<'\t'<<node->body<<'\t'<<node->information<<'\t'<<node->picture<<'\t'<<node->backButton<<'\t'<<node->learnButton<<'\t'<<s.helpHistory.Topics().size()<<'\n';
    // 출력 원문은 변경하지 않고 진단 행에서만 줄바꿈/탭을 공백으로 표시한다.
    for (const auto& [token,node]:s.helpBodies) { auto body=std::string(node->body.data());std::replace(body.begin(),body.end(),'\n',' ');std::replace(body.begin(),body.end(),'\t',' ');out<<"raw_help_body\t"<<token<<'\t'<<node->text<<'\t'<<body<<'\n'; }
    // 현재 유효 링크는 NUL 경계 순서로 읽는다. 이전 주제의 뒤쪽 보존 바이트는 새 링크로 보고하지 않는다.
    for (const auto& [body,links]:s.helpLinks) for (std::size_t offset=0;offset<links.cursor;) { const auto text=ResolveHtmlLink(links,links.base+static_cast<std::uint32_t>(offset));out<<"raw_help_link\t"<<body<<'\t'<<links.base+offset<<'\t'<<text<<'\n';offset+=text.size()+1; }
    out<<"raw_menu_input\t"<<s.lastInput[0]<<'\t'<<s.lastInput[1]<<'\t'<<s.lastInput[2]<<'\t'<<s.lastInput[3]<<'\n';
    // 비용 표시도 같은 등록 토큰·원본 현재 필드와 소유 문자열 체인을 보고한다.
    for (const auto& [token,node]:s.costs) out<<"raw_sp_gump\t"<<token<<'\t'<<node->id<<'\t'<<node->parent<<'\t'<<node->flags<<'\t'<<node->x<<'\t'<<node->y<<'\t'<<node->velocity<<'\t'<<node->acceleration<<'\t'<<node->stamp<<'\t'<<node->head<<'\n';
    // 실제 각 메뉴의 현재 배치/선택/확장/링크를 같은 저장소에서 보고한다.
    for (const auto& [token,node]:s.menus) { const auto rect=s.common.collection.Bounds(token);out<<"raw_menu\t"<<token<<'\t'<<node->id<<'\t'<<node->parent<<'\t'<<node->flags<<'\t'<<node->menuFlags<<'\t'<<node->selection<<'\t'<<node->expanding<<'\t'<<rect.left<<'\t'<<rect.top<<'\t'<<rect.right<<'\t'<<rect.bottom<<'\n'; }
    // 객체 정보 메뉴의 실제 Form/내용물과 등록 일련번호도 같은 노드에서 읽는다.
    for (const auto& [token,node]:s.menus) out<<"raw_menu_object\t"<<token<<'\t'<<node->object<<'\t'<<node->form<<'\t'<<node->serial<<'\n';
    // fancy 제목의 현재 원본 줄/조각을 읽으며 표시 기대 문구를 별도로 만들지 않는다.
    for (const auto& [token,node]:s.menus) if (const auto* title=Find(s.titles,node->title)) {
        const auto& buffer=*s.buffers->Binding(title->buffer).state;std::size_t line{};
        // 미완료 끝 줄까지 실제 hardLines의 소유 체인을 읽는다.
        for (auto head:buffer.hardLines) { std::string label;
            // 그림/서식은 출력에 남기고 저장된 문자 조각만 진단에 넣는다.
            for (auto part=head;part;part=s.fragments.fragments.at(part).next) { const auto& bytes=s.fragments.fragments.at(part).text;label.append(bytes.begin(),std::find(bytes.begin(),bytes.end(),'\0')); }
            std::replace(label.begin(),label.end(),'\t',' ');std::replace(label.begin(),label.end(),'\n',' ');if (head) out<<"raw_menu_heading\t"<<token<<'\t'<<node->title<<'\t'<<line<<'\t'<<label<<'\n';++line;
        }
    }
    // 사용자 검사도 실제 현재 StyleText/명령/배치와 모든 부모의 표시 상태만 읽는다.
    for (const auto& [token,node]:s.menus) {
        bool visible=true;
        // 닫히거나 숨겨진 조상 아래 항목을 활성 입력으로 보고하지 않는다.
        for (auto current=token;current;current=s.common.node(current)->parent) { const auto flags=s.common.node(current)->flags;if (!(flags&1) || (flags&0x10000)) { visible=false;break; } }
        // 원본 항목의 실제 명령/인자와 글자 조각을 각각 기록한다.
        for (std::uint32_t index=0;index<node->itemCount;++index) {
            const auto& item=node->items[index];std::string label;
            if (item.styled) {
                // 그림/서식은 픽셀 출력에 남기고 실제 문자열 조각만 자동 검사 라벨로 읽는다.
                for (auto part=item.styled;part;part=s.fragments.fragments.at(part).next) { const auto& bytes=s.fragments.fragments.at(part).text;label.append(bytes.begin(),std::find(bytes.begin(),bytes.end(),'\0')); }
            } else label.assign(item.label.begin(),std::find(item.label.begin(),item.label.end(),'\0'));
            std::replace(label.begin(),label.end(),'\t',' ');std::replace(label.begin(),label.end(),'\n',' ');
            out<<"raw_menu_item\t"<<token<<'\t'<<index<<'\t'<<item.flags<<'\t'<<item.command<<'\t'<<item.parameter<<'\t'<<visible<<'\t'<<label<<'\n';
        }
    }
    return out.str();
}
}
