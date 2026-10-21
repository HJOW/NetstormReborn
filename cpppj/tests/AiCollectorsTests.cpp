// 원본 수집자 선택의 단락 평가·난수 소비·후보/현재 입력을 합성으로 검사한다. 작성·컴파일만 한다.
#include "o/RawAiCollectors.h"
#include "RawSceneSupport.h"
#include <limits>
#include <tuple>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 본체/목록/사제와 원본 난수에 명시 하위 효과를 연결한다.
struct CollectorsInput {
    RawAiBrainMemory memory{0x46000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //명시0 입력이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},localOwner{1},invalidGraph{254}; //같은 실제 본체와 현재 전역이다.
    SidPool pool{OriginalEdition::Patch1078,32768,true};std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //현재 사제/생산 슬롯과 TYPE다.
    GameRandom random{100};double now{16777216.25}; //첫1000난수가100인 경계 입력과 QWORD 시각이다.
    Sid priest{},ground{},flying{};std::map<std::uint32_t,Sid> products; //같은 풀의 명시 사제/생산 입력이다.
    AiCollectorsHooks hooks;bool affordable{true};std::size_t failures{}; //선택 효과/비용과 실패시킬 후보 개수다.
    std::vector<std::uint32_t> productQueries,affordQueries;std::vector<CanonPlacementQuery> candidates; //실제 조회 인자/순서다.
    std::vector<std::tuple<std::uint32_t,float,float,std::uint32_t,std::uint32_t>> deployed;std::vector<std::tuple<std::uint32_t,std::int32_t,std::int32_t>> logs;std::uint32_t sounds{},enemyQueries{},hpQueries{}; //실제 배치/장치 요청 관찰이다.
    // 자기 영역과 지상/비행 두 타입의 실제 목록 backing을 준비한다.
    CollectorsInput() {
        priest=New();ground=New();flying=New();Put(pool.AllocatedBytes(priest),26,10);types[133].flags2=0x10000;types[134].flags2=0x20000;products={{133,ground},{134,flying}};
        Field(0x260,2);Field(0x264,3);Field(0x294,100);Field(0x298,7);Field(0x2a0,priest.value);Field(0x388,1);Field(0x284,memory.Allocate(8));Field(0x28c,1);Put(memory.Bytes(Field(0x284)),0,133);Put(memory.Bytes(Field(0x284)),4,134);Field(0x2dc,memory.Allocate(8));Field(0x228,std::bit_cast<std::uint32_t>(2.25F));Bounds(16,32,80,96);
        hooks.product=[this](auto brain,auto type) { CHECK(brain==token);productQueries.push_back(type);const auto found=products.find(type);return found==products.end() ? Sid{} : found->second; };hooks.canAfford=[this](auto brain,auto type) { CHECK(brain==token);affordQueries.push_back(type);return affordable; };hooks.maxHitPoints=[this](Sid sid) { CHECK(sid==priest);++hpQueries;return 100; };hooks.enemyAvailable=[this](auto owner) { CHECK(owner==3);++enemyQueries;return true; };
        hooks.mayPlace=[this](const auto& query) { candidates.push_back(query);return candidates.size()>failures; };hooks.deploy=[this](auto brain,auto type,float x,float y,auto argument,auto direction) { CHECK(brain==token);deployed.emplace_back(type,x,y,argument,direction); };hooks.log=[this](auto brain,auto type,auto x,auto y) { CHECK(brain==token);logs.emplace_back(type,x,y); };hooks.dropSound=[this] { ++sounds; };
    }
    // 실제 할당 슬롯을 명시0 입력으로 준비한다.
    Sid New() { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});return sid; }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 선택된 본체 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 자기 사각형을 명시 입력으로 쓴다.
    void Bounds(float left,float top,float right,float bottom) { Field(0x270,std::bit_cast<std::uint32_t>(left));Field(0x274,std::bit_cast<std::uint32_t>(top));Field(0x278,std::bit_cast<std::uint32_t>(right));Field(0x27c,std::bit_cast<std::uint32_t>(bottom)); }
    // 원본 QWORD 기한을 명시 입력으로 쓴다.
    void Due(std::size_t at,double time) { const auto bits=std::bit_cast<std::uint64_t>(time);Field(at,static_cast<std::uint32_t>(bits));Field(at+4,static_cast<std::uint32_t>(bits>>32)); }
    // 현재 실제 본체의 QWORD 시각을 읽는다.
    double Time(std::size_t at) const { return std::bit_cast<double>((std::uint64_t(Field(at+4))<<32)|Field(at)); }
    // 같은 실제 저장소/현재 참조로 수집자 몸체를 구성한다.
    RawAiCollectors Make() { return RawAiCollectors(memory,pool,types,random,AiCollectorsGlobals{now,localOwner,[this] { return invalidGraph; }},hooks); }
};

// 신전/그래프/후보 관문은 난수와 선택되지 않은 하위를 요구하지 않는다.
TEST_CASE(AiCollectors_PrerequisitesHaveNoRandomEffects) {
    CollectorsInput input;input.hooks={};const auto seed=input.random.State();input.Field(0x294,0);input.Make().Ensure(input.token);input.Field(0x294,100);input.Field(0x298,254);input.Make().Ensure(input.token);input.Field(0x298,7);input.Field(0x28c,0);input.Make().Ensure(input.token);CHECK(input.random.State()==seed && input.deployed.empty());const auto small=input.memory.Allocate(4);CHECK(Throws([&] { input.Make().Ensure(small); }));
}
// 일반 선택은1000→후보종류→y→x 난수와 세 번 생산 조회를 원문 순서로 소비한다.
TEST_CASE(AiCollectors_NormalProductionAndRandomOrder) {
    CollectorsInput input;GameRandom expected{100};CHECK(expected.Next(1000)==100);static_cast<void>(expected.Next(1));const auto y=32.0F+static_cast<float>(expected.Next(64)),x=16.0F+static_cast<float>(expected.Next(64));input.Make().Ensure(input.token);CHECK(input.productQueries==std::vector<std::uint32_t>({133,133,133}) && input.affordQueries==std::vector<std::uint32_t>({133}) && input.candidates.size()==1 && input.deployed.size()==1 && input.random.State()==expected.State());if (!input.candidates.empty()) { const auto& query=input.candidates[0];CHECK(query.type==133 && query.argument==0 && query.x==x && query.y==y && query.flags==0 && query.owner==2 && query.mode==1); }CHECK(input.Time(0x5a8)==input.now+2.25 && input.Time(0x5b0)==input.now+3.25 && input.sounds==0 && input.logs.empty());
}
// 부족하지 않음/기한 전/보류 조각/자금 부족은 각 원문 지점에서만 종료한다.
TEST_CASE(AiCollectors_ReadyPendingMoneyAndSatisfiedGates) {
    CollectorsInput satisfied;satisfied.Field(0x2cc,1);GameRandom first{100};static_cast<void>(first.Next(1000));satisfied.Make().Ensure(satisfied.token);CHECK(satisfied.random.State()==first.State() && satisfied.productQueries.empty());
    CollectorsInput late;late.Due(0x5a8,late.now+1);late.Make().Ensure(late.token);GameRandom onlyChoice{100};static_cast<void>(onlyChoice.Next(1));CHECK(late.random.State()==onlyChoice.State() && late.productQueries.size()==2 && late.deployed.empty());
    CollectorsInput pending;pending.pool.AllocatedBytes(pending.ground)[21]=1;pending.Make().Ensure(pending.token);CHECK(pending.productQueries.size()==3 && pending.affordQueries.empty() && pending.candidates.empty());
    CollectorsInput poor;poor.affordable=false;poor.Make().Ensure(poor.token);CHECK(poor.affordQueries.size()==1 && poor.candidates.empty());
}
// 난수로 고른 타입의 생산이 없으면 현재 첫 후보로 바꾸고 다시 두 번 조회한다.
TEST_CASE(AiCollectors_MissingRandomProductFallsBackToFirst) {
    CollectorsInput input;input.Field(0x28c,2);input.products.erase(134);input.Make().Ensure(input.token);CHECK(input.productQueries==std::vector<std::uint32_t>({134,133,133}) && input.deployed.size()==1 && std::get<0>(input.deployed[0])==133);
}
// 취약 적용 비행 타입은 일반 충족/보류/자금 관문을 추가하지 않으며 HP를 최대HP 하위 전에 저장한다.
TEST_CASE(AiCollectors_VulnerableEnemyPrefersFlyingWithoutExtraGates) {
    CollectorsInput input;input.random.SetState(1);input.Field(0x248,0x100);input.Field(0x28c,2);input.Field(0x2cc,1);input.pool.AllocatedBytes(input.flying)[21]=1;input.hooks.canAfford={};input.hooks.maxHitPoints=[&](Sid sid) { CHECK(sid==input.priest);Put(input.pool.AllocatedBytes(sid),26,1000);return 100; };input.Make().Ensure(input.token);CHECK(input.productQueries==std::vector<std::uint32_t>({133,134}) && input.affordQueries.empty() && input.enemyQueries==1 && input.deployed.size()==1 && std::get<0>(input.deployed[0])==134);
    CollectorsInput existing;existing.random.SetState(1);existing.Field(0x248,0x100);existing.Field(0x2cc,1);existing.Field(0x2e4,1);const auto flight=existing.New();existing.pool.AllocatedBytes(flight)[10]=134;existing.pool.AllocatedBytes(flight)[11]=3;Put(existing.memory.Bytes(existing.Field(0x2dc)),0,flight.value);existing.Make().Ensure(existing.token);CHECK(existing.Make().HasFlying(existing.token) && existing.deployed.empty() && existing.productQueries.empty());
}
// 실패 후보는 최대20회에서 끝나며 각 시도에서 현재 경계를 따로 저장한다.
TEST_CASE(AiCollectors_AttemptLimitAndCurrentBoundsSnapshot) {
    CollectorsInput full;full.failures=20;full.Make().Ensure(full.token);CHECK(full.candidates.size()==20 && full.deployed.empty() && full.Time(0x5a8)==0);
    CollectorsInput changed;changed.Field(0x260,0x102);changed.hooks.mayPlace=[&](const auto& query) { changed.candidates.push_back(query);if (changed.candidates.size()==1) { changed.Bounds(128,96,160,128);return false; }return true; };changed.Make().PlaceInHome(changed.token,133);CHECK(changed.candidates.size()==2 && changed.candidates[0].owner==2 && changed.candidates[1].owner==2 && changed.candidates[1].x>=128 && changed.candidates[1].x<160 && changed.candidates[1].y>=96 && changed.candidates[1].y<128 && changed.deployed.size()==1);
}
// 배치 하위 뒤 현재 rate/시각, 로그 뒤 현재 로컬 소유자를 다시 읽는다.
TEST_CASE(AiCollectors_DeploymentNoticeAndLocalSoundUseCurrentValues) {
    CollectorsInput input;input.Field(0xb00,1);input.hooks.deploy=[&](auto brain,auto type,float x,float y,auto argument,auto direction) { CHECK(brain==input.token);input.deployed.emplace_back(type,x,y,argument,direction);input.now+=3;input.Field(0x228,std::bit_cast<std::uint32_t>(4.25F)); };input.hooks.log=[&](auto brain,auto type,auto x,auto y) { CHECK(brain==input.token);input.logs.emplace_back(type,x,y);input.localOwner=2; };input.Make().DeployWithNotice(input.token,133,-4.5F,7.75F,9,11);CHECK((input.logs==std::vector<std::tuple<std::uint32_t,std::int32_t,std::int32_t>>({{133,-4,7}}) && input.Time(0x5a8)==input.now+4.25 && input.Time(0x5b0)==input.now+5.25 && input.sounds==1));
}
// 정수 절반HP/목록 물리 범위·unordered 기한과 선택된 효과 누락을 확인한다.
TEST_CASE(AiCollectors_PhysicalAndSelectedMissingContracts) {
    CollectorsInput input;auto collectors=input.Make();Put(input.pool.AllocatedBytes(input.priest),26,50);CHECK(!collectors.Vulnerable(input.priest));Put(input.pool.AllocatedBytes(input.priest),26,49);CHECK(collectors.Vulnerable(input.priest));input.Field(0x2e4,1);input.Field(0x2dc,input.memory.Allocate(1));CHECK(Throws([&] { collectors.HasFlying(input.token); }));
    CollectorsInput missing;missing.hooks.mayPlace={};CHECK(Throws([&] { missing.Make().Ensure(missing.token); }));CHECK(missing.deployed.empty() && missing.Time(0x5a8)==0);CollectorsInput zero;zero.Bounds(16,32,80,32);CHECK(Throws([&] { zero.Make().PlaceInHome(zero.token,133); }));CollectorsInput unordered;unordered.Due(0x5a8,std::numeric_limits<double>::quiet_NaN());unordered.Make().Ensure(unordered.token);CHECK(unordered.deployed.empty() && unordered.productQueries.size()==2);
}
}
