// 실제00486440의 최초 원문 입력·모든 요청/난수/옵션·Player 변경을 대조한다.
#include "ConstructionSupport.h"
#include "o/RawWorldGeyser.h"

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace { // 기대 생성 결과 없이 동일한 외부 입력만 준비한다.
constexpr std::uint32_t kWorldGeyserCases=354; //독립 두 emulator의 중복 제거 입력 수다.
// 네 float를 원문16BYTE의 little endian 문자열로 보존한다.
std::string WorldGeyserRectangle(FortRandomRectangle rectangle) {
    std::array<std::uint8_t,16> bytes{};Put(bytes,0,Bits(rectangle.left));Put(bytes,4,Bits(rectangle.top));Put(bytes,8,Bits(rectangle.right));Put(bytes,12,Bits(rectangle.bottom));return Hex(bytes);
}
}
// 원본이 실제 전달한 요청량/허용인수/순서/시드와 현재 입력의 전체 바이트를 비교한다.
TEST_CASE(WorldGeyser_IndependentNative354) {
    std::ifstream input(NETSTORM_WORLD_GEYSER_FIXTURE);if (!input) throw std::runtime_error("세계 간헐천 부모 원문 fixture 없음");
    std::string line;std::uint32_t count{};
    while (std::getline(input,line)) { //각 원문 입력을 독립 Player/옵션/타입에 설치한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=16) throw std::runtime_error("세계 간헐천 부모 원문 열 수 오류");++count;
        bool battle=Number(row[0])!=0;std::array<std::uint8_t,87> options{};std::vector<std::uint8_t> players(1620);std::array<std::uint32_t,4> types{100,184,101,185};
        options[6]=static_cast<std::uint8_t>(Number(row[3]));options[17]=static_cast<std::uint8_t>(Number(row[4]));options[18]=static_cast<std::uint8_t>(Number(row[5]));options[9]=static_cast<std::uint8_t>(Number(row[6]));
        for (std::uint32_t owner=1;owner<=8;++owner) { //원문 예산의 실제 입력 위치만 작성한다.
            Put(players,owner*0xb4+0x2c,Number(row[1])&(1U<<(owner-1)) ? 1U : 0U);options[62+owner]=Number(row[2])&(1U<<(owner-1)) ? 128 : 0;
        }
        const auto alter=Number(row[9]),consume=Number(row[10]);std::uint32_t timeCalls{},decorCalls{},popCalls{},densityCalls{},demoCalls{},demo{};std::string trace;
        const auto event=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; //명시 외부 경계의 실제 순서만 기록한다.
        WorldGeyserHooks hooks;
        hooks.log=[&](auto text) { CHECK(text=="Make Local Geysers\n");event("Log"); };
        hooks.local=[&] { event("Local");if (alter==1) { battle=true;options[9]=1; }if (alter==7) types={101,184,100,185}; };
        hooks.configInt=[&](auto key,auto value) { CHECK(key=="predictableGeysers");CHECK(value==1);event("Config");if (alter==8) { options[18]=1;options[17]=3;options[6]=0; }return static_cast<std::int32_t>(Number(row[7])); };
        hooks.wallSeconds=[&] { event("Time");return std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(row[8])))+timeCalls++*0.125; };
        hooks.budget=[&] { event("Budget");return WorldGeyserBudget(OriginalEdition::Patch1078,battle,players,options); };
        hooks.density=[&] { event("Density");++densityCalls;if (alter==10) { constexpr std::array<std::uint8_t,5> values{1,1,2,0,3};options[17]=values[std::min(densityCalls-1,4U)]; }return WorldGeyserDensity(options[17]); };
        hooks.spacing=[&] { event("Spacing");return WorldGeyserSpacing(options[6]); };
        hooks.demo=[&] { event("Demo");++demoCalls;if (alter==5) demo=demoCalls==1 ? 1U : 0U;else if (alter==9) demo=1;return demo!=0 ? 1U : 0U; };
        hooks.whole=[&] { event("Whole");return FortRandomRectangle{16.25F,24.5F,240.75F,232.25F}; };
        hooks.partition=[&](std::int32_t cell,std::int32_t grid) { event("Part:"+std::to_string(cell)+":"+std::to_string(grid));return FortRandomRectangle{cell*10.25F,24.5F,cell*10.25F+32.5F,72.25F}; };
        hooks.decorate=[&](GameRandom& random,auto rectangle,std::int32_t region,std::uint32_t quantity,std::uint32_t type,std::uint32_t force) {
            event("Decor:"+std::to_string(random.State())+":"+WorldGeyserRectangle(rectangle)+":"+std::to_string(region)+","+std::to_string(quantity)+","+std::to_string(type)+","+std::to_string(force));++decorCalls;
            if (alter==2 && decorCalls==1) options[9]=0;if (alter==6 && decorCalls==1) types[3]=100;if (consume) random.SetState(random.State()+0x10203);
        };
        hooks.populate=[&](std::int32_t quantity,GameRandom& random,auto rectangle,GeyserPlacementOptions flags) {
            event("Pop:"+std::to_string(std::bit_cast<std::uint32_t>(quantity))+":"+std::to_string(random.State())+":"+WorldGeyserRectangle(rectangle)+":"+std::to_string(flags.excludePlayers)+","+std::to_string(flags.allowLand)+","+std::to_string(flags.allowVoid));++popCalls;
            if (alter==3 && popCalls==1) { options[18]=1;options[17]=3; }if (alter==4 && popCalls==1) { Put(players,2*0xb4+0x2c,0);options[63]=1; }if (consume) random.SetState(random.State()+0x10203);
        };
        RawWorldGeyser(OriginalEdition::Patch1078,{battle,options,types},std::move(hooks)).Generate();
        const std::string typeText=std::to_string(types[0])+","+std::to_string(types[1])+","+std::to_string(types[2])+","+std::to_string(types[3]);
        const std::array<std::string,5> actual{trace,Hex(options),Hex(players),typeText,std::to_string(demo)};
        for (std::size_t field=0;field<actual.size();++field) { //최초 불일치의 사례/필드를 드러내며 fixture를 수정하지 않는다.
            if (actual[field]!=row[11+field]) throw std::runtime_error("세계 간헐천 부모 불일치 사례"+std::to_string(count)+" 필드"+std::to_string(field)+" 실제="+actual[field]+" 원문="+row[11+field]);
        }
    }
    CHECK(count==kWorldGeyserCases);
}
// 미선택 시각/배치 하위를 요구하지 않으며 선택된 하위 누락은 선행 효과를 남긴다.
TEST_CASE(WorldGeyser_SelectedHooksPreserveEffects) {
    bool battle=false;std::array<std::uint8_t,87> options{};std::array<std::uint32_t,4> types{100,184,101,185};std::string trace;
    WorldGeyserHooks hooks;hooks.log=[&](auto) { trace+='L'; };bool failed=false;
    try { RawWorldGeyser(OriginalEdition::Patch1078,{battle,options,types},hooks).Generate(); } catch (const std::logic_error&) { failed=true; }CHECK(failed);CHECK(trace=="L");
    hooks.local=[&] { trace+='O'; };hooks.configInt=[](auto,auto) { return 1; };hooks.budget=[] { return 0; };hooks.density=[] { return 0; };hooks.spacing=[] { return 3; };
    trace.clear();RawWorldGeyser(OriginalEdition::Patch1078,{battle,options,types},hooks).Generate();CHECK(trace=="LO");
    battle=true;options[9]=1;trace.clear();failed=false;
    try { RawWorldGeyser(OriginalEdition::Patch1078,{battle,options,types},hooks).Generate(); } catch (const std::logic_error&) { failed=true; }CHECK(failed);CHECK(trace=="LO");
    failed=false;try { static_cast<void>(WorldGeyserDensity(4)); } catch (const std::out_of_range&) { failed=true; }CHECK(failed);
}
