// 원본 메뉴 계층의 실제 처리기를 단일 Client 등록표·트리·입력·표면에 조립한다.
#include "client/RawMenuControls.h"
#include "client/DrawSort.h"
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
    mutable std::array<std::uint32_t,4> lastInput{}; // 실제 파생 가상 입력의 마지막 토큰/사건/좌표를 진단 보고에만 보관한다.
    std::map<std::uint32_t,std::unique_ptr<MenuGumpNode>> menus; // 같은 기본 배경을 가진 실제 메뉴 노드다.
    std::map<std::uint32_t,std::unique_ptr<GumpDecorationNode>> decorations; // 실제 모서리 장식 노드다.
    std::map<std::uint32_t,std::unique_ptr<ScrollingTextGumpNode>> titles; // 실제 버퍼/Slider를 소유하는 제목 노드다.
    std::map<std::uint32_t,std::unique_ptr<ScrollGumpNode>> sliders; // 실제 제목 Slider 노드다.
    std::map<std::uint32_t,std::unique_ptr<ScrollGumpArrowNode>> arrows; // 공통 ButtonGump와 공유하는 화살표다.
    std::map<std::uint32_t,std::unique_ptr<ScrollGumpThumbNode>> thumbs; // 실제 배경 손잡이다.
    std::map<std::uint32_t,std::unique_ptr<GumpBackgroundNode>> backgrounds; // 화살표가 소유하는 실제 배경이다.
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

    // 모든 가상 훅은 실제 호출 때 현재 파생 저장소를 재조회한다.
    Impl(RawMenuTree supplied,const GameAssets& suppliedAssets,FontStore& suppliedFonts,RawMenuHooks suppliedHooks)
        :common(std::move(supplied)),assets(suppliedAssets),fonts(suppliedFonts),hooks(std::move(suppliedHooks)),metrics(fonts,suppliedAssets.Edition()),backgroundAssets(assets),markupAssets(assets) {
        if (common.create.collection.edition!=assets.Edition() || !common.token || !common.node || !common.attach || !common.changed || !hooks.command || !hooks.readInt || !hooks.sound || !hooks.poll || !hooks.cursor)
            throw std::invalid_argument("실제 메뉴 판본/공통 트리/Client 연결 오류");
        const auto& fort=assets.Find("fortGump");const auto backgroundType=static_cast<std::uint32_t>(fort.block+o::kFirstAssetTypeNumber);
        widgetState.backgroundType=backgroundType;widgetState.backgroundFrame=1;widgetState.objectFormType=3;
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
        MarkupHooks ph;BindMarkupAssets(pictureState,ph,markupAssets);ph.color=[] { throw std::logic_error("메뉴 mapped 그림의 실제 전용 치환표 미연결"); };ph.shape=[this](auto request) { activePicture.shape(request); };pictures=std::make_unique<Markup>(fragments,pictureState,std::move(ph),assets.Edition());
        StyleTextMarkupHooks mh;BindStyleTextMarkupAssets(mh,assets);BindStyleTextMarkupPalette(mh,palette);mh.sound=hooks.sound;markup=std::make_unique<StyleTextMarkup>(fragments,*pictures,markupState,std::move(mh),assets.Edition());
        StyleTextHooks sh;BindStyleTextFonts(sh,metrics);BindStyleTextPalette(sh,palette);BindMarkup(sh,*pictures);sh=markup->Hooks(std::move(sh));
        sh.textDraw=[this](auto bytes,auto point,const auto& context) { if (!activeText) throw std::logic_error("메뉴 글자 실제 Draw 문맥 없음");activeText->Draw(bytes,point,DecodeGumpTextContext(context)); };
        style=std::make_unique<StyleText>(fragments,std::move(sh),assets.Edition());StyleTextBufferHooks bh;BindStyleTextBufferFonts(bh,metrics);BindStyleTextBufferMarkup(bh,*pictures);buffers=std::make_unique<StyleTextBufferStore>(fragments,*style,std::move(bh),[](auto) {});
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
        gm.redraw=[this](auto token,auto erase) { Redraw(token,erase); };gm.baseRedraw=[this](auto,bool) { common.changed(); };menu=std::make_unique<MenuGump>(common.create,menuState,common.lifetime,std::move(gm));
        MenuGumpLayoutHooks lh;lh.node=[this](auto token) { return Find(menus,token); };lh.baseNode=common.node;lh.push=[this](auto token) { common.tree.Push(token); };lh.attach=common.attach;BindMenuLayoutText(lh,metrics);BindMenuLayoutStyleText(lh,*style);BindMenuTitleLayout(lh,*title);
        layout=std::make_unique<MenuGumpLayout>(common.create,*menu,std::move(lh));MenuGumpWidgetHooks mw;mw.node=[this](auto token) { return Find(menus,token); };mw.base=common.node;mw.allocate=[this](auto bytes) { return Allocate(bytes,0); };mw.attach=common.attach;
        mw.selectionNotice=[](auto,auto) { throw std::logic_error("메뉴 선택 외부 callback 미연결"); };widgets=std::make_unique<MenuGumpWidgets>(common.create,menuState,widgetState,*backgroundCreate,*decoration,*titleWidgets,*layout,*style,common.lifetime,common.collection,common.tree,std::move(mw));
        MenuGumpControlHooks ch;ch.node=[this](auto token) { return Find(menus,token); };ch.base=common.node;ch.readInt=hooks.readInt;controls=std::make_unique<MenuGumpControls>(common.create,menuState,*menu,*layout,*titleMarkup,std::move(ch));
        MenuGumpInputHooks ih;ih.node=[this](auto token) { return Find(menus,token); };ih.base=common.node;
        ih.broadcast=[this](auto token,auto event,auto first,auto second) { Broadcast(token,event,first,second); };ih.handle=[this](auto token,auto event,auto first,auto second) { return Handle(token,event,first,second); };ih.push=[this](auto token) { common.tree.Push(token); };ih.attach=common.attach;
        ih.redraw=[this](auto token,auto erase) { Redraw(token,erase); };ih.command=hooks.command;ih.sound=hooks.sound;ih.invalidate=[this](auto) { common.changed(); };input=std::make_unique<MenuGumpInput>(common.create,inputState,*menu,common.collection,common.events,common.lifetime,common.tree,std::move(ih));
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
        else throw std::logic_error("원본 메뉴 파생 할당 크기/문맥 오류");return token;
    }
    // 모든 공통 처리는 같은 실제 파생 기본 주소를 받는다.
    GumpCreateNode* Node(std::uint32_t token) const {
        if (auto* n=Find(menus,token)) return n;if (auto* n=Find(decorations,token)) return n;if (auto* n=Find(titles,token)) return n;if (auto* n=Find(sliders,token)) return n;
        if (auto* n=Find(arrows,token)) return n;if (auto* n=Find(thumbs,token)) return n;return Find(backgrounds,token);
    }
    // 화살표 배경처럼 공통 생성자가 소유한 실제 자식도 같은 출력 가지에 포함한다.
    bool OwnsBranch(std::uint32_t token) const {
        // 현재 실제 부모를 따라가며 메뉴 소유 조상을 찾는다.
        for (auto current=token;current;current=common.node(current)->parent) if (Node(current)) return true;return false;
    }
    // 배경을 공유하는 파생 객체만 현재 배경 필드로 연결한다.
    GumpBackgroundNode* Background(std::uint32_t token) const {
        if (auto* n=Find(menus,token)) return n;if (auto* n=Find(decorations,token)) return n;if (auto* n=Find(thumbs,token)) return n;
        if (auto* n=Find(backgrounds,token)) return n;return common.background ? common.background(token) : nullptr;
    }
    // 각 실제 가상 Attach를 현재 분류로 호출한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
        if (menus.contains(token)) widgets->Attach(token,x,y,parent,mode);else if (titles.contains(token)) titleWidgets->Attach(token,x,y,parent,mode);
        else if (sliders.contains(token)) slider->Attach(token,x,y,parent,mode);else if (arrows.contains(token)) common.stones.Attach(token,x,y,parent,mode);else common.tree.Attach(token,x,y,parent,mode);
    }
    // 기존 표시 통지와 원본의 실제 메뉴 재그리기 요청을 유지한다.
    void Redraw(std::uint32_t token,bool erase) const { if (menus.contains(token)) menu->Redraw(token,erase);else if (decorations.contains(token)) decoration->Redraw(token,erase);else common.changed(); }
    // 부모가 전달한 원본 사건을 대상 자신에게 분배한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
        lastInput={token,event,first,second};
        if (menus.contains(token)) return input->Handle(token,event,first,second);if (titles.contains(token)) return titleWidgets->Handle(token,event,first,second);
        if (sliders.contains(token)) return slider->Handle(token,event,first,second);if (thumbs.contains(token)) return slider->ThumbHandle(token,event,first,second);return 0;
    }
    // 공통 Frame의 현재 마우스 좌표와 실제 파생 통지를 사용한다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
        if (menus.contains(token)) input->Broadcast(token,event,first,second);else if (titles.contains(token)) titleWidgets->Broadcast(token,event,first,second);else if (thumbs.contains(token)) slider->ThumbBroadcast(token,event,first,second);
    }
};
// 처리기가 참조하는 구현 주소를 고정한다.
RawMenuControls::RawMenuControls(RawMenuTree tree,const GameAssets& assets,FontStore& fonts,RawMenuHooks hooks):impl_(std::make_unique<Impl>(std::move(tree),assets,fonts,std::move(hooks))) {}
// 노드는 소유자의 공통 GumpLifetime으로 먼저 삭제한다.
RawMenuControls::~RawMenuControls()=default;
// 공통 생성과 분배는 별도 기본 노드 복사본을 만들지 않는다.
GumpCreateNode* RawMenuControls::Node(std::uint32_t token) const { return impl_->Node(token); }
// 스크롤 화살표도 같은 ButtonGump 상태를 사용한다.
StoneButtonGumpNode* RawMenuControls::Button(std::uint32_t token) const { return Find(impl_->arrows,token); }
// 메뉴·장식·손잡이의 실제 배경을 반환한다.
GumpBackgroundNode* RawMenuControls::Background(std::uint32_t token) const { return impl_->Background(token); }
// 현재 실제 파생 Attach를 선택한다.
void RawMenuControls::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const { impl_->Attach(token,x,y,parent,mode); }
// 직접 사건을 현재 대상 자신에게만 전달한다.
std::uint32_t RawMenuControls::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { return impl_->Handle(token,event,first,second); }
// 현재 대상의 실제 표시 통지를 실행한다.
void RawMenuControls::Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { impl_->Broadcast(token,event,first,second); }
// 원본 메뉴 확장 프레임을 진행한다.
void RawMenuControls::Idle(std::uint32_t token) const { if (impl_->menus.contains(token)) impl_->menu->Idle(token); }
// 같은 Client에 실제 재그리기 요청을 전달한다.
void RawMenuControls::Redraw(std::uint32_t token,bool erase) const { impl_->Redraw(token,erase); }
// 원본 파서·제목·메뉴가 소유한 자원을 각 소멸자로 반납한다.
void RawMenuControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) const {
    auto& s=*impl_;if (s.menus.contains(token)) s.widgets->DeleteScalar(token,flags);else if (s.titles.contains(token)) s.titleWidgets->DeleteScalar(token,flags);
    else if (s.decorations.contains(token)) s.decoration->DeleteScalar(token,flags);else if (s.sliders.contains(token) || s.thumbs.contains(token) || s.arrows.contains(token)) s.sliderWidgets->DeleteScalar(token,flags);else s.common.lifetime.DeleteScalar(token,flags);
}
// 공통 scalar의 release만 실제 저장소를 지운다.
void RawMenuControls::Release(std::uint32_t token) { auto& s=*impl_;s.menus.erase(token);s.decorations.erase(token);s.titles.erase(token);s.sliders.erase(token);s.arrows.erase(token);s.thumbs.erase(token);s.backgrounds.erase(token); }
// controlled 스택의 실제 루트 메뉴를 공통 루트에 부착한다.
void RawMenuControls::Open(ScreenPoint point,const std::function<void(const MenuGumpWidgets&,const MenuGumpControls&,const MenuGump&)>& builder) {
    Close();auto& s=*impl_;if (!builder || s.menuState.index) throw std::invalid_argument("실제 메뉴 builder/controlled 스택 계약 오류");
    builder(*s.widgets,*s.controls,*s.menu);if (s.menuState.index!=1) throw std::logic_error("실제 메뉴 builder 루트 한 칸 필요");s.popup=s.menu->Current();s.menu->AttachCurrent(point.x,point.y,s.common.create.collection.root);s.common.changed();
}
// 원본의 sticky 팝업 삭제를 같은 수명 경로로 실행한다.
void RawMenuControls::Close() { auto& s=*impl_;s.input->ClosePopups();s.common.lifetime.Cleanup();s.common.changed(); }
// 커서 Take 등의 실행 중 스택이 현재 메뉴를 참조할 수 있으므로 원본의 지연 삭제 시점을 보존한다.
void RawMenuControls::RequestClosePopups() { auto& s=*impl_;s.input->ClosePopups();s.common.changed(); }
// 파일 팔레트의 실제 기본색을 생성 전역과 Draw 전역에 함께 공급한다.
void RawMenuControls::SetPalette(const PaletteColorTable& palette) {
    const auto dim=std::find_if(palette.named.begin(),palette.named.end(),[](const NamedPaletteColor& color) { return color.red==47 && color.green==24 && color.blue==17; }); // 원본005c78d0/CD0055bad4의 실제 목표 RGB다.
    if (dim==palette.named.end()) throw std::invalid_argument("메뉴 원본 비활성/그림자 색 전역 없음");
    auto& s=*impl_;s.palette=palette;s.widgetState.foreground=palette.basic[7];s.widgetState.shadow=dim->index;s.drawState.foreground=palette.basic[7];s.drawState.disabled=dim->index;SetGumpBackgroundColors(s.backgroundState,palette);
}
// 현재 노드를 원본 수집·정렬 순서로 같은8비트 표면에 출력한다.
void RawMenuControls::Draw(std::span<std::uint8_t> pixels,int pitch,int height,const PaletteColorTable& palette,const GumpShadeMaps& maps) {
    auto& s=*impl_;if (s.activeText) throw std::logic_error("실제 메뉴 Draw 재진입 불가");SetPalette(palette);
    SetGumpDrawClip(s.clip,{0,0,s.common.create.screenWidth,height});GumpPixelTarget target(pixels,pitch,height,s.clip);GumpTextDraw text(s.fonts,MakeGumpGlyphPixelHooks(target,s.colors),s.assets.Edition());GumpBackgroundPixelTarget bgPixels(target,s.backgroundAssets,maps,s.colors.vfxColors);MarkupPixelTarget picturePixels(target,s.markupAssets,s.colors.vfxColors,s.pictureColors);
    s.activeText=&text;s.activeBackground=bgPixels.Hooks(s.clip);s.activePicture=picturePixels.Hooks({});GumpDrawHooks draw;
    // 예외 반환에서도 현재 표면을 참조한 일시 훅을 파기해 다음 출력에 댕글링 참조를 남기지 않는다.
    struct ResetDraw {
        Impl& state; // 현재 출력 동안만 연결된 실제 픽셀 문맥이다.
        ~ResetDraw() { state.activeText=nullptr;state.activeBackground={};state.activePicture={}; } // 정상/예외 반환의 같은 정리다.
    } reset{s};
    draw.draw=[&](auto token) {
        if (s.decorations.contains(token)) s.decoration->Draw(token);else if (s.titles.contains(token)) s.title->Draw(token);else if (s.arrows.contains(token)) s.sliderWidgets->DrawArrow(token,s.common.collection,text,target);
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
    for (const auto& [token,node]:s.menus) for (std::uint32_t index=0;index<node->itemCount;++index) {
        const auto& item=node->items[index];std::string value;
        if (item.styled) {
            // 실제 소유 체인의 모든 일반 문자열 조각을 순서대로 읽는다.
            for (auto part=item.styled;part;part=s.fragments.fragments.at(part).next) { const auto& bytes=s.fragments.fragments.at(part).text;value.append(bytes.begin(),std::find(bytes.begin(),bytes.end(),'\0')); }
        } else value=std::string(item.label.data(),std::find(item.label.begin(),item.label.end(),'\0')-item.label.begin());
        if (value==label && (node->flags&1)) { const auto p=s.common.collection.Position(token);const auto offset=s.menu->ItemPosition(token,static_cast<std::int32_t>(index));return ScreenPoint{p.x+static_cast<int>(node->width/2),p.y+static_cast<int>(offset[1]+node->heights[index]/2)}; }
    }
    return {};
}
// 현재 루트 메뉴의 실제 항목 영역을 보고/구형 라벨 어댑터에 빌려준다.
std::vector<GumpControl> RawMenuControls::Controls() const {
    auto& s=*impl_;std::vector<GumpControl> result;const auto* node=Find(s.menus,s.popup);if (!node) return result;const auto position=s.common.collection.Position(s.popup);
    // 원본 항목 크기/활성 flags를 직접 사용한다.
    for (std::uint32_t index=0;index<node->itemCount;++index) { const auto offset=s.menu->ItemPosition(s.popup,static_cast<std::int32_t>(index));const auto top=position.y+static_cast<int>(offset[1]);result.push_back({static_cast<int>(index),{position.x,top,position.x+static_cast<int>(node->width),top+static_cast<int>(node->heights[index])},true,(node->items[index].flags&0x200)==0,true}); }return result;
}
// 원본 생성/소멸 개수와 실제 현재 표시 여부를 구분해 인스턴스 생존을 반환한다.
bool RawMenuControls::HasMenus() const { return impl_->menuState.instances!=0; }
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
    out<<"raw_menu_input\t"<<s.lastInput[0]<<'\t'<<s.lastInput[1]<<'\t'<<s.lastInput[2]<<'\t'<<s.lastInput[3]<<'\n';
    // 실제 각 메뉴의 현재 배치/선택/확장/링크를 같은 저장소에서 보고한다.
    for (const auto& [token,node]:s.menus) { const auto rect=s.common.collection.Bounds(token);out<<"raw_menu\t"<<token<<'\t'<<node->id<<'\t'<<node->parent<<'\t'<<node->flags<<'\t'<<node->menuFlags<<'\t'<<node->selection<<'\t'<<node->expanding<<'\t'<<rect.left<<'\t'<<rect.top<<'\t'<<rect.right<<'\t'<<rect.bottom<<'\n'; }
    return out.str();
}
}
