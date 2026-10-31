// 원본 메뉴 계층의 실제 처리기를 단일 Client 등록표·트리·입력·표면에 조립한다.
#include "client/RawMenuControls.h"
#include "client/HelpLinkPolicy.h" //원래 토큰/접두 비교를 Help와 전체 HTML 링크가 함께 사용한다.
#include "client/TraceChat.h" // 원문 진단 전달도 같은 실제 Chat 공장을 빌린다.
#include "client/TellFileMenus.h"
#include "client/DrawSort.h"
#include "client/GumpFormBinding.h"
#include "client/FloatingSpGump.h"
#include "client/RawKnowledgeMenu.h"
#include "client/RawCampaignSession.h"
#include "client/QuickHelpWindow.h"
#include "client/MissionLoadingMessage.h"
#include "client/MissionGuideInput.h"
#include "client/QuickHelpRequest.h"
#include "client/GumpHelp.h"
#include "client/SpriteColorMaps.h"
#include "client/ImageGump.h"
#include "client/MetaImageGump.h"
#include "client/MetaImageList.h"
#include "client/EmbeddedTell.h"
#include "client/EditorIslandWindow.h"
#include "o/RawFortTerritorySave.h"
#include <iostream>
#include "client/ProductionColorMaps.h"
#include "client/RawNsInfoContext.h"
#include "client/ClientGameplayGlobals.h"
#include "client/HudNsInfoCreate.h"
#include "client/RawTextEntryControls.h"
#include "client/RawChatWindowContext.h"
#include "client/RawBackgroundGifControls.h"
#include "client/RawMovieControls.h"
#include "client/RawHtmlWindowContext.h"
#include "client/HtmlWindowFit.h"
#include "client/HtmlWindowMetadata.h"
#include "client/HtmlWindowFactory.h"
#include "client/TellMapMenus.h"
#include "client/ClientDotCommandGlobals.h"
#include "client/TextEntryStorage.h"
#include "client/SaveFortNameDialog.h"
#include "client/EditorFiles.h"
#include <cctype>
#include <cstdio>
#include <set>
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
#include <sstream>
#include <limits>
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
    std::map<std::uint32_t,std::string> helpStrings;std::uint32_t nextHelpString{0x61100000U}; // 정적61000001..5/기본HTML61010000/전용HTML62000000..6fffffff와 겹치지 않는 공통 소유 문자열 영역이다.
    std::optional<RawNsInfoContext> nsContext; // 같은 Client 전역/물리/문자열 수명을 빌리는 현재 생성 문맥이다.
    std::map<std::uint32_t,std::unique_ptr<HudNsInfoNode>> nsRoots;std::map<std::uint32_t,std::unique_ptr<HudPlayerInfoNode>> nsPlayers; // 실제 NSInfo/PlayerInfo 파생 저장소다.
    std::map<std::uint32_t,std::unique_ptr<HudBattleInfoNode>> nsBattles;std::map<std::uint32_t,std::unique_ptr<HudPlayerButtonNode>> nsButtons; // 실제 BattleInfo/특수 버튼이다.
    std::map<std::uint32_t,std::unique_ptr<TextGumpNode>> plainTexts;std::map<std::uint32_t,std::string> nsTranslated; // 같은 실제 한 줄 처리기와 번역 포인터의 현재 바이트다.
    std::unique_ptr<HudNsInfoCreate> nsCreate;std::unique_ptr<HudNsInfo> nsGeometry;bool nsRootAllocation{}; // 같은 Native 생성/일반 부착 몸체와 첫0xd8 할당의 형 문맥이다.
    std::set<std::uint32_t> nsOwned; // 원문 생성 경계가 직접 할당한50노드의 실제 토큰이다. 포인터 대입/부착 전 실패도 소유한다.
    std::unique_ptr<RawTextEntryControls> textEntries; // 같은 공통 수명/클립과 실제 문자 물리를 사용하는 편집 노드다.
    std::map<std::uint32_t,std::unique_ptr<SaveFortNameDialogNode>> saveFortRoots; // 같은 장식/가상 분배가 사용하는 실제0xdc 부모다.
    std::map<std::uint32_t,SaveFortNameDialogContext> saveFortContexts; // 각 부모보다 오래 유지하는 현재 작업본/이름 callback이다.
    std::set<std::uint32_t> saveFortOwned; // 원문 포인터 대입 전 실패도 같은 실제 정리 대상으로 보관한다.
    const SaveFortNameDialogContext* saveFortBuilding{};std::unique_ptr<SaveFortNameDialog> saveFortDialog; // 동기 생성 장치와 장기 원문 부모 처리기다.
    std::optional<RawChatWindowContext> chatContext; // 실제 Client 전역과 선택 장치 수명을 빌린다.
    std::map<std::uint32_t,std::unique_ptr<HudChatNode>> chatRoots;std::set<std::uint32_t> chatOwned; // 실제 root와 기본 생성 전 직접 할당도 소유한다.
    std::unique_ptr<ChatWindow> chatWindow;std::unique_ptr<HudChatGeometry> chatGeometry; // 같은 원문 공장/입력/일반 부착 본체다.
    std::unique_ptr<RawMovieControls> movies; // 공통 등록/입력/idle/삭제가 공유하는 실제 영상 소유자다.
    std::unique_ptr<RawBackgroundGifControls> gifWindows; // 같은 실제 기본 노드/소유 GIF/픽셀 조립이다.
    std::optional<RawHtmlWindowContext> htmlContext;std::map<std::uint32_t,std::unique_ptr<HtmlWindowGumpNode>> htmlRoots; // 같은 Client와 실제8192BYTE/20행 저장소다.
    std::set<std::uint32_t> htmlOwned,htmlConstructed;std::unique_ptr<HtmlWindowGump> htmlWindow;std::unique_ptr<HtmlWindowFit> htmlFit; // 직접 할당/완전 생성 여부와 같은 원문 수명/전체 배치 본체다.
    std::unique_ptr<HtmlWindowMetadata> htmlMetadata; // 현재 실제 부모/메뉴/명령 고리의 원문 메타 본체다.
    std::unique_ptr<HtmlWindowFactory> htmlFactory;std::map<std::uint32_t,std::uint32_t> htmlLinkBases;std::uint32_t nextHtmlLinkBase{0x71000000}; // 실제 부모 링크 주소 별칭은 Help/고리와 겹치지 않으며 반납 뒤 재사용하지 않는다.
    std::int32_t modalMinimumDepth{};std::uint32_t modalLowest{}; //466600/464090의 현재 최소 깊이와 실제 선택 노드다.
    RawMenuTree common; // Client 돌 버튼과 같은 공통 상태/처리기다.
    const GameAssets& assets; // 보호 TYPE/SHP를 읽는 같은 자산 저장소다.
    FontStore& fonts; // Client 글꼴 캐시를 공유한다.
    RawMenuExternalGumps commonButtons; //공통 호스트 돌 버튼과 자식 배경의 실제 가상 Draw이며 외부 HUD 소유 등록과 수명을 구별한다.
    std::map<std::uint32_t,RawMenuExternalGumps> external; // 생산/미니맵의 독립 수명도 같은 실제 표면과 가상 Draw를 공유한다.
    std::map<std::uint32_t,RawMenuExternalGumps> ownedExternal;std::uint32_t nextExternal{}; // 기존 슬롯과 분리한 실제 처리기 수명 토큰이며 반납한 번호를 재사용하지 않는다.
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
    std::map<std::uint32_t,std::unique_ptr<MetaImageGumpNode>> metaImages; // 실제0xa8 파생은 창/목록의 Image를 빌린다.
    std::unique_ptr<MetaImageGump> metaImage;MetaImageGumpHooks activeMetaImage; // 같은 공통 생성/부착/출력과 일시 픽셀 문맥이다.
    ColorMap metaDisabled{}; // 원문00556038과 같은 파일 팔레트에서 만든 실제 비활성 색표다.
    std::map<std::uint32_t,std::unique_ptr<MetaImageListNode>> imageLists; // 같은 실제 등록표의21칸 목록 저장소다.
    std::unique_ptr<MetaImageList> imageList;std::function<void(ScreenRect,std::uint32_t)> activeImageListFill; // 실제 목록 몸체와 현재 선택 체크무늬 표면이다.
    std::set<std::uint32_t> embeddedTells; // 내장 설명 부모만 추적하며 자식은 같은 공통 트리가 소유한다.
    std::map<std::uint32_t,std::unique_ptr<EditorIslandWindowNode>> islandRoots; // 원문 실제 장식 창의 고정 저장소다.
    std::map<std::uint32_t,EditorIslandWindowContext> islandContexts; // 각 창과 같은 수명의 실제 Session 장치 대여다.
    std::map<std::uint32_t,std::unique_ptr<RawImage>> islandImages; // 목록/루트 드래그가 빌린 실제 후보 Image 소유자다.
    std::set<std::uint32_t> islandOwned;std::uint32_t islandReferences{},islandPulseId{}; // 상태 정리용 토큰과 원본593154/5ca930의 장기BSS0이다.
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
        GumpDecorationHooks dh;dh.node=[this](auto token)->GumpDecorationNode* { if (auto* node=Find(islandRoots,token)) return node;if (auto* node=Find(saveFortRoots,token)) return node;if (auto* node=Find(htmlRoots,token)) return node;return Find(decorations,token); };dh.base=common.node;BindGumpDecorationAssets(dh,assets);
        dh.shape=[this](auto type,auto frame,auto point) { activeBackground.shape({type,frame,point,false}); };dh.invalidate=[this](auto rect) { common.Invalidate(rect,0); }; /*원문465e60→499370은 장식 여백을 포함한 XYWH와 flags0을 전달한다.*/
        decoration=std::make_unique<GumpDecoration>(common.create,decorationState,*backgroundCreate,*backgroundDraw,common.collection,common.lifetime,std::move(dh));
        MarkupHooks ph;BindMarkupAssets(pictureState,ph,markupAssets);ph.color=[this] { if (!activePicture.color) throw std::logic_error("메뉴 그림 실제 Draw 색표 문맥 없음");activePicture.color(); };ph.shape=[this](auto request) { activePicture.shape(request); };pictures=std::make_unique<Markup>(fragments,pictureState,std::move(ph),assets.Edition());
        StyleTextMarkupHooks mh;BindStyleTextMarkupAssets(mh,assets);BindStyleTextMarkupPalette(mh,palette);mh.sound=hooks.sound;markup=std::make_unique<StyleTextMarkup>(fragments,*pictures,markupState,std::move(mh),assets.Edition());
        StyleTextHooks sh;BindStyleTextFonts(sh,metrics);BindStyleTextPalette(sh,palette);BindMarkup(sh,*pictures);sh=markup->Hooks(std::move(sh));
        sh.textDraw=[this](auto bytes,auto point,const auto& context) { if (!activeText) throw std::logic_error("메뉴 글자 실제 Draw 문맥 없음");activeText->Draw(bytes,point,DecodeGumpTextContext(context)); };
        style=std::make_unique<StyleText>(fragments,std::move(sh),assets.Edition());StyleTextBufferHooks bh;BindStyleTextBufferFonts(bh,metrics);BindStyleTextBufferMarkup(bh,*pictures);buffers=std::make_unique<StyleTextBufferStore>(fragments,*style,std::move(bh),[](auto) {});
        costText=std::make_unique<TextGump>(common.create,costTextState,common.factory,common.lifetime,common.collection,clip,*style,
            TextGumpHooks{[this](auto token) -> TextGumpNode* { return Find(plainTexts,token) ? Find(plainTexts,token) : Find(costs,token) ? Find(costs,token) : ExternalText(token); },common.node});
        if (assets.Edition()==o::OriginalEdition::Patch1078) {
            FloatingSpGumpHooks fh;fh.node=[this](auto token) { return Find(costs,token); };
            fh.now=[this] { if (!hooks.gameSeconds) throw std::logic_error("비용 표시의 실제 게임 시계 미연결");return hooks.gameSeconds(); };
            fh.allocate=[this](auto bytes) { return Allocate(bytes,3); };fh.constructText=[this](auto token) { costText->Construct(token,0,0,0,0); };
            fh.clearText=[this](auto token) { costText->Clear(token); };fh.fitText=[this](auto token) { costText->Fit(token); };
            fh.styledText=[this](auto token,std::string_view text) { auto& node=*Find(costs,token);style->Parse(node.head,text,node.textContext); };
            fh.idle=[this](auto token,bool enabled) { common.lifetime.SetIdle(token,enabled); };fh.redraw=[this](auto token,bool erase) { Redraw(token,erase); };
            fh.kill=[this](auto token) { common.lifetime.Kill(token); };fh.attach=common.attach;
            fh.reference=[](auto,auto) { throw std::logic_error("비용 표시00464540의 실제 객체 참조 미연결"); };
            fh.deleteText=[this](auto token,auto flags) { return costText->DeleteScalar(token,flags); };costGump=std::make_unique<FloatingSpGump>(common.create,std::move(fh));
            MetaImageGumpHooks meta;meta.node=[this](auto token) { return Find(metaImages,token); };meta.attach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };
            meta.position=[this](auto token) { return common.collection.Position(token); };meta.color=[this](auto color) { activeMetaImage.color(color); };meta.draw=[this](auto request) { activeMetaImage.draw(request); };
            metaImage=std::make_unique<MetaImageGump>(common.create,common.factory,std::move(meta));metaDisabled=BuildProductionColorMaps(picturePalette).disabled;
            MetaImageListHooks list;list.node=[this](auto token) { return Find(imageLists,token); };list.image=[this](auto token) { return Find(metaImages,token); };list.createImage=[this](RawImage& image) { return CreateMeta(image); };
            list.attach=common.attach;list.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };list.push=[this](auto token) { common.tree.Push(token); };
            list.kill=[this](auto token) { common.lifetime.Kill(token); };list.destructBase=[this](auto token) { common.lifetime.DestructBase(token); };list.bounds=[this](auto token) { return common.collection.Bounds(token); };
            list.redraw=[this](auto token,bool erase) { Redraw(token,erase); };list.notify=[this](auto token,auto event,auto first,auto second) { static_cast<void>(common.events.Bubble(token,event,first,second)); };
            list.green=[this] { return palette.basic[2]; };list.fill=[this](auto rect,auto color) { activeImageListFill(rect,color); };imageList=std::make_unique<MetaImageList>(common.create,common.factory,std::move(list));
        }
        ScrollGumpHooks sl;sl.node=[this](auto token) { return Find(sliders,token); };sl.thumbNode=[this](auto token) { return Find(thumbs,token); };sl.baseNode=common.node;sl.push=[this](auto token) { common.tree.Push(token); };sl.attach=common.attach;sl.redraw=[this](auto token,bool erase) { Redraw(token,erase); };sl.poll=hooks.poll;
        slider=std::make_unique<ScrollGump>(common.create,sliderState,common.tree,common.collection,common.events,common.input,std::move(sl));
        ScrollGumpWidgetHooks sw;sw.node=[this](auto token) { return Find(sliders,token); };sw.arrow=[this](auto token) { return Find(arrows,token); };sw.thumb=[this](auto token) { return Find(thumbs,token); };sw.base=common.node;
        sw.allocate=[this](auto bytes) { return Allocate(bytes,2); };sw.attach=common.attach;sliderWidgets=std::make_unique<ScrollGumpWidgets>(common.create,sliderWidgetState,common.factory,common.stones,*backgroundCreate,*slider,common.lifetime,std::move(sw));
        ScrollingTextGumpHooks th;th.node=[this](auto token) { return Find(titles,token); };th.baseNode=common.node;th.redraw=[this](auto token,bool erase) { Redraw(token,erase); };BindScrollingTextBuffers(th,*buffers);BindScrollingTextSlider(th,*slider);
        title=std::make_unique<ScrollingTextGump>(common.create,common.collection,clip,fragments,*style,std::move(th));
        ScrollingTextGumpWidgetHooks tw;tw.node=[this](auto token) { return Find(titles,token); };tw.slider=[this](auto token) { return Find(sliders,token); };tw.base=common.node;
        tw.allocate=[this](auto bytes) { return Allocate(bytes,1); };tw.push=[this](auto token) { common.tree.Push(token); };tw.attach=common.attach;tw.redraw=[this](auto token,bool erase) { Redraw(token,erase); };tw.cursor=hooks.cursor;
        titleWidgets=std::make_unique<ScrollingTextGumpWidgets>(common.create,titleState,common.factory,common.tree,common.lifetime,common.events,common.input,fragments,*buffers,*title,*sliderWidgets,std::move(tw));
        titleMarkup=std::make_unique<ScrollingTextGumpMarkup>(*title,*style,*pictures,*markup,ScrollingTextGumpMarkupHooks{[this](auto token) { return Find(titles,token); },common.node});
        MenuGumpHooks gm;gm.node=[this](auto token) { return Find(menus,token); };gm.baseNode=common.node;gm.push=[this](auto token) { common.tree.Push(token); };gm.pop=[this](auto token) { const auto* node=common.node(token);common.attach(token,node->x,node->y,0,0); };gm.attach=common.attach;
        BindMenuGumpObjectAttach(gm,[this](std::uint32_t token) { return menus.contains(token) ? widgets.get() : nullptr; });
        gm.redraw=[this](auto token,auto erase) { Redraw(token,erase); };gm.baseRedraw=[this](auto token,bool erase) { common.RedrawBase(token,erase); };menu=std::make_unique<MenuGump>(common.create,menuState,common.lifetime,std::move(gm));
        MenuGumpLayoutHooks lh;lh.node=[this](auto token) { return Find(menus,token); };lh.baseNode=common.node;lh.push=[this](auto token) { common.tree.Push(token); };lh.attach=common.attach;BindMenuLayoutText(lh,metrics);BindMenuLayoutStyleText(lh,*style);BindMenuTitleLayout(lh,*title);
        layout=std::make_unique<MenuGumpLayout>(common.create,*menu,std::move(lh));MenuGumpWidgetHooks mw;mw.node=[this](auto token) { return Find(menus,token); };mw.base=common.node;mw.allocate=[this](auto bytes) { return Allocate(bytes,0); };mw.attach=common.attach;
        mw.objectForm=[this](std::uint32_t type,std::uint32_t flags) { return FormHooks().objectForm(type,flags); };
        mw.formOwner=[this](std::uint32_t form,std::uint32_t owner) { FormHooks().formOwner(form,owner); };
        mw.formObject=[this](std::uint32_t form,std::uint32_t object,std::uint32_t flags) { FormHooks().formObject(form,object,flags); };
        mw.selectionNotice=[](auto,auto) { throw std::logic_error("메뉴 선택 외부 callback 미연결"); };widgets=std::make_unique<MenuGumpWidgets>(common.create,menuState,widgetState,*backgroundCreate,*decoration,*titleWidgets,*layout,*style,common.lifetime,common.collection,common.tree,std::move(mw));
        MenuGumpControlHooks ch;ch.node=[this](auto token) { return Find(menus,token); };ch.base=common.node;ch.readInt=hooks.readInt;controls=std::make_unique<MenuGumpControls>(common.create,menuState,*menu,*layout,*titleMarkup,std::move(ch));
        MenuGumpInputHooks ih;ih.node=[this](auto token) { return Find(menus,token); };ih.base=common.node;
        ih.broadcast=[this](auto token,auto event,auto first,auto second) { Broadcast(token,event,first,second); };ih.handle=[this](auto token,auto event,auto first,auto second) { return Handle(token,event,first,second); };ih.push=[this](auto token) { common.tree.Push(token); };ih.attach=common.attach;
        ih.redraw=[this](auto token,auto erase) { Redraw(token,erase); };ih.command=hooks.command;ih.sound=hooks.sound;ih.invalidate=[this](auto rect) { common.Invalidate(rect,0); }; /*원문476e00→499370은 현재 선택 행/compact 전체의 flags0을 전달한다.*/input=std::make_unique<MenuGumpInput>(common.create,inputState,*menu,common.collection,common.events,common.lifetime,common.tree,std::move(ih));
    }
    // 현재 토큰을 소유한 외부 파생을 등록 순서대로 조회하며 다른 UI의 수명을 바꾸지 않는다.
    const RawMenuExternalGumps* External(std::uint32_t token) const {
        if (commonButtons.owns && commonButtons.owns(token)) return &commonButtons; //메인 버튼과 자식 배경도 GIF/메뉴와 같은 전역 깊이 정렬에서 한 번 출력한다.
        // 등록하지 않은 슬롯과 비소유 노드는 그대로 다음 처리기로 넘긴다.
        for (const auto& [slot,externalHooks]:external) if (externalHooks.owns && externalHooks.owns(token)) return &externalHooks;
        for (const auto& [registration,externalHooks]:ownedExternal) if (externalHooks.owns(token)) return &externalHooks; // 살아 있는 각 HUD의 같은 파생만 조회한다.
        return nullptr;
    }
    // 외부 TextGump만 같은 한 줄 글자 처리기에 빌린다.
    TextGumpNode* ExternalText(std::uint32_t token) const {
        // 생산 글자와 다른 파생을 같은 노드로 잘못 분류하지 않는다.
        for (const auto& [slot,externalHooks]:external) if (externalHooks.text) if (auto* node=externalHooks.text(token)) return node;
        for (const auto& [registration,externalHooks]:ownedExternal) if (externalHooks.text) if (auto* node=externalHooks.text(token)) return node; // 새/이전 생산 글자를 동일 StyleText에 공급한다.
        return nullptr;
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
    // Native new의 현재 생성 위치에 따라 같은 크기라도 올바른 실제 파생 저장소를 만든다.
    std::uint32_t AllocateNs(std::size_t bytes) {
        if (bytes==0xd8 && !nsRootAllocation) { const auto token=Allocate(bytes,0);nsOwned.insert(token);return token; }
        if (bytes==0xb8) { const auto token=common.token();backgrounds.emplace(token,std::make_unique<GumpBackgroundNode>());nsOwned.insert(token);return token; }
        if (bytes==0x170) { const auto token=AllocateHelp(bytes);nsOwned.insert(token);return token; }
        const auto token=common.token();
        if (bytes==0xd8 && nsRootAllocation) { nsRootAllocation=false;nsRoots.emplace(token,std::make_unique<HudNsInfoNode>()); }
        else if (bytes==0xbc) nsPlayers.emplace(token,std::make_unique<HudPlayerInfoNode>());
        else if (bytes==0x16c) nsBattles.emplace(token,std::make_unique<HudBattleInfoNode>());
        else if (bytes==0xc8) nsButtons.emplace(token,std::make_unique<HudPlayerButtonNode>());
        else if (bytes==0xa4) plainTexts.emplace(token,std::make_unique<TextGumpNode>());
        else throw std::logic_error("NSInfo 실제 파생 할당 크기/문맥 오류");nsOwned.insert(token);return token;
    }
    // 원문 전역은 실제 고정 소유자를 사용 시점마다 읽으며 배경/색/TYPE는 공통 UI에서 읽는다.
    std::uint32_t NsRead(std::uint32_t address) const {
        auto& value=*nsContext->globals;
        switch (address) {
        case 0x540421:return value.nsKind;case 0x540440:return value.playerKind;case 0x540457:return value.battleKind;
        case 0x5467b8:return value.zone;case 0x540bac:return value.zoneFormat;case 0x593150:return value.current;
        case 0x54db4c:return std::bit_cast<std::uint32_t>(value.serverSelection);case 0x594fc8:return nsContext->storage->ownerMode.challenge;case 0x595344:return nsContext->gameplay->player.playerKnown;
        default:return nsContext->read(address);
        }
    }
    // 원문 세 정적 키와 같은 현재 번역/서버 버퍼/공유 형식 고리를 구별한다.
    std::string NsText(std::uint32_t token,bool original=false) const {
        if (!original) { const auto found=nsTranslated.find(token);if (found!=nsTranslated.end()) return found->second; }
        switch (token) {
        case 0x50c894:return "~w~E%s\nPlayers in Zone %d\n";case 0x50cd3c:return "~r~EBATTLEMASTER";case 0x50cd2c:return "Start Battle";
        case 0x594970: { const auto& value=nsContext->globals->serverName;return {value.data(),static_cast<std::size_t>(std::find(value.begin(),value.end(),'\0')-value.begin())}; }
        default:return std::string(nsContext->text(token));
        }
    }
    // 전체 원문 생성자는 기존 실제 하위 처리기에만 연결한다. 메타맵 전체 후속은 선택 시 거부한다.
    void BindNs(RawNsInfoContext context) {
        if (!context.globals || !context.gameplay || !context.storage || !context.read || !context.translate || !context.storeText || !context.text || !context.constructButton || assets.Edition()!=o::OriginalEdition::Patch1078 || context.storage->Source()!=&assets) throw std::invalid_argument("NSInfo의 실제 Client/자산/문자열 연결 불일치");
        if (nsContext) { if (context.globals!=nsContext->globals || context.gameplay!=nsContext->gameplay || context.storage!=nsContext->storage) throw std::logic_error("NSInfo 실제 소유자 교체 전 정리 필요");return; }
        nsContext=std::move(context);HudNsInfoCreateHooks h;
        h.node=[this](auto token) { return Find(nsRoots,token); };h.player=[this](auto token) { return Find(nsPlayers,token); };h.battle=[this](auto token) { return Find(nsBattles,token); };h.button=[this](auto token) { return Find(nsButtons,token); };h.base=common.node;
        h.textFlags=[this](auto token)->std::uint32_t& { return titles.at(token)->textFlags; };h.buttonFlags=[this](auto token)->std::uint32_t& { return nsButtons.at(token)->buttonFlags; };
        h.read=[this](auto address) { return NsRead(address); };h.allocate=[this](auto bytes) { return AllocateNs(bytes); };h.baseConstruct=[this](auto token,auto kind,float width,float height,auto flags) { common.factory.Construct(token,kind,width,height,flags); };
        h.background=[this](auto token,float width,float height,auto flags,auto padding,auto type,auto frame) { return backgroundCreate->Construct(token,width,height,flags,padding,type,frame); };
        h.scroll=[this](auto token,auto width,auto height,auto font,auto color) { return titleWidgets->Construct(token,width,height,font,color); };h.text=[this](auto token,auto width,auto height,auto font,auto color) { return costText->Construct(token,width,height,font,color); };
        h.buttonBase=[this](auto token,auto width,auto height,auto flags) { nsContext->constructButton(token,width,height,flags); };
        h.stone=[this](auto token,auto width,auto height,auto label,auto color,auto context) { if (context) throw std::logic_error("NSInfo 돌 버튼의 비null 원본 문맥 미연결");const auto value=NsText(label);return common.stones.Construct(token,width,height,value.c_str(),color,nullptr); }; // 이 원문 생성자의 여섯째 인자는 항상 null이다.
        h.scrollable=[this](auto token,auto value) { titleWidgets->SetScrollable(token,value); };h.slider=[this](auto token,auto value) { titleWidgets->SetSlider(token,value); };h.wrap=[this](auto token,auto value) { titleWidgets->SetWrap(token,std::bit_cast<std::int32_t>(value)); };
        h.parseScroll=[this](auto token,auto text) { titleMarkup->Parse(token,NsText(text)); };h.parseText=[this](auto token,auto text) { auto& node=*plainTexts.at(token);style->Parse(node.head,NsText(text),node.textContext); };
        h.fitText=[this](auto token,auto width,auto height) { costText->Fit(token,std::bit_cast<std::int32_t>(width),std::bit_cast<std::int32_t>(height)); };h.fitStone=[this](auto token) { common.stones.FitToText(token); };
        h.attach=common.attach;h.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };
        h.translate=[this](auto token) { nsTranslated[token]=nsContext->translate(NsText(token,true));return token; };h.type=[this](auto type) { if (type>=assets.TypeTable().Types().size()) throw std::out_of_range("NSInfo 실제 TYPE 범위");return 0x100000U+type; }; // 유효한 실제 TYPE0도 비null인 같은 논리 레코드 토큰이다.
        h.serverName=[this] { nsContext->globals->ResolveServerName(nsContext->translate);return 0x594970U; };
        h.formatZone=[this](auto format,auto server,auto zone) { const auto fmt=NsText(format),name=NsText(server);const auto size=std::snprintf(nullptr,0,fmt.c_str(),name.c_str(),zone);if (size<0) throw std::runtime_error("NSInfo 원본 zone 형식 오류");std::string value(static_cast<std::size_t>(size)+1,'\0');std::snprintf(value.data(),value.size(),fmt.c_str(),name.c_str(),zone);value.resize(size);return nsContext->storeText(value); };
        h.assertion=[](auto line) { throw std::logic_error("NSInfo 원본 진단: "+std::to_string(line)); };h.metamapLayout=[](auto,auto) { throw std::logic_error("NSInfo 자식의 전체 메타맵 배치 미연결"); };
        nsCreate=std::make_unique<HudNsInfoCreate>(nsContext->globals->current,std::move(h));HudNsInfoHooks geometry;
        geometry.node=[this](auto token) { return Find(nsRoots,token); };geometry.base=common.node;geometry.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };geometry.push=[this](auto token) { common.tree.Push(token); };geometry.attach=common.attach;
        nsGeometry=std::make_unique<HudNsInfo>(HudNsInfoModeView{[this] { return NsRead(0x594fc8); },[this] { return NsRead(0x595344); }},std::move(geometry));
    }
    // 전용 메타맵 입력은 미복원을 숨기지 않는다. 초기 일반 노드의 기본 Help는 같은 실제 경로다.
    std::uint32_t NsHandle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
        const auto code=event&0x03ffffffU;
        if ((nsRoots.contains(token) && second && (code==titleState.leftTagEvent || code==titleState.rightTagEvent)) || (NsRead(0x594fc8) && NsRead(0x595344))) throw std::logic_error("NSInfo 전용 메타플레이어 입력 미연결");
        return BaseHelp(token,event,first,second);
    }
    #include "client/RawMenuHtml.inc" // 같은 실제 장식/본문/돌 버튼과 부모 수명을 연결한다.
    #include "client/RawMenuChat.inc" // 같은 Impl 저장소에 실제 Chat 공장만 연결한다.
    #include "client/RawMenuSaveFort.inc" // 같은 장식/문자/두 버튼과 실제 작업본으로 Save as를 조립한다.
    #include "client/RawMenuEditorIsland.inc" // 실제 단일 창/후보 갱신/입력과 공통 기하·수명을 연결한다.
    // 동적 Image 생성은 공개 공장과 목록 자식에서 같은 실제 처리기를 사용한다.
    std::uint32_t CreateMeta(RawImage& image) {
        if (!metaImage) throw std::logic_error("MetaImage 실제10.78 공장 미설치");const auto token=common.token();metaImages.emplace(token,std::make_unique<MetaImageGumpNode>());return metaImage->Construct(token,image);
    }
    // 모든 공통 처리는 같은 실제 파생 기본 주소를 받는다.
    GumpCreateNode* Node(std::uint32_t token) const {
        if (auto* node=Find(islandRoots,token)) return node;
        if (auto* node=Find(imageLists,token)) return node;
        if (auto* node=Find(metaImages,token)) return node;
        if (auto* node=Find(saveFortRoots,token)) return node;
        if (auto* node=Find(htmlRoots,token)) return node;
        if (movies) if (auto* node=movies->Node(token)) return node;
        if (gifWindows) if (auto* node=gifWindows->Node(token)) return node;
        if (auto* node=Find(chatRoots,token)) return node;
        if (textEntries) if (auto* node=textEntries->Node(token)) return node;
        if (auto* n=Find(nsRoots,token)) return n;if (auto* n=Find(nsPlayers,token)) return n;if (auto* n=Find(nsBattles,token)) return n;if (auto* n=Find(nsButtons,token)) return n;if (auto* n=Find(plainTexts,token)) return n;
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
        if (auto* node=Find(islandRoots,token)) return node;
        if (auto* node=Find(saveFortRoots,token)) return node;
        if (auto* node=Find(htmlRoots,token)) return node;
        if (textEntries) if (auto* node=textEntries->Node(token)) return node;
        if (auto* n=Find(menus,token)) return n;if (auto* n=Find(decorations,token)) return n;if (auto* n=Find(thumbs,token)) return n;
        if (auto* n=Find(backgrounds,token)) return n;return common.background ? common.background(token) : nullptr;
    }
    // 각 실제 가상 Attach를 현재 분류로 호출한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
        if (islandRoots.contains(token)) { IslandLayout(token).Attach(token,x,y,parent,mode);return; }
        if (imageLists.contains(token)) { imageList->Attach(token,x,y,parent,mode);return; }
        if (metaImages.contains(token)) { metaImage->Attach(token,x,y,parent,mode);return; }
        if (htmlRoots.contains(token)) { if (!htmlContext->read) throw std::logic_error("HTML 실제 현재 뷰/화면 전역 미연결");htmlFit->Attach(token,x,y,parent,mode); }else if (chatRoots.contains(token)) chatWindow->Attach(token,x,y,parent,mode);else if (textEntries && textEntries->Node(token)) textEntries->Attach(token,x,y,parent,mode);else if (nsRoots.contains(token)) nsGeometry->Attach(token,x,y,parent,mode);else if (nsPlayers.contains(token)) nsCreate->AttachPlayer(token,x,y,parent,mode);else if (nsBattles.contains(token)) nsCreate->AttachBattle(token,x,y,parent,mode);else if (helpWindows.contains(token)) helpWindow->Attach(token,x,y,parent,mode);else if (helpBodies.contains(token)) helpBody->Attach(token,x,y,parent,mode);else if (helpButtons.contains(token)) common.stones.Attach(token,x,y,parent,mode);
        else if (helpImages.contains(token)) helpImage->Attach(token,x,y,parent,mode);else if (menus.contains(token)) widgets->Attach(token,x,y,parent,mode);else if (titles.contains(token)) titleWidgets->Attach(token,x,y,parent,mode);
        else if (sliders.contains(token)) slider->Attach(token,x,y,parent,mode);else if (arrows.contains(token)) common.stones.Attach(token,x,y,parent,mode);else common.tree.Attach(token,x,y,parent,mode);
    }
    // 기존 표시 통지와 원본의 실제 메뉴 재그리기 요청을 유지한다.
    void Redraw(std::uint32_t token,bool erase) const { if (menus.contains(token)) menu->Redraw(token,erase);else if (decorations.contains(token) || saveFortRoots.contains(token) || islandRoots.contains(token)) decoration->Redraw(token,erase);else common.RedrawBase(token,erase); }
    // 부모가 전달한 원본 사건을 대상 자신에게 분배한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
        lastInput={token,event,first,second};
        if (islandRoots.contains(token)) return const_cast<Impl*>(this)->HandleIsland(token,{event,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)});
        if (imageLists.contains(token)) return imageList->Handle(token,event,first,second);
        if (saveFortRoots.contains(token)) return saveFortDialog->Handle(token,event,first,second);
        if (htmlRoots.contains(token)) return htmlWindow->Handle(token,event,first,second);
        if (movies && movies->Node(token)) return movies->Handle(token,event);
        if (gifWindows && gifWindows->Node(token)) return gifWindows->Handle(token,event,first,second);
        if (chatRoots.contains(token)) {
            std::optional<std::string_view> text; // 문자열 삽입 사건에서만 같은 현재 포인터를 읽는다.
            if ((event&0x03ffffffU)==ChatRead(0x542b24)) { const auto found=chatContext->globals->strings.find(second);text=found==chatContext->globals->strings.end() ? chatContext->text(second) : std::string_view(found->second); }
            return chatWindow->Handle(token,event,first,second,text);
        }
        if (textEntries && textEntries->Node(token)) return textEntries->Handle(token,event,first,second);
        if (nsRoots.contains(token) || nsPlayers.contains(token) || nsBattles.contains(token)) return NsHandle(token,event,first,second);
        if (helpWindows.contains(token)) return HelpEvent(token,{event,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)});
        if (menus.contains(token)) return input->Handle(token,event,first,second);if (titles.contains(token)) return titleWidgets->Handle(token,event,first,second);
        if (sliders.contains(token)) return slider->Handle(token,event,first,second);if (thumbs.contains(token)) return slider->ThumbHandle(token,event,first,second);return BaseHelp(token,event,first,second);
    }
    // 공통 Frame의 현재 마우스 좌표와 실제 파생 통지를 사용한다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
        if (islandRoots.contains(token)) { IslandLayout(token).Motion(token,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second));return; }
        if (menus.contains(token)) input->Broadcast(token,event,first,second);else if (titles.contains(token)) titleWidgets->Broadcast(token,event,first,second);else if (thumbs.contains(token)) slider->ThumbBroadcast(token,event,first,second);
    }
    #include "client/RawMenuLoading.inc"
    // 전용 창의 serial은 등록 인덱스와 다르다. 죽은 노드는 원본464020처럼 제외한다.
    std::uint32_t Reference(std::uint32_t serial) const {
        if (!serial) return 0;
        // 루트 칸0을 제외한 현재 전체 등록표를 조회한다.
        for (std::size_t index=1;index<common.create.entries.size();++index) if (const auto token=common.create.entries[index]) { const auto* node=common.node(token);if (node && node->serial==serial && !(node->flags&0x10000)) return token; }return 0;
    }
    // 빈 요청/정적 themeHelp/현재 HTML 링크/입력 문자열 고리의 실제 수명을 구별한다.
    std::string HelpText(std::uint32_t token) const {
        if (const auto found=helpStrings.find(token);found!=helpStrings.end()) return found->second;
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
    // 기본 HTML 파서가 같은 실제 본문/링크 저장소를 쓰고 현재 전역/효과를 공유한다.
    void RenderEmbeddedTell(std::uint32_t token,std::string_view source) {
        if (!helpBody || !helpHooks.globals) throw std::logic_error("내장 TELL의 실제 HtmlBody/Client HTML 전역 미연결");
        helpBody->SetHtml(token,source,[this,token](auto input,std::span<char> body,std::span<char> linkBytes) {
            HtmlTextParserState parser;auto effects=helpHooks.html;auto& links=helpLinks.at(token);links.cursor=0;links.capacity=linkBytes.size();links.bytes.assign(linkBytes.begin(),linkBytes.end());
            const auto publish=[&] { const auto count=std::min(parser.output.size(),body.size()-1);std::copy_n(parser.output.begin(),count,body.begin());body[count]=0; }; // 현재 접두부는 외부 HTML 효과와 실패 경계에서도 실제 부모에 보인다.
            const auto command=effects.command;const auto condition=effects.condition;const auto picture=effects.picture;const auto background=effects.background;
            if (command) effects.command=[&](auto& state,auto text) { publish();command(state,text);publish(); };
            if (condition) effects.condition=[&](auto& state,auto text,auto closing) { publish();condition(state,text,closing);publish(); };
            if (picture) effects.picture=[&](auto& state,auto text) { publish();return picture(state,text); };
            if (background) effects.background=[&](auto& state,auto text) { publish();background(state,text);publish(); };
            effects.link=[&](auto&,auto text,auto closing) { publish();links.bytes.assign(linkBytes.begin(),linkBytes.end());const auto result=HtmlLinkText(links,text,closing);std::copy(links.bytes.begin(),links.bytes.end(),linkBytes.begin());return result; };
            try { static_cast<void>(HtmlTextParser(parser,*helpHooks.globals,std::move(effects),assets.Edition()).Parse(input,static_cast<std::int32_t>(body.size()))); }
            catch (...) { publish();throw; } // 파싱 실패 앞의 원문 접두부/링크와 전역 부수 효과를 보존한다.
            publish();
        });
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
        HelpLinkHooks link;link.script=helpHooks.script;link.help=[this](auto topic,auto mode,auto learn) { OpenHelp(helpHooks.storeText(topic),static_cast<std::int32_t>(mode),learn); };
        link.external=[this](auto original,auto,auto) { helpHooks.external(original); };DispatchHelpLink(assets.Edition(),value,link);
    }
    // 466600의 실제 dialog 등록/이전 창 깊이와 같은 목록을 사용한다.
    std::uint32_t OpenHelp(std::uint32_t topic,std::int32_t mode,std::uint32_t learnType) {
        if (!helpWindow) throw std::logic_error("전용 Help Client 경계 미설치");bodyState.screenWidth=common.create.screenWidth;bodyState.screenHeight=common.create.screenHeight;helpState.battle=helpHooks.battle();helpState.challenge=helpHooks.challenge();helpState.scriptName=helpHooks.scriptName();
        QuickHelpOpenHooks h;h.closePopups=[this] { input->ClosePopups(); };h.lookup=[this](auto serial) { return Reference(serial); };h.noModal=[this] { return common.life.currentDialog==0; };h.currentModal=[this] { return common.life.currentDialog; };h.construct=[this](auto type) { return helpWindow->Construct(AllocateHelp(0xd0),type); };h.node=common.node;h.attach=common.attach;h.addDepth=[this](auto token,auto delta) { common.tree.AddDepth(token,delta); };
        h.modal=[this](auto token,auto enabled) { SetModal(token,enabled); };
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
RawMenuControls::~RawMenuControls() {
    CloseEditorIslandWindows();CloseEmbeddedTells();CloseSaveFortNameDialogs();CloseMovies();CloseHtmlWindows();CloseBackgroundGifs();CloseChatWindows();auto& s=*impl_;if (s.chatContext && s.chatContext->globals->uiOwner==&s) { s.buffers->Destroy(0x548758);s.chatContext->globals->uiOwner=nullptr; } // 표시 자식 뒤에 Client 공유 버퍼의 실제 조각을 반납한다.
}
// 실제 생산 자식도 기존 파서/글리프/배경과 같은 현재 문맥을 사용한다.
void RawMenuControls::SetExternalGumps(RawMenuExternalGumps hooks) { SetExternalGumps(0,std::move(hooks)); }
// 고정 공통 버튼의 수명은 이 메뉴 처리기와 같으며 HUD 외부 등록 표에는 추가하지 않는다.
void RawMenuControls::BindCommonButtons(RawMenuExternalGumps hooks) { if (!hooks.owns || !hooks.draw) throw std::invalid_argument("공통 돌 버튼의 실제 소유/표면 Draw 미연결");impl_->commonButtons=std::move(hooks); }
// 빈 소유자 훅은 해당 슬롯만 반납한다. 다른 살아 있는 월드 UI는 유지한다.
void RawMenuControls::SetExternalGumps(std::uint32_t slot,RawMenuExternalGumps hooks) { if (hooks.owns) impl_->external[slot]=std::move(hooks);else impl_->external.erase(slot); }
// 원문 가상 Draw의 호스트 연결 수명만 관리하며 Gump 등록ID나 깊이는 만들지 않는다.
std::uint32_t RawMenuControls::RegisterExternalGumps(RawMenuExternalGumps hooks) {
    auto& s=*impl_;if (!hooks.owns || !hooks.draw) throw std::invalid_argument("외부 Gump의 실제 소유/출력 장치 필요");
    if (s.nextExternal==std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("외부 Gump 출력 소유 토큰 소진");
    const auto registration=s.nextExternal+1;s.ownedExternal.emplace(registration,std::move(hooks));s.nextExternal=registration;return registration;
}
// 파생 처리기 반납은 자신이 받은 등록만 지우며 새 처리기의 출력은 유지한다.
void RawMenuControls::UnregisterExternalGumps(std::uint32_t registration) { impl_->ownedExternal.erase(registration); }
// 기본 Help가 읽는 노드/문자열과 메뉴 생성기는 현재 공통 상태다.
std::uint32_t RawMenuControls::BaseHandle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const { return impl_->BaseHelp(token,event,first,second); }
const TextGump& RawMenuControls::TextBackend() const { return *impl_->costText; } // 같은 한 줄 처리기다.
const StyleText& RawMenuControls::StyleBackend() const { return *impl_->style; } // 같은 조각 소유자다.
// 초기 도움말 문자열도 다른 메뉴/생산 노드와 같은 공통 수명에서 반납한다.
void RawMenuControls::SetHelpText(std::uint32_t token,std::string_view text) {
    auto& s=*impl_;auto* node=s.common.node(token);if (!node) throw std::out_of_range("도움말 실제 노드 없음");s.helpStrings.erase(node->textPointer);node->textPointer=0;
    if (s.nextHelpString==0x61ffffffU) throw std::overflow_error("도움말 문자열 주소 영역 소진");const auto value=++s.nextHelpString;s.helpStrings.emplace(value,text);node->textPointer=value;
}
// 현재 alternate 포인터만 교체하며 다른 기본 필드나 도움말 제목은 보존한다.
void RawMenuControls::SetHelpWord(std::uint32_t token,std::string_view text) {
    auto& s=*impl_;auto* node=s.common.node(token);if (!node) throw std::out_of_range("도움말 키 실제 노드 없음");s.helpStrings.erase(node->alternateTextPointer);node->alternateTextPointer=0;
    if (s.nextHelpString==0x61ffffffU) throw std::overflow_error("도움말 키 주소 영역 소진");const auto value=++s.nextHelpString;s.helpStrings.emplace(value,text);node->alternateTextPointer=value;
}
void RawMenuControls::MeasureText(std::span<const char> bytes,const std::array<std::uint32_t,8>& context,std::int32_t& width,std::int32_t& height,std::uint32_t limit) const { impl_->metrics.Measure(bytes,DecodeGumpTextContext(context),width,height,std::bit_cast<std::int32_t>(limit)); } // 같은 원본 문맥과 signed 길이다.
const GumpBackgroundCreate& RawMenuControls::BackgroundBackend() const { return *impl_->backgroundCreate; } // 같은 실제 자산이다.
QuickHelpPopupState& RawMenuControls::PopupState() const { return impl_->popupState; } // 현재 팝업 표식 사본을 만들지 않는다.
void RawMenuControls::EnableHelp(RawHelpHooks hooks) { impl_->EnableHelp(std::move(hooks)); } // 같은 실제 처리기를 설치한다.
std::uint32_t RawMenuControls::OpenHelp(std::uint32_t topic,std::int32_t mode,std::uint32_t learnType) { return impl_->OpenHelp(topic,mode,learnType); } // 원본 전용 재사용 경로다.
// 같은 현재 transient를 실제 원본 생성/갱신 경로에 전달한다.
std::uint32_t RawMenuControls::ShowMissionLoading(RawHtmlWindowContext context,std::string_view message,std::function<std::string()> cancelText) { return impl_->ShowLoading(std::move(context),message,std::move(cancelText)); }
// 새 화면에서 이전 안내를 현재 serial로 반납하며 다른 메뉴/생산 자식은 건드리지 않는다.
void RawMenuControls::CloseMissionLoading() { auto& s=*impl_;if (const auto token=s.LoadingRoot()) s.common.lifetime.Kill(token);if (s.htmlContext) s.htmlContext->globals->window.transientSerial=0;s.common.changed(); } // 원문4c8dc0의Kill 뒤 현재 안내serial0을 유지한다.
// 자동 검사도 그림과 같은 실제 공통 경계를 읽는다.
std::optional<ScreenRect> RawMenuControls::MissionLoadingBounds() const { const auto& s=*impl_;if (const auto token=s.LoadingRoot()) return s.common.collection.Bounds(token);return {}; }
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
void RawMenuControls::Idle(std::uint32_t token) const { if (impl_->movies && impl_->movies->Node(token)) impl_->movies->Idle(token);else if (impl_->islandRoots.contains(token)) impl_->IdleIsland(token);else if (impl_->htmlRoots.contains(token)) impl_->htmlWindow->Idle(token);else if (impl_->textEntries && impl_->textEntries->Node(token)) impl_->textEntries->Idle(token);else if (impl_->menus.contains(token)) impl_->menu->Idle(token);else if (impl_->costs.contains(token)) impl_->costGump->Idle(token); }
// 같은 Client에 실제 재그리기 요청을 전달한다.
void RawMenuControls::Redraw(std::uint32_t token,bool erase) const { impl_->Redraw(token,erase); }
// 원본 파서·제목·메뉴가 소유한 자원을 각 소멸자로 반납한다.
void RawMenuControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) const {
    if (impl_->islandRoots.contains(token)) { impl_->DestructIsland(token);if (flags&1) { impl_->islandContexts.erase(token);impl_->islandOwned.erase(token);impl_->islandRoots.erase(token); }return; }
    if (impl_->imageLists.contains(token)) { impl_->imageList->Destruct(token);if (flags&1) impl_->imageLists.erase(token);return; }
    auto& s=*impl_;if (s.saveFortRoots.contains(token)) s.decoration->DeleteScalar(token,flags);
    else if (s.htmlRoots.contains(token)) { if (s.htmlConstructed.contains(token)) s.htmlWindow->Destruct(token);else s.common.lifetime.DestructBase(token);if (flags&1) { s.htmlConstructed.erase(token);s.htmlOwned.erase(token);s.htmlLinkBases.erase(token);s.htmlRoots.erase(token); } }
    else if (s.movies && s.movies->Node(token)) s.movies->DeleteScalar(token,flags);else if (s.gifWindows && s.gifWindows->Node(token)) s.gifWindows->DeleteScalar(token,flags);else if (s.chatRoots.contains(token)) { s.chatWindow->Destruct(token);if (flags&1) { s.chatOwned.erase(token);s.chatRoots.erase(token); } } // 원문 scalar 반납은 같은 실제 Chat 저장소만 해제한다.
    else if (s.textEntries && s.textEntries->Node(token)) { s.textEntries->DeleteScalar(token,flags);if (flags&1) s.chatOwned.erase(token); }else if (s.helpWindows.contains(token)) { s.helpHistory=QuickHelpHistory{};s.popupState.command=0;s.common.lifetime.DeleteScalar(token,flags); } // 실제4c6d40은 이력/명령만 지우고80바이트 인자는 보존한다.
    else if (s.helpButtons.contains(token)) s.common.stones.DeleteScalar(token,flags);else if (s.menus.contains(token)) s.widgets->DeleteScalar(token,flags);else if (s.titles.contains(token)) s.titleWidgets->DeleteScalar(token,flags);
    else if (s.decorations.contains(token)) s.decoration->DeleteScalar(token,flags);else if (s.costs.contains(token)) s.costGump->DeleteScalar(token,flags);else if (s.plainTexts.contains(token)) s.costText->DeleteScalar(token,flags);else if (s.sliders.contains(token) || s.thumbs.contains(token) || s.arrows.contains(token)) s.sliderWidgets->DeleteScalar(token,flags);else s.common.lifetime.DeleteScalar(token,flags);
}
// 공통 scalar의 release만 실제 저장소를 지운다.
void RawMenuControls::Release(std::uint32_t token) { auto& s=*impl_;s.islandOwned.erase(token);s.islandContexts.erase(token);s.islandRoots.erase(token);s.embeddedTells.erase(token);s.imageLists.erase(token);s.metaImages.erase(token);s.saveFortOwned.erase(token);s.saveFortContexts.erase(token);s.saveFortRoots.erase(token);s.htmlOwned.erase(token);s.htmlConstructed.erase(token);s.htmlLinkBases.erase(token);s.htmlRoots.erase(token);if (s.gifWindows) s.gifWindows->Release(token);s.chatOwned.erase(token);s.chatRoots.erase(token);if (s.textEntries) s.textEntries->Release(token);s.nsOwned.erase(token);s.nsRoots.erase(token);s.nsPlayers.erase(token);s.nsBattles.erase(token);s.nsButtons.erase(token);s.plainTexts.erase(token);s.helpStrings.erase(token);s.helpWindows.erase(token);s.helpBodies.erase(token);s.helpButtons.erase(token);s.helpImages.erase(token);s.helpLinks.erase(token);s.menus.erase(token);s.decorations.erase(token);s.titles.erase(token);s.sliders.erase(token);s.arrows.erase(token);s.thumbs.erase(token);s.backgrounds.erase(token);s.costs.erase(token); }
// 생성자는 같은 등록표를 사용하고 Image의 소유권은 호출자에게 남긴다.
std::uint32_t RawMenuControls::CreateMetaImage(RawImage& image) {
    return impl_->CreateMeta(image);
}
// 같은 원문 파생 저장소를 빌리며 이미 반납한 토큰에는 nullptr를 반환한다.
MetaImageGumpNode* RawMenuControls::MetaImageNode(std::uint32_t token) const { return Find(impl_->metaImages,token); }
// 현재 판본의 실제 메타 이미지 몸체를 빌린다.
const MetaImageGump& RawMenuControls::MetaImageBackend() const { if (!impl_->metaImage) throw std::logic_error("MetaImage 실제10.78 처리기 없음");return *impl_->metaImage; }
// 원본 목록 생성은 같은 기본 등록표와 공통 단조 토큰을 사용한다.
std::uint32_t RawMenuControls::CreateImageList(std::int32_t columns) {
    auto& s=*impl_;if (!s.imageList) throw std::logic_error("Image 목록 실제10.78 공장 미설치");const auto token=s.common.token();s.imageLists.emplace(token,std::make_unique<MetaImageListNode>());return s.imageList->Construct(token,columns);
}
// 선택과 원문 ID는 같은 실제 목록 필드에서 읽는다.
MetaImageListNode* RawMenuControls::ImageListNode(std::uint32_t token) const { return Find(impl_->imageLists,token); }
// 실제 목록의 원문 몸체를 빌린다.
const MetaImageList& RawMenuControls::ImageListBackend() const { if (!impl_->imageList) throw std::logic_error("Image 목록 실제10.78 처리기 없음");return *impl_->imageList; }
// 내장 설명은 일반 HTML 팝업의 인스턴스/이력/phase와 별도로 같은 공통 등록표에 생성한다.
std::uint32_t RawMenuControls::ShowEmbeddedTell(std::string_view name,o::ConfigInterface& config,std::function<std::optional<std::string>(std::string_view)> mission) {
    auto& s=*impl_;if (!s.helpBody) throw std::logic_error("내장 TELL의 실제 공통 HtmlBody 처리기 미설치");EmbeddedTellHooks h;
    h.missing=[](auto key) { std::clog<<"TELL cannot find \""<<key<<"\".\n"; };
    h.construct=[&] { s.bodyState.screenWidth=s.common.create.screenWidth;s.bodyState.screenHeight=s.common.create.screenHeight;const auto token=s.AllocateHelp(0x40a4);s.embeddedTells.insert(token);return s.helpBody->Construct(token); };
    h.setHtml=[&](auto token,auto body) { const auto bytes=o::EncodeOriginalText(body);s.RenderEmbeddedTell(token,{reinterpret_cast<const char*>(bytes.data()),bytes.size()}); };h.serial=[&](auto token) { return s.common.node(token)->serial; };
    return ShowEmbeddedTellInConfig(name,config,std::move(mission),std::move(h));
}
// 미부착 상태에서도 현재 등록된 실제 부모를 먼저 Kill하고 자식 우선으로 반납한다.
void RawMenuControls::CloseEmbeddedTells() {
    auto& s=*impl_;for (const auto token:s.embeddedTells) { const auto* node=s.common.node(token);if (node && !(node->flags&0x10000U)) s.common.lifetime.Kill(token); } // Kill은 즉시 저장소를 해제하지 않는다.
    s.common.lifetime.Cleanup();
}
// 실제 공장 뒤 원문 일반 입력 부모의 가운데 부착/모달 등록 순서를 수행한다.
std::uint32_t RawMenuControls::ShowEditorIslandWindow(EditorIslandWindowContext context) { auto& s=*impl_;const auto token=s.CreateIsland(std::move(context));s.Attach(token,0,0,s.common.create.collection.root,2);s.SetModal(token,true);return token; }
// 반납 전까지 같은 실제 창의 현재 필드를 빌린다.
const EditorIslandWindowNode* RawMenuControls::EditorIslandWindow(std::uint32_t token) const { return Find(impl_->islandRoots,token); }
// 원본 참조 수는 Kill 즉시0이 되지 않는다.
std::uint32_t RawMenuControls::EditorIslandWindowCount() const { return impl_->islandReferences; }
// 상태 이탈은 창 밖 루트 드래그까지 반납한 뒤 비소유 Image 참조를 해제한다.
void RawMenuControls::CloseEditorIslandWindows() {
    auto& s=*impl_;
    for (std::size_t id=1;id<s.common.create.entries.size();++id) { const auto token=s.common.create.entries[id];if (s.islandOwned.contains(token)) { const auto* node=s.common.node(token);if (node && !(node->flags&0x10000U)) s.common.lifetime.Kill(token); } } // 미부착/부분 생성도 원본 등록표에서 찾는다.
    s.common.lifetime.Cleanup();
    while (!s.islandOwned.empty()) { const auto token=*s.islandOwned.begin();if (std::find(s.common.create.entries.begin(),s.common.create.entries.end(),token)!=s.common.create.entries.end()) throw std::logic_error("섬 추가 정리 뒤 실제 등록 노드 잔류");Release(token); } // 기본 생성 전 할당만 직접 반납한다.
    s.islandImages.clear();s.common.changed();
}
// 전체 Native wrapper를 호출하며 첫0xd8의 파생 종류 문맥은 예외에도 반납한다.
std::uint32_t RawMenuControls::CreateNsInfo(RawNsInfoContext context) {
    auto& s=*impl_;s.BindNs(std::move(context));s.nsRootAllocation=true;
    struct Reset { bool& value;~Reset() { value=false; } }; // Native 게임 전역이 아닌 호스트 할당 문맥이다.
    Reset reset{s.nsRootAllocation};return s.nsCreate->CreateHidden();
}
// 실제 편집 처리기는 같은 공통 장치에 한 번 설치하고 현재 팔레트를 공급한다.
RawTextEntryControls& RawMenuControls::EnableTextEntry(RawTextEntryContext context) {
    auto& s=*impl_;if (!s.textEntries) { s.textEntries=std::make_unique<RawTextEntryControls>(s.common,s.fonts,*s.backgroundCreate,*s.backgroundDraw,*s.slider,*s.sliderWidgets,
        [&s](auto token) { return Find(s.sliders,token); },[&s](auto bytes) { return s.Allocate(bytes,1); },std::move(context));s.textEntries->SetPalette(s.palette); }return *s.textEntries;
}
RawTextEntryControls* RawMenuControls::TextEntryControls() const { return impl_->textEntries.get(); } // 실제 설치 여부만 관측한다.
// 원문 공장을 호출하는 동안만 생성 번역 문맥을 빌리고 각 root에는 실제 장치 수명을 보관한다.
std::uint32_t RawMenuControls::ShowSaveFortNameDialog(SaveFortNameDialogContext context,std::uint32_t fortMode,std::uint32_t canCancel,std::optional<std::string_view> initial) {
    auto& s=*impl_;if (!context.work || !context.editor.now || !context.translate || !context.fortPath || !context.tell || (fortMode && (!context.header || !context.setName)) || (!fortMode && !context.readFort)) throw std::invalid_argument("Save as 실제 작업본/Client/이름 장치 누락");
    EnableTextEntry(context.editor);s.BindSaveFortDialog();if (s.saveFortBuilding) throw std::logic_error("Save as 생성 안의 재귀 생성 미지원");s.saveFortBuilding=&context;
    struct Reset { Impl& state;~Reset() { state.saveFortBuilding=nullptr; } }; // 정상/예외 반환에서 생성 지역 문맥 별칭만 반납한다.
    Reset reset{s};return s.saveFortDialog->Show(fortMode,canCancel,context.currentName,initial);
}
// 송금 숫자 창은 파일 장치를 요구하지 않고 같은 문자/Gump 수명에 전용 확정을 설치한다.
std::uint32_t RawMenuControls::ShowSendMoneyDialog(SaveFortNameDialogContext context,std::uint32_t target) {
    auto& s=*impl_;if (!context.sendMoney || !context.editor.now || !context.translate) throw std::invalid_argument("송금 숫자 창의 실제 장치 누락");
    EnableTextEntry(context.editor);s.BindSaveFortDialog();if (s.saveFortBuilding) throw std::logic_error("숫자 창 생성 안의 재귀 생성 미지원");s.saveFortBuilding=&context;
    struct Reset { Impl& state;~Reset() { state.saveFortBuilding=nullptr; } }; // 생성 성공/실패와 무관하게 지역 문맥 별칭만 반납한다.
    Reset reset{s};return s.saveFortDialog->ShowMoney(target);
}
// 실제 반납 전까지 같은 root/자식 필드를 읽으며 등록 또는 별도 복사본으로 대체하지 않는다.
const SaveFortNameDialogNode* RawMenuControls::SaveFortNameNode(std::uint32_t token) const { return Find(impl_->saveFortRoots,token); }
// Session 장치가 사라지기 전에 부분 생성도 같은 등록표·지연 Kill·자식 우선 소멸로 반납한다.
void RawMenuControls::CloseSaveFortNameDialogs() {
    auto& s=*impl_;
    for (std::size_t id=1;id<s.common.create.entries.size();++id) { const auto token=s.common.create.entries[id];if (s.saveFortOwned.contains(token)) { const auto* node=s.common.node(token);if (node && !(node->flags&0x10000U)) s.common.lifetime.Kill(token); } } // 부착 전 실패한 등록 자식도 함께 정리한다.
    s.common.lifetime.Cleanup();
    while (!s.saveFortOwned.empty()) { const auto token=*s.saveFortOwned.begin();if (std::find(s.common.create.entries.begin(),s.common.create.entries.end(),token)!=s.common.create.entries.end()) throw std::logic_error("Save as 정리 뒤 등록 노드 잔류");Release(token); } // 기본 생성 전 미등록 할당만 직접 반납한다.
}
void RawMenuControls::CloseTextEntries() { if (impl_->textEntries) impl_->textEntries->Close(); } // 같은 공통 Kill/Cleanup이다.
// 현재 실제 Client 파일/설정을 같은 공통 장치의 GIF 공장에 한 번 연결한다.
// 같은 공통 수명에 실제 영상 장치를 처음 한 번만 연결한다.
RawMovieControls& RawMenuControls::EnableMovies(RawMovieContext context) { auto& s=*impl_;if (!s.movies) s.movies=std::make_unique<RawMovieControls>(s.common,std::move(context));return *s.movies; }
// 현재 singleton 소유자와 실제 반납 경계를 조회/실행한다.
RawMovieControls* RawMenuControls::Movies() const { return impl_->movies.get(); }
void RawMenuControls::CloseMovies() { if (impl_->movies) impl_->movies->Close(); } // Gump scalar가 singleton을 지울 때까지 살아 있다.
RawBackgroundGifControls& RawMenuControls::EnableBackgroundGifs(RawBackgroundGifContext context) { auto& s=*impl_;if (!s.gifWindows) s.gifWindows=std::make_unique<RawBackgroundGifControls>(s.common,std::move(context),[&s](auto token,auto event,auto first,auto second) { return s.BaseHelp(token,event,first,second); });return *s.gifWindows; }
std::uint32_t RawMenuControls::CreateHtmlWindow(RawHtmlWindowContext context,std::uint32_t width,std::uint32_t height) { return impl_->CreateHtml(std::move(context),width,height); } // 실제 부모 생성자다.
const HtmlWindowGump* RawMenuControls::HtmlBackend() const { return impl_->htmlWindow.get(); } // 같은 원문 본체다.
std::uint32_t RawMenuControls::ProcessHtmlMetadata(std::uint32_t token,std::span<char> buffer) { if (!impl_->htmlMetadata) throw std::logic_error("HTML 실제 부모 미설치");return impl_->htmlMetadata->Process(token,buffer); } // 현재 물리 버퍼를 복사하지 않는다.
std::uint32_t RawMenuControls::ShowHtmlWindow(RawHtmlWindowContext context,std::string_view source,std::uint32_t suppress,std::uint32_t width,std::uint32_t height) { impl_->BindHtml(std::move(context));return impl_->htmlFactory->Show(source,suppress,width,height); } // 같은 공통 부모/전체 공장만 호출한다.
std::string RawMenuControls::ResolveHtmlWindowLink(std::uint32_t token) const { return impl_->HtmlLink(token); } // 살아 있는 실제 소유 바이트를 복사해 반환한다.
// 원문4a00c0과 같은 Help/명령/외부 링크 분배기를 현재 부모 바이트에 적용한다.
void RawMenuControls::ActivateHtmlWindowLink(std::uint32_t token) { impl_->HelpLink(impl_->HtmlLink(token)); }
// 원본004cc210의 Cheat 자식은 controlled 세로 메뉴가 아닌 직접 생성한 단일 명령 메뉴다.
std::uint32_t RawMenuControls::CreateCheatMenu(const std::function<std::string(std::string_view)>& translate,const std::function<std::uint32_t()>& command) {
    auto& s=*impl_;const auto token=s.Allocate(0xb90,0);if (!token) return 0;
    if (!translate) throw std::logic_error("Cheat 메뉴의 현재 번역 미연결");const auto label=translate("Get 10,000");
    if (!command) throw std::logic_error("Cheat 메뉴의 현재 명령 미연결");const MenuGumpConstructionEntry entry{label,0x10181U,command(),10000U,1U};
    return s.widgets->Construct(token,std::span(&entry,1),0x80000U);
}
void RawMenuControls::CloseHtmlWindows() { impl_->CloseHtml(); } // 실제 공통 수명을 사용한다.
// 현재 부착된 실제 부모만 사용자 선택을 받는 창으로 조회한다.
bool RawMenuControls::HtmlWindowsOpen() const {
    const auto& s=*impl_;for (const auto& [token,node]:s.htmlRoots) if (s.htmlConstructed.contains(token) && node->parent && (node->flags&1U) && !(node->flags&0x10000U)) return true; // 생성만 된 미부착 부모와 Kill된 부모를 표시 상태에 합치지 않는다.
    return false;
}
// 메타 카운터가0으로 돌아간 뒤에도 같은 실제 직계 메뉴를 보고한다.
std::uint32_t RawMenuControls::HtmlMenuColumns() const {
    const auto& s=*impl_;const auto* root=s.common.life.dialogCount ? Find(s.htmlRoots,s.common.life.dialogs[s.common.life.dialogCount-1]) : nullptr;if (!root) return 0;std::uint32_t count{};
    for (auto child=root->child;child;child=s.common.node(child)->next) if (s.menus.contains(child)) ++count; // 같은 부모의 현재 형제 연결만 읽고 하위 팝업은 열로 세지 않는다.
    return count;
}
// 현재 실제 부모에 Kill을 전달하고 같은 입력 드레인 안에서 소멸하지 않는다.
void RawMenuControls::RequestCloseHtmlWindows() {
    auto& s=*impl_;for (const auto& [token,node]:s.htmlRoots) if (s.htmlConstructed.contains(token) && !(node->flags&0x10000U)) s.common.lifetime.Kill(token); // Kill은 저장소 반납과 달라 같은 map 순회를 보존한다.
}
RawBackgroundGifControls* RawMenuControls::BackgroundGifs() const { return impl_->gifWindows.get(); } // 같은 실제 소유자만 조회한다.
void RawMenuControls::CloseBackgroundGifs() { if (impl_->gifWindows) impl_->gifWindows->Close(); } // 실제 공통 Kill/Cleanup이다.
// 전체 실제 Chat 생성자는 네 자식과 공유 버퍼를 원문 순서대로 조립한다.
std::uint32_t RawMenuControls::CreateChatWindow(RawChatWindowContext context) { auto& s=*impl_;s.BindChat(std::move(context),*this);return s.chatWindow->Create(); }
std::uint32_t RawMenuControls::ChatWindowRoot() const { const auto& s=*impl_;if (!s.chatContext) return 0;const auto token=s.chatContext->globals->window.current;const auto* node=Find(s.chatRoots,token);return node && !(node->flags&0x10000U) ? token : 0U; } // 현재 전역을 임의 복구하지 않는다.
//이름/접두를따로캐시하지않고원문이사용한현재Chat과실제입력을호출위치에서읽는다.
bool RawMenuControls::ChatContainsRecipient(std::string_view name) const { const auto& s=*impl_;if (!s.chatContext || !s.chatWindow) throw std::logic_error("현재Chat의대상포함검사장치없음");return s.chatWindow->ContainsRecipient(s.chatContext->globals->window.current,name); }
// 원문0042e410은 현재 Chat을 다시 읽으며 입력 표시 자체와 별도 부모 사건이다.
bool RawMenuControls::OpenChatInput(const InputEvent& event) {
    auto& s=*impl_;return DispatchChatInputKey(event,[&s] { return s.chatGeometry ? s.chatGeometry->InputVisible() : 0U; },
        [&s] { if (s.chatGeometry) s.chatGeometry->SetInputVisible(s.chatContext->globals->window.current,1); },
        [&s] { if (s.chatContext && s.chatContext->globals->window.current) s.common.events.Bubble(s.chatContext->globals->window.current,0x02000088U,0,1); });
}
// Chat 생성 전/정리 후에는 원문 전역0처럼 효과가 없고 요청 소비는 호출자가 수행한다.
void RawMenuControls::ToggleChatInput() { if (impl_->chatGeometry) impl_->chatGeometry->ToggleInputVisible(); }
// 원문 수신 뒤의 표시도 현재 Chat을 재조회하고 같은 실제 부모 체인에 사건을 전달한다.
void RawMenuControls::ShowChatWindow() { auto& s=*impl_;if (s.chatContext && s.chatContext->globals->window.current) s.common.events.Bubble(s.chatContext->globals->window.current,0x02000088U,0,1); }
// 팔레트 교체 전 값을 저장하지 않고 같은 현재 Chat 글자색을 읽는다.
std::uint32_t RawMenuControls::ChatMessageColor() const { return impl_->palette.named.at(33).index; }
const ChatWindow* RawMenuControls::ChatBackend() const { return impl_->chatWindow.get(); } // 실제 설치한 본체다.
const HudChatGeometry* RawMenuControls::ChatGeometryBackend() const { return impl_->chatGeometry.get(); } // 같은 현재 자식 기하다.
void RawMenuControls::AppendChatLog(std::uint32_t token,std::string_view text,std::span<const std::uint32_t> context) { auto& s=*impl_;if (!s.chatWindow) throw std::logic_error("Chat 실제 로그 처리기 없음");s.chatWindow->AppendLog(token,text,context); } // 같은 실제 공유 로그/스타일 파서와 원문42eca0 부모를 사용한다.
// 원문0042f100의 현재 root 관문과 자식 문맥/스크롤 보존은 기존 동일 AppendLog 본체로 실행한다.
void RawMenuControls::PrintChat(std::string_view text) { const auto& s=*impl_;const auto token=s.chatContext ? s.chatContext->globals->window.current : 0U;if (token) AppendChatLog(token,text); }
// 색을 읽은 뒤의 서식 처리와 현재 Chat 포인터 재조회를 공통 로그 부모에 공급한다.
void RawMenuControls::ForwardDiagnosticChat(std::uint32_t enabled,std::string_view format) {
    auto& s=*impl_;TraceChatHooks hooks;hooks.color=[&s] { return s.palette.named.at(9).index; };
    hooks.currentChat=[&s] { return s.chatContext ? s.chatContext->globals->window.current : 0U; };
    hooks.append=[this](auto token,auto text,auto context) { AppendChatLog(token,text,context); };
    ForwardTraceToChat(enabled,format,hooks);
}
std::uint32_t RawMenuControls::InvokeChatCommand(std::uint32_t callback) {
    auto& s=*impl_;if (!s.chatContext) throw std::logic_error("Chat 실제 명령 문맥 없음");
    if (callback==0x42f680U) { if (const auto token=s.chatContext->globals->window.current) { const auto* node=Find(s.chatRoots,token);if (!node) throw std::logic_error("Chat clear 실제 현재 root 없음");s.title->ReplaceText(node->text,std::span("",1),0);s.title->RequestBottom(node->text); }return 0; } // 원문은 완료 입력 줄만 제거하며 미완료 끝 줄을 보존한다.
    if (callback==0x42e520U) { if (!s.chatContext->quit) throw std::logic_error("Chat 실제 종료 사건 예약 미연결");s.chatContext->quit(s.ChatRead(0x542944),0,0);return 0; }throw std::logic_error("Chat 지원하지 않는 원문 callback"); // 현재 전체DWORD 사건을 예약 시점에 읽는다.
}
void RawMenuControls::CloseChatWindows() {
    auto& s=*impl_;
    for (std::size_t id=1;id<s.common.create.entries.size();++id) { const auto token=s.common.create.entries[id];const auto* node=s.chatOwned.contains(token) ? s.common.node(token) : nullptr;if (node && !(node->flags&0x10000U)) s.common.lifetime.Kill(token); } // 실제 미부착/부분 자식도 등록표에서 반납한다.
    s.common.lifetime.Cleanup();
    while (!s.chatOwned.empty()) { const auto token=*s.chatOwned.begin();if (std::find(s.common.create.entries.begin(),s.common.create.entries.end(),token)!=s.common.create.entries.end()) throw std::logic_error("Chat 정리 뒤 등록 객체 잔류");Release(token); } // 기본 생성 전 저장소만 직접 반납한다.
}
// 실제 현재 토큰의 생존 여부만 조회한다. 원문이 보존하는 죽은 current 전역을 임의0으로 쓰지 않는다.
std::uint32_t RawMenuControls::NsInfoRoot() const { const auto& s=*impl_;if (!s.nsContext) return 0;const auto token=s.nsContext->globals->current;const auto* node=Find(s.nsRoots,token);return node && !(node->flags&0x10000U) ? token : 0U; }
// 공통 기본 버튼 처리기는 실제 특수 파생의 동일 기본 주소를 빌린다.
ButtonGumpNode* RawMenuControls::BaseButton(std::uint32_t token) const { if (auto* node=Find(impl_->nsButtons,token)) return node;return Button(token); }
// 미부착/부분 생성도 실제 등록표와 자식 우선 Kill/Cleanup으로만 반납한다.
void RawMenuControls::CloseNsInfo() {
    auto& s=*impl_;for (std::size_t id=1;id<s.common.create.entries.size();++id) { const auto token=s.common.create.entries[id];const auto* node=s.nsOwned.contains(token) ? s.common.node(token) : nullptr;if (node && !(node->flags&0x10000U)) s.common.lifetime.Kill(token); } // 부모에 붙기 전의 실제 자식도 반납하며 죽은 등록 칸을 다시 Kill하지 않는다.
    s.common.lifetime.Cleanup();
    while (!s.nsOwned.empty()) { // new 성공 뒤 기본 생성자 전 번역 실패로 미등록 상태인 저장소만 별도 반납한다.
        const auto token=*s.nsOwned.begin();if (std::find(s.common.create.entries.begin(),s.common.create.entries.end(),token)!=s.common.create.entries.end()) throw std::logic_error("NSInfo 정리 뒤 실제 등록 객체가 남음");Release(token); // 미등록 id0으로 공통 Kill을 호출하지 않는다.
    }
    s.common.changed();
}
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
    const auto information=std::find_if(palette.named.begin(),palette.named.end(),[](const NamedPaletteColor& color) { return color.red==219 && color.green==213 && color.blue==203; }); // 원본005b5dc8과 CD 대응의 실제 기본 설명 색이다.
    if (information==palette.named.end()) throw std::invalid_argument("원본 설명 기본색 전역 없음");
    auto& s=*impl_;s.palette=palette;s.helpState.color=s.bodyState.color=information->index;s.helpState.buttonUnused=palette.named.at(9).index;s.widgetState.foreground=palette.basic[7];s.widgetState.shadow=dim->index;s.drawState.foreground=palette.basic[7];s.drawState.disabled=dim->index;SetGumpBackgroundColors(s.backgroundState,palette);if (s.textEntries) s.textEntries->SetPalette(palette); // 같은 현재 팔레트를 편집 배경/선택에도 공급한다.
}
// 현재 노드를 원본 수집·정렬 순서로 같은8비트 표면에 출력한다.
void RawMenuControls::Draw(std::span<std::uint8_t> pixels,int pitch,int height,const PaletteColorTable& palette,const GumpShadeMaps& maps,std::optional<ScreenRect> clip) {
    auto& s=*impl_;const auto region=clip.value_or(ScreenRect{0,0,s.common.create.screenWidth,height}); //독립 표시/10.72의 기존 전체 수집 계약이다.
    std::array<o::RenderCollectionEntry,kDrawSubmissionCapacity> entries{};const auto count=CollectDrawing({0,0,s.common.create.screenWidth,height},entries,kDrawSubmissionCapacity);
    auto used=std::span(entries).first(static_cast<std::size_t>(count));SortDrawEntries(used,s.assets.Edition());
    WithDrawing(pixels,pitch,height,palette,maps,region,[&](const DrawingToken& draw) { for (const auto& entry:used) draw(entry.token); }); //기존 분리 경로의 정렬/출력 순서를 유지한다.
}
// 별도 UI 목록이나 전체 화면 사본을 만들지 않고 원문의 현재 dirty를 실제 수집기에 전달한다.
std::int32_t RawMenuControls::CollectDrawing(ScreenRect clip,std::span<o::RenderCollectionEntry> output,std::int32_t limit) const { return impl_->common.collection.Collect(clip,output,limit); }
// 모든 종류의 통합 정렬/출력 동안 같은 UI 장치를 한 번만 유지한다. 토큰 함수는 본문 밖에 보관하면 안 된다.
void RawMenuControls::WithDrawing(std::span<std::uint8_t> pixels,int pitch,int height,const PaletteColorTable& palette,const GumpShadeMaps& maps,ScreenRect region,const std::function<void(const DrawingToken&)>& body) {
    auto& s=*impl_;if (s.activeText || !body) throw std::logic_error("실제 메뉴 Draw 재진입/본문 누락");SetPalette(palette);
    SetGumpDrawClip(s.clip,region);GumpPixelTarget target(pixels,pitch,height,s.clip);GumpTextDraw text(s.fonts,MakeGumpGlyphPixelHooks(target,s.colors),s.assets.Edition());GumpBackgroundPixelTarget bgPixels(target,s.backgroundAssets,maps,s.colors.vfxColors);MarkupPixelTarget picturePixels(target,s.markupAssets,s.colors.vfxColors,s.pictureColors);
    std::unique_ptr<ImageGumpPixelTarget> helpPictures; // 10.78 전용 Help를 설치한 판본에서만 해당 픽셀 처리기를 생성한다.
    if (s.helpImage) { helpPictures=std::make_unique<ImageGumpPixelTarget>(target,s.assets,s.pictureColors,[](auto) -> ColorMap { throw std::logic_error("Help 모드8 이외 소유자 색표 미연결"); });s.activeImage=helpPictures->Hooks({}); }
    if (s.metaImage) s.activeMetaImage=BindMetaImagePixels({},target,s.colors.vfxColors,s.metaDisabled);
    if (s.imageList) s.activeImageListFill=[&target](auto rect,auto color) { target.Fill(rect,color,true); };
    s.activeText=&text;s.activeBackground=bgPixels.Hooks(s.clip);s.activePicture=picturePixels.Hooks({});GumpDrawHooks draw;
    // 예외 반환에서도 현재 표면을 참조한 일시 훅을 파기해 다음 출력에 댕글링 참조를 남기지 않는다.
    struct ResetDraw {
        Impl& state; // 현재 출력 동안만 연결된 실제 픽셀 문맥이다.
        ~ResetDraw() { state.activeText=nullptr;state.activeBackground={};state.activePicture={};state.activeImage={};state.activeMetaImage={};state.activeImageListFill={}; } // 정상/예외 반환의 같은 정리다.
    } reset{s};
    draw.draw=[&](auto token) {
        if (s.imageLists.contains(token)) { s.imageList->Draw(token);return; }
        if (s.metaImages.contains(token)) { s.metaImage->Draw(token);return; }
        if (s.movies && s.movies->Node(token)) s.movies->Draw(token,target,s.clip.clip);else if (s.gifWindows && s.gifWindows->Node(token)) s.gifWindows->Draw(token,target);else if (s.textEntries && s.textEntries->Node(token)) s.textEntries->Draw(token,text,target);else if (s.helpButtons.contains(token)) s.common.stones.Draw(token,s.common.collection,text,target);else if (s.helpImages.contains(token)) s.helpImage->Draw(token);else if (const auto* external=s.External(token)) external->draw(token,target,s.colors,*s.backgroundDraw);else if (s.costs.contains(token) || s.plainTexts.contains(token)) s.costText->Draw(token);else if (s.nsButtons.contains(token)) throw std::logic_error("PlayerInfo 전용 버튼의 전체 메타맵 Draw 미연결");else if (s.islandRoots.contains(token) || s.saveFortRoots.contains(token) || s.htmlRoots.contains(token) || s.decorations.contains(token)) s.decoration->Draw(token);else if (s.titles.contains(token)) s.title->Draw(token);else if (s.arrows.contains(token)) s.sliderWidgets->DrawArrow(token,s.common.collection,text,target);
        else if (s.thumbs.contains(token)) s.sliderWidgets->DrawThumb(token,s.common.collection,*s.backgroundDraw);else if (s.Background(token)) s.backgroundDraw->Draw(token);
    };
    MenuGumpDrawHooks mh;mh.node=[&](auto token) { return Find(s.menus,token); };mh.base=s.common.node;GumpDecorationPixelTarget decorationPixels(target,s.assets);BindMenuGumpDrawAssets(mh,s.assets,decorationPixels);
    MenuGumpDraw menuDraw(s.common.create,s.clip,s.drawState,s.common.collection,*s.backgroundDraw,text,*s.style,*s.layout,std::move(mh));BindMenuGumpDraw(draw,[&](auto token)->const MenuGumpDraw* { return s.menus.contains(token) ? &menuDraw : nullptr; });GumpDraw dispatch(s.common.collection,s.clip,std::move(draw));
    const DrawingToken submit=[&](auto token) { if (s.OwnsBranch(token)) dispatch.Draw(token); }; //기존 공통/외부 파생과 빈 기본 Gump의 실제 분배 범위를 유지한다.
    body(submit);
}
// 실제 입력/출력과 같은 현재 배치에서 자동 입력 지점을 읽는다.
std::optional<ScreenPoint> RawMenuControls::Point(std::string_view label) const {
    auto& s=*impl_;
    constexpr std::string_view kMenuCommandPrefix="menu-command:"; // 검사전용명령/인자쌍으로동명이항목의실제누를곳을조회한다.
    std::optional<std::array<std::uint32_t,2>> requested; // 실제항목의두DWORD이며명령을직접실행하거나숨긴항목을열지않는다.
    if (label.starts_with(kMenuCommandPrefix)) {
        std::istringstream fields(std::string(label.substr(kMenuCommandPrefix.size())));std::array<std::uint32_t,2> values{};char comma{}; // 두값은보고서원문과같은16진수다.
        if (!(fields>>std::hex>>values[0]>>comma>>values[1]) || comma!=',' || !(fields>>std::ws).eof()) throw std::invalid_argument("메뉴 항목 조회는 menu-command:명령16진수,인자16진수여야 함");
        requested=values;
    }
    if (const auto* root=s.common.life.dialogCount ? Find(s.saveFortRoots,s.common.life.dialogs[s.common.life.dialogCount-1]) : nullptr) {
        for (const auto token:{root->ok,root->cancel}) { const auto* node=Find(s.helpButtons,token);if (node && std::string_view(node->label.data())==label) { const auto rect=s.common.collection.Bounds(token);return ScreenPoint{(rect.left+rect.right)/2,(rect.top+rect.bottom)/2}; } } // 실제 최상위 모달의 두 버튼만 현재 공통 경계로 읽는다.
    }
    // 가장 위의 실제 모달 HTML 부모에서 돌 버튼의 현재 공통 경계를 읽는다.
    if (const auto* root=s.common.life.dialogCount ? Find(s.htmlRoots,s.common.life.dialogs[s.common.life.dialogCount-1]) : nullptr) {
        for (std::uint32_t index=0;index<root->controlCount;++index) { const auto* node=Find(s.helpButtons,root->controls[index].child);if (node && std::string_view(node->label.data())==label) { const auto rect=s.common.collection.Bounds(root->controls[index].child);return ScreenPoint{(rect.left+rect.right)/2,(rect.top+rect.bottom)/2}; } } // 생성 순서의 실제 controlCount 안에서만 조회한다.
    }
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
        if ((requested ? item.command==(*requested)[0] && item.parameter==(*requested)[1] : value==label) && (node->flags&1)) {
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
    auto& s=*impl_;std::ostringstream out;if (s.movies) out<<s.movies->Report();out<<"raw_menus\t"<<s.menuState.instances<<'\t'<<s.menuState.index<<'\t'<<s.menus.size()<<'\t'<<s.titles.size()<<'\t'<<s.fragments.fragments.size()<<'\n';
    if (s.chatContext) { //실제 표시/편집 버퍼를 읽기만 하며 입력 검증/그리기를 호출하지 않는다.
        out<<"raw_chat_globals\t"<<s.chatContext->globals->window.current<<'\t'<<s.chatContext->globals->inputLineOff<<'\n';
        const auto& chat=*s.chatContext->globals;out<<"raw_chat_log\t"<<chat.log.lineCount<<'\t'<<chat.receivedMask<<'\t'<<chat.ignored.size()<<'\n'; // 실제 공유 로그/마스크만 읽으며 수신이나 서식 파싱을 실행하지 않는다.
        for (std::size_t line=0;line<chat.log.hardLines.size();++line) { // 현재 소유한 원문 명시 줄의 조각을 그대로 보고한다.
            auto token=chat.log.hardLines[line];std::size_t visited{};
            while (token && visited++<s.fragments.fragments.size()) { // 손상 순환이 보고를 멈추지 않게 실제 소유 조각 수로만 제한한다.
                const auto found=s.fragments.fragments.find(token);if (found==s.fragments.fragments.end()) break;const auto& fragment=found->second;
                out<<"raw_chat_fragment\t"<<line<<'\t'<<token<<'\t'<<static_cast<int>(fragment.kind)<<'\t';constexpr std::string_view digits="0123456789abcdef"; // 실제 좁은 문자와 NUL을 같은 원시 hex로 직렬화한다.
                for (const auto value:fragment.text) { const auto byte=static_cast<unsigned char>(value);out<<digits[byte>>4]<<digits[byte&15]; } // 저장 문자열의 실제 전체 바이트만 읽는다.
                for (const auto value:fragment.context) out<<'\t'<<value;out<<'\n';token=fragment.next; // 현재 문맥과 실제 다음 링크를 보존한다.
            }
        }
        const auto& completion=s.chatContext->globals->completion.bytes;std::uint32_t completionIndex{},completionLength{},completionCount{}; //실제장기BSS의원시값을읽으며자동완성을실행하지않는다.
        std::memcpy(&completionIndex,completion.data(),4);std::memcpy(&completionLength,completion.data()+4,4);std::memcpy(&completionCount,completion.data()+0x88,4);out<<"raw_chat_completion\t"<<completionIndex<<'\t'<<completionLength<<'\t'<<completionCount<<'\n';
        for (std::uint32_t index=0;index<std::min(completionCount,128U);++index) { //보고는현재저장된후보만읽으며같은단어의순환상태를바꾸지않는다.
            out<<"raw_chat_completion_candidate\t"<<index<<'\t';constexpr char digits[]="0123456789abcdef"; //후보의원본BYTE표시표다.
            for (std::size_t at=0x90+std::size_t(index)*128;at<completion.size() && completion[at];++at) { const auto byte=completion[at];out<<digits[byte>>4]<<digits[byte&15]; } //번역복사로슬롯을넘는C문자열도실제NUL까지읽는다.
            out<<'\n';
        }
        for (const auto& [token,node]:s.chatRoots) { //부분 생성은 있는 자식/버퍼만 관측한다.
            const auto* console=s.textEntries ? s.textEntries->Console(node->console) : nullptr;out<<"raw_chat_window\t"<<token<<'\t'<<node->flags<<'\t'<<node->parent<<'\t'<<node->console<<'\t'<<(console ? console->flags : 0)<<'\t'<<std::uint32_t(node->helperVisible)<<'\n';
            if (console) if (const auto* buffer=s.textEntries->Storage().Find(console->buffer)) { out<<"raw_chat_input\t"<<token<<'\t';for (const auto value:buffer->Text()) { //제어문자를 TSV 구분자와 혼동하지 않도록 실제 BYTE를16진수로 기록한다.
                constexpr char digits[]="0123456789abcdef";const auto byte=static_cast<unsigned char>(value);out<<digits[byte>>4]<<digits[byte&15];
            }out<<'\n'; }
        }
    }
    for (const auto& [token,node]:s.saveFortRoots) { out<<"raw_save_fort_name\t"<<token<<'\t'<<node->id<<'\t'<<node->serial<<'\t'<<node->flags<<'\t'<<node->entry<<'\t'<<node->ok<<'\t'<<node->cancel<<'\t'<<node->fortMode<<'\t'<<node->canCancel<<'\n'; } // 원문 실제 모달/필드만 자동 관측에 제공한다.
    out<<"raw_external_gumps\t"<<s.external.size()<<'\t'<<s.ownedExternal.size()<<'\n'; // 고정 슬롯과 실제 HUD 처리기 수명을 구별하는 읽기 관측이다.
    if (s.nsContext) out<<"raw_nsinfo\t"<<s.nsContext->globals->current<<'\t'<<s.nsRoots.size()<<'\t'<<s.nsPlayers.size()<<'\t'<<s.nsBattles.size()<<'\t'<<s.nsButtons.size()<<'\t'<<s.plainTexts.size()<<'\t'<<s.titles.size()<<'\n'; // 실제 현재 전역/등록 파생만 관측한다.
    out<<"raw_nsinfo_owned\t"<<s.nsOwned.size()<<'\n'; // 기본 생성자 전 미등록 저장소까지 같은 실제 소유 집합에서 관측한다.
    for (const auto& [token,node]:s.nsRoots) { // 실제 파생 필드와 이미 파싱한 현재 버퍼에서만 제목을 관측한다.
        out<<"raw_nsinfo_root\t"<<token<<'\t'<<node->id<<'\t'<<node->serial<<'\t'<<node->parent<<'\t'<<node->flags<<'\t'<<node->background<<'\t'<<node->roster<<'\t'<<node->title<<'\t'<<node->battle<<'\n';
        if (const auto* title=Find(s.titles,node->title);title && title->buffer) { const auto& buffer=*s.buffers->Binding(title->buffer).state;std::string label;
            for (auto head:buffer.hardLines) { // 완료/미완료 줄의 현재 소유 조각만 문자열로 읽는다.
                for (auto part=head;part;part=s.fragments.fragments.at(part).next) { const auto& bytes=s.fragments.fragments.at(part).text;label.append(bytes.begin(),std::find(bytes.begin(),bytes.end(),'\0')); } // 실제 그림/서식은 글자 진단으로 대체하지 않는다.
            }
            std::replace(label.begin(),label.end(),'\n',' ');std::replace(label.begin(),label.end(),'\t',' ');out<<"raw_nsinfo_caption\t"<<token<<'\t'<<buffer.lineCount<<'\t'<<label<<'\n';
        }
    }
    if (s.htmlContext) { // 실제 모달/부모 입력을 원본과 대조할 때도 같은 공통 저장소만 읽는다.
        out<<"raw_html_globals\t"<<s.htmlContext->globals->window.instances<<'\t'<<s.htmlContext->globals->window.lastCloseTime<<'\n';
        out<<"raw_modal_globals\t"<<s.common.life.dialogCount<<'\t'<<s.common.life.currentDialog<<'\t'<<s.modalMinimumDepth<<'\t'<<s.modalLowest<<'\n';
        for (const auto token:s.common.create.entries) if (token) { const auto* node=s.common.node(token);out<<"raw_modal_node\t"<<token<<'\t'<<node->flags<<'\t'<<node->depth<<'\t'<<node->child<<'\t'<<node->next<<'\t'<<node->parent<<'\n'; } // 선택 탐색이 실제로 보는 모든 현재 자식/형제/flags/depth를 관측한다.
    }
    for (const auto& [token,node]:s.htmlRoots) { // 같은 부모/본문/소유 조각을 읽으며 미작성 인자는 읽지 않는다.
        out<<"raw_html_root\t"<<token<<'\t'<<node->serial<<'\t'<<node->flags<<'\t'<<node->controlCount<<'\t'<<node->body<<'\t'<<node->requestedWidth<<'\t'<<node->requestedHeight<<'\n';
        for (std::uint32_t index=0;index<std::min(node->controlCount,static_cast<std::uint32_t>(node->controls.size()));++index) { // 실제 부모가 보유한 버튼의 현재 원문 인자/표시 경계만 보고한다.
            const auto& entry=node->controls[index];const auto* button=Find(s.helpButtons,entry.child);if (!button) continue;
            bool visible=node->parent!=0 && button->parent!=0;
            for (auto current=entry.child;visible && current;current=s.common.node(current)->parent) { const auto flags=s.common.node(current)->flags;if (!(flags&1U) || (flags&0x10000U)) visible=false; } // Kill/미부착/숨긴 조상을 실제 트리에서 구별한다.
            const auto bounds=s.common.collection.Bounds(entry.child);std::string label(button->label.begin(),std::find(button->label.begin(),button->label.end(),'\0'));
            std::replace(label.begin(),label.end(),'\n',' ');std::replace(label.begin(),label.end(),'\t',' ');
            out<<"raw_html_button\t"<<token<<'\t'<<index<<'\t'<<entry.child<<'\t'<<entry.command<<'\t'<<entry.parameter<<'\t'<<button->serial<<'\t'<<bounds.left<<'\t'<<bounds.top<<'\t'<<bounds.right<<'\t'<<bounds.bottom<<'\t'<<visible<<'\t'<<label<<'\n';
        }
        if (const auto* body=Find(s.titles,node->body);body && body->buffer) { const auto& buffer=*s.buffers->Binding(body->buffer).state;std::string text;
            for (const auto head:buffer.hardLines) for (auto part=head;part;part=s.fragments.fragments.at(part).next) { const auto& bytes=s.fragments.fragments.at(part).text;text.append(bytes.begin(),std::find(bytes.begin(),bytes.end(),'\0')); } // 실제 스타일/그림은 바꾸지 않고 진단 문자열만 이어 붙인다.
            std::replace(text.begin(),text.end(),'\n',' ');std::replace(text.begin(),text.end(),'\t',' ');out<<"raw_html_body\t"<<token<<'\t'<<node->body<<'\t'<<buffer.lineCount<<'\t'<<text<<'\n';
        }
    }
    out<<"raw_gump_help_strings\t"<<s.helpStrings.size()<<'\n'; // 같은 공통 scalar가 반납한 실제 도움말 문자열 수를 읽기만 한다.
    // 로딩 부모와 실제 자식의 현재 토큰/직사각형만 읽어 잔존·재사용·화면 위치를 관측한다.
    if (const auto token=s.LoadingRoot()) {
        const auto& fields=s.LoadingNode(token);const auto rect=s.common.collection.Bounds(token);
        out<<"raw_mission_loading\t"<<token<<'\t'<<s.common.node(token)->serial<<'\t'<<fields.body<<'\t'<<fields.controls[0].child<<'\t'<<rect.left<<'\t'<<rect.top<<'\t'<<rect.right<<'\t'<<rect.bottom<<'\n';
    }
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
