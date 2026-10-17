// 원본 포획 본문과 별도 수신를 실제 일반 finder/현재 raw 상태로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierSteal.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 새 독립 원문의 입력 DWORD를 부호 변환 없이 읽는다.
std::vector<std::uint32_t> StealNumbers(std::string_view text) { std::vector<std::uint32_t> result;
    // 각 입력 칸의 비트를 보존한다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// 실제 일반 finder를 사용하며 큰 외부 효과만 원문과 같은 입력으로 공급한다.
struct CarrierStealScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원문과 같은50바이트 물리 저장소다.
    SquidHash hash; // 원본 stub 후보와 같은 체인의 실제 네 단계 공간이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 발자국/genus다.
    std::array<std::uint32_t,81> alliances{}; // 역방향과 정방향을 독립 입력한다.
    std::uint32_t haltCounter{}; // 실제 크랩 전체가 공유하는00545428이다.
    std::uint32_t carrierType{156},priestType{158}; // 현재 타입 전역이다.
    FormPacketHeader header{0x60,1,10}; // 현재 원본 헤더다.
    std::span<const std::uint8_t> currentPacket; // 효과 중 caller가 가진 같은packet을 변경할 수 있다.
    bool boss{}; // 원본 현재 권한이다.
    std::uint32_t editor{},owner{}; // 내용물 조회 뒤 편집 모드와 로컬 수신 owner다.
    std::vector<std::uint32_t> row; // 독립 원문의30DWORD 입력이다.
    std::string trace,packet; // 실제 외부 호출과 같은 명령 바이트다.
    // 할당 여부를 새 필터로 넣지 않는 원본 물리 raw 입력이다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
    // 원문과 같은 효과/현재 raw 변이만 공급한다.
    void Mutate(std::string_view where) {
        const auto m=row[17];if (m==1 && where=="find-parent") editor=1;
        if (m==2 && where=="find-donor") { Put(Raw(Sid{52}),18,158,2);Raw(Sid{52})[12]=1; }
        if (m==3 && where=="find-donor") priestType=154;
        if (m==4 && where=="detach") { Raw(Sid{52})[11]=1;Raw(Sid{50})[11]=1; }
        if (m==5 && where=="attach") Raw(Sid{60})[10]=71;
        if (m==6 && where=="preview") Put(Raw(Sid{52}),18,155,2);
        if (m==7 && where=="broadcast") { Raw(Sid{60})[10]=71;Put(Raw(Sid{52}),18,155,2); }
        if (m==8 && where=="remove") Put(Raw(Sid{52}),18,155,2);
        if (m==9 && where=="sound") { Put(Raw(Sid{52}),18,155,2);boss=false; }
        if (m==10 && where=="detach") Put({const_cast<std::uint8_t*>(currentPacket.data()),currentPacket.size()},4,61,2);
        if (m==11 && where=="attach") Put({const_cast<std::uint8_t*>(currentPacket.data()),currentPacket.size()},4,60,2);
        if (m==12 && where=="allocation") { boss=false;Raw(Sid{50})[40]=9; }
        if (m==13 && where=="check") { boss=true;Raw(Sid{50})[40]=4; }
    }
    // 호출 인자의 DWORD 순서/폭을 그대로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) { if (!trace.empty()) trace+=';';trace+=name;
        // 인자를 정렬하거나 WORD로 줄이지 않는다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 입력만 설치하며 예상 성공 여부/명령은 계산하지 않는다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);CHECK(row.size()==30);boss=row[1]!=0;editor=row[2];owner=row[23];haltCounter=row[24];carrierType=row[21];priestType=row[22];header={static_cast<std::uint8_t>(row[19]),static_cast<std::uint8_t>(row[18]),static_cast<std::uint16_t>(row[20])};trace.clear();packet="-";hash.Reset();alliances.fill(0);alliances[row[6]*9+row[7]]=row[8];alliances[row[7]*9+row[6]]=row[9];
        // 실제 단일 칸 발자국/genus 입력이다. 예상 후보/분기 결과는 공급하지 않는다.
        for (auto& type:types) { type.footX=1;type.footY=1;type.flags2=0; }types[row[10]].flags2=row[5];types[154].flags2=row[16];types[158].flags2=row[16];types[155].flags2=row[16]^0x8000U;
        // 원문과 같은 네 슬롯의 입력과 미사용 바이트 패턴이다.
        for (const auto sid:{Sid{50},Sid{52},Sid{60},Sid{61}}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(sid.value==50 ? 156U : sid.value==52 ? 6U : sid.value==60 ? row[10] : 71U);raw[11]=static_cast<std::uint8_t>(sid.value==52 ? row[15] : sid.value==50 ? row[14] : 0U);raw[34]=static_cast<std::uint8_t>(sid.value==50 ? row[6] : sid.value==60 ? row[7] : sid.value==52 ? 0U : 2U);raw[40]=static_cast<std::uint8_t>(sid.value==50 ? row[26] : 0U);Put(raw,14,std::bit_cast<std::uint32_t>(20.75F));Put(raw,18,std::bit_cast<std::uint32_t>(21.5F)); }Put(Raw(Sid{52}),18,row[12],2);Raw(Sid{52})[12]=static_cast<std::uint8_t>(row[13]);
        const auto chain=row[4]==0 ? std::vector<Sid>{Sid{50}} : row[4]==1 ? std::vector<Sid>{Sid{60},Sid{50}} : std::vector<Sid>{Sid{61},Sid{60},Sid{50}};
        // 원문 finder 경계의 물리 체인을 실제 같은 버킷에 설치한다.
        for (std::size_t i=0;i<chain.size();++i) Put(Raw(chain[i]),4,i+1<chain.size() ? chain[i+1].value : 0U,2);hash.Bucket(1,20.75F,21.5F)=chain.front().value;
    }
    // 반환을 대신 계산하지 않고 실제 큰 하위 효과만 관찰한다.
    CarrierStealHooks Hooks() {
        CarrierStealHooks hooks;
        hooks.commonHalt=[this](Sid sid) { Event("B",{sid.value});return row[29]; };
        hooks.allocateRegular=[this] { Event("N",{0x28});Mutate("allocation");return row[27]!=0; };
        hooks.regular=[this](Sid sid,std::uint32_t event,float payload) { Event("R",{sid.value,event,std::bit_cast<std::uint32_t>(payload)}); };
        hooks.check=[this](Sid sid) { Event("K",{sid.value});Mutate("check"); };
        hooks.damageablePostPop=[this](Sid sid,std::uint32_t flags) { Event("Z",{sid.value,flags}); };
        hooks.find=[this](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { Event("F",{sid.value,mask,kind,index});Mutate(sid==Sid{50} ? "find-parent" : "find-donor");return Sid{static_cast<std::uint16_t>(sid==Sid{50} ? row[3] : sid==Sid{60} && row[11] ? 52U : 0U)}; };
        hooks.detach=[this](Sid sid,std::uint32_t flags) { Event("D",{sid.value,flags});Mutate("detach"); };
        hooks.attach=[this](Sid sid,Sid parent,std::uint32_t flags) { Event("A",{sid.value,parent.value,flags});Mutate("attach"); };
        hooks.preview=[this](Sid sid) { Event("P",{sid.value});Mutate("preview"); };
        hooks.broadcast=[this](std::span<const std::uint8_t> bytes) { packet=Hex(bytes);if (!trace.empty()) trace+=';';trace+="S:"+packet;Mutate("broadcast"); };
        hooks.movementType=[this](std::uint32_t type) { Event("M",{type,0,1});return 40U; };
        hooks.removeMovement=[this](Sid sid,std::uint32_t type) { Event("X",{sid.value,type,0,1});Mutate("remove"); };
        hooks.goHome=[this](Sid sid) { Event("H",{sid.value}); };
        // 전역음향의 다섯 인자를 낮은 폭으로 줄이지 않고 기록한다.
        hooks.sound=[this](std::string_view name,std::uint32_t loop,std::uint32_t volume,std::uint32_t pan,std::uint32_t priority,std::uint32_t limit) { if (!trace.empty()) trace+=';';trace+="W:"+std::string(name);for (const auto value:{loop,volume,pan,priority,limit}) trace+=':'+std::to_string(value);Mutate("sound"); };return hooks;
    }
    // 실제 결과와 현재 전역을 원문과 같은 순서로 기록한다.
    std::string State(std::uint32_t result) const { std::string text;
        // 현재 필드를 폭 축소 없이 비교한다.
        for (const auto value:{result,std::uint32_t(boss),editor,priestType,haltCounter}) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text;
    }
};
}
// 독립 원문의1105개 전체 입력을53비트 기대값과 비교하며64비트 원문도 보존한다.
TEST_CASE(CarrierSteal_NativeBodies) {
    CarrierStealScene scene;std::size_t count{},diagnostics{}; // 대조한 행 수와 최초 불일치 기록 수다.
    // 실제 검색/수신 결과를 두정밀도 원문과 별도로 비교한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_STEAL_FIXTURE).rows) {
        CHECK(fields.size()==9);if (fields.size()!=9) continue;scene.Prepare(StealNumbers(fields[0]));auto hooks=scene.Hooks();
        RawCarrierSteal steal(scene.pool,scene.hash,scene.types,scene.boss,scene.editor,scene.owner,scene.alliances,scene.carrierType,scene.priestType,scene.header,scene.haltCounter,std::move(hooks));std::uint32_t result{};
        if (scene.row[0]==0) {
            result=steal.Try(Sid{50});
        } else if (scene.row[0]==2) result=steal.Halt(Sid{50});else if (scene.row[0]==3) steal.PostPop(Sid{50},scene.row[25]);else { std::array<std::uint8_t,10> packet{};packet[0]=static_cast<std::uint8_t>(scene.row[18]&0x7f);packet[1]=static_cast<std::uint8_t>(scene.row[19]);Put(packet,2,scene.row[20],2);Put(packet,4,50,2);Put(packet,6,52,2);Put(packet,8,60,2);scene.currentPacket=packet;steal.Receive(scene.owner,0,packet); }
        const auto trace=scene.trace.empty() ? "-" : scene.trace,state=scene.State(result),raws=Hex(scene.Raw(Sid{50}))+","+Hex(scene.Raw(Sid{52}))+","+Hex(scene.Raw(Sid{60}))+","+Hex(scene.Raw(Sid{61}));
        if ((trace!=fields[1] || state!=fields[2] || raws!=fields[3] || scene.packet!=fields[4]) && diagnostics++<5) std::cout<<"포획 입력 "<<fields[0]<<"\n실제 "<<trace<<' '<<state<<' '<<raws<<' '<<scene.packet<<"\n원본 "<<fields[1]<<' '<<fields[2]<<' '<<fields[3]<<' '<<fields[4]<<'\n';
        CHECK(trace==fields[1]);CHECK(state==fields[2]);CHECK(raws==fields[3]);CHECK(scene.packet==fields[4]);CHECK(fields[1]==fields[5] && fields[2]==fields[6] && fields[3]==fields[7] && fields[4]==fields[8]);++count;
    }
    CHECK(count==1105);
}
// 비권한 조기종료와 실제로 도달한 누락 경계를 구분한다.
TEST_CASE(CarrierSteal_LazyBoundariesAndPacketBounds) {
    CarrierStealScene scene;RawCarrierSteal steal(scene.pool,scene.hash,scene.types,scene.boss,scene.editor,scene.owner,scene.alliances,scene.carrierType,scene.priestType,scene.header,scene.haltCounter,{});CHECK(steal.Try(Sid{50})==10);CHECK(&steal.Pool()==&scene.pool);
    std::vector<Sid> parents;CarrierStealHooks hooks;hooks.commonHalt=[&](Sid parent) { parents.push_back(parent);return 11U; };RawCarrierSteal shared(scene.pool,scene.hash,scene.types,scene.boss,scene.editor,scene.owner,scene.alliances,scene.carrierType,scene.priestType,scene.header,scene.haltCounter,std::move(hooks));
    CHECK(shared.Halt(Sid{50})==11 && shared.Halt(Sid{60})==11 && shared.Halt(Sid{50})==11 && shared.Halt(Sid{60})==10);CHECK(scene.haltCounter==0 && (parents==std::vector<Sid>{Sid{50},Sid{60},Sid{50}}));
    Put(scene.Raw(Sid{50}),0,0x503f20);CHECK(shared.Handles(Sid{50}));Put(scene.Raw(Sid{50}),0,0x503ff0);CHECK(!shared.Handles(Sid{50}));
    scene.boss=true;bool rejected{};try { static_cast<void>(steal.Try(Sid{50})); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    std::array<std::uint8_t,7> packet{};rejected=false;try { steal.Receive(0,0,packet); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
