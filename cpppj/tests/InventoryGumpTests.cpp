// 독립 원본170행의 전체 격자/항목 바이트·호출과 공통 트리의 실제 수명을 검사한다.
#include "GumpRawSupport.h"
#include "client/InventoryGump.h"
#include "client/GumpTree.h"
#include "client/GumpLifetime.h"

namespace {
using namespace netstorm::test::gumpraw;
constexpr std::uint32_t kNode=0x12401000,kFirstItem=0x12403000; // 원본 격리 관측과 같은 안정 토큰이다.
// 원본 생성자가 쓰지 않는 바이트도 임의 입력으로 남긴다.
GumpCreateNode InitialBase() { return Decode(std::vector<std::uint8_t>(160,0xa5)); }
// 파생 필드의 전체 원문 입력을 준비한다.
InventoryGridNode InitialGrid() {
    InventoryGridNode node;static_cast<GumpCreateNode&>(node)=InitialBase();node.minimumColumns=node.minimumRows=std::bit_cast<std::int32_t>(0xa5a5a5a5U);
    node.cellWidth=node.cellHeight=node.itemScale=Float(0xa5a5a5a5);node.contentMask=node.acceptedCursor=node.requireImageHit=node.fixed=0xa5a5a5a5;return node;
}
// 미작성 +a4/+b2를 포함한 항목 입력이다.
InventoryItemNode InitialItem() {
    InventoryItemNode node;static_cast<GumpCreateNode&>(node)=InitialBase();node.content=node.reservedA4=node.requireImageHit=node.additional=0xa5a5a5a5;
    node.blinkUntil=std::bit_cast<double>(0xa5a5a5a5a5a5a5a5ULL);node.blink=node.decoration=0xa5;node.reservedB2=0xa5a5;node.iconScale=Float(0xa5a5a5a5);return node;
}
// 호스트 배치 대신 실제 원본 offset으로 현재 격자를 기록한다.
std::string EncodeGrid(const InventoryGridNode& n) {
    auto raw=Bytes(Encode(n));raw.resize(0xc4);Put(raw,160,static_cast<std::uint32_t>(n.minimumColumns));Put(raw,164,static_cast<std::uint32_t>(n.minimumRows));Put(raw,168,Bits(n.cellWidth));Put(raw,172,Bits(n.cellHeight));
    Put(raw,176,n.contentMask);Put(raw,180,n.acceptedCursor);Put(raw,184,n.requireImageHit);Put(raw,188,n.fixed);Put(raw,192,Bits(n.itemScale));return Hex(raw);
}
// 항목의 double/두 BYTE/미작성WORD도 원본 폭으로 기록한다.
std::string EncodeItem(const InventoryItemNode& n) {
    auto raw=Bytes(Encode(n));raw.resize(0xc0);Put(raw,160,n.content);Put(raw,164,n.reservedA4);const auto bits=std::bit_cast<std::uint64_t>(n.blinkUntil);
    Put(raw,168,static_cast<std::uint32_t>(bits));Put(raw,172,static_cast<std::uint32_t>(bits>>32));raw[176]=n.blink;raw[177]=n.decoration;Put(raw,178,n.reservedB2,2);Put(raw,180,n.requireImageHit);Put(raw,184,n.additional);Put(raw,188,Bits(n.iconScale));return Hex(raw);
}
// 완성 입력과 효과 관측을 보관한다. 기대 분기/배치값은 계산하지 않는다.
struct InventoryScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원본 내용물의 같은 물리 범위다.
    ContainedFinderState finder;GumpCreateState create; // 같은 검색 정책과 단일 등록표다.
    InventoryGridNode grid=InitialGrid(); // 격자의 안정된 파생 저장소다.
    std::map<std::uint32_t,InventoryItemNode> items;std::map<std::uint32_t,GumpCreateNode> bases; // 실제 같은 생성/수명 노드다.
    ScreenPoint point{7,-3};std::uint32_t cursor{},parent{50},forward{},baseReturn{},inside{};double now{}; // 독립 외부 입력이다.
    std::string trace;InventoryGumpHooks hooks;GumpCreate factory; // 실제 생성자와 관측 경계다.
    // 원본과 같은 기본/파생 주소만 공급한다.
    InventoryScene():factory(create,{[this](auto token) { return Node(token); },[this](auto bytes) { CHECK(bytes==160);bases.emplace(42,GumpCreateNode{});return 42U; }}) {
        create.serial=23;hooks.grid=[this](auto token) { return token==kNode ? &grid : nullptr; };
        hooks.item=[this](auto token)->InventoryItemNode* { const auto found=items.find(token);return found==items.end() ? nullptr : &found->second; };
        hooks.assertion=[this](std::string_view text) { Event("Assert,"+Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()})); };
        hooks.clearChildren=[this](auto token) { CHECK(token==kNode);Event("Clear"); };
        hooks.baseAttach=[this](auto token,float x,float y,auto p,auto mode) { CHECK(token==kNode);Event("BaseAttach",{Bits(x),Bits(y),p,mode}); };
        hooks.allocateItem=[this](auto size) { CHECK(size==0xc0);const auto token=kFirstItem+static_cast<std::uint32_t>(items.size())*0x100;items.emplace(token,InitialItem());Event("New",{static_cast<std::uint32_t>(size),token});return token; };
        hooks.attach=[this](auto token,float x,float y,auto p,auto mode) { if (token==kNode) Event("V",{Bits(x),Bits(y),p,mode});else Event("ItemAttach",{token,Bits(x),Bits(y),p,mode}); };
        hooks.push=[this](auto token) { CHECK(token==kNode);Event("Push"); };hooks.bind=[this](auto token,auto sid) { CHECK(token==kNode);Event("Bind",{sid});grid.object=sid; };
        hooks.idle=[this](auto token,bool enabled) { CHECK(enabled);Event("Idle",{token,1});Node(token)->flags|=0x80; };
        hooks.now=[this] { Event("Now");return now; };hooks.contentParent=[this](auto sid) { Event("Parent",{sid});return parent; };
        hooks.forward=[this](auto token,auto event,auto first,auto second) { CHECK(token==kNode);Event("Forward",{event,first,second});return forward; };
        hooks.baseHandle=[this](auto token,auto event,auto first,auto second) { CHECK(token==kNode);Event("BaseHandle",{event,first,second});return baseReturn; };
        hooks.local=[this](auto token,auto event,ScreenPoint p) { CHECK(token==kNode);Event("Local",{event,static_cast<std::uint32_t>(p.x),static_cast<std::uint32_t>(p.y)});return std::array<float,2>{7.5F,-3.25F}; };
        hooks.inside=[this](auto token,std::array<float,2> p) { CHECK(token==kNode);Event("Inside",{Bits(p[0]),Bits(p[1])});return inside!=0; };hooks.cancelCursor=[this] { Event("Cancel"); };
    }
    // 생존 노드는 실제 기본 생성자가 사용하는 같은 주소다.
    GumpCreateNode* Node(std::uint32_t token) {
        if (token==kNode) return &grid;if (auto it=items.find(token);it!=items.end()) return &it->second;
        if (auto it=bases.find(token);it!=bases.end()) return &it->second;return nullptr;
    }
    // 호출 인자만 원문 구분자로 기록한다.
    void Event(std::string text,std::initializer_list<std::uint32_t> args={}) {
        if (!trace.empty()) trace+=';';trace+=text;
        // 전체 DWORD를 부호 해석 없이 보존한다.
        for (const auto value:args) trace+=','+std::to_string(value);
    }
    // 원본에 공급한 flags/slot을 실제 별도 ContentForm에 쓴다.
    void Contents(std::string_view text) {
        const auto parts=text=="-" ? std::vector<std::string>{} : Split(std::string(text),';');auto raw=InputRaw(pool,Sid{50});Put(raw,6,parts.empty() ? 0U : 100U,2);
        // 목록과 실제 next WORD를 입력 순서대로 작성한다.
        for (std::size_t i=0;i<parts.size();++i) {
            const auto fields=Split(parts[i],':');raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(100+i)});raw[10]=6;Put(raw,4,i+1<parts.size() ? static_cast<std::uint32_t>(101+i) : 0U,2);Put(raw,20,Number(fields[0]),2);raw[23]=static_cast<std::uint8_t>(Number(fields[1]));
        }
    }
};
// 전체 원본170행을 재생해 모든 바이트와 외부 호출·순서·반환을 비교한다.
TEST_CASE(InventoryGump_Native170) {
    std::size_t count{};
    // 각 행은 독립 입력이며 같은 행 안의 생성/부착은 하나의 실제 등록표를 공유한다.
    for (const auto& row:LoadFixture(NETSTORM_INVENTORY_GRID_FIXTURE).rows) {
        CHECK(row.size()==6);InventoryScene s;InventoryGump inventory(s.create,s.factory,s.pool,s.finder,{s.point,s.cursor},s.hooks);std::string result="-",node;
        if (row[0]=="Grid") { const auto args=Inputs(row[1]);result=std::to_string(inventory.ConstructGrid(kNode,std::bit_cast<std::int32_t>(args[0]),std::bit_cast<std::int32_t>(args[1]),args[2])); }
        else if (row[0]=="Item") {
            const auto args=Split(row[1],',');s.now=std::stod(args[4]);auto raw=InputRaw(s.pool,Sid{static_cast<std::uint16_t>(Number(args[2]))});Put(raw,20,Number(args[3]),2);s.items.emplace(kNode,InitialItem());
            // 독립 단일 항목은 격자와 같은 주소를 쓰므로 기본 조회를 항목에 연결한 생성자를 사용한다.
            GumpCreate factory(s.create,{[&s](auto token)->GumpCreateNode* { return &s.items.at(token); },{}});auto hooks=s.hooks;hooks.idle=[&s](auto token,bool enabled) { CHECK(enabled);s.Event("Idle",{token,1});s.items.at(token).flags|=0x80; };
            InventoryGump item(s.create,factory,s.pool,s.finder,{s.point,s.cursor},hooks);result=std::to_string(item.ConstructItem(kNode,Float(Number(args[0])),Float(Number(args[1])),Number(args[2])));node=EncodeItem(s.items.at(kNode));s.items.clear();
        } else {
            inventory.ConstructGrid(kNode,2,6,8);s.grid.object=50;s.grid.x=1.25F;s.grid.y=-2.5F;s.trace.clear();
            if (row[0]=="Attach") {
                const auto parts=Split(row[1],'|');const auto a=Inputs(parts[0]);s.grid.minimumColumns=std::bit_cast<std::int32_t>(a[0]);s.grid.minimumRows=std::bit_cast<std::int32_t>(a[1]);s.grid.contentMask=a[2];s.grid.fixed=a[3];s.grid.itemScale=Float(a[4]);s.grid.requireImageHit=a[5];
                s.grid.baseWidth=Float(a[6]);s.grid.baseHeight=Float(a[7]);s.grid.width=Float(a[8]);s.grid.height=Float(a[9]);s.Contents(parts[1]);inventory.AttachGrid(kNode,5,6,0,a[10]);
            } else if (row[0]=="Handle") {
                const auto a=Inputs(row[1]);s.parent=a[1];auto raw=InputRaw(s.pool,Sid{100});Put(raw,20,a[2],2);s.inside=a[3];s.cursor=a[4];s.grid.acceptedCursor=a[5];s.forward=a[6];s.baseReturn=a[7];result=std::to_string(inventory.HandleGrid(kNode,a[0],100,9));
            } else { CHECK(row[0]=="Gone");inventory.ParentGone(kNode,Number(row[1])); }
        }
        if (node.empty()) node=EncodeGrid(s.grid);std::string children;
        // 새 항목은 실제 생성한 전체 raw 필드만 직렬화한다.
        for (const auto& [token,item]:s.items) { if (!children.empty()) children+=';';children+=std::to_string(token)+'='+EncodeItem(item); }
        // 순회 자체는 원본/기존 검색기 각각의 책임이므로 원본의 순회 경계 로그만 비교에서 제외한다.
        std::string nativeTrace;
        // 생성/부착/전달 효과는 모두 원래 순서를 유지한다.
        for (const auto& event:Split(row[5],';')) if (event!="Next" && event!="Begin,50") { if (!nativeTrace.empty()) nativeTrace+=';';nativeTrace+=event; }
        const auto actualTrace=s.trace.empty() ? "-" : s.trace;const auto itemText=children.empty() ? "-" : children;
        if (result!=row[2] || node!=row[3] || itemText!=row[4] || actualTrace!=nativeTrace) std::printf("생산 격자 원문%zu %s: result%d node%d items%d trace%d\n실제:%s\n원본:%s\n",count,row[0].c_str(),result==row[2],node==row[3],itemText==row[4],actualTrace==nativeTrace,actualTrace.c_str(),nativeTrace.c_str());
        if (node!=row[3]) { const auto actual=Bytes(node),expected=Bytes(row[3]);
            // 실패한 원문 offset만 표시하며 기대 바이트는 수정하지 않는다.
            for (std::size_t offset=0;offset<actual.size();offset+=4) if (Get(actual,offset)!=Get(expected,offset)) std::printf("+%02zx 실제%08x 원본%08x\n",offset,Get(actual,offset),Get(expected,offset));
        }
        CHECK(result==row[2]);CHECK(node==row[3]);CHECK(itemText==row[4]);CHECK(actualTrace==nativeTrace);++count;
    }
    CHECK(count==170);
}

// 같은 실제 등록표/트리에 격자와 항목을 두 번 부착하고 이전 자식의 지연 삭제까지 검사한다.
TEST_CASE(InventoryGump_SharedTreeRebuildAndLifetime) {
    InventoryScene s;GumpTreeState treeState;GumpLifetimeState lifeState;GumpTree* currentTree{};GumpLifetime* currentLife{};
    GumpTreeHooks th;th.node=[&s](auto token) { return s.Node(token); };th.redraw=[](auto,bool) {};th.push=[&](auto token) { currentTree->Push(token); };th.kill=[&](auto token) { currentLife->Kill(token); };th.attach=[&](auto token,float x,float y,auto p,auto mode) { currentTree->Attach(token,x,y,p,mode); };
    GumpTree tree(s.create,treeState,th);currentTree=&tree;GumpLifetimeHooks lh;lh.node=th.node;lh.push=th.push;lh.deletingDestructor=[&](auto token,auto flags) { currentLife->DeleteScalar(token,flags); };lh.release=[&s](auto token) { s.items.erase(token); };
    GumpLifetime life(s.create,lifeState,lh);currentLife=&life;s.create.screenWidth=640;s.create.screenHeight=480;const auto root=s.factory.InitializeRoot();
    s.hooks.clearChildren=[&](auto token) { while (s.Node(token)->child) life.Kill(s.Node(token)->child); };s.hooks.baseAttach=[&](auto token,float x,float y,auto p,auto mode) { tree.Attach(token,x,y,p,mode); };
    s.hooks.attach=s.hooks.baseAttach;s.hooks.idle=[&](auto token,bool enable) { life.SetIdle(token,enable); };
    InventoryGump inventory(s.create,s.factory,s.pool,s.finder,{s.point,s.cursor},s.hooks);inventory.ConstructGrid(kNode,1,6,8);s.grid.form=0;s.grid.object=50;s.grid.baseWidth=s.grid.width=40;s.grid.baseHeight=s.grid.height=250;
    s.Contents("8:0;8:1;64:255");inventory.AttachGrid(kNode,0,26,root,0);CHECK(s.create.count==4);CHECK(s.items.size()==2);CHECK(s.grid.parent==root);
    // 두 항목은 Deck 사본이 아닌 실제 내용물100/101을 참조하고 같은 부모에 존재한다.
    for (const auto& [token,item]:s.items) { CHECK(item.content==100 || item.content==101);CHECK(item.parent==kNode);CHECK(s.create.entries[item.id]==token);s.Node(token)->form=0; }
    const auto oldItems=s.items.size();s.Contents("8:3");inventory.AttachGrid(kNode,0,26,root,0);CHECK(lifeState.killCount==oldItems);CHECK(s.items.size()==3);life.Cleanup();CHECK(s.items.size()==1);CHECK(s.create.count==3);
    CHECK(s.items.begin()->second.content==100 && s.items.begin()->second.parent==kNode);CHECK(s.items.begin()->second.y==125.0F);CHECK(s.create.entries[s.items.begin()->second.id]==s.items.begin()->first);
}
}
