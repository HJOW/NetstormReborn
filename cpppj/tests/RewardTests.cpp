// 삭제/해체 보상(SP)과 SP 저장소·난수를 저장된 독립 기계어 결과와 비교한다.
#include "TestSupport.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidReward.h"
#include "o/TerrainBuilder.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 풀 크기와 합성 객체의 SID는 기계어 생성기와 같다.
constexpr std::uint32_t kCapacity=32768;
constexpr Sid kSid{6};
// 원문 TSV/콤마 항목을 순서대로 분리한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin=0;
    // 마지막 빈 항목도 보존한다.
    for (;;) {
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==std::string_view::npos ? end : end-begin));
        if (end==std::string_view::npos) return result;
        begin=end+1;
    }
}
// 기계어 출력만 읽으므로 일반 CTest에는 원본 PE/Python이 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_REWARD_FIXTURE);
        if (!input) throw std::runtime_error("보상 fixture 없음");
        std::vector<std::vector<std::string>> result;std::string line;
        // 주석은 실행 입력이 아니다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();return rows;
}
// 지정 폭의 little endian raw 필드를 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 호스트 정렬/엔디언을 입력에 섞지 않는다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 콤마 항목을 부호 없는 정수 목록으로 읽는다.
std::vector<std::uint64_t> Numbers(const std::string& text) {
    std::vector<std::uint64_t> values;
    // 음수는 DWORD로 감긴 표현이 아니라 부호 있는 10진수이므로 별도 변환한다.
    for (const auto& item:Split(text,',')) values.push_back(static_cast<std::uint64_t>(std::stoll(item)));
    return values;
}
// Python zlib와 같은 전체 버퍼 Adler-32다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 표준 소수와 중간 넘침을 막는 누적 폭이다.
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;
    for (auto byte:bytes) { a=(a+byte)%prime;b=(b+a)%prime; }
    return static_cast<std::uint32_t>((b<<16)|a);
}
// 저장소의 32개 배열을 물리 순서대로 직렬화한다.
std::uint32_t PoolsAdler(const ScrambledSpStore& store) {
    std::vector<std::uint8_t> bytes;
    // 각 DWORD를 낮은 바이트부터 넣는다.
    for (auto value:store.Pools()) for (int i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    return Adler(bytes);
}
// 기계어 입력 한 줄을 C++ 상태로 만들고 보상을 실행해 모든 관찰 값을 비교한다.
void Replay(const std::vector<std::string>& row) {
    CHECK(row.size()==30);
    const bool patch=row[0]=="originals";
    const auto edition=patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
    for (Sid sid:{Sid{5},Sid{6},Sid{7}}) CHECK(pool.Allocate(2)==sid);
    // 객체 타입과 두 보조 타입 기록은 생성기와 같은 순서로 덮어쓴다.
    const auto number=static_cast<std::size_t>(std::stoul(row[1]));
    const std::array<std::pair<std::size_t,std::string>,3> records{{{number,row[2]},{147,row[3]},{164,row[4]}}};
    for (const auto& [index,text] : records) {
        const auto v=Numbers(text);auto& type=types[index];
        type.cost=std::bit_cast<float>(static_cast<std::uint32_t>(v[0]));type.maxHitPoints=static_cast<std::int32_t>(v[1]);
        type.flags1=static_cast<std::uint32_t>(v[2]);type.flags2=static_cast<std::uint32_t>(v[3]);type.group=10;
    }
    auto raw=pool.AllocatedBytes(kSid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
    raw[10]=static_cast<std::uint8_t>(number);raw[11]=static_cast<std::uint8_t>(std::stoul(row[5]));
    raw[patch ? 40 : 35]=static_cast<std::uint8_t>(std::stoul(row[6]));
    Put(raw,26,static_cast<std::uint32_t>(std::stoll(row[7])),patch ? 4 : 2);
    SquidPostPopState bookkeeping;bookkeeping.localOwner=static_cast<std::uint8_t>(std::stoul(row[9]));
    const auto aiMask=std::stoul(row[14]);
    for (std::size_t owner=0;owner<bookkeeping.aiAttached.size();++owner) bookkeeping.aiAttached[owner]=(aiMask>>owner)&1;
    SquidRewardState state;const auto flags=std::stoul(row[13]);
    state.percent=static_cast<std::uint32_t>(std::stoll(row[11]));state.geyserPool=static_cast<std::int32_t>(std::stoll(row[12]));
    state.fixedSpA=flags&1;state.fixedSpB=flags&2;state.halfHealthMode=flags&4;state.aiMirrorDisabled=flags&8;
    state.walletClamp=flags&0x10;state.priestQuarterHp=flags&0x20;state.debugAsserts=patch && (flags&0x40);
    const auto wallets=Numbers(row[15]);
    for (std::size_t i=0;i<wallets.size();++i) state.aiWallet[i]=static_cast<std::int32_t>(wallets[i]);
    // 패치판은 같은 재시드/채움 입력으로 실제 저장 함수와 같은 호출 순서의 초기 SP를 만든다.
    const auto seed=static_cast<std::uint32_t>(std::stoull(row[17])),fill=static_cast<std::uint32_t>(std::stoull(row[18]));
    GameRandom rng(0);std::unique_ptr<ScrambledSpStore> store;
    const auto sp=Numbers(row[16]);
    if (patch) {
        store=std::make_unique<ScrambledSpStore>(rng,[seed] { return seed; },fill);
        for (std::uint32_t owner=0;owner<ScrambledSpStore::kOwners;++owner) store->Set(owner,static_cast<std::uint32_t>(sp[owner]));
        CHECK(rng.State()==std::stoull(row[23]));CHECK(PoolsAdler(*store)==std::stoull(row[24]));
    } else state.cdLocalSp=std::bit_cast<float>(static_cast<std::uint32_t>(sp[0]));
    // 파생 개수는 입력 목록을 호출 순서대로 돌려주고 base이면 훅 없이 상태 비트를 쓴다.
    std::vector<std::int32_t> script;std::uint32_t uiCalls=0;
    if (row[19]!="base") for (auto value:Numbers(row[19])) script.push_back(static_cast<std::int32_t>(value));
    std::size_t cursor=0;SquidRewardHooks hooks;
    hooks.refreshSp=[&uiCalls] { ++uiCalls; };
    if (row[19]!="base") hooks.virtualCount=[&](Sid) { CHECK(cursor<script.size());return script.at(cursor++); };
    SquidReward reward(pool,types,bookkeeping,state,store.get(),hooks);
    reward.Refund(kSid,static_cast<std::uint32_t>(std::stoul(row[8])),row[10]=="1");
    CHECK(static_cast<std::uint32_t>(state.geyserPool)==std::stoull(row[20]));
    const auto expectedWallets=Numbers(row[21]);CHECK(expectedWallets.size()==state.aiWallet.size());
    for (std::size_t i=0;i<expectedWallets.size();++i) CHECK(static_cast<std::uint32_t>(state.aiWallet[i])==static_cast<std::uint32_t>(expectedWallets[i]));
    const auto expectedSp=Numbers(row[22]);
    if (patch) {
        CHECK(expectedSp.size()==ScrambledSpStore::kOwners);
        for (std::uint32_t owner=0;owner<ScrambledSpStore::kOwners;++owner) CHECK(store->Get(owner)==static_cast<std::uint32_t>(expectedSp[owner]));
        CHECK(rng.State()==std::stoull(row[25]));CHECK(PoolsAdler(*store)==std::stoull(row[26]));CHECK(store->Offset()==std::stoull(row[27]));
    } else { CHECK(expectedSp.size()==1);CHECK(std::bit_cast<std::uint32_t>(state.cdLocalSp)==static_cast<std::uint32_t>(expectedSp[0])); }
    CHECK(uiCalls==std::stoull(row[28]));CHECK(reward.VirtualCountCalls()==std::stoull(row[29]));
}
// 판본 이름으로 고른 행을 모두 재생한다.
void ReplayEdition(std::string_view name,std::size_t expectedMinimum) {
    std::size_t count=0;
    // 같은 장면 생성기를 세 PE가 공유한다.
    for (const auto& row:Fixture()) { if (row[0]==name) { Replay(row);++count; } }
    CHECK(count>=expectedMinimum);
}
// 명시적인 C++ 보호 예외를 원본 assert 실행 없이 확인한다.
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
}

// 패치 10.78의 실제 기계어 결과를 재생한다.
TEST_CASE(reward_patch_x86_fixture) { ReplayEdition("originals",1000); }
// CD 10.72 배포본의 실제 기계어 결과를 재생한다.
TEST_CASE(reward_cd_x86_fixture) { ReplayEdition("originalCD",1000); }
// 추가 10.37 실행 파일은 CD와 같은 코드 배치지만 별도 PE의 출력을 재생한다.
TEST_CASE(reward_1037_x86_fixture) { ReplayEdition("original1037",1000); }

// 전역 난수는 지형 생성기가 쓰는 같은 선형 합동식이며 0 상태와 0 limit을 구별한다.
TEST_CASE(reward_game_random_matches_terrain_generator) {
    GameRandom random(0);std::uint32_t terrain=0;
    // 처음 256번의 결과와 상태 전진이 같다.
    for (int i=0;i<256;++i) { CHECK(random.Next(97)==static_cast<std::uint32_t>(TerrainBuilder::Next(terrain,97)));CHECK(random.State()==terrain); }
    CHECK(Throws([&] { random.Next(0); }));
}
// 저장소는 소유자 범위와 재시드 콜백을 변경 전에 거부하고, 값 비트는 항상 되읽힌다.
TEST_CASE(reward_sp_store_roundtrip_and_guards) {
    GameRandom random(123);
    ScrambledSpStore missing(random,{},0);CHECK(Throws([&] { missing.Get(0); }));CHECK(!missing.Initialized());
    ScrambledSpStore store(random,[] { return 0x12345678U; },0xcdcdcdcdU);
    CHECK(Throws([&] { store.Get(9); }));CHECK(Throws([&] { store.Set(9,1); }));CHECK(!store.Initialized());
    // 세 값과 모든 소유자가 서로 간섭하지 않는다.
    for (std::uint32_t owner=0;owner<9;++owner) store.Set(owner,0x80000001U+owner*0x01010101U);
    for (std::uint32_t owner=0;owner<9;++owner) CHECK(store.Get(owner)==0x80000001U+owner*0x01010101U);
    store.Set(4,0xffffffffU);CHECK(store.Get(4)==0xffffffffU);CHECK(store.Get(3)==0x80000001U+3*0x01010101U);
}
// 보상 입력 오류는 상태를 바꾸기 전에 거부한다.
TEST_CASE(reward_rejects_bad_inputs_before_writing) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);
    pool.Allocate(2);auto raw=pool.AllocatedBytes({5});raw[10]=82;types[82].cost=100.0f;
    SquidPostPopState bookkeeping;SquidRewardState state;GameRandom rng(0);
    ScrambledSpStore store(rng,[] { return 1U; });
    CHECK(Throws([&] { SquidReward bad(pool,types,bookkeeping,state,nullptr); }));
    SquidReward reward(pool,types,bookkeeping,state,&store);
    CHECK(Throws([&] { reward.Refund({5},9,false); }));
    CHECK(Throws([&] { reward.Refund({0},1,false); }));
    CHECK(!store.Initialized());CHECK(state.geyserPool==0);
    SidPool cd(OriginalEdition::Cd1072,kCapacity,true);std::vector<RiftTypeRecord> cdTypes(171);
    CHECK(Throws([&] { SquidReward bad(cd,cdTypes,bookkeeping,state,&store); }));
}

namespace {
// 보상과 장부는 생성 시 타입을 복사하므로 비용을 넣은 표를 먼저 만든다.
std::vector<RiftTypeRecord> SceneTypes() {
    std::vector<RiftTypeRecord> types(188);types[82].cost=100.0f;return types;
}
// 삭제 장부와 보상이 같은 풀/장부를 쓰는 최소 장면이다. 실제 공간 해제는 하지 않고 직접 pre/post만 실행한다.
struct LifecycleScene {
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    SquidUnpop unpop;
    RawSquidDestroy destroy;
    SquidPostPopState bookkeeping;
    SquidDeletionState deletion;
    SquidRewardState rewardState;
    GameRandom rng;
    ScrambledSpStore store;
    std::uint32_t uiCalls{};
    SquidReward reward;
    SquidDestroyLifecycle lifecycle;
    // 소유자 1의 보통 객체(타입 82, 개수 2)와 소유자 0의 SP 0, 소유자 3의 SP 1000을 준비한다.
    LifecycleScene()
        :pool(OriginalEdition::Patch1078,kCapacity,true),types(SceneTypes()),unpop(pool,hash,spots),destroy(pool,unpop,types),rng(0),
         store(rng,[] { return 77U; },0xcdcdcdcdU),
         reward(pool,types,bookkeeping,rewardState,&store,SquidRewardHooks{{},[this] { ++uiCalls; }}),
         lifecycle(pool,types,bookkeeping,deletion,destroy,SquidDeletionHooks{{},{}},nullptr,&reward) {
        for (Sid sid:{Sid{5},Sid{6},Sid{7}}) CHECK(pool.Allocate(2)==sid);
        auto raw=pool.AllocatedBytes({5});std::fill(raw.begin(),raw.end(),std::uint8_t{});
        raw[10]=82;raw[11]=0x40;raw[34]=1;
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
        bookkeeping.localOwner=2;bookkeeping.depth=3;
        store.Set(0,std::bit_cast<std::uint32_t>(0.0f));store.Set(3,std::bit_cast<std::uint32_t>(1000.0f));
    }
    // 두 훅을 순서대로 호출한다.
    void Run(std::uint32_t flags) { lifecycle.PreDestroy({5},flags);lifecycle.PostDestroy({5},flags); }
    float Sp(std::uint32_t owner) { return std::bit_cast<float>(store.Get(owner)); }
};
}
// 삭제 flags의 수신자 필드가 실제 보상 계산으로 이어져 SP와 샘 풀에 반영된다.
TEST_CASE(reward_lifecycle_pays_recipient_and_updates_geyser_pool) {
    LifecycleScene scene;
    scene.Run(3U<<16);
    // 비용 100×(개수 2+1)=300, 25% 지급은 75다.
    CHECK(scene.Sp(3)==1075.0f);CHECK(scene.rewardState.geyserPool==225);CHECK(scene.uiCalls==1);
    CHECK(scene.bookkeeping.totalCost==-100); // postDestroy는 타입 단가 하나만 차감한다.
}
// 보상 없음 flag는 SP/샘 풀을 건드리지 않고, 수신자 0은 지급액 0이지만 샘 풀에 전체 비용이 들어간다.
TEST_CASE(reward_lifecycle_flags_gate_refund) {
    LifecycleScene noRefund;
    noRefund.Run((3U<<16)|0x200000U);
    CHECK(noRefund.Sp(3)==1000.0f);CHECK(noRefund.rewardState.geyserPool==0);CHECK(noRefund.uiCalls==0);
    LifecycleScene nobody;
    nobody.Run(0);
    CHECK(nobody.rewardState.geyserPool==300);CHECK(nobody.uiCalls==1);CHECK(nobody.Sp(0)==0.0f);
}
// 다른 풀/장부를 쓰는 보상은 연결할 수 없고, 계산할 수 없는 수신자는 삭제 장부 쓰기 전에 거부된다.
TEST_CASE(reward_lifecycle_rejects_mismatched_or_bad_recipient) {
    LifecycleScene scene;SquidPostPopState other;
    SquidReward foreign(scene.pool,scene.types,other,scene.rewardState,&scene.store);
    CHECK(Throws([&] { SquidDestroyLifecycle bad(scene.pool,scene.types,scene.bookkeeping,scene.deletion,scene.destroy,SquidDeletionHooks{},nullptr,&foreign); }));
    const auto before=scene.bookkeeping.totalCost;
    CHECK(Throws([&] { scene.lifecycle.PreDestroy({5},9U<<16); }));
    CHECK(scene.bookkeeping.totalCost==before);CHECK(scene.rewardState.geyserPool==0);
}
