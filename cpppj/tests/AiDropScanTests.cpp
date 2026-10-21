// 원본 공용 난수/다리 주변 spot·지도/진행 예산·30후보 압축을 검사한다. 작성·컴파일만 한다.
#include "o/RawAiDropScan.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 본체/슬롯·전체 지도와 원본 공용 난수에 명시 하위 효과를 연결한다.
struct DropScanScene {
    RawAiBrainMemory memory{0x52000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }};std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; // 같은 실제 본체 핸들이다.
    SidPool pool{OriginalEdition::Patch1078,32768,false};std::vector<RiftTypeRecord> types{188};std::vector<RiftTypeFrames> frames{188,RiftTypeFrames({})}; // 실제 풀/현재 TYPE·frame 표다.
    std::array<std::uint8_t,65536> spots{};std::array<std::uint16_t,65536> surfaces{};std::array<std::uint32_t,81> allies{}; // 같은 현재 원본 지도/동맹 입력이다.
    GameRandom random{0x12345678};std::uint32_t bridge{82};AiBuildSearchBounds bounds{10,10,246,246}; // 현재 공용 난수/다리/CRT 스캔 경계다.
    AiDropScanHooks hooks;bool free{true},accepted{};std::vector<std::int32_t> values;std::function<void(std::size_t)> during; // 명시 실제 하위 반환/효과다.
    std::size_t frees{},considers{};std::vector<RawPathPoint> points;std::vector<std::uint32_t> diagnostics; // 실제 호출/좌표·진단 관찰이다.
    // SID5는 J/K 다리, SID6은 일반 자기 표면이며 원본 진행은10/10이다.
    DropScanScene() {
        CHECK(pool.Allocate(2)==Sid{5} && pool.Allocate(2)==Sid{6});auto first=pool.AllocatedBytes(Sid{5}),second=pool.AllocatedBytes(Sid{6});std::fill(first.begin(),first.end(),std::uint8_t{});std::fill(second.begin(),second.end(),std::uint8_t{});first[10]=82;first[34]=second[34]=1;second[10]=94;frames[82]=RiftTypeFrames({{'J',0,0,0},{'K',0,0,0},{'A',0,0,0}});Field(0x260,1);Field(0x2c8,1);Field(0xaf8,10);Field(0xafc,10);allies[1*9+1]=1;
        hooks.free=[this](auto type,auto,auto) { CHECK(type==80);++frees;return free; };
        hooks.consider=[this](auto brain,auto type,RawPathPoint point,auto direction,RawPathPoint& best,Sid& target) { CHECK(brain==token && type==80 && direction==2 && !target.value);points.push_back(point);const auto call=considers++;if (during) during(call);if (!accepted) return false;best=point;target=Sid{9};const auto value=values.empty() ? 10 : values.at(std::min(call,values.size()-1));Field(0x3e8,std::bit_cast<std::uint32_t>(value));return true; };
        hooks.assertion=[this](std::string_view text,auto line) { CHECK(text=="totalChoices > 0" && line==0x1ac);diagnostics.push_back(line); };
    }
    // 같은 현재 본체 DWORD만 입력한다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 원문 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 현재 다리의 물리 프레임을 선택한다.
    void Frame(std::uint32_t value) { Put(pool.AllocatedBytes(Sid{5}),36,value); }
    // 명시 좌표의 실제 surface/spot을 입력한다.
    void Cell(std::int32_t x,std::int32_t y,Sid surface,std::uint8_t spot=2) { const auto at=static_cast<std::size_t>(y*256+x);surfaces[at]=surface.value;spots[at]=spot; }
    // 현재 선택 배열의 한 항목을 명시 입력한다.
    void Choice(std::size_t index,float x,float y,std::uint32_t target) { Field(0x408+index*8,std::bit_cast<std::uint32_t>(x));Field(0x40c+index*8,std::bit_cast<std::uint32_t>(y));Field(0x4f8+index*4,target); }
    // 같은 실제 저장소·난수를 빌린 처리기를 만든다.
    RawAiDropScan Actions() { return RawAiDropScan(memory,pool,types,frames,spots,surfaces,allies,random,{bridge,bounds},hooks); }
};
}

// J는 위쪽 세 칸을 우선하며 기준Y 쓰기 뒤 X에 공용 난수3을 더한다.
TEST_CASE(AiDropScan_BridgeJPreferredSideAndSharedRandom) {
    DropScanScene scene;GameRandom expected(scene.random.State());const auto draw=expected.Next(3);std::int32_t x{20},y{30};CHECK(scene.Actions().AdjustBridge(Sid{5},x,y) && x==20+static_cast<std::int32_t>(draw) && y==29 && scene.random.State()==expected.State());
}
// 전체BYTE0을 검사하므로 고위 spot 비트도 위쪽을 막고 아래쪽을 선택한다.
TEST_CASE(AiDropScan_BridgeJWholeByteFallbackAndFailureNoRandom) {
    DropScanScene scene;scene.spots[29*256+20]=0x40;GameRandom expected(scene.random.State());const auto draw=expected.Next(3);std::int32_t x{20},y{30};CHECK(scene.Actions().AdjustBridge(Sid{5},x,y) && x==20+static_cast<std::int32_t>(draw) && y==33);const auto state=scene.random.State();scene.spots[31*256+20]=1;x=20;y=30;CHECK(!scene.Actions().AdjustBridge(Sid{5},x,y) && x==20 && y==30 && scene.random.State()==state);
}
// K는 왼쪽 우선/오른쪽 대체와 세로 난수이며 선형 행 이웃을 좌표 clamp하지 않는다.
TEST_CASE(AiDropScan_BridgeKAndPhysicalRowBoundary) {
    DropScanScene scene;scene.Frame(1);GameRandom expected(scene.random.State());const auto draw=expected.Next(3);std::int32_t x{0},y{30};CHECK(scene.Actions().AdjustBridge(Sid{5},x,y) && x==-1 && y==30+static_cast<std::int32_t>(draw));scene.spots[30*256+19]=1;x=20;y=30;const auto next=expected.Next(3);CHECK(scene.Actions().AdjustBridge(Sid{5},x,y) && x==23 && y==30+static_cast<std::int32_t>(next));
}
// 다른 TYPE는 프레임을 읽지 않고 J/K 이외 코드는 난수를 쓰지 않는다.
TEST_CASE(AiDropScan_BridgeCurrentTypeAndActualFrameCode) {
    DropScanScene scene;std::int32_t x{20},y{30};Put(scene.pool.AllocatedBytes(Sid{6}),36,0xffffffffU);CHECK(!scene.Actions().AdjustBridge(Sid{6},x,y));scene.Frame(2);const auto state=scene.random.State();CHECK(!scene.Actions().AdjustBridge(Sid{5},x,y) && scene.random.State()==state);scene.Frame(0);scene.bridge=83;CHECK(!scene.Actions().AdjustBridge(Sid{5},x,y) && scene.random.State()==state);
}
// 스캔은 X를 먼저 증가시키고 inclusive 우측/하단 소진 뒤 X/Y를 좌상단으로 되돌린다.
TEST_CASE(AiDropScan_CursorPreincrementAndInclusiveCompletion) {
    DropScanScene scene;scene.bounds={10,10,13,10};scene.Cell(10,10,Sid{5});CHECK(!scene.Actions().Scan(scene.token,80,2) && scene.Field(0xaf8)==10 && scene.Field(0xafc)==10 && !scene.considers);scene.Field(0xaf8,12);scene.Cell(13,10,Sid{5});CHECK(!scene.Actions().Scan(scene.token,80,2) && scene.considers==1 && scene.points.front().y==9);
}
// 빈 지도에서 일반1000/공급500 예산을 정확히 차감하며 재개 위치를 보존한다.
TEST_CASE(AiDropScan_EmptyMapNormalAndSupplyBudgets) {
    DropScanScene normal;CHECK(normal.Actions().Scan(normal.token,80,2) && normal.Field(0xaf8)==62 && normal.Field(0xafc)==14);DropScanScene supply;supply.types[80].flags2=0x800;CHECK(supply.Actions().Scan(supply.token,80,2) && supply.Field(0xaf8)==36 && supply.Field(0xafc)==12);
}
// 다리 후보 한 번은5+125+1을 쓰며 승인 실패도125를 차감한다.
TEST_CASE(AiDropScan_BridgeBudgetConsumesSharedRandomOnRejectedCandidates) {
    // 일반은8회/공급은4회 뒤 예산이소진되며 같은공용난수도그횟수만큼전진한다.
    for (bool supply:{false,true}) { DropScanScene scene;scene.surfaces.fill(5);if (supply) scene.types[80].flags2=0x800;GameRandom expected(scene.random.State());const auto count=supply ? 4U : 8U;for (std::uint32_t index=0;index<count;++index) expected.Next(3);CHECK(scene.Actions().Scan(scene.token,80,2) && scene.considers==count && !scene.frees && scene.Field(0xaf8)==10+count && scene.random.State()==expected.State()); }
}
// 동맹 행은 surfaceOwner*9+owner이며 자기 발자국 실패도추가10을 쓴다.
TEST_CASE(AiDropScan_DirectedAllianceAndOwnFootprintBudget) {
    DropScanScene own;own.surfaces.fill(6);own.spots.fill(2);own.types[80].flags2=0x800;own.free=false;CHECK(own.Actions().Scan(own.token,80,2) && own.frees==32 && !own.considers && own.Field(0xaf8)==42);
    DropScanScene ally;ally.surfaces.fill(6);ally.spots.fill(2);ally.types[80].flags2=0x800;ally.pool.AllocatedBytes(Sid{6})[34]=2;ally.allies[2*9+1]=1;CHECK(ally.Actions().Scan(ally.token,80,2) && !ally.frees && !ally.considers && ally.Field(0xaf8)==42);DropScanScene reverse;reverse.surfaces.fill(6);reverse.spots.fill(2);reverse.types[80].flags2=0x800;reverse.pool.AllocatedBytes(Sid{6})[34]=2;reverse.allies[1*9+2]=1;CHECK(reverse.Actions().Scan(reverse.token,80,2) && !reverse.frees && reverse.Field(0xaf8)==94);
}
// flags/spot/2c8 관문이 막히면 전체 발자국과 후보 승인을 호출하지 않는다.
TEST_CASE(AiDropScan_NonbridgeEligibilityGatesAndMaskedSpot) {
    DropScanScene scene;scene.bounds={10,10,13,10};scene.types[80].flags2=0x800;scene.Cell(11,10,Sid{6},0x82);scene.Cell(12,10,Sid{6},0x42);scene.Cell(13,10,Sid{6},0x40);CHECK(!scene.Actions().Scan(scene.token,80,2) && scene.frees==2 && scene.considers==2);scene.Field(0x2c8,0);scene.frees=scene.considers=0;CHECK(!scene.Actions().Scan(scene.token,80,2) && !scene.frees && !scene.considers);
}
// 같은호출의경계/시설flags는하위효과뒤에도최초값이며진행은본체에남는다.
TEST_CASE(AiDropScan_BoundsAndFlagsArePerCallSnapshot) {
    DropScanScene scene;scene.bounds={10,10,13,10};scene.types[80].flags2=0x800;scene.Cell(11,10,Sid{6});scene.Cell(12,10,Sid{6});scene.Cell(13,10,Sid{6});scene.during=[&](auto call) { if (!call) { scene.bounds={100,100,102,102};scene.types[80].flags2=0; } };CHECK(!scene.Actions().Scan(scene.token,80,2) && scene.frees==3 && scene.considers==3 && scene.Field(0xaf8)==10 && scene.Field(0xafc)==10);
}
// 엄격한점수개선은목록을다시시작하고동률은추가한다.미사용꼬리는지우지않는다.
TEST_CASE(AiDropScan_ImprovementResetsAndEqualScoreAppends) {
    DropScanScene scene;scene.bounds={10,10,13,10};scene.types[80].flags2=0x800;scene.accepted=true;scene.values={20,20,30};scene.Field(0x570,2);scene.Field(0x3e8,10);scene.Choice(0,1,2,7);scene.Choice(1,3,4,8);scene.Cell(11,10,Sid{6});scene.Cell(12,10,Sid{6});scene.Cell(13,10,Sid{6});CHECK(!scene.Actions().Scan(scene.token,80,2) && scene.Field(0x570)==1 && scene.Field(0x3e8)==30 && std::bit_cast<float>(scene.Field(0x408))==13 && scene.Field(0x4f8)==9);CHECK(std::bit_cast<float>(scene.Field(0x410))==12 && scene.Field(0x4fc)==9);
}
// 가득찬30개동률목록은같은공용난수로한칸만교체하고count를증가시키지않는다.
TEST_CASE(AiDropScan_FullThirtyChoicesRandomReplacement) {
    DropScanScene scene;scene.bounds={10,10,11,10};scene.types[80].flags2=0x800;scene.Cell(11,10,Sid{6});scene.accepted=true;scene.Field(0x570,30);scene.Field(0x3e8,10);
    // 미선택29칸의원문도보존되는지각각비교한다.
    for (std::size_t index=0;index<30;++index) scene.Choice(index,static_cast<float>(100+index),200,static_cast<std::uint32_t>(1000+index));GameRandom expected(scene.random.State());const auto selected=expected.Next(30);CHECK(!scene.Actions().Scan(scene.token,80,2) && scene.Field(0x570)==30 && scene.random.State()==expected.State());
    // 선택된칸이외에는좌표와표적어느것도재작성하지않는다.
    for (std::size_t index=0;index<30;++index) { CHECK(std::bit_cast<float>(scene.Field(0x408+index*8))==(index==selected ? 11.0F : static_cast<float>(100+index)));CHECK(scene.Field(0x4f8+index*4)==(index==selected ? 9U : static_cast<std::uint32_t>(1000+index))); }
}
// 승인거부하위가변경한실제점수/count도사본으로되돌리지않는다.
TEST_CASE(AiDropScan_RejectedCandidateKeepsCurrentStoredEffects) {
    DropScanScene scene;scene.bounds={10,10,11,10};scene.types[80].flags2=0x800;scene.Cell(11,10,Sid{6});scene.during=[&](auto) { scene.Field(0x3e8,123);scene.Field(0x570,3); };CHECK(!scene.Actions().Scan(scene.token,80,2) && scene.Field(0x3e8)==123 && scene.Field(0x570)==3);
}
// 후보하위가없으면선증가된진행과이미소비한다리난수는보존한다.
TEST_CASE(AiDropScan_MissingConsiderKeepsCursorAndBridgeRandom) {
    DropScanScene scene;scene.Cell(11,10,Sid{5});scene.hooks.consider={};GameRandom expected(scene.random.State());expected.Next(3);CHECK(Throws([&] { scene.Actions().Scan(scene.token,80,2); }) && scene.Field(0xaf8)==11 && scene.random.State()==expected.State() && !scene.Field(0x570));
}
// 선택 제거는count선감소후XY/표적을안정압축하고꼬리/점수/진행을보존한다.
TEST_CASE(AiDropScan_RemoveStableCompactionAndUntouchedTail) {
    DropScanScene scene;scene.Field(0x570,3);scene.Field(0x3e8,99);scene.Choice(0,1,2,7);scene.Choice(1,3,4,8);scene.Choice(2,5,6,9);scene.Actions().Remove(scene.token,1);CHECK(scene.Field(0x570)==2 && std::bit_cast<float>(scene.Field(0x410))==5 && scene.Field(0x4fc)==9 && std::bit_cast<float>(scene.Field(0x418))==5 && scene.Field(0x500)==9 && scene.Field(0x3e8)==99 && scene.Field(0xaf8)==10);scene.Actions().Remove(scene.token,999);CHECK(scene.Field(0x570)==1 && std::bit_cast<float>(scene.Field(0x408))==1);
}
// count0 진단이돌아오면원본처럼감소를계속하며누락진단은앞에서실패한다.
TEST_CASE(AiDropScan_RemoveDiagnosticContinuationAndPrefix) {
    DropScanScene scene;scene.Actions().Remove(scene.token,0);CHECK(scene.Field(0x570)==0xffffffffU && scene.diagnostics==std::vector<std::uint32_t>{0x1ac});scene.Field(0x570,0);scene.hooks.assertion={};CHECK(Throws([&] { scene.Actions().Remove(scene.token,0); }) && !scene.Field(0x570));scene.Field(0x570,2);CHECK(Throws([&] { scene.Actions().Remove(scene.token,-1); }) && scene.Field(0x570)==1);
}
