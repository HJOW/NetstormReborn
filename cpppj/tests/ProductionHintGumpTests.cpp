// 새 제품 코드 전에 동결한 실제 PE 관측의 전체 필드/효과를 대조한다.
#include "GumpRawSupport.h"
#include "RawSceneSupport.h"
#include "client/ProductionHintGump.h"
using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
constexpr std::uint32_t kNode=0x12401000,kOwned=0x12403000,kReference=0x12405000; // 원본 격리 실행과 같은 명시 경계 토큰이다.
constexpr std::uint32_t kCanary=0xa5a5a5a5; // 생성자가 쓰지 않은 필드를 구별하는 입력이다.
// 관측값은 계산하지 않고 Text/heap/시계/Draw/부착 경계만 재생한다.
struct HintScene {
    GumpCreateState create;ProductionHintState state;ProductionHintNode node; // 단일 현재 등록/전역/저장소다.
    double now{3.25};std::string owned,trace;ScreenPoint point{17,-9};ScreenRect box{10,20,40,56}; // 외부 명시 입력과 효과 기록이다.
    // 원본 초기 canary 바이트를 호스트 의미 필드로 설치한다.
    HintScene() {
        static_cast<GumpCreateNode&>(node)=Decode(std::vector<std::uint8_t>(0xa0,0xa5));node.head=node.reserved=node.owned=node.reference=kCanary;node.until=std::bit_cast<double>(std::uint64_t{0xa5a5a5a5a5a5a5a5});create.collection.root=kReference;state.lastSerial=0x76543210;
    }
    // 모든 경계 효과를 원본 순서대로 기록한다.
    void Event(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 문자열을 원본 TSV와 같은 바이트16진수로 기록한다.
    static std::string TextHex(std::string_view value) { return Hex(std::span(reinterpret_cast<const std::uint8_t*>(value.data()),value.size())); }
    // 원본 snapshot이 실제 읽은116바이트만 같은 위치 순서로 직렬화한다.
    std::string Snapshot() const {
        const auto raw=Bytes(Encode(node));std::vector<std::uint8_t> result;
        // 기본 생성과 wrapper/Draw가 다루는 실제 DWORD다.
        for (const auto at:{0,4,0x20,0x2c,0x30,0x3c,0x40,0x44,0x48,0x4c,0x50}) result.insert(result.end(),raw.begin()+at,raw.begin()+at+4);
        result.insert(result.end(),raw.begin()+0x5c,raw.begin()+0x6c);result.insert(result.end(),raw.begin()+0x80,raw.end());
        const auto append=[&](std::uint32_t value) { const auto bytes=std::bit_cast<std::array<std::uint8_t,4>>(value);result.insert(result.end(),bytes.begin(),bytes.end()); }; // 현재 DWORD만 추가한다.
        append(node.head!=0);append(node.reserved);const auto time=std::bit_cast<std::array<std::uint8_t,8>>(node.until);result.insert(result.end(),time.begin(),time.end());append(node.owned!=0);append(node.reference);return Hex(result);
    }
    // 실제 새 모듈에 기대 출력 대신 원본과 같은 외부 입력을 공급한다.
    ProductionHintHooks Hooks() {
        ProductionHintHooks h;h.node=[this](auto token) { CHECK(token==kNode);return &node; };h.now=[this] { Event("Now");return now; };
        h.allocate=[this](auto size) { Event("New,"+std::to_string(size));CHECK(size==0xb8);return kNode; };
        h.constructText=[this](auto) { Event("Text");node.vtable=0x5144d4;node.flags=0x400001;node.serial=41;node.head=0; };
        h.duplicate=[this](std::string_view text) { owned=text;Event("Dup,"+TextHex(text));return kOwned; };h.string=[this](auto token)->std::string_view { CHECK(token==kOwned);return owned; };
        h.free=[this](auto token) { CHECK(token==kOwned);Event("Free,1"); };h.kill=[this](auto) { Event("Kill"); };
        h.translate=[this](std::string_view text) { Event("Translate,"+TextHex(text));return std::string(text); };
        h.style=[this](auto,std::string_view text) { Event("Style,"+TextHex(text));node.head=0x12408000; };
        h.fit=[this](auto,auto x,auto y) { Event("Fit,"+std::to_string(x)+','+std::to_string(y));node.baseWidth=node.width=51.25F;node.baseHeight=node.height=13.5F; };
        h.idle=[this](auto,bool on) { Event("Idle,"+std::to_string(on));node.flags|=0x80; };
        h.bounds=[this](auto token) { Event("Bounds,"+std::to_string(token==kReference));return box; };h.position=[this](auto) { Event("Position");return point; };
        h.draw=[this](auto head,auto p) { Event("Draw,"+std::to_string(head!=0)+','+std::to_string(std::bit_cast<std::uint32_t>(p.x))+','+std::to_string(std::bit_cast<std::uint32_t>(p.y))+",0"); };
        h.attach=[this](auto,float x,float y,auto parent,auto mode) { Event("Attach,"+std::to_string(Bits(x))+','+std::to_string(Bits(y))+','+std::to_string(parent)+','+std::to_string(mode));node.x=x;node.y=y; };
        h.deleteText=[this](auto token,auto flags) { Event("TextDelete");node.vtable=0x50ac70;if (flags&1) Event("Free,2");return token; };return h;
    }
};
}
// 두 x87 정밀도에서 동일했던126개의 모든 입력을 빠짐없이 대조한다.
TEST_CASE(ProductionHintGump_Native126) {
    std::size_t count{}; // 실제 비교한 원문 행 수다.
    // fixture는 읽기만 하며 실행 중 기대값을 갱신하지 않는다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_PRODUCTION_HINT_FIXTURE).rows) {
        CHECK(row.size()==5);if (row.size()!=5) return;const auto v=Split(row[1],',');HintScene s;ProductionHintGump g(s.create,s.state,s.Hooks());
        const auto d=[&](std::size_t i) { return std::bit_cast<double>(std::stoull(v.at(i))); }; // 명시 double 입력 비트다.
        const auto u=[&](std::size_t i) { return static_cast<std::uint32_t>(std::stoull(v.at(i))); }; // 명시 DWORD 입력이다.
        if (row[0]=="C") { s.now=d(0);s.state.white=u(2);s.state.black=u(3);s.state.kind=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(u(4)));const std::array<std::string_view,3> text{"","~0~w~ENot enough Storm Power to build this","~0~w~ENone in inventory"};g.Construct(kNode,text.at(u(1))); }
        else if (row[0]=="I") { s.node.until=d(0);s.now=d(1);g.Idle(kNode); }
        else if (row[0]=="D") { s.node.head=u(0);s.point={std::bit_cast<std::int32_t>(u(1)),std::bit_cast<std::int32_t>(u(2))};g.Draw(kNode); }
        else if (row[0]=="X") { s.node.owned=u(0) ? kOwned : 0;CHECK(g.DeleteScalar(kNode,u(1))==kNode); }
        else { CHECK(row[0]=="W");s.now=d(3);s.box=u(4) ? ScreenRect{-17,20,2147483647,static_cast<std::int32_t>(0x80000000U)} : ScreenRect{0,0,36,36};g.Show(u(0)!=0,std::bit_cast<std::int32_t>(u(1)),std::bit_cast<std::int32_t>(u(2)),kReference); }
        if (s.Snapshot()!=row[2] || s.trace!=row[4]) std::printf("생산 안내 차이 %zu %s\n%s\n%s\n",count,row[1].c_str(),s.Snapshot().c_str(),s.trace.c_str());
        CHECK(s.Snapshot()==row[2]);CHECK(s.state.lastSerial==Number(row[3]));CHECK(s.trace==row[4]);++count;
    }
    CHECK(count==126);
}
