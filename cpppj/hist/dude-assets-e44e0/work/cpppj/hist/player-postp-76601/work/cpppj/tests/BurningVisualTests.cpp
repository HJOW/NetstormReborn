// 두 보호 PE의 Burning 생성자와 같은 공통표시 등록/150입자 초기화 상태를 대조한다.
#include "ConstructionSupport.h"
#include "o/RawBurningVisual.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 원본 입력/출력은 독립된 저장 fixture에서만 읽는다.
const auto& BurningRows() { static const auto rows=LoadFixture(NETSTORM_BURNINGPROCESS_FIXTURE).rows;return rows; }
// 원본포인터 대신 같은 실제표시목록에 넣는 명시호스트 토큰이다.
constexpr std::uint32_t kBurningToken=777;
// 현재 의미DWORD를 순서대로 보존하며 기대값을 계산하지 않는다.
std::string BurningJoin(const std::vector<std::uint32_t>& values,char separator) {
    std::string result;
    // 모든 현재값을 원본 관찰의 열 순서로 직렬화한다.
    for (const auto value:values) { if (!result.empty()) result+=separator;result+=std::to_string(value); }
    return result;
}
// 등록/메모리의 명시 입력경계를 원본과 같은 인자·순서로 기록한다.
std::vector<std::string> RunBurning(const std::vector<std::string>& row) {
    const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;SidPool pool(edition,kCapacity,true);
    const auto depth=edition==OriginalEdition::Patch1078 ? 35U : 33U;
    InputRaw(pool,{})[depth]=static_cast<std::uint8_t>(std::stoi(row[5]));InputRaw(pool,Sid{50})[depth]=static_cast<std::uint8_t>(std::stoi(row[4]));
    const auto initial=std::array<std::vector<std::uint32_t>,4>{{{}, {0}, {111,0,222}, {111,222}}}.at(Number(row[3]));std::vector<std::string> events;
    RenderRegistryHooks registryHooks;
    registryHooks.reserve=[&](std::uint32_t amount) { events.push_back("N:"+std::to_string(amount));return true; };
    registryHooks.releaseArray=[&] { events.push_back("L:registry"); };
    registryHooks.attach=[&](RenderProcessFields& fields,std::uint32_t type,Sid parent,std::uint32_t flags) { events.push_back("B:"+std::to_string(type)+':'+std::to_string(parent.value)+":0:"+std::to_string(flags));fields.form=83;fields.parent=parent.value; };
    RawRenderRegistry registry(pool,std::move(registryHooks),initial);BurningVisualState state;state.palette=1;BurningVisualHooks hooks;
    hooks.reserve=[&](std::uint32_t amount) { events.push_back("N:"+std::to_string(amount));return true; };
    RawBurningVisual visual(pool,registry,state,std::move(hooks));BurningVisualFields fields;visual.Initialize(fields,kBurningToken,Sid{static_cast<std::uint16_t>(Number(row[1]))},Number(row[2]));
    const auto& r=fields.registration;const auto last=std::bit_cast<std::uint64_t>(fields.lastTime),elapsed=std::bit_cast<std::uint64_t>(fields.elapsed);
    const std::vector<std::uint32_t> values{static_cast<std::uint32_t>(r.form),static_cast<std::uint32_t>(r.parent),Bits(r.left),Bits(r.top),Bits(r.right),Bits(r.bottom),static_cast<std::uint32_t>(r.slot),static_cast<std::uint32_t>(r.zOrder),fields.capacity,fields.total,
        static_cast<std::uint32_t>(last),static_cast<std::uint32_t>(last>>32),static_cast<std::uint32_t>(elapsed),static_cast<std::uint32_t>(elapsed>>32),fields.palette,fields.sound,fields.stage,fields.image.pixels.empty() ? 0U : 1U,
        static_cast<std::uint32_t>(fields.image.width),static_cast<std::uint32_t>(fields.image.height),static_cast<std::uint32_t>(fields.image.hotspotX),static_cast<std::uint32_t>(fields.image.hotspotY),fields.victim.value,fields.mode,fields.rows.empty() ? 0U : 1U,static_cast<std::uint32_t>(fields.selected),fields.off ? 1U : 0U};
    std::vector<std::uint8_t> particles(fields.particles.size()*24);std::uint32_t sum=0;
    // host padding은읽지않고 현재150개입자의 원본6필드만24바이트로만든다.
    for (std::size_t index=0;index<fields.particles.size();++index) { const auto& particle=fields.particles[index];auto bytes=std::span<std::uint8_t>(particles).subspan(index*24,24);Put(bytes,0,Bits(particle.x));Put(bytes,4,Bits(particle.y));Put(bytes,8,Bits(particle.velocityX));Put(bytes,12,Bits(particle.velocityY));Put(bytes,16,Bits(particle.life));Put(bytes,20,particle.source ? 1U : 0U); }
    // 현재전체 입자바이트의 누적값과 Adler를독립관찰과비교한다.
    for (const auto byte:particles) sum+=byte;
    std::string actions;
    // 메모리와부착의순서는별도원본기대칸을읽을때까지계산하지않는다.
    for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
    return {BurningJoin(values,','),BurningJoin(std::vector<std::uint32_t>(registry.Slots().begin(),registry.Slots().end()),','),actions,std::to_string(Adler(particles)),std::to_string(sum)};
}
// 한판본의전체 독립 생성자입력을같은현재 C++초기화몸체에재생한다.
void ReplayBurning(std::string_view edition) {
    std::size_t count=0;int failures=0;
    // 기대열은원본출력으로만사용하며초기행의metadata는입력칸에서만읽는다.
    for (const auto& row:BurningRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==12);const auto current=RunBurning(row);const auto expected=std::vector<std::string>(row.begin()+7,row.end());const bool same=current==expected;CHECK(same);
        if (!same) {
            // 최초원본/C++차이를양쪽값으로그대로보여준다.
            for (std::size_t index=0;index<current.size();++index) if (current[index]!=expected[index]) std::printf("%s BurningCtor 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),current[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(count==128 && failures==0);
}
}
// 10.78의 현재팔레트·모드0/1·표시목록과 실제150입자 zero를대조한다.
TEST_CASE(BurningVisual_PatchWarmConstructor) { ReplayBurning("originals"); }
// CD10.72의별도 SystemProcess/particle 초기화와같은모드/깊이/확장순서를대조한다.
TEST_CASE(BurningVisual_CdWarmConstructor) { ReplayBurning("originalCD"); }
// 첫팔레트가없는객체를이름없는빈색표로완료처리하지않는다. 새원본assert관찰로세지않는다.
TEST_CASE(BurningVisual_MissingColdPaletteRejected) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);RenderRegistryHooks registryHooks;registryHooks.reserve=[](std::uint32_t) { return true; };registryHooks.releaseArray=[] {};registryHooks.attach=[](RenderProcessFields&,std::uint32_t,Sid,std::uint32_t) { throw std::logic_error("부모0입력의미허용부착"); };RawRenderRegistry registry(pool,std::move(registryHooks));BurningVisualState state;BurningVisualHooks hooks;hooks.reserve=[](std::uint32_t) { return true; };RawBurningVisual visual(pool,registry,state,std::move(hooks));BurningVisualFields fields;CHECK(Throws([&] { visual.Initialize(fields,kBurningToken,{},0); }));CHECK(fields.capacity==150 && fields.particles.size()==150 && state.palette==0);
}
