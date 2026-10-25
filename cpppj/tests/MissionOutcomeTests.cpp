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

// 기존 native 입력 전체 또는 callback/방향별동맹16개만 실제 raw Player/span backend로 실행한다.
void CheckOutcomeFixture(bool live) {
    std::ifstream input(NETSTORM_MISSIONOUTCOME_FIXTURE);if (!input) throw std::runtime_error("미션 승패 fixture 없음");
    std::string line;std::array<std::uint32_t,2> counts{};std::uint32_t number=0;std::set<std::string> liveAlliesSeen;
    // fixture의 각 사례는 새 풀/공유 상태에서 독립적으로 시작한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=19) throw std::runtime_error("미션 승패 fixture 열 수 오류");++number;
        if (live && Number(row[6])==0) {
            if (row[1]!="Check" || row[2]!="1" || row[3]!="0" || row[4]!="0" || row[5]!="1") continue;
            const auto pairs=Inputs(row[12]); // 단방향0/비영 조합마다 기존 native 사례 하나만 추가 대조한다.
            if ((pairs[11]!=0 && pairs[11]!=7) || (pairs[19]!=0 && pairs[19]!=9) ||
                !liveAlliesSeen.insert(row[0]+':'+std::to_string(pairs[11])+':'+std::to_string(pairs[19])).second) continue;
        }
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
        std::vector<std::uint8_t> records(9U*(patch ? 0xb4U : 0xacU));std::array<std::uint32_t,81> currentAllies{};
        std::uint32_t local=state.localPlayer,demo=state.demo,blocked=state.tellBlocked;
        // 조회 callback의 관찰용 view만 만들며 실제 outcome 입력은 계속 raw span에서 읽는다.
        const auto current=[&] {
            auto view=state;if (!live) return view;view.localPlayer=local;view.demo=demo;view.tellBlocked=blocked;
            // 각 기록의 실제 폭/필드만 관찰한다. outcome state의 입력 사본은 읽지 않는다.
            for (std::size_t p=0;p<9;++p) {
                view.players[p]={records[p*(patch ? 0xb4U : 0xacU)+0x50],Get(records,p*(patch ? 0xb4U : 0xacU)+(patch ? 0x7cU : 0x74U))};
                // 방향별 원본 DWORD를 관찰용 문자열에만 복사한다.
                for (std::size_t other=0;other<9;++other) view.allies[p][other]=currentAllies[p*9+other];
            }
            return view;
        };
        // native callback이 지정한 입력 변경을 실제 연결 원문에 쓰며 latch는 같은 장기 상태에 쓴다.
        const auto publish=[&](const MissionOutcomeState& view) {
            local=view.localPlayer;demo=view.demo;blocked=view.tellBlocked;state.latches=view.latches;
            // 원본 Player 필드와 방향별 표의 같은 현재 저장소를 갱신한다.
            for (std::size_t p=0;p<9;++p) {
                records[p*(patch ? 0xb4U : 0xacU)+0x50]=view.players[p].flags;Put(records,p*(patch ? 0xb4U : 0xacU)+(patch ? 0x7cU : 0x74U),view.players[p].temple);
                // 양방향을 합치지 않고 각 DWORD를 그대로 쓴다.
                for (std::size_t other=0;other<9;++other) currentAllies[p*9+other]=view.allies[p][other];
            }
        };
        if (live) { publish(state);state.players={};state.allies={};state.localPlayer=9;state.demo=7;state.tellBlocked=9; }
        const auto mutation=Number(row[6]);std::set<std::string> mutated;std::string trace;MissionOutcomeHooks hooks;
        // callback마다 현재 raw view를 읽고 해당 native 입력 변화만 원문에 반영한다.
        const auto change=[&](std::string_view kind) { if (!live) { Mutate(kind,mutation,mutated,state,priests);return; }auto view=current();Mutate(kind,mutation,mutated,view,priests);publish(view); };
        // 독립 native가 관찰한 입력/잠금 문자열을 현재 live backend에서도 같은 시점에 만든다.
        const auto snapshot=[&] { auto view=current();return Snapshot(view,priests); };
        // 영역 경계는 명시한 소유자만 반환한다. 호출 인자는 단정도 비트로 기록한다.
        hooks.regionOwner=[&](float x,float y) { Add(trace,"region:"+ValuesText(std::array{Bits(x),Bits(y)})+'#'+snapshot());change("region");return Number(row[7]); };
        // 섹션 callback 뒤 상태 변화와 최종 잠금 쓰기 순서를 원본 관찰과 비교한다.
        hooks.section=[&](std::string_view name) { Add(trace,"section:"+std::string(name)+'#'+snapshot());change("section");return Number(row[5])!=0; };
        hooks.closeTransientGumps=[&] { Add(trace,"close#"+snapshot());change("close"); };
        hooks.onSection=[&] { Add(trace,"stop#"+snapshot());change("stop"); };
        hooks.invalidPlayerAssert=[&](std::uint32_t player) { Add(trace,"assert:"+std::to_string(player)+'#'+snapshot()); };
        RawMissionOutcome outcome(pool,priests,state,MakeMissionOutcomeHooks(pool,reward,std::move(hooks)));std::uint32_t result=0;
        if (live) outcome.BindCurrentInputs({records,currentAllies,[&] { return local; },[&] { return demo; },[&] { return blocked; }});
        if (row[1]=="Check") outcome.Check();
        else if (row[1]=="Dead") result=outcome.PriestDead(Number(row[8]));
        else if (row[1]=="Captured") result=outcome.PriestCaptured(Number(row[8]));
        else if (row[1]=="Event") result=outcome.Event(state.goodTeamDead,Number(row[8])!=0,"BadTeamDead");
        else throw std::runtime_error("미션 fixture 동작 이름 오류");
        const auto observed=snapshot();
        if (observed!=row[16] || trace!=row[17] || result!=Number(row[18])) {
            std::fprintf(stderr,"missionoutcome row %u/%s/%s\n",number,row[0].c_str(),row[1].c_str());
            CHECK(observed==row[16]);CHECK(trace==row[17]);CHECK(result==Number(row[18]));
        }
    }
    CHECK(counts[0]==(live ? 8U : kCases) && counts[1]==(live ? 8U : kCases));
}
// 두 판본 native 전체 루트 출력/사제 상태/HP 경계/동맹/효과 당시 상태를 직접 대조한다.
TEST_CASE(MissionOutcome_IndependentPeFixture) { CheckOutcomeFixture(false); }
// 기존 native callback8개/방향별동맹8개를 raw 원문에 적용한다. 독립 state 입력을 오염시켜 stale 사본 사용을 검출한다.
TEST_CASE(MissionOutcome_CurrentInputsNativeCallbacks) { CheckOutcomeFixture(true); }

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

// 실제 dead/captured 조회가 공유표의2/1/0 변경을 즉시 읽으며 데이터 reset도 같은 주소에 적용된다.
TEST_CASE(PriestOwnerState_OutcomeReadsCurrentSharedTableAfterDataReset) {
    SidPool pool(OriginalEdition::Patch1078,kOutcomeCapacity);PriestPostPopState priests;MissionOutcomeState state;std::array<std::uint32_t,kPlayerCount+1> shared{};shared.fill(31);priests.ownerState.fill(2);priests.sharedOwnerState=&shared;
    priests.priests.entries={50,60,77};priests.priests.count=1;state.players[1].flags=0x22;MissionOutcomeHooks hooks;
    hooks.maxHitPoints=[](Sid)->int { throw std::logic_error("사제 owner 상태 조회에서HP선택"); };hooks.regionOwner=[](float,float)->std::uint32_t { throw std::logic_error("사제 owner 상태 조회에서지역선택"); };
    hooks.section=[](std::string_view)->bool { throw std::logic_error("사제 owner 상태 조회에서Tell선택"); };hooks.closeTransientGumps=hooks.onSection=[] { throw std::logic_error("사제 owner 상태 조회에서UI선택"); };
    RawMissionOutcome outcome(pool,priests,state,std::move(hooks));const auto entries=priests.priests.entries;const auto fallback=priests.ownerState;
    shared[1]=2;CHECK(outcome.PriestDead(1) && !outcome.PriestCaptured(1));shared[1]=1;CHECK(!outcome.PriestDead(1) && outcome.PriestCaptured(1));ResetPriestOwnerState(priests.OwnerState(),1);
    CHECK(!outcome.PriestDead(1) && !outcome.PriestCaptured(1) && shared[1]==0 && shared[0]==31);CHECK(priests.ownerState==fallback && priests.priests.entries==entries && priests.priests.count==1);
}
