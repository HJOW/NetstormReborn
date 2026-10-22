// 원본600입력의 실제 효과 순서/두슬롯/임시속도 복원·난수 소비를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPanic.h"
#include "o/SpStore.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

TEST_CASE(PriestPanic_Native600) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/priest-panic-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 결과를 쓰지 않고 명시 경계의 입력·현재 자료 변화만 공급한다.
        const auto parts=Split(row[0],',');std::vector<std::uint32_t> c;for (const auto& value:parts) c.push_back(Number(value)); // 독립 원문의 아홉 입력이다.
        SidPool pool(OriginalEdition::Patch1078,24000);const auto write=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(raw.data()),50); }; // 준비/경계가 바꿀 실제 동일 슬롯만 제공한다.
        auto actor=write({50}),temple=write({60});std::fill(actor.begin(),actor.end(),std::uint8_t{});std::fill(temple.begin(),temple.end(),std::uint8_t{});Put(actor,10,158,1);Put(actor,26,c[1]);Put(actor,34,1,1);Put(temple,34,c[2] ? 1 : 2,1);Put(actor,14,std::bit_cast<std::uint32_t>(50.25F));Put(actor,18,std::bit_cast<std::uint32_t>(50.75F));std::vector<DudeNoticeTypeData> speeds(188);speeds[158].speed=std::bit_cast<float>(c[3]);speeds[159].speed=2.5F;GameRandom random(c[7]);std::uint32_t lands{},regions{},randoms{};std::string effects;
        const auto event=[&](std::string_view tag,auto... values) { if (!effects.empty()) effects+=';';effects+=tag;((effects+=':'+std::to_string(values)),...); }; // 원문 호출 순서의 전체DWORD 비트를 기록한다.
        PriestPanicHooks hooks;
        hooks.temple=[&](float x,float y) { event("T",std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y));if (c[8]==1) { Put(actor,14,std::bit_cast<std::uint32_t>(51.25F));Put(actor,26,20);Put(actor,34,2,1);Put(temple,34,2,1); }return Sid{static_cast<std::uint16_t>(c[0] ? 60 : 0)}; };
        hooks.maximum=[&](Sid sid) { event("H",sid.value);if (c[8]==2) { Put(actor,10,159,1);Put(actor,34,2,1);Put(temple,34,2,1); }return 100; };
        hooks.target=[&](Sid sid,Sid target)->std::uintptr_t { event("V",sid.value,target.value,0,0,std::bit_cast<std::uint32_t>(speeds[actor[10]].speed));if (c[8]==3) { Put(actor,10,159,1);Put(actor,18,std::bit_cast<std::uint32_t>(52.5F));speeds[158].speed=9.25F; }return c[4]; };
        hooks.land=[&](float x,float y) { event("L",std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y));++lands;if (lands==1 && c[8]==4) { Put(actor,14,std::bit_cast<std::uint32_t>(61.25F));Put(actor,18,std::bit_cast<std::uint32_t>(62.5F)); }return !(c[5]==0 || (c[5]==1 && (lands==2 || lands==3))); };
        hooks.region=[&](float x,float y) { event("G",std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y));++regions;if (c[8]==6 && regions==1) { Put(actor,14,std::bit_cast<std::uint32_t>(81.25F));Put(actor,18,std::bit_cast<std::uint32_t>(82.5F)); }return c[5]==3 && (regions==1 || regions==3 || regions==5) ? 2U : 1U; };
        hooks.random=[&](float low,float high) { event("R",std::bit_cast<std::uint32_t>(low),std::bit_cast<std::uint32_t>(high));++randoms;if (c[8]==5 && randoms==1) { Put(actor,14,std::bit_cast<std::uint32_t>(71.25F));Put(actor,18,std::bit_cast<std::uint32_t>(72.5F)); }return double(random.Next(65536))/65536.0*std::abs(double(high)-low)+low; };
        hooks.point=[&](Sid sid,float x,float y)->std::uintptr_t { event("P",sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),0,0,0,0,0);if (c[8]==7) Put(actor,34,3,1);return c[6]; };
        RawPriestPanic logic(pool,speeds,std::move(hooks));logic.Run({50});const auto speedBits=std::to_string(std::bit_cast<std::uint32_t>(speeds[158].speed))+','+std::to_string(std::bit_cast<std::uint32_t>(speeds[159].speed));
        if (effects!=row[1] || Hex(actor)+Hex(temple)!=row[2] || speedBits!=row[3] || std::to_string(random.State())!=row[4]) { std::cout<<"panic 입력"<<row[0]<<" 실제"<<effects<<'\n';CHECK(false); }++count;
    }
    CHECK(count==600);
}
