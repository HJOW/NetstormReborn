// 실제 Guide 반복/얇은 단계/월드 진입 Pause의 전체 상태와 외부 호출 순서를 대조한다.
#include "RawSceneSupport.h"
#include "client/MissionLifecycle.h"
#include "client/State.h"
#include "o/RawMissionGuide.h"

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kLifecycleTable=0x12002000U; // 원본 격리 가상표의 비교 식별값이며 호스트 주소로 실행하지 않는다.
// 원본 입력의 같은 장기 상태/정지/현재 포인터와 명시 외부 효과만 공급한다.
struct LifecycleScene {
    std::array<std::uint64_t,12> input{}; // 연산/flags/순번/현재/pending/창/Tell/깊이/자기정지/종료요청/반복/기한 비트다.
    MissionStartState state;MissionFrameGateState gate; // 실제 원본 필드의 단일 저장소이며 별도 단계 사본을 두지 않는다.
    bool active{};std::uint32_t depth{},finish{};std::string trace; // 현재 식별자·외부 정지 깊이·Frame 요청/효과다.
    RawMissionFrameGate frameGate; // 실제 기존 Pause/Postpone를 같은 상태에 호출한다.
    RawMissionGuide guide; // 실제 기존 Guide 반복 몸체다.
    // 독립 입력만 준비하며 예상 다음 순번/효과는 계산하지 않는다.
    explicit LifecycleScene(std::string_view values):frameGate(gate,FrameHooks()),guide(OriginalEdition::Patch1078,GuideHooks()) {
        const auto fields=Split(values,',');CHECK(fields.size()==input.size());
        // 마지막 double 비트도 잘리지 않도록 원문64비트 열을 읽는다.
        for (std::size_t index=0;index<input.size();++index) input[index]=std::stoull(fields[index]);
        state.flags=static_cast<std::uint32_t>(input[1]);state.sequence=static_cast<std::uint32_t>(input[2]);active=input[3]!=0;depth=static_cast<std::uint32_t>(input[7]);finish=static_cast<std::uint32_t>(input[9]);
        gate.nextStageSeconds=std::bit_cast<double>(input[11]);gate.scriptPaused=static_cast<std::uint32_t>(input[8]);
    }
    // 실제 호출 한 번의 효과 순서를 이어 붙인다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 원본 현재 포인터 읽기마다 같은 관측을 남긴다.
    MissionStartState* Current() { Add(active ? "C,1" : "C,0");return active ? &state : nullptr; }
    // 도달하지 않는 공통 프레임 경계는 성공으로 채우지 않고 명시 거부한다.
    MissionFrameGateHooks FrameHooks() {
        MissionFrameGateHooks hooks;hooks.gameSeconds=[] { return 100.125; };hooks.paused=[this] { Add("Q");return depth!=0; };hooks.pause=[this] { Add("H");++depth; };
        hooks.resume=hooks.checkOutcome=hooks.tellStage=hooks.tellAt=hooks.endDemo=[] { throw std::logic_error("수명 원문에서 미선택 공통 프레임 효과"); };
        hooks.dialogsClear=hooks.knowledgeOpen=hooks.guideOpen=hooks.blockingGumpOpen=[]()->bool { throw std::logic_error("수명 원문에서 미선택 UI 조회"); };
        hooks.tutorial=[](std::uint32_t,std::uint32_t)->std::uint32_t { throw std::logic_error("수명 원문에서 미선택 Early 판정"); };return hooks;
    }
    // 실제 큰 Init/Start/Frame의 명시 경계는 독립 원문과 같은 요청만 공급한다.
    TutorialMissionStageHooks StageHooks() {
        TutorialMissionStageHooks hooks;
        hooks.initialize=[this] { Add("I");state.flags|=1U;PauseMissionWorldEntry([this] { return Current()!=nullptr; },[this] { frameGate.PauseForDialog(); }); };
        hooks.tellGlobal=[this](std::string_view name,std::uint32_t mode) { CHECK(mode==1);Add("G,"+Hex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()})); };
        hooks.start=[this] { Add("S");gate.started=1;RawMissionGuide::Advance(state); };
        hooks.frame=[this] { Add("F");if (finish) { state.flags|=1U;finish=0; } };
        hooks.showStage=[this] { Add("Show"); };
        hooks.tellMission=[this](std::string_view name) { Add("T,"+Hex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()}));return static_cast<std::uint32_t>(input[6]); };
        hooks.closeTransient=[this] { Add("K"); };hooks.pause=[this] { frameGate.PauseForDialog(); };return hooks;
    }
    // 원문 현재 포인터/관문/해제의 명시 입력을 같은 Guide에 연결한다.
    MissionGuideHooks GuideHooks() {
        MissionGuideHooks hooks;hooks.current=[this] { return Current(); };hooks.pending=[this] { Add("P");return static_cast<std::uint32_t>(input[4]); };hooks.dialogsClear=[this] { Add("D");return input[5]!=0; };
        hooks.stage=[this](MissionStartState& target,std::uint32_t sequence) { CHECK(&target==&state);RunTutorialMissionStage(target,sequence,StageHooks()); };
        hooks.postpone=[this](MissionStartState&) { frameGate.Postpone(); };hooks.release=[this] { Add("L");active=false; };hooks.invalidStage=[this] { Add("A"); };return hooks;
    }
    // 독립 실제 결과와 같은208바이트/현재/정지/trace를 직렬화한다.
    std::string Run() {
        // 입력이 지정한 별도 호출 수만큼 정상 Guide 또는 진입 Pause를 실행한다.
        for (std::uint64_t call=0;call<input[10];++call) {
            if (input[0]) PauseMissionWorldEntry([this] { return Current()!=nullptr; },[this] { frameGate.PauseForDialog(); });else guide.Run();
        }
        std::array<std::uint8_t,0xd0> raw{};
        // 원본 준비의 무관한 stale 바이트도 유지한다.
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*37U+41U);
        Put(raw,0,kLifecycleTable);Put(raw,0x54,state.flags);Put(raw,0x58,state.sequence);Put(raw,0x80,0x3f000000U);Put(raw,0x88,gate.started);Put(raw,0xb8,gate.scriptPaused);
        const auto next=std::bit_cast<std::uint64_t>(gate.nextStageSeconds); // 원본 double 저장의 전체 비트다.
        // 기한의 낮은/높은 BYTE를 모두 보존한다.
        for (std::size_t index=0;index<8;++index) raw[0x78+index]=static_cast<std::uint8_t>(next>>(index*8));
        return std::to_string(active ? 1 : 0)+'|'+std::to_string(depth)+'|'+Hex(raw)+'|'+(trace.empty() ? "-" : trace);
    }
};
}

// 제품 작성 전 실제831입력의 전체 상태/호출 순서와 원본 가상 단계 흐름을 비교한다.
TEST_CASE(MissionLifecycle_Native831) {
    const auto rows=LoadFixture(NETSTORM_MISSION_LIFECYCLE_FIXTURE).rows;CHECK(rows.size()==831);
    // 기대값은 새로 합성하지 않고 최초 PE 결과를 그대로 사용한다.
    for (const auto& row:rows) { CHECK(row.size()==2);LifecycleScene scene(row[0]);CHECK(scene.Run()==row[1]); }
}

// 현재 전환 큐가 Guide를 막는 값과 빈 현재의 진입 Pause 무동작을 확인한다.
TEST_CASE(MissionLifecycle_ActualPendingAndEmptyEntry) {
    State state;CHECK(!state.Pending());state.Post({"Tell","ABORT"});CHECK(state.Pending());state.Post({"Tell","TryAgain"});const auto action=state.Take();CHECK(action && action->argument=="ABORT" && !state.Pending());
    PauseMissionWorldEntry([] { return false; },{});CHECK(Throws([] { PauseMissionWorldEntry([] { return true; },{}); }));
}
