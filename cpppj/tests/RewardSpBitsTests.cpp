// 실제 두 SP 가산 부모의 큰 유한/무한대/NaN/서브노멀 비트와 wallet 변환을 재생한다.
#include "RawSceneSupport.h"
#include "o/SquidReward.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 기존 보상 기대값을 바꾸지 않고 새 원본 관측의 저장 float/정수 지급·장치 순서를 비교한다.
TEST_CASE(RewardSpBits_ReplaysNativeLocalAndOtherOwnerIncludingZero) {
    const auto fixture=LoadFixture(NETSTORM_SP_ADD_BITS_FIXTURE);std::uint32_t checked{};
    for (const auto& row:fixture.rows) { // 소유자0/1/3과 두 x87 관측을 같은 원본 입력 비트로 구분한다.
        if (row[0]=="entry") continue;
        SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);SquidPostPopState book;SquidRewardState state;
        const auto owner=Number(row[1]),fixed=Number(row[2]);state.fixedSpA=(fixed&1)!=0;state.fixedSpB=(fixed&2)!=0;
        GameRandom random(1);ScrambledSpStore store(random,[] { return 77U; });store.Set(owner,Number(row[3]));std::vector<std::string> trace;std::uint32_t mirror{};
        SquidReward reward(pool,types,book,state,&store,{{},[&] { trace.push_back("refresh"); }});
        reward.BindPatchWallet([&](auto selected,auto amount) { CHECK(selected==owner);mirror=std::bit_cast<std::uint32_t>(amount);trace.push_back("wallet"); });
        reward.AddSp(owner,std::bit_cast<float>(Number(row[4])));
        if (store.Get(owner)!=Number(row[6])) std::fprintf(stderr,"SP 비트행%u input%08x fixed%u expected%08x actual%08x\n",checked,Number(row[3]),fixed,Number(row[6]),store.Get(owner));
        CHECK(store.Get(owner)==Number(row[6]) && mirror==Number(row[7]));CHECK(trace==std::vector<std::string>({"wallet","refresh"}) && row[8]=="get;set;wallet;refresh");++checked;
    }
    CHECK(checked==504);
}
