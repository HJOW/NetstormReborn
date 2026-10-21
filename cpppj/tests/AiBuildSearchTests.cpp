// 실제574상태와spot으로초기화/행재개/시간관문/거리저장을검사한다.작성·컴파일만한다.
#include "o/RawAiBuildSearch.h"
#include "RawSceneSupport.h"
#include <cmath>
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은실제본체/TYPE/spot과명시벽시각/배치효과를준비한다.
struct AiSearchScene {
    static constexpr std::uint32_t kTypeBase=0x3e000000; //검사의opaque32 TYPE표기준이며500stride를보존한다.
    RawAiBrainMemory memory{0x4d000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x5a}); }};std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //실제본체와고유핸들이다.
    std::vector<RiftTypeRecord> types{188};std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536,2);AiBuildSearchHooks hooks; //현재표/월드spot/명시하위다.
    std::vector<CanonPlacementQuery> queries;std::vector<double> times;std::size_t clockCalls{};double now{};bool allow{true}; //실제호출/벽시각입력과배치결과다.
    // 기본범위는 x2..4/y2..3이고모든칸이비어있다.
    AiSearchScene() { types[80].footX=types[80].footY=1;types[81].footX=types[81].footY=1;hooks.wall=[this] { const auto index=clockCalls++;if (times.empty()) return now;CHECK(index<times.size());return index<times.size() ? times[index] : std::numeric_limits<double>::quiet_NaN(); };hooks.mayPlace=[this](const auto& query) { queries.push_back(query);return allow; };Make().Initialize(token,80,{1,1,5,4},{0,0},0x102); }
    // 현재state상대DWORD만쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),0x574+at,value); }
    // 현재state상대DWORD를읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),0x574+at); }
    // 같은현재저장소/전역효과를빌린다.
    RawAiBuildSearch Make() { return RawAiBuildSearch(memory,types,spots,kTypeBase,hooks); }
};
}
// footprint를더한왼쪽/위절삭과전체owner/cache를쓰고34BYTE주변원문을보존한다.
TEST_CASE(AiBuildSearch_InitializeRawStateBoundsAndSurroundings) {
    AiSearchScene input;input.types[80].footX=2;input.types[80].footY=3;const auto before=std::vector<std::uint8_t>(input.memory.Bytes(input.token).begin(),input.memory.Bytes(input.token).end());const auto nan=std::bit_cast<float>(0x7fc01234U);input.Make().Initialize(input.token,80,{-1.9F,-2.9F,7.9F,9.9F},{nan,-0.0F},0xdead0102);CHECK(input.Field(0)==80 && input.Field(4)==0 && input.Field(8)==0xc61c4000U && input.Field(0xc)==0xc61c4000U && input.Field(0x10)==0x7fc01234U && input.Field(0x14)==0x80000000U && input.Field(0x18)==AiSearchScene::kTypeBase+80*500 && input.Field(0x1c)==0xdead0102U && input.Field(0x20)==0 && input.Field(0x24)==0 && input.Field(0x28)==7 && input.Field(0x2c)==9 && input.Field(0x30)==0);const auto raw=input.memory.Bytes(input.token);CHECK(std::equal(before.begin(),before.begin()+0x574,raw.begin()) && std::equal(before.begin()+0x5a8,before.end(),raw.begin()+0x5a8));
}
// 3f마스크/전체footprint를검사하며0이하폭/높이는칸접근없이성공할수있다.
TEST_CASE(AiBuildSearch_FreeFootprintMaskAndEmptyDimensions) {
    AiSearchScene input;input.types[80].footX=2;input.types[80].footY=3;input.spots[5*256+4]=0xc2;CHECK(input.Make().FreeFootprint(80,5,6));input.spots[5*256+4]=0x42;CHECK(input.Make().FreeFootprint(80,5,6));input.spots[5*256+4]=3;CHECK(!input.Make().FreeFootprint(80,5,6));input.types[80].footY=0;CHECK(input.Make().FreeFootprint(80,-2,-2));input.types[80].footY=3;input.types[80].footX=0;CHECK(input.Make().FreeFootprint(80,-2,-2));
}
// TYPE0은시계도요구하지않고TYPE비영의빈행범위는최초시계만읽는다.
TEST_CASE(AiBuildSearch_TypeZeroAndCompletedClockOrder) {
    AiSearchScene zero;zero.Field(0,0);zero.hooks={};zero.Make().Scan(zero.token,1);CHECK(zero.clockCalls==0);AiSearchScene empty;empty.Field(0x30,empty.Field(0x2c));empty.hooks.mayPlace={};empty.Make().Scan(empty.token,1);CHECK(empty.clockCalls==1 && empty.queries.empty());
}
// 순서대로strict최대거리만전체배치에넘기며owner는현재BYTE/나머지인자는원본값이다.
TEST_CASE(AiBuildSearch_FarthestCandidateAndActualQueryArguments) {
    AiSearchScene input;input.Make().Scan(input.token,0);CHECK(input.clockCalls==1 && input.queries.size()==4 && input.Field(4)==std::bit_cast<std::uint32_t>(25.0F) && input.Field(8)==std::bit_cast<std::uint32_t>(4.0F) && input.Field(0xc)==std::bit_cast<std::uint32_t>(3.0F) && input.Field(0x30)==4 && input.Field(0)==80);
    // 선택된네후보의현재인자를비교하며실제허용결과를다시계산하지않는다.
    for (const auto& query:input.queries) CHECK(query.type==80 && !query.argument && !query.flags && query.owner==2 && query.mode==1);
    CHECK(input.queries[0].x==2 && input.queries[0].y==2 && input.queries[3].x==4 && input.queries[3].y==3);input.Make().Scan(input.token,0);CHECK(input.clockCalls==2 && input.queries.size()==4);
}
// 동률은앞선후보를유지하고거리0/NaN은최초score0보다크지않다.
TEST_CASE(AiBuildSearch_StrictTieZeroAndUnorderedScore) {
    AiSearchScene tie;tie.Make().Initialize(tie.token,80,{0,0,3,2},{1.5F,1},2);tie.Make().Scan(tie.token,0);CHECK(tie.queries.size()==1 && tie.Field(8)==std::bit_cast<std::uint32_t>(1.0F));AiSearchScene zero;zero.Make().Initialize(zero.token,80,{0,0,2,2},{1,1},2);zero.Make().Scan(zero.token,0);CHECK(zero.queries.empty() && zero.Field(8)==0xc61c4000U);AiSearchScene nan;nan.Field(4,0x7fc01234U);nan.Make().Scan(nan.token,0);CHECK(nan.queries.empty() && nan.Field(4)==0x7fc01234U);
}
// spot탈락은현재TYPE를조회하지않고선택된칸에서만실제footprint를요구한다.
TEST_CASE(AiBuildSearch_SpotGateKeepsLazyTypeAccess) {
    AiSearchScene input;input.Field(0,200);std::fill(input.spots.begin(),input.spots.end(),std::uint8_t{});input.Make().Scan(input.token,0);CHECK(input.queries.empty() && input.Field(0x30)==4);input.Field(0x30,2);input.spots[2*256+2]=2;CHECK(Throws([&] { input.Make().Scan(input.token,0); }) && input.Field(0x30)==2);
}
// 현재행을완료하고다음행예산에서멈추며다음호출은그행부터현재score를유지해재개한다.
TEST_CASE(AiBuildSearch_BudgetedRowPauseAndResume) {
    AiSearchScene input;input.times={0,0.02,0.031};input.Make().Scan(input.token,1);CHECK(input.clockCalls==3 && input.Field(0x30)==3 && input.Field(4)==std::bit_cast<std::uint32_t>(20.0F) && input.queries.size()==3);input.times={1,1};input.clockCalls=0;input.Make().Scan(input.token,1);CHECK(input.clockCalls==2 && input.Field(0x30)==4 && input.Field(4)==std::bit_cast<std::uint32_t>(25.0F) && input.queries.size()==4);
}
// deadline동일은진행하고unordered는중단한다.비예산모드는최초시각NaN도행완료를막지않는다.
TEST_CASE(AiBuildSearch_BudgetEqualityAndUnorderedClock) {
    AiSearchScene equal;equal.times={0,static_cast<double>(0.03F),std::numeric_limits<double>::quiet_NaN()};equal.Make().Scan(equal.token,1);CHECK(equal.Field(0x30)==3 && equal.queries.size()==3);AiSearchScene unordered;unordered.times={std::numeric_limits<double>::quiet_NaN(),0};unordered.Make().Scan(unordered.token,1);CHECK(unordered.Field(0x30)==2 && unordered.queries.empty());AiSearchScene full;full.times={std::numeric_limits<double>::quiet_NaN()};full.Make().Scan(full.token,0);CHECK(full.clockCalls==1 && full.Field(0x30)==4);
}
// 배치효과가시간을초과시켜도행안에서는시계를추가조회하지않는다.
TEST_CASE(AiBuildSearch_BudgetChecksOnlyBetweenRows) {
    AiSearchScene input;input.hooks.mayPlace=[&](const auto& query) { input.queries.push_back(query);input.now=1;return true; };input.Make().Scan(input.token,1);CHECK(input.clockCalls==3 && input.queries.size()==3 && input.Field(0x30)==3 && input.Field(8)==std::bit_cast<std::uint32_t>(4.0F));
}
// 행snapshot과cachedTYPE/owner의현재재조회및배치후무조건score저장을보존한다.
TEST_CASE(AiBuildSearch_CurrentCachedTypeOwnerAndRowSnapshot) {
    AiSearchScene input;input.hooks.mayPlace=[&](const auto& query) { input.queries.push_back(query);input.Field(4,std::bit_cast<std::uint32_t>(100000.0F));input.Field(0x18,AiSearchScene::kTypeBase+81*500);input.Field(0x1c,0x1ff);input.Field(0x20,100);input.Field(0x30,10);return true; };input.Make().Scan(input.token,0);CHECK(input.queries.size()==3 && input.queries[0].type==80 && input.queries[1].type==81 && input.queries[1].owner==255 && input.queries[2].y==2 && input.Field(4)==std::bit_cast<std::uint32_t>(20.0F) && input.Field(0x30)==11 && input.Field(0x20)==100);
}
// maxX는후보효과뒤현재값을읽고cachedTYPE/선택하위누락은앞선시계효과까지만보존한다.
TEST_CASE(AiBuildSearch_CurrentMaxAndSelectedMissingEffect) {
    AiSearchScene bound;bound.Field(0x2c,3);bound.hooks.mayPlace=[&](const auto& query) { bound.queries.push_back(query);bound.Field(0x28,3);return true; };bound.Make().Scan(bound.token,0);CHECK(bound.queries.size()==1 && bound.Field(0x30)==3);AiSearchScene invalid;invalid.Field(0x18,AiSearchScene::kTypeBase+1);CHECK(Throws([&] { invalid.Make().Scan(invalid.token,0); }) && invalid.clockCalls==1 && invalid.Field(4)==0 && invalid.Field(0x30)==2);AiSearchScene missing;missing.hooks.mayPlace={};CHECK(Throws([&] { missing.Make().Scan(missing.token,0); }) && missing.clockCalls==1 && missing.Field(4)==0 && missing.Field(0x30)==2);
}
// x차이는float저장/y차이는미축소이며거리비교는float score저장보다먼저다.
TEST_CASE(AiBuildSearch_DistanceComparisonBeforeFloatStore) {
    AiSearchScene input;input.Make().Initialize(input.token,80,{0,0,2,2},{-33554432.0F,-33554432.0F},2);const auto same=std::bit_cast<std::uint32_t>(std::ldexp(1.0F,51));input.Field(4,same);input.Make().Scan(input.token,0);CHECK(input.queries.size()==1 && input.Field(4)==same && input.Field(8)==std::bit_cast<std::uint32_t>(1.0F) && input.Field(0xc)==std::bit_cast<std::uint32_t>(1.0F));
}
// 초기화의TYPE조회실패는먼저쓴TYPE를유지하며지도밖주소를합법칸으로clamp하지않는다.
TEST_CASE(AiBuildSearch_InitializeFailurePrefixAndPhysicalBounds) {
    AiSearchScene input;const auto cached=input.Field(0x18);CHECK(Throws([&] { input.Make().Initialize(input.token,200,{},{},2); }) && input.Field(0)==200 && input.Field(0x18)==cached);AiSearchScene address;address.Field(0x20,0xffffffffU);CHECK(Throws([&] { address.Make().Scan(address.token,0); }) && address.clockCalls==1 && address.Field(0x30)==2);
}
