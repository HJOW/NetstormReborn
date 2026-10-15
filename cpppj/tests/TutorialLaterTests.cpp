// Early Missions3~6의 전체 원본 분기·현재 읽기·부수효과를 독립 PE 관찰과 대조한다.
#include "ConstructionSupport.h"
#include "o/RawTutorialLater.h"
#include <map>
#include <set>
using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
constexpr std::size_t kLaterInputs=533; // 중복 제거한 독립 원본 입력 수다.
const std::map<std::uint32_t,std::uint32_t> kTypeGlobals{{0x541174,71},{0x541178,72},{0x541184,75},{0x541188,76},{0x54118c,77},{0x541190,78},{0x541244,123},{0x541258,128},{0x541264,131},{0x541268,132},{0x54126c,133},{0x541288,140},{0x54128c,141},{0x5412ec,165}}; // 원본 PE에서 읽은 명시 타입 전역 입력이다.
// 모든 입력은 IEEE64 비트까지 보존하며 결과는 계산하지 않는다.
std::vector<std::uint64_t> LaterNumbers(std::string_view text) {
    std::vector<std::uint64_t> result;
    // 각 입력 열의 원래64비트를 그대로 읽는다.
    for (const auto& field:Split(text,',')) result.push_back(std::stoull(field));return result;
}
// 공통 미션14필드를 실제 폭으로 직렬화해 native 전체 상태와 비교한다.
std::string LaterGate(const MissionFrameGateState& state) {
    const std::array<std::uint64_t,14> values{std::bit_cast<std::uint32_t>(state.stage),state.stageDirty,state.tutorialNumber,state.demo,std::bit_cast<std::uint64_t>(state.startSeconds),std::bit_cast<std::uint64_t>(state.nextStageSeconds),Bits(state.delaySeconds),std::bit_cast<std::uint32_t>(state.lockFrames),state.started,std::bit_cast<std::uint64_t>(state.demoEndSeconds),std::bit_cast<std::uint64_t>(state.timerSeconds),std::bit_cast<std::uint64_t>(state.tellAtSeconds),state.scriptPaused,std::bit_cast<std::uint32_t>(state.clearFrames)};
    std::string result;
    // 값의 IEEE/부호 있는 DWORD 비트를 십진수로 보존한다.
    for (auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
}
// 실제 native와 같은 외부 입력/선택 효과만 공급한다. 판정/제작/부류/게이트는 제품 몸체다.
struct LaterEnvironment {
    std::vector<std::uint64_t> input; // 원본30개 명시 입력이며 callback 변경도 여기에만 적용한다.
    MissionFrameGateState& gate; // 제품이 수정하는 유일한 현재 단계/타이머다.
    std::uint32_t owner{1},depth{},selected{}; // 명시 외부 Player/정지깊이/선택 입력이다.
    std::string trace; // 실제 현재 TYPE 재조회와 외부 하위 호출의 순서다.
    std::set<std::string> changed; // 각 외부 효과의 첫 입력 변경만 실행한다.
    // 호출한 외부 경계가 현재 입력을 변경하는 정해진 재진입 장면이다.
    void Effect(const std::string& name) {
        if (!trace.empty()) trace+=',';trace+=name;
        if (!changed.insert(name).second) return;
        const auto mutation=input[29]; // 명시 입력이 선택한 효과만 실행한다.
        if (mutation==1 && name.starts_with('F')) owner=2;
        else if (mutation==2 && name.starts_with('I')) input[5]=0;
        else if (mutation==3 && name=="S") selected=2;
        else if (mutation==4 && name.starts_with('T')) gate.stage='D';
        else if (mutation==5 && name=="K") { depth=3;gate.scriptPaused=7; }
    }
    // 조회한 원본 전역 주소와 반환한 현재 타입을 native 메모리 읽기에 대조한다.
    std::uint32_t Type(std::uint32_t address) { Effect("Y"+std::to_string(address));return kTypeGlobals.at(address); }
    // 직접 단계 판정에 도달하지 않는 큰 공통 프레임 효과는 성공으로 채우지 않는다.
    MissionFrameGateHooks GateHooks() {
        MissionFrameGateHooks hooks;
        hooks.gameSeconds=[this] { return std::bit_cast<double>(input[26]); };
        hooks.paused=[this] { Effect("Q");return depth!=0; };hooks.pause=[this] { Effect("P");++depth; };
        hooks.resume=hooks.checkOutcome=hooks.tellStage=hooks.tellAt=hooks.endDemo=[] { throw std::logic_error("직접 튜토리얼3~6에서 다른 프레임 하위 선택"); };
        hooks.dialogsClear=hooks.knowledgeOpen=hooks.guideOpen=hooks.blockingGumpOpen=[]()->bool { throw std::logic_error("직접 튜토리얼3~6에서 프레임UI 선택"); };
        hooks.tutorial=[](std::uint32_t,std::uint32_t)->std::uint32_t { throw std::logic_error("직접 튜토리얼3~6의 중복 분배"); };return hooks;
    }
    // 원본에서 대체한 실제 외부 하위와 같은 반환·인자·효과 순서를 사용한다.
    TutorialLaterHooks Hooks() {
        TutorialLaterHooks hooks;
        hooks.findPriest=[this](std::uint32_t player) { const auto result=static_cast<std::uint32_t>(input[player==1 ? 13 : 14]);Effect("F"+std::to_string(player));return result; };
        hooks.graphFor=[this](std::uint32_t sid) { const auto result=static_cast<std::uint32_t>(input[sid==input[13] ? 15 : 16]);Effect("G"+std::to_string(sid));return result; };
        hooks.productionContains=[this](std::uint32_t type) { const bool result=(input[5]&(std::uint64_t{1}<<(type%32)))!=0;Effect("C"+std::to_string(type));return result; };
        hooks.indicateTechnology=[this](std::uint32_t type) { Effect("I"+std::to_string(type)); };
        hooks.regionFor=[this](float x,float y) { CHECK(x==12.0F && y==14.0F);const auto result=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(input[18]));Effect("R");return result; };
        hooks.nearby=[this](float x,float y,std::uint32_t mask) { CHECK(x==12.0F && y==14.0F);const auto result=static_cast<std::uint32_t>(input[20]);Effect("N"+std::to_string(mask));return result; };
        hooks.priestCaptured=[this](std::uint32_t sid) { const bool result=input[17]!=0;Effect("H"+std::to_string(sid));return result; };
        hooks.selected=[this] { const auto result=selected;Effect("S");return result; };
        hooks.gameSeconds=[this] { return std::bit_cast<double>(input[26]); };
        hooks.resetSelection=[this] { selected=0;Effect("Z"); };hooks.clearSelection=[this] { selected=0;Effect("X"); };
        hooks.tell=[this](std::string_view section) { const bool result=input[22]!=0;Effect("T"+std::string(section));return result; };
        hooks.closeTransientGumps=[this] { Effect("K"); };return hooks;
    }
};
}
// 독립 PE의533고유입력과 전체 공통 필드/옵션/제한/일회성 경고/현재 읽기/반환을 대조한다.
TEST_CASE(TutorialLater_Native533) {
    std::size_t compared=0;
    // 행마다 새 실제 저장소를 사용해 이전 단계의 일회성 표식이 섞이지 않게 한다.
    for (const auto& row:LoadFixture(NETSTORM_TUTORIAL_LATER_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==11);auto values=LaterNumbers(row[0]);CHECK(values.size()==30);
        MissionFrameGateState state;state.stage=static_cast<std::int32_t>(values[1]);state.stageDirty=7;state.started=1;state.startSeconds=2.;state.lockFrames=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(values[28]));state.timerSeconds=std::bit_cast<double>(values[25]);
        LaterEnvironment environment{values,state};environment.depth=static_cast<std::uint32_t>(values[27]);environment.selected=static_cast<std::uint32_t>(values[21]);
        SidPool pool(OriginalEdition::Patch1078,32768);std::vector<RiftTypeRecord> types(188);std::array<std::uint8_t,1620> players{};
        Put(InputRaw(pool,Sid{1}),10,static_cast<std::uint32_t>(values[9]),1);Put(InputRaw(pool,Sid{1}),0x22,static_cast<std::uint32_t>(values[10]),1);Put(InputRaw(pool,Sid{1}),14,Bits(12.F));Put(InputRaw(pool,Sid{1}),18,Bits(14.F));Put(InputRaw(pool,Sid{2}),10,static_cast<std::uint32_t>(values[12]),1);
        types[static_cast<std::size_t>(values[9])].flags2=static_cast<std::uint32_t>(values[11]);Put(players,180+0x78,static_cast<std::uint32_t>(values[19]));
        SquidPostPopState counts;counts.globalCounts.fill(static_cast<std::uint32_t>(values[6]));counts.localCounts.fill(static_cast<std::uint32_t>(values[7]));counts.globalCounts[123]=static_cast<std::uint32_t>(values[8]);
        std::array<std::uint32_t,256> restrictions{};restrictions[165]=9;RawTotalMade made(OriginalEdition::Patch1078,types,counts,restrictions);RawMissionFrameGate gate(state,environment.GateHooks());
        MissionStartState mission;mission.fieldC0=static_cast<std::uint32_t>(values[23]);mission.fieldC4=static_cast<std::uint32_t>(values[24]);std::array<std::uint8_t,8> options{3,4,5,6,7,8,9,10};
        const std::uint32_t range=0x1230001,placed=0x1230002,contained=0x1230003,technology=0x1230004,capture=0x1230005,ascend=0x1230006; // native와 같은 서로 다른 실제 명령 입력이다.
        RawTutorialLater body(pool,types,players,made,gate,mission,{options,environment.owner,[&](std::uint32_t address) { return environment.Type(address); },range,placed,contained,technology,capture,ascend},environment.Hooks());
        const std::array<std::uint32_t,3> event{static_cast<std::uint32_t>(values[2]),static_cast<std::uint32_t>(values[3]),static_cast<std::uint32_t>(values[4])};
        const auto result=body.Handle(static_cast<std::uint32_t>(values[0]),event);
        if (LaterGate(state)!=row[1] || environment.trace!=row[3]) throw std::runtime_error("튜토리얼3~6 native 행 "+std::to_string(compared)+" 입력 "+row[0]+" 현재 "+LaterGate(state)+" 기대 "+row[1]+" 효과 "+environment.trace+" 기대 "+row[3]);
        CHECK(environment.depth==Number(row[2]) && result==Number(row[4]));CHECK(Hex(options)==row[5] && restrictions[165]==Number(row[6]));CHECK(environment.selected==Number(row[7]) && mission.fieldC0==Number(row[8]) && mission.fieldC4==Number(row[9]) && environment.owner==Number(row[10]));++compared;
    }
    CHECK(compared==kLaterInputs);
}
// 누락된 생산 하위/잘못된 입력은 단계 변경이나 옵션 쓰기 전에 명시 거부한다.
TEST_CASE(TutorialLater_RejectsMissingConsumerAndEvent) {
    MissionFrameGateState state;LaterEnvironment environment{std::vector<std::uint64_t>(30),state};SidPool pool(OriginalEdition::Patch1078,32768);std::vector<RiftTypeRecord> types(188);std::array<std::uint8_t,1620> players{};SquidPostPopState counts;std::array<std::uint32_t,256> restrictions{};RawTotalMade made(OriginalEdition::Patch1078,types,counts,restrictions);RawMissionFrameGate gate(state,environment.GateHooks());MissionStartState mission;std::array<std::uint8_t,3> options{};const std::uint32_t command=1; // 이 검사는 실제 효과에 도달하지 않는 계약 입력이다.
    TutorialLaterState inputs{options,environment.owner,[&](std::uint32_t address) { return environment.Type(address); },command,command,command,command,command,command};auto hooks=environment.Hooks();hooks.productionContains={};
    CHECK(Throws([&] { RawTutorialLater body(pool,types,players,made,gate,mission,inputs,hooks); }));
    RawTutorialLater body(pool,types,players,made,gate,mission,inputs,environment.Hooks());CHECK(Throws([&] { static_cast<void>(body.Handle(3,std::array<std::uint32_t,2>{})); }));CHECK(Throws([&] { static_cast<void>(body.Handle(2,std::array<std::uint32_t,3>{})); }));CHECK(state.stage==65 && options[2]==0 && environment.trace.empty());
}
