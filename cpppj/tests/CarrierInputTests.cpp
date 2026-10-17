// C++ 작성 전에 동결한 보호PE의 전체 부모/효과 순서/원문 바이트를 대조한다.
#include "RawSceneSupport.h"
#include "client/RawCarrierInput.h"
#include <iostream>
using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 독립 입력과 현재 저장소만 구성하며 커서/분기 기대값은 fixture에서 읽는다.
struct CarrierInputScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; //원본과 같은 물리50BYTE 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //효과 뒤 바뀌는 실제 TYPE다.
    std::array<CarrierInputType,188> audio{}; //명시 현재 move123/blocked124 입력이다.
    std::array<std::uint32_t,188> stamps{}; //같은 현재 TYPE+1f0 장부다.
    std::array<std::uint32_t,2> markers{99,100}; //독립 원문에 공급한 실제 두 표시 타입이다.
    CarrierInputState state; //원본 Carrier.cpp 전역의 현재 상태다.
    std::vector<std::uint32_t> c; //독립 입력33DWORD다.
    std::uint32_t frame{},local{}; //현재 효과 뒤 다시 읽는 프레임/owner다.
    std::string trace; //실제 호출 순서이며 결과를 공급하지 않는다.
    // 검사만 물리 슬롯에 입력을 쓴다. 제품은 읽기 전용이다.
    std::span<std::uint8_t> Raw(Sid sid) { auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
    // 실제 두 float의 DWORD 비트다.
    static std::uint32_t B(float value) { return std::bit_cast<std::uint32_t>(value); }
    // 원문과 같은 전체 인자를 순서대로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) { if (!trace.empty()) trace+=';';trace+=name;
        // 각 원문DWORD를 부호변환 없이 남긴다.
        for (auto v:values) trace+=':'+std::to_string(v);
    }
    // 원문 관측기의 명시 외부 변경만 재생한다.
    void Mutate(std::string_view where) {
        const auto m=c[32];if (m==1 && where=="assert") types[c[29]].flags2=0x20000;if (m==2 && where=="immobile") Raw(Sid{50})[40]=0x20;
        if (m==3 && where=="candidate") types[c[29]].flags2=0x20000;if (m==4 && where=="candidate") types[c[29]].flags1=0x1000000;
        if (m==5 && where=="ack") Raw(Sid{50})[34]=2;if (m==6 && where=="allowed") local=2;if (m==7 && where=="select") state.cursor=18;if (m==8 && where=="early") Raw(Sid{50})[34]=2;
    }
    // 기대출력 없이 현재 원문 입력만 초기화한다.
    explicit CarrierInputScene(const std::vector<std::string>& row) {
        // 입력필드를 원문 순서대로 읽는다.
        for (const auto& v:Split(row[0],',')) c.push_back(Number(v));CHECK(c.size()==33);frame=c[27];local=c[5];state.cursor=c[25];state.dais=77;state.insidePoint={11,12};state.markers=c[22]!=0;state.markerCounter=1;state.markerTypes=markers;types[c[29]].flags1=c[2];types[c[29]].flags2=c[1];audio[c[29]]={123,124};stamps[c[29]]=c[26];
        // 독립 원문과 같은 여섯 물리 슬롯을 바이트별로 준비한다.
        for (const auto id:{50,60,61,62,63,64}) { auto raw=Raw(Sid{static_cast<std::uint16_t>(id)});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(c[29]);raw[34]=static_cast<std::uint8_t>(c[4]);raw[40]=static_cast<std::uint8_t>(c[3]);Put(raw,14,B(20));Put(raw,18,B(21)); }Put(Raw(Sid{60}),18,134,2);
    }
    // 기존 큰 하위의 독립 입력만 공급하고 부모의 실제 분기를 관측한다.
    CarrierInputHooks Hooks() {
        CarrierInputHooks h;h.assertion=[this](std::uint32_t line) { Event("I",{line});Mutate("assert"); };h.immobile=[this](Sid sid) { Event("H",{sid.value});Mutate("immobile");return c[6]!=0; };
        h.content=[this](Sid sid,std::uint32_t mask) { Event("C",{sid.value,mask,0,0});return Sid{static_cast<std::uint16_t>(mask==0x20 ? c[7] : 0)}; };
        h.candidate=[this](Sid sid,RawPathPoint p) { Event("Q",{sid.value,B(p.x),B(p.y)});Mutate("candidate");return std::pair{Sid{static_cast<std::uint16_t>(c[8])},c[9]!=0}; };
        h.dropPoint=[this](Sid sid,std::uint32_t type) { Event("D",{sid.value,type});return std::array<float,2>{30,31}; };
        h.graph=[this](Sid sid) { Event("G",{sid.value});return sid.value==50 || c[11] ? c[10] : c[10]+1; };
        h.connected=[this](RawPathPoint a,RawPathPoint b) { Event("L",{B(a.x),B(a.y),B(b.x),B(b.y)});return c[12]!=0; };h.surface=[this](RawPathPoint p) { Event("S",{B(p.x),B(p.y)});return c[13]; };
        h.geyser=[this](RawPathPoint p) { Event("Y",{B(p.x),B(p.y),122});return Sid{static_cast<std::uint16_t>(c[14])}; };h.validGeyser=[this](Sid sid,Sid target) { Event("V",{sid.value,target.value});return c[15]!=0; };
        h.returnPlace=[this](RawPathPoint p) { Event("R",{B(p.x),B(p.y),0});return Sid{static_cast<std::uint16_t>(c[16])}; };h.daisAt=[this](RawPathPoint p) { Event("A",{B(p.x),B(p.y),0x400000});return Sid{static_cast<std::uint16_t>(c[17])}; };
        h.altarAllowed=[this](Sid dais,Sid sid) { Event("F",{dais.value,sid.value});Mutate("allowed");return c[18]!=0; };h.earlySacrifice=[this] { Event("E");Mutate("early");return c[19]!=0; };
        h.localOwner=[this] { return local; };h.multiple=[this] { Event("M");return c[21]; };h.checking=[this] { return c[20]!=0; };
        h.select=[this](std::uint32_t a,std::uint32_t b,std::uint32_t d) { Event("U",{a,b,d});Mutate("select"); };h.highlight=[this](std::uint32_t mode) { Event("B",{mode}); };h.selectCursor=[this](std::uint32_t mode) { Event("X",{mode}); };
        h.request=[this](Sid sid,RawPathPoint p,std::uint32_t sacrifice) { Event("T",{sid.value,B(p.x),B(p.y),sacrifice}); };h.pickup=[this](Sid sid) { Event("P",{sid.value}); };h.dropCargo=[this](Sid sid,std::uint32_t v) { Event("N",{sid.value,v}); };
        h.earlyLocal=[this] { Event("W"); };h.earlyRemote=[this](std::uint32_t owner) { Event("J",{owner});trace+=":EarlySacrifice"; };h.showSacrifice=[this](std::uint32_t dais) { Event("K",{dais}); };
        h.marker=[this](RawPathPoint p,std::uint32_t type) { Event("Z",{B(p.x),B(p.y),type}); };h.refreshCursor=[this] { Event("O"); };
        h.random=[this](std::int32_t lo,std::int32_t hi) { Event("?",{static_cast<std::uint32_t>(lo),static_cast<std::uint32_t>(hi)});return 1000; };
        h.namedSound=[this](bool priest,std::uint32_t index) { const std::array<std::uint32_t,2> priests{1,3};const std::array<std::uint32_t,4> golems{1,2,4,5}; //보호 PE의 실제 이름 표는 연속 번호가 아니다.
            const auto name=!priest && index==4 ? "@1" : std::string(priest ? "priestMove" : "golemMove")+std::to_string(priest ? priests.at(index) : golems.at(index))+".wav";Event("a:"+name,{0,0,0,1,0});Mutate("ack"); };
        h.sound=[this](std::uint32_t handle) { Event("s",{handle,0,0,0,1,0});Mutate("ack"); };return h;
    }
    // 실제 부모를 실행한 뒤 원문 전체 상태를 직렬화한다.
    std::vector<std::string> Run() {
        RawCarrierInput body(pool,types,audio,stamps,frame,state,Hooks());RawPathPoint point{std::bit_cast<float>(c[30]),std::bit_cast<float>(c[31])};std::string result="-";
        if (c[0]==0) result=std::to_string(body.Evaluate(Sid{50},point));else if (c[0]==1) body.Highlight(c[24],point,c[23]);else if (c[0]==2) body.Commit(c[24],point);else body.Acknowledge(Sid{50},std::bit_cast<std::int32_t>(c[28]));
        std::array<std::uint8_t,8> p{};Put(p,0,B(point.x));Put(p,4,B(point.y));std::string globals,raws;
        // 원문의 별도 DWORD와 여섯 슬롯 전체를 비교한다.
        for (const auto value:{state.cursor,state.dais,B(state.insidePoint[0]),B(state.insidePoint[1]),std::bit_cast<std::uint32_t>(state.markerCounter),local}) { if (!globals.empty()) globals+=',';globals+=std::to_string(value); }
        for (const auto sid:{50,60,61,62,63,64}) { if (!raws.empty()) raws+=';';raws+=Hex(Raw(Sid{static_cast<std::uint16_t>(sid)})); }
        const auto& type=types[c[29]];return {result,trace.empty() ? "-" : trace,Hex(p),globals,std::to_string(type.flags1)+','+std::to_string(type.flags2)+','+std::to_string(stamps[c[29]]),raws};
    }
};
}
// 원본의 모든 입력을 두 정밀도의 동일 출력과 대조한다.
TEST_CASE(CarrierInput_Native454WholeParentsAndCurrentEffects) {
    std::size_t count{}; //실제로 대조한 독립 입력 수다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIER_INPUT_FIXTURE).rows) { CarrierInputScene scene(row);const auto actual=scene.Run();const std::vector<std::string> expected(row.begin()+1,row.begin()+7);CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t i=0;i<actual.size();++i) if (actual[i]!=expected[i]) std::cout<<"CarrierInput row"<<count<<" col"<<i<<" expected="<<expected[i]<<" actual="<<actual[i]<<'\n';return; }CHECK(expected==std::vector<std::string>(row.begin()+7,row.end()));++count;
    }CHECK(count==454);
}
