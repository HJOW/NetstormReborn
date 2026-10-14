// 실제 초기화/helper/CRT 결과를 같은 게임/CRT/mode 전체DWORD에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGameRandomSeed.h"
using namespace netstorm::test::rawscene;

// mode/Unix시각/이전CRT/임시mode 교차의 독립 원본 입력을 직접 비교한다.
TEST_CASE(CampaignSeed_ReplaysPatchNativeStates) {
    std::size_t count=0;
    // 기대 계산을 재현하지 않고 원본의 전체 세 상태와 실제 시각 요청 수를 비교한다.
    for (const auto& row:LoadFixture(NETSTORM_CAMPAIGN_SEED_FIXTURE).rows) {
        GameRandom game(0x12345678);auto mode=Number(row[1]),crt=Number(row[3]);std::uint32_t calls=0;
        RawGameRandomSeed seed(game,crt,mode,[&] { ++calls;CHECK(mode!=0);return static_cast<std::int32_t>(std::stoll(row[2])); });
        const auto value=Number(row[0]) ? seed.Reseed() : seed.Initialize();
        CHECK(mode==Number(row[4]) && game.State()==Number(row[5]) && value==Number(row[5]) && crt==Number(row[6]));
        CHECK(calls==Number(row[7]) && Number(row[8])==104);++count;
    }
    CHECK(count==96);
}

// 같은 원본 게임 재시드를 실제 SP 초기화 callback에 연결하고 모드/상태 수명을 확인한다.
TEST_CASE(CampaignSeed_ComposesActualSpInitialization) {
    GameRandom game;std::uint32_t crt=7,mode=0,calls=0;
    RawGameRandomSeed seed(game,crt,mode,[&] { ++calls;CHECK(mode==1);return 946684800; });
    ScrambledSpStore sp(game,[&] { return seed.Reseed(); });sp.Initialize();
    CHECK(sp.Initialized() && mode==0 && calls==1);
    const auto state=game.State();sp.Initialize();CHECK(calls==2 && game.State()==state);
    CHECK(Throws([&] { RawGameRandomSeed missing(game,crt,mode,{}); }));
    RawGameRandomSeed failing(game,crt,mode,[]()->std::int32_t { throw std::runtime_error("명시시각실패"); });
    CHECK(Throws([&] { failing.Reseed(); }) && mode==0);
}
