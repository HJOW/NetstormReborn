// 단위865: PATCH 정수 비용 캐시와 현재 TYPE float/플래그를 별도 입력으로 구별한다. 원본 비용 fixture는 변경하지 않는다.
#include "RawSceneSupport.h"
#include "o/SquidReward.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// 이 파일의 작은 명시 입력 장면만 숨긴다.
namespace {
// 현재 TYPE와 최초 로딩 표를 분리해 후처리/보상에 같은 비용 캐시를 연결하는 장면이다.
struct LoadedCostScene {
    SidPool pool; // 실제 슬롯 풀이다.
    GameRandom random; // PATCH SP 구성에만 쓰며 비용 조회는 난수를 소비하지 않는다.
    ScrambledSpStore store; // PATCH 보상 모듈 구성용 저장소다.
    SquidPostPopState book; // 현재 후처리의 실제 장부다.
    SquidRewardState mode; // HP/보상 입력이다.
    std::vector<RiftTypeRecord> live; // HP/플래그를 수정하는 현재 표다.
    std::vector<RiftTypeRecord> loaded; // 대여 전에 준비하고 이후 변경하지 않는 최초 비용표다.
    Sid source; // 같은 표의 TYPE74를 가지는 실제 슬롯이다.
    // 최초 독립 비용3.75·로딩 비용7.75와 TYPE75의11.9를 명시 입력으로 준비한다.
    explicit LoadedCostScene(OriginalEdition edition=OriginalEdition::Patch1078)
        :pool(edition,kCapacity,false),store(random,[] { return 1U; }),live(edition==OriginalEdition::Patch1078 ? 188 : 171),loaded(live.size()),source(pool.Allocate(2)) {
        live[74].cost=3.75F;live[74].flags1=0x10;live[74].maxHitPoints=200;loaded=live;loaded[74].cost=7.75F;loaded[75].cost=11.9F;
        auto raw=pool.AllocatedBytes(source);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x501dd0);raw[10]=74;raw[edition==OriginalEdition::Patch1078 ? 34 : 32]=1;
        book.localOwner=1;book.graphsEnabled=false;
    }
    // 실제 caller가 증가한 깊이를 공통 부모가 한 번 감소하도록 호출한다.
    void Run(SquidPostPop& post) { ++book.depth;post.PostPopBase(source,0x2000001); }
};
}
TEST_CASE(LoadedCostTable_PatchSnapshotSurvivesCurrentNanAndReadsCurrentRegistrationFlags) {
    LoadedCostScene scene;SquidPostPop post(scene.pool,scene.live,scene.book);post.BindCurrentTypes(scene.live);scene.book.providers.entries.resize(1);
    scene.live[74].cost=std::numeric_limits<float>::quiet_NaN();scene.live[74].flags1|=0x10000000U;scene.Run(post);
    CHECK(scene.book.TotalCost()==3 && scene.book.providers.Count()==1 && scene.book.GlobalCounts()[74]==1 && scene.book.depth==0);
}
TEST_CASE(LoadedCostTable_PostAndRewardShareLoadedCostsWithoutChangingCurrentHp) {
    LoadedCostScene scene;SquidPostPop post(scene.pool,scene.live,scene.book);SquidReward reward(scene.pool,scene.live,scene.book,scene.mode,&scene.store);
    post.BindCurrentTypes(scene.live);post.BindLoadedCosts(scene.loaded);reward.BindCurrentTypes(scene.live);reward.BindLoadedCosts(scene.loaded);
    scene.live[74].cost=999;scene.live[74].maxHitPoints=400;scene.Run(post);
    CHECK(scene.book.TotalCost()==7 && reward.TypeCostUnits(74)==7 && reward.MaxHitPoints(scene.source)==400);
    scene.pool.AllocatedBytes(scene.source)[10]=75;scene.live[75].cost=777;scene.Run(post);CHECK(scene.book.TotalCost()==18 && reward.TypeCostUnits(75)==11);
}
TEST_CASE(LoadedCostTable_NewExecutorsKeepOriginalLoaderInsteadOfMutatedFloat) {
    LoadedCostScene scene;
    // 두 실행기를 순서대로 새로 만들어도 같은 불변 로딩 표가 비용 원천이다.
    for (const auto currentCost:{1000.0F,2000.0F}) {
        scene.live[74].cost=currentCost;SquidPostPop post(scene.pool,scene.live,scene.book);SquidReward reward(scene.pool,scene.live,scene.book,scene.mode,&scene.store);
        post.BindCurrentTypes(scene.live);post.BindLoadedCosts(scene.loaded);reward.BindCurrentTypes(scene.live);reward.BindLoadedCosts(scene.loaded);
        scene.Run(post);CHECK(reward.TypeCostUnits(74)==7);
    }
    CHECK(scene.book.TotalCost()==14 && scene.book.depth==0);
}
TEST_CASE(LoadedCostTable_InvalidRebindKeepsPreviousCacheAndEmptyRestoresOwnedSnapshot) {
    LoadedCostScene scene;SquidPostPop post(scene.pool,scene.live,scene.book);SquidReward reward(scene.pool,scene.live,scene.book,scene.mode,&scene.store);
    post.BindLoadedCosts(scene.loaded);reward.BindLoadedCosts(scene.loaded);const auto wrong=std::span<const RiftTypeRecord>(scene.loaded).first(187);
    CHECK(Throws([&] { post.BindLoadedCosts(wrong); }));CHECK(Throws([&] { reward.BindLoadedCosts(wrong); }));scene.Run(post);CHECK(scene.book.TotalCost()==7 && reward.TypeCostUnits(74)==7);
    post.BindLoadedCosts({});reward.BindLoadedCosts({});scene.Run(post);CHECK(scene.book.TotalCost()==10 && reward.TypeCostUnits(74)==3);
    LoadedCostScene cd(OriginalEdition::Cd1072);SquidPostPop cdPost(cd.pool,cd.live,cd.book);SquidReward cdReward(cd.pool,cd.live,cd.book,cd.mode,nullptr);
    CHECK(Throws([&] { cdPost.BindLoadedCosts(cd.loaded); }));CHECK(Throws([&] { cdReward.BindLoadedCosts(cd.loaded); }));
}
