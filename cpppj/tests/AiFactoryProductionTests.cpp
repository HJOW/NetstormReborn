// AI 원소별 기술 선택/시설 생산 큐의 합성 검사다. 작성·컴파일만 하며 실행하지 않는다.
#include "o/RawAiFactoryProduction.h"
#include "RawSceneSupport.h"
#include <map>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 본체/목록/기술·생산 내용물과 명시 하위 효과를 준비한다.
struct FactoryAiInput {
    RawAiBrainMemory memory{0x47000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //실제 소유 블록의 명시0 입력이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},invalidGraph{254}; //현재 본체/무효 그래프다.
    SidPool pool{OriginalEdition::Patch1078,32768,true};std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //같은 기술/시설 원문의 풀과 TYPE다.
    ContainedFinderState finder;Sid parent{},factory{},other{},firstTech{}; //원본 내용물 커서와 실제 부모/시설/기술이다.
    double now{16777216.25};AiFactoryProductionHooks hooks; //현재 QWORD 시각과 선택 효과다.
    std::map<std::uint32_t,Sid> products;std::vector<std::uint32_t> queries;std::vector<std::string> trace; //현재 생산 입력과 실제 호출 순서다.
    std::uint32_t added{},upgrades{},levels{},logs{}; //외부 효과의 실제 호출 횟수다.
    // 각 큐의 실제 backing과 원소가 같은 첫 기술을 준비한다.
    FactoryAiInput() {
        parent=New();factory=New();other=New();pool.AllocatedBytes(factory)[10]=85;pool.AllocatedBytes(other)[10]=85;types[85].theme=3;types[100].theme=3;types[101].theme=3;types[102].theme=1;
        Field(0x260,2);Field(0x294,100);Field(0x298,7);Field(0x2cc,1);Field(0x2d8,1200);Field(0x228,std::bit_cast<std::uint32_t>(2.5F));Field(0x30c,memory.Allocate(16));Field(0x310,4);Field(0x314,1);Put(memory.Bytes(Field(0x30c)),0,factory.value);Field(0x318,memory.Allocate(16));Field(0x31c,4);firstTech=Content(parent,100,4);
        hooks.technology=[this](auto owner) { CHECK(owner==Field(0x260));trace.push_back("technology");return parent; };hooks.product=[this](auto brain,auto type) { CHECK(brain==token);queries.push_back(type);const auto found=products.find(type);return found==products.end() ? Sid{} : found->second; };
        hooks.addProduct=[this](auto brain,Sid target,auto type) { CHECK(brain==token && target==factory);trace.push_back("add");++added;products[type]=Content(parent,type,8);Content(target,type,16); };hooks.level=[this](Sid target) { CHECK(target==factory);++levels;trace.push_back("level");return std::uint32_t(pool.Slot(target)[11]>>5); };hooks.upgrade=[this](Sid target) { CHECK(target==factory);++upgrades;trace.push_back("upgrade"); };hooks.log=[this](auto brain,AiFactoryAction,std::uint32_t) { CHECK(brain==token);++logs;trace.push_back("log"); };
    }
    // 실제 할당 슬롯을 명시0으로 준비한다.
    Sid New() { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});return sid; }
    // 같은 풀의 부모 head에 기술/생산 내용물 입력을 연결한다.
    Sid Content(Sid owner,std::uint32_t type,std::uint32_t flags) { const auto sid=New();auto raw=pool.AllocatedBytes(sid);raw[10]=6;Put(raw,18,type,2);Put(raw,20,flags,2);Put(raw,4,Get(pool.Slot(owner),6,2),2);Put(pool.AllocatedBytes(owner),6,sid.value,2);return sid; }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 선택된 본체 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 목록의 실제 칸을 읽는다.
    std::uint32_t Item(std::size_t pointer,std::size_t index) const { return Get(memory.Bytes(Field(pointer)),index*4); }
    // 현재 QWORD 생각 기한을 읽는다.
    double Time(std::size_t at) const { return std::bit_cast<double>((std::uint64_t(Field(at+4))<<32)|Field(at)); }
    // 같은 저장소/현재 참조를 빌리는 시설 생산 몸체다.
    RawAiFactoryProduction Make() { return RawAiFactoryProduction(memory,pool,types,finder,AiFactoryProductionGlobals{now,[this] { return invalidGraph; }},hooks); }
};

// 원소/flags4/현재 생산 보유를 원래 내용물 순서대로 검사한다.
TEST_CASE(AiFactoryProduction_FirstMatchingKnownUnregisteredType) {
    FactoryAiInput input;input.Content(input.parent,102,4);input.Content(input.parent,101,0);CHECK(input.Make().SelectTechnology(input.token,3)==100 && input.queries==std::vector<std::uint32_t>({100}));input.products[100]=input.firstTech;CHECK(input.Make().SelectTechnology(input.token,3)==0);input.types[102].theme=3;CHECK(input.Make().SelectTechnology(input.token,3)==102);
}
// battery mode0/1/2/기타의 본래 원소·중립3 치환을 교차 확인한다.
TEST_CASE(AiFactoryProduction_BatteryThemeReplacementModes) {
    FactoryAiInput input;input.types[100].theme=1;input.types[100].flags2=0x800;
    // 원본 치환 필드의 네 값을 현재 본체에 각각 공급한다.
    for (std::uint32_t mode=0;mode<4;++mode) {
        input.Field(0x3b8,mode);
        // 본래 원소/중립/다른 원소에서 첫 기술 선택을 대조한다.
        for (std::int32_t theme:{1,3,2}) { const bool matches=mode==0 ? theme==1 : mode==1 ? theme==3 : mode==2 ? theme==1 || theme==3 : false;CHECK(input.Make().SelectTechnology(input.token,theme)==(matches ? 100U : 0U)); }
    }
}
// 현재 후보의 링크가 생산 조회에서 바뀌어도 커서가 먼저 저장한 next는 유지한다.
TEST_CASE(AiFactoryProduction_FinderKeepsSavedNextAcrossProductEffects) {
    FactoryAiInput input;const auto newer=input.Content(input.parent,101,4);input.hooks.product=[&](auto brain,auto type) { CHECK(brain==input.token);input.queries.push_back(type);if (type==101) { Put(input.pool.AllocatedBytes(newer),4,0,2);return newer; }return Sid{}; };CHECK(input.Make().SelectTechnology(input.token,3)==100 && input.queries==std::vector<std::uint32_t>({101,100}));
}
// 한 종류 추가 후 슬롯이 남으면 같은 첫 시설을 미처리 큐에 유지한다.
TEST_CASE(AiFactoryProduction_AddOneProductKeepsFactoryWithRoom) {
    FactoryAiInput input;input.Field(0xb00,1);input.Make().Manage(input.token);CHECK(input.added==1 && input.levels==1 && input.Field(0x314)==1 && input.Field(0x320)==0 && input.logs==1 && input.Make().CountProducts(input.factory)==1 && input.Time(0x5a8)==input.now+2.5 && input.Time(0x5b0)==input.now+3.5);
}
// 가득 찬 시설의 모든 중복을 압축하고 현재 level/남은 기술로 업그레이드 대기만 추가한다.
TEST_CASE(AiFactoryProduction_FullFactoryMovesToUpgradeQueue) {
    FactoryAiInput input;input.Content(input.parent,101,4);input.Content(input.factory,99,16);Put(input.memory.Bytes(input.Field(0x30c)),4,input.factory.value);Put(input.memory.Bytes(input.Field(0x30c)),8,input.other.value);Put(input.memory.Bytes(input.Field(0x30c)),12,0xdeadbeef);input.Field(0x314,3);input.Make().Manage(input.token);CHECK(input.added==1 && input.levels==2 && input.Field(0x314)==1 && input.Item(0x30c,0)==input.other.value && input.Item(0x30c,3)==0xdeadbeef && input.Field(0x320)==1 && input.Item(0x318,0)==input.factory.value);
    FactoryAiInput maxed;maxed.pool.AllocatedBytes(maxed.factory)[11]=64;maxed.Content(maxed.parent,101,4);maxed.Content(maxed.factory,90,16);maxed.Content(maxed.factory,91,16);maxed.Content(maxed.factory,92,16);maxed.Make().Manage(maxed.token);CHECK(maxed.added==1 && maxed.Field(0x314)==0 && maxed.Field(0x320)==0);
}
// 후보가 없으면 큐를 정리하되 행동 기한/업그레이드를 합성하지 않는다.
TEST_CASE(AiFactoryProduction_NoCandidateAndDuplicateProductCounting) {
    FactoryAiInput input;input.products[100]=input.firstTech;input.Make().Manage(input.token);CHECK(input.Field(0x314)==0 && !input.added && !input.upgrades && input.Time(0x5a8)==0);input.Content(input.factory,100,16);const auto duplicate=input.Content(input.factory,100,0x110);input.pool.AllocatedBytes(duplicate)[11]=3;input.Content(input.factory,100,8);CHECK(input.Make().CountProducts(input.factory)==2);
}
// 미처리0/충족/지갑1200/업그레이드 대기와 원본 요청→제거→기한의 순서다.
TEST_CASE(AiFactoryProduction_UpgradeGatesAndRequestOrder) {
    FactoryAiInput input;input.Field(0x314,0);input.Field(0x320,2);Put(input.memory.Bytes(input.Field(0x318)),0,input.factory.value);Put(input.memory.Bytes(input.Field(0x318)),4,input.factory.value);input.Field(0x2d8,1199);input.Make().Manage(input.token);CHECK(!input.upgrades && input.Field(0x320)==2);input.Field(0x2d8,1200);input.hooks.upgrade=[&](Sid sid) { CHECK(sid==input.factory && input.Field(0x320)==2 && input.Time(0x5a8)==0);++input.upgrades; };input.Make().Manage(input.token);CHECK(input.upgrades==1 && input.Field(0x320)==0 && input.Time(0x5a8)==input.now+2.5);
}
// 실제 선택된 미복원 업그레이드/누락 등록은 앞선 큐를 소비하지 않은 채 실패한다.
TEST_CASE(AiFactoryProduction_SelectedMissingAndPhysicalContracts) {
    FactoryAiInput missing;missing.hooks.addProduct={};CHECK(Throws([&] { missing.Make().Manage(missing.token); }));CHECK(missing.Field(0x314)==1 && missing.Time(0x5a8)==0);FactoryAiInput upgrade;upgrade.Field(0x314,0);upgrade.Field(0x320,1);Put(upgrade.memory.Bytes(upgrade.Field(0x318)),0,upgrade.factory.value);upgrade.hooks.upgrade={};CHECK(Throws([&] { upgrade.Make().Manage(upgrade.token); }));CHECK(upgrade.Field(0x320)==1 && upgrade.Time(0x5a8)==0);
    FactoryAiInput inactive;inactive.Field(0x294,0);inactive.hooks={};inactive.Make().Manage(inactive.token);inactive.Field(0x294,100);inactive.Field(0x298,254);inactive.Make().Manage(inactive.token);CHECK(inactive.trace.empty());const auto small=inactive.memory.Allocate(4);CHECK(Throws([&] { inactive.Make().SelectTechnology(small,3); }));
}
// 완료 시설 중복을 허용하며 TYPE의 음수 비용을 반올림 없이 절삭한다.
TEST_CASE(AiFactoryProduction_UpgradedRequeuesAndSubtractsTypeCost) {
    FactoryAiInput input;input.types[85].cost=500.75F;input.hooks.demo=[] { return false; };input.Make().Upgraded(input.token,input.factory);CHECK(input.Field(0x314)==2 && input.Item(0x30c,1)==input.factory.value && input.Field(0x2d8)==700);input.Make().Upgraded(input.token,input.factory);CHECK(input.Field(0x314)==3 && input.Item(0x30c,2)==input.factory.value && input.Field(0x2d8)==200);
}
// 목록이 가득 차도 지갑 처리는 계속하며 데모 판정 뒤 현재 지갑에1190하한을 적용한다.
TEST_CASE(AiFactoryProduction_UpgradedFullQueueAndCurrentDemoWallet) {
    FactoryAiInput input;input.Field(0x314,4);input.types[85].cost=1500.75F;input.hooks.demo=[&] { CHECK(input.Field(0x314)==4 && std::bit_cast<std::int32_t>(input.Field(0x2d8))==-300);input.Field(0x2d8,1400);return true; };input.Make().Upgraded(input.token,input.factory);CHECK(input.Field(0x314)==4 && input.Field(0x2d8)==1400);input.hooks.demo=[] { return true; };input.Make().Upgraded(input.token,input.factory);CHECK(input.Field(0x2d8)==1190);
}
}
