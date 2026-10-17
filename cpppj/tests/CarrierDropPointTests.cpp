// 실제998개 원문 입력의 위치/전체 효과/현재 전역/raw50바이트를 비교한다.
#include "RawSceneSupport.h"
#include "client/RawCarrierDropPoint.h"
#include "client/RawCarrierMenu.h"
#include <sstream>

using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 고정 입력18열은 원문 DWORD이며 음수/float도 먼저 비트를 보존한다.
enum Column : std::size_t { Operation,X,Y,Owner,TypeFlags,FootX,FootY,CenterX,CenterY,Limit,InvalidX,InvalidY,SuccessOrdinal,Result,Mutation,Argument,QueryFlags,QueryOwner };
// 기대값이나 분기 계산 없이 입력 DWORD만 해석한다.
std::vector<std::uint32_t> Numbers(const std::string& text) {
    std::vector<std::uint32_t> values;
    // 각 열을 원본 unsigned32 문자열로 읽는다.
    for (const auto& value:Split(text,',')) values.push_back(Number(value));return values;
}
// 독립 관찰과 같은 입력 저장소/명시 경계 효과만 소유한다.
struct PointScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // Native와 같은50바이트 실제 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 target123의 flags/foot을 참조한다.
    CarrierDropPointState state; // 현재 지도 한계와 실패점이다.
    std::vector<std::uint32_t> input; // 원문18DWORD 입력이다.
    std::string trace; // 실제 배치/중심 경계 인자의 호출 순서다.
    std::uint32_t queries{}; // 명시 배치 경계의 현재 호출 횟수다.
    // 명시 입력과 경계 효과에만 쓰는 raw 접근이며 제품의 할당 판정은 우회하지 않는다.
    std::span<std::uint8_t> Raw() { const auto slot=pool.Slot({50});return {const_cast<std::uint8_t*>(slot.data()),slot.size()}; }
    // 전체 raw를 A5로 채우고 원문에 지정된 입력 필드만 덮어쓴다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        input=values;queries=0;trace.clear();auto raw=Raw();std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});
        raw[10]=158;raw[34]=static_cast<std::uint8_t>(input[Owner]);Put(raw,14,input[X]);Put(raw,18,input[Y]);
        types[123].flags2=input[TypeFlags];types[123].footX=std::bit_cast<std::int32_t>(input[FootX]);types[123].footY=std::bit_cast<std::int32_t>(input[FootY]);
        state.mapLimit=std::bit_cast<std::int32_t>(input[Limit]);state.invalidPoint={std::bit_cast<float>(input[InvalidX]),std::bit_cast<float>(input[InvalidY])};
    }
    // unsigned32 인수를 원문과 같은 순서로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 기록 중 DWORD의 부호를 재해석하지 않는다.
        for (const auto value:values) trace+=','+std::to_string(value);
    }
    // 원문에 명시한 배치 반환/재진입 변경과 중심 입력만 공급한다. 위치는 계산하지 않는다.
    CarrierDropPointHooks Hooks() {
        return {[this](const CanonPlacementQuery& q) {
            ++queries;Event("P",{q.type,q.argument,std::bit_cast<std::uint32_t>(q.x),std::bit_cast<std::uint32_t>(q.y),q.flags,q.owner,q.mode});
            if (queries==1) {
                const auto mutation=input[Mutation];auto raw=Raw();
                if (mutation==1 || mutation==8) raw[34]=static_cast<std::uint8_t>(input[Owner]+3);
                if (mutation==2) { types[123].footX=0;types[123].footY=0; }
                if (mutation==3 || mutation==8) { types[123].footX=3;types[123].footY=3; }
                if (mutation==4 || mutation==8) state.mapLimit=64;
                if (mutation==5 || mutation==8) state.invalidPoint={17.25f,18.5f};
                if (mutation==6) { Put(raw,14,std::bit_cast<std::uint32_t>(99.0f));Put(raw,18,std::bit_cast<std::uint32_t>(-77.0f)); }
                if (mutation==7) types[123].flags2=0x100;
                if (mutation==9) types[123].footY=1;
            }
            return queries==input[SuccessOrdinal] && input[Result]!=0;
        },[this](Sid sid) { Event("C",{sid.value});return std::array{std::bit_cast<float>(input[CenterX]),std::bit_cast<float>(input[CenterY])}; }};
    }
    // 원문에서 관찰한 지도/실패점/TYPE 여섯 DWORD를 그대로 직렬화한다.
    std::string Snapshot() const {
        const std::array values{static_cast<std::uint32_t>(state.mapLimit),std::bit_cast<std::uint32_t>(state.invalidPoint[0]),std::bit_cast<std::uint32_t>(state.invalidPoint[1]),types[123].flags2,static_cast<std::uint32_t>(types[123].footX),static_cast<std::uint32_t>(types[123].footY)};
        std::string result;
        // 관측 열 순서대로 현재 값만 모은다.
        for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
    }
};
// 결과 두 float를 산술 없이 저장한 비트와 비교한다.
std::string PointBits(std::array<float,2> point) { return std::to_string(std::bit_cast<std::uint32_t>(point[0]))+','+std::to_string(std::bit_cast<std::uint32_t>(point[1])); }
}
// 위치와 발자국 탐색의 모든 실제 입력/효과/현재 전역 및 raw 슬롯을 독립 기계어 관찰과 비교한다.
TEST_CASE(CarrierDropPoint_NativeFindAndFootprint) {
    PointScene scene;std::size_t count{};
    // 이전 결과를 새 입력으로 사용하지 않고 매 행 전체 입력을 다시 공급한다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIER_DROP_POINT_FIXTURE).rows) {
        CHECK(row.size()==6);const auto input=Numbers(row[0]);if (input[Operation]==2) continue;scene.Prepare(input);
        RawCarrierDropPoint point(scene.pool,scene.types,scene.state,scene.Hooks());
        const auto result=input[Operation]==0 ? point.Find({50},123) : point.FindFootprint(123,input[Argument],std::bit_cast<float>(input[X]),std::bit_cast<float>(input[Y]),input[QueryFlags],input[QueryOwner]);
        const bool same=PointBits(result)==row[1] && (scene.trace.empty() ? "-" : scene.trace)==row[2] && scene.Snapshot()==row[3] && Hex(scene.pool.Slot({50}))==row[4];CHECK(same);
        if (!same) { std::printf("운반물 위치 입력=%s\n실제점=%s 원문점=%s\n실제호출=%s\n원문호출=%s\n",row[0].c_str(),PointBits(result).c_str(),row[1].c_str(),scene.trace.c_str(),row[2].c_str());break; }++count;
    }
    CHECK(count==742);
}
// 기본53비트 결과를 비교하며 거대한 입력의64비트 차이를 숨기거나 기대값을 재생성하지 않는다.
TEST_CASE(CarrierDropPoint_NativeSnapPrecision) {
    std::size_t count{},different{};
    // 비유한 값·하위DWORD 경계도 원문 비트로 대조한다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIER_DROP_POINT_FIXTURE).rows) {
        const auto input=Numbers(row[0]);if (input[Operation]!=2) continue;
        const auto point=RawCarrierDropPoint::Snap({std::bit_cast<float>(input[X]),std::bit_cast<float>(input[Y])});CHECK(PointBits(point)==row[1]);
        if (row[5]!="-") { CHECK(row[1]!=row[5]);++different; }++count;
    }
    CHECK(count==256 && different==31);
}
// 같은 실제 전체 파이프라인과 현재 TYPE 중심을 메뉴 경계까지 연결한다. 공간 검사는 강제 허용 경로다.
TEST_CASE(CarrierDropPoint_RealPipelineCenterAndMenu) {
    PointScene scene;scene.Prepare(Numbers("0,1109450752,1112211456,1,0,2,3,0,0,256,0,0,0,1,0,0,0,1"));
    SquidHash hash;std::vector<PriestPlainCanonType> frames(188);std::vector<SquidDisplayShape> shapes(188);std::vector<PriestTypeHotspot> hotspots(188);
    std::vector<std::uint8_t> spots(65536);std::vector<std::uint16_t> islands(65536);std::vector<CanonPlacementIslandRegion> regions(128);
    CanonPlacementPipelineState placementState;placementState.placement.forcePlacement=1;SquidPostPopState books;SquidPostPopList additional;ContainedFinderState contained;
    RawCanonPlacementPipeline pipeline(scene.pool,hash,{scene.types,frames,shapes,hotspots,spots,islands,regions},books,additional,contained,placementState);
    std::vector<GunTargetType> targetTypes(188);GunTargetState targetsState;RawGunTarget centers(scene.pool,hash,scene.types,targetTypes,shapes,targetsState,{});
    RawCarrierDropPoint point(scene.pool,scene.types,scene.state,MakeCarrierDropPointHooks(scene.pool,pipeline,centers));CarrierMenuHooks menu;
    BindCarrierMenuDropPoint(menu,scene.pool,point);CHECK((menu.dropPoint({50},123)==std::array<float,2>({41,51})));CHECK(placementState.placement.blockedRelation==0);
    scene.types[123].flags2=0x100;scene.types[158].footX=2;scene.types[158].footY=3;
    CHECK((menu.dropPoint({50},123)==std::array<float,2>({40,50})));
    scene.types[158].footX=4;scene.types[158].footY=5;CHECK((menu.dropPoint({50},123)==std::array<float,2>({39,49})));
    // 실제 Carrier.Open→contained finder→현재 위치→전체 MayPlace→Drop 항목을 이어 호출한다.
    auto raw=scene.Raw();Put(raw,6,60,2);raw[40]=0;scene.types[123].flags2=0;contained.containedType=6;
    const auto inputSlot=scene.pool.Slot({60});std::span<std::uint8_t> content{const_cast<std::uint8_t*>(inputSlot.data()),inputSlot.size()};
    std::fill(content.begin(),content.end(),std::uint8_t{});content[10]=6;Put(content,18,123,2);
    std::uint32_t local=1,help=0x03000046;bool editor=false;float cdSp{};CarrierMenuState menuState{local,editor,scene.state.mapLimit,cdSp,help};
    int headers=0,bases=0;std::vector<bool> enabled;
    menu.groundImmobile=[](Sid sid) { CHECK(sid.value==50);return false; };
    menu.description=[](std::uint32_t type) { CHECK(type==123);return std::string("Artifact"); };
    menu.format=[](std::string_view key,std::span<const PriestMenuArgument>) { return std::string(key); };
    menu.header=[&](std::string_view) { ++headers; };
    menu.item=[&](std::string_view,std::uint32_t command,std::uint32_t parameter,bool allowed) { CHECK(command==menuState.dropCommand && parameter==50);enabled.push_back(allowed); };
    menu.damageableMenu=[&](Sid sid) { CHECK(sid.value==50);++bases; };
    RawCarrierMenu carrier(scene.pool,scene.types,contained,menuState,menu);placementState.placement.blockedRelation=77;carrier.Open({50});
    CHECK(placementState.placement.blockedRelation==0);scene.state.mapLimit=0;carrier.Open({50});
    CHECK(headers==2 && bases==2 && enabled==std::vector<bool>({true,false}));scene.state.mapLimit=256;
    SidPool other(OriginalEdition::Patch1078,24000,false);CHECK(Throws([&] { MakeCarrierDropPointHooks(other,pipeline,centers); }));CHECK(Throws([&] { BindCarrierMenuDropPoint(menu,other,point); }));
}
// 누락 경계·판본/타입/SID 범위는 원문에서 해당 분기를 쓰는 시점에 거부한다.
TEST_CASE(CarrierDropPoint_Contracts) {
    PointScene scene;scene.Prepare(Numbers("0,1109450752,1112211456,1,0,2,3,0,0,256,0,0,0,1,0,0,0,1"));
    RawCarrierDropPoint missing(scene.pool,scene.types,scene.state,{});CHECK(Throws([&] { missing.Find({50},123); }));
    scene.types[123].flags2=0x100;CHECK(Throws([&] { missing.Find({50},123); }));CHECK(Throws([&] { missing.Find({50},188); }));
    CHECK(Throws([&] { missing.Find({24000},123); }));CHECK(Throws([&] { missing.FindFootprint(123,0,20,30,0,1); }));
    scene.types[123].footX=0;CHECK(missing.FindFootprint(123,0,20,30,0,1)==scene.state.invalidPoint);
    SidPool cd(OriginalEdition::Cd1072,24000,false);CHECK(Throws([&] { RawCarrierDropPoint invalid(cd,scene.types,scene.state,{}); }));
    CHECK(Throws([&] { RawCarrierDropPoint invalid(scene.pool,{},scene.state,{}); }));
}
