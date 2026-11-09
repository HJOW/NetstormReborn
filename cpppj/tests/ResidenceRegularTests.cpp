// 거주 구역의 게으른 선택과 Fort 전용 생성 부모의 현재 raw 경계를 확인한다.
#include "RawSceneSupport.h"
#include "o/RawResidenceRegular.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::o; // 제품의 실제 풀/생성 부모만 호출한다.
TEST_CASE(ResidenceRegular_LazyDefaultAndEdition) { // 비선택 사건은 Pool 슬롯/난수/설정을 읽지 않는다.
    SidPool pool(OriginalEdition::Patch1078,24000,true);SquidHash hash;SquidPostPopList temples;ResidenceRegularHooks hooks;const auto payload=std::bit_cast<float>(0x7fc01234U);CHECK(std::bit_cast<std::uint32_t>(RunResidenceRegular(pool,hash,{},{},temples,Sid{},99,0,payload,hooks))==0x7fc01234U);
    bool failed{};try { RunResidenceRegular(pool,hash,{},{},temples,Sid{},2,0,payload,hooks); }catch (const std::logic_error&) { failed=true; }CHECK(failed);
    SidPool cd(OriginalEdition::Cd1072,15000,true);failed=false;try { RunResidenceRegular(cd,hash,{},{},temples,Sid{},99,0,payload,hooks); }catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
}
TEST_CASE(ResidenceDudeSpawn_CurrentFieldsAndConditionalTask) { // Fort/현재 두 목록의 count 관문과 진입점 뒤 owner/생성 뒤 현재 상태를 대조한다.
    SidPool pool(OriginalEdition::Patch1078,24000,true);const Sid source{50},born{60};const auto slot=[&pool](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>{const_cast<std::uint8_t*>(raw.data()),raw.size()}; }; // 명시 raw 입력만 현재 물리에 배치한다.
    SquidPostPopList population{std::vector<std::uint32_t>(30),0},residences{std::vector<std::uint32_t>(30),1};ResidenceDudeHooks hooks;bool fort{};std::uint32_t entrances{},spawns{},tasks{};
    hooks.entrance=[&](auto selected) { CHECK(selected==source);++entrances;slot(source)[34]=7;return std::array<float,2>{12,24}; };hooks.spawn=[&](auto x,auto y,auto owner) { CHECK(x==12 && y==24 && owner==7);++spawns;return born; };hooks.task=[&](auto selected,auto code,auto delay) { CHECK(selected==born && code==0x5102 && std::bit_cast<std::uint32_t>(delay)==0);++tasks; };
    for (const auto free:{0U,1U,2U}) { // source의 free만 차단하며 dead는 원문의 자연 통과다.
        for (const auto state:{0U,1U,2U,4U}) { // 생성된 Dude의 free/dead 두 비트가 작업 예약을 결정한다.
            entrances=spawns=tasks=0;slot(source)[11]=static_cast<std::uint8_t>(free);slot(born)[11]=static_cast<std::uint8_t>(state);fort=false;RunResidenceDudeSpawn(pool,fort,population,residences,source,hooks);CHECK(entrances==0 && spawns==0 && tasks==0);fort=true;RunResidenceDudeSpawn(pool,fort,population,residences,source,hooks);CHECK(entrances==(free==1 ? 0U:1U) && spawns==entrances && tasks==(free!=1 && !(state&3U) ? 1U:0U));
        }
    }
    population.count=1;RunResidenceDudeSpawn(pool,fort,population,residences,Sid{},{});population.count=2;RunResidenceDudeSpawn(pool,fort,population,residences,Sid{},{}); // 현재 Dude 수가 거주 구역 수 이상이면 source0 진단이나 Runtime을 선택하지 않는다.
    population.count=0;residences.count=0;RunResidenceDudeSpawn(pool,fort,population,residences,Sid{},{});residences.count=1;slot(source)[11]=0;hooks.spawn=[](auto,auto,auto) { return Sid{}; };tasks=0;RunResidenceDudeSpawn(pool,fort,population,residences,source,hooks);CHECK(tasks==0);
}
