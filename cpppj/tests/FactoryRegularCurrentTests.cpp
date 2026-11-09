// Factory의 독립 원본656입력과 현재 조회/0범위 예외 선행 난수를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFactoryRegular.h"
#include "o/SpStore.h"
#include <bit>
#include <iostream>
using namespace netstorm::test::rawscene; //독립 입력/바이트 서식만 공유한다.
namespace { //게임과 독립된 raw 입력·선택 장치만 검사에 둔다.
struct FactoryRegularScene { //사건/조회 순서와 분기는 제품 본문이 실행한다.
    SidPool pool{OriginalEdition::Patch1078,24000,true}; //원본 관측의 부모50을 포함한 물리 풀이다.
    GameRandom random{1}; //원문과 같은 실제 LCG를 사용한다.
    std::vector<std::uint32_t> row; //8DWORD 명시 입력이며 기대값은 계산하지 않는다.
    std::string trace; //실제로 선택한 외부 경계의 순서다.
    std::span<std::uint8_t> Raw() { const auto view=pool.Slot(Sid{50});return {const_cast<std::uint8_t*>(view.data()),view.size()}; } //fixture용 물리 슬롯만 직접 채운다.
    void Prepare(const std::string& input) { row.clear();for (const auto& field:Split(input,',')) row.push_back(Number(field));std::fill(Raw().begin(),Raw().end(),std::uint8_t{0xa5});Put(Raw(),0,0x11010000U);Raw()[10]=100;Raw()[11]=static_cast<std::uint8_t>((row[3]<<5)|4);random.SetState(row[6]);trace.clear(); } //같은 입력 바이트/seed를 재배치한다.
    void Event(const std::string& text) { if (!trace.empty()) trace+=';';trace+=text; } //원문이 선택한 효과만 서식화한다.
    FactoryRegularHooks Hooks() { //큰 업그레이드/표시·진행은 원본 관측과 같은 명시 경계다.
        FactoryRegularHooks hooks;
        hooks.level=[this](auto parent) { CHECK(parent.value==50);const auto level=Raw()[11]>>5;Event("L:"+std::to_string(level));return std::int32_t(level); };
        hooks.firstFrame=[this](auto parent,auto level) { CHECK(parent.value==50);const auto type=Raw()[10];Event("T:"+std::to_string(type));const auto first=std::bit_cast<std::int32_t>(row[4]+(type==101 ? 100U:0U)+std::uint32_t(level)*10U);if (row[7]) { Raw()[10]=101;Raw()[11]=static_cast<std::uint8_t>((((row[3]+1U)&7U)<<5)|4U); }return first; };
        hooks.frameCount=[this](auto parent,auto) { CHECK(parent.value==50);const auto type=Raw()[10];Event("T:"+std::to_string(type));return std::bit_cast<std::int32_t>(row[5]+(type==101 ? 1U:0U)); };
        hooks.random=[this](auto limit) { Event("R:"+std::to_string(limit));if (!limit) { static_cast<void>(random.Next(1));throw std::domain_error("원문 난수0범위"); }return random.Next(limit); };
        hooks.frame=[this](auto parent,auto value,auto flags) { CHECK(parent.value==50);Event("S:"+std::to_string(std::bit_cast<std::uint32_t>(value))+":"+std::to_string(flags)); };
        hooks.advance=[this](auto parent,auto value,auto flags) { CHECK(parent.value==50);Event("A:"+std::to_string(value)+":"+std::to_string(flags)); };
        hooks.begin=[this](auto parent,auto payload) { CHECK(parent.value==50);Event("U:"+std::to_string(std::bit_cast<std::uint32_t>(payload))); };hooks.finish=[this](auto parent) { CHECK(parent.value==50);Event("F");return -1.0F; };
        hooks.flare=[this](auto parent,auto number,auto code) { Event("P:"+std::to_string(parent.value)+":"+std::to_string(number)+":"+std::to_string(code)); };
        hooks.damageable=[this](auto parent,auto event,auto count,auto payload) { CHECK(parent.value==50);Event("B:"+std::to_string(event)+":"+std::to_string(count)+":"+std::to_string(std::bit_cast<std::uint32_t>(payload)));return 0.375F; };
        return hooks;
    }
};
}
TEST_CASE(FactoryRegular_Original656Inputs) { //현재 두 조회·실제 난수·예외와 최종 raw를 독립 원문 결과에 대조한다.
    FactoryRegularScene scene;std::size_t count{},faults{},diagnostics{};
    for (const auto& fields:LoadFixture(NETSTORM_FACTORY_REGULAR_CURRENT_FIXTURE).rows) { //원래 기대 반환/순서/seed/50BYTE는 수정하지 않는다.
        CHECK(fields.size()==5);if (fields.size()!=5) continue;scene.Prepare(fields[0]);std::string result;
        try { result=std::to_string(std::bit_cast<std::uint32_t>(RunFactoryRegular(scene.pool,Sid{50},scene.row[0],scene.row[1],std::bit_cast<float>(scene.row[2]),scene.Hooks()))); }catch (const std::domain_error&) { result="fault";++faults; }
        if ((scene.trace!=fields[2] || result!=fields[1]) && diagnostics++<5) std::cout<<fields[0]<<"\nactual="<<scene.trace<<"\noriginal="<<fields[2]<<'\n';
        CHECK(result==fields[1]);CHECK((scene.trace.empty() ? std::string("-"):scene.trace)==fields[2]);CHECK(scene.random.State()==Number(fields[3]));CHECK(Hex(scene.Raw())==fields[4]);++count;
    }CHECK(count==656 && faults==128);
}
TEST_CASE(FactoryRegular_LazySelectedEffectsAndEdition) { //조기 반환은 관련 없는 조회/기본/표시 장치를 요구하지 않는다.
    SidPool pool(OriginalEdition::Patch1078,24000,true);FactoryRegularHooks hooks;CHECK(RunFactoryRegular(pool,Sid{},4,1,9.0F,hooks)==0.0F);hooks.finish=[](auto) { return 0.125F; };CHECK(RunFactoryRegular(pool,Sid{},3,1,9.0F,hooks)==0.125F);
    bool failed{};try { RunFactoryRegular(pool,Sid{},3,0,9.0F,hooks); }catch (const std::logic_error&) { failed=true; }CHECK(failed);
    SidPool cd(OriginalEdition::Cd1072,15000,true);failed=false;try { RunFactoryRegular(cd,Sid{},4,1,9.0F,hooks); }catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
}
