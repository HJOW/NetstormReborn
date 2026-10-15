// buried 전용 PostPop의 원본 전체 객체/현재 전역/개별 예약과 부분 쓰기를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBuriedPostPop.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본과 같은 입력만 소유하고 출력은 실제 제품 몸체가 기록한다.
struct BuriedScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 고정 free-list 구간을 실제 확보한 공유 풀이다.
    bool boss{};std::uint32_t loading{},mask{},chosen{},mutation{},allocations{}; // 현재 변경 가능한 외부 입력과 개별 할당 횟수다.
    std::string trace; // 선택된 실제 하위 호출의 현재 인자만 보존한다.
    // CSV의 일곱 입력을 원본50BYTE 슬롯에 준비하며 비교 결과를 계산하지 않는다.
    explicit BuriedScene(const std::vector<std::string>& input):boss(Number(input[0])!=0),loading(Number(input[1])),mask(Number(input[3])),chosen(Number(input[5])),mutation(Number(input[6])) {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==50) break;if (!sid.value || sid.value>50) throw std::logic_error("buried 검사 SID 준비 오류"); } // 관찰 대상50까지 실제 슬롯을 확보한다.
        auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x50a748);raw[10]=112;raw[11]=0;raw[34]=1;raw[40]=0;Put(raw,12,Number(input[4]),2);
    }
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 호출 순서와 인자 비트를 기록한다.
    // 외부 반환/변경 입력만 공급하고 제품 분기나 최종 QA를 대신 계산하지 않는다.
    BuriedPostPopHooks Hooks() {
        BuriedPostPopHooks hooks;
        hooks.chooseType=[this](std::uint32_t tier) { Event("Choose,"+std::to_string(tier));if (mutation==3) { boss=false;loading=1; }return chosen; };
        hooks.reserveRegular=[this] { Event("Reserve,40");const bool success=((mask>>allocations)&1U)!=0;++allocations;if (mutation==1 && allocations==1) Put(pool.AllocatedBytes(Sid{50}),12,0xbeef,2);return success; };
        hooks.addRegular=[this](Sid sid,std::uint32_t event,float delay) { Event("Regular,"+std::to_string(event)+','+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(delay)));if (mutation==2 && event==0) loading=1; };
        hooks.base=[this](Sid sid,std::uint32_t flags) { Event("Base,"+std::to_string(flags));if (mutation==4) Put(pool.AllocatedBytes(sid),12,0x7777,2); };return hooks;
    }
};
}
// 580개 원본 입력을 헤더를 제외하고 전수 재생한다. 실패한 첫 할당도 다음 할당을 막지 않아야 한다.
TEST_CASE(BuriedPostPop_Native580) {
    const auto fixture=LoadFixture(NETSTORM_BURIED_POSTPOP_FIXTURE);CHECK(fixture.rows.size()==581 && fixture.rows.front()[0]=="input");
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 각 원본 행은 독립된 현재 전역/풀에서 실행한다.
        const auto& row=fixture.rows[index];CHECK(row.size()==5);const auto input=Split(row[0],',');CHECK(input.size()==7);BuriedScene scene(input);RawBuriedPostPop body(scene.pool,scene.boss,scene.loading,scene.Hooks());body.PostPop(Sid{50},Number(input[2]));
        CHECK(Hex(scene.pool.Slot(Sid{50}))==row[1]);CHECK(scene.boss==(Number(row[2])!=0));CHECK(scene.loading==Number(row[3]));CHECK(scene.trace==row[4]);
    }
}
// 미선택 보상 추첨은 불필요하며 선택된 예약 실패는 앞선 QA 쓰기만 보존한다.
TEST_CASE(BuriedPostPop_RequiredBoundariesAndPartialWrites) {
    BuriedScene scene(Split("1,1,1,3,150,187,0",','));auto hooks=scene.Hooks();hooks.chooseType={};RawBuriedPostPop loading(scene.pool,scene.boss,scene.loading,hooks);loading.PostPop(Sid{50},1);CHECK(Get(scene.pool.Slot(Sid{50}),12,2)==150 && scene.allocations==2);
    scene.loading=0;CHECK(Throws([&] { loading.PostPop(Sid{50},1); }));CHECK(Get(scene.pool.Slot(Sid{50}),12,2)==150);
    hooks=scene.Hooks();hooks.reserveRegular={};RawBuriedPostPop missing(scene.pool,scene.boss,scene.loading,hooks);CHECK(Throws([&] { missing.PostPop(Sid{50},1); }));CHECK(Get(scene.pool.Slot(Sid{50}),12,2)==187);
    Put(scene.pool.AllocatedBytes(Sid{50}),0,0x501dd0);CHECK(!missing.Handles(Sid{50}) && Throws([&] { missing.PostPop(Sid{50},1); }));
}
