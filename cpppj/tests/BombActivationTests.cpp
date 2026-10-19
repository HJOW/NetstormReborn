// 독립 원본434입력의 주문 활성화 비용/진단/할당/예약을 실제 제품과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBombActivation.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원본의 선언 경제/생성 입력만 공급하며 수신 관문은 제품이 실행한다.
struct ActivationScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; //원본50BYTE 물리 부모다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //현재 TYPE/genus다.
    std::uint32_t localOwner{}; //실제 수신이 참조하는 전체DWORD다.
    std::vector<std::uint32_t> input; //독립 원문의7개 정수 입력이다.
    std::string trace; //호출 순서와 DWORD 인자다.
    std::span<std::uint8_t> Raw() { const auto raw=pool.Slot(Sid{50});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } //검증의 명시 물리 입력을 설치한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) { //실제 호출 인자를 원문 표현으로 보존한다.
        if (!trace.empty()) trace+=';';trace+=name;
        // 각 인자 비트와 순서를 유지한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    void Prepare(std::string_view text) { //기대값을 계산하지 않고 raw/현재 전역을 초기화한다.
        input.clear();trace.clear();
        // 입력 일곱 칸을 원래 DWORD로 읽는다.
        for (const auto& value:Split(text,',')) input.push_back(Number(value));
        CHECK(input.size()==7);auto raw=Raw();std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=168;raw[11]=static_cast<std::uint8_t>(input[0]);raw[34]=static_cast<std::uint8_t>(input[1]);localOwner=input[2];types[168].flags2=input[4];types[169].flags2=0;
    }
    BombActivationHooks Hooks() { //원본에 선언한 큰 외부 효과의 입력만 동일하게 공급한다.
        BombActivationHooks hooks;hooks.cost=[this](std::uint32_t type) { CHECK(type==168);return static_cast<std::int32_t>(input[3]); };
        hooks.subtract=[this](std::uint32_t owner,float amount) { Event("subtract",{owner,std::bit_cast<std::uint32_t>(amount)});if (input[6]==1) Raw()[10]=169; };
        hooks.assertion=[this](std::uint32_t line) { Event("assert",{line});if (input[6]==2) Raw()[10]=169; };
        hooks.allocate=[this](std::uint32_t bytes) { Event("allocate",{bytes});return input[5]!=0; };
        hooks.construct=[this](std::uint32_t event,Sid parent,float delay) { Event("construct",{event,parent.value,std::bit_cast<std::uint32_t>(delay)}); };return hooks;
    }
};
}
// 실제 원본 기대와 전체 부모50바이트를 두 정밀도 모두 대조한다.
TEST_CASE(BombActivation_Native434Inputs) {
    ActivationScene scene;std::size_t count{},diagnostics{};const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-activation-x86.tsv").string();
    // 원본 입력을 재생하고 실제 효과만 관측한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==5);if (row.size()!=5) continue;scene.Prepare(row[0]);RawBombActivation activation(scene.pool,scene.types,scene.localOwner,scene.Hooks());std::array<std::uint8_t,6> packet{1,0x5c,6,0,50,0};activation.Receive(packet);
        const auto trace=scene.trace.empty() ? "-" : scene.trace,raw=Hex(scene.Raw());
        if ((trace!=row[1] || raw!=row[2]) && diagnostics++<4) std::cout<<"활성화 입력 "<<row[0]<<"\n실제 "<<trace<<' '<<raw<<"\n원본 "<<row[1]<<' '<<row[2]<<'\n';
        CHECK(trace==row[1]);CHECK(raw==row[2]);CHECK(row[1]==row[3] && row[2]==row[4]);++count;
    }
    CHECK(count==434);
}
// 짧은 패킷 거부와 dead의 미도달 효과/실제 선택 경계 누락을 확인한다.
TEST_CASE(BombActivation_PacketBoundsAndLazyBoundaries) {
    ActivationScene scene;RawBombActivation activation(scene.pool,scene.types,scene.localOwner,{});CHECK(&activation.Pool()==&scene.pool);
    std::array<std::uint8_t,5> shortPacket{};bool rejected{};try { activation.Receive(shortPacket); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    std::array<std::uint8_t,6> packet{1,0x5c,6,0,50,0};scene.Raw()[11]=2;activation.Receive(packet);scene.Raw()[11]=0;scene.Raw()[10]=168;scene.types[168].flags2=0x100;
    rejected=false;try { activation.Receive(packet); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}
