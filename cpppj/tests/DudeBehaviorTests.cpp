// 제품 전 관측한410입력의 부모 반환·효과 순서·population 전체를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDudeBehavior.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 현재 슬롯과 지도는 원본 입력 그대로이며 큰 효과는 같은 명시 경계에서 기록한다.
TEST_CASE(DudeBehavior_Native410) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);const auto slot=pool.Slot(Sid{50});std::span<std::uint8_t> raw(const_cast<std::uint8_t*>(slot.data()),slot.size());std::vector<std::uint8_t> spots(65536);std::size_t rows{};
    const std::array<std::array<std::uint32_t,5>,4> patterns{{{{50,51,50,52,99}},{{51,52,53,54,99}},{{50,50,50,50,99}},{{51,50,52,50,99}}}}; // 실제 원본에 준 목록의 마지막99도 stale 보존 대상이다.
    // 원본410입력과 두 정밀도의 동일 결과를 빠짐없이 검사한다.
    for (const auto& row:LoadFixture(NETSTORM_DUDE_BEHAVIOR_FIXTURE).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;const auto input=Split(row[0],',');CHECK(input.size()==6);if (input.size()!=6) return;
        std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x508cc8);raw[10]=70;raw[11]=static_cast<std::uint8_t>(Number(input[1]));Put(raw,12,Number(input[2]),2);Put(raw,14,std::bit_cast<std::uint32_t>(20.75F));Put(raw,18,std::bit_cast<std::uint32_t>(21.25F));raw[40]=static_cast<std::uint8_t>(Number(input[5])>>8);
        std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(Number(input[3])^6U));spots[21*256+20]=static_cast<std::uint8_t>(Number(input[3]));bool checking=Number(input[4])!=0;
        SquidPostPopList population;const auto& values=patterns[Number(input[5])&3U];population.entries.assign(values.begin(),values.end());population.count=4;std::string trace;
        const auto append=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 원본 효과 호출의 순서를 보존한다.
        DudeBehaviorHooks hooks;hooks.task=[&](Sid sid,std::uint32_t event,float delay) { append("task:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(delay))); };
        hooks.movementType=[&](std::uint32_t type) { append("type:"+std::to_string(type));return 24U; };
        hooks.find=[&](Sid sid,std::uint32_t type,std::uint32_t index,bool remove) { append("lookup:"+std::to_string(sid.value)+':'+std::to_string(type)+':'+std::to_string(index)+':'+std::to_string(remove));return Sid{}; };
        hooks.destroy=[&](Sid sid,std::uint32_t flags) { append("destroy:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.basePostDestroy=[&](Sid sid,std::uint32_t flags) { append("post:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.assertion=[&](std::uint32_t line) { append("assert:"+std::to_string(line)); };
        RawDudeBehavior body(pool,spots,{checking,population},std::move(hooks));const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end());std::string result;
        switch (Number(input[0])) {
        case 0:result=std::to_string(body.Halt(Sid{50}));break;
        case 1:result=std::to_string(body.CheckSpace(Sid{50}));break;
        case 2:result=std::to_string(body.Fall(Sid{50}));break;
        case 3:result=std::to_string(body.Arrival(Sid{50}));break;
        case 4:body.PostDestroy(Sid{50},0x12345678);result="-";break;
        default:CHECK(false);return;
        }
        std::string list;
        // count 밖의 물리 꼬리를 포함해 독립 원본의 전체5DWORD를 대조한다.
        for (const auto value:population.entries) { if (!list.empty()) list+=',';list+=std::to_string(value); }
        result+='|'+(trace.empty() ? std::string("-") : trace)+'|'+std::to_string(population.count)+'|'+list;
        CHECK(result==row[1] && row[1]==row[2] && std::equal(raw.begin(),raw.end(),before.begin()));++rows;
    }
    CHECK(rows==410);
}
