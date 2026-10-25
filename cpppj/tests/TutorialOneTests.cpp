// 실제 PE 입력의 전체 표/단계 호출 순서를 원본 집계와 Early1 판정기에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawTotalMade.h"
#include "o/RawTutorialOne.h"
#include <limits>
#include <map>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 실행기에 전달한 독립 카운터 입력을 같은 DWORD로 준비한다. 기대 결과를 계산하지 않는다.
std::uint32_t InputCount(std::size_t index,std::size_t kind) { return static_cast<std::uint32_t>((index+1)*(0x1020304U+kind*17U)); }
// fixture의 원본 float32 바이트를 반올림하지 않고 읽는다.
float InputFloat(std::string_view text) {
    std::uint32_t result=0;
    // 네 little endian 바이트를 원래 순서로 조합한다.
    for (std::size_t i=0;i<4;++i) result|=std::uint32_t(std::stoul(std::string(text.substr(i*2,2)),nullptr,16))<<(8*i);
    return std::bit_cast<float>(result);
}
// 같은 실제 네 표의 전체1024바이트를 원본 fixture와 비교할 문자열로 만든다.
std::string TableHex(std::span<const std::uint32_t> table) {
    std::vector<std::uint8_t> bytes(table.size()*4); // 호스트 endian과 무관한 원본 직렬화 자료다.
    // 현재 입력/결과의 모든DWORD를 주변 바이트와 섞지 않고 기록한다.
    for (std::size_t i=0;i<table.size();++i) Put(bytes,i*4,table[i]);
    return Hex(bytes);
}
}
// PATCH/CD 각각52고유 입력에서 전체 카운터/허용 표와 반환·assert 횟수를 대조한다.
TEST_CASE(TutorialOne_TotalMadeNative104) {
    std::map<std::string,std::size_t> count; // 판본별 고유 입력 수를 검증한다.
    // 원본 독립 결과를 읽으며 검사 코드에서 getter의 기대 분기를 재계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_TUTORIAL_ONE_FIXTURE).rows) {
        if (row[0]!="Total") continue;CHECK(row.size()==11);
        const auto edition=row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        SquidPostPopState state;std::array<std::uint32_t,256> restrictions{};
        // Native에 전달한 같은 TYPE 부류 입력을 준비한다.
        for (std::size_t i=0;i<types.size();++i) types[i].flags2=1U<<(i%6);
        // 같은 공유 장부와 허용 표를 독립 입력값으로 채운다.
        for (std::size_t i=0;i<256;++i) {
            state.localCounts[i]=InputCount(i,0);state.localSecondaryCounts[i]=InputCount(i,1);
            state.globalCounts[i]=InputCount(i,2);restrictions[i]=InputCount(i,3);
        }
        std::uint32_t assertions=0;RawTotalMade totals(edition,types,state,restrictions,[&](TotalMadeAssertion) { ++assertions; });
        const auto operation=Number(row[2]);const auto first=static_cast<std::int32_t>(std::stoll(row[3]));
        const auto second=static_cast<std::int32_t>(std::stoll(row[4]));std::uint32_t result=0;
        switch (operation) {
        case 0: totals.AllowAll(first);break;
        case 1: totals.Allow(first,second);break;
        case 2: result=totals.Allowed(first);break;
        case 3: totals.ResetCounts();break;
        case 4: result=totals.Local(first);break;
        case 5: result=totals.Built(first);break;
        case 6: result=totals.Global(first);break;
        case 7: result=std::bit_cast<std::uint32_t>(totals.BuiltForGenus(static_cast<std::uint32_t>(first)));break;
        default: CHECK(false);break;
        }
        CHECK(result==Number(row[5]));CHECK(assertions==Number(row[6]));
        CHECK(TableHex(state.localCounts)==row[7]);CHECK(TableHex(state.localSecondaryCounts)==row[8]);
        CHECK(TableHex(state.globalCounts)==row[9]);CHECK(TableHex(restrictions)==row[10]);++count[row[1]];
    }
    CHECK(count["originals"]==52);CHECK(count["originalCD"]==52);
}
// Native104의 실제 reset 행을 Session 없는 helper와 공유 장부 두 경로에 각각 재생한다.
TEST_CASE(TutorialOne_TotalMadeSharedResetAndDirectHelperReplayNative104) {
    std::map<std::string,std::size_t> resets; // PATCH/CD에서 실제 operation3 행을 각각 선택했는지 확인한다.
    for (const auto& row:LoadFixture(NETSTORM_TUTORIAL_ONE_FIXTURE).rows) { // 기존 기대 fixture의 전체 표를 그대로 비교한다.
        if (row[0]!="Total" || Number(row[2])!=3) continue;CHECK(row.size()==11);
        const auto edition=row[1]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);RawTotalMadeStorage input;std::array<std::uint32_t,256> restrictions{}; // 원본 입력과 같은 크기의 독립 물리다.
        for (std::size_t index=0;index<256;++index) { // 256번째 물리 칸에도 다른 비트를 주어 보존 범위를 확인한다.
            input.localCounts[index]=InputCount(index,0);input.localSecondaryCounts[index]=InputCount(index,1);input.globalCounts[index]=InputCount(index,2);restrictions[index]=InputCount(index,3);
        }
        auto storage=input;const auto* local=storage.localCounts.data();const auto* secondary=storage.localSecondaryCounts.data();const auto* global=storage.globalCounts.data(); // reset 전후 주소도 같은 물리다.
        ResetTotalMadeCounts(storage);CHECK(TableHex(storage.localCounts)==row[7] && TableHex(storage.localSecondaryCounts)==row[8] && TableHex(storage.globalCounts)==row[9]);
        CHECK(storage.localCounts.data()==local && storage.localSecondaryCounts.data()==secondary && storage.globalCounts.data()==global && TableHex(restrictions)==row[10]);
        storage=input;SquidPostPopState counts{.sharedTotalMade=&storage};counts.graphsEnabled=true;counts.localOwner=3;counts.depth=7;counts.totalCost=91;counts.productionDirty=13;counts.providers={{9,10},1};counts.ownerFactories[1]={{11,12},1}; // 세 표와 무관한 현재 제어/목록 입력이다.
        const auto fallbackLocal=counts.localCounts,fallbackSecondary=counts.localSecondaryCounts,fallbackGlobal=counts.globalCounts;std::uint32_t assertions{}; // 미사용 독립 배열도 reset의 대상이 아니다.
        RawTotalMade totals(edition,types,counts,restrictions,[&](TotalMadeAssertion) { ++assertions; });const SquidPostPopState& current=counts;
        CHECK(counts.LocalCounts().data()==local && current.LocalCounts().data()==local && counts.LocalSecondaryCounts().data()==secondary && current.LocalSecondaryCounts().data()==secondary && counts.GlobalCounts().data()==global && current.GlobalCounts().data()==global);
        totals.ResetCounts();CHECK(TableHex(storage.localCounts)==row[7] && TableHex(storage.localSecondaryCounts)==row[8] && TableHex(storage.globalCounts)==row[9]);
        CHECK(assertions==Number(row[6]) && TableHex(restrictions)==row[10] && counts.localCounts==fallbackLocal && counts.localSecondaryCounts==fallbackSecondary && counts.globalCounts==fallbackGlobal);
        CHECK(counts.graphsEnabled && counts.localOwner==3 && counts.depth==7 && counts.totalCost==91 && counts.productionDirty==13 && counts.providers.count==1 && counts.providers.entries==std::vector<std::uint32_t>({9,10}) && counts.ownerFactories[1].count==1 && counts.ownerFactories[1].entries==std::vector<std::uint32_t>({11,12}));
        ++resets[row[1]];
    }
    CHECK(resets["originals"]>0 && resets["originalCD"]>0);
}

// 실제 PATCH266고유 입력에서 단락평가/반환/다중Graph/SP 포함 경계와NaN을 대조한다.
TEST_CASE(TutorialOne_StageNative266) {
    std::size_t count=0; // 동일 fixture의 단계 입력 수만 센다.
    // 기대 호출 순서는 실제 switch 명령이 기록한 문자열을 직접 소비한다.
    for (const auto& row:LoadFixture(NETSTORM_TUTORIAL_ONE_FIXTURE).rows) {
        if (row[0]!="Stage") continue;CHECK(row.size()==9);
        auto stage=std::bit_cast<std::int32_t>(Number(row[1]));std::uint32_t local=1,shown=Number(row[4]),home=0x730000,reclaim=0x123456;
        std::vector<std::uint8_t> players(9*0xb4);Put(players,0xb4+0x7c,55);
        std::map<std::uint32_t,std::uint32_t> graphs;SquidPostPopList geysers;
        // Native의 같은 현재Graph와 실제 목록 순서를 입력한다.
        for (const auto& item:Split(row[6],',')) {
            const auto pair=Split(item,':');const auto sid=Number(pair[0]);graphs[sid]=Number(pair[1]);
            if (sid!=55) geysers.entries.push_back(sid);
        }
        geysers.count=static_cast<std::uint32_t>(geysers.entries.size());
        std::string trace; // 실제 호출한 경계 순서를 보존한다.
        const auto append=[&](std::string text) { if (!trace.empty()) trace+=',';trace+=text; };
        TutorialOneHooks hooks;
        hooks.builtForGenus=[&](std::uint32_t genus) { CHECK(genus==4);append("B");return static_cast<std::int32_t>(std::stoll(row[3])); };
        hooks.graphFor=[&](std::uint32_t sid) { append("G"+std::to_string(sid));return graphs.at(sid); };
        hooks.money=[&](std::uint32_t owner) { append("M"+std::to_string(owner));return InputFloat(row[5]); };
        hooks.advance=[&] { append("A"); };hooks.spawnGeysers=[&] { append("S"); };
        RawTutorialOne tutorial({stage,local,shown,home,reclaim},players,geysers,std::move(hooks));
        CHECK(tutorial.Handle(Number(row[2]))==Number(row[7]));CHECK((trace.empty() ? "-" : trace)==row[8]);++count;
    }
    CHECK(count==266);
}
// 상태/목록을 복제하지 않고 callback 뒤 현재 count/허용 표/장부를 사용하는지 확인한다.
TEST_CASE(TutorialOne_SharedStateAndMutableList) {
    std::vector<RiftTypeRecord> types(188);types[82].flags2=4;
    SquidPostPopState counts;std::array<std::uint32_t,256> restrictions{};
    RawTotalMade totals(OriginalEdition::Patch1078,types,counts,restrictions);
    totals.Allow(82,0);CHECK(!totals.Allowed(82));totals.Allow(82,1);CHECK(totals.Allowed(82));
    counts.localSecondaryCounts[82]=19;CHECK(totals.BuiltForGenus(4)==19);
    std::int32_t stage='D';std::uint32_t local=1,shown=1,home=123,reclaim=456;
    std::vector<std::uint8_t> players(9*0xb4);Put(players,0xb4+0x7c,55);
    SquidPostPopList geysers{{80,81,82},3};std::uint32_t advances=0,lookups=0;
    TutorialOneHooks hooks;
    hooks.builtForGenus=[&](std::uint32_t genus) { return totals.BuiltForGenus(genus); };
    hooks.graphFor=[&](std::uint32_t) { ++lookups;return 2U; };
    hooks.money=[](std::uint32_t) { return 600.0F; };
    hooks.advance=[&] { ++advances;stage='E';geysers.count=1; };
    hooks.spawnGeysers=[] { CHECK(false); };
    RawTutorialOne tutorial({stage,local,shown,home,reclaim},players,geysers,std::move(hooks));
    CHECK(tutorial.Handle(0)==0);CHECK(stage=='E');CHECK(advances==1);CHECK(lookups==2);
    CHECK(tutorial.Handle(0)==0);CHECK(advances==2);
}
