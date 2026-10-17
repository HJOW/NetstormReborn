// 독립 보호PE의 이동 요청/수신을 현재 원문과 전체 경로 바이트로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierMove.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원문 입력26DWORD를 비트 변환 없이 읽는다.
std::vector<std::uint32_t> MoveNumbers(std::string_view text) { std::vector<std::uint32_t> values;
    //부호 비트를 유지하고 열 순서대로 읽는다.
    for (const auto& part:Split(text,',')) values.push_back(Number(part));return values;
}
// 큰 외부 하위의 명시 입력/변경만 재생하고 제품이 분기·패킷·현재 읽기를 수행한다.
struct MoveScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; //보호PE와 같은 물리50BYTE 슬롯이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //효과 뒤 바뀔 수 있는 현재 TYPE다.
    bool fort{}; //594fb8의 현재 fort 모드이며 에디터와 다르다.
    CarrierMoveState state; //현재 요청 BYTE/BYTE/WORD다.
    RawPathRuntime path{OriginalEdition::Patch1078}; //같은100076BYTE 경로이며 기대 결과를 주입하지 않는다.
    std::vector<std::uint32_t> row; //원문 독립 입력이다.
    std::array<std::uint8_t,18> received{}; //효과가 변경할 수 있는 caller 소유 수신 버퍼다.
    std::string trace,packet; //실제 외부 호출 순서와 송신 바이트다.
    //할당 상태와 별개인 원본 물리 입력을 준비한다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
    //독립 입력과 전체 경로에 대한 검사 전용 쓰기다.
    std::span<std::uint8_t> PathBytes() { const auto bytes=path.Bytes();return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
    //원본 실제 인자 순서를 DWORD로 관측한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> args={}) { if (!trace.empty()) trace+=';';trace+=name;
        //원문과 같은 전체DWORD 인자를 남긴다.
        for (const auto value:args) trace+=':'+std::to_string(value);
    }
    //원본 관측기에 공급한 외부 변경만 재생한다.
    void Mutate(std::string_view where) {
        const auto m=row[17];
        if (m==1 && where=="select") types[133].flags2=0;
        if (m==2 && where=="assert") types[133].flags2=0x20000;
        if (m==3 && where=="connected") Raw(Sid{50})[10]=134;
        if (m==4 && where=="movement") { state.request.protocol=0xff;state.request.length=3; }
        if (m==5 && where=="path") { Put(PathBytes(),0x186dc,row[18]);Put(PathBytes(),0x186e0,row[19]); }
        if (m==6 && where=="select") Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(24.5F));
        if (m==7 && where=="select") types[133].flags2=0x20000;
        if (m==8 && where=="select") state.request.message=0x55;
        if (m==9 && where=="assert") types[133].flags2=0x10000;
        if (m==10 && where=="connected") types[133].flags2=0;
        if (m==11 && where=="stop") Raw(Sid{50})[34]=2;
        if (m==12 && where=="stop") fort=true;
        if (m==13 && where=="move") { Put(received,4,51,2);Put(received,6,std::bit_cast<std::uint32_t>(24.5F)); }
        if (m==14 && where=="candidate") { Put(received,4,51,2);Put(received,10,std::bit_cast<std::uint32_t>(25.5F)); }
        if (m==15 && where=="return") { Put(received,4,51,2);Put(received,6,std::bit_cast<std::uint32_t>(26.5F)); }
        if (m==16 && where=="genus") { Put(received,4,51,2);Put(received,10,std::bit_cast<std::uint32_t>(27.5F)); }
        if (m==17 && where=="allowed") Put(received,14,0);
        if (m==18 && where=="allowed") Put(received,4,51,2);
        if (m==19 && where=="candidate") types[133].flags2=0x10000;
        if (m==20 && where=="return") Put(received,14,0);
    }
    //실제 원문 입력만 두 슬롯/Path/헤더에 설치한다.
    void Prepare(std::vector<std::uint32_t> values) {
        row=std::move(values);CHECK(row.size()==26);trace.clear();packet="-";fort=row[5]!=0;state.request={static_cast<std::uint8_t>(row[23]),static_cast<std::uint8_t>(row[22]),static_cast<std::uint16_t>(row[24])};
        //콜백 대체 부모도 별개 물리 슬롯으로 유지한다.
        for (const auto sid:{Sid{50},Sid{51}}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x12200000);raw[10]=133;raw[11]=static_cast<std::uint8_t>(row[2]);raw[34]=static_cast<std::uint8_t>(row[3]);Put(raw,14,std::bit_cast<std::uint32_t>(20.5F));Put(raw,18,std::bit_cast<std::uint32_t>(21.5F)); }
        //경로 접두의 미사용 바이트 보존도 원본 Adler와 대조한다.
        std::fill(PathBytes().begin(),PathBytes().end(),std::uint8_t{0xa5});Put(PathBytes(),0x186dc,row[20]);Put(PathBytes(),0x186e0,row[21]);
        //사용 중인 두 TYPE 이외에는 원문처럼 빈 입력이다.
        for (auto& type:types) type.flags2=0;types[133].flags2=row[1];types[134].flags2=0x20000;
        received={static_cast<std::uint8_t>(row[22]&0x7f),static_cast<std::uint8_t>(row[23])};Put(received,2,row[24],2);Put(received,4,50,2);Put(received,6,row[18]);Put(received,10,row[19]);Put(received,14,row[14]);
    }
    //큰 하위만 원래 인자로 기록하며 계산 결과는 제품에 맡긴다.
    CarrierMoveHooks Hooks() {
        CarrierMoveHooks hooks;
        hooks.selected=[this] { Event("S");return Sid{static_cast<std::uint16_t>(row[6])}; };
        hooks.select=[this](std::uint32_t a,std::uint32_t b,std::uint32_t c) { Event("C",{a,b,c});Mutate("select"); };
        hooks.assertion=[this](std::uint32_t line) { Event("I",{line});Mutate("assert"); };
        hooks.connected=[this](RawPathPoint source,RawPathPoint destination) { Event("L",{std::bit_cast<std::uint32_t>(source.x),std::bit_cast<std::uint32_t>(source.y),std::bit_cast<std::uint32_t>(destination.x),std::bit_cast<std::uint32_t>(destination.y)});Mutate("connected");return row[7]!=0; };
        hooks.movement=[this](std::uint32_t type) { Event("M",{type,0,0});Mutate("movement");return 40U; };
        hooks.path=[this](Sid sid,std::uint32_t type) { Event("P",{sid.value,type});Mutate("path");return row[8] ? &path : nullptr; };
        hooks.send=[this](std::int32_t target,std::span<const std::uint8_t> bytes) { packet=Hex(bytes);if (!trace.empty()) trace+=';';trace+="T:"+std::to_string(target)+":"+packet; };
        hooks.regular=[this](Sid sid,std::uint32_t event) { Event("E",{sid.value,event});Mutate(event==0x5105 ? "stop" : "move");return row[event==0x5105 ? 15 : 16]!=0; };
        hooks.candidate=[this](Sid sid,RawPathPoint point) { Event("Q",{sid.value,std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y)});Mutate("candidate");return std::pair{Sid{static_cast<std::uint16_t>(row[9])},true}; };
        hooks.returnPlace=[this](RawPathPoint point) { Event("R",{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y),0});Mutate("return");return Sid{static_cast<std::uint16_t>(row[10])}; };
        hooks.target=[this](Sid parent,Sid target) { Event("D",{parent.value,target.value,0,0}); };
        hooks.genusAt=[this](RawPathPoint point,std::uint32_t genus) { Event("G",{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y),genus});Mutate("genus");return Sid{static_cast<std::uint16_t>(row[genus==0x10000000 ? 11 : 12])}; };
        hooks.geyser=[this](Sid parent,Sid target) { Event("Y",{parent.value,target.value});return row[25] ? &path : nullptr; };
        hooks.altarAllowed=[this](Sid altar,Sid parent) { Event("A",{altar.value,parent.value});Mutate("allowed");return row[13]!=0; };
        hooks.sacrifice=[this](Sid parent,Sid altar) { Event("F",{parent.value,altar.value}); };
        hooks.start=[this](RawPathStartRequest r) { Event("B",{r.parent.value,std::bit_cast<std::uint32_t>(r.destination.x),std::bit_cast<std::uint32_t>(r.destination.y),r.replaceStart,r.helper,r.tailWord,r.tailByte,r.reserved});return row[25] ? &path : nullptr; };return hooks;
    }
    //효과 뒤 현재 전역을 원문과 같은 순서로 읽는다.
    std::string State() const { return std::to_string(fort)+","+std::to_string(types[133].flags2)+","+std::to_string(state.request.protocol)+","+std::to_string(state.request.message)+","+std::to_string(state.request.length); }
};
}
//두 정밀도에서 동결한982 독립 입력 전수를 제품에 대조한다.
TEST_CASE(CarrierMove_NativeRequestAndReceive) {
    MoveScene scene;std::size_t count{},diagnostics{}; //입력 전수와 최초 불일치 출력 제한이다.
    //원본 결과는 읽기만 하며 제품의 실제 효과를 비교한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_MOVE_FIXTURE).rows) {
        CHECK(fields.size()==11);if (fields.size()!=11) continue;scene.Prepare(MoveNumbers(fields[0]));RawCarrierMove move(scene.pool,scene.types,scene.fort,scene.state,scene.Hooks());
        if (!scene.row[0]) move.Request(Sid{50},{std::bit_cast<float>(scene.row[18]),std::bit_cast<float>(scene.row[19])},scene.row[14]);else move.Receive(scene.row[4],scene.received);
        const std::array actual{scene.trace.empty() ? std::string("-") : scene.trace,scene.State(),scene.packet,Hex(scene.Raw(Sid{50}))+","+Hex(scene.Raw(Sid{51})),std::to_string(Adler(scene.path.Bytes().first(0x186ec)))};
        //각 관측 전체를 비교하고 실패한 입력/효과를 보존한다.
        for (std::size_t i=0;i<actual.size();++i) { if (actual[i]!=fields[i+1] && diagnostics++<8) std::cout<<"이동입력 "<<fields[0]<<" 열"<<i<<"\n실제 "<<actual[i]<<"\n원본 "<<fields[i+1]<<'\n';CHECK(actual[i]==fields[i+1]);CHECK(fields[i+1]==fields[i+6]); }++count;
    }
    CHECK(count==982);
}
//수신 물리 범위와 도달한 하위 누락은 월드 변경 전에 드러난다.
TEST_CASE(CarrierMove_LazyBoundariesAndPhysicalLength) {
    MoveScene scene;scene.Prepare(MoveNumbers("1,65536,1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1102315520,1102839808,0,0,1,64,18,1"));RawCarrierMove move(scene.pool,scene.types,scene.fort,scene.state,{});CHECK(&move.Pool()==&scene.pool);move.Receive(1,scene.received);
    bool rejected{};try { move.Receive(1,std::span(scene.received).first(17)); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    rejected=false;try { move.Request(Sid{50},{0,0},0); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}
