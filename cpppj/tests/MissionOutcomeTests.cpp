// 두 실제 PE의 승패 조건/잠금/효과 순서를 같은 명시 입력의 C++ 결과와 직접 대조한다.
#include "ConstructionSupport.h"
#include "o/RawMissionOutcome.h"
#include <set>
#include <sstream>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 독립 관찰의 입력 수와 실제 SidPool이 요구하는 서버/예측 영역 용량이다. 관찰의 사용 SID는 128 미만이다.
constexpr std::uint32_t kCases=364,kOutcomeCapacity=32768;
// DWORD 목록을 fixture와 같은 쉼표 순서로 기록한다.
template<class Values> std::string ValuesText(const Values& values) {
    std::string result;
    // 현재 요소를 그대로 읽으며 기대 상태를 계산하지 않는다.
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }
    return result;
}
// 원본 E0~188의 43개 잠금 순서를 typed 미션 상태에 대응한다.
std::array<std::uint32_t*,43> Latches(MissionOutcomeState& state) {
    std::array<std::uint32_t*,43> values{};
    // 플레이어 0의 잠금은 원본 검사에 없으며 1~8만 연결한다.
    for (std::size_t player=1;player<=kPlayerCount;++player) {
        auto& latch=state.latches[player];const auto first=(player-1)*5;
        values[first]=&latch.templeDead;values[first+1]=&latch.templeHalfDead;values[first+2]=&latch.priestDead;
        values[first+3]=&latch.priestCaptured;values[first+4]=&latch.priestSaved;
    }
    values[40]=&state.goodTeamDead;values[41]=&state.badTeamDead;values[42]=&state.failed;return values;
}
// 모든 실제 효과 시점의 현재 공유 상태를 원본과 같은 문자열로 보존한다.
std::string Snapshot(MissionOutcomeState& state,const PriestPostPopState& priests) {
    std::array<std::uint32_t,9> flags{},temples{};std::array<std::uint32_t,43> latches{};
    // 입력 플래그와 신전 번호를 슬롯 번호 순서로 기록한다.
    for (std::size_t player=0;player<flags.size();++player) { flags[player]=state.players[player].flags;temples[player]=state.players[player].temple; }
    const auto pointers=Latches(state);
    // 모든 잠금의 현재 DWORD를 기록한다. bool로 줄이지 않는다.
    for (std::size_t i=0;i<latches.size();++i) latches[i]=*pointers[i];
    return ValuesText(std::array{state.localPlayer,state.demo,state.tellBlocked})+'/'+ValuesText(flags)+'/'+ValuesText(temples)+'/'+
        ValuesText(priests.ownerState)+'/'+ValuesText(latches);
}
// trace의 마지막 빈 필드도 fixture와 비교할 수 있도록 별도 구분자를 사용한다.
void Add(std::string& trace,std::string value) { if (!trace.empty()) trace+='|';trace+=value; }
// 임의 성공/실패 조건을 계산하지 않고 명시한 callback 입력 변화만 수행한다.
void Mutate(std::string_view kind,std::uint32_t mutation,std::set<std::string>& mutated,MissionOutcomeState& state,PriestPostPopState& priests) {
    if (!mutated.insert(std::string(kind)).second) return;
    if (kind=="section" && mutation==1) { state.localPlayer=2;state.players[2].flags=0;priests.ownerState[1]=2;state.players[1].temple=0; }
    else if (kind=="close" && mutation==2) { state.tellBlocked=1;state.demo=1;priests.ownerState[1]=1; }
    else if (kind=="stop" && mutation==3) { state.latches[1].templeHalfDead=7;priests.ownerState[2]=2; }
    else if (kind=="region" && mutation==4) state.localPlayer=2;
}
}

// 두 판본 native 전체 루트 출력/사제 상태/HP 경계/동맹/효과 당시 상태를 직접 대조한다.
TEST_CASE(MissionOutcome_IndependentPeFixture) {
    std::ifstream input(NETSTORM_MISSIONOUTCOME_FIXTURE);if (!input) throw std::runtime_error("미션 승패 fixture 없음");
    std::string line;std::array<std::uint32_t,2> counts{};std::uint32_t number=0;
    // fixture의 각 사례는 새 풀/공유 상태에서 독립적으로 시작한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=19) throw std::runtime_error("미션 승패 fixture 열 수 오류");++number;
        const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");++counts[patch ? 0 : 1];
        SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kOutcomeCapacity);PriestPostPopState priests;MissionOutcomeState state;
        state.localPlayer=Number(row[2]);state.demo=Number(row[3]);state.tellBlocked=Number(row[4]);
        const auto flags=Inputs(row[9]),temples=Inputs(row[10]),statuses=Inputs(row[11]),allies=Inputs(row[12]),latches=Inputs(row[13]);
        // 번호 0의 입력도 보존해 유효 번호만 읽는지 검사한다.
        for (std::size_t player=0;player<9;++player) {
            state.players[player]={static_cast<std::uint8_t>(flags[player]),temples[player]};priests.ownerState[player]=statuses[player];
            // 동맹 표는 bool이 아닌 원본 DWORD 값이다.
            for (std::size_t other=0;other<9;++other) state.allies[player][other]=allies[player*9+other];
        }
        const auto pointers=Latches(state);
        // 원본 시작 잠금은 0뿐 아니라 비제로 다른 DWORD도 포함한다.
        for (std::size_t i=0;i<pointers.size();++i) *pointers[i]=latches[i];
        priests.priests.entries=Inputs(row[14]);priests.priests.count=static_cast<std::uint32_t>(priests.priests.entries.size());
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        // raw 필드는 기계어 입력의 폭/비트로만 작성한다.
        for (const auto& item:Split(row[15],';')) {
            const auto node=Inputs(item);auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(node[0])});
            raw[10]=static_cast<std::uint8_t>(node[1]);raw[11]=static_cast<std::uint8_t>(node[2]);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(node[3]);
            Put(raw,26,node[4],patch ? 4 : 2);Put(raw,14,node[6]);Put(raw,18,node[7]);
            types[node[1]].maxHitPoints=std::bit_cast<std::int32_t>(node[5]);types[node[1]].flags1=0x10;
        }
        SquidPostPopState bookkeeping;SquidRewardState rewardState;GameRandom random;ScrambledSpStore store(random,[]{ return 1u; });
        SquidReward reward(pool,types,bookkeeping,rewardState,patch ? &store : nullptr);
        const auto mutation=Number(row[6]);std::set<std::string> mutated;std::string trace;MissionOutcomeHooks hooks;
        // 영역 경계는 명시한 소유자만 반환한다. 호출 인자는 단정도 비트로 기록한다.
        hooks.regionOwner=[&](float x,float y) { Add(trace,"region:"+ValuesText(std::array{Bits(x),Bits(y)})+'#'+Snapshot(state,priests));Mutate("region",mutation,mutated,state,priests);return Number(row[7]); };
        // 섹션 callback 뒤 상태 변화와 최종 잠금 쓰기 순서를 원본 관찰과 비교한다.
        hooks.section=[&](std::string_view name) { Add(trace,"section:"+std::string(name)+'#'+Snapshot(state,priests));Mutate("section",mutation,mutated,state,priests);return Number(row[5])!=0; };
        hooks.closeTransientGumps=[&] { Add(trace,"close#"+Snapshot(state,priests));Mutate("close",mutation,mutated,state,priests); };
        hooks.onSection=[&] { Add(trace,"stop#"+Snapshot(state,priests));Mutate("stop",mutation,mutated,state,priests); };
        hooks.invalidPlayerAssert=[&](std::uint32_t player) { Add(trace,"assert:"+std::to_string(player)+'#'+Snapshot(state,priests)); };
        RawMissionOutcome outcome(pool,priests,state,MakeMissionOutcomeHooks(pool,reward,std::move(hooks)));std::uint32_t result=0;
        if (row[1]=="Check") outcome.Check();
        else if (row[1]=="Dead") result=outcome.PriestDead(Number(row[8]));
        else if (row[1]=="Captured") result=outcome.PriestCaptured(Number(row[8]));
        else if (row[1]=="Event") result=outcome.Event(state.goodTeamDead,Number(row[8])!=0,"BadTeamDead");
        else throw std::runtime_error("미션 fixture 동작 이름 오류");
        const auto observed=Snapshot(state,priests);
        if (observed!=row[16] || trace!=row[17] || result!=Number(row[18])) {
            std::fprintf(stderr,"missionoutcome row %u/%s/%s\n",number,row[0].c_str(),row[1].c_str());
            CHECK(observed==row[16]);CHECK(trace==row[17]);CHECK(result==Number(row[18]));
        }
    }
    CHECK(counts[0]==kCases && counts[1]==kCases);
}

// 같은 사건 callback의 재진입도 최종 latch 쓰기 전에는 실행 가능하다. 원본 호출 순서를 C++에서 별도로 확인한다.
TEST_CASE(MissionOutcome_ReentrantEventCommitsAfterCallbacks) {
    SidPool pool(OriginalEdition::Patch1078,kOutcomeCapacity);PriestPostPopState priests;MissionOutcomeState state;std::string trace;bool nested=false;
    RawMissionOutcome* current=nullptr;MissionOutcomeHooks hooks;
    hooks.maxHitPoints=[](Sid) { return 100; };hooks.regionOwner=[](float,float) { return 0u; };
    hooks.section=[&](std::string_view) {
        trace+='S';CHECK(state.badTeamDead==0);
        if (!nested) { nested=true;CHECK(current->Event(state.badTeamDead,true,"BadTeamDead"));CHECK(state.badTeamDead==1); }
        return true;
    };
    hooks.closeTransientGumps=[&] { trace+='C'; };hooks.onSection=[&] { trace+='V'; };
    RawMissionOutcome outcome(pool,priests,state,std::move(hooks));current=&outcome;
    CHECK(outcome.Event(state.badTeamDead,true,"BadTeamDead"));CHECK(trace=="SSCVCV");
    CHECK(state.badTeamDead==1);CHECK(!outcome.Event(state.badTeamDead,false,"BadTeamDead"));CHECK(state.badTeamDead==1);
}

// 연결 누락/다른 풀을 조기 거부하고 실제 HP adapter의 참조 동일성을 확인한다.
TEST_CASE(MissionOutcome_RejectsMissingHooksAndForeignReward) {
    SidPool pool(OriginalEdition::Cd1072,kOutcomeCapacity),other(OriginalEdition::Cd1072,kOutcomeCapacity);PriestPostPopState priests;MissionOutcomeState state;
    bool rejected=false;try { RawMissionOutcome invalid(pool,priests,state,{}); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    std::vector<RiftTypeRecord> types(171);SquidPostPopState bookkeeping;SquidRewardState rewardState;SquidReward reward(other,types,bookkeeping,rewardState,nullptr);
    rejected=false;try { MakeMissionOutcomeHooks(pool,reward,{}); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
