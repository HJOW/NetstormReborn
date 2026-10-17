// 원본 전달 본문과 주기 분기를 실제 일반 finder/현재 raw 상태로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierTransfer.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 새 독립 원문의 입력 DWORD를 부호 변환 없이 읽는다.
std::vector<std::uint32_t> TransferNumbers(std::string_view text) { std::vector<std::uint32_t> result;
    // 각 입력 칸의 비트를 보존한다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// 실제 일반 finder를 사용하며 큰 외부 효과만 원문과 같은 입력으로 공급한다.
struct CarrierTransferScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원문과 같은50바이트 물리 저장소다.
    SquidHash hash; // 원본 stub 후보와 같은 체인의 실제 네 단계 공간이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 발자국/genus다.
    CarrierTransferState state; // 모든 부모가 공유하는 원본 세 DWORD다.
    bool boss{}; // 원본 현재 권한이다.
    std::uint32_t editor{},owner{}; // 내용물 조회 뒤 편집 모드와 로컬 수신 owner다.
    std::vector<std::uint32_t> row; // 독립 원문의24DWORD 입력이다.
    std::string trace,packet; // 실제 외부 호출과 같은 명령 바이트다.
    // 할당 여부를 새 필터로 넣지 않는 원본 물리 raw 입력이다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
    // 원문과 같은 효과/현재 raw 변이만 공급한다.
    void Mutate(std::string_view where) {
        const auto m=row[16];if (m==1 && where=="find-parent") editor=1;
        if (m==2 && where=="find-parent") state.cooldown=1;
        if (m==3 && where=="find-donor") { Put(Raw(Sid{52}),18,155,2);state.nuggetType=155; }
        if (m==4 && where=="detach") { Raw(Sid{52})[11]=1;Raw(Sid{50})[11]=1; }
        if (m==5 && where=="attach") Raw(Sid{60})[10]=133;
        if (m==6 && where=="preview") Put(Raw(Sid{52}),18,155,2);
        if (m==7 && where=="broadcast") { Raw(Sid{60})[10]=133;Put(Raw(Sid{52}),18,155,2); }
        if (m==8 && where=="remove") Put(Raw(Sid{52}),18,155,2);
        if (m==9 && where=="choose") { boss=false;state.cooldown=9; }
        if (m==10 && where=="go") Put(Raw(Sid{52}),18,155,2);
    }
    // 호출 인자의 DWORD 순서/폭을 그대로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) { if (!trace.empty()) trace+=';';trace+=name;
        // 인자를 정렬하거나 WORD로 줄이지 않는다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 입력만 설치하며 예상 카운터/성공 여부/명령은 계산하지 않는다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);CHECK(row.size()==24);boss=row[1]!=0;editor=row[2];owner=row[21];state={row[5],row[4],77,row[22],row[23],{static_cast<std::uint8_t>(row[19]),static_cast<std::uint8_t>(row[18]),static_cast<std::uint16_t>(row[20])}};trace.clear();packet="-";hash.Reset();
        // 실제 단일 칸 발자국 입력이며 생성 결과는 공급하지 않는다.
        for (auto& type:types) { type.footX=1;type.footY=1;type.flags2=0; }types[154].flags2=row[14];types[155].flags2=row[14]^0x8000U;
        // 원문과 같은 네 슬롯의 입력과 미사용 바이트 패턴이다.
        for (const auto sid:{Sid{50},Sid{52},Sid{60},Sid{61}}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(sid.value==50 ? 187U : sid.value==52 ? 6U : sid.value==60 ? row[13] : 133U);raw[11]=static_cast<std::uint8_t>(sid.value==52 ? row[9] : sid.value==50 ? row[10] : 0U);raw[34]=static_cast<std::uint8_t>(sid.value==50 ? row[12] : sid.value==60 ? row[11] : sid.value==52 ? 0U : 2U);raw[40]=0;Put(raw,14,std::bit_cast<std::uint32_t>(20.75F));Put(raw,18,std::bit_cast<std::uint32_t>(21.5F)); }Put(Raw(Sid{52}),18,row[8],2);
        const auto chain=row[6]==0 ? std::vector<Sid>{Sid{50}} : row[6]==1 ? std::vector<Sid>{Sid{60},Sid{50}} : std::vector<Sid>{Sid{61},Sid{60},Sid{50}};
        // 원문 finder 경계의 물리 체인을 같은 실제 버킷에 설치한다.
        for (std::size_t i=0;i<chain.size();++i) Put(Raw(chain[i]),4,i+1<chain.size() ? chain[i+1].value : 0U,2);hash.Bucket(1,20.75F,21.5F)=chain.front().value;
    }
    // 반환을 대신 계산하지 않고 실제 큰 하위 효과만 관찰한다.
    CarrierTransferHooks Hooks() {
        CarrierTransferHooks hooks;
        hooks.find=[this](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { Event("F",{sid.value,mask,kind,index});Mutate(sid==Sid{50} ? "find-parent" : "find-donor");return Sid{static_cast<std::uint16_t>(sid==Sid{50} ? row[3] : sid==Sid{60} && row[7] ? 52U : 0U)}; };
        hooks.detach=[this](Sid sid,std::uint32_t flags) { Event("D",{sid.value,flags});Mutate("detach"); };
        hooks.attach=[this](Sid sid,Sid parent,std::uint32_t flags) { Event("A",{sid.value,parent.value,flags});Mutate("attach"); };
        hooks.preview=[this](Sid sid) { Event("P",{sid.value});Mutate("preview"); };
        hooks.broadcast=[this](std::span<const std::uint8_t> bytes) { packet=Hex(bytes);if (!trace.empty()) trace+=';';trace+="S:"+packet;Mutate("broadcast"); };
        hooks.movementType=[this](std::uint32_t type) { Event("M",{type,0,1});return 40U; };
        hooks.removeMovement=[this](Sid sid,std::uint32_t type) { Event("X",{sid.value,type,0,1});Mutate("remove"); };
        hooks.chooseGeyser=[this](Sid sid) { Event("G",{sid.value});Mutate("choose");return Sid{static_cast<std::uint16_t>(row[15])}; };
        hooks.goGeyser=[this](Sid sid,Sid target) { Event("V",{sid.value,target.value});Mutate("go"); };
        hooks.goHome=[this](Sid sid) { Event("H",{sid.value}); };
        hooks.commonHalt=[this](Sid sid) { Event("C",{sid.value});return row[17]; };return hooks;
    }
    // 실제 결과와 현재 전역을 원문과 같은 순서로 기록한다.
    std::string State(std::uint32_t result) const { std::string text;
        // 현재 필드를 폭 축소 없이 비교한다.
        for (const auto value:{result,state.haltCounter,state.cooldown,state.lastDonor,std::uint32_t(boss),editor,state.nuggetType}) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text;
    }
};
}
// 독립 원문의1110개 전체 입력을53비트 기대값과 비교하며64비트 원문도 보존한다.
TEST_CASE(CarrierTransfer_NativeBodies) {
    CarrierTransferScene scene;std::size_t count{},diagnostics{}; // 대조 행 수와 출력할 불일치 수다.
    // 성공/조기 종료/로컬 수신/주기 분기를 모두 제품 본문으로 재생한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_TRANSFER_FIXTURE).rows) {
        CHECK(fields.size()==9);if (fields.size()!=9) continue;scene.Prepare(TransferNumbers(fields[0]));RawCarrierTransfer transfer(scene.pool,scene.hash,scene.types,scene.boss,scene.editor,scene.owner,scene.state,scene.Hooks());std::uint32_t result{};
        if (scene.row[0]==0) result=transfer.Transfer(Sid{50});else if (scene.row[0]==1) result=transfer.Halt(Sid{50});else { std::array<std::uint8_t,10> packet{};packet[0]=static_cast<std::uint8_t>(scene.row[18]&0x7f);packet[1]=static_cast<std::uint8_t>(scene.row[19]);Put(packet,2,scene.row[20],2);Put(packet,4,50,2);Put(packet,6,52,2);Put(packet,8,60,2);transfer.Receive(scene.owner,0,packet); }
        const auto trace=scene.trace.empty() ? "-" : scene.trace,state=scene.State(result),raws=Hex(scene.Raw(Sid{50}))+","+Hex(scene.Raw(Sid{52}))+","+Hex(scene.Raw(Sid{60}))+","+Hex(scene.Raw(Sid{61}));
        if ((trace!=fields[1] || state!=fields[2] || raws!=fields[3] || scene.packet!=fields[4]) && diagnostics++<5) std::cout<<"전달 입력 "<<fields[0]<<"\n실제 "<<trace<<' '<<state<<' '<<raws<<' '<<scene.packet<<"\n원본 "<<fields[1]<<' '<<fields[2]<<' '<<fields[3]<<' '<<fields[4]<<'\n';
        CHECK(trace==fields[1]);CHECK(state==fields[2]);CHECK(raws==fields[3]);CHECK(scene.packet==fields[4]);++count;
    }
    CHECK(count==1110);
}
// 주기 카운터는 부모별로 분리하지 않으며 도달하지 않은 효과는 요구하지 않는다.
TEST_CASE(CarrierTransfer_SharedCounterAndLazyBoundaries) {
    CarrierTransferScene scene;scene.boss=false;scene.state.haltCounter=0;CarrierTransferHooks hooks;std::vector<Sid> calls;hooks.commonHalt=[&](Sid sid) { calls.push_back(sid);return 11U; };RawCarrierTransfer transfer(scene.pool,scene.hash,scene.types,scene.boss,scene.editor,scene.owner,scene.state,std::move(hooks));
    CHECK(transfer.Halt(Sid{50})==11);CHECK(transfer.Halt(Sid{60})==11);CHECK(transfer.Halt(Sid{50})==11);CHECK(transfer.Halt(Sid{60})==10);CHECK(scene.state.haltCounter==0);CHECK((calls==std::vector<Sid>{Sid{50},Sid{60},Sid{50}}));CHECK(&transfer.Pool()==&scene.pool);
    scene.state.haltCounter=0x7fffffff;CHECK(transfer.Halt(Sid{50})==10 && scene.state.haltCounter==0);RawCarrierTransfer missing(scene.pool,scene.hash,scene.types,scene.boss,scene.editor,scene.owner,scene.state,{});bool rejected{};try { static_cast<void>(missing.Halt(Sid{50})); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && scene.state.haltCounter==1);
    std::array<std::uint8_t,7> shortPacket{};rejected=false;try { transfer.Receive(999,0xffffffff,shortPacket); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
