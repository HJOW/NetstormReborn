// 썬 펜스 부모의 분기를 독립 원본 출력으로 확인하고 미연결 하위의 부분 쓰기를 검사한다.
#include "RawSceneSupport.h"
#include "o/RawFencePostPop.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 관찰과 같은 입력만 공급한다. 실제 부모의 쓰기와 분기는 제품에서 수행한다.
struct FenceScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 실제 확보한 동일50BYTE 슬롯 풀이다.
    std::uint32_t theme{},neighbors{},mask{},count{},mutation{},allocations{},lines{}; // 하위 반환값/개별 할당/상태 변경 입력이다.
    std::string trace; // 실제 선택된 효과의 인자와 순서다.
    // 부모50과 네 이웃51~54의 전체 입력을 원문과 같은 패턴으로 준비한다.
    explicit FenceScene(const std::vector<std::string>& input):theme(Number(input[1])),neighbors(Number(input[2])),mask(Number(input[3])),count(Number(input[4])),mutation(Number(input[5])) {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==54) break;if (!sid.value || sid.value>54) throw std::logic_error("펜스 검사 슬롯 확보 실패"); } // 관찰 대상54까지 실제 할당한다.
        for (std::uint16_t number=50;number<=54;++number) { // 각각의 현재 좌표와 owner/padding을 독립적으로 설정한다.
            auto raw=pool.AllocatedBytes(Sid{number});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x509470);raw[10]=136;raw[11]=0;raw[34]=1;raw[40]=0;
            Put(raw,14,std::bit_cast<std::uint32_t>(number+.25f));Put(raw,18,std::bit_cast<std::uint32_t>(number+.5f));
        }
    }
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 호출 사이 구분자를 보존한다.
    // 명시 외부 효과의 반환과 변경 입력만 제공하며 부모의 예약/쓰기는 계산하지 않는다.
    FencePostPopHooks Hooks() {
        FencePostPopHooks hooks;
        hooks.theme=[this](Sid sid) { CHECK(sid.value==50);Event("Theme,50");if (mutation==1) pool.AllocatedBytes(sid)[34]=7;return theme; };
        hooks.neighbor=[this](Sid sid,std::uint32_t direction) { CHECK(sid.value==50);Event("Neighbor,50,"+std::to_string(direction));if (mutation==2 && direction==0) pool.AllocatedBytes(sid)[34]=6;return Sid{static_cast<std::uint16_t>(neighbors&(1U<<(direction/2)) ? 51+direction/2 : 0)}; };
        hooks.reserveRegular=[this] { Event("Reserve,40");const bool success=((mask>>allocations)&1U)!=0;++allocations;if (mutation==3 && allocations==1) Put(pool.AllocatedBytes(Sid{51}),14,0x42f00000);return success; };
        hooks.addRegular=[this](Sid sid,std::uint32_t event,float delay) { Event("Regular,"+std::to_string(event)+','+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(delay))); };
        hooks.clearLine=[this](std::array<float,4> points) {
            std::string call="Clear"; // 네 현재 좌표의 float 비트만 기록한다.
            for (const auto point:points) call+=','+std::to_string(std::bit_cast<std::uint32_t>(point)); // 원문 인자 순서로 보존한다.
            Event(call);++lines;if (mutation==4 && lines==1) { Put(pool.AllocatedBytes(Sid{52}),18,0x42fa0000);pool.AllocatedBytes(Sid{50})[34]=5; }
        };
        hooks.connections=[this](Sid sid) { CHECK(sid.value==50);Event("Connections,50");if (mutation==5) Put(pool.AllocatedBytes(sid),46,77);return count; };
        hooks.damageable=[this](Sid sid,std::uint32_t flags) { CHECK(sid.value==50);Event("Base,"+std::to_string(flags));if (mutation==6) Put(pool.AllocatedBytes(sid),12,0x7777,2); };return hooks;
    }
    // 부모와 외부 효과가 바꾼 네 이웃 슬롯을 함께 대조한다.
    std::string Objects() const {
        std::string result; // 같은 다섯 현재 슬롯의 순서대로 결합한 결과다.
        for (std::uint16_t number=50;number<=54;++number) result+=Hex(pool.Slot(Sid{number})); // 보존 영역을 포함한 전체 바이트를 읽는다.
        return result;
    }
};
}
// 330개의 고유 입력은 두 원본 정밀도에서 같았으며 C++와 전체250BYTE/호출을 대조한다.
TEST_CASE(FencePostPop_Native330) {
    const auto fixture=LoadFixture(NETSTORM_FENCE_POSTPOP_FIXTURE);CHECK(fixture.rows.size()==331 && fixture.rows.front()[0]=="input");
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 매 입력을 독립 풀에서 재생해 앞선 원문의 결과를 재사용하지 않는다.
        const auto& row=fixture.rows[index];CHECK(row.size()==3);const auto input=Split(row[0],',');CHECK(input.size()==6);FenceScene scene(input);RawFencePostPop body(scene.pool,scene.Hooks());body.PostPop(Sid{50},Number(input[0]));
        CHECK(Hex(scene.pool.Slot(Sid{50})).substr(0,8)=="70945000");CHECK(scene.Objects()==row[1]);CHECK(scene.trace==row[2]);
    }
}
// 선택하지 않은 접두에는 하위 조회가 필요 없고 실패 시 앞선 owner 쓰기만 남아야 한다.
TEST_CASE(FencePostPop_RequiredBoundariesAndPartialWrites) {
    FenceScene scene(Split("1,0,0,63,4,0",','));auto hooks=scene.Hooks();hooks.theme={};RawFencePostPop missingTheme(scene.pool,hooks);
    missingTheme.PostPop(Sid{50},2);CHECK(scene.trace=="Base,2");CHECK(Throws([&] { missingTheme.PostPop(Sid{50},1); }));
    hooks=scene.Hooks();hooks.connections={};RawFencePostPop missingCount(scene.pool,hooks);CHECK(Throws([&] { missingCount.PostPop(Sid{50},1); }));
    CHECK(Get(scene.pool.Slot(Sid{50}),42)==1 && Get(scene.pool.Slot(Sid{50}),46)==0xa5a5a5a5 && scene.allocations==0);
    hooks=scene.Hooks();hooks.damageable={};RawFencePostPop missingBase(scene.pool,hooks);CHECK(Throws([&] { missingBase.PostPop(Sid{50},1); }));
    CHECK(Get(scene.pool.Slot(Sid{50}),46)==4 && scene.allocations==1);
    Put(scene.pool.AllocatedBytes(Sid{50}),0,0x509930);CHECK(!missingBase.Handles(Sid{50}) && Throws([&] { missingBase.PostPop(Sid{50},1); }));
}
