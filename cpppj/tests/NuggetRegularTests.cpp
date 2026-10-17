// 너겟의원본9019입력에서반환/프레임/낙하/난수/효과순서를재생한다.
#include "RawSceneSupport.h"
#include "o/RawNuggetRegular.h"
#include "o/SpStore.h"
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNativeVt=0x50dc30U,kChangedVt=0x20000800U; // 원문관측의현재VT와콜백이바꾸는명시VT다.
// 입력DWORD와기대값은분리해서읽는다.
std::vector<std::uint32_t> NuggetNumbers(std::string_view text) {
    std::vector<std::uint32_t> row;
    // 원문의상위비트도보존한다.
    for (const auto& value:Split(text,',')) row.push_back(Number(value));
    return row;
}
// 현재물리raw/점유지도/RNG와명시하위효과만공급한다.
struct NuggetRegularScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원문과같은50바이트물리객체다.
    std::vector<std::int32_t> counts=std::vector<std::int32_t>(188); // TYPE+170의독립입력이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 실제좌표색인이읽는고정입력지도다.
    std::vector<std::uint32_t> row; //15DWORD의원문입력이다.
    GameRandom random; // 원본4558f0과같은게임LCG상태다.
    bool boss{}; // 현재삭제권한이다.
    std::string trace; // 효과의현재순서다.
    // 원본이직접읽는물리슬롯을상태필터없이초기화한다.
    std::span<std::uint8_t> Raw() { const auto raw=pool.Slot(Sid{50});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 원문경계의전체DWORD인자를보존한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 인자순서와원문폭을보존한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 지도를입력으로채울뿐조회좌표의기대색인을계산하지않는다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);CHECK(row.size()==15);boss=row[9]!=0;random.SetState(row[12]);trace.clear();auto raw=Raw();std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});
        Put(raw,0,kNativeVt);raw[10]=154;raw[11]=static_cast<std::uint8_t>(row[10]);Put(raw,14,row[7]);Put(raw,18,row[8]);Put(raw,36,row[4]);counts[154]=std::bit_cast<std::int32_t>(row[5]);
        // 원문과같은고정패턴/xor또는균일BYTE입력이다.
        for (std::size_t i=0;i<spots.size();++i) spots[i]=static_cast<std::uint8_t>((row[14] ? ((i*17)^((i>>8)*93)) : 0U)^row[6]);
    }
    // 프레임/공간/삭제의큰경계만명시구현하고부모계산은제품몸체를사용한다.
    NuggetRegularHooks Hooks() {
        NuggetRegularHooks hooks;
        hooks.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) { Event("F",{sid.value,std::bit_cast<std::uint32_t>(frame),flags});Put(Raw(),36,std::bit_cast<std::uint32_t>(frame));if (row[13]==1) { Put(Raw(),0,kChangedVt);Raw()[10]=155; } };
        hooks.refresh=[this](Sid sid) { Event(Get(Raw(),0)==kChangedVt ? "X" : "R",{sid.value}); };
        hooks.allocate=[this] { Event("A",{40});return row[11]!=0; };
        hooks.randomInteger=[this](std::int32_t low,std::int32_t high) { Event("I",{std::bit_cast<std::uint32_t>(low),std::bit_cast<std::uint32_t>(high)});return low+static_cast<std::int32_t>(random.Next(static_cast<std::uint32_t>(high-low))); };
        hooks.construct=[this](Sid sid,std::uint32_t event,float payload) { Event("C",{event,sid.value,std::bit_cast<std::uint32_t>(payload)}); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event("D",{sid.value,flags});Raw()[11]|=2; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("U",{sid.value,flags});Raw()[11]|=4;if (row[13]==2) { Put(Raw(),0,kChangedVt);Put(Raw(),14,std::bit_cast<std::uint32_t>(42.5F));Put(Raw(),18,std::bit_cast<std::uint32_t>(43.5F)); } };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event(Get(Raw(),0)==kChangedVt ? "Y" : "P",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags});Put(Raw(),14,std::bit_cast<std::uint32_t>(x));Put(Raw(),18,std::bit_cast<std::uint32_t>(y));Raw()[11]&=static_cast<std::uint8_t>(~4U); };
        hooks.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { Event("B",{sid.value,event,count,std::bit_cast<std::uint32_t>(payload)});return std::bit_cast<float>(row[3]); };
        return hooks;
    }
};
}
// 기대값은 독립 PE의 원문에서만 읽고 제품 코드로 생성하지 않는다.
TEST_CASE(NuggetRegular_NativeAnimationFallGroundAndRandom) {
    NuggetRegularScene scene;std::size_t count{},diagnostics{}; // 독립 입력과 처음8개 불일치의 진단 수다.
    // 기본CRT의53비트 원문을 모두 재생하고64비트 관측은 별도로 보존한다.
    for (const auto& fields:LoadFixture(NETSTORM_NUGGET_REGULAR_FIXTURE).rows) {
        CHECK(fields.size()==9);if (fields.size()!=9) continue;scene.Prepare(NuggetNumbers(fields[0]));RawNuggetRegular logic(scene.pool,scene.counts,scene.spots,scene.boss,scene.Hooks());
        const auto result=scene.row[0]==0 ? std::bit_cast<std::uint32_t>(logic.Handle(Sid{50},scene.row[1],scene.row[2],std::bit_cast<float>(scene.row[3]))) : static_cast<std::uint32_t>(logic.CanFall(Sid{50}));
        if ((result!=Number(fields[1]) || (scene.trace.empty() ? "-" : scene.trace)!=fields[2] || Hex(scene.Raw())!=fields[3] || scene.random.State()!=Number(fields[4])) && diagnostics++<8) std::cout<<"너겟입력 "<<fields[0]<<"\n실제 "<<result<<' '<<scene.trace<<' '<<Hex(scene.Raw())<<' '<<scene.random.State()<<"\n원본 "<<fields[1]<<' '<<fields[2]<<' '<<fields[3]<<' '<<fields[4]<<'\n';
        CHECK(result==Number(fields[1]));CHECK((scene.trace.empty() ? "-" : scene.trace)==fields[2]);CHECK(Hex(scene.Raw())==fields[3]);CHECK(scene.random.State()==Number(fields[4]));++count;
    }
    CHECK(count==9019);
}
// 초기count와정상분배/명시늦은효과누락은서로구별한다.
TEST_CASE(NuggetRegular_LazyBoundariesAndExactVtable) {
    NuggetRegularScene scene;scene.Prepare({0,0,1,0x3e99999aU,1,4,0,0x41a40000U,0x41aa0000U,1,0,1,0,0,0});RawNuggetRegular logic(scene.pool,scene.counts,scene.spots,scene.boss,{});
    CHECK(logic.Handles(Sid{50}));CHECK(logic.Handle(Sid{50},2,0,0.3F)==0.3F);Put(scene.Raw(),0,0xdeadbeefU);CHECK(!logic.Handles(Sid{50}));Put(scene.Raw(),0,kNativeVt);scene.Raw()[10]=155;CHECK(!logic.Handles(Sid{50}));
    bool rejected{};try { static_cast<void>(logic.Handle(Sid{50},2,1,0.3F)); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}
