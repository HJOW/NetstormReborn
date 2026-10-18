// 43e640/43ddd0/43ec00을 기존 격자·항목·SHP·글자 및 현재 Session에 조립한다.
#include "client/RawProductionControls.h"
#include "client/InventoryItemGump.h"
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
struct BudgetNode: TextGumpNode { std::int32_t amount{},flash{};double until{}; }; // +a4/+b0/+a8이다.
constexpr double kFlashInterval=0.15000000596046448; // PE506858의 실제 승격 float다.
constexpr std::array<std::uint32_t,12> kStandard{82,155,158,164,135,133,156,73,170,146,144,126}; // 실제43be30의 일반 색표 대상이다.
constexpr std::array<std::uint32_t,5> kSpecial{111,141,137,94,97}; // 실제 전용 색표 대상이다.
}
// 현재 월드를 참조하는 처리기/원본 파생 필드의 단일 고정 주소 소유자다.
struct RawProductionControls::Impl {
    RawMenuTree common;RawMenuControls& menus;RawCampaignSession& session;const GameAssets& assets;RawProductionHooks hooks; // 공통 상태와 외부 장치다.
    ProductionMaintenanceGlobals& productionGlobals;ProductionMaintenancePanel& productionPanel;std::unique_ptr<RawProductionMaintenance> maintenance; // 전역/패널의 단일 상태와 같은 Session 유지 몸체다.
    GumpCreateNode root;std::uint32_t rootToken{},budgetToken{},top{},middle{},bottom{},bridgeGrid{},teleportGrid{},separator{},slotLimit{}; // 43e640의 실제 자식 포인터다.
    std::map<std::uint32_t,std::unique_ptr<InventoryGridNode>> grids;std::map<std::uint32_t,std::unique_ptr<InventoryItemNode>> items; // 같은 현재 SID 격자/항목이다.
    std::map<std::uint32_t,std::unique_ptr<ImageGumpNode>> images;std::map<std::uint32_t,std::unique_ptr<TextGumpNode>> texts; // 실제 자식 그림/수량이다.
    std::map<std::uint32_t,std::unique_ptr<GumpBackgroundNode>> backgrounds;BudgetNode budget; // 배경과 실제 예산 파생이다.
    std::uint32_t owner{},inputDisabled{},lastType{},bridgeType{82},backgroundType{},backgroundFrame{},policyFirst{},policyByte{};std::uint32_t& stock; // 현재 owner/정책과 공유하는 실제5318f4 설정이다.
    bool editor{};float pixelsX{16},pixelsY{11};ScreenPoint point{},shadow{5,5};std::uint32_t cursorFlags{}; // 현재 원본 배율/커서 전역이다.
    std::vector<std::uint32_t> ownerMaps;std::map<std::uint32_t,OwnerColorMaps> ownerColors;ProductionColorMaps colors;GumpShadeMaps shades; // 실제 원본 색표와 존재 토큰이다.
    PriestPlacementAssets geometry;o::PriestPlacementShapeState measureState;o::RawCanonPixelShape measure; // 실제 전체 Canon 경계/프레임이다.
    GumpDrawState hitClip;std::vector<std::uint8_t> hitBytes;ColorMap hitColors{};GumpPixelTarget hitTarget; // 원본 이미지 hit의 실제200×200 임시 표면이다.
    std::unique_ptr<ImageGumpPixelTarget> hitRaster;ImageGumpHooks activeImage;const GumpBackgroundDraw* activeBackground{}; // 실제 현재 출력 문맥이다.
    std::unique_ptr<InventoryGump> grid;std::unique_ptr<InventoryItemGump> item;std::unique_ptr<ImageGump> image; // 기존 원본 전체 몸체다.
    // 모든 훅은 실행 시 현재 공통 노드와 같은 Session을 재조회한다.
    Impl(RawMenuTree tree,RawMenuControls& menu,RawCampaignSession& game,const GameAssets& data,ProductionMaintenanceGlobals& globals,ProductionMaintenancePanel& panel,RawProductionHooks supplied)
        :common(std::move(tree)),menus(menu),session(game),assets(data),hooks(std::move(supplied)),productionGlobals(globals),productionPanel(panel),stock(globals.stockEnabled),ownerMaps(data.TypeTable().Types().size()),geometry(data),
        measure(game.Core().pool,data.TypeTable().Types(),geometry.frames,geometry.shapes,geometry.hotspots,geometry.geometry,measureState),hitBytes(40000,255),hitTarget(hitBytes,200,200,hitClip) {
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
        gh.idle=[this](auto token,bool enabled) { common.lifetime.SetIdle(token,enabled); };gh.now=hooks.now;gh.contentParent=[this](auto sid) { return Word(session.Core().pool.Slot({static_cast<std::uint16_t>(sid)}),16); };
        gh.forward=[this](auto token,auto event,auto first,auto second) { return common.events.Bubble(token,event,first,second); };gh.baseHandle=[](auto,auto,auto,auto) { return 0U; };
        gh.local=[this](auto token,auto,ScreenPoint p) { return common.collection.ScreenToLocal(token,p); };gh.inside=[this](auto token,auto p) { const auto& n=*common.node(token);return p[0]>=0 && p[1]>=0 && p[0]<n.width && p[1]<n.height; };
        gh.cancelCursor=[this] { session.Cursor().Cancel(); };gh.assertion=[](auto text) { throw std::logic_error(std::string(text)); };
        grid=std::make_unique<InventoryGump>(common.create,common.factory,session.Core().pool,session.ProductionFinder(),InventoryGumpState{point,cursorFlags},std::move(gh));
        InventoryItemHooks h;h.node=[this](auto token) { return Find(items,token); };h.base=common.node;BindInventoryItemAssets(h,assets,geometry,measure);
        h.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };h.clearChildren=[this](auto token) { ClearChildren(token); };h.cleanup=[this] { common.lifetime.Cleanup(); };
        h.robust=[this](auto sid,bool) { return sid<session.Core().pool.Capacity() && session.Core().pool.Slot({static_cast<std::uint16_t>(sid)})[10]==session.ProductionFinder().containedType; };h.assertion=[](auto text) { throw std::logic_error(std::string(text)); };
        h.allocate=[this](auto bytes) { const auto token=common.token();if (bytes==0xb4) images.emplace(token,std::make_unique<ImageGumpNode>());else if (bytes==0xa4) texts.emplace(token,std::make_unique<TextGumpNode>());else throw std::logic_error("생산 자식 할당 크기 오류");return token; };
        h.image=[this](auto token,auto type,auto frame,auto mode) { image->Construct(token,type,static_cast<std::uint32_t>(frame),mode); };h.text=[this](auto token) { menus.TextBackend().Construct(token,0,0,0,0); };
        h.styledText=[this](auto token,std::string_view text) { Style(token,text); };h.fitText=[this](auto token) { menus.TextBackend().Fit(token); };h.attach=common.attach;
        h.costUnits=[this](auto type) { return session.HitPoints().TypeCostUnits(type); };h.money=[this](auto who) { return session.Sp(who); };h.position=[this](auto token) { return common.collection.Position(token); };
        h.background=[this](auto rect,auto flags,auto type,auto frame) { if (!activeBackground) throw std::logic_error("생산 장식 실제 표면 없음");activeBackground->Texture(rect,flags,type,frame); };
        h.local=[this](auto token,auto,auto x,auto y) { return common.collection.ScreenToLocal(token,{std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)}); };h.knowledgeOpen=[this] { return session.CursorDropSettings().lockDepth==0 ? 1U : 0U; };
        h.imageHit=[this](auto token,float x,float y) { return image->Hit(token,x,y); };h.denied=[this](auto first,auto flags) { return first==policyFirst && static_cast<std::uint8_t>(flags)==policyByte ? 1U : 0U; };h.tell=hooks.tell;
        h.forward=[this](auto token,auto event,auto first,auto second) { return common.events.Bubble(token,event,first,second); };h.about=[](auto,auto,auto)->std::uint32_t { throw std::logic_error("생산 항목 About 메뉴 미연결"); };
        item=std::make_unique<InventoryItemGump>(session.Core().pool,InventoryItemState{owner,editor,stock,inputDisabled,lastType,bridgeType,pixelsX,pixelsY,backgroundType,backgroundFrame},std::move(h));
        maintenance=session.MakeProductionMaintenance(productionGlobals,productionPanel,hooks.now,[this](std::uint32_t mask) {
            // 원본은 텔레포트 격자를 먼저, 다리 격자를 뒤에 Push/가상부착한다.
            for (const auto& [token,bit]:std::array<std::pair<std::uint32_t,std::uint32_t>,2>{{{teleportGrid,1},{bridgeGrid,2}}}) if (mask&bit) { common.tree.Push(token);const auto& n=*grids.at(token);grid->AttachGrid(token,n.x,n.y,0,0); }common.changed();
        });
        menus.SetExternalGumps({[this](auto token) { return Text(token); },[this](auto token) { return Node(token)!=nullptr; },[this](auto token,auto& target,auto& currentColors,auto& background) { Draw(token,target,currentColors,background); }});
    }
    // 현재 원본 공유 전역은 실제 Session에서 매 가상 호출 직전에 읽는다.
    void Prepare() { owner=session.LocalOwner();editor=session.Editor()!=0;stock=static_cast<std::uint32_t>(hooks.configInt("useProductTimers",1));productionGlobals.bridgeType=session.Core().bridgeState.bridgeType;point={session.CursorDropSettings().screenX,session.CursorDropSettings().screenY};cursorFlags=session.CursorDropSettings().inputFlags; }
    // 외부 기본 생성자/수명은 같은 파생 기본 주소만 본다.
    GumpCreateNode* Node(std::uint32_t token) const {
        if (token==rootToken && token) return const_cast<GumpCreateNode*>(&root);if (token==budgetToken && token) return const_cast<BudgetNode*>(&budget);
        if (auto* n=Find(grids,token)) return n;if (auto* n=Find(items,token)) return n;if (auto* n=Find(images,token)) return n;if (auto* n=Find(texts,token)) return n;return Find(backgrounds,token);
    }
    // 실제 TextGump 파생만 같은 글자 처리기에 공급한다.
    TextGumpNode* Text(std::uint32_t token) const { return token==budgetToken && token ? const_cast<BudgetNode*>(&budget) : Find(texts,token); }
    // 삭제 중 머리가 바뀌므로 매번 현재 실제 child를 읽는다.
    void ClearChildren(std::uint32_t token) { while (common.node(token)->child) common.lifetime.Kill(common.node(token)->child); }
    // 4644e0은0부모에 무동작이며 이전 Form을 먼저 지우지 않는다.
    void Bind(std::uint32_t token,std::uint32_t object) {
        if (!object) return;auto& forms=session.GumpForms();const auto form=forms.Create(2);auto& n=*common.node(token);forms.SetOwner(form,n.id);forms.Attach(form,{static_cast<std::uint16_t>(object)},0);n.flags|=0x40;n.object=object;n.form=form.value;
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
        const auto token=common.token();backgrounds.emplace(token,std::make_unique<GumpBackgroundNode>());menus.BackgroundBackend().Construct(token,width,height,flags,1,backgroundType,frame);return token;
    }
    // 원본43e640의 생성 순서와 실제 두 격자/예산/배경 자식이다.
    void Open() {
        Prepare();rootToken=common.token();common.factory.Construct(rootToken,0,0,0,1);root.vtable=0x5069c0;root.command=44;root.flags|=0xe00;root.baseWidth=root.width=40;root.baseHeight=root.height=static_cast<float>(common.create.screenHeight);common.lifetime.SetIdle(rootToken,true);
        budgetToken=common.token();menus.TextBackend().Construct(budgetToken,0,0,0,0);budget.vtable=0x506948;common.lifetime.SetIdle(budgetToken,true);budget.alignFlags|=4;common.tree.Attach(budgetToken,0,0,rootToken,0);BudgetText();
        top=Background(40,root.baseHeight,4);auto& a=*backgrounds.at(top);a.alignFlags|=1;a.baseHeight=a.height=23;common.tree.Attach(top,0,0,rootToken,0);
        if (!productionPanel.initialized) { productionPanel.bridgeUntil=hooks.now();productionPanel.initialized=true; } // 원본43e640은+c8을 생성 시 현재 벽시각으로 초기화한다.
        slotLimit=static_cast<std::uint32_t>(session.BattleOption(1))*2+2;productionPanel.slots=static_cast<std::int32_t>(slotLimit);bridgeGrid=common.token();grids.emplace(bridgeGrid,std::make_unique<InventoryGridNode>());grid->ConstructGrid(bridgeGrid,2,static_cast<std::int32_t>(slotLimit/2),0x40);auto& b=*grids.at(bridgeGrid);b.fixed=1;b.baseHeight=b.height=static_cast<float>(slotLimit*36/2);b.acceptedCursor=0x40;b.alignFlags|=1;b.itemScale=std::bit_cast<float>(0x3f2147aeU);common.attach(bridgeGrid,0,26,rootToken,0);
        middle=Background(40,root.baseHeight,2);auto& c=*backgrounds.at(middle);c.alignFlags|=1;c.baseHeight=c.height=b.baseHeight;common.tree.Attach(middle,0,26,rootToken,0);
        teleportGrid=common.token();grids.emplace(teleportGrid,std::make_unique<InventoryGridNode>());grid->ConstructGrid(teleportGrid,1,6,8);auto& d=*grids.at(teleportGrid);d.baseHeight=d.height=250;d.acceptedCursor=8;d.alignFlags|=1;common.attach(teleportGrid,0,c.y+c.baseHeight,rootToken,0);
        bottom=Background(40,root.baseHeight,2);auto& e=*backgrounds.at(bottom);e.alignFlags|=1;e.baseHeight=e.height=250;common.tree.Attach(bottom,0,d.y,rootToken,0);
        separator=Background(root.baseWidth,5,0,0x2a00000);auto& f=*backgrounds.at(separator);f.vtable=0x514fd8;f.command=55;f.flags|=0x60200;f.alignFlags|=1;common.lifetime.SetOverlay(separator,true);common.tree.Attach(separator,0,21,rootToken,0);
        // 원본 상위 프레임의 초기 왼쪽80/하단400 배치에 대응한다. 드래그 프레임 자체는 후속 복원이다.
        root.baseWidth=root.width=80;root.baseHeight=root.height=static_cast<float>(common.create.screenHeight-80);Attach(rootToken,0,0,common.create.collection.root,0);Handle(rootToken,0x0200003d,1,0);
        // Client는 설치가 끝난 월드에 패널을 붙이므로 기존 내용물도 현재 원본 격자로 재부착한다.
        for (const auto current:{bridgeGrid,teleportGrid}) { common.tree.Push(current);const auto& n=*grids.at(current);grid->AttachGrid(current,n.x,n.y,0,0); }
        common.changed();
    }
    // 원본43ddd0의 Push/현재 크기/재부착/깊이 순서를 보존한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        Prepare();if (grids.contains(token)) { grid->AttachGrid(token,x,y,parent,mode);return; }if (items.contains(token)) { item->Attach(token,x,y,parent,mode);return; }if (images.contains(token)) { image->Attach(token,x,y,parent,mode);return; }
        common.tree.Attach(token,x,y,parent,mode);if (token!=rootToken) return;
        common.tree.Push(budgetToken);common.tree.Attach(budgetToken,0,2,rootToken,2);common.tree.Push(bottom);auto& e=*backgrounds.at(bottom);e.baseHeight=root.baseHeight-e.y;e.height=e.baseHeight*e.scale;common.tree.Attach(bottom,e.x,e.y,0,0);
        common.tree.Push(teleportGrid);auto& d=*grids.at(teleportGrid);d.baseHeight=root.baseHeight-d.y;d.height=d.baseHeight*d.scale;const auto& c=*backgrounds.at(middle);grid->AttachGrid(teleportGrid,0,c.y+c.baseHeight,rootToken,0);
        ++backgrounds.at(top)->depth;budget.depth=backgrounds.at(top)->depth-1;++backgrounds.at(middle)->depth;++backgrounds.at(bottom)->depth;
    }
    // 클릭된 실제 내용물 SID를 그대로447000에 넘긴다. 실패 피드백의 미복원은 명시 거부한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
        Prepare();if (grids.contains(token)) return grid->HandleGrid(token,event,first,second);if (items.contains(token)) return item->Handle(token,event,first,second);if (token!=rootToken) return 0;
        const auto code=event&0x03ffffffU;
        if (code==0x02000008) {
            const auto raw=session.Core().pool.Slot({static_cast<std::uint16_t>(first)});if ((Word(raw,20)&0x100) || session.CursorDropSettings().lockDepth==0) return 1;
            if (Word(raw,20)&8) {
                if (!item->Affordable({static_cast<std::uint16_t>(first)})) { budget.flash=10;budget.until=hooks.now()+kFlashInterval;throw std::logic_error("생산 SP 부족 원본 tooltip 미연결"); }
                if (stock && !raw[30]) throw std::logic_error("생산 재고 없음 원본 tooltip 미연결");
            }
            if (!session.Core().types[Word(raw,18)].flags2) return 1;session.Cursor().Take({static_cast<std::uint16_t>(first)},(Word(raw,20)&8) ? 0x41U : 1U);hooks.sound("getPiece.wav");common.changed();return 1;
        }
        if (code==0x0200003d) { Bind(bridgeGrid,session.CurrentTechnologyParent());Bind(teleportGrid,session.CurrentTechnologyParent());if (first==1) session.SynchronizeProductionContents();return 0; }
        if (code==0x020000f4) {
            o::RawContainedFinder finder(session.Core().pool,session.ProductionFinder());
            // 실제 현재 기술의flags40/슬롯 BYTE가 같은 항목을 순서대로 찾는다.
            for (auto sid=finder.Begin({static_cast<std::uint16_t>(session.CurrentTechnologyParent())});sid.value;sid=finder.Next()) { const auto raw=session.Core().pool.Slot(sid);if ((Word(raw,20)&0x40) && raw[23]==first) { session.Cursor().Take(sid,0);hooks.sound("getPiece.wav");return 1; } }
        }
        return 0;
    }
    // 43e530의 금액 표시와43ef10의 dirty/부모 재연결→공급/내용물 유지 순서다.
    void Idle(std::uint32_t token) {
        Prepare();if (token==budgetToken) {
            const auto target=session.PlayerKnown() ? static_cast<std::int32_t>(session.Sp(owner)) : 0;
            if (budget.flash && budget.until<=hooks.now()) { --budget.flash;budget.until=hooks.now()+kFlashInterval;BudgetText(); }
            if (budget.amount!=target) { const auto distance=static_cast<std::int64_t>(target)-budget.amount;budget.amount+=distance<0 ? (distance<-300 ? -100 : distance<-30 ? -10 : -1) : (distance>300 ? 100 : distance>30 ? 10 : 1);if (session.Core().ownerMode.fort) budget.amount=target;BudgetText(); }
        } else if (token==rootToken) {
            if (session.UpdateProductionContents()) common.changed();
            const auto parent=session.CurrentTechnologyParent();const auto& mode=session.Core().ownerMode;if (!parent || (!mode.fort && !mode.battle) || mode.challenge) return;
            // 원본은 실제 부모가 바뀐 격자만 Push/Bind/재부착한다.
            if (!editor) for (const auto current:{bridgeGrid,teleportGrid}) if (grids.at(current)->object!=parent) { common.tree.Push(current);Bind(current,parent);auto& n=*grids.at(current);grid->AttachGrid(current,n.x,n.y,0,0); }
            maintenance->Run();
        } else if (auto* n=Find(items,token)) {
            if (!n->blink) { common.lifetime.SetIdle(token,false);n->flags|=1;common.changed();auto raw=session.Core().pool.AllocatedBytes({static_cast<std::uint16_t>(n->content)});raw[21]&=0xfd; }
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
        Prepare();if (Text(token)) { menus.TextBackend().Draw(token);return; }if (backgrounds.contains(token)) { background.Draw(token);return; }
        if (items.contains(token)) { activeBackground=&background;try { item->Draw(token); } catch (...) { activeBackground=nullptr;throw; }activeBackground=nullptr;return; }
        if (images.contains(token)) { ImageGumpPixelTarget raster(target,assets,current.vfxColors,[this](auto color) { return Color(color); });activeImage=raster.Hooks({});try { image->Draw(token); } catch (...) { activeImage={};throw; }activeImage={}; }
    }
};
// 공통 분배가 호출하기 전에 저장소/훅을 준비하고 Open은 소유자 설치 뒤 실행한다.
RawProductionControls::RawProductionControls(RawMenuTree tree,RawMenuControls& menus,RawCampaignSession& session,const GameAssets& assets,ProductionMaintenanceGlobals& globals,ProductionMaintenancePanel& panel,RawProductionHooks hooks):impl_(std::make_unique<Impl>(std::move(tree),menus,session,assets,globals,panel,std::move(hooks))) {} // 같은 상태를 빌리고 별도 사본을 만들지 않는다.
void RawProductionControls::Close() { if (impl_->rootToken) { impl_->common.lifetime.Kill(impl_->rootToken);impl_->common.lifetime.Cleanup();impl_->rootToken=0; } } // 같은 자식 우선 수명으로 닫는다.
RawProductionControls::~RawProductionControls() { impl_->menus.SetExternalGumps({}); } // 공유 출력의 참조부터 끊는다.
void RawProductionControls::Open() { impl_->Open(); } // 실제 생성과 첫 갱신이다.
GumpCreateNode* RawProductionControls::Node(std::uint32_t token) const { return impl_->Node(token); } // 같은 저장소다.
GumpBackgroundNode* RawProductionControls::Background(std::uint32_t token) const { return Find(impl_->backgrounds,token); } // 같은 배경이다.
void RawProductionControls::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) { impl_->Attach(token,x,y,parent,mode); } // 가상+20이다.
std::uint32_t RawProductionControls::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) { return impl_->Handle(token,event,first,second); } // 가상+28이다.
void RawProductionControls::Idle(std::uint32_t token) { impl_->Idle(token); } // 같은 현재 idle 표다.
void RawProductionControls::ParentGone(std::uint32_t token,std::uint32_t parent) { if (impl_->grids.contains(token)) impl_->grid->ParentGone(token,parent);else impl_->common.lifetime.Kill(token); } // 가상+4를 구별한다.
void RawProductionControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) { if (impl_->Text(token)) impl_->menus.TextBackend().DeleteScalar(token,flags);else impl_->common.lifetime.DeleteScalar(token,flags); } // 같은 자식 우선 소멸이다.
void RawProductionControls::Release(std::uint32_t token) { auto& s=*impl_;s.grids.erase(token);s.items.erase(token);s.images.erase(token);s.texts.erase(token);s.backgrounds.erase(token); } // 실제 release만 저장소를 반납한다.
// 파일 팔레트 변경만 색표를 다시 만들며 일시 번개에는 재계산하지 않는다.
void RawProductionControls::SetPalette(std::span<const ScreenColor> palette) {
    if (palette.size()!=256) throw std::invalid_argument("생산 팔레트256색 필요");std::array<std::uint32_t,256> logical{};
    // 원본 PALETTEENTRY의 성분 순서를 보존한다.
    for (std::size_t i=0;i<logical.size();++i) logical[i]=std::uint32_t(palette[i].red)|(std::uint32_t(palette[i].green)<<8)|(std::uint32_t(palette[i].blue)<<16)|(std::uint32_t(palette[i].reserved)<<24);
    auto& s=*impl_;s.colors=BuildProductionColorMaps(logical);s.shades=BuildGumpShadeMaps(logical);
    // 전용 HSL 표도 원본에 등록된 다섯 TYPE만 생성한다.
    for (const auto type:kSpecial) s.ownerColors[type]=BuildSpecialOwnerColorMaps(logical,type);
}
// 자동 입력도 실제 항목/내용물 SID/화면 경계를 읽는다.
std::string RawProductionControls::Report() const {
    const auto& s=*impl_;std::ostringstream out;out.precision(17);out<<"raw_production_gump\t"<<s.rootToken<<'\t'<<s.root.id<<'\t'<<s.root.width<<'\t'<<s.root.height<<'\t'<<s.budget.amount<<'\n';
    out<<"raw_production_maintenance\t"<<s.productionGlobals.stockEnabled<<'\t'<<s.productionGlobals.sequence<<'\t'<<s.productionPanel.bridgeUntil<<'\t'<<s.productionPanel.oldMoney<<'\t'<<s.productionPanel.temple<<'\t'<<s.productionPanel.slots<<'\t'<<s.session.Core().processState.now<<'\n';
    o::RawContainedFinder finder(s.session.Core().pool,s.session.ProductionFinder());
    // 진단은 표시 항목 사본이 아니라 같은 현재 기술의 실제 내용물/종료값을 읽는다.
    for (auto sid=finder.Begin({static_cast<std::uint16_t>(s.session.CurrentTechnologyParent())});sid.value;sid=finder.Next()) {
        const auto raw=s.session.Core().pool.Slot(sid);const auto kind=Word(raw,18),flags=Word(raw,20);
        if (flags&0x40) out<<"raw_production_bridge\t"<<sid.value<<'\t'<<Word(raw,12)<<'\t'<<std::uint32_t(raw[30])<<'\t'<<std::bit_cast<std::int32_t>(Word(raw,31)|(Word(raw,33)<<16))<<'\t'<<flags<<'\n';
        if ((flags&8) && !s.productionGlobals.stockUntil.empty()) out<<"raw_production_stock_timer\t"<<kind<<'\t'<<s.productionGlobals.stockUntil.at(kind)<<'\n';
    }
    // 각 현재 항목의 원본 내용물과 실제 경계를 기록한다.
    for (const auto& [token,n]:s.items) { const auto r=s.common.collection.Bounds(token);const auto raw=s.session.Core().pool.Slot({static_cast<std::uint16_t>(n->content)});out<<"raw_production_item\t"<<token<<'\t'<<n->content<<'\t'<<Word(raw,18)<<'\t'<<r.left<<'\t'<<r.top<<'\t'<<r.right<<'\t'<<r.bottom<<'\t'<<n->serial<<'\n'; }
    return out.str();
}
}

// 원본 항목/이미지의 현재 경계와 실제 Hit만 조회해 UI 검사 클릭점을 반환한다.
std::optional<netstorm::client::ScreenPoint> netstorm::client::RawProductionControls::Point(std::uint32_t type) const {
    auto& s=*impl_;
    // 같은 내용물이 실제 표시된 항목인지 현재 원문으로 확인한다.
    for (const auto& [token,n]:s.items) {
        if (Word(s.session.Core().pool.Slot({static_cast<std::uint16_t>(n->content)}),18)!=type || !(n->flags&1) || (n->flags&0x10000)) continue;
        const auto r=s.common.collection.Bounds(token);const int width=r.right-r.left,height=r.bottom-r.top;
        const auto hit=[&](int x,int y) { for (auto child=n->child;child;child=s.common.node(child)->next) if (s.images.contains(child) && s.image->Hit(child,static_cast<float>(x),static_cast<float>(y))) return true;return false; }; // 실제 이미지 자식만 적중 검사한다.
        if (hit(width/2,height/2)) return ScreenPoint{r.left+width/2,r.top+height/2};
        // 가운데가 투명하면 실제 격자 영역 안의 불투명 점을 찾는다.
        for (int y=1;y<height;++y) for (int x=1;x<width;++x) if (hit(x,y)) return ScreenPoint{r.left+x,r.top+y};
    }
    return {};
}
