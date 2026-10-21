// 원본 초기 전략의 순서·공유 저장소·분기 경계를 합성 입력으로 검사한다. 실행은 중단 지시 해제 뒤 수행한다.
#include "o/RawAiStrategySetup.h"
#include "o/CanonDecoder.h"
#include "RawSceneSupport.h"
#include <limits>
#include <tuple>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 내용물의 실제 생성/부착을 같은 풀에 적용하는 작은 검사 장면이다.
struct StrategyInput {
    RawAiBrainMemory memory{0x43000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //명시0 본체 입력이다.
    SidPool pool{OriginalEdition::Patch1078,32768,true}; //같은 PATCH 물리 슬롯을 사용한다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //실제 TYPE 표의 크기다.
    SquidPostPopState bookkeeping;SquidPostPop post{pool,types,bookkeeping}; //Player getter 조립에 쓰는 같은 월드다.
    std::array<std::uint8_t,1620> players{};std::array<std::uint32_t,81> allies{}; //현재9Player와 한 방향 동맹이다.
    PlayerWorldState playerState;std::unique_ptr<RawPlayerWorld> player; //현재 기술 부모를 직접 읽는다.
    ContainedFinderState finderState;std::unique_ptr<RawCarrierPreDestroy> finder; //기존 실제 조건 검색 몸체다.
    ContainedCreateState createState;std::unique_ptr<RawContainedCreate> create; //기존 내용물 생성과 같은 체인이다.
    std::unique_ptr<RawAiPreparation> preparation; //같은 본체의 적 재선택을 사용한다.
    GameRandom random{0x12345678}; //다른 복사본 없이 전달할 명시 원본 난수 입력이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},product{133},invalidGraph{254}; //본체/현재 전역이다.
    std::int32_t noRegion{127},mapLimit{256};double now{16777216.25}; //현재 signed 지역/지도와 QWORD 시각이다.
    Sid technology{},temple{}; //같은 풀에 할당한 실제 부모들이다.
    AiStrategySetupHooks hooks; //선택된 원본 월드/장치 하위 입력이다.
    std::vector<std::tuple<Sid,Sid,std::uint32_t>> attached;std::vector<std::string> assertions;std::vector<std::string> trace; //실제 효과 순서의 관찰값이다.
    // 기존 모듈을 같은 저장소에 조립하며 게임/원본 실행을 대신하지 않는다.
    StrategyInput() {
        PlayerWorldHooks playerHooks;playerHooks.assertion=[](PlayerWorldAssertion) { throw std::logic_error("검사 외 Player 진단"); };playerHooks.find=[](float,float,std::uint32_t)->Sid { throw std::logic_error("검사 외 Player 찾기"); };playerHooks.create=[](std::uint32_t,std::uint32_t)->Sid { throw std::logic_error("검사 외 Player 생성"); };playerHooks.setOwner=[](Sid,std::uint32_t) { throw std::logic_error("검사 외 Player 소유자 변경"); };playerHooks.pop=[](Sid,float,float,std::uint32_t) { throw std::logic_error("검사 외 Player 공간 등록"); };playerHooks.reserveRegular=[]()->bool { throw std::logic_error("검사 외 Player 예약"); };playerHooks.addRegular=[](Sid,std::uint32_t,float) { throw std::logic_error("검사 외 Player 프로세스"); };
        playerState.playerKnown=1;player=std::make_unique<RawPlayerWorld>(pool,post,players,playerState,std::move(playerHooks));
        CarrierPreDestroyHooks findHooks;findHooks.validateDamageable=[](Sid,std::uint32_t) { throw std::logic_error("검사 외 삭제 사전검사"); };findHooks.damageablePre=findHooks.validateDamageable;findHooks.dependentFound=[] { throw std::logic_error("검사 외 종속 통지"); };finder=std::make_unique<RawCarrierPreDestroy>(pool,finderState,std::move(findHooks));
        ContainedCreateHooks createHooks;createHooks.create=[this](auto type,auto flags) { CHECK(type==6 && flags==2);const auto sid=New();pool.AllocatedBytes(sid)[10]=6;return sid; };createHooks.notifySurface=[](Sid) { throw std::logic_error("검사 외 표면 통지"); };createHooks.attach=[this](Sid form,Sid parent,std::uint32_t flags) { Link(parent,form);attached.emplace_back(form,parent,flags);trace.push_back("create"); };create=std::make_unique<RawContainedCreate>(pool,createState,std::move(createHooks));
        technology=New();temple=New();pool.AllocatedBytes(temple)[10]=87;types[87].theme=2;
        // 자기 지역/적 지역과 기술부모를 명시하고 자기 방향 동맹만 설정한다.
        for (std::uint32_t owner=0;owner<9;++owner) { Put(players,owner*180+0x78,127);allies[owner*9+owner]=1; }
        Put(players,180+0x9c,technology.value);Put(players,360+0x9c,technology.value);Put(players,180+0x78,3);Put(players,360+0x78,4);
        Field(0x260,1);Field(0x264,2);Field(0x238,3);Field(0x600,std::bit_cast<std::uint32_t>(3.25F));Field(0xaf0,0xc61c4000);Field(0xaf4,0xc61c4000);
        AiPreparationHooks prep;prep.priest=[](auto owner) { return owner==2 ? 99U : 0U; };preparation=std::make_unique<RawAiPreparation>(memory,pool,players,allies,AiPreparationGlobals{playerState.battleOwner,playerState.playerKnown,noRegion,[this] { return invalidGraph; },now},std::move(prep));
        hooks.regionBounds=[this](auto region) { CHECK(region==3);trace.push_back("bounds");return std::array<float,4>{16,32,64,96}; };hooks.temple=[this](auto region) { CHECK(region==3);trace.push_back("temple");return temple.value; };hooks.destroy=[this](Sid form,auto flags) { CHECK(flags==0);CHECK(Get(pool.Slot(technology),6,2)==form.value);Put(pool.AllocatedBytes(technology),6,Get(pool.Slot(form),4,2),2);trace.push_back("destroy"); };hooks.assertion=[this](auto text) { assertions.emplace_back(text); };
    }
    // 같은 풀의 실제 할당을0 입력으로 준비한다.
    Sid New() { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});return sid; }
    // 부모 head와 새 내용물 next를 같은 풀에서 연결한다.
    void Link(Sid parent,Sid form) { Put(pool.AllocatedBytes(form),4,Get(pool.Slot(parent),6,2),2);Put(pool.AllocatedBytes(parent),6,form.value,2); }
    // 원본 내용물 조건/패턴의 명시 입력을 만든다.
    Sid Content(Sid parent,std::uint32_t flags,std::uint32_t type,std::uint32_t pattern) { const auto sid=New();auto raw=pool.AllocatedBytes(sid);raw[10]=6;Put(raw,20,flags,2);Put(raw,18,type,2);Put(raw,12,pattern,2);Link(parent,sid);return sid; }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 해당 본체 DWORD만 바꾼다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 같은 실제 부모/내용물/난수/전역을 연결한다.
    std::unique_ptr<RawAiStrategySetup> Make() { return std::make_unique<RawAiStrategySetup>(memory,*player,*preparation,*finder,*create,finderState,types,random,allies,AiStrategySetupGlobals{playerState.battleOwner,noRegion,product,createState.bridgeType,mapLimit,now},hooks); }
};

// 적/양쪽 지역이 없을 때 하위나258/25c를 건드리지 않고 재시도를 허용한다.
TEST_CASE(AiStrategySetup_RegionPrerequisitesAndEnemyRetry) {
    StrategyInput input;Put(input.players,360+0x78,127);input.hooks={};const auto before=std::vector<std::uint8_t>(input.memory.Bytes(input.token).begin(),input.memory.Bytes(input.token).end());input.Make()->Initialize(input.token);CHECK(std::equal(before.begin(),before.end(),input.memory.Bytes(input.token).begin()) && input.attached.empty());
    input.Field(0x264,0);input.Make()->Initialize(input.token);CHECK(input.Field(0x264)==2 && !input.Field(0x258) && !input.Field(0x25c));
    Put(input.players,180+0x78,127);input.Field(0x264,1);input.Make()->Initialize(input.token);CHECK(input.Field(0x264)==2 && input.trace.empty());
}
// 자기 경계→다리→현재 신전→첫 생산→258의 실제 순서와 QWORD 기한을 확인한다.
TEST_CASE(AiStrategySetup_LocalInitializationSharesContentsAndClock) {
    StrategyInput input;input.Make()->Initialize(input.token);CHECK(input.Field(0x258)==1 && input.Field(0x25c)==0 && input.Field(0x628)==3 && input.Field(0x5f4)==3 && input.Field(0x390)==2);CHECK(std::bit_cast<float>(input.Field(0x268))==40 && std::bit_cast<float>(input.Field(0x26c))==64);CHECK(input.attached.size()==4 && input.trace==std::vector<std::string>({"bounds","create","create","create","temple","create"}));CHECK(input.Field(0x608)==0);const auto deadline=std::bit_cast<double>((std::uint64_t(input.Field(0x634))<<32)|input.Field(0x630));CHECK(deadline==16777219.5);
    const auto [form,parent,flags]=input.attached.back();CHECK(parent==input.temple && flags==0x10 && Get(input.pool.Slot(form),18,2)==133 && Get(input.pool.Slot(form),20,2)==0x10);CHECK(!input.Make()->FindProduct(input.token,133).value);
}
// 원격AI는 mask8 기술부모와mask10 신전 생산을 함께 만들고 기존mask8이 있으면 첫 생산을 생략한다.
TEST_CASE(AiStrategySetup_RemoteProductAndExistingTeleport) {
    StrategyInput input;input.Field(0x260,2);Put(input.players,360+0x78,3);Put(input.players,180+0x78,4);input.Field(0x264,1);input.Field(0x238,0);auto setup=input.Make();setup->Initialize(input.token);CHECK(input.attached.size()==2 && input.Field(0x258)==1);if (input.attached.size()==2) { CHECK(std::get<1>(input.attached[0])==input.technology && Get(input.pool.Slot(std::get<0>(input.attached[0])),20,2)==8);CHECK(std::get<1>(input.attached[1])==input.temple && Get(input.pool.Slot(std::get<0>(input.attached[1])),20,2)==0x10); }CHECK(setup->FindProduct(input.token,133).value!=0);
    const auto oldCount=input.attached.size();input.Field(0x390,3);setup->Initialize(input.token);CHECK(input.attached.size()==oldCount && input.Field(0x390)==3);
}
// 보류 중인 다리는 총수에만 포함하며 기존 패턴의 TYPE/free/dead를 임의 필터하지 않는다.
TEST_CASE(AiStrategySetup_BridgeScanCountsPendingSeparately) {
    StrategyInput input;input.Content(input.technology,0x40,99,7);const auto pending=input.Content(input.technology,0x140,98,8);input.pool.AllocatedBytes(pending)[11]=3;input.Content(input.technology,0x20,82,9);input.Make()->SeedBridges(input.token,input.technology,3);CHECK(input.attached.size()==1 && input.Field(0x628)==2 && input.Field(0x608)==7 && input.Field(0x60c)==0 && input.Field(0x5f4)==1);CHECK(Get(input.pool.Slot(std::get<0>(input.attached[0])),20,2)==0x40);
}
// 직선 강제도 매번 난수를 소비하고 serial signed 감김에서도4배수 규칙을 유지한다.
TEST_CASE(AiStrategySetup_BridgeRandomAndSignedSerial) {
    StrategyInput input;GameRandom expected{input.random.State()};std::array<std::uint32_t,5> patterns{};
    // 기대값은 원본 난수와 가중 표의 독립 호출로 만들고 생산 횟수만 명시한다.
    for (std::size_t index=0;index<patterns.size();++index) { patterns[index]=static_cast<std::uint32_t>(SelectBridgePattern(static_cast<std::int32_t>(expected.Next(10000))));if (index%4==0) patterns[index]=0; }
    input.Make()->SeedBridges(input.token,input.technology,5);CHECK(input.random.State()==expected.State() && input.attached.size()==5);
    // 실제 재고와 실제 생성 내용물의 같은 패턴을 대조한다.
    for (std::size_t index=0;index<patterns.size();++index) CHECK(input.Field(0x608+index*4)==patterns[index] && Get(input.pool.Slot(std::get<0>(input.attached[index])),12,2)==patterns[index]);
    StrategyInput wrap;wrap.Field(0x5f4,0x80000000U);wrap.Make()->SeedBridges(wrap.token,wrap.technology,1);CHECK(wrap.Field(0x608)==0 && wrap.Field(0x5f4)==0x80000001U);
}
// 원본 중복 진단을 계속 진행시킨 뒤 현재 원격 내용물을 삭제하고 새 생산을 등록한다.
TEST_CASE(AiStrategySetup_ProductAssertionsAndSelectedMissingEffects) {
    StrategyInput input;input.Field(0x260,2);const auto previous=input.Content(input.technology,8,133,0);input.Make()->AddProduct(input.token,input.temple,133);CHECK(input.assertions==std::vector<std::string>({"!haveProd( typeNum )","!haveTeleportable( typeNum )"}));CHECK(input.trace==std::vector<std::string>({"destroy","create","create"}) && input.Make()->FindProduct(input.token,133)!=previous);
    StrategyInput missing;missing.hooks.regionBounds={};CHECK(Throws([&] { missing.Make()->Initialize(missing.token); }));CHECK(!missing.Field(0x258) && missing.attached.empty());StrategyInput noTemple;noTemple.hooks.temple={};CHECK(Throws([&] { noTemple.Make()->Initialize(noTemple.token); }));CHECK(noTemple.attached.size()==3 && !noTemple.Field(0x258) && noTemple.Field(0x628)==3);
    StrategyInput overflow;CHECK(Throws([&] { overflow.Make()->SeedBridges(overflow.token,overflow.technology,9); }));CHECK(overflow.Field(0x628)==8 && overflow.attached.size()==8 && !overflow.Field(0x258));
}
// signed 비용+1200의 엄격한 경계와 목표 우선순위를 독립 기대 표로 대조한다.
TEST_CASE(AiStrategySetup_GoalPrioritiesAndOverflow) {
    StrategyInput input;auto setup=input.Make();
    // 원본 네 목표의 선택 조건을 교차 입력한다.
    for (std::uint32_t attached:{0U,1U}) {
        // 경제/공격 기능 비트의 독립 조합이다.
        for (std::uint32_t abilities:{0U,4U,16U,20U}) {
            // 비용+1200 경계 아래/같음/초과와 signed 감김을 포함한다.
            for (const auto [cost,wallet,rich]:std::array<std::tuple<std::uint32_t,std::uint32_t,bool>,4>{{{100,1299,false},{100,1300,false},{100,1301,true},{0x7fffffffU,0,true}}}) {
                // 자기/적 그래프가 같은 경우 공격 목표1은 선택되지 않는다.
                for (bool sameGraph:{false,true}) {
                    // 특정 좌표가 있더라도 목표1 또는 초기 경제 목표2가 우선한다.
                    for (bool point:{false,true}) { input.Field(0x2d0,attached);input.Field(0x248,abilities);input.Field(0x290,cost);input.Field(0x2d8,wallet);input.Field(0x298,3);input.Field(0x2ac,sameGraph ? 3U : 4U);input.Field(0xaf0,std::bit_cast<std::uint32_t>(point ? 32.0F : 0.0F));input.Field(0xaf4,std::bit_cast<std::uint32_t>(32.0F));const auto expected=attached && rich && (abilities&4) && !sameGraph ? 1U : !attached && (abilities&16) ? 2U : point ? 4U : attached && rich ? 3U : (abilities&16) ? 2U : 3U;setup->SelectGoal(input.token);CHECK(input.Field(0x2d4)==expected); }
                }
            }
        }
    }
}
// 같은 목표에서는 전체 본체를 보존하고 변경 때만 원본6DWORD를 초기화한다.
TEST_CASE(AiStrategySetup_GoalResetOnlyOnChangeAndStrictPointBounds) {
    StrategyInput input;input.Field(0x2d4,3);input.Field(0x5f0,77);input.Field(0x3cc,88);auto setup=input.Make();const auto original=std::vector<std::uint8_t>(input.memory.Bytes(input.token).begin(),input.memory.Bytes(input.token).end());setup->SelectGoal(input.token);CHECK(std::equal(original.begin(),original.end(),input.memory.Bytes(input.token).begin()));input.Field(0xaf0,std::bit_cast<std::uint32_t>(1.0F));input.Field(0xaf4,std::bit_cast<std::uint32_t>(255.0F));setup->SelectGoal(input.token);CHECK(input.Field(0x2d4)==4 && input.Field(0x5f0)==0 && input.Field(0x3cc)==0 && input.Field(0x3d0)==0xc61c4000U && input.Field(0x3d4)==0xc61c4000U && input.Field(0x3d8)==1);
    // 지도 끝·0·NaN은 특정 좌표 목표로 인정하지 않는다.
    for (float invalid:{0.0F,256.0F,std::numeric_limits<float>::quiet_NaN()}) { input.Field(0xaf0,std::bit_cast<std::uint32_t>(invalid));setup->SelectGoal(input.token);CHECK(input.Field(0x2d4)==3); }
}
}
