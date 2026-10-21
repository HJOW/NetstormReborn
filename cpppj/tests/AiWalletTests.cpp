// 실제 Player/본체와 SP 부모의 지갑 가산을 검사하도록 작성했다. 구동 중단 지시에 따라 실행하지 않는다.
#include "o/RawAiWallet.h"
#include "o/SquidReward.h"
#include "RawSceneSupport.h"
#include <bit>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 조회는현재Player포인터의signed지갑만읽으며데모/SP/본문바이트를변경하지않는다.
TEST_CASE(AiWallet_BalanceUsesCurrentBrainWithoutDemoOrWrites) {
    std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory{0x4f000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x5a}); }};const auto first=memory.Allocate(RawAiBrainConstructor::kBytes),second=memory.Allocate(RawAiBrainConstructor::kBytes);Put(players,2*180+0xb0,first);Put(memory.Bytes(first),0x2d8,0x80000001U);Put(memory.Bytes(second),0x2d8,1200);std::size_t demos{};RawAiWallet wallet(memory,players,{[] { return false; },[&] { ++demos;return true; }});const auto before=std::vector<std::uint8_t>(memory.Bytes(first).begin(),memory.Bytes(first).end());CHECK(wallet.Balance(2)==std::bit_cast<std::int32_t>(0x80000001U));Put(players,2*180+0xb0,second);CHECK(wallet.Balance(2)==1200 && !demos && std::equal(before.begin(),before.end(),memory.Bytes(first).begin()));Put(players,2*180+0xb0,0);CHECK(wallet.Balance(2)==0 && !demos);
}
// 모드우선/비양수owner/NULLbrain과손상된물리포인터를구별한다.실행하지않는다.
TEST_CASE(AiWallet_BalanceDisabledSignedOwnerAndPhysicalBounds) {
    std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory{0x4f000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }};bool disabled=true;std::size_t modes{};Put(players,180+0xb0,0xdeadbeefU);RawAiWallet wallet(memory,players,{[&] { ++modes;return disabled; },[]()->bool { throw std::logic_error("조회에서데모호출금지"); }});CHECK(wallet.Balance(1)==0 && modes==1);disabled=false;CHECK(wallet.Balance(0)==0 && wallet.Balance(0xffffffffU)==0 && wallet.Balance(0x80000000U)==0 && wallet.Balance(8)==0);CHECK(Throws([&] { wallet.Balance(9); }) && Throws([&] { wallet.Balance(1); }));
}

// 비활성·signed 비양수·NULL brain은 지갑과 데모 경계를 선택하지 않는다.
TEST_CASE(AiWallet_DisabledSignedOwnerAndNullBrain) {
    std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory(0x10000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); });bool disabled=true;std::size_t modeReads{},demoReads{}; // 원본의 두 현재 읽기 위치를 분리한 입력이다.
    RawAiWallet wallet(memory,players,{[&] { ++modeReads;return disabled; },[&] { ++demoReads;return false; }});
    Put(players,180+0xb0,0xdead0000);wallet.Credit(1,42);CHECK(modeReads==1 && !demoReads && memory.Allocations().empty());disabled=false;
    // signed DWORD 비양수는 표 밖 주소로 해석하지 않는다.
    for (const auto owner:{0U,0xffffffffU,0x80000000U}) wallet.Credit(owner,42);
    wallet.Credit(8,42);CHECK(modeReads==5 && !demoReads);CHECK(Throws([&] { wallet.Credit(9,42); }));CHECK(Throws([&] { wallet.Credit(1,42); }));CHECK(!demoReads);
}

// 독립 기대 DWORD로 signed 감김/데모 하한과 본체 전체의 나머지 바이트 보존을 확인한다.
TEST_CASE(AiWallet_WrapDemoFloorAndUnrelatedBytes) {
    std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory(0x10000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); });const auto token=memory.Allocate(RawAiBrainConstructor::kBytes);Put(players,180+0xb0,token);bool demo{};std::size_t demoReads{}; // 실제 등록 본체와 Player 포인터만 만든 입력이다.
    RawAiWallet wallet(memory,players,{[] { return false; },[&] { ++demoReads;return demo; }});
    constexpr std::array<std::array<std::uint32_t,4>,8> rows{{{0x7fffffff,1,0,0x80000000},{0xffffffff,2,0,1},{1200,0xfffffff6,1,1190},{1191,0,1,1191},{0x80000000,0,1,1190},{0x7fffffff,1,1,1190},{0,0xffffffff,0,0xffffffff},{1201,0,1,1201}}}; // 초기값/가산 비트/데모/기대값을 원본 분기에서 명시한 입력이다.
    // 각 행은 계산기를 기대값으로 쓰지 않고 본체 전체와 Player 표를 직접 대조한다.
    for (const auto& row:rows) {
        auto bytes=memory.Bytes(token);Put(bytes,0x2d8,row[0]);auto expected=std::vector<std::uint8_t>(bytes.begin(),bytes.end());Put(expected,0x2d8,row[3]);const auto before=players;demo=row[2]!=0;
        wallet.Credit(1,std::bit_cast<std::int32_t>(row[1]));CHECK(std::equal(expected.begin(),expected.end(),memory.Bytes(token).begin()) && players==before);
    }
    CHECK(demoReads==8 && memory.Allocations().size()==1 && memory.Frees().empty());
}

// 하위 미션 조회가 Player/지갑을 바꾸면 현재 값을 읽되 저장한 brain 포인터는 유지한다.
TEST_CASE(AiWallet_DemoBoundaryRereadsSavedBrain) {
    std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory(0x10000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{}); });const auto first=memory.Allocate(RawAiBrainConstructor::kBytes),second=memory.Allocate(RawAiBrainConstructor::kBytes);Put(players,180+0xb0,first);Put(memory.Bytes(first),0x2d8,7);Put(memory.Bytes(second),0x2d8,5000);std::size_t reads{}; // 현재 포인터 변경을 관측하기 위한 두 실제 본체다.
    RawAiWallet wallet(memory,players,{[] { return false; },[&] { if (++reads!=1) return false;CHECK(Get(memory.Bytes(first),0x2d8)==8);Put(players,180+0xb0,second);Put(memory.Bytes(first),0x2d8,0x80000001);return true; }});
    wallet.Credit(1,1);CHECK(Get(memory.Bytes(first),0x2d8)==1190 && Get(memory.Bytes(second),0x2d8)==5000 && Get(players,180+0xb0)==second);
    wallet.Credit(1,20);CHECK(Get(memory.Bytes(first),0x2d8)==1190 && Get(memory.Bytes(second),0x2d8)==5020 && reads==2);
}

// 호스트 물리 계약은 크기 없는/해제된 포인터를 지갑 성공으로 대신하지 않는다.
TEST_CASE(AiWallet_RejectsMissingPhysicalBodyAndReaders) {
    std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory(0x10000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{}); });const AiWalletReads reads{[] { return false; },[] { return true; }}; // 실제 getter 묶음과 잘못된 크기 입력이다.
    CHECK(Throws([&] { RawAiWallet invalid(memory,std::span(players).first(1619),reads); }));CHECK(Throws([&] { RawAiWallet invalid(memory,players,{}); }));
    RawAiWallet wallet(memory,players,reads);const auto small=memory.Allocate(0x2dc);Put(players,180+0xb0,small);CHECK(Throws([&] { wallet.Credit(1,5); }) && !Get(memory.Bytes(small),0x2d8));memory.Free(small);CHECK(Throws([&] { wallet.Credit(1,5); }));
}

// 보상 부모는 실제 SP/현재 brain/표시를 함께 갱신하고 값형 부착/지갑 배열은 읽거나 복사하지 않는다.
TEST_CASE(AiWallet_RewardUsesSameBrainAndKeepsLegacySeparate) {
    std::array<std::uint8_t,1620> players{};RawAiBrainMemory memory(0x10000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{}); });const auto token=memory.Allocate(RawAiBrainConstructor::kBytes);Put(players,180+0xb0,token);Put(memory.Bytes(token),0x2d8,777);RawAiWallet wallet(memory,players,{[] { return false; },[] { return false; }}); // 실제 지갑은777이고 독립 배열은 다른 값을 가진다.
    SidPool pool(OriginalEdition::Patch1078,32768);std::vector<RiftTypeRecord> types(188);SquidPostPopState bookkeeping;SquidRewardState state;state.aiWallet.fill(9000);state.aiMirrorDisabled=true;state.walletClamp=true;GameRandom random(123);ScrambledSpStore store(random,[] { return 100U; });store.Set(1,std::bit_cast<std::uint32_t>(1000.0F));std::size_t refreshes{};
    SquidReward reward(pool,types,bookkeeping,state,&store,{{},[&] { ++refreshes;CHECK(Get(memory.Bytes(token),0x2d8)==877);CHECK(std::bit_cast<float>(store.Get(1))==1100.75F); }});
    reward.BindPatchWallet([&](auto owner,auto amount) { wallet.Credit(owner,amount); });reward.AddSp(1,100.75F);CHECK(refreshes==1 && !bookkeeping.aiAttached[1] && state.aiWallet[1]==9000); // 가산 float는 SP에 그대로, 정수 절삭100만 실제 지갑에 전달한다.
    const auto randomBefore=random.State();reward.CreditWallet(1,-4);CHECK(Get(memory.Bytes(token),0x2d8)==873 && random.State()==randomBefore && refreshes==1 && std::bit_cast<float>(store.Get(1))==1100.75F);
    CHECK(Throws([&] { reward.BindPatchWallet({}); }));reward.CreditWallet(1,1);CHECK(Get(memory.Bytes(token),0x2d8)==874);
    SidPool cdPool(OriginalEdition::Cd1072,32768);std::vector<RiftTypeRecord> cdTypes(171);SquidPostPopState cdBook;cdBook.aiAttached[1]=true;SquidRewardState cdState;cdState.aiWallet[1]=77;SquidReward cd(cdPool,cdTypes,cdBook,cdState,nullptr);CHECK(Throws([&] { cd.BindPatchWallet([](auto,auto) {}); }));cd.CreditWallet(1,5);CHECK(cdState.aiWallet[1]==82);
}
