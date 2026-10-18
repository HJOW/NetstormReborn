// 보호PE의 독립 회수 요청/54h 송수신 원문과 현재 효과 뒤 재독을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSalvageCommand.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원본 입력DWORD를 부호/float 변환 없이 읽는다.
std::vector<std::uint32_t> Numbers(std::string_view text) {
    std::vector<std::uint32_t> result;
    for (const auto& part:Split(text,',')) result.push_back(Number(part)); // 입력 열의 원래 비트를 보존한다.
    return result;
}
// 실제 계산을 대체하지 않고 큰 세계 효과에 공급한 명시 입력만 재생한다.
struct SalvageScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원본 격리 실행과 같은 실제 풀 용량이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 비용 진단 표다.
    std::uint32_t local{},altar{164},dais{165},nugget{154}; // 호출 중 바뀔 수 있는 원본DWORD다.
    std::int32_t deny{}; // 회수 금지의 전체 원본 비트다.
    bool checking{},mission{},boss{}; // 실제 세 관문의 현재 값이다.
    SalvageCommandState state{local,deny,checking,mission,boss,altar,dais,nugget}; // 모든 저장소를 복제하지 않고 빌린다.
    std::vector<std::uint32_t> input; // 독립 원문 입력21DWORD다.
    std::array<std::uint8_t,8> received{}; // 콜백에서 바뀔 수 있는 caller 소유 패킷이다.
    std::string trace,packet; // 실제 순서와8바이트 송신의 관측이다.
    std::uint32_t line{}; // 현재 원본 진단 행 번호다.
    // 할당기 상태를 합성하지 않는 검사 전용 물리 raw 입력이다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 실제 원본 인자의 폭/순서를 유지한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> args={}) {
        if (!trace.empty()) trace+=';';trace+=name;
        for (const auto value:args) trace+=':'+std::to_string(value); // 전체DWORD를 원문과 같은 열 순서로 기록한다.
    }
    // 격리 원본에도 공급한 외부 변경만 재생한다.
    void Mutate(std::string_view boundary,Sid sid={50}) {
        const auto mutation=input[17];
        if (boundary=="query") {
            if (mutation==1) Raw(Sid{51})[11]=3;
            if (mutation==2) deny=1;
            if (mutation==8) altar=128;
        }
        if (boundary=="assert" && line==0x295) {
            if (mutation==3) { Raw(Sid{50})[10]=6;Put(Raw(Sid{50}),18,154,2); }
            if (mutation==4) { local=2;deny=1; }
        }
        if (boundary=="assert" && line==0x563 && mutation==5) {
            boss=true;Put(received,4,51,2);Put(received,6,0xffff,2);Put(Raw(sid),0,0x12200100);Raw(sid)[34]=2;
        }
        if (boundary=="refund") {
            if (mutation==6) { Put(Raw(sid),0,0x12200100);Put(received,4,51,2);Put(received,6,0,2); }
            if (mutation==7) { Raw(sid)[34]=2;Raw(sid)[11]=1; }
        }
    }
    // 원본 하위 관측과 같은 경계이며 환불/삭제 결과를 제품에 주입하지 않는다.
    SalvageCommandHooks Hooks() {
        SalvageCommandHooks hooks;
        hooks.daisAt=[this](Sid sid) { Event("Q",{sid.value});Mutate("query",sid);return Sid{static_cast<std::uint16_t>(sid.value==50 && input[14] ? 51 : sid.value==51 && input[14]==2 ? 52 : 0)}; };
        hooks.tell=[this](std::string_view text) { CHECK(text=="DenySalvage");Event("T:DenySalvage"); };
        hooks.assertion=[this](std::uint32_t value) { line=value;Event("A",{value});Mutate("assert"); };
        hooks.send=[this](std::int32_t target,std::span<const std::uint8_t> raw) { Event("S",{static_cast<std::uint32_t>(target)});packet=Hex(raw); };
        hooks.refund=[this](Sid sid,std::uint32_t owner,bool salvage) { Event("R",{sid.value,owner,static_cast<std::uint32_t>(salvage)});Mutate("refund",sid); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event(Get(Raw(sid),0)==0x12200000 ? "D" : "d",{sid.value,flags}); };
        return hooks;
    }
    // 실제 입력을 초기화하며 원문 기대 열은 사용하지 않는다.
    void Prepare(std::vector<std::uint32_t> values) {
        input=std::move(values);CHECK(input.size()==21);local=input[6];deny=std::bit_cast<std::int32_t>(input[7]);checking=input[8]!=0;mission=input[9]!=0;boss=input[10]!=0;altar=164;dais=165;nugget=154;trace.clear();packet="-";line=0;
        state.request={static_cast<std::uint8_t>(input[19]),static_cast<std::uint8_t>(input[18]),static_cast<std::uint16_t>(input[20])};CHECK(input[13]==pool.Capacity());
        for (std::uint16_t number=1;number<100;++number) Initialize(Sid{number}); // 실제 범위 안의 앞 후보다.
        Initialize(Sid{23999}); // 실제 마지막 유효 후보를 별도로 준비한다.
        if (input[14]) { Raw(Sid{51})[10]=static_cast<std::uint8_t>(input[14]==2 ? 164 : 128);Raw(Sid{52})[10]=128; }
        types[164].cost=std::bit_cast<float>(input[15]);types[165].cost=std::bit_cast<float>(input[16]);
        received={static_cast<std::uint8_t>(input[18]&0x7f),static_cast<std::uint8_t>(input[19])};Put(received,2,input[20],2);Put(received,4,input[1],2);Put(received,6,input[11]>>16,2);
    }
    // 원본 물리50BYTE의 미사용 부분도 같은 명시A5 입력으로 유지한다.
    void Initialize(Sid sid) {
        auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(input[3]);raw[11]=static_cast<std::uint8_t>(input[2]);raw[34]=static_cast<std::uint8_t>(input[5]);Put(raw,18,input[4],2);
    }
    // 현재 전체 비트와 세 raw 슬롯을 원문 형식으로 반환한다.
    std::array<std::string,5> Observed() {
        const auto globals=std::to_string(local)+","+std::to_string(std::bit_cast<std::uint32_t>(deny))+","+std::to_string(checking)+","+std::to_string(mission)+","+std::to_string(boss)+","+std::to_string(altar);
        return {trace.empty() ? "-" : trace,packet,globals,Hex(Raw(Sid{50}))+","+Hex(Raw(Sid{51}))+","+Hex(Raw(Sid{52})),Hex(received)};
    }
};
}
// 두 x87 정밀도에서 실제로 반환한 요청/송신/수신3458입력을 전수 대조한다.
TEST_CASE(SalvageCommand_NativeRequestSendReceive) {
    SalvageScene scene;std::size_t count{},diagnostics{};std::array<std::size_t,3> operations{}; // 모든 연산의 관측 누락도 확인한다.
    for (const auto& row:LoadFixture(NETSTORM_SALVAGE_COMMAND_FIXTURE).rows) { // 원문은 읽기만 하고 제품 계산 결과로 갱신하지 않는다.
        CHECK(row.size()==11);if (row.size()!=11) continue;scene.Prepare(Numbers(row[0]));RawSalvageCommand body(scene.pool,scene.types,scene.state,scene.Hooks());
        if (scene.input[0]==0) body.Request(scene.input[1]);else if (scene.input[0]==1) body.Send(scene.input[1],scene.input[11]);else body.Receive(scene.input[12],scene.received);
        const auto observed=scene.Observed();
        for (std::size_t field=0;field<observed.size();++field) { // 전체 효과/패킷/현재 전역/raw/수신 버퍼를 두 독립 기대와 비교한다.
            if (observed[field]!=row[1+field] && diagnostics++<3) std::cout<<"회수 원문 차이 input"<<row[0]<<" field"<<field<<" 기대"<<row[1+field]<<" 실제"<<observed[field]<<"\n";
            CHECK(observed[field]==row[1+field] && observed[field]==row[6+field]);
        }
        ++operations[scene.input[0]];++count;
    }
    CHECK(count==3458 && operations[0]==1971 && operations[1]==400 && operations[2]==1087);
}
