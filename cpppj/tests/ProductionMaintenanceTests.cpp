// C++ 이전에 동결한 원본43ef10의 전체 내용물·시계·패널 필드와 효과 순서를 대조한다.
#include "ConstructionSupport.h"
#include "client/RawProductionMaintenance.h"
#include "o/CanonDecoder.h"
#include "o/RawSquidNeighbors.h"
using namespace netstorm::test::construction;
using namespace netstorm::client;
namespace {
// 실제 원문 double의8바이트를 기존16진수 직렬화기에 공급한다.
std::string DoubleHex(double value) { return Hex(std::bit_cast<std::array<std::uint8_t,8>>(value)); }
}
// 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 순환/0번/해제와 잘못된 설정의 원자성을 확인한다.
TEST_CASE(ProductionMaintenance_TestBridgeSequence) {
    ProductionMaintenanceGlobals globals;CHECK(!globals.NextTestBridgePattern());globals.ConfigureTestBridgePatterns("3, 0,25");
    CHECK(globals.NextTestBridgePattern()==3U && globals.NextTestBridgePattern()==0U && globals.NextTestBridgePattern()==25U && globals.NextTestBridgePattern()==3U && globals.testBridgeDraws==4);
    // 잘못된 항목 때문에 진행 중인 순서와 소비 횟수가 초기화되지 않아야 한다.
    for (const auto invalid:{"26","-1","1,","x","1,,2"}) { bool rejected{};try { globals.ConfigureTestBridgePatterns(invalid); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected && globals.testBridgeDraws==4 && globals.testBridgePatterns.size()==3); }
    globals.ConfigureTestBridgePatterns("off");CHECK(globals.testBridgePatterns.empty() && !globals.testBridgeDraws && !globals.NextTestBridgePattern());
}
// 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 최종 덮어쓰기/0번/빈자리 관문/원래 난수 소비를 검사한다.
TEST_CASE(ProductionMaintenance_TestBridgeOverrideAfterOriginalRules) {
    SidPool pool(OriginalEdition::Patch1078,65535);CHECK(pool.Allocate(2)==Sid{5});auto raw=pool.AllocatedBytes({5});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=152;raw[34]=1;
    ContainedFinderState finder;ProductionMaintenanceGlobals globals;globals.forcedPattern=12;globals.ConfigureTestBridgePatterns("3,0");ProductionMaintenancePanel panel;panel.slots=1;std::vector<std::uint32_t> supplied;std::uint32_t randomCalls{};
    ProductionMaintenanceReads reads;reads.active=[] { return true; };reads.technology=reads.templateTechnology=[] { return 5U; };reads.temple=[] { return 1U; };reads.editor=reads.generatorTimer=reads.stockRate=[] { return 0U; };reads.artificial=[](Sid) { return false; };
    ProductionMaintenanceHooks hooks;hooks.wallSeconds=hooks.gameSeconds=[] { return 100.0; };hooks.money=[] { return 100.0F; };hooks.touchDeck=[] {};hooks.pattern=[&] { ++randomCalls;return 7U; };
    hooks.create=[&](Sid,std::uint32_t,std::uint32_t pattern,std::uint32_t,std::uint32_t,std::uint32_t) { supplied.push_back(pattern); };hooks.destroy=[](Sid) {};hooks.allowed=[](auto) { return true; };hooks.cost=[](auto) { return 0; };hooks.refresh=[](auto) {};
    RawProductionMaintenance maintenance(pool,188,finder,globals,panel,reads,hooks);maintenance.Run();maintenance.Run();CHECK((supplied==std::vector<std::uint32_t>{3,0}) && randomCalls==2 && globals.sequence==2);
    panel.slots=0;maintenance.Run();CHECK(globals.testBridgeDraws==2 && randomCalls==2);globals.ConfigureTestBridgePatterns("off");panel.slots=1;maintenance.Run();CHECK(supplied.back()==12 && randomCalls==3 && globals.sequence==3);
}
// 기본53비트 출력 전체를 검증하며 보조64비트 차이는 원본 보고서에 별도 보존한다.
TEST_CASE(ProductionMaintenance_NativeAllInputs) {
    std::size_t count{};
    // 명시 입력을 복원하고 기대값은 원본 파일에서만 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_PRODUCTION_IDLE_FIXTURE).rows) {
        CHECK(row.size()==6);const auto text=Split(row[0],',');CHECK(text.size()==27);std::array<std::uint64_t,27> values{};
        // double비트와 signed입력의 낮은DWORD를 모두 보존한다.
        for (std::size_t i=0;i<values.size();++i) values[i]=std::stoull(text[i]);
        const auto number=[&](std::size_t i) { return static_cast<std::uint32_t>(values[i]); }; // 원문 DWORD 입력이다.
        const auto real=[&](std::size_t i) { return std::bit_cast<double>(values[i]); }; // 원문 double 입력이다.
        SidPool pool(OriginalEdition::Patch1078,65535);ContainedFinderState finder;
        // 같은 로컬 SID5..8을 확보하며 생성/삭제 외부 효과는 아래의 관측 경계로 둔다.
        for (std::uint16_t id=5;id<=8;++id) { CHECK(pool.Allocate(2)==Sid{id});auto raw=pool.AllocatedBytes({id});std::fill(raw.begin(),raw.end(),std::uint8_t{0}); }
        auto parent=pool.AllocatedBytes({5});Put(parent,6,6,2);parent[10]=152;parent[34]=1;
        const std::array<std::uint32_t,3> kinds{82,128,133}; // 원본 입력의 다리와 두 생산 종류다.
        // 각 내용물의 실제 타입/next/종류/flags/수량을 같은 위치에 공급한다.
        for (std::size_t i=0;i<3;++i) { auto raw=pool.AllocatedBytes({static_cast<std::uint16_t>(6+i)});Put(raw,0,0x1240f000);Put(raw,4,i==2 ? 0U : static_cast<std::uint32_t>(7+i),2);raw[10]=6;Put(raw,18,kinds[i],2);Put(raw,20,i ? 8U : number(20),2);Put(raw,12,i ? 0U : number(22),2);raw[30]=static_cast<std::uint8_t>(number(i ? 23+i : 19)); }
        Put(pool.AllocatedBytes({6}),31,number(21));ProductionMaintenanceGlobals globals;globals.stockUntil.resize(188);globals.stockUntil[128]=globals.stockUntil[133]=real(26);globals.stockEnabled=number(11);globals.stockLimit=std::bit_cast<std::int32_t>(number(12));globals.quickBridges=number(14);globals.forcedPattern=number(15);globals.sequence=number(16);
        ProductionMaintenancePanel panel;panel.bridgeUntil=real(10);panel.oldMoney=std::bit_cast<std::int32_t>(number(17));panel.slots=static_cast<std::int32_t>(number(5));panel.temple=number(4);
        ProductionMaintenanceReads reads;reads.active=[&] { return number(0)==1 || number(0)==2; };reads.technology=reads.templateTechnology=[&] { return number(2); };reads.temple=[&] { return number(3); };reads.editor=[&] { return number(1); };reads.generatorTimer=[&] { return number(7); };reads.stockRate=[&] { return number(13); };reads.artificial=[&](Sid) { return number(6)!=0; };
        std::string events;const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 원본 효과 순서를 그대로 직렬화한다.
        ProductionMaintenanceHooks hooks;hooks.wallSeconds=[&] { return real(8); };hooks.gameSeconds=[&] { return real(9); };hooks.money=[&] { return std::bit_cast<float>(number(18)); };hooks.touchDeck=[] {};
        hooks.pattern=[&] { event("R:10000");return static_cast<std::uint32_t>(SelectBridgePattern(1234)); };
        hooks.create=[&](Sid sid,std::uint32_t type,std::uint32_t argument,std::uint32_t flags,std::uint32_t attach,std::uint32_t quality) { event("C:"+std::to_string(sid.value)+','+std::to_string(type)+','+std::to_string(argument)+','+std::to_string(flags)+','+std::to_string(attach)+','+std::to_string(quality)); };
        hooks.destroy=[&](Sid sid) { event("D:"+std::to_string(sid.value)); };hooks.allowed=[&](std::uint32_t type) { return ((number(23)>>static_cast<unsigned>(type==133))&1U)!=0; };hooks.cost=[](std::uint32_t type) { return type==128 ? 400 : 200; };hooks.refresh=[&](std::uint32_t mask) { event("U:"+std::to_string(mask)); };
        RawProductionMaintenance maintenance(pool,188,finder,globals,panel,std::move(reads),std::move(hooks));maintenance.Run();
        std::string raw,times;
        // 현재 세 원본 슬롯의 전체150바이트와188개double을 생략 없이 비교한다.
        for (std::uint16_t id=6;id<=8;++id) raw+=Hex(pool.Slot({id}));
        for (const auto value:globals.stockUntil) times+=DoubleHex(value);
        CHECK(DoubleHex(panel.bridgeUntil)+','+std::to_string(std::bit_cast<std::uint32_t>(panel.oldMoney))+','+std::to_string(panel.temple)==row[1]);CHECK(raw==row[2]);CHECK(times==row[3]);CHECK(std::to_string(globals.sequence)==row[4]);CHECK((events.empty() ? "-" : events)==row[5]);++count;
    }
    CHECK(count==86);
}
// 첫 다리 취득의0/0을 포함한 모든 유한 경계를 원본 발자국/실제 첫 이웃과 대조한다.
TEST_CASE(ProductionMaintenance_TransientNeighborNativeBoundary) {
    std::size_t count{};
    // 입력 좌표/발자국만 설치하고 기대 사각형은 원본16바이트를 그대로 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_NEIGHBOR_BOUNDARY_FIXTURE).rows) {
        CHECK(row.size()==5);SidPool pool(OriginalEdition::Patch1078,65535);const auto source=pool.Allocate(2);auto raw=pool.AllocatedBytes(source);std::fill(raw.begin(),raw.end(),std::uint8_t{0});raw[10]=82;raw[40]=8;Put(raw,14,Number(row[0]));Put(raw,18,Number(row[1]));
        std::vector<RiftTypeRecord> types(188);types[82].flags1=0x800;types[82].flags2=4;types[82].footX=types[82].footY=static_cast<std::int32_t>(Number(row[2]));std::vector<RiftTypeFrames> frames(188,RiftTypeFrames(std::vector<FrameCode>{}));frames[82]=Frames(0);
        SquidHash hash;hash.Cell(0,20,21)=source.value;std::vector<std::uint8_t> spots(65536);RawSquidNeighbors neighbors(pool,hash,spots,types,frames);const auto foot=neighbors.Footprint(source);
        const std::array<float,4> actual{foot.left,foot.top,foot.right,foot.bottom};CHECK(Hex(std::bit_cast<std::array<std::uint8_t,16>>(actual))==row[3]);CHECK(RawSquidNeighborWalk(neighbors,source,0).Current().value==Number(row[4]));++count;
    }
    CHECK(count==18);
}
