// 원본 PE의 실제 공통 프레임/예약 반환과 같은 명시 입력의 전체 상태·효과 순서를 대조한다.
#include "TestSupport.h"
#include "o/RawMissionFrameGate.h"
#include <array>
#include <bit>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace netstorm::o;
namespace {
// 원본 fixture의 고유 입력 개수이며 같은 사례를 다른 검사에서 다시 더하지 않는다.
constexpr std::size_t kMissionFrameCases=328;
// 마지막 빈 필드를 보존하며 TSV/쉼표 목록을 분리한다.
std::vector<std::string> SplitMissionFrame(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t first=0;
    // 구분자 사이의 원본 비트/효과 문자열을 순서대로 보존한다.
    while (true) {
        const auto end=text.find(delimiter,first);result.emplace_back(text.substr(first,end==std::string_view::npos ? end : end-first));
        if (end==std::string_view::npos) return result;first=end+1;
    }
}
// unsigned64 실제 비트는 DWORD를 거쳐 축소하지 않는다.
std::uint64_t MissionNumber(const std::string& text) { return std::stoull(text); }
// fixture의 DWORD 비트를 signed 상태 필드로 되돌린다.
std::int32_t MissionSigned(std::uint64_t value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)); }
// fixture의14필드를 실제 원본 상태로 구성한다. 기대 결과를 계산하지 않는다.
MissionFrameGateState MissionState(std::string_view text) {
    const auto fields=SplitMissionFrame(text,',');if (fields.size()!=14) throw std::runtime_error("미션 프레임 상태 열 수 오류");
    std::array<std::uint64_t,14> values{};
    // 모든 값은 원본 입력의 IEEE/DWORD 비트다.
    for (std::size_t i=0;i<values.size();++i) values[i]=MissionNumber(fields[i]);
    MissionFrameGateState state;
    state.stage=MissionSigned(values[0]);state.stageDirty=static_cast<std::uint32_t>(values[1]);state.tutorialNumber=static_cast<std::uint32_t>(values[2]);state.demo=static_cast<std::uint32_t>(values[3]);
    state.startSeconds=std::bit_cast<double>(values[4]);state.nextStageSeconds=std::bit_cast<double>(values[5]);state.delaySeconds=std::bit_cast<float>(static_cast<std::uint32_t>(values[6]));
    state.lockFrames=MissionSigned(values[7]);state.started=static_cast<std::uint32_t>(values[8]);state.demoEndSeconds=std::bit_cast<double>(values[9]);
    state.timerSeconds=std::bit_cast<double>(values[10]);state.tellAtSeconds=std::bit_cast<double>(values[11]);state.scriptPaused=static_cast<std::uint32_t>(values[12]);state.clearFrames=MissionSigned(values[13]);return state;
}
// 전체 상태의 실제 비트를 native fixture와 같은 순서로 읽는다.
std::string MissionSnapshot(const MissionFrameGateState& state) {
    const std::array<std::uint64_t,14> values{std::bit_cast<std::uint32_t>(state.stage),state.stageDirty,state.tutorialNumber,state.demo,
        std::bit_cast<std::uint64_t>(state.startSeconds),std::bit_cast<std::uint64_t>(state.nextStageSeconds),std::bit_cast<std::uint32_t>(state.delaySeconds),
        std::bit_cast<std::uint32_t>(state.lockFrames),state.started,std::bit_cast<std::uint64_t>(state.demoEndSeconds),
        std::bit_cast<std::uint64_t>(state.timerSeconds),std::bit_cast<std::uint64_t>(state.tellAtSeconds),state.scriptPaused,std::bit_cast<std::uint32_t>(state.clearFrames)};
    std::string result;
    // 현재 필드만 출력하며 원본 기대값을 다시 계산하지 않는다.
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
}
// 원본 실행기의 명시 외부 입력/효과만 공급한다. gate 분기 자체의 복제는 하지 않는다.
struct MissionFrameEnvironment {
    MissionFrameGateState& state; // 처리기와 실제로 같은 단계/타이머/잠금 저장소다.
    double now{}; // 독립 입력 게임 시각이다. 벽시각이나 합성 틱으로 변환하지 않는다.
    std::uint32_t depth{},event{},mutation{}; // 외부 정지 깊이·현재 사건·명시 재진입 입력이다.
    bool dialogs{},knowledge{},guide{},blocking{}; // 실제 UI 조회의 명시 반환 입력이다.
    std::set<std::string> changed; // 선택 외부 효과를 한 번만 적용하는 입력 장부다.
    std::string trace; // 원본과 비교할 외부 효과/조회 순서다.
    // 각 외부 효과 첫 호출에서만 원본 실행기와 동일한 입력 변경을 적용한다.
    void Effect(const std::string& kind) {
        if (!trace.empty()) trace+=',';trace+=kind;
        if (!changed.insert(kind).second) return;
        if (mutation==1 && kind=="O") { state.lockFrames=0;state.tutorialNumber=2; }
        else if (mutation==2 && kind=="S") { state.scriptPaused=1;++depth;dialogs=false; }
        else if (mutation==3 && kind=="L") { state.lockFrames=0;state.demo=3; }
        else if (mutation==4 && kind=="D") state.lockFrames=2;
        else if (mutation==5 && kind=="G") state.scriptPaused=0;
        else if (mutation==6 && kind=="E") { state.demoEndSeconds=100.0;state.demo=0; }
    }
    // 실제 native의 각 외부 함수 관찰 경계와 같은 이름/현재값을 제공한다.
    MissionFrameGateHooks Hooks() {
        MissionFrameGateHooks hooks;
        hooks.gameSeconds=[this] { return now; };hooks.paused=[this] { Effect("Q");return depth!=0; };
        hooks.checkOutcome=[this] { Effect("O"); };hooks.dialogsClear=[this] { Effect("D");return dialogs; };
        hooks.knowledgeOpen=[this] { Effect("K");return knowledge; };hooks.guideOpen=[this] { Effect("G");return guide; };
        hooks.blockingGumpOpen=[this] { Effect("B");return blocking; };hooks.tellStage=[this] { Effect("S"); };
        hooks.tellAt=[this] { Effect("L"); };hooks.endDemo=[this] { Effect("E"); };
        hooks.tutorial=[this](std::uint32_t number,std::uint32_t current) { CHECK(current==event);Effect("T"+std::to_string(number));return 0xa000U+number; };
        hooks.pause=[this] { Effect("P");++depth; };hooks.resume=[this] { Effect("R");--depth; };return hooks;
    }
};
}

// 실제 원본 네 몸체328개의 정상 반환/입력/전체 상태/외부 조회와 효과를 직접 대조한다.
TEST_CASE(MissionFrameGate_IndependentPeFixture) {
    std::ifstream input(NETSTORM_MISSIONFRAME_FIXTURE);if (!input) throw std::runtime_error("미션 프레임 fixture 없음");
    std::string line;std::size_t count=0;std::set<std::string> unique;
    // 각 행은 독립 native와 동일한 새 상태/외부 입력에서 시작한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto fields=SplitMissionFrame(line,'\t');if (fields.size()!=14) throw std::runtime_error("미션 프레임 fixture 열 수 오류");++count;
        CHECK(unique.insert(line.substr(0,line.find_last_of('\t'))).second);
        auto state=MissionState(fields[9]);MissionFrameEnvironment environment{state};
        environment.event=static_cast<std::uint32_t>(MissionNumber(fields[1]));environment.now=std::bit_cast<double>(MissionNumber(fields[2]));
        environment.depth=static_cast<std::uint32_t>(MissionNumber(fields[3]));environment.dialogs=MissionNumber(fields[4])!=0;
        environment.knowledge=MissionNumber(fields[5])!=0;environment.guide=MissionNumber(fields[6])!=0;environment.blocking=MissionNumber(fields[7])!=0;
        environment.mutation=static_cast<std::uint32_t>(MissionNumber(fields[8]));RawMissionFrameGate gate(state,environment.Hooks());CHECK(&gate.State()==&state);
        std::uint32_t result=0;
        if (fields[0]=="Frame") result=gate.RunFrame(environment.event);
        else if (fields[0]=="Advance") static_cast<void>(gate.Advance());
        else if (fields[0]=="Pause") gate.PauseForDialog();
        else if (fields[0]=="Postpone") gate.Postpone();
        else throw std::runtime_error("알 수 없는 미션 프레임 fixture 연산");
        CHECK(MissionSnapshot(state)==fields[10]);CHECK(environment.depth==MissionNumber(fields[11]));CHECK(environment.trace==fields[12]);CHECK(result==MissionNumber(fields[13]));
    }
    CHECK(count==kMissionFrameCases);
}

// 모든 큰 하위 경계는 명시해야 한다. 누락을 실행하지 않는 정상 미션으로 숨기지 않는다.
TEST_CASE(MissionFrameGate_RejectsMissingBoundary) {
    MissionFrameGateState state;MissionFrameEnvironment environment{state};auto hooks=environment.Hooks();hooks.checkOutcome={};bool rejected=false;
    try { const RawMissionFrameGate gate(state,std::move(hooks)); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected);
}
