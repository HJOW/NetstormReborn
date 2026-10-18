// 독립 원본132행의 항목·실제 그림/TextGump 생성·클릭·비용·장식 전체를 대조한다.
#include "GumpRawSupport.h"
#include "DudeSpawnSupport.h"
#include "client/InventoryItemGump.h"
#include "client/ImageGump.h"
#include "client/TextGump.h"
#include "client/GumpTree.h"

namespace {
using namespace netstorm::test::gumpraw;
constexpr std::uint32_t kItem=0x12401000,kBorn=0x12404000; // 원본 관측과 같은 안정 토큰이다.
// 원본이 쓰지 않는 필드도 임의 바이트로 남긴다.
GumpCreateNode InitialBase() { return Decode(std::vector<std::uint8_t>(160,0xa5)); }
// 실제 항목0xc0바이트를 원문 offset에 맞춰 기록한다.
std::string EncodeItem(const InventoryItemNode& n) {
    auto raw=Bytes(Encode(n));raw.resize(192);Put(raw,160,n.content);Put(raw,164,n.reservedA4);const auto time=std::bit_cast<std::uint64_t>(n.blinkUntil);Put(raw,168,static_cast<std::uint32_t>(time));Put(raw,172,static_cast<std::uint32_t>(time>>32));raw[176]=n.blink;raw[177]=n.decoration;Put(raw,178,n.reservedB2,2);Put(raw,180,n.requireImageHit);Put(raw,184,n.additional);Put(raw,188,Bits(n.iconScale));return Hex(raw);
}
// 실제 생성한 이미지0xb4바이트만 기록한다.
std::string EncodeImage(const ImageGumpNode& n) {
    auto raw=Bytes(Encode(n));raw.resize(180);Put(raw,160,n.type);Put(raw,164,n.frame);Put(raw,168,n.mode);Put(raw,172,Bits(n.hotspotX));Put(raw,176,Bits(n.hotspotY));return Hex(raw);
}
// 원본과 같은16개 프레임 코드 입력이다. 품질 프레임의 기대 검색 결과를 계산하지 않는다.
RiftTypeFrames Frames() {
    std::vector<FrameCode> codes(16,{'A','P',0,0});codes[8].side='B';codes[9].flags=0x20;codes[10]={'B','P',0,0x20};codes[11].flags=0x40;codes[12]={'B','P',0,0x40};return RiftTypeFrames(std::move(codes));
}
// 같은 풀/등록표·실제 이미지와TextGump 생성자를 사용하는 독립 입력 장면이다.
struct ItemScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true};GumpCreateState create; // 실제 내용물과 단일 Gump 등록표다.
    InventoryItemNode item;InventoryGridNode grid;std::map<std::uint32_t,ImageGumpNode> images;std::map<std::uint32_t,TextGumpNode> texts;std::map<std::uint32_t,GumpCreateNode> bases; // 안정된 같은 노드 저장소다.
    std::vector<std::pair<std::uint32_t,std::size_t>> born; // 실제 할당 순서와 크기다.
    RiftTypeFrames frames=Frames();std::int32_t gumpFrame{-1},cost{800};std::uint32_t owner{1},stock{},disabled{},last{},bridge{82},background{96},backgroundFrame{2};bool editor{}; // 원문 입력이다.
    float cellX{16},cellY{11},money{800};std::uint32_t bounds{},cells{},opened{},secondOpen{},knowledgeCalls{},children{},denied{},forward{},robust{1}; // 명시 조회 응답이다.
    ScreenPoint point{17,-9},shadow{5,5};std::array<std::uint32_t,188> ownerMaps{}; // 그림의 같은 현재 전역이다.
    std::string trace;InventoryItemHooks hooks;GumpCreate factory;GumpLifetimeState lifeState;StyleTextState fragments;StyleText style;GumpDrawState clip;TextGumpState textState; // 실제 기존 기본/텍스트 처리기다.
    std::unique_ptr<GumpLifetime> life;std::unique_ptr<GumpCollection> collection;std::unique_ptr<ImageGump> image;std::unique_ptr<TextGump> text; // 저장소보다 먼저 해제한다.
    // 원본과 같은 실제 생성자를 준비하고 명시 외부 경계만 관측한다.
    ItemScene():factory(create,{[this](auto token) { return Base(token); },{}}),style(fragments,{},OriginalEdition::Patch1078) {
        create.serial=23;static_cast<GumpCreateNode&>(item)=InitialBase();item.vtable=0x5068f4;item.flags=9;item.child=0;item.serial=41;item.content=128;item.reservedA4=item.requireImageHit=item.additional=0xa5a5a5a5;item.blinkUntil=std::bit_cast<double>(0xa5a5a5a5a5a5a5a5ULL);item.blink=item.decoration=0xa5;item.reservedB2=0xa5a5;item.iconScale=Float(0xa5a5a5a5);
        auto raw=InputRaw(pool,Sid{128});Put(raw,18,128,2);collection=std::make_unique<GumpCollection>(create.collection,factory.CollectionHooks());GumpLifetimeHooks lh;lh.node=[this](auto token) { return Base(token); };life=std::make_unique<GumpLifetime>(create,lifeState,lh);
        ImageGumpHooks ih;ih.node=[this](auto token)->ImageGumpNode* { return images.contains(token) ? &images.at(token) : nullptr; };ih.type=[](auto) { return ImageGumpType{0x16000000,2,3}; };
        ih.header=[this](auto kind,auto frame) { Event("Header",{kind,0,frame});return ImageGumpHeader{40,60,12,-7}; };image=std::make_unique<ImageGump>(create,factory,ImageGumpState{owner,ownerMaps,shadow},ih);
        text=std::make_unique<TextGump>(create,textState,factory,*life,*collection,clip,style,TextGumpHooks{[this](auto token)->TextGumpNode* { return texts.contains(token) ? &texts.at(token) : nullptr; },[this](auto token) { return Base(token); }});
        hooks.node=[this](auto token) { return token==kItem ? &item : nullptr; };hooks.base=[this](auto token) { return Base(token); };hooks.type=[this](auto) { return InventoryItemType{gumpFrame,&frames}; };
        hooks.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { Event("Attach",{token,Bits(x),Bits(y),parent,mode}); };
        hooks.clearChildren=[this](auto) { Event("Clear"); };hooks.cleanup=[this] { Event("Cleanup"); };hooks.robust=[this](auto sid,bool allow) { Event("Robust",{sid,allow ? 1U : 0U});return robust!=0; };
        hooks.assertion=[this](std::string_view message) { Event("Assert,"+Hex({reinterpret_cast<const std::uint8_t*>(message.data()),message.size()})); };
        hooks.measure=[this](auto kind,auto argument,bool explicitFrame) { Event("Measure",{kind,argument,0,explicitFrame ? 1U : 0U});constexpr std::array<std::array<float,4>,3> rects{{{0,0,64,96},{-10,-7,10,30},{1000,1000,0,0}}};const auto& r=rects.at(bounds);return PriestPlacementPixelShape{{r[0],r[1],r[2],r[3]},16,11}; };
        hooks.walk=[this](CanonTypeQuery query,const std::function<void(InventoryItemCell)>& visit) {
            Event("Walk",{query.type,static_cast<std::uint32_t>(query.argument),static_cast<std::uint32_t>(query.direction),Bits(query.x),Bits(query.y),query.explicitFrame ? 1U : 0U});
            const std::array<InventoryItemCell,2> data{{{7,cells==2 ? -0.5F : 0.0F,cells==2 ? 2.25F : 0.0F},{8,1.5F,-1.0F}}};
            // 명시 decoder 경계 입력만 순서대로 전달한다.
            for (std::uint32_t i=0;i<cells;++i) { visit(data[i]);Event("Advance"); }
        };
        hooks.allocate=[this](auto size) { const auto token=kBorn+static_cast<std::uint32_t>(born.size())*256;born.emplace_back(token,size);Event("New",{static_cast<std::uint32_t>(size),token});if (size==180) { auto& n=images[token];static_cast<GumpCreateNode&>(n)=InitialBase(); }else { CHECK(size==164);auto& n=texts[token];static_cast<GumpCreateNode&>(n)=InitialBase();n.head=0xa5a5a5a5; }return token; };
        hooks.image=[this](auto token,auto kind,auto frame,auto mode) { image->Construct(token,kind,static_cast<std::uint32_t>(frame),mode); };hooks.text=[this](auto token) { text->Construct(token,0,0,0,0); };
        hooks.styledText=[this](auto token,std::string_view value) { Event("Text,"+std::to_string(token)+','+Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()})); };hooks.fitText=[this](auto token) { Event("Fit",{token,0,0}); };hooks.attach=hooks.baseAttach;
        hooks.costUnits=[this](auto) { return cost; };hooks.money=[this](auto ownerValue) { Event("Money",{ownerValue});return money; };hooks.position=[this](auto) { Event("Position");return point; };
        hooks.background=[this](ScreenRect r,auto flags,auto type,auto frame) { Event("Background",{static_cast<std::uint32_t>(r.left),static_cast<std::uint32_t>(r.top),static_cast<std::uint32_t>(r.right),static_cast<std::uint32_t>(r.bottom),flags,type,frame}); };
        hooks.local=[this](auto,auto event,auto first,auto second) { Event("Local",{event,first,second});return std::array<float,2>{7.5F,-3.25F}; };
        hooks.knowledgeOpen=[this] { const auto result=knowledgeCalls ? secondOpen : opened;++knowledgeCalls;Event("Knowledge",{result});return result; };
        hooks.imageHit=[this](auto token,float x,float y) { Event("ImageHit",{token,Bits(x),Bits(y)});return token==0x12403200U || children!=3 ? 1U : 0U; };
        hooks.denied=[this](auto first,auto flags) { Event("Denied",{first,flags});return denied; };hooks.forward=[this](auto,auto event,auto first,auto second) { Event("Forward",{event,first,second});return forward; };
        hooks.tell=[this](std::string_view value) { Event("Tell,"+Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()})); };hooks.about=[this](auto,auto first,auto second) { Event("About",{first,second});return 0x80000009U; };
    }
    // 단일 생성/트리 처리기에 모든 파생의 같은 기본 주소를 공급한다.
    GumpCreateNode* Base(std::uint32_t token) {
        if (token==kItem) return &item;if (token==43) return &grid;if (images.contains(token)) return &images.at(token);if (texts.contains(token)) return &texts.at(token);if (bases.contains(token)) return &bases.at(token);return nullptr;
    }
    // 실제 인자를 원문 구분자로만 기록한다.
    void Event(std::string value,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=value;
        // 전체 DWORD 비트를 부호 해석 없이 보존한다.
        for (const auto number:values) trace+=','+std::to_string(number);
    }
    // 같은 실제 원본 현재 상태의 참조 묶음이다.
    InventoryItemState State() { return {owner,editor,stock,disabled,last,bridge,cellX,cellY,background,backgroundFrame}; }
};
}

// 전체 원본의 생성 항목/이미지/TextGump 바이트와 호출·인자·순서/반환을 대조한다.
TEST_CASE(InventoryItemGump_Native132) {
    std::size_t count{};
    // 각 행에서 현재 실제 파생 저장소와 공통 생성자를 새로 조립한다.
    for (const auto& row:LoadFixture(NETSTORM_INVENTORY_ITEM_FIXTURE).rows) {
        CHECK(row.size()==7);ItemScene s;const auto a=Inputs(row[1]);InventoryItemGump item(s.pool,s.State(),s.hooks);std::string result="-";auto raw=InputRaw(s.pool,Sid{128});
        if (row[0]=="Attach") {
            Put(raw,18,a[0],2);Put(raw,20,a[1],2);Put(raw,12,a[2],2);raw[30]=static_cast<std::uint8_t>(a[3]);s.gumpFrame=std::bit_cast<std::int32_t>(a[4]);s.item.width=Float(a[5]);s.item.height=Float(a[6]);s.item.iconScale=Float(a[7]);s.stock=a[8];s.cells=a[9];s.bounds=a[10];s.money=Float(a[11]);s.editor=a[12]!=0;s.item.flags=9|(a[13] ? 0x10000U : 0U);s.robust=a[14];item.Attach(kItem,5,6,0,7);
        } else if (row[0]=="Handle") {
            s.opened=a[1];s.disabled=a[2];s.item.requireImageHit=a[3];s.children=a[4];s.denied=a[5];s.forward=a[6];s.last=a[7];s.secondOpen=a[8];
            if (s.children) {
                const std::array<std::uint32_t,3> tokens{0x12403000,0x12403100,0x12403200};const auto n=s.children==3 ? 3U : 1U;s.item.child=tokens[0];
                // 나머지 자식 필드는 원본과 같은 임의 기본 입력이다.
                for (std::uint32_t i=0;i<n;++i) { auto& node=s.bases[tokens[i]];node=InitialBase();node.next=i+1<n ? tokens[i+1] : 0;node.command=((s.children!=2 && i!=0) || s.children==1) ? 3U : 45U; }
            }
            result=std::to_string(item.Handle(kItem,a[0],17,0xfffffff7));
        } else if (row[0]=="Draw") { s.item.decoration=static_cast<std::uint8_t>(a[0]);s.item.baseWidth=Float(a[1]);s.item.baseHeight=Float(a[2]);item.Draw(kItem); }
        else { CHECK(row[0]=="Afford");s.cost=std::bit_cast<std::int32_t>(a[0]);s.money=Float(a[1]);s.editor=a[2]!=0;result=std::to_string(item.Affordable(Sid{128})); }
        std::string nodes;
        // 모든 실제 생성 노드를 원본 할당 순서대로 기록한다.
        for (const auto& [token,size]:s.born) { if (!nodes.empty()) nodes+=';';nodes+=std::to_string(token)+'=';if (size==180) nodes+=EncodeImage(s.images.at(token));else { auto data=Bytes(Encode(s.texts.at(token)));data.resize(164);Put(data,160,s.texts.at(token).head);nodes+=Hex(data); } }
        const auto encoded=EncodeItem(s.item),trace=s.trace.empty() ? std::string("-") : s.trace;if (result!=row[2] || encoded!=row[3] || std::to_string(s.last)!=row[4] || (nodes.empty() ? "-" : nodes)!=row[5] || trace!=row[6]) std::printf("생산항목 원문%zu %s result%d item%d last%d nodes%d trace%d\n실제:%s\n원본:%s\n",count,row[0].c_str(),result==row[2],encoded==row[3],std::to_string(s.last)==row[4],(nodes.empty() ? "-" : nodes)==row[5],trace==row[6],trace.c_str(),row[6].c_str());
        CHECK(result==row[2]);CHECK(encoded==row[3]);CHECK(std::to_string(s.last)==row[4]);CHECK((nodes.empty() ? "-" : nodes)==row[5]);CHECK(trace==row[6]);++count;
    }
    CHECK(count==132);
}

// 실제 자산의 전체 패턴 측정→이미지 자식→현재 트리/적중→같은 내용물SID 전달을 검사한다.
TEST_CASE(InventoryItemGump_ActualAssetsTreePixelsAndClick) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game;
    // 각 실제 자산을 같은 원본 내용물→격자→항목→그림 경로에 독립 공급한다.
    for (const std::uint32_t kind:{133U,128U}) {
        ItemScene s;PriestPlacementAssets geometry(assets);PriestPlacementShapeState scales;RawCanonPixelShape measure(s.pool,assets.TypeTable().Types(),geometry.frames,geometry.shapes,geometry.hotspots,geometry.geometry,scales);BindInventoryItemAssets(s.hooks,assets,geometry,measure);
        s.create.screenWidth=s.create.screenHeight=200;s.bases[42]={};s.factory.Construct(42,0,200,200,1);s.create.collection.root=42;s.item.form=0;
        GumpTreeState treeState;GumpTree* current{};GumpLifetime* currentLife{};GumpTreeHooks th;th.node=[&](auto token) { return s.Base(token); };th.redraw=[](auto,bool) {};th.push=[&](auto token) { current->Push(token); };th.kill=[&](auto token) { currentLife->Kill(token); };th.attach=[&](auto token,float x,float y,auto parent,auto mode) { current->Attach(token,x,y,parent,mode); };GumpTree tree(s.create,treeState,th);current=&tree;
        GumpLifetimeState ls;GumpLifetimeHooks lh;lh.node=th.node;lh.push=th.push;lh.deletingDestructor=[&](auto token,auto flags) { currentLife->DeleteScalar(token,flags); };lh.release=[&](auto token) { s.images.erase(token);s.texts.erase(token); };GumpLifetime life(s.create,ls,lh);currentLife=&life;
        const auto clear=[&](auto token) { while (s.Base(token)->child) life.Kill(s.Base(token)->child); };s.hooks.baseAttach=th.attach;s.hooks.attach=th.attach;s.hooks.clearChildren=clear;s.hooks.cleanup=[&] { life.Cleanup(); };s.hooks.local=[&](auto token,auto,auto x,auto y) { return s.collection->ScreenToLocal(token,{std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)}); };
        s.hooks.allocate=[&](auto size) { CHECK(size==180);const auto token=kBorn+static_cast<std::uint32_t>(s.born.size())*256;s.born.emplace_back(token,size);s.images.emplace(token,ImageGumpNode{});return token; };
        std::vector<std::uint8_t> pixels(40000,255);GumpDrawState clip;SetGumpDrawClip(clip,{0,0,200,200});GumpPixelTarget target(pixels,200,200,clip);ColorMap colors{};ImageGumpHooks ih;ih.node=[&](auto token)->ImageGumpNode* { return s.images.contains(token) ? &s.images.at(token) : nullptr; };BindImageGumpAssets(ih,assets);ih.position=[&](auto token) { return s.collection->Position(token); };ImageGumpPixelTarget raster(target,assets,colors,[](auto)->ColorMap { throw std::logic_error("검사 현재색표 미연결"); });ih=raster.Hooks(std::move(ih));ImageGump image(s.create,s.factory,{s.owner,s.ownerMaps,s.shadow},ih);
        s.hooks.image=[&](auto token,auto type,auto frame,auto mode) { image.Construct(token,type,static_cast<std::uint32_t>(frame),mode); };s.hooks.imageHit=[&](auto token,float x,float y) { return image.Hit(token,x,y); };s.money=99999;s.stock=0;
        std::array<std::uint32_t,3> selected{};s.hooks.forward=[&](auto,auto event,auto sid,auto serial) { selected={event,sid,serial};return 17U; };InventoryItemGump item(s.pool,s.State(),s.hooks);
        ContainedFinderState finder;std::uint32_t cursor{};InventoryGumpHooks gh;gh.grid=[&](auto token) { return token==43 ? &s.grid : nullptr; };gh.item=[&](auto token) { return token==kItem ? &s.item : nullptr; };gh.clearChildren=clear;gh.baseAttach=th.attach;gh.allocateItem=[&](auto size) { CHECK(size==192);return kItem; };gh.attach=[&](auto token,float x,float y,auto parent,auto mode) { item.Attach(token,x,y,parent,mode); };InventoryGump grid(s.create,s.factory,s.pool,finder,{s.point,cursor},gh);
        auto raw=InputRaw(s.pool,Sid{50});Put(raw,6,128,2);raw=InputRaw(s.pool,Sid{128});raw[10]=6;Put(raw,4,0,2);Put(raw,18,kind,2);Put(raw,20,8,2);Put(raw,12,0,2);raw[30]=3;raw[23]=0;
        grid.ConstructGrid(43,1,6,8);s.grid.object=50;s.grid.width=s.grid.baseWidth=40;s.grid.height=s.grid.baseHeight=250;grid.AttachGrid(43,0,0,42,0);CHECK(!s.images.empty());CHECK(s.item.content==128 && s.item.parent==43 && s.create.entries[s.item.id]==kItem);CHECK(s.grid.child==kItem);CHECK(s.create.count==4);
        std::optional<ScreenPoint> point;
        // 실제 생성·부착된 그림 자식을 그리며 같은 투명 적중에서 첫 실제 클릭점을 찾는다.
        for (const auto& [token,node]:s.images) { CHECK(node.parent==kItem);CHECK(s.create.entries[node.id]==token);image.Draw(token);
            // 항목 안의 실제 불투명 점만 선택한다.
            for (int y=1;y<199 && !point;++y) {
                // 매 점은 원본200×200 적중 버퍼의 물리 범위 안이다.
                for (int x=1;x<39 && !point;++x) if (image.Hit(token,static_cast<float>(x),static_cast<float>(y))) point=ScreenPoint{x,y};
            }
        }
        CHECK(point.has_value());CHECK(std::count_if(pixels.begin(),pixels.end(),[](auto color) { return color!=255; })>0);
        if (point) { selected={};const auto origin=s.collection->Position(kItem);CHECK(item.Handle(kItem,0x10000,static_cast<std::uint32_t>(point->x+origin.x),static_cast<std::uint32_t>(point->y+origin.y))==17);CHECK((selected==std::array<std::uint32_t,3>{0x02000008,128,s.item.serial}));CHECK(s.last==kind); }
        life.Kill(43);life.Cleanup();CHECK(s.images.empty() && s.create.count==1);CHECK(s.create.entries[0]==42 && ls.killCount==0);
    }
}
