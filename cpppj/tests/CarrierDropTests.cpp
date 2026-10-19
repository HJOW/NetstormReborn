// 동결 원본413입력의 Cast/Drop 요청·메뉴·서버 효과/패킷/전체 슬롯을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierDrop.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 검증 원문의 평탄한 정수 JSON 입력만 읽으며 출력이나 게임 분기를 계산하지 않는다.
struct DropInput {
    std::string text; //원문 정수 객체이며 모든 키는 ASCII 고정 문자열이다.
    std::uint32_t operator()(std::string_view key) const { //지정된 정수 필드를 찾아 부호 없는 DWORD로 읽는다.
        const auto token='"'+std::string(key)+"\":";const auto position=text.find(token);if (position==std::string::npos) throw std::runtime_error("Drop 입력 키 누락");return static_cast<std::uint32_t>(std::stoull(text.substr(position+token.size())));
    }
};
// 원문과 같은 외부 경계 입력만 공급하며 요청/수신 분기는 제품 몸체가 실행한다.
struct DropScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; //원본50BYTE 물리 슬롯의 저장소다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //각 입력의 현재 genus 표다.
    ContainedFinderState contained; //실제 제품 contained finder를 사용한다.
    CarrierDropState state; //현재 헤더와 소유자/진단 전역이다.
    DropInput input; //큰 외부 효과의 독립 입력이다.
    std::map<std::uint32_t,std::int32_t> costs; //원본의 인코딩 비용을 복호화한 현재 단위 값이다.
    std::string trace,packet; //실제 효과 순서와 실제 요청 패킷이다.
    std::span<std::uint8_t> receivePacket; //외부 효과가 바꾸는 호출자 소유 버퍼의 별명이다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } //검증 입력을 물리 슬롯에만 설치한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) { //관측된 인자들을 원문 형식으로 직렬화한다.
        if (!trace.empty()) trace+=';';trace+=name;
        // 각 인자의 실제 DWORD 비트를 유지한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    void Blob(std::string_view name,std::uint32_t target,std::span<const std::uint8_t> bytes,bool includeTarget) { //실제 패킷 바이트만 원문 효과 뒤에 붙인다.
        if (!trace.empty()) trace+=';';trace+=name;if (includeTarget) trace+=':'+std::to_string(target);trace+=':'+Hex(bytes);
    }
    void Prepare(std::string text) { //원본 입력 raw/전역만 준비하고 예상 반환/효과는 공급하지 않는다.
        input.text=std::move(text);trace.clear();packet="-";costs.clear();state=CarrierDropState{};
        state.localOwner=input("local");state.server=input("server");state.checking=input("checking");state.numTypes=input("count");
        state.request={static_cast<std::uint8_t>(input("message")),static_cast<std::uint8_t>(input("protocol")),static_cast<std::uint16_t>(input("length"))};
        // 원문과 같은7개 슬롯의 미사용 바이트도 매 입력 초기화한다.
        for (const auto sid:{Sid{50},Sid{60},Sid{61},Sid{62},Sid{70},Sid{80},Sid{81}}) {
            auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);
            raw[10]=static_cast<std::uint8_t>(sid.value==50 ? 133 : sid.value>=60 && sid.value<=62 ? input("contentType") : sid.value==80 ? 51 : 52);
            raw[11]=static_cast<std::uint8_t>(sid.value==50 ? input("parentFlags") : sid.value==60 ? input("contentFlags") : 0);raw[34]=static_cast<std::uint8_t>(input("owner"));raw[40]=static_cast<std::uint8_t>(input("extra"));
            Put(raw,14,input("x"));Put(raw,18,input("y"));
            if (sid.value>=60 && sid.value<=62) { Put(raw,18,sid.value==60 ? input("kind") : 169,2);raw[22]=static_cast<std::uint8_t>(input("flag"));raw[20]=static_cast<std::uint8_t>(sid.value==60 ? input("mark") : 1); }
        }
        std::copy(Raw(Sid{50}).begin(),Raw(Sid{50}).end(),Raw(Sid{51}).begin());const auto count=input("chain");Put(Raw(Sid{50}),6,count ? 60 : 0,2);
        // 현재 raw의 next를 실제 제품 finder가 따라가도록 구성한다.
        for (std::uint32_t index=0;index<count;++index) Put(Raw(Sid{static_cast<std::uint16_t>(60+index)}),4,index+1<count ? 61+index : 0,2);
        // 원문의 TYPE/비용 입력을 현재 값으로 설치한다.
        for (auto& type:types) type.flags2=0;
        types[133].flags2=input("carrierGenus");types[168].flags2=input("genus");
        // 동일한 기본 비용 네 항목은 외부 효과가 나중에 바꿀 수 있다.
        for (const auto kind:{0U,168U,169U,188U}) costs[kind]=static_cast<std::int32_t>(input("cost"));
    }
    CarrierDropHooks Hooks() { //원본에 선언된 큰 외부 효과만 동일 입력과 인자로 재생한다.
        CarrierDropHooks hooks;
        hooks.immobile=[this](Sid parent) { Event("immobile",{parent.value});return input("immobile")!=0; };
        hooks.cost=[this](std::uint32_t type) { return costs.at(type); };
        hooks.afford=[this](float value) { Event("afford",{std::bit_cast<std::uint32_t>(value)});if (input("mutation")==1) costs[input("kind")]=77;return input("afford")!=0; };
        hooks.subtract=[this](std::uint32_t owner,float value) { Event("subtract",{owner,std::bit_cast<std::uint32_t>(value)});if (input("mutation")==2) Put(Raw(Sid{60}),18,169,2); };
        hooks.project=[this](std::array<float,2> point) { Event("project",{std::bit_cast<std::uint32_t>(point[0]),std::bit_cast<std::uint32_t>(point[1])});return std::array<std::int32_t,2>{321,234}; };
        hooks.popup=[this](std::int32_t cost,std::array<std::int32_t,2> point,std::uint32_t flags) { Event("popup",{static_cast<std::uint32_t>(cost),static_cast<std::uint32_t>(point[0]),static_cast<std::uint32_t>(point[1]),flags}); };
        hooks.regular=[this](Sid parent,float delay) { Event("regular",{parent.value,std::bit_cast<std::uint32_t>(delay)}); };
        hooks.attached=[this](Sid parent,const std::function<void(Sid)>& visitor) { Event("attached",{parent.value});if (input("attached")) { visitor(Sid{80});visitor(Sid{81}); } };
        hooks.notify=[this](Sid process,std::uint32_t message,std::uint32_t first,std::uint32_t second) { Event("process",{process.value});Event("notify",{message,first,second}); };
        hooks.select=[this](std::uint32_t sid,std::uint32_t force,std::uint32_t active) { Event("select",{sid,force,active}); };
        hooks.send=[this](std::int32_t target,std::span<const std::uint8_t> bytes) { packet=Hex(bytes);Blob("send",static_cast<std::uint32_t>(target),bytes,true); };
        hooks.pickup=[this](Sid parent) { Event("pickup",{parent.value}); };
        hooks.baseMenu=[this](Sid parent,std::uint32_t,Sid) { Event("base-menu",{parent.value});return 7U; };
        hooks.assertion=[this](std::uint32_t line) { Event("assert",{line}); };
        hooks.dropPoint=[this](Sid parent,std::uint32_t kind) { Event("point",{parent.value,kind});return input("point") ? std::array<float,2>{std::bit_cast<float>(input("x")),std::bit_cast<float>(input("y"))} : std::array<float,2>{-10000,-10000}; };
        hooks.queue=[this](std::uint32_t command,std::uint32_t kind,std::uint32_t zero) { Event("queue",{command,kind,zero});if (input("mutation")==3) Raw(Sid{50})[34]=2;if (input("mutation")==4) Put(receivePacket,4,51,2); };
        hooks.badOwner=[this](std::uint32_t kind,std::uint32_t sender) { Event("bad-owner",{kind,sender}); };
        hooks.money=[this](std::uint32_t owner) { Event("money",{owner});return std::bit_cast<float>(input("money")); };
        hooks.refund=[this](std::uint32_t kind,std::uint32_t owner,std::array<float,2> point,std::uint32_t argument,std::uint32_t flags) { Event("refund",{kind,owner,std::bit_cast<std::uint32_t>(point[0]),std::bit_cast<std::uint32_t>(point[1]),argument,flags}); };
        hooks.movement=[this](std::uint32_t type) { Event("movement",{type});return 10U; };
        hooks.removeMovement=[this](Sid parent,std::uint32_t type,std::uint32_t first,std::uint32_t second) { Event("remove-path",{parent.value,type,first,second});if (input("mutation")==5) Raw(Sid{50})[34]=2; };
        hooks.detach=[this](Sid parent,std::uint32_t flags) { Event("detach",{parent.value,flags}); };
        hooks.unpop=[this](Sid parent,std::uint32_t flags) { Event("unpop",{parent.value,flags}); };
        hooks.create=[this](std::uint32_t kind,std::uint32_t flags) { Event("create",{kind,flags});Raw(Sid{70})[10]=static_cast<std::uint8_t>(kind);if (input("mutation")==6) Put(receivePacket,4,51,2);return Sid{70}; };
        hooks.owner=[this](Sid sid,std::uint32_t owner) { Event("owner",{sid.value,owner});Raw(sid)[34]=static_cast<std::uint8_t>(owner); };
        hooks.argument=[this](Sid sid,std::uint32_t argument) { Put(Raw(sid),12,argument,2); };
        hooks.pop=[this](Sid sid,std::array<float,2> point,std::uint32_t flags) { Event("pop",{sid.value,std::bit_cast<std::uint32_t>(point[0]),std::bit_cast<std::uint32_t>(point[1]),flags}); };
        hooks.activate=[this](std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> bytes) { CHECK(flags==0);Blob("local-send",owner,bytes,true); };
        hooks.broadcast=[this](std::span<const std::uint8_t> bytes) { Blob("broadcast",0,bytes,false); };
        hooks.place=[this](std::uint32_t kind,std::array<float,2> point,std::uint32_t owner,std::uint32_t argument,std::uint32_t first,std::uint32_t second,double amount,std::uint32_t flags) {
            const auto wide=std::bit_cast<std::uint64_t>(amount);Event("place",{kind,std::bit_cast<std::uint32_t>(point[0]),std::bit_cast<std::uint32_t>(point[1]),owner,argument,first,second,static_cast<std::uint32_t>(wide),static_cast<std::uint32_t>(wide>>32),flags});return input("create")!=0;
        };
        hooks.destroy=[this](Sid content,std::uint32_t flags) { Event("destroy",{content.value,flags,Raw(content)[22]}); };
        hooks.standing=[this](Sid parent) { Event("standing",{parent.value});return input("standing")!=0; };
        hooks.escape=[this](Sid parent,float delay,std::int32_t direction) { Event("escape",{parent.value,std::bit_cast<std::uint32_t>(delay),static_cast<std::uint32_t>(direction)}); };
        return hooks;
    }
    std::string Raws() { //원문과 같은7슬롯의 전체 바이트를 직렬화한다.
        std::string result;
        // 미사용 바이트 보존도 비교에 포함한다.
        for (const auto sid:{Sid{50},Sid{60},Sid{61},Sid{62},Sid{70},Sid{80},Sid{81}}) { if (!result.empty()) result+=',';result+=Hex(Raw(sid)); }return result;
    }
};
}
// 제품의 실제 finder/요청/메뉴/수신 결과를 두 정밀도의 고정 원문과 대조한다.
TEST_CASE(CarrierDrop_Native413Inputs) {
    DropScene scene;std::size_t count{},diagnostics{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/carrier-drop-x86.tsv").string();
    // 각 입력은 독립 관측의 기대값을 읽기만 한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==7);if (row.size()!=7) continue;scene.Prepare(row[0]);RawCarrierDrop drop(scene.pool,scene.types,scene.contained,scene.state,scene.Hooks());
        if (scene.input("op")==0) drop.Request(Sid{50},Sid{static_cast<std::uint16_t>(scene.input("chosen"))});
        else if (scene.input("op")==1) CHECK(drop.Command(Sid{50},0x0300011a,Sid{50})==1);
        else { std::array<std::uint8_t,9> packet{1,0x50,9,0,50,0};Put(packet,6,scene.input("chosen"),2);packet[8]=static_cast<std::uint8_t>(scene.input("flag"));scene.receivePacket=packet;drop.Receive(scene.input("sender"),packet); }
        const auto trace=scene.trace.empty() ? "-" : scene.trace,raws=scene.Raws();
        if ((trace!=row[1] || scene.packet!=row[2] || raws!=row[3]) && diagnostics++<5) std::cout<<"Drop 입력 "<<row[0]<<"\n실제 "<<trace<<' '<<scene.packet<<' '<<raws<<"\n원본 "<<row[1]<<' '<<row[2]<<' '<<row[3]<<'\n';
        CHECK(trace==row[1]);CHECK(scene.packet==row[2]);CHECK(raws==row[3]);CHECK(row[1]==row[4] && row[2]==row[5] && row[3]==row[6]);++count;
    }
    CHECK(count==413);
}
// 원본의 거부 관문에서는 도달하지 않은 경계를 요구하지 않으며 짧은 패킷은 사전에 거부한다.
TEST_CASE(CarrierDrop_LazyBoundariesAndPacketBounds) {
    DropScene scene;RawCarrierDrop drop(scene.pool,scene.types,scene.contained,scene.state,{});CHECK(&drop.Pool()==&scene.pool);
    std::array<std::uint8_t,8> packet{};bool rejected{};try { drop.Receive(1,packet); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    std::array<std::uint8_t,9> dead{};Put(dead,4,50,2);scene.Raw(Sid{50})[11]=2;drop.Receive(1,dead);
    scene.Raw(Sid{50})[11]=0;rejected=false;try { static_cast<void>(drop.Command(Sid{50},0x0300011a,Sid{50})); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}
