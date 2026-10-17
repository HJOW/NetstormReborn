// 원본 연결 정리/이웃 갱신/preDestroy의 전체250바이트와 모든 효과 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFenceLifetime.h"
#include <cstdio>
using namespace netstorm::test::rawscene;
namespace {
// 저장된 DWORD 입력을 순서대로 읽는다.
std::vector<std::uint32_t> Numbers(std::string_view text) { std::vector<std::uint32_t> values;for (const auto& item:Split(text,',')) values.push_back(Number(item));return values; }
// 실제 계산 대신 원문에 공급한 외부 이웃/효과 입력만 준비하는 장면이다.
struct FenceScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 실제 단일50바이트 raw 레이아웃이다.
    std::vector<std::uint32_t> input;std::string trace;bool mutated{};std::uint32_t reserved{}; // 명시 입력과 실제 경계 호출의 현재 상태다.
    // 실제 Pool에서 검사 슬롯을 확보한다.
    FenceScene() { for (std::uint16_t sid=5;sid<=54;++sid) CHECK(pool.Allocate(2)==Sid{sid}); }
    // dead 입력도 준비하는 검사 전용 접근이다. 제품의 할당 규칙은 바꾸지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 원문의 모든 인수 비트를 같은 순서로 기록한다.
    void Event(std::string_view kind,std::initializer_list<std::uint32_t> values={}) { if (!trace.empty()) trace+=';';trace+=kind;for (const auto value:values) trace+=':'+std::to_string(value); }
    // 최초 선택된 외부 효과만 다음 현재 상태를 바꾸는 명시 입력이다.
    void Mutate(std::string_view kind) {
        if (mutated) return;const auto mode=input[7];
        if (!((mode==1 && kind=="M") || (mode==2 && kind=="D") || (mode==3 && kind=="B") || (mode==4 && kind=="L") || (mode==5 && kind=="N"))) return;mutated=true;
        if (mode==1) { Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(125.5f));Raw(Sid{50})[34]=3; }
        if (mode==2) Raw(Sid{52})[11]=2;if (mode==3) Raw(Sid{50})[10]=128;if (mode==4) Raw(Sid{50})[11]=2;if (mode==5) Raw(Sid{53})[11]=2;
    }
    // 독립 입력만 새로 준비하며 이전 결과를 다음 행으로 넘기지 않는다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        CHECK(values.size()==10);input=values;trace.clear();mutated=false;reserved=0;
        // parent/네 이웃 raw의 무관한 바이트도 표식으로 보존한다.
        for (std::uint16_t sid=50;sid<=54;++sid) {
            auto bytes=Raw(Sid{sid});std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa5});Put(bytes,0,1);bytes[10]=sid==50 ? 136 : static_cast<std::uint8_t>(values[4]);bytes[11]=static_cast<std::uint8_t>(values[sid==50 ? 1 : 3]);bytes[34]=sid==50 ? static_cast<std::uint8_t>(values[8]) : 2;bytes[40]=sid==50 ? static_cast<std::uint8_t>(values[2]) : 0;Put(bytes,14,std::bit_cast<std::uint32_t>(static_cast<float>(100+sid)));Put(bytes,18,std::bit_cast<std::uint32_t>(static_cast<float>(120+sid)));
        }
    }
    // 실제 몸체의 계산/순서 대신 외부 입력과 가상 효과만 제공한다.
    FenceLifetimeHooks Hooks() {
        FenceLifetimeHooks hooks;
        hooks.neighbor=[this](Sid sid,std::uint32_t direction,bool allied) { const auto index=direction/2;std::uint32_t result=sid.value==50 ? 51+(allied ? index : 3-index) : direction==0 ? 50 : 0;if (input[5]&(1U<<index)) result=0;Event("S",{sid.value,direction,allied ? 1U : 0U,result});return Sid{static_cast<std::uint16_t>(result)}; };
        hooks.clearMarkers=[this](Sid sid,Sid other) { Event("M",{sid.value,other.value});Mutate("M"); };
        hooks.clearLine=[this](std::array<float,4> points,std::uint32_t first,std::uint32_t second,Sid a,Sid b) { Event("L",{std::bit_cast<std::uint32_t>(points[0]),std::bit_cast<std::uint32_t>(points[1]),std::bit_cast<std::uint32_t>(points[2]),std::bit_cast<std::uint32_t>(points[3]),0,first,second,a.value,b.value});Mutate("L"); };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("D",{sid.value,flags});Raw(sid)[11]|=4;Mutate("D"); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { Event("U",{sid.value,flags});Raw(sid)[11]&=~4;Mutate("U"); };
        hooks.rebuild=[this](Sid sid) { Event("B",{sid.value});Mutate("B"); };
        hooks.reserveRegular=[this] { Event("N",{0x28});const auto success=(input[6]&(1U<<reserved++))!=0;Mutate("N");return success; };
        hooks.addRegular=[this](Sid parent,std::uint32_t event,float value) { Event("A",{event,parent.value,std::bit_cast<std::uint32_t>(value)}); };
        hooks.damageable=[this](Sid sid,std::uint32_t flags) { Event("P",{sid.value,flags}); };return hooks;
    }
    // 전체 다섯 슬롯을 원문 순서 그대로 반환한다.
    std::string Snapshot() { std::string text;for (std::uint16_t sid=50;sid<=54;++sid) text+=Hex(pool.Slot(Sid{sid}));return text; }
};
}
// 두 실제 정밀도에서 일치한207입력의 전체 raw·효과를 재계산 없이 비교한다.
TEST_CASE(FenceLifetime_NativeClearRefreshAndPre207) {
    FenceScene scene;std::size_t count{};
    // 원문 결과는 독립 TSV에서 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_FENCE_LIFETIME_FIXTURE).rows) {
        CHECK(row.size()==3);scene.Prepare(Numbers(row[0]));RawFenceLifetime logic(scene.pool,scene.Hooks());if (scene.input[0]==0) logic.Clear(Sid{50});else if (scene.input[0]==1) logic.Refresh(Sid{50});else logic.PreDestroy(Sid{50},scene.input[9]);
        const auto trace=scene.trace.empty() ? "-" : scene.trace;const bool same=trace==row[1] && scene.Snapshot()==row[2];CHECK(same);
        if (!same) { std::printf("펜스 수명 행%zu 입력%s\n기대%s\n실제%s\n",count,row[0].c_str(),row[1].c_str(),trace.c_str());break; }++count;
    }
    CHECK(count==207);
}
// 잘못된 판본/범위/선택한 누락 효과를 허용하지 않는다.
TEST_CASE(FenceLifetime_GuardsAndSamePool) {
    FenceScene scene;scene.Prepare(Numbers("0,0,0,0,136,0,3,0,1,64"));RawFenceLifetime logic(scene.pool,scene.Hooks());CHECK(&logic.Pool()==&scene.pool);
    SidPool cd(OriginalEdition::Cd1072,24000,false);CHECK(Throws([&] { RawFenceLifetime invalid(cd,{}); }));RawFenceLifetime missing(scene.pool,{});CHECK(Throws([&] { missing.Clear(Sid{50}); }));CHECK(Throws([&] { logic.Refresh(Sid{}); }));
}
