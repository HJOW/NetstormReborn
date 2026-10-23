// 실제양판본 Money/SP전수와같은월드장부·분산저장소·게임난수 연결을검사한다.
#include "RawSceneSupport.h"
#include "o/RawFortMoneyLoad.h"
#include "client/RawSurfaceWorld.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본기대SP/배열/난수를재계산하지않고fixture의그대로반환한다.
const std::vector<std::vector<std::string>>& MoneyRows() {
    static const auto rows=LoadFixture(NETSTORM_FORTMONEY_FIXTURE).rows;return rows;
}

// 실제SP/별도CD전역/9개Player raw배열/깊이와모든효과의입력저장소다.
struct MoneyScene {
    OriginalEdition edition; // 원본Player stride/SP배치를선택하는판본이다.
    SquidRewardState reward; // 기존SP/모드의공유상태다.
    SquidPostPopState players; // 현재localOwner가있는원본장부다.
    std::vector<std::uint8_t> records; // 현재9개Player의실제stride 바이트배열이다.
    std::uint32_t depth{}; // 같은로딩깊이DWORD다.
    FortMoneyState state; // 현재guard/client/시작SP/99회검사전역이다.
    GameRandom random; // 분산SP가실제로소비하는게임난수다.
    ScrambledSpStore store; // 현재입력의lazy/초기화분산SP저장소다.
    std::vector<std::string> events; // 실제assert/SP표시의순서다.
    std::vector<std::uint32_t> depths; // 동일월드의시작/끝깊이전달을확인한다.
    std::uint32_t assertions{}; // 원본assert보고횟수다.
    std::uint32_t kind{}; // wrapper/패치지정/스트림본문 중선택한원본진입점이다.
    std::int32_t owner{},amount{}; // 패치직접지정의signed입력이다.
    FortMoneyCursor stream; // 현재원본pos/end DWORD다.
    std::unique_ptr<RawFortMoneyLoad> money; // 위공유자료만참조하는생산Money 모듈이다.
    // 독립Native입력의동일seed/fill·raw모드를구성한다. 반환기대값은계산하지않는다.
    MoneyScene(OriginalEdition selected,const std::vector<std::string>& row)
        :edition(selected),records(9U*(selected==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)),
         store(random,[seed=Number(Split(row[1],',')[14])] { return seed; },Number(Split(row[1],',')[15])) {
        const auto values=Split(row[1],',');CHECK(values.size()==19);
        kind=Number(values[0]);reward.fixedSpA=Number(values[1])!=0;reward.halfHealthMode=Number(values[2])!=0;
        state.economyBlocked=Number(values[3])!=0;state.clientActive=Number(values[4])!=0;reward.fixedSpB=Number(values[5])!=0;
        players.localOwner=static_cast<std::uint8_t>(Number(values[6]));state.difficulty=static_cast<std::uint8_t>(Number(values[7]));
        state.cdStartingSp=static_cast<std::int32_t>(std::stol(values[8]));depth=Number(values[10]);
        stream={Number(values[11]),Number(values[12])};state.templatePresent=Number(values[13])!=0;state.spCheckCount=17;
        owner=std::bit_cast<std::int32_t>(Number(values[17]));amount=std::bit_cast<std::int32_t>(Number(values[18]));
        const auto stride=selected==OriginalEdition::Patch1078 ? 0xb4U : 0xacU;
        // 9개Player의존재여부를추정하지않고원본flagsBYTE 입력만쓴다.
        for (std::size_t index=0;index<9;++index) records[index*stride+0x50]=static_cast<std::uint8_t>((Number(values[9])&(1U<<index)) ? 0x20U : 0U);
        reward.cdLocalSp=123.5f;
        if (selected==OriginalEdition::Patch1078 && Number(values[16])) store.Initialize();
        money=std::make_unique<RawFortMoneyLoad>(selected,reward,players,records,depth,state,
            selected==OriginalEdition::Patch1078 ? &store : nullptr,FortMoneyHooks{
                [this](FortMoneyAssertion code) { ++assertions;events.push_back(code==FortMoneyAssertion::MissingTemplate ? "A:T" : "A:P"); },
                [this](std::uint32_t value) { depths.push_back(value); },[this] { events.push_back("U"); }});
    }
};

// native의전체288DWORD 분산배열/논리SP/현재난수·진단을양판본독립입력과대조한다.
void ReplayMoney(const char* editionName) {
    const auto edition=std::string_view(editionName)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    std::size_t count=0; // 현재판본에서실제로대조한fixture 행수다.
    // 각원본입력을생산모듈의동일진입점으로실행한다.
    for (const auto& row:MoneyRows()) {
        if (row[0]!=editionName) continue;CHECK(row.size()==12);
        MoneyScene scene(edition,row);const auto before=scene.records;
        if (scene.kind==1) scene.money->SetPlayerSp(scene.owner,scene.amount);
        else if (scene.kind==2) scene.money->ReadMoney(scene.stream);
        else scene.money->Load(scene.stream);
        std::string values="-",pools="-",events;
        if (edition==OriginalEdition::Patch1078 && scene.store.Initialized()) {
            values.clear();std::array<std::uint8_t,32*9*4> raw{};
            // 각owner의실제SP Get을읽으며이미초기화된저장소의난수는소비하지않는다.
            for (std::uint32_t owner=0;owner<9;++owner) { if (owner) values+=',';values+=std::to_string(scene.store.Get(owner)); }
            // 원본288 DWORD 순서대로현재전체물리배열을읽는다.
            for (std::size_t index=0;index<scene.store.Pools().size();++index) Put(raw,index*4,scene.store.Pools()[index]);
            pools=Hex(raw);
        } else if (edition==OriginalEdition::Cd1072) values=std::to_string(std::bit_cast<std::uint32_t>(scene.reward.cdLocalSp));
        // 실제효과목록은원본의순서와함께비교한다.
        for (const auto& event:scene.events) { if (!events.empty()) events+=';';events+=event; }
        if (events.empty()) events="-";
        const bool same=scene.depth==Number(row[2]) && scene.stream.position==Number(row[3]) && scene.state.spCheckCount==Number(row[4]) &&
            scene.store.Initialized()==(Number(row[5])!=0) && scene.store.Offset()==Number(row[6]) && scene.random.State()==Number(row[7]) &&
            values==row[8] && pools==row[9] && events==row[10] && scene.assertions==Number(row[11]);
        CHECK(same);CHECK(scene.records==before);++count;
        if (!same) { std::printf("%s Money 행%zu %s 기대SP %s 실제SP %s\n",editionName,count,row[1].c_str(),row[8].c_str(),values.c_str());break; }
    }
    CHECK(count==(edition==OriginalEdition::Patch1078 ? 244U : 164U));
}
}

// 패치wrapper/8플레이어순서/직접SP지정·분산배열·공유난수를독립native와전수대조한다.
TEST_CASE(FortMoneyLoad_ReplaysPatchNative) { ReplayMoney("originals"); }
// CD저장4바이트소비/시작SP·로컬flags 정책과별도float전역을전수대조한다.
TEST_CASE(FortMoneyLoad_ReplaysCdNative) { ReplayMoney("originalCD"); }

// 실제RawSurfaceWorld의장부와같은store/random을참조하여현재owner/모드가즉시반영됨을확인한다.
TEST_CASE(FortMoneyLoad_SharesCurrentWorldOwnerStoreAndRandom) {
    constexpr auto edition=OriginalEdition::Patch1078; // 패치SP 저장을선택하는현재판본이다.
    std::vector<RiftTypeRecord> types(188); // Money가읽지않는world형식의타입표이며실제자산대조로세지않는다.
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({})); // Money소비자에필요한정상길이의미사용프레임표다.
    std::vector<SquidDisplayShape> shapes(188); // 이검사에서는렌더/Pop을실행하지않는다.
    BridgeConnectState bridges; // 동일월드연결의실제타입번호이며이검사에서표면을생성하지않는다.
    bridges.bridgeType=82;bridges.islandType=94;bridges.stalagType=95;bridges.connectorType=155;bridges.noIslandType=157;
    netstorm::client::RawSurfaceWorld world(edition,types,frames,shapes,bridges,97);
    auto core=world.Core();SquidRewardState reward;FortMoneyState state;GameRandom random;
    ScrambledSpStore store(random,[] { return 42U; });
    std::vector<std::uint8_t> records(9U*0xb4U);std::uint32_t depth=3,refreshes=0;
    state.clientActive=true;core.bookkeeping.localOwner=1;
    RawFortMoneyLoad money(edition,reward,core.bookkeeping,records,depth,state,&store,{
        [](FortMoneyAssertion) { throw std::logic_error("정상Money 소비자의진단"); },
        [&world](std::uint32_t value) { world.SetLoadingDepth(value); },[&refreshes] { ++refreshes; }});
    money.SetPlayerSp(1,7000);CHECK(store.Get(1)==std::bit_cast<std::uint32_t>(7000.0f));CHECK(refreshes==1);
    core.bookkeeping.localOwner=2;reward.fixedSpB=true;
    money.SetPlayerSp(2,3000);CHECK(store.Get(2)==std::bit_cast<std::uint32_t>(194162.0f));CHECK(refreshes==2);
    CHECK(random.State()!=0);CHECK(depth==3);
}

// 진단에서 현재 섹션을 바꾸면 새 getter는 증가한 깊이에서 변경된 cursor를 소비하고 실패 때 깊이를 복구한다.
TEST_CASE(FortMoneyLoad_SelectsAfterDepthAndTemplateDiagnostic) {
    SquidRewardState reward;reward.fixedSpA=true;SquidPostPopState players;players.localOwner=1;FortMoneyState state;state.templatePresent=false;
    GameRandom random(12345);ScrambledSpStore sp(random,[] { return 12345U; });std::vector<std::uint8_t> records(9*0xb4);std::uint32_t depth=3;
    FortMoneyCursor first{17,4},second{19,4};auto* current=&first;std::vector<std::string> effects;
    RawFortMoneyLoad money(OriginalEdition::Patch1078,reward,players,records,depth,state,&sp,{
        [&](FortMoneyAssertion reason) { CHECK(reason==FortMoneyAssertion::MissingTemplate && depth==4);effects.push_back("A");current=&second;state.templatePresent=true; },
        [&](auto value) { CHECK(value==depth);effects.push_back("L"+std::to_string(value)); },[] { throw std::logic_error("고정 Fort SP의 예상 밖 표시"); }});
    money.LoadSelected([&]() -> FortMoneyCursor& { CHECK(depth==4 && current==&second);effects.push_back("S4");current->position=0;return *current; });
    CHECK(effects==std::vector<std::string>({"L4","A","S4","L3"}) && first.position==17 && second.position==4 && depth==3);
    CHECK(sp.Get(1)==std::bit_cast<std::uint32_t>(194162.0f));effects.clear();
    CHECK(Throws([&] { money.LoadSelected([&]() -> FortMoneyCursor& { CHECK(depth==4);effects.push_back("SF");throw std::runtime_error("선택 파일 실패 입력"); }); }));
    CHECK(effects==std::vector<std::string>({"L4","SF","L3"}) && depth==3);effects.clear();
    CHECK(Throws([&] { money.LoadSelected({}); }) && effects.empty() && depth==3);
}
