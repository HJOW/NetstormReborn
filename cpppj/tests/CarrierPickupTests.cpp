// 독립원문의일반포획전체효과와현재슬롯/종속finder를대조한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierPickup.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원문입력DWORD의비트를그대로읽는다.
std::vector<std::uint32_t> PickupNumbers(std::string_view text) { std::vector<std::uint32_t> values;
    // 각칸을독립값으로읽으며부호변환을하지않는다.
    for (const auto& part:Split(text,',')) values.push_back(Number(part));return values;
}
// 원본과같은큰외부효과입력만공급하고분기/임시dead/종속선택은제품이수행한다.
struct CarrierPickupScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원문의50바이트물리슬롯이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재TYPE/Genus이다.
    ContainedFinderState contained; // 원본실제종속finder가읽는현재전역이다.
    bool& boss=contained.boss; // 원본동일권한전역을두본문이공유한다.
    bool single{}; //594fa4의현재단독미션입력이다.
    CarrierPickupState state; // 현재요청헤더/입력명령/특정타입이다.
    std::vector<std::uint32_t> row; // 독립원문29DWORD 입력이다.
    std::string trace,packet; // 실제호출순서와실제송신바이트다.
    Sid target{60}; // 외부변환/입력큐효과가관찰할현재대상이다.
    std::span<std::uint8_t> currentPacket; // caller 소유수신버퍼의별명입력이다.
    // 입력은할당여부와별개인원본물리raw이며제품의공개쓰기API를추가하지않는다.
    std::span<std::uint8_t> Raw(Sid sid) { auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
    // 관측한인자를DWORD로기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) { if (!trace.empty()) trace+=';';trace+=name;
        // 실제폭과인자순서를보존한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 원문에명시한외부변경만재생하며예상반환/순서는계산하지않는다.
    void Mutate(std::string_view where,Sid sid={}) {
        const auto m=row[17];
        if (m==1 && where=="candidate") boss=false;
        if (m==2 && where=="candidate") { single=false;Raw(Sid{60})[10]=155; }
        if (m==3 && where=="assertion") boss=true;
        if (m==4 && where=="reveal") { Put(currentPacket,4,51,2);state.command=0xffffffff; }
        if (m==5 && where=="queue") { Raw(target)[10]=155;Put(Raw(target),12,0x2222,2); }
        if (m==6 && where=="find") state.silentType=169;
        if (m==7 && where=="destroy-old") state.silentType=row[13];
        if (m==8 && where=="after-discard") { Raw(target)[10]=133;Put(Raw(target),12,0x3333,2); }
        if (m==9 && where=="create") Raw(target)[11]=0x0f;
        if (m==10 && where=="create") Put(Raw(target),12,0x4444,2);
        if (m==11 && where=="detach" && sid==Sid{80}) Put(Raw(Sid{82}),4,0,2);
        if (m==12 && where=="detach" && sid==Sid{80}) Raw(Sid{82})[10]=7;
        if (m==13 && where=="attach") Raw(target)[10]=70;
        if (m==14 && where=="create") contained.containedType=3;
        if (m==15 && where=="destroy-old") types[Raw(target)[10]].flags2=0x100;
        if (m==16 && where=="reveal") boss=false;
        if (m==17 && where=="queue") Put(currentPacket,4,51,2);
        if (m==18 && where=="create") Raw(target)[10]=70;
        if (m==19 && where=="send") state.request={0x55,0xff,3};
    }
    // 실제입력raw/현재TYPE/현재전역만설치한다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);CHECK(row.size()==29);boss=row[1]!=0;single=row[2]!=0;contained.containedType=row[22];contained.checkingDead=row[23]!=0;state={{static_cast<std::uint8_t>(row[19]),static_cast<std::uint8_t>(row[18]),static_cast<std::uint16_t>(row[20])},row[15],row[14]};trace.clear();packet="-";target=Sid{60};
        // 원문과같은아홉슬롯과미사용바이트패턴이다.
        for (const auto sid:{Sid{50},Sid{51},Sid{52},Sid{60},Sid{61},Sid{70},Sid{80},Sid{81},Sid{82}}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(sid.value==60 ? row[4] : sid.value==61 ? row[10] : sid.value==50 || sid.value==51 ? 133U : sid.value==81 ? 3U : 6U);raw[11]=static_cast<std::uint8_t>(sid.value==50 ? row[6] : sid.value==60 ? row[7] : sid.value>=80 ? 8U : 0U);raw[40]=0;Put(raw,14,row[26]);Put(raw,18,row[27]);Put(raw,4,0,2);Put(raw,6,0,2); }
        Put(Raw(Sid{52}),18,row[13],2);Put(Raw(Sid{50}),6,row[12],2);
        const auto chain=row[16]==0 ? std::vector<Sid>{} : row[16]==1 ? std::vector<Sid>{Sid{80},Sid{82}} : row[16]==2 ? std::vector<Sid>{Sid{80},Sid{81},Sid{82}} : std::vector<Sid>{Sid{81},Sid{80},Sid{82}};
        // 오벨리스크변환은큰명시효과이며두대상은같은종속입력을가진다.
        for (const auto sid:{Sid{60},Sid{61}}) { Put(Raw(sid),6,chain.empty() ? 0U : chain.front().value,2);Put(Raw(sid),12,row[21],2); }
        // 현재next를바꾸는효과보다앞서기본finder가선택한다.
        for (std::size_t i=0;i<chain.size();++i) Put(Raw(chain[i]),4,i+1<chain.size() ? chain[i+1].value : 0U,2);
        // 현재genus만입력하며선택결과를별도공급하지않는다.
        for (auto& type:types) type.flags2=0;types[row[4]].flags2=row[8];types[row[10]].flags2=row[11];types[155].flags2=0x2100;types[133].flags2=0x100;
    }
    // 현재외부효과를원래인자로만관찰한다.
    CarrierPickupHooks Hooks() {
        CarrierPickupHooks hooks;
        hooks.candidate=[this](Sid sid,float x,float y) { Event("P",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});Mutate("candidate");return std::pair{Sid{static_cast<std::uint16_t>(row[3])},row[25]!=0}; };
        hooks.send=[this](std::int32_t owner,std::span<const std::uint8_t> bytes) { packet=Hex(bytes);if (!trace.empty()) trace+=';';trace+="S:"+std::to_string(owner)+":"+packet;Mutate("send"); };
        hooks.assertion=[this](std::uint32_t line) { Event("I",{line});Mutate("assertion"); };
        hooks.reveal=[this](Sid sid) { Event("V",{sid.value});target=Sid{static_cast<std::uint16_t>(row[9])};Mutate("reveal");return target; };
        hooks.queue=[this](std::uint32_t command,std::int32_t sid,std::int32_t parent) { target=Sid{static_cast<std::uint16_t>(sid)};Event("Q",{command,static_cast<std::uint32_t>(sid),static_cast<std::uint32_t>(parent)});Mutate("queue"); };
        hooks.find=[this](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { Event("F",{sid.value,mask,kind,index});Mutate("find");return Sid{static_cast<std::uint16_t>(row[12])}; };
        hooks.afterDiscard=[this] { Event("B");Mutate("after-discard"); };
        hooks.create=[this](Sid parent,std::uint32_t kind,std::uint32_t argument,std::uint32_t flags,std::uint32_t attachFlags,std::uint32_t quality) { Event("C",{parent.value,kind,argument,flags,attachFlags,quality,Raw(target)[11]});Mutate("create");return Sid{static_cast<std::uint16_t>(row[28])}; };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event("D",{sid.value,flags,Raw(sid)[11],Raw(sid)[10]});Mutate(sid==Sid{52} ? "destroy-old" : "destroy-target",sid); };
        hooks.detach=[this](Sid sid,std::uint32_t flags) { Event("U",{sid.value,flags});Mutate("detach",sid); };
        hooks.attach=[this](Sid sid,Sid parent,std::uint32_t flags) { Event("A",{sid.value,parent.value,flags});Mutate("attach",sid); };return hooks;
    }
    // 실제최종현재전역을원문과같은순서로읽는다.
    std::string State() const { std::string result;
        // bool/WORD도원문의DWORD표현으로직렬화한다.
        for (const auto value:{std::uint32_t(boss),std::uint32_t(single),state.silentType,state.command,contained.containedType,std::uint32_t(state.request.protocol),std::uint32_t(state.request.message),std::uint32_t(state.request.length)}) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
    }
};
}
// 독립원문의요청/서버/기존내용물제거를실제제품과전체raw로대조한다.
TEST_CASE(CarrierPickup_NativeBodies) {
    CarrierPickupScene scene;std::size_t count{},diagnostics{}; // 대조한입력수와최초불일치기록수다.
    // 원본입력은재생하지만원본결과는계산하지않는다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_PICKUP_FIXTURE).rows) {
        CHECK(fields.size()==9);if (fields.size()!=9) continue;scene.Prepare(PickupNumbers(fields[0]));RawCarrierPickup pickup(scene.pool,scene.types,scene.contained,scene.boss,scene.single,scene.state,scene.Hooks());
        if (scene.row[0]==0) pickup.Request(Sid{50});else if (scene.row[0]==2) pickup.Discard(Sid{50});else { std::array<std::uint8_t,9> packet{static_cast<std::uint8_t>(scene.row[18]&0x7f),static_cast<std::uint8_t>(scene.row[19])};Put(packet,2,scene.row[20],2);Put(packet,4,50,2);Put(packet,6,60,2);packet[8]=static_cast<std::uint8_t>(scene.row[5]);scene.currentPacket=packet;pickup.Receive(scene.row[24],0,packet); }
        std::string raws;
        // 원문과같은아홉물리슬롯의미사용바이트도보존했는지대조한다.
        for (const auto sid:{Sid{50},Sid{51},Sid{52},Sid{60},Sid{61},Sid{70},Sid{80},Sid{81},Sid{82}}) { if (!raws.empty()) raws+=',';raws+=Hex(scene.Raw(sid)); }
        const auto trace=scene.trace.empty() ? "-" : scene.trace,state=scene.State();
        if ((trace!=fields[1] || state!=fields[2] || raws!=fields[3] || scene.packet!=fields[4]) && diagnostics++<5) std::cout<<"포획입력 "<<fields[0]<<"\n실제 "<<trace<<' '<<state<<' '<<raws<<' '<<scene.packet<<"\n원본 "<<fields[1]<<' '<<fields[2]<<' '<<fields[3]<<' '<<fields[4]<<'\n';
        CHECK(trace==fields[1]);CHECK(state==fields[2]);CHECK(raws==fields[3]);CHECK(scene.packet==fields[4]);CHECK(fields[1]==fields[5] && fields[2]==fields[6] && fields[3]==fields[7] && fields[4]==fields[8]);++count;
    }
    CHECK(count==2657);
}
// 선택하지않은효과는요구하지않으며실제도달한경계누락만거부한다.
TEST_CASE(CarrierPickup_LazyBoundariesAndPacketBounds) {
    CarrierPickupScene scene;CarrierPickupHooks hooks;hooks.candidate=[](Sid,float,float) { return std::pair{Sid{},false}; };RawCarrierPickup pickup(scene.pool,scene.types,scene.contained,scene.boss,scene.single,scene.state,std::move(hooks));pickup.Request(Sid{50});CHECK(&pickup.Pool()==&scene.pool);
    std::array<std::uint8_t,8> shortPacket{};bool rejected{};try { pickup.Receive(0xffffffff,0xffffffff,shortPacket); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    RawCarrierPickup missing(scene.pool,scene.types,scene.contained,scene.boss,scene.single,scene.state,{});rejected=false;try { missing.Discard(Sid{50}); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}
