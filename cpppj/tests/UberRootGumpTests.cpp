// 실제 PE의 전체 Uber 부모 관측과 공통 등록/트리/수명의 루트 교체를 대조한다.
#include "GumpRawSupport.h"
#include "client/UberRootGump.h"
#include "client/GumpLifetime.h"
#include "client/GumpTree.h"
#include <map>
#include <variant>
#include <sstream>
#include <iostream>

using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
constexpr std::uint32_t kNodes=0x1100000,kStride=0x200,kTable=0x1000000,kTitle=0x565e70,kFormat=0x1003100; // Native의 고정 명시 주소다.
constexpr std::uint32_t kCount=0x565dc8,kSerial=0x540418,kRegistry=0x563e38; // 같은 현재 Gump 원시 전역이다.
constexpr std::array<std::uint32_t,13> kGlobals{0x540451,0x5411c8,0x542664,0x5ca920,0x5ca924,0x5ca928,0x5c78f0,0x5c78f4,0x531860,0x531864,0x540cec,0x540cf8,0x542a04}; // Native의 입력/출력 순서다.
struct ExplicitStop {}; // 명시 callback 중단을 Native 정상 반환과 구별한다.
// 비어 있는 마지막 필드도 보존하는 원시 입력 분리다.
std::vector<std::string> Parts(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; } // 입력 경계 이외 값은 계산하지 않는다.
}
// 입력의 signed/unsigned 십진 DWORD를 같은 하위 비트로 읽는다.
std::uint32_t N(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
// 현재 원시 주소의 물리 슬롯 번호다.0은 유효 노드가 아니다.
std::size_t Index(std::uint32_t token) {
    if (token<kNodes || (token-kNodes)%kStride || (token-kNodes)/kStride>=32) throw std::runtime_error("Uber 검사 토큰 범위");return (token-kNodes)/kStride;
}
// 같은 슬롯 번호의32비트 명시 주소다.
std::uint32_t Address(std::size_t index) { return kNodes+static_cast<std::uint32_t>(index)*kStride; }
// 실제 포인터 문자열/라벨 바이트를 Native trace 형식으로 기록한다.
std::string TextHex(std::string_view text) { return Hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())); }
// 이번 원문 비교는 ASCII 메시지 이름만 대소문자를 무시한다.
std::string Fold(std::string value) { for (auto& c:value) if (c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');return value; } // 바이트별 원본 입력 범위다.
// 같은 실제 Uber의 뒤10DWORD를 현재 주소로 빌린다. 호스트 배치를 원본 offset으로 가정하지 않는다.
std::array<std::uint32_t*,10> RootFields(UberRootNode& n) { return {&n.vertical,&n.minimapDivider,&n.chatDivider,&n.minimap,&n.production,&n.reservedB4,&n.chat,&n.reservedBC,&n.knowledge,&n.message}; }
// 원시 잔여 BYTE와 같은 실제 기본/파생 저장소다.
struct Slot {
    std::array<std::uint8_t,0x180> raw{}; // 명시 장치의 전체 물리 guard/미작성 바이트다.
    std::variant<GumpCreateNode,UberRootNode,HudDividerNode> value; // 각 토큰의 실제 고정 파생 주소다.
    char kind{'b'};bool released{}; // 실제 할당종류와 통합 수명 검사에서만 쓰는 free 관측이다.
    // 현재 파생의 같은 기본 주소를 제공한다.
    GumpCreateNode* Base() { return released ? nullptr : std::visit([](auto& n)->GumpCreateNode* { return &n; },value); }
    // 원문 byte 입력을 같은 의미 필드에 복원한다.
    void Decode(char type) {
        kind=type;const auto base=netstorm::test::gumpraw::Decode(raw);
        if (type=='r') { UberRootNode n;static_cast<GumpCreateNode&>(n)=base;const auto fields=RootFields(n);for (std::size_t i=0;i<fields.size();++i) *fields[i]=Get(raw,0xa0+i*4);value=n; } // 실제10확장필드다.
        else if (type=='d') {
            HudDividerNode n;static_cast<GumpCreateNode&>(n)=base;n.shape=Get(raw,0xa0);n.tileWidth=std::bit_cast<std::int32_t>(Get(raw,0xa4));n.tileHeight=std::bit_cast<std::int32_t>(Get(raw,0xa8));n.type=Get(raw,0xac);n.frame=Get(raw,0xb0);n.backgroundFlags=Get(raw,0xb4);n.axis=Get(raw,0xb8);
            n.minX=std::bit_cast<std::int32_t>(Get(raw,0xbc));n.maxX=std::bit_cast<std::int32_t>(Get(raw,0xc0));n.minY=std::bit_cast<std::int32_t>(Get(raw,0xc4));n.maxY=std::bit_cast<std::int32_t>(Get(raw,0xc8));
            for (std::size_t i=0;i<n.slaves.size();++i) n.slaves[i]={Get(raw,0xcc+i*8),Get(raw,0xd0+i*8)};value=n; // 실제 고정10슬롯이다.
        } else value=base;
    }
    // 현재 기본/파생 필드만 원래 위치에 쓰고 미작성 guard는 보존한다.
    std::array<std::uint8_t,0x180> Encode() {
        auto result=raw;const auto base=Bytes(netstorm::test::gumpraw::Encode(*Base()));std::copy(base.begin(),base.end(),result.begin());
        if (kind=='r') { auto& n=std::get<UberRootNode>(value);const auto fields=RootFields(n);for (std::size_t i=0;i<fields.size();++i) Put(result,0xa0+i*4,*fields[i]); } // 현재 확장10DWORD다.
        if (kind=='d') {
            const auto& n=std::get<HudDividerNode>(value);Put(result,0xa0,n.shape);Put(result,0xa4,static_cast<std::uint32_t>(n.tileWidth));Put(result,0xa8,static_cast<std::uint32_t>(n.tileHeight));Put(result,0xac,n.type);Put(result,0xb0,n.frame);Put(result,0xb4,n.backgroundFlags);Put(result,0xb8,n.axis);
            Put(result,0xbc,static_cast<std::uint32_t>(n.minX));Put(result,0xc0,static_cast<std::uint32_t>(n.maxX));Put(result,0xc4,static_cast<std::uint32_t>(n.minY));Put(result,0xc8,static_cast<std::uint32_t>(n.maxY));
            for (std::size_t i=0;i<n.slaves.size();++i) { Put(result,0xcc+i*8,n.slaves[i].first);Put(result,0xd0+i*8,n.slaves[i].second); } // 실제 현재 표를 그대로 기록한다.
        }
        return result;
    }
};
// 기대값을 계산하지 않고 원문 입력/명시 외부 장치/같은 실제 Gump 생성기를 조립한다.
struct UberScene {
    std::array<Slot,32> slots;GumpCreateState create;std::unique_ptr<GumpCreate> factory; // 같은 실제 고정 주소와 등록표다.
    std::map<std::uint32_t,std::uint32_t> globals;std::array<std::uint32_t,4> scales{};std::map<std::uint32_t,char> used; // 현재 입력 전역과 관측된 할당이다.
    std::string title,trace,stop;std::uint32_t test{},fail{},allocations{};std::size_t next{6};std::map<std::string,std::uint32_t> calls; // 현재 장치/명시 중단이다.
    struct Effect { std::string when;std::uint32_t address{},value{}; };std::vector<Effect> effects; // 순서를 보존한 현재주소 변경이다.
    // 실제 Native의 앞11입력만 읽고 결과 열은 참조하지 않는다.
    explicit UberScene(const std::vector<std::string>& row) {
        for (auto& slot:slots) { slot.raw.fill(static_cast<std::uint8_t>(N(row[7])));slot.Decode('b'); } // 미작성 전체 BYTE의 독립 입력이다.
        slots[0].Decode('r');used={{Address(0),'r'},{Address(1),'b'}};create.collection.edition=netstorm::o::OriginalEdition::Patch1078;create.serial=31;
        for (const auto& pair:Parts(row[2],',')) { const auto p=Parts(pair,':');globals[N(p[0])]=N(p[1]); } // 실제 주소/현재 DWORD다.
        const auto s=Parts(row[3],',');for (std::size_t i=0;i<scales.size();++i) scales[i]=N(s.at(i)); // 네 물리 float 입력이다.
        if (row[4]!="-") { const auto raw=Bytes(row[4]);title.assign(raw.begin(),raw.end()); }test=N(row[5]);stop=row[9];fail=N(row[10]);
        if (!row[8].empty()) for (const auto& effect:Parts(row[8],';')) { const auto p=Parts(effect,'/');effects.push_back({p[0],N(p[1]),N(p[2])}); } // 각 외부 callback의 추가 입력이다.
        factory=std::make_unique<GumpCreate>(create,GumpCreateHooks{[this](auto token) { return Base(token); },{}});BaseDevice(Address(1));Base(Address(1))->flags=0x801;
        const auto children=N(row[6]);Base(Address(1))->child=children ? Address(2) : 0;
        for (std::uint32_t i=0;i<children;++i) { const auto token=Address(i+2);used[token]='b';BaseDevice(token);auto& n=*Base(token);n.parent=Address(1);n.next=i+1<children ? Address(i+3) : 0;n.x=13.5F+i;n.y=-7.25F-i; } // 같은 기존 직계 링크/좌표 입력이다.
    }
    // null/해제/물리 범위를 같은 고정 주소에서 확인한다.
    GumpCreateNode* Base(std::uint32_t token) { return token ? slots[Index(token)].Base() : nullptr; }
    // 실제 변형만 같은 typed node로 반영한다. 결과 필드를 계산하지 않는다.
    void Write(std::uint32_t address,std::uint32_t value) {
        if (address>=kNodes && address<kNodes+slots.size()*kStride) { auto& slot=slots[(address-kNodes)/kStride];slot.raw=slot.Encode();Put(slot.raw,(address-kNodes)%kStride,value);slot.Decode(slot.kind); }
        else if (address==kCount) create.count=value;else if (address==kSerial) create.serial=value;else globals[address]=value;
    }
    // 같은 현재 전역/메시지 첫BYTE를 읽는다.
    std::uint32_t Read(std::uint32_t address) const { if (address==kTitle) return title.empty() ? 0 : static_cast<unsigned char>(title[0]);return globals.at(address); }
    // Native와 같은 기본효과 다음에 실제 trace/명시 변경/중단을 수행한다.
    void Event(std::string name,std::initializer_list<std::uint32_t> values={}) {
        std::string row=name;for (auto value:values) row+=':'+std::to_string(value); // 실제 전달값만 기록한다.
        Record(name,std::move(row));
    }
    // 문자열 인자가 있는 실제 trace도 같은 callback 순서에 참여한다.
    void Record(const std::string& name,std::string row) {
        if (!trace.empty()) trace+=';';trace+=row;const auto current=name+std::to_string(++calls[name]);
        for (const auto& effect:effects) if (effect.when==current) Write(effect.address,effect.value); // 입력한 현재주소 효과만 적용한다.
        if (stop==current) throw ExplicitStop{};
    }
    // 명시 하위의 고정 새 토큰이며 현재 파생 주소를 실제로 설치한다.
    std::uint32_t Fresh(char kind) { const auto token=Address(next++);slots[Index(token)].Decode(kind);used[token]=kind;return token; }
    // Native 공통 생성 장치와 같은 실제 GumpCreate를 사용한다.
    void BaseDevice(std::uint32_t token,std::uint32_t command=0,float width=0,float height=0,std::uint32_t flags=1) { factory->Construct(token,command,width,height,flags);Base(token)->vtable=kTable; }
    // 네 명시 전체공장 장치의 현재 결과/스케일이다. 실제 하위 본체의 검증을 대신하지 않는다.
    std::uint32_t Child(const std::string& name) {
        const auto token=Fresh('b');BaseDevice(token,0,0,0,name=="knowledge" ? 0 : 1);
        if (name=="minimap" || name=="knowledge") Base(token)->scale=Float(scales[name=="minimap" ? 0 : 1]);Event(name,{token});return token;
    }
    // 원본 부모가 호출하는 모든 선택 장치를 같은 현재 저장소에 제공한다.
    UberRootHooks Hooks() {
        UberRootHooks h;h.node=[this](auto token) { return std::get_if<UberRootNode>(&slots[Index(token)].value); };h.divider=[this](auto token) { return std::get_if<HudDividerNode>(&slots[Index(token)].value); };h.base=[this](auto token) { return Base(token); };
        h.read=[this](auto address) { return Read(address); };h.write=[this](auto address,auto value) { Write(address,value); };
        h.allocate=[this](auto size) { const auto token=++allocations==fail ? 0 : Fresh(size==0xc8 ? 'r' : size==0x11c ? 'd' : 'm');Event("alloc",{static_cast<std::uint32_t>(size),token});return token; };
        h.constructBase=[this](auto token,auto kind,float w,float ht,auto flags) { BaseDevice(token,kind,w,ht,flags);Event("base",{token,kind,Bits(w),Bits(ht),flags}); };
        h.constructDivider=[this](auto token,auto axis,auto w,auto ht,auto flags,auto type,auto frame) { BaseDevice(token,55,static_cast<float>(w),static_cast<float>(ht));auto& n=std::get<HudDividerNode>(slots[Index(token)].value);n.flags|=0x60200;n.axis=axis;n.slaves.fill({0,0});Event("divider",{token,axis,static_cast<std::uint32_t>(w),static_cast<std::uint32_t>(ht),flags,type,frame}); };
        h.createMinimap=[this] { return Child("minimap"); };h.createProduction=[this] { return Child("production"); };h.createKnowledge=[this] { return Child("knowledge"); };h.createChat=[this] { return Child("chat"); };
        h.emptyObserver=[this] { Event("map"); };h.refreshTechnology=[this] { Event("refresh"); };h.cleanup=[this] { Event("cleanup"); }; // map은최초Native자료의4437f0빈호출관측라벨이다.
        h.attach=[this](auto token,float x,float y,auto parent,auto mode) { auto& n=*Base(token);n.flags&=~8U;n.parent=parent;n.x=x;n.y=y;Event("attach",{token,Bits(x),Bits(y),parent,mode}); };
        h.move=[this](auto token,float x,float y,auto parent) { Event("move",{token,Bits(x),Bits(y),parent}); };h.push=[this](auto token) { Event("push",{token}); };
        h.registerSlave=[this](auto master,auto slave,auto flags) { Event("slave",{master,slave,flags}); };h.type=[this](auto type) { Event("type",{type});return 0U; };
        h.translate=[this](std::string_view value) { Record("translate","translate:"+TextHex(value));return 0x12345678U; };h.help=[this](auto token,auto text) { Event("help",{token,text}); };h.helpWord=[this](auto token,std::string_view word) { Record("helpword","helpword:"+std::to_string(token)+':'+TextHex(word)); };
        h.savedMessage=[] { return kTitle; };h.equalText=[this](auto name,std::string_view value) { const bool equal=Fold(title)==Fold(std::string(value));Record("equal","equal:"+std::to_string(name)+':'+TextHex(value)+':'+std::to_string(equal));return equal; };
        h.constructMessage=[this](auto token,auto name) { BaseDevice(token,0,Float(scales[2]),Float(scales[3]));Event("message",{token,name}); };
        h.formatMessage=[this](const auto& args) { std::string row="format";for (auto value:args) row+=':'+std::to_string(value);Record("format",row);return kFormat; }; // 원문9인자다.
        h.testMessage=[this](auto condition) { Event("test",{condition});return test; };h.visitMessage=[this](auto name) { Event("visit",{name}); };
        h.messageEvent=[this](auto token,auto event,auto first,auto second) { Event("event",{token,event,first,second}); };h.overlay=[this](auto token,bool value) { Event("overlay",{token,static_cast<std::uint32_t>(value)}); };h.setDepth=[this](auto token,auto depth) { Event("depth",{token,depth}); };h.replaceRoot=[this](auto token) { Event("replace",{token}); };return h;
    }
    // 원문 현재 전역의 같은 출력 순서다.
    std::string Globals() const {
        std::string out;for (auto address:kGlobals) { if (!out.empty()) out+=',';out+=std::to_string(address)+':'+std::to_string(globals.at(address)); } // 고정 입력 순서다.
        return out+','+std::to_string(kCount)+':'+std::to_string(create.count)+','+std::to_string(kSerial)+':'+std::to_string(create.serial);
    }
    // 실제1000칸의 연속값만 압축한다.
    std::string Registry() const {
        std::string out;for (std::size_t i=0;i<create.entries.size();) { auto end=i+1;while (end<create.entries.size() && create.entries[end]==create.entries[i]) ++end;if (!out.empty()) out+=';';out+=std::to_string(end-i)+':'+std::to_string(create.entries[i]);i=end; }return out;
    }
    // 실제 파생 필드/미작성 guard 전체를 기록한다.
    std::string Nodes() { std::string out;for (const auto& [token,kind]:used) { if (!out.empty()) out+=';';out+=std::to_string(token)+':'+kind+':'+Hex(slots[Index(token)].Encode()); }return out; }
};
}
// 실제80입력×두정밀도의전체노드/등록/현재전역/순서/부분효과를 대조한다.
TEST_CASE(UberRoot_Native160) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_UBER_ROOT_FIXTURE).rows) { // 앞11입력과뒤7관측을 엄격히 분리한다.
        CHECK(row.size()==18);if (row.size()!=18) continue;UberScene scene(row);UberRootGump root(scene.create,scene.Hooks());bool completed=false;std::uint32_t result{};
        try { if (row[1]=="construct") result=root.Construct(Address(0));else root.Replace();completed=true; }
        catch (const ExplicitStop&) {}catch (const std::runtime_error&) { CHECK(N(row[10])!=0); }
        if (completed!=(N(row[11])!=0) || scene.Globals()!=row[13] || scene.Registry()!=row[14] || scene.Nodes()!=row[15] || scene.trace!=row[16]) std::cerr<<"Uber Native "<<row[0]<<'\n';
        CHECK(completed==(N(row[11])!=0));if (completed && row[1]=="construct") CHECK(result==N(row[12]));CHECK(scene.Globals()==row[13]);CHECK(scene.Registry()==row[14]);CHECK(scene.Nodes()==row[15]);CHECK(scene.trace==row[16]);++count;
    }
    CHECK(count==160);
}
// 원문 부모를 실제 공통 GumpCreate/Tree/Kill/Cleanup에 연결해 기존 자식을 새 루트에 보존한다.
TEST_CASE(UberRoot_RealTreeAndLifetimeTransfer) {
    const auto rows=LoadFixture(NETSTORM_UBER_ROOT_FIXTURE).rows;const auto found=std::find_if(rows.begin(),rows.end(),[](const auto& row) { return row[0].starts_with("replace-1024-768-empty-") && row[0].ends_with("27f"); });CHECK(found!=rows.end());if (found==rows.end()) return;
    auto input=*found;input[6]="3";UberScene scene(input);const auto before=scene.create.count;auto hooks=scene.Hooks();GumpTreeState treeState;GumpLifetimeState lifeState;std::unique_ptr<GumpTree> tree;std::unique_ptr<GumpLifetime> lifetime;
    for (std::size_t i=2;i<5;++i) { auto& n=*scene.Base(Address(i));n.flags&=~8U;n.previous=i==2 ? 0 : Address(i-1); } // 실제공통트리의부착완료/양방향링크 입력이다.
    GumpTreeHooks th;th.node=hooks.base;th.redraw=[](auto,auto) {};th.push=[&](auto token) { tree->Push(token); };th.attach=[&](auto token,float x,float y,auto parent,auto mode) { tree->Attach(token,x,y,parent,mode); };th.kill=[&](auto token) { lifetime->Kill(token); };tree=std::make_unique<GumpTree>(scene.create,treeState,std::move(th));
    GumpLifetimeHooks lh;lh.node=hooks.base;lh.push=[&](auto token) { tree->Push(token); };lh.deletingDestructor=[&](auto token,auto flags) { lifetime->DeleteScalar(token,flags); };lh.release=[&](auto token) { scene.slots[Index(token)].released=true; };lifetime=std::make_unique<GumpLifetime>(scene.create,lifeState,std::move(lh));
    hooks.attach=[&](auto token,float x,float y,auto parent,auto mode) { tree->Attach(token,x,y,parent,mode); };hooks.push=[&](auto token) { tree->Push(token); };hooks.cleanup=[&] { lifetime->Cleanup(); };hooks.replaceRoot=[&](auto token) { lifetime->ReplaceRoot(token); };
    UberRootGump root(scene.create,std::move(hooks));root.Replace();const auto current=scene.create.entries[0];CHECK(current!=Address(1) && current && scene.create.collection.root==current);CHECK(!scene.Base(Address(1)));CHECK(scene.create.count==before+7);
    std::size_t children{};for (auto child=scene.Base(current)->child;child;child=scene.Base(child)->next) { CHECK(scene.Base(child)->parent==current);++children; } // 새실제7자식과이전3자식이 같은트리에 있다.
    CHECK(children==10);for (std::size_t i=2;i<5;++i) CHECK(scene.Base(Address(i))->parent==current); // 이전실제주소와serial/등록은보존한다.
    CHECK(std::count(scene.create.entries.begin(),scene.create.entries.end(),current)==1);lifetime->Kill(current);lifetime->Cleanup();CHECK(scene.create.count==0 && !scene.create.entries[0]);
}
// 실제 도킹은 별도8포인터 사본/등록0루트가 아니라 해당 Uber this의 현재 확장 필드를 읽는다.
TEST_CASE(UberRoot_DockReadsLiveThisFields) {
    const auto rows=LoadFixture(NETSTORM_UBER_ROOT_FIXTURE).rows;UberScene scene(rows.at(0));auto effects=scene.Hooks();UberRootGump root(scene.create,effects);root.Construct(Address(0));const auto view=root.DockView(Address(0));
    CHECK(view.token(HudDockPart::Root)==Address(0) && scene.create.entries[0]==Address(1));
    std::int32_t width=1024,height=768,left=77,top=0,mapWidth=947,mapHeight=700;std::uint32_t show=1,refresh{};float saved{};std::array<float,4> floats{};ScreenRect rectangle{},bottom{};std::vector<std::array<std::int32_t,4>> invalidated;const std::int8_t color=55; // 도킹이 직접 쓰는 실제 참조 전역이다.
    HudDockHooks hooks;hooks.node=effects.base;hooks.divider=effects.divider;hooks.baseAttach=effects.attach;hooks.attach=effects.attach;hooks.push=effects.push;hooks.move=effects.move;
    hooks.resolveSerial=[&](auto serial) { for (auto token:scene.create.entries) if (token && scene.Base(token)->serial==serial && !(scene.Base(token)->flags&0x10000)) return token;return 0U; }; // 같은 실제 현재 등록표다.
    hooks.bubble=[&](auto token,auto event,auto first,auto second) { scene.Event("bubble",{token,event,first,second}); };hooks.closePopups=[&] { scene.Event("close"); };
    hooks.invalidate=[&](auto x,auto y,auto w,auto h,auto) { invalidated.push_back({x,y,w,h}); };
    HudDock dock(view,{width,height,show,left,top,mapWidth,mapHeight,floats,rectangle,bottom,refresh,saved},color,std::move(hooks));auto& n=std::get<UberRootNode>(scene.slots[0].value);
    n.vertical=n.minimapDivider;n.minimap=n.production; // 도킹 생성뒤 같은 실제 필드를 바꿔 포인터 사본을 검출한다.
    CHECK(view.token(HudDockPart::Vertical)==n.minimapDivider && view.token(HudDockPart::Minimap)==n.production);
    CHECK(dock.Handle(0x0200003a,scene.Base(n.vertical)->serial,0)==1);CHECK(left==80 && mapWidth==944 && rectangle.left==80 && rectangle.right==1024 && invalidated.size()==2);
}
// 전체 자식 중 연결 누락은 실제 위치에서 실패하며 빈 노드를 성공으로 돌려주지 않는다.
TEST_CASE(UberRoot_MissingFactoryKeepsPriorEffects) {
    const auto rows=LoadFixture(NETSTORM_UBER_ROOT_FIXTURE).rows;UberScene scene(rows.at(0));auto hooks=scene.Hooks();hooks.createProduction={};UberRootGump root(scene.create,std::move(hooks));bool rejected=false;
    try { root.Construct(Address(0)); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);const auto& node=std::get<UberRootNode>(scene.slots[0].value);CHECK(node.vertical && node.minimapDivider && node.chatDivider && node.minimap);CHECK(scene.calls["minimap"]==1 && scene.calls["production"]==0);
}
// 이전루트가없는잘못된caller도새생성/임시등록반납뒤실패하며등록0과그별칭을다르게남기지않는다.
TEST_CASE(UberRoot_MissingOldRootFailsAfterConstructionEffects) {
    const auto rows=LoadFixture(NETSTORM_UBER_ROOT_FIXTURE).rows;UberScene scene(rows.at(0));scene.create.entries[0]=0;scene.create.collection.root=0;scene.create.count=0;
    UberRootGump root(scene.create,scene.Hooks());bool rejected=false;try { root.Replace(); }catch (const std::runtime_error&) { rejected=true; }
    CHECK(rejected && !scene.create.entries[0] && !scene.create.collection.root);CHECK(scene.create.count==8 && scene.calls["minimap"]==1 && scene.calls["chat"]==1);CHECK(scene.calls["replace"]==0);
}
// 실제빈RET에는게임하위구현을요구하지않으며나머지전체자식은그대로생성한다.
TEST_CASE(UberRoot_EmptyLeafNeedsNoGameHook) {
    const auto rows=LoadFixture(NETSTORM_UBER_ROOT_FIXTURE).rows;UberScene scene(rows.at(0));auto hooks=scene.Hooks();hooks.emptyObserver={};UberRootGump root(scene.create,std::move(hooks));CHECK(root.Construct(Address(0))==Address(0));
    CHECK(scene.calls["map"]==0 && scene.calls["divider"]==3 && scene.calls["minimap"]==1 && scene.calls["production"]==1 && scene.calls["knowledge"]==1 && scene.calls["chat"]==1);
}
