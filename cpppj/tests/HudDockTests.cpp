// 실제 PE의 독립 HUD 본체 관측을 같은 노드·콜백·전체 raw 상태로 대조한다.
#include "GumpRawSupport.h"
#include "client/HudDock.h"
#include "client/GumpTree.h"
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string_view>

using namespace netstorm::client;
namespace {
using namespace netstorm::test::gumpraw;
constexpr std::uint32_t kNodeAddress=0x01000100,kNodeStride=0x200; // 원문의 격리 포인터를 검사 토큰과 구별한다.
constexpr std::size_t kRawBytes=0x11c,kNodeCount=8,kColumns=14,kExpectedRows=2150; // 전체 raw와 독립 관측의 고정 폭이다.
struct ExplicitStop {}; // 명시 callback 중단이며 원문 정상 반환과 구별한다.

// 마지막 빈 항목까지 유지해 TSV/효과 목록의 원시 경계를 나눈다.
std::vector<std::string> DockSplit(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{}; // 현재 필드의 시작점과 결과다.
    for (;;) { // 입력 구분자 사이의 부분만 복사한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1;
    }
}
// 부호 있는 값도 하위DWORD 비트로 유지한다.
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
// 원시 주소0 또는 고정노드 주소를1기반 검사 토큰으로 바꾼다.
std::uint32_t Token(std::uint32_t address) {
    if (!address) return 0;if (address<kNodeAddress || (address-kNodeAddress)%kNodeStride || (address-kNodeAddress)/kNodeStride>=kNodeCount) throw std::runtime_error("HUD raw 포인터 범위");
    return (address-kNodeAddress)/kNodeStride+1;
}
// 토큰을 원문의 고정격리 주소로만 직렬화한다.
std::uint32_t Address(std::uint32_t token) { return token ? kNodeAddress+(token-1)*kNodeStride : 0; }
// 같은 float32의 원시 비트다.
std::uint32_t FBits(float value) { return std::bit_cast<std::uint32_t>(value); }
// DWORD 비트를 float32로 읽는다.
float FValue(std::uint32_t value) { return std::bit_cast<float>(value); }
// 모든 signed 정수 비트는 구현 정의 변환 없이 읽는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }

// 계산 기대값 없이 독립 입력의 실제 저장소와 명시 외부장치만 공급한다.
struct DockScene {
    std::array<HudDividerNode,kNodeCount> storage{}; // 실제 공통 기본노드와 확장필드를 함께 보유한다.
    HudDockNodes children{}; // HUD 현재 직계포인터다.
    std::int32_t screenWidth{},screenHeight{},mapLeft{},mapTop{},mapWidth{},mapHeight{}; // 실제 signed 전역이다.
    std::uint32_t showHud{},fullRefresh{},presentMask{}; // 실제 모드/갱신과 명시 serial 장치다.
    float savedChatY{}; // 원본 숨기기 전 채팅 위치다.
    std::array<float,4> floatRect{}; // 실제 float32 지도LTRB다.
    ScreenRect mapRect{},leftBottomRect{}; // 실제 지도/좌하단 사각형이다.
    std::int8_t color{55}; // 보호PE00540452의 실제 signed BYTE다.
    std::string operation,trace; // 선택한 실제 진입과 외부호출순서다.
    std::vector<std::vector<std::string>> effects; // 입력6열 안의 명시 callback 효과만 보관한다.
    std::map<std::string,std::size_t> calls; // 경계별1기반 호출번호다.
    GumpCreateState create; // 공통Move의 같은 기본 상태다.
    GumpTreeState treeState; // 공통Move의 같은 트리 상태다.
    GumpTree tree{create,treeState,TreeHooks()}; // 실제00464d10과 같은 공통Move 구현을 재사용한다.
    std::unique_ptr<HudDock> dock; // 입력을 읽은 뒤 동일 노드/전역 참조로 생성한다.

    // 유효한 토큰에 대해 같은 상속 저장소를 반환한다.
    HudDividerNode& Node(std::uint32_t token) { if (!token || token>storage.size()) throw std::runtime_error("HUD 검사 토큰");return storage[token-1]; }
    // 원시 필드의 입력을 같은 실제 노드로 읽는다.
    void DecodeNode(std::size_t index,std::span<const std::uint8_t> raw) {
        auto& n=storage[index];static_cast<GumpCreateNode&>(n)=Decode(raw);n.parent=Token(n.parent);
        n.shape=Get(raw,0xa0);n.tileWidth=Signed(Get(raw,0xa4));n.tileHeight=Signed(Get(raw,0xa8));n.type=Get(raw,0xac);n.frame=Get(raw,0xb0);n.backgroundFlags=Get(raw,0xb4);
        n.axis=Get(raw,0xb8);n.minX=Signed(Get(raw,0xbc));n.maxX=Signed(Get(raw,0xc0));n.minY=Signed(Get(raw,0xc4));n.maxY=Signed(Get(raw,0xc8));
        for (std::size_t i=0;i<n.slaves.size();++i) n.slaves[i]={Get(raw,0xcc+8*i),Get(raw,0xd0+8*i)}; // 모든20DWORD를 원래 슬롯 순서대로 읽는다.
        if (operation=="hud" && index==0) { children={1,Token(n.shape),Token(static_cast<std::uint32_t>(n.tileWidth)),Token(static_cast<std::uint32_t>(n.tileHeight)),Token(n.type),Token(n.frame),Token(n.axis),Token(static_cast<std::uint32_t>(n.maxX))}; }
    }
    // 호스트 필드를 원문 offset에만 적는다. 기대 기하/분기를 계산하지 않는다.
    std::vector<std::uint8_t> EncodeNode(std::size_t index) const {
        const auto& n=storage[index];auto base=static_cast<GumpCreateNode>(n);base.vtable=0;base.parent=Address(base.parent);auto raw=Bytes(Encode(base));raw.resize(kRawBytes);
        Put(raw,0xa0,n.shape);Put(raw,0xa4,static_cast<std::uint32_t>(n.tileWidth));Put(raw,0xa8,static_cast<std::uint32_t>(n.tileHeight));Put(raw,0xac,n.type);Put(raw,0xb0,n.frame);Put(raw,0xb4,n.backgroundFlags);
        Put(raw,0xb8,n.axis);Put(raw,0xbc,static_cast<std::uint32_t>(n.minX));Put(raw,0xc0,static_cast<std::uint32_t>(n.maxX));Put(raw,0xc4,static_cast<std::uint32_t>(n.minY));Put(raw,0xc8,static_cast<std::uint32_t>(n.maxY));
        for (std::size_t i=0;i<n.slaves.size();++i) { Put(raw,0xcc+8*i,n.slaves[i].first);Put(raw,0xd0+8*i,n.slaves[i].second); } // 원본의 전체 고정 슬롯을 비교한다.
        if (operation=="hud" && index==0) {
            Put(raw,0xa0,Address(children.vertical));Put(raw,0xa4,Address(children.minimapDivider));Put(raw,0xa8,Address(children.chatDivider));Put(raw,0xac,Address(children.minimap));Put(raw,0xb0,Address(children.production));Put(raw,0xb8,Address(children.chat));Put(raw,0xc0,Address(children.knowledge));
        }
        return raw;
    }
    // 콜백과 실행 뒤 같은22DWORD 전역/장치 상태를 직렬화한다.
    std::array<std::uint32_t,22> Globals() const {
        return {std::uint32_t(screenWidth),std::uint32_t(screenHeight),showHud,std::uint32_t(mapLeft),std::uint32_t(mapTop),std::uint32_t(mapWidth),std::uint32_t(mapHeight),FBits(floatRect[0]),FBits(floatRect[1]),FBits(floatRect[2]),FBits(floatRect[3]),std::uint32_t(mapRect.left),std::uint32_t(mapRect.top),std::uint32_t(mapRect.right),std::uint32_t(mapRect.bottom),std::uint32_t(leftBottomRect.left),std::uint32_t(leftBottomRect.top),std::uint32_t(leftBottomRect.right),std::uint32_t(leftBottomRect.bottom),fullRefresh,FBits(savedChatY),presentMask};
    }
    // 입력된 실제 전역을 참조가 가리키는 호스트 필드에 쓴다.
    void SetGlobals(const std::array<std::uint32_t,22>& g) {
        screenWidth=Signed(g[0]);screenHeight=Signed(g[1]);showHud=g[2];mapLeft=Signed(g[3]);mapTop=Signed(g[4]);mapWidth=Signed(g[5]);mapHeight=Signed(g[6]);
        for (std::size_t i=0;i<floatRect.size();++i) floatRect[i]=FValue(g[7+i]); // float 저장을 비트 단위로 보존한다.
        mapRect={Signed(g[11]),Signed(g[12]),Signed(g[13]),Signed(g[14])};leftBottomRect={Signed(g[15]),Signed(g[16]),Signed(g[17]),Signed(g[18])};fullRefresh=g[19];savedChatY=FValue(g[20]);presentMask=g[21];
    }
    // 현재 경계의 raw 장치만 변경한다. 기대 출력 자료는 여기서 읽지 않는다.
    void Effect(const std::vector<std::string>& e) {
        const auto value=Number(e[3]);
        if (e[1].starts_with("node")) {
            static const std::map<std::string,std::size_t> fields{{"flags",4},{"parent",0x14},{"type",0x20},{"serial",0x2c},{"scale",0x38},{"baseWidth",0x3c},{"baseHeight",0x40},{"width",0x44},{"height",0x48},{"x",0x4c},{"y",0x50},{"axis",0xb8},{"minX",0xbc},{"maxX",0xc0},{"minY",0xc4},{"maxY",0xc8},{"link0serial",0xcc},{"link0flags",0xd0},{"link1serial",0xd4},{"link1flags",0xd8},{"vertical",0xa0},{"minimapDivider",0xa4},{"chatDivider",0xa8},{"minimap",0xac},{"production",0xb0},{"chat",0xb8},{"knowledge",0xc0}}; // 원본offset만 아는 명시장치 표다.
            const auto index=std::stoull(e[1].substr(4));auto raw=EncodeNode(index);Put(raw,fields.at(e[2]),value);DecodeNode(index,raw);
        } else if (e[1]=="global") {
            static const std::array<std::string_view,22> names{"screenWidth","screenHeight","showHud","mapLeft","mapTop","mapWidth","mapHeight","floatLeft","floatTop","floatRight","floatBottom","rectLeft","rectTop","rectRight","rectBottom","bottomLeft","bottomTop","bottomRight","bottomBottom","fullRefresh","savedChatY","presentMask"}; // 보고서와 같은 전역 순서다.
            auto g=Globals();const auto found=std::find(names.begin(),names.end(),e[2]);if (found==names.end()) throw std::runtime_error("HUD 전역효과 이름");g[static_cast<std::size_t>(found-names.begin())]=value;SetGlobals(g);
        } else if (e[1]!="control") throw std::runtime_error("HUD 효과 대상");
    }
    // 원문과 같은 장치 기본효과 뒤 같은 호출번호의 mutation을 적용한다.
    void Event(std::string name,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=name;
        for (const auto value:values) trace+=':'+std::to_string(value); // 실제 인자만 순서대로 관측한다.
        const auto trigger=name+std::to_string(++calls[name]);bool stop{};
        for (const auto& effect:effects) { // 경계 안의 효과 순서와 부분 중단을 보존한다.
            if (effect[0]!=trigger) continue;Effect(effect);if (effect[1]=="control" && effect[2]=="stop" && effect[3]=="1") stop=true;
        }
        if (stop) throw ExplicitStop{};
    }
    // Native 명시 기본Attach 장치와 같은 상태/인자를 제공한다.
    void BaseAttach(std::string name,std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        auto& n=Node(token);n.flags&=~8U;n.x=x;n.y=y;if (parent) n.parent=parent;Event(std::move(name),{token,FBits(x),FBits(y),parent,mode});
    }
    // Native 가상Push 경계이며 공통 트리 분리 자체를 검증 범위로 확대하지 않는다.
    void Push(std::uint32_t token) { Node(token).flags|=8;Event("push",{token}); }
    // 실제 nested divider본체와 일반UI의 명시Attach 경계를 구별한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        if (operation=="hud" && token>=2 && token<=4) dock->Attach(token,x,y,parent,mode);else BaseAttach("attach",token,x,y,parent,mode);
    }
    // 공통 Move에 필요한 실제노드와 명시 최종 가상경계를 공급한다.
    GumpTreeHooks TreeHooks() {
        GumpTreeHooks hooks;hooks.node=[this](auto token)->GumpCreateNode* { return &Node(token); };hooks.redraw=[](auto,auto) {};hooks.push=[this](auto token) { Push(token); };hooks.attach=[this](auto token,auto x,auto y,auto parent,auto mode) { Attach(token,x,y,parent,mode); };hooks.kill=[](auto) {};return hooks;
    }
    // 원문의 나머지 명시 외부경계를 같은 공유상태로 연결한다.
    HudDockHooks Hooks() {
        HudDockHooks hooks;hooks.node=[this](auto token)->GumpCreateNode* { return &Node(token); };hooks.divider=[this](auto token) { return &Node(token); };
        hooks.constructBackground=[this](auto token,float w,float h,auto flags,auto type,auto frame) { auto& n=Node(token);n.flags=9;n.scale=1;n.baseWidth=n.width=w;n.baseHeight=n.height=h;n.type=type;n.frame=frame;n.backgroundFlags=flags;Event("background",{token,FBits(w),FBits(h),flags,1,type,frame}); };
        hooks.activate=[this](auto token,auto active) { Event("activate",{token,active}); };hooks.baseAttach=[this](auto token,auto x,auto y,auto parent,auto mode) { BaseAttach("baseAttach",token,x,y,parent,mode); };
        hooks.push=[this](auto token) { Push(token); };hooks.attach=[this](auto token,auto x,auto y,auto parent,auto mode) { Attach(token,x,y,parent,mode); };hooks.move=[this](auto token,auto x,auto y,auto parent) { tree.Move(token,x,y,parent); };
        hooks.resolveSerial=[this](auto serial) { std::uint32_t token{};for (std::size_t i=0;i<storage.size();++i) if ((presentMask&(1U<<i)) && storage[i].serial==serial) { token=static_cast<std::uint32_t>(i+1);break; } /* 첫현재일치만선택한다. */ Event("resolve",{serial,token});return token; };
        hooks.bubble=[this](auto token,auto event,auto first,auto second) { Event("bubble",{token,event,first,second});if (operation=="hud") dock->Handle(event,first,second); };
        hooks.closePopups=[this] { Event("closePopups"); };hooks.invalidate=[this](auto x,auto y,auto w,auto h,auto flag) { Event("invalidate",{std::uint32_t(x),std::uint32_t(y),std::uint32_t(w),std::uint32_t(h),flag}); };
        hooks.finishResize=[this] { Event("finish"); };hooks.assertion=[this](auto line) { Event("assert",{line}); };hooks.baseHandle=[this](auto event,auto first,auto second) { Event("baseHandle",{event,first,second});return 73; };
        hooks.chatVisible=[this](bool value) { Event("chatVisible",{value ? 1U : 0U}); };hooks.chatInput=[this](bool value) { Event("chatInput",{value ? 1U : 0U}); };return hooks;
    }
    // 앞6열만 읽어 제품 실행과 외부장치 입력을 설치한다.
    explicit DockScene(const std::vector<std::string>& row):operation(row[1]) {
        const auto raw=Bytes(row[2]);if (raw.size()!=kRawBytes*kNodeCount) throw std::runtime_error("HUD 입력raw 폭");
        for (std::size_t i=0;i<storage.size();++i) DecodeNode(i,std::span<const std::uint8_t>(raw).subspan(i*kRawBytes,kRawBytes)); // 고정8개현재노드를 설치한다.
        const auto globals=DockSplit(row[3],':');if (globals.size()!=22) throw std::runtime_error("HUD 전역 입력 폭");std::array<std::uint32_t,22> g{};
        for (std::size_t i=0;i<g.size();++i) g[i]=Number(globals[i]); // signed 전역도 현재DWORD 그대로 공급한다.
        SetGlobals(g);if (row[5]!="-") for (const auto& effect:DockSplit(row[5],';')) effects.push_back(DockSplit(effect,'/')); // 기대자료를참조하지않는callback입력이다.
        dock=std::make_unique<HudDock>(children,HudDockGlobals{screenWidth,screenHeight,showHud,mapLeft,mapTop,mapWidth,mapHeight,floatRect,mapRect,leftBottomRect,fullRefresh,savedChatY},color,Hooks());
    }
    // 현재전체노드의 원시 관측을 붙인다.
    std::string NodesOutput() const { std::string result;for (std::size_t i=0;i<storage.size();++i) result+=Hex(EncodeNode(i));return result; /*원문주소순서다.*/ }
    // 실제전체전역과순수조회장치를 관측한다.
    std::string GlobalsOutput() const { std::string result;for (auto value:Globals()) { if (!result.empty()) result+=':';result+=std::to_string(value); } return result; /*원문전역순서다.*/ }
};
}

// 생성/등록/모든관계BYTE/중첩모드·채팅·viewport와callback부분쓰기를전체비교한다.
TEST_CASE(HudDock_IndependentNativeBodiesAndNestedViewport) {
    std::ifstream input(NETSTORM_HUD_DOCK_GEOMETRY_FIXTURE);CHECK(input.good());std::string line;std::size_t rows{},stopped{};
    while (std::getline(input,line)) { // 원문관측후반열은 비교에만 사용한다.
        if (line.empty() || line.starts_with("name\t")) continue;const auto row=DockSplit(line,'\t');CHECK(row.size()==kColumns);if (row.size()!=kColumns) continue;
        const auto failures=netstorm::test::FailureCount();DockScene scene(row);const auto parts=DockSplit(row[4],':');std::vector<std::uint32_t> args;
        for (const auto& part:parts) args.push_back(Number(part)); // 앞6열의실제인자만읽는다.
        std::uint32_t result{};bool completed=true;
        try {
            if (row[1]=="construct") result=scene.dock->Construct(1,args[0],Signed(args[1]),Signed(args[2]),args[3],args[4],args[5]);
            else if (row[1]=="register") scene.dock->RegisterSlave(1,args[0],args[1]);
            else if (row[1]=="reflow") scene.dock->Attach(1,FValue(args[0]),FValue(args[1]),args[2],args[3]);
            else if (row[1]=="hud") result=scene.dock->Handle(args[0],args[1],args[2]);else throw std::runtime_error("HUD 진입이름");
        } catch (const ExplicitStop&) { completed=false;++stopped; }
        CHECK(completed==(row[6]=="1"));CHECK(scene.NodesOutput()==row[7]);CHECK(scene.GlobalsOutput()==row[8]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[9]);
        if (completed && (row[1]=="construct" || row[1]=="hud")) CHECK(result==Number(row[10])); // void/미완료EAX를의미반환값으로비교하지않는다.
        if (completed) CHECK(Number(row[11])==4+args.size()*4);CHECK(Number(row[12])>0 && Number(row[12])>=Number(row[13]));++rows;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"HUD native 입력: %s\n",row[0].c_str());
    }
    CHECK(rows==kExpectedRows);CHECK(stopped==6);
}
