// 원본 간헐천 고유 삭제2016입력의 전체raw/목록 꼬리와 호출 순서를 재생한다.
#include "RawSceneSupport.h"
#include "o/RawGeyserLifetime.h"
using namespace netstorm::test::rawscene;
namespace {
// 원본 명시 DWORD 입력을 축소 없이 읽는다.
std::vector<std::uint32_t> LifeNumbers(std::string_view text) { std::vector<std::uint32_t> result;/* 원문 순서로 모든 DWORD를 읽는다. */ for (const auto& part:Split(text,',')) result.push_back(Number(part));return result; }
}
// 현재extra/편집모드/중복과0/효과뒤변경/직접 프레임 DWORD를 두 정밀도의 원문과 대조한다.
TEST_CASE(GeyserLifetime_NativeCurrentStateAndAllDuplicateRemoval) {
    SidPool pool(OriginalEdition::Patch1078,24000);std::size_t count{}; // 원문의 실제 전체 대조 수다.
    // 기존 목격 자료를 한 행씩 공급하며 예상 목록이나 분기를 계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_GEYSER_LIFETIME_FIXTURE).rows) {
        CHECK(row.size()==9);if (row.size()!=9) continue;const auto c=LifeNumbers(row[0]);CHECK(c.size()==7);if (c.size()!=7) continue;
        auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),0x10000);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});auto parent=raw.subspan(50*50,50);
        Put(parent,0,0x50da88);Put(parent,10,122,1);Put(parent,11,2,1);Put(parent,40,c[1],1);Put(parent,34,3,1);Put(parent,36,777);Put(parent,14,0x41a20000);Put(parent,18,0x41ac0000);
        const std::array<std::vector<std::uint32_t>,3> patterns{{{},{50,77,50,0,50,88},{77,88,0,99}}}; // 원래 목록의 입력이며 사용하지 않는 물리 꼬리는99다.
        SquidPostPopList list{std::vector<std::uint32_t>(8,99),static_cast<std::uint32_t>(patterns[c[4]].size())};std::copy(patterns[c[4]].begin(),patterns[c[4]].end(),list.entries.begin());std::uint32_t editor=c[2];std::string events;
        // 실제 발생 순서의 DWORD 인자를 같은 원문 문자열로 붙인다.
        const auto event=[&](char kind,std::initializer_list<std::uint32_t> args) { if (!events.empty()) events+=';';events+=kind;for (const auto value:args) events+=':'+std::to_string(value); };
        GeyserLifetimeHooks hooks;
        hooks.defaultFrame=[&](Sid sid) { event('F',{sid.value});if (c[5]==1) parent[40]=8;return c[3]; };
        hooks.update=[&](Sid sid) { event('U',{sid.value,Get(parent,36)});if (c[5]==2 || c[5]==3) parent[40]=c[5]==2 ? 8 : 0; };
        hooks.registerAi=[&](Sid sid,std::uint32_t state) { event('I',{sid.value,state});if (c[5]==4) { list.entries[0]=50;list.entries[1]=77;list.entries[2]=50;list.count=3;parent[40]=8; }if (c[5]==5) { editor=1;Put(parent,36,12345); } };
        hooks.damageablePre=[&](Sid sid,std::uint32_t flags) { event('P',{sid.value,flags}); };hooks.damageablePost=[&](Sid sid,std::uint32_t flags) { event('T',{sid.value,flags}); };
        hooks.dropIsland=[&](float x,float y,std::uint32_t flags,std::uint32_t owner) { event('D',{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags,owner});if (c[5]==6) { Put(parent,14,0x3fc00000);Put(parent,18,0x40200000);parent[34]=9;parent[40]=9; } };
        RawGeyserLifetime life(pool,list,editor,std::move(hooks));CHECK(life.Handles(Sid{50}) && &life.Pool()==&pool);if (c[0]) life.PostDestroy(Sid{50},c[6]);else life.PreDestroy(Sid{50},c[6]);
        std::string values; // 미사용 물리 꼬리까지8개를 모두 대조한다.
        for (const auto value:list.entries) { if (!values.empty()) values+=',';values+=std::to_string(value); }
        CHECK(events==row[1] && std::to_string(list.count)+','+std::to_string(editor)==row[2] && Hex(parent)==row[3] && values==row[4]);CHECK(std::equal(row.begin()+1,row.begin()+5,row.begin()+5));++count;
    }
    CHECK(count==2016);
}
// 현재 가상 표만 선택하고 실제 도달한 효과의 누락과 CD 혼합을 거부한다.
TEST_CASE(GeyserLifetime_EditionAndSelectedBoundary) {
    SidPool pool(OriginalEdition::Patch1078,24000);SquidPostPopList list;std::uint32_t editor{};RawGeyserLifetime life(pool,list,editor,{});CHECK(!life.Handles(Sid{0}));CHECK(Throws([&] { life.PreDestroy(Sid{50},0); }));
    CHECK(Throws([&] { SidPool cd(OriginalEdition::Cd1072,24000);RawGeyserLifetime bad(cd,list,editor,{}); }));
}
