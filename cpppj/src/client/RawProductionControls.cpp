// 43e640/43ddd0/43ec00을 기존 격자·항목·SHP·글자 및 현재 Session에 조립한다.
#include "client/ClientColorSortInput.h"
#include "client/RawProductionControls.h"
#include "client/ProductionShortcuts.h"
#include "client/HudDividerInput.h"
#include "client/InventoryItemGump.h"
#include "client/ProductionHintGump.h"
#include "client/ImageGump.h"
#include "client/OwnerColorMaps.h"
#include "client/ProductionColorMaps.h"
#include "client/PaletteShade.h"
#include "client/RawCampaignCursor.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <sstream>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 안정된 파생 저장소의 주소만 반환한다.
template<class T> T* Find(const std::map<std::uint32_t,std::unique_ptr<T>>& nodes,std::uint32_t token) {
    const auto it=nodes.find(token);return it==nodes.end() ? nullptr : it->second.get();
}
// 같은 원본50바이트 슬롯의 비정렬 WORD다.
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t at) { return bytes[at]|(std::uint32_t(bytes[at+1])<<8); }
// 43e490의 실제 예산 표시 확장 필드다.
struct BudgetNode: TextGumpNode { std::int32_t amount{},flash{};double until{}; }; // +a4/+b0/+a8의 실제 금액/점멸/기한이다.
// 원본 부모의 자식 필드와 유지 상태를 같은 실제 Gump 파생 객체에 보관한다.
struct ProductionRootNode:GumpCreateNode {
    std::uint32_t top{},middle{},bottom{},reservedAC{},bridgeGrid{},teleportGrid{},budgetToken{},separator{},reservedC0{},reservedC4{}; // +a0..c4의 실제 자식/예약 필드다.
    ProductionMaintenancePanel panel;double secondTimer{}; // +c8/d0/d8/dc/e0의 실제 원문 유지 상태이며 별도 패널 사본이 아니다.
};
// 초기 생성에 필요한 실제 owner가 모두 있어야 하며 다른 archive를 섞지 않는다.
ProductionControlContext RequireContext(ProductionControlContext context,const GameAssets& assets) {
    if (!context.storage || !context.gameplay || !context.players || context.storage->pool.Edition()!=o::OriginalEdition::Patch1078 || (context.storage->Source() && context.storage->Source()!=&assets)) throw std::invalid_argument("생산의 실제 현재 Context 미연결/불일치");return context;
}
constexpr double kFlashInterval=0.15000000596046448; // PE506858의 실제 승격 float다.
constexpr std::array<std::uint32_t,12> kStandard{82,155,158,164,135,133,156,73,170,146,144,126}; // 실제43be30의 일반 색표 대상이다.
constexpr std::array<std::uint32_t,5> kSpecial{111,141,137,94,97}; // 실제 전용 색표 대상이다.
}
// 현재 월드를 참조하는 처리기/원본 파생 필드의 단일 고정 주소 소유자다.
struct RawProductionControls::Impl {
    RawMenuTree common;RawMenuControls& menus;ProductionControlContext context;const GameAssets& assets;RawProductionHooks hooks; // 공통 상태와 외부 장치다.
    std::uint32_t drawRegistration{}; // 같은 메뉴 수집기에 등록한 자기 출력 수명만 반납한다.
    ProductionMaintenanceGlobals& productionGlobals;std::unique_ptr<RawProductionMaintenance> maintenance; // 전역/패널의 단일 상태와 같은 Session 유지 몸체다.
    ProductionRootNode root;ProductionMaintenancePanel& productionPanel{root.panel};std::uint32_t rootToken{};bool constructing{}; // 파생 본체/유지 상태의 동일 주소와 생성 중 추가 동기화 억제다.
    std::uint32_t &budgetToken{root.budgetToken},&top{root.top},&middle{root.middle},&bottom{root.bottom},&bridgeGrid{root.bridgeGrid},&teleportGrid{root.teleportGrid},&separator{root.separator};std::uint32_t slotLimit{}; // 43e640의 실제 자식 포인터다.
    std::map<std::uint32_t,std::unique_ptr<InventoryGridNode>> grids;std::map<std::uint32_t,std::unique_ptr<InventoryItemNode>> items; // 같은 현재 SID 격자/항목이다.
    std::map<std::uint32_t,std::unique_ptr<ImageGumpNode>> images;std::map<std::uint32_t,std::unique_ptr<TextGumpNode>> texts; // 실제 자식 그림/수량이다.
    std::map<std::uint32_t,std::unique_ptr<GumpBackgroundNode>> backgrounds;BudgetNode budget; // 배경과 실제 예산 파생이다.
    std::uint32_t owner{},inputDisabled{},lastType{},bridgeType{82},backgroundType{},backgroundFrame{};std::uint32_t& stock; // 현재 owner/정책과 공유하는 실제5318f4 설정이다.
    bool editor{};float pixelsX{16},pixelsY{11};ScreenPoint point{},shadow{5,5};std::uint32_t cursorFlags{}; // 현재 원본 배율/커서 전역이다.
    std::vector<std::uint32_t> ownerMaps;std::map<std::uint32_t,OwnerColorMaps> ownerColors;ProductionColorMaps colors;GumpShadeMaps shades; // 실제 원본 색표와 존재 토큰이다.
    PriestPlacementAssets geometry;o::PriestPlacementShapeState measureState;o::RawCanonPixelShape measure; // 실제 전체 Canon 경계/프레임이다.
    GumpDrawState hitClip;std::vector<std::uint8_t> hitBytes;ColorMap hitColors{};GumpPixelTarget hitTarget; // 원본 이미지 hit의 실제200×200 임시 표면이다.
    std::unique_ptr<ImageGumpPixelTarget> hitRaster;ImageGumpHooks activeImage;const GumpBackgroundDraw* activeBackground{}; // 실제 현재 출력 문맥이다.
    ProductionHintState hintState;std::unique_ptr<ProductionHintGump> hint;std::map<std::uint32_t,std::unique_ptr<ProductionHintNode>> hints; // 공통 루트에 붙는 실제 실패 안내다.
    std::map<std::uint32_t,std::string> hintStrings;std::uint32_t hintStringSerial{0x70000000U}; // 원본 strdup/free의 단일 소유 문자열이다.
    std::map<std::uint32_t,std::unique_ptr<HudDividerNode>> dividers;HudDockHooks dividerHooks;std::int8_t dividerKind{55}; // 실제0x11c 저장소와 같은 공통 분배다.
    std::unique_ptr<HudDividerGump> dividerGeometry;std::unique_ptr<HudDividerInput> dividerInput; // 원문 생성/부착과 가상 입력을 공유한다.
    std::unique_ptr<InventoryGump> grid;std::unique_ptr<InventoryItemGump> item;std::unique_ptr<ImageGump> image; // 기존 원본 전체 몸체다.
    // 모든 훅은 실행 시 현재 공통 노드와 같은 Session을 재조회한다.
    Impl(RawMenuTree tree,RawMenuControls& menu,ProductionControlContext current,const GameAssets& data,ProductionMaintenanceGlobals& globals,RawProductionHooks supplied)
        :common(std::move(tree)),menus(menu),context(RequireContext(std::move(current),data)),assets(data),hooks(std::move(supplied)),productionGlobals(globals),stock(globals.stockEnabled),ownerMaps(data.TypeTable().Types().size()),geometry(data),
        measure(context.storage->pool,context.storage->types,geometry.frames,geometry.shapes,geometry.hotspots,geometry.geometry,measureState),hitBytes(40000,255),hitTarget(hitBytes,200,200,hitClip) {
        if (!hooks.now || !hooks.sound || !hooks.tell || !hooks.configInt || assets.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("실제 생산 패널 연결/판본 오류");
        backgroundType=static_cast<std::uint32_t>(assets.Find("fortGump").block+o::kFirstAssetTypeNumber);SetGumpDrawClip(hitClip,{0,0,200,200});
        // 원본의 일반 소유자 표만 즉시 만들고 전용 표는 파일 팔레트 설치 때 만든다.
        for (const auto type:kStandard) { ownerMaps.at(type)=0x100000U+type*2304U;ownerColors.emplace(type,BuildStandardOwnerColorMaps(type,bridgeType)); }
        // 전용 표의 존재 여부도 실제 TYPE별 등록을 따른다.
        for (const auto type:kSpecial) ownerMaps.at(type)=0x100000U+type*2304U;
        hitRaster=std::make_unique<ImageGumpPixelTarget>(hitTarget,assets,hitColors,[this](auto token) { return Color(token); });
        ImageGumpHooks ih;ih.node=[this](auto token) { return Find(images,token); };BindImageGumpAssets(ih,assets);ih.position=[this](auto token) { return common.collection.Position(token); };ih.attach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };
        ih.hit=hitRaster->Hooks({}).hit;ih.draw=[this](auto request) { activeImage.draw(request); };ih.color=[this](auto token) { activeImage.color(token); };image=std::make_unique<ImageGump>(common.create,common.factory,ImageGumpState{owner,ownerMaps,shadow},std::move(ih));
        InventoryGumpHooks gh;gh.grid=[this](auto token) { return Find(grids,token); };gh.item=[this](auto token) { return Find(items,token); };gh.clearChildren=[this](auto token) { ClearChildren(token); };gh.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };
        gh.allocateItem=[this](auto size) { if (size!=0xc0) throw std::logic_error("생산 항목 할당 크기 오류");const auto token=common.token();items.emplace(token,std::make_unique<InventoryItemNode>());return token; };gh.attach=common.attach;gh.push=[this](auto token) { common.tree.Push(token); };gh.bind=[this](auto token,auto object) { Bind(token,object); };
        gh.idle=[this](auto token,bool enabled) { common.lifetime.SetIdle(token,enabled); };gh.now=hooks.now;gh.contentParent=[this](auto sid) { return Word(context.storage->pool.Slot({static_cast<std::uint16_t>(sid)}),16); };
        gh.forward=[this](auto token,auto event,auto first,auto second) { return common.events.Bubble(token,event,first,second); };gh.baseHandle=[this](auto token,auto event,auto first,auto second) { return menus.BaseHandle(token,event,first,second); }; // 원본 격자도 같은 실제 기본 도움말을 사용한다.
        gh.local=[this](auto token,auto,ScreenPoint p) { return common.collection.ScreenToLocal(token,p); };gh.inside=[this](auto token,auto p) { const auto& n=*common.node(token);return p[0]>=0 && p[1]>=0 && p[0]<n.width && p[1]<n.height; };
        gh.cancelCursor=[this] { Session().Cursor().Cancel(); };gh.assertion=[](auto text) { throw std::logic_error(std::string(text)); };
        grid=std::make_unique<InventoryGump>(common.create,common.factory,context.storage->pool,context.gameplay->finder,InventoryGumpState{point,cursorFlags},std::move(gh));
        InventoryItemHooks h;h.node=[this](auto token) { return Find(items,token); };h.base=common.node;BindInventoryItemAssets(h,assets,geometry,measure);
        h.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };h.clearChildren=[this](auto token) { ClearChildren(token); };h.cleanup=[this] { common.lifetime.Cleanup(); };
        h.robust=[this](auto sid,bool) { return sid<context.storage->pool.Capacity() && context.storage->pool.Slot({static_cast<std::uint16_t>(sid)})[10]==context.gameplay->finder.containedType; };h.assertion=[](auto text) { throw std::logic_error(std::string(text)); };
        h.allocate=[this](auto bytes) { const auto token=common.token();if (bytes==0xb4) images.emplace(token,std::make_unique<ImageGumpNode>());else if (bytes==0xa4) texts.emplace(token,std::make_unique<TextGumpNode>());else throw std::logic_error("생산 자식 할당 크기 오류");return token; };
        h.image=[this](auto token,auto type,auto frame,auto mode) { image->Construct(token,type,static_cast<std::uint32_t>(frame),mode); };h.text=[this](auto token) { menus.TextBackend().Construct(token,0,0,0,0); };
        h.styledText=[this](auto token,std::string_view text) { Style(token,text); };h.fitText=[this](auto token) { menus.TextBackend().Fit(token); };h.attach=common.attach;
        h.costUnits=[this](auto type) { return Session().HitPoints().TypeCostUnits(type); };h.money=[this](auto who) { return Session().Sp(who); };h.position=[this](auto token) { return common.collection.Position(token); };
        h.background=[this](auto rect,auto flags,auto type,auto frame) { if (!activeBackground) throw std::logic_error("생산 장식 실제 표면 없음");activeBackground->Texture(rect,flags,type,frame); };
        h.local=[this](auto token,auto,auto x,auto y) { return common.collection.ScreenToLocal(token,{std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)}); };h.knowledgeOpen=[this] { return context.gameplay->cursor.lockDepth==0 ? 1U : 0U; };
        h.imageHit=[this](auto token,float x,float y) { return image->Hit(token,x,y); };h.denied=[this](auto first,auto flags) { return first==productionGlobals.policyFirst && static_cast<std::uint8_t>(flags)==productionGlobals.policyByte ? 1U : 0U; };h.tell=hooks.tell;
        h.forward=[this](auto token,auto event,auto first,auto second) { return common.events.Bubble(token,event,first,second); };h.about=[this](auto token,auto x,auto y)->std::uint32_t { if (!hooks.about) throw std::logic_error("생산 항목 About 메뉴 미연결");return hooks.about(*items.at(token),{std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)},[this](o::Sid sid) { return item->Affordable(sid)!=0; }); };
        item=std::make_unique<InventoryItemGump>(context.storage->pool,InventoryItemState{owner,editor,stock,inputDisabled,lastType,bridgeType,pixelsX,pixelsY,backgroundType,backgroundFrame},std::move(h));
        ProductionHintHooks ph;ph.node=[this](auto token) { return Find(hints,token); };ph.now=hooks.now;
        ph.allocate=[this](auto size) { if (size!=0xb8) throw std::logic_error("생산 안내 할당 크기 오류");const auto token=common.token();hints.emplace(token,std::make_unique<ProductionHintNode>());return token; };
        ph.constructText=[this](auto token) { menus.TextBackend().Construct(token,0,0,6,0); };ph.kill=[this](auto token) { common.lifetime.Kill(token);common.changed(); };
        ph.duplicate=[this](std::string_view text) { if (hintStringSerial==0xffffffffU) throw std::overflow_error("생산 안내 문자열 토큰 소진");const auto token=++hintStringSerial;hintStrings.emplace(token,text);return token; };
        ph.string=[this](auto token)->std::string_view { return hintStrings.at(token); };ph.free=[this](auto token) { if (hintStrings.erase(token)!=1) throw std::logic_error("생산 안내 문자열 중복 반납"); };
        ph.translate=[this](auto key) { if (!hooks.translate) throw std::logic_error("생산 안내 번역 미연결");return hooks.translate(key); };ph.style=[this](auto token,auto text) { Style(token,text); };
        ph.fit=[this](auto token,auto x,auto y) { menus.TextBackend().Fit(token,x,y); };ph.idle=[this](auto token,bool enabled) { common.lifetime.SetIdle(token,enabled); };
        ph.bounds=[this](auto token) { return common.collection.Bounds(token); };ph.position=[this](auto token) { return common.collection.Position(token); };ph.draw=[this](auto head,auto p) { menus.StyleBackend().Draw(head,p); };
        ph.attach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };ph.deleteText=[this](auto token,auto flags) { return menus.TextBackend().DeleteScalar(token,flags); };
        hint=std::make_unique<ProductionHintGump>(common.create,hintState,std::move(ph));
        dividerHooks.node=common.node;dividerHooks.divider=[this](auto token) { return Find(dividers,token); };
        dividerHooks.constructBackground=[this](auto token,float w,float h,auto flags,auto type,auto frame) { menus.BackgroundBackend().Construct(token,w,h,flags,1,type,frame); };
        dividerHooks.activate=[this](auto token,auto enabled) { common.lifetime.SetOverlay(token,enabled!=0); };
        dividerHooks.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };dividerHooks.push=[this](auto token) { common.tree.Push(token); };dividerHooks.attach=common.attach;
        dividerHooks.move=[this](auto token,float x,float y,auto mode) { common.tree.Move(token,x,y,mode); };dividerHooks.resolveSerial=[this](auto serial) { return Reference(serial); };
        dividerHooks.bubble=[this](auto token,auto event,auto a,auto b) { common.events.Bubble(token,event,a,b); };dividerHooks.closePopups=[this] { menus.RequestClosePopups(); };
        dividerGeometry=std::make_unique<HudDividerGump>(common.create.screenWidth,common.create.screenHeight,dividerKind,dividerHooks);
        HudDividerInputHooks dh;dh.divider=[this](auto token) { return Find(dividers,token); };dh.baseHandle=[this](auto token,auto event,auto a,auto b) { return menus.BaseHandle(token,event,a,b); };
        dh.base=common.node;dh.cursor=hooks.cursor;dh.poll=hooks.poll;dh.menuInstances=[this] { return menus.MenuInstances(); };dividerInput=std::make_unique<HudDividerInput>(common.collection,common.input,common.tree,std::move(dh));
        drawRegistration=menus.RegisterExternalGumps({[this](auto token) { return Text(token); },[this](auto token) { return Node(token)!=nullptr; },[this](auto token,auto& target,auto& currentColors,auto& background) { Draw(token,target,currentColors,background); }});
    }
    // 초기 생성도 실제 현재 전역을 읽으며 빈 Session이나 고정 성공값을 만들지 않는다.
    void ReadCurrent() { owner=context.gameplay->player.battleOwner;editor=context.gameplay->world.editor!=0;productionGlobals.bridgeType=context.storage->links.bridgeType;backgroundType=productionGlobals.backgroundType;backgroundFrame=productionGlobals.backgroundFrame;point={context.gameplay->cursor.screenX,context.gameplay->cursor.screenY};cursorFlags=context.gameplay->cursor.inputFlags; }
    // 원문 생성 중에는 호스트의 현재 옵션 재동기화를 섞지 않는다.
    void Prepare() { ReadCurrent();if (!constructing) stock=static_cast<std::uint32_t>(hooks.configInt("useProductTimers",1)); }
    // 선택한 게임 효과에서만 현재 실제 Runtime을 요구한다.
    RawCampaignSession& Session() const { return context.gameplay->RequireSession(); }
    // 원문 known0에서는 실제 Player 포인터를 읽지 않는다. 알려진 현재 parent만 같은9기록에서 읽는다.
    std::uint32_t Technology() const { if (!context.gameplay->player.playerKnown) return 0;return Technology(context.gameplay->player.battleOwner); }
    // 명시 owner 조회는 known 진단과 실제 물리 범위를 보존한다.
    std::uint32_t Technology(std::uint32_t who) const {
        if (!context.gameplay->player.playerKnown) throw std::logic_error("생산의 명시 PlayerKnown 진단");const auto bytes=context.players->Records();const auto at=std::size_t(who)*180+0x9c;
        if (who>=9 || at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("생산의 실제 Player 기술 필드 없음");return Word(bytes,at)|(Word(bytes,at+2)<<16);
    }
    // 현재 모드/기술이 없으면 원문 생산 단락은 자연 반환하며 Runtime을 요구하지 않는다.
    bool UpdateContents() { const auto& mode=context.storage->ownerMode;if ((!mode.fort && !mode.battle && !mode.challenge) || mode.challenge || !Technology()) return false;return Session().UpdateProductionContents(); }
    // 실제 현재 기술을 다루는 단계에 도달했을 때만 Session의 유지 효과를 연결한다.
    void EnsureMaintenance() {
        maintenance=Session().MakeProductionMaintenance(productionGlobals,productionPanel,hooks.now,[this](std::uint32_t mask) {
            // 원본은 텔레포트 격자를 먼저, 다리 격자를 뒤에 Push/가상부착한다.
            for (const auto& [token,bit]:std::array<std::pair<std::uint32_t,std::uint32_t>,2>{{{teleportGrid,1},{bridgeGrid,2}}}) if (mask&bit) { common.tree.Push(token);const auto& n=*grids.at(token);grid->AttachGrid(token,n.x,n.y,0,0); }common.changed();
        });
    }
    // 외부 기본 생성자/수명은 같은 파생 기본 주소만 본다.
    GumpCreateNode* Node(std::uint32_t token) const {
        if (token==rootToken && token) return const_cast<ProductionRootNode*>(&root);if (token==budgetToken && token) return const_cast<BudgetNode*>(&budget);
        if (auto* n=Find(dividers,token)) return n;if (auto* n=Find(grids,token)) return n;if (auto* n=Find(items,token)) return n;if (auto* n=Find(images,token)) return n;if (auto* n=Find(texts,token)) return n;if (auto* n=Find(hints,token)) return n;return Find(backgrounds,token);
    }
    // 실제 TextGump 파생만 같은 글자 처리기에 공급한다.
    TextGumpNode* Text(std::uint32_t token) const { if (token==budgetToken && token) return const_cast<BudgetNode*>(&budget);if (auto* n=Find(hints,token)) return n;return Find(texts,token); }
    // 삭제 중 머리가 바뀌므로 매번 현재 실제 child를 읽는다.
    void ClearChildren(std::uint32_t token) { while (common.node(token)->child) common.lifetime.Kill(common.node(token)->child); }
    // 4644e0은0부모에 무동작이며 이전 Form을 먼저 지우지 않는다.
    void Bind(std::uint32_t token,std::uint32_t object) {
        if (!object) return;auto& forms=Session().GumpForms();const auto form=forms.Create(2);auto& n=*common.node(token);forms.SetOwner(form,n.id);forms.Attach(form,{static_cast<std::uint16_t>(object)},0);n.flags|=0x40;n.object=object;n.form=form.value;
    }
    // 같은 StyleText의 실제 소유 체인/문맥을 직접 변경한다.
    void Style(std::uint32_t token,std::string_view value) { auto& n=*Text(token);menus.StyleBackend().Parse(n.head,value,n.textContext); }
    // 원본43da10의 단계색·그림·점멸과 깊이 보존이다.
    void BudgetText() {
        const auto color=budget.amount<1001 ? 'r' : budget.amount<2001 ? 'y' : 'w';const auto text="~3~E~"+std::string(1,color)+std::to_string(budget.amount)+"~[I"+std::to_string(backgroundType)+".3]";
        menus.TextBackend().Clear(budgetToken);const auto depth=budget.depth;common.tree.Push(budgetToken);
        if (!(budget.flash&1)) { Style(budgetToken,editor ? std::string_view{} : std::string_view(text));if (!editor) menus.TextBackend().Fit(budgetToken); }
        common.tree.Attach(budgetToken,0,2,0,2);budget.depth=depth;common.changed();
    }
    // 같은 실제 배경 생성자의 TYPE/SHP 정보로 자식을 만든다.
    std::uint32_t Background(float width,float height,std::uint32_t frame,std::uint32_t flags=0x2000000) {
        const auto token=common.token();backgrounds.emplace(token,std::make_unique<GumpBackgroundNode>());menus.BackgroundBackend().Construct(token,width,height,flags,1,productionGlobals.backgroundType,frame);return token;
    }
    //0043e640은 생성만 하며 도킹80/하단80예약/즉시Sync를 수행하지 않는다.
    std::uint32_t Construct() {
        if (rootToken) throw std::logic_error("현재 생산 패널 중복 생성");
        struct Phase { bool& active;explicit Phase(bool& value):active(value) { active=true; }~Phase() { active=false; } }; // 부분 실패에서도 생성 단계의 호스트 동기화 억제를 반납한다.
        Phase phase(constructing);ReadCurrent();rootToken=common.token();common.factory.Construct(rootToken,0,0,0,1);root.vtable=0x5069c0;
        productionPanel.bridgeUntil=0;root.secondTimer=0;root.command=44;root.flags|=0xe00;productionPanel.oldMoney=0;productionPanel.temple=0;common.lifetime.SetIdle(rootToken,true);
        productionGlobals.policyFirst=0;productionGlobals.policyByte=0;root.baseWidth=40;root.width=static_cast<float>(40.0*root.scale);
        root.baseHeight=static_cast<float>(common.create.screenHeight);root.height=static_cast<float>(static_cast<double>(common.create.screenHeight)*root.scale);
        budgetToken=0;budgetToken=common.token();menus.TextBackend().Construct(budgetToken,0,0,0,0);budget.vtable=0x506948;budget.until=0;productionGlobals.budgetSerial=budget.serial;budget.amount=0;budget.flash=0;common.lifetime.SetIdle(budgetToken,true);
        if (!hooks.translate) throw std::logic_error("생산 생성자의 실제 번역 경계 미연결");menus.SetHelpText(budgetToken,hooks.translate("Storm Power Available"));menus.SetHelpWord(budgetToken,"moneyGump");budget.alignFlags|=4;common.tree.Attach(budgetToken,0,0,rootToken,2);BudgetText();
        top=0;top=Background(root.baseWidth,root.baseHeight,4);auto& a=*backgrounds.at(top);a.alignFlags|=1;a.baseHeight=23;a.height=static_cast<float>(23.0*a.scale);common.tree.Attach(top,0,0,rootToken,0);
        root.reservedAC=0;productionPanel.bridgeUntil=hooks.now();productionPanel.initialized=true;slotLimit=std::uint32_t(context.options[1])*2+2;productionPanel.slots=static_cast<std::int32_t>(slotLimit);
        bridgeGrid=0;bridgeGrid=common.token();grids.emplace(bridgeGrid,std::make_unique<InventoryGridNode>());grid->ConstructGrid(bridgeGrid,2,static_cast<std::int32_t>(slotLimit/2),0x40);auto& b=*grids.at(bridgeGrid);b.fixed=1;b.baseHeight=static_cast<float>(slotLimit*36/2);b.height=static_cast<float>(static_cast<double>(slotLimit*36/2)*b.scale);b.acceptedCursor=0x40;
        menus.SetHelpText(bridgeGrid,hooks.translate("Production Window"));menus.SetHelpWord(bridgeGrid,"teleportView");b.alignFlags|=1;b.itemScale=std::bit_cast<float>(0x3f2147aeU);common.attach(bridgeGrid,0,26,rootToken,0);
        middle=0;middle=Background(root.baseWidth,root.baseHeight,productionGlobals.panelFrame);menus.SetHelpText(middle,hooks.translate("Production Window"));menus.SetHelpWord(middle,"teleportView");auto& c=*backgrounds.at(middle);c.alignFlags|=1;c.baseHeight=static_cast<float>(slotLimit*36/2);c.height=static_cast<float>(static_cast<double>(slotLimit*36/2)*c.scale);common.tree.Attach(middle,0,26,rootToken,0);
        teleportGrid=0;teleportGrid=common.token();grids.emplace(teleportGrid,std::make_unique<InventoryGridNode>());grid->ConstructGrid(teleportGrid,1,6,8);auto& d=*grids.at(teleportGrid);d.baseHeight=250;d.height=static_cast<float>(250.0*d.scale);d.acceptedCursor=8;d.alignFlags|=1;common.attach(teleportGrid,0,static_cast<float>(static_cast<double>(c.y)+c.baseHeight),rootToken,0);
        bottom=0;bottom=Background(root.baseWidth,root.baseHeight,productionGlobals.panelFrame);menus.SetHelpText(bottom,hooks.translate("Production Window"));menus.SetHelpWord(bottom,"teleportView");auto& e=*backgrounds.at(bottom);e.alignFlags|=1;e.baseHeight=250;e.height=static_cast<float>(250.0*e.scale);common.tree.Attach(bottom,0,d.y,rootToken,0);
        separator=0;separator=common.token();dividers.emplace(separator,std::make_unique<HudDividerNode>());dividerGeometry->Construct(separator,0,static_cast<std::int32_t>(root.baseWidth),5,0x2a00000,productionGlobals.backgroundType,productionGlobals.backgroundFrame);dividers.at(separator)->alignFlags|=1;common.attach(separator,0,21,rootToken,0);
        root.reservedC0=0;root.reservedC4=0;productionGlobals.rootSerial=root.serial;common.changed();return rootToken;
    }
    // 현재 Client의 설치 후 추가 배치/한 번의 Sync를 생성자 뒤에 적용한다.
    void Open() {
        Construct();root.baseWidth=root.width=80;root.baseHeight=root.height=static_cast<float>(common.create.screenHeight-80);Attach(rootToken,0,0,common.create.collection.root,0);Handle(rootToken,0x0200003d,1,0);
        for (const auto current:{bridgeGrid,teleportGrid}) { common.tree.Push(current);const auto& n=*grids.at(current);grid->AttachGrid(current,n.x,n.y,0,0); } // 설치된 현재 내용물도 실제 격자로 다시 부착한다.
        common.changed();
    }
    // 원본43ddd0의 Push/현재 크기/재부착/깊이 순서를 보존한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        if (token && token==budgetToken) { common.tree.Attach(token,x,y,parent,mode);return; } //예산 가상+20은00466750 기본 부착이며43ddd0의 생산 설정 조회를 거치지 않는다.
        Prepare();if (dividers.contains(token)) { dividerGeometry->Attach(token,x,y,parent,mode);return; }if (grids.contains(token)) { grid->AttachGrid(token,x,y,parent,mode);return; }if (items.contains(token)) { item->Attach(token,x,y,parent,mode);return; }if (images.contains(token)) { image->Attach(token,x,y,parent,mode);return; }
        common.tree.Attach(token,x,y,parent,mode);if (token!=rootToken) return;
        common.tree.Push(budgetToken);common.tree.Attach(budgetToken,0,2,rootToken,2);common.tree.Push(bottom);auto& e=*backgrounds.at(bottom);e.baseHeight=root.baseHeight-e.y;e.height=e.baseHeight*e.scale;common.tree.Attach(bottom,e.x,e.y,0,0);
        common.tree.Push(teleportGrid);auto& d=*grids.at(teleportGrid);d.baseHeight=root.baseHeight-d.y;d.height=d.baseHeight*d.scale;const auto& c=*backgrounds.at(middle);grid->AttachGrid(teleportGrid,0,c.y+c.baseHeight,rootToken,0);
        ++backgrounds.at(top)->depth;budget.depth=backgrounds.at(top)->depth-1;++backgrounds.at(middle)->depth;++backgrounds.at(bottom)->depth;
    }
    // 464020은 등록 인덱스가 아니라 +2c 일련번호로 살아 있는 노드를 찾는다.
    std::uint32_t Reference(std::uint32_t serial) const {
        // 원본 검색은 루트 칸0을 제외하며 죽은 노드는 건너뛴다.
        for (std::size_t i=1;i<common.create.entries.size();++i) if (const auto token=common.create.entries[i]) { const auto* n=common.node(token);if (n && n->serial==serial && !(n->flags&0x10000)) return token; }return 0;
    }
    // 실제 항목 일련번호의 현재 화면 위치에서 원본 실패 안내를 생성한다.
    void Failure(bool empty,std::uint32_t serial) { const auto reference=Reference(serial);const auto p=common.collection.Position(reference);hint->Show(empty,p.x,p.y,reference);common.changed(); }
    // 클릭된 실제 내용물 SID를 그대로447000에 넘기며 실패는 취득 전에 안내한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
        Prepare();if (dividers.contains(token)) return dividerInput->Handle(token,event,first,second);if (grids.contains(token)) return grid->HandleGrid(token,event,first,second);if (items.contains(token)) return item->Handle(token,event,first,second);if (token!=rootToken) return menus.BaseHandle(token,event,first,second);
        const auto code=event&0x03ffffffU;
        if (code==0x02000008) {
            const auto raw=context.storage->pool.Slot({static_cast<std::uint16_t>(first)});if ((Word(raw,20)&0x100) || context.gameplay->cursor.lockDepth==0) return 1;
            if (Word(raw,20)&8) {
                if (!item->Affordable({static_cast<std::uint16_t>(first)})) { budget.flash=10;budget.until=hooks.now()+kFlashInterval;Failure(false,second);return 1; }
                if (stock && !raw[30]) { Failure(true,second);return 1; }
            }
            if (!context.storage->types[Word(raw,18)].flags2) return 1;Session().Cursor().Take({static_cast<std::uint16_t>(first)},(Word(raw,20)&8) ? 0x41U : 1U);hooks.sound("getPiece.wav");common.changed();return 1;
        }
        if (code==0x0200003d) { Bind(bridgeGrid,Technology());Bind(teleportGrid,Technology());if (first==1) Session().SynchronizeProductionContents();return 0; }
        if (code==kProductionBridgeCommand) {
            o::RawContainedFinder finder(context.storage->pool,context.gameplay->finder);
            // 실제 현재 기술의flags40/슬롯 BYTE가 같은 항목을 순서대로 찾는다.
            for (auto sid=finder.Begin({static_cast<std::uint16_t>(Technology(context.gameplay->player.templateOwner))});sid.value;sid=finder.Next()) { const auto raw=context.storage->pool.Slot(sid);if ((Word(raw,20)&0x40) && raw[23]==first) { Session().Cursor().Take(sid,0);hooks.sound("getPiece.wav");return 1; } }
        }
        return 0;
    }
    // 43e530의 금액 표시와43ef10의 dirty/부모 재연결→공급/내용물 유지 순서다.
    void Idle(std::uint32_t token) {
        Prepare();if (hints.contains(token)) { hint->Idle(token);return; }if (token==budgetToken) {
            const auto target=context.gameplay->player.playerKnown ? static_cast<std::int32_t>(Session().Sp(owner)) : 0;
            if (budget.flash && budget.until<=hooks.now()) { --budget.flash;budget.until=hooks.now()+kFlashInterval;BudgetText(); }
            if (budget.amount!=target) { const auto distance=static_cast<std::int64_t>(target)-budget.amount;budget.amount+=distance<0 ? (distance<-300 ? -100 : distance<-30 ? -10 : -1) : (distance>300 ? 100 : distance>30 ? 10 : 1);if (context.storage->ownerMode.fort) budget.amount=target;BudgetText(); }
        } else if (token==rootToken) {
            if (UpdateContents()) common.changed();
            const auto parent=Technology();const auto& mode=context.storage->ownerMode;if (!parent || (!mode.fort && !mode.battle) || mode.challenge) return;
            // 원본은 실제 부모가 바뀐 격자만 Push/Bind/재부착한다.
            if (!editor) for (const auto current:{bridgeGrid,teleportGrid}) if (grids.at(current)->object!=parent) { common.tree.Push(current);Bind(current,parent);auto& n=*grids.at(current);grid->AttachGrid(current,n.x,n.y,0,0); }
            if (!maintenance) EnsureMaintenance();maintenance->Run();
        } else if (auto* n=Find(items,token)) {
            if (!n->blink) { common.lifetime.SetIdle(token,false);n->flags|=1;common.changed();auto raw=context.storage->pool.AllocatedBytes({static_cast<std::uint16_t>(n->content)});raw[21]&=0xfd; }
            else if (n->blinkUntil<=hooks.now()) { --n->blink;n->blinkUntil=hooks.now()+0.1;n->flags^=1;common.changed(); }
        }
    }
    // 실제 원본 토큰을 현재 색표 바이트로만 해석한다.
    ColorMap Color(std::uint32_t token) {
        if (token==0x556038) return colors.disabled;if (token==0x556338) return colors.underlay;if (token==0x556438) return shades.dark;
        // 등록한 TYPE/owner의 실제 표만 허용한다.
        for (const auto& [type,maps]:ownerColors) if (token>=ownerMaps[type] && token<ownerMaps[type]+2304U && (token-ownerMaps[type])%256==0) return maps[(token-ownerMaps[type])/256];
        throw std::logic_error("생산 아이콘 원본 색표 토큰 없음");
    }
    // 같은 공통 정렬/클립/표면에서 실제 각 파생 Draw를 호출한다.
    void Draw(std::uint32_t token,GumpPixelTarget& target,GumpGlyphColorState& current,GumpBackgroundDraw& background) {
        Prepare();if (hints.contains(token)) { hint->Draw(token);return; }if (Text(token)) { menus.TextBackend().Draw(token);return; }if (backgrounds.contains(token) || dividers.contains(token)) { background.Draw(token);return; }
        if (items.contains(token)) { activeBackground=&background;try { item->Draw(token); } catch (...) { activeBackground=nullptr;throw; }activeBackground=nullptr;return; }
        if (images.contains(token)) { ImageGumpPixelTarget raster(target,assets,current.vfxColors,[this](auto color) { return Color(color); });activeImage=raster.Hooks({});try { image->Draw(token); } catch (...) { activeImage={};throw; }activeImage={}; }
    }
};
// 공통 분배가 호출하기 전에 저장소/훅을 준비하고 Open은 소유자 설치 뒤 실행한다.
RawProductionControls::RawProductionControls(RawMenuTree tree,RawMenuControls& menus,RawCampaignSession& session,const GameAssets& assets,ProductionMaintenanceGlobals& globals,RawProductionHooks hooks):RawProductionControls(std::move(tree),menus,session.ProductionContext(),assets,globals,std::move(hooks)) {}
// 실제 초기 Context는 현재 Runtime이 없어도 전체 자식 생성/그리기에 필요한 물리를 제공한다.
RawProductionControls::RawProductionControls(RawMenuTree tree,RawMenuControls& menus,ProductionControlContext context,const GameAssets& assets,ProductionMaintenanceGlobals& globals,RawProductionHooks hooks):impl_(std::make_unique<Impl>(std::move(tree),menus,std::move(context),assets,globals,std::move(hooks))) {} // 같은 상태를 빌리고 별도 사본을 만들지 않는다.
void RawProductionControls::Close() {
    // 안내는 생산 루트의 형제이므로 같은 수명 처리기로 별도 삭제한다.
    for (const auto& [token,n]:impl_->hints) if (!(n->flags&0x10000)) impl_->common.lifetime.Kill(token);
    impl_->common.lifetime.Cleanup();
    // 부분 생성의 미부착 자식도 실제 등록표에서만 찾는다. 미등록 저장소로 루트0을 지우지 않는다.
    for (std::uint32_t id=1;id<impl_->common.create.entries.size();++id) {
        const auto token=impl_->common.create.entries[id];if (auto* node=Node(token);node && !(node->flags&0x10000U)) impl_->common.lifetime.Kill(token);
    }
    impl_->common.lifetime.Cleanup();impl_->rootToken=0;impl_->budgetToken=0;
} // 같은 자식 우선 수명으로 현재 생산 소유자의 등록만 반납한다.
RawProductionControls::~RawProductionControls() { impl_->menus.UnregisterExternalGumps(impl_->drawRegistration); } // 자기 출력만 반납하며 다른 세대의 생산은 유지한다.
std::uint32_t RawProductionControls::RootToken() const { return impl_->rootToken; } // 동일 실제 루트 주소 별칭이다.
bool RawProductionControls::HasNodes() const {
    const auto& s=*impl_;return s.rootToken || s.budgetToken || !s.dividers.empty() || !s.grids.empty() || !s.items.empty() || !s.images.empty() || !s.texts.empty() || !s.backgrounds.empty() || !s.hints.empty();
} // 실제 scalar 해제가 모두 끝나기 전에는 처리기 주소를 반납하지 않는다.
std::uint32_t RawProductionControls::Construct() { return impl_->Construct(); } // 실제 원문 생성만 수행한다.
void RawProductionControls::Open() { impl_->Open(); } // 실제 생성과 첫 갱신이다.
// 예산/아이템/등록ID를 재생성하지 않고 기존 Attach의 자식 재배치만 수행한다.
void RawProductionControls::Resize(int screenHeight) {
    if (screenHeight<=80 || !impl_->rootToken) throw std::invalid_argument("생산 패널 화면 높이/수명 오류");
    const float height=static_cast<float>(screenHeight-80); // 기존 Open의 하단 예약과 같은 값이다.
    if (impl_->root.baseHeight==height) return;
    impl_->root.baseHeight=impl_->root.height=height;impl_->common.tree.Push(impl_->rootToken);impl_->Attach(impl_->rootToken,0,0,impl_->common.create.collection.root,0);
}
GumpCreateNode* RawProductionControls::Node(std::uint32_t token) const { return impl_->Node(token); } // 같은 저장소다.
GumpBackgroundNode* RawProductionControls::Background(std::uint32_t token) const { if (auto* divider=Find(impl_->dividers,token)) return divider;return Find(impl_->backgrounds,token); } // 같은 배경이다.
void RawProductionControls::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) { impl_->Attach(token,x,y,parent,mode); } // 가상+20이다.
std::uint32_t RawProductionControls::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) { return impl_->Handle(token,event,first,second); } // 가상+28이다.
void RawProductionControls::Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) { if (impl_->dividers.contains(token)) impl_->dividerInput->Broadcast(token,event,first,second); } // 같은 현재 입력/이동 처리기다.
void RawProductionControls::Idle(std::uint32_t token) { impl_->Idle(token); } // 같은 현재 idle 표다.
void RawProductionControls::ParentGone(std::uint32_t token,std::uint32_t parent) { if (impl_->grids.contains(token)) impl_->grid->ParentGone(token,parent);else impl_->common.lifetime.Kill(token); } // 가상+4를 구별한다.
void RawProductionControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) { if (impl_->hints.contains(token)) impl_->hint->DeleteScalar(token,flags);else if (impl_->Text(token)) impl_->menus.TextBackend().DeleteScalar(token,flags);else impl_->common.lifetime.DeleteScalar(token,flags); } // 같은 자식 우선 소멸이다.
void RawProductionControls::Release(std::uint32_t token) {
    auto& s=*impl_;if (token==s.rootToken) s.rootToken=0;if (token==s.budgetToken) s.budgetToken=0; // inline 주소도 scalar 해제 뒤에는 원래 토큰으로 조회할 수 없다.
    s.dividers.erase(token);s.grids.erase(token);s.items.erase(token);s.images.erase(token);s.texts.erase(token);s.backgrounds.erase(token);s.hints.erase(token);
} // 공통 Release에서만 토큰과 동적 저장소를 반납하며 Kill 단계에는 주소를 유지한다.
// 파일 팔레트 변경만 색표를 다시 만들며 일시 번개에는 재계산하지 않는다.
void RawProductionControls::SetPalette(std::span<const ScreenColor> palette,std::span<const std::uint32_t> colorSortLogical) {
    if (palette.size()!=256) throw std::invalid_argument("생산 팔레트256색 필요");std::array<std::uint32_t,256> logical{};
    // 원본 PALETTEENTRY의 성분 순서를 보존한다.
    for (std::size_t i=0;i<logical.size();++i) logical[i]=std::uint32_t(palette[i].red)|(std::uint32_t(palette[i].green)<<8)|(std::uint32_t(palette[i].blue)<<16)|(std::uint32_t(palette[i].reserved)<<24);
    auto& s=*impl_;s.colors=BuildProductionColorMaps(ColorSortLogicalInput(colorSortLogical,logical));s.shades=BuildGumpShadeMaps(ColorSortLogicalInput(colorSortLogical,logical));const auto paletteColors=BuildPaletteColorTable(logical);s.hintState.white=paletteColors.basic[7];s.hintState.black=paletteColors.basic[0];
    // 전용 HSL 표도 원본에 등록된 다섯 TYPE만 생성한다.
    for (const auto type:kSpecial) s.ownerColors[type]=BuildSpecialOwnerColorMaps(ColorSortLogicalInput(colorSortLogical,logical),type);
}
// 자동 입력도 실제 항목/내용물 SID/화면 경계를 읽는다.
std::string RawProductionControls::Report() const {
    const auto& s=*impl_;std::ostringstream out;out.precision(17);out<<"raw_production_gump\t"<<s.rootToken<<'\t'<<s.root.id<<'\t'<<s.root.width<<'\t'<<s.root.height<<'\t'<<s.budget.amount<<'\n';
    out<<"raw_production_maintenance\t"<<s.productionGlobals.stockEnabled<<'\t'<<s.productionGlobals.sequence<<'\t'<<s.productionPanel.bridgeUntil<<'\t'<<s.productionPanel.oldMoney<<'\t'<<s.productionPanel.temple<<'\t'<<s.productionPanel.slots<<'\t'<<(s.hooks.gameSeconds ? s.hooks.gameSeconds() : s.Session().Core().processState.Now())<<'\n';
    o::RawContainedFinder finder(s.context.storage->pool,s.context.gameplay->finder);
    // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 비활성 기본 실행에는 조작 행을 만들지 않는다.
    if (!s.productionGlobals.testBridgePatterns.empty()) {
        out<<"test_bridge_patterns\t"<<s.productionGlobals.testBridgeDraws;
        // 현재 지정 목록을 실제 공급 횟수와 함께 읽기 전용으로 기록한다.
        for (const auto pattern:s.productionGlobals.testBridgePatterns) out<<'\t'<<pattern;out<<'\n';
    }
    // 진단은 표시 항목 사본이 아니라 같은 현재 기술의 실제 내용물/종료값을 읽는다.
    for (auto sid=finder.Begin({static_cast<std::uint16_t>(s.Technology())});sid.value;sid=finder.Next()) {
        const auto raw=s.context.storage->pool.Slot(sid);const auto kind=Word(raw,18),flags=Word(raw,20);
        if (flags&0x40) out<<"raw_production_bridge\t"<<sid.value<<'\t'<<Word(raw,12)<<'\t'<<std::uint32_t(raw[30])<<'\t'<<std::bit_cast<std::int32_t>(Word(raw,31)|(Word(raw,33)<<16))<<'\t'<<flags<<'\n';
        if ((flags&8) && !s.productionGlobals.stockUntil.empty()) out<<"raw_production_stock_timer\t"<<kind<<'\t'<<s.productionGlobals.stockUntil.at(kind)<<'\n';
    }
    // 각 현재 항목의 원본 내용물과 실제 경계를 기록한다.
    for (const auto& [token,n]:s.items) { const auto r=s.common.collection.Bounds(token);const auto raw=s.context.storage->pool.Slot({static_cast<std::uint16_t>(n->content)});out<<"raw_production_item\t"<<token<<'\t'<<n->content<<'\t'<<Word(raw,18)<<'\t'<<r.left<<'\t'<<r.top<<'\t'<<r.right<<'\t'<<r.bottom<<'\t'<<n->serial<<'\n'; }
    out<<"raw_production_budget_flash\t"<<s.budget.flash<<'\t'<<s.budget.until<<'\n';
    // 진단도 같은 소유 문자열과 실제 만료 시각을 읽는다.
    for (const auto& [token,n]:s.hints) out<<"raw_production_hint\t"<<token<<'\t'<<n->serial<<'\t'<<n->reference<<'\t'<<n->until<<'\t'<<n->flags<<'\t'<<s.hintStrings.at(n->owned)<<'\n';
    return out.str();
}
}

// 원본 항목/이미지의 현재 경계와 실제 Hit만 조회해 UI 검사 클릭점을 반환한다.
std::optional<netstorm::client::ScreenPoint> netstorm::client::RawProductionControls::Point(std::uint32_t type) const {
    auto& s=*impl_;
    // 같은 내용물이 실제 표시된 항목인지 현재 원문으로 확인한다.
    for (const auto& [token,n]:s.items) {
        if (Word(s.context.storage->pool.Slot({static_cast<std::uint16_t>(n->content)}),18)!=type || !(n->flags&1) || (n->flags&0x10000)) continue;
        const auto r=s.common.collection.Bounds(token);const int width=r.right-r.left,height=r.bottom-r.top;
        const auto hit=[&](int x,int y) { for (auto child=n->child;child;child=s.common.node(child)->next) if (s.images.contains(child) && s.image->Hit(child,static_cast<float>(x),static_cast<float>(y))) return true;return false; }; // 실제 이미지 자식만 적중 검사한다.
        if (hit(width/2,height/2)) return ScreenPoint{r.left+width/2,r.top+height/2};
        // 가운데가 투명하면 실제 격자 영역 안의 불투명 점을 찾는다.
        for (int y=1;y<height;++y) for (int x=1;x<width;++x) if (hit(x,y)) return ScreenPoint{r.left+x,r.top+y};
    }
    return {};
}
