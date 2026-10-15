// 실제 Tutorial2·공유 gate/TotalMade의 독립 PE 출력과 전체 상태/부수효과 순서를 대조한다.
#include "ConstructionSupport.h"
#include "o/RawTutorialTwo.h"
#include <fstream>
#include <set>
#include <utility>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
constexpr std::size_t kTutorialTwoCases=346; // 중복 제거한 새 PATCH 독립 입력 수다.
constexpr std::uint32_t kTutorialPoolCapacity=32768; // 실제 SidPool의 서버/예측 영역을 포함하는 검사 물리 용량이다.
// IEEE64/부호 없는 필드를 DWORD로 줄이지 않고 읽는다.
std::uint64_t TwoNumber(const std::string& text) { return std::stoull(text); }
// 원본signedDWORD비트를후속비교용int32로해석한다.
std::int32_t TwoSigned(std::uint64_t value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)); }
// 원본 옵션의 16진수 BYTE를 변환하며 주위 옵션도 함께 보존한다.
std::vector<std::uint8_t> TwoBytes(std::string_view text) {
    if (text.size()%2) throw std::runtime_error("Tutorial2 옵션16진수 길이 오류");
    std::vector<std::uint8_t> result;
    // 각각 두 글자를 한 BYTE로 해석하며 타입 번호/DWORD로 확장하지 않는다.
    for (std::size_t i=0;i<text.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(i,2)),nullptr,16)));
    return result;
}
// 실제 fixture의14필드 입력을 그대로 공통 게이트 상태에 전달한다.
MissionFrameGateState TwoGateState(const std::string& text) {
    const auto fields=Split(text,',');if (fields.size()!=14) throw std::runtime_error("Tutorial2 미션 상태 열 수 오류");
    std::array<std::uint64_t,14> values{};
    // 입력 IEEE/DWORD비트만읽으며기대상태를계산하지않는다.
    for (std::size_t i=0;i<values.size();++i) values[i]=TwoNumber(fields[i]);
    MissionFrameGateState state;state.stage=TwoSigned(values[0]);state.stageDirty=static_cast<std::uint32_t>(values[1]);
    state.tutorialNumber=static_cast<std::uint32_t>(values[2]);state.demo=static_cast<std::uint32_t>(values[3]);
    state.startSeconds=std::bit_cast<double>(values[4]);state.nextStageSeconds=std::bit_cast<double>(values[5]);state.delaySeconds=Float(static_cast<std::uint32_t>(values[6]));
    state.lockFrames=TwoSigned(values[7]);state.started=static_cast<std::uint32_t>(values[8]);state.demoEndSeconds=std::bit_cast<double>(values[9]);
    state.timerSeconds=std::bit_cast<double>(values[10]);state.tellAtSeconds=std::bit_cast<double>(values[11]);state.scriptPaused=static_cast<std::uint32_t>(values[12]);state.clearFrames=TwoSigned(values[13]);return state;
}
// 실제14필드의현재비트를원본과같은순서로기록한다.
std::string TwoGateSnapshot(const MissionFrameGateState& state) {
    const std::array<std::uint64_t,14> values{std::bit_cast<std::uint32_t>(state.stage),state.stageDirty,state.tutorialNumber,state.demo,
        std::bit_cast<std::uint64_t>(state.startSeconds),std::bit_cast<std::uint64_t>(state.nextStageSeconds),Bits(state.delaySeconds),
        std::bit_cast<std::uint32_t>(state.lockFrames),state.started,std::bit_cast<std::uint64_t>(state.demoEndSeconds),std::bit_cast<std::uint64_t>(state.timerSeconds),
        std::bit_cast<std::uint64_t>(state.tellAtSeconds),state.scriptPaused,std::bit_cast<std::uint32_t>(state.clearFrames)};
    std::string result;
    // 현재원본폭필드만출력하며기대값을재계산하지않는다.
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
}
// 실제 원본 입력/관찰 경계만 제공한다. 단계 판단과 Advance/Pause/TotalMade는 제품 몸체를 사용한다.
struct TwoEnvironment {
    MissionFrameGateState& mission; // Tutorial2와 gate가공유하는유일한단계/타이머다.
    SquidPostPopState& counts; // 원본과같은postPop의누적제작표다.
    std::uint32_t depth{},selected{},mutation{}; // 명시외부정지깊이/현재선택/재진입입력이다.
    double now{}; // 실제기계어에입력한같은double게임시각이다.
    bool production{},tellResult{}; // 현재생산목록/NotVortex창생성의명시반환입력이다.
    std::string trace; // 외부조회/효과의실제호출순서다.
    std::set<std::string> changed; // 명시외부효과의첫호출만입력을바꾸는장부다.
    // 선택한외부효과뒤공유입력을바꾸며gate/Tutorial2의분기를복제하지않는다.
    void Effect(const std::string& kind) {
        if (!trace.empty()) trace+=',';trace+=kind;
        if (!changed.insert(kind).second) return;
        if (mutation==1 && kind=="R") counts.localSecondaryCounts[123]=1;
        else if (mutation==2 && kind=="Tell") production=true;
        else if (mutation==3 && kind=="Z") mission.timerSeconds=99.0;
        else if (mutation==4 && kind=="K") { depth=3;mission.scriptPaused=7; }
    }
    // 공통프레임자체를호출하지않는범위의큰미션하위는명시예외로남긴다.
    MissionFrameGateHooks GateHooks() {
        MissionFrameGateHooks hooks;hooks.gameSeconds=[this] { return now; };hooks.paused=[this] { Effect("Q");return depth!=0; };
        hooks.pause=[this] { Effect("P");++depth; };hooks.resume=[] { throw std::logic_error("Tutorial2 직접판정은Resume미실행"); };
        hooks.checkOutcome=hooks.tellStage=hooks.tellAt=hooks.endDemo=[] { throw std::logic_error("Tutorial2 직접판정의다른미션하위미실행"); };
        hooks.dialogsClear=hooks.knowledgeOpen=hooks.guideOpen=hooks.blockingGumpOpen=[]()->bool { throw std::logic_error("Tutorial2 직접판정의프레임UI미실행"); };
        hooks.tutorial=[](std::uint32_t,std::uint32_t)->std::uint32_t { throw std::logic_error("Tutorial2 직접판정의중복분배미실행"); };return hooks;
    }
    // 원본 실제 UI/생산/옵션하위의 명시인자/반환/순서만 공급한다.
    TutorialTwoHooks Hooks() {
        TutorialTwoHooks hooks;hooks.gameSeconds=[this] { return now; };hooks.refreshEnergyRange=[this] { Effect("R"); };
        hooks.selected=[this] { Effect("S");return selected; };hooks.productionContains=[this](std::uint32_t type) { CHECK(type==71);Effect("C71");return production; };
        hooks.resetSelection=[this] { Effect("Z");selected=0; };hooks.clearSelection=[this] { Effect("X");selected=0; };
        hooks.tell=[this](std::string_view section) { CHECK(section=="NotVortex");Effect("Tell");return tellResult; };
        hooks.closeTransientGumps=[this] { Effect("K"); };return hooks;
    }
};
}

// 원본346고유입력의전체공유상태/옵션8BYTE/금지/허용/선택/정지/효과순서/반환을직접대조한다.
TEST_CASE(TutorialTwo_IndependentPeFixture) {
    std::ifstream input(NETSTORM_TUTORIALTWO_FIXTURE);if (!input) throw std::runtime_error("Tutorial2 fixture 없음");
    std::string line;std::size_t count=0;
    // 각행을새raw풀과장부에서시작하여이전예약/선택효과가섞이지않는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=27) throw std::runtime_error("Tutorial2 fixture 열 수 오류");++count;
        SidPool pool(OriginalEdition::Patch1078,kTutorialPoolCapacity);std::vector<RiftTypeRecord> types(188);SquidPostPopState counts;
        types[71].flags2=0x20;types[85].flags2=0x4000;types[123].flags2=0x200;InputRaw(pool,Sid{1})[10]=71;InputRaw(pool,Sid{2})[10]=123;
        counts.localSecondaryCounts[123]=Number(row[10]);counts.localSecondaryCounts[85]=Number(row[11]);counts.localSecondaryCounts[71]=Number(row[12]);
        std::array<std::uint32_t,256> restrictions{};restrictions[85]=Number(row[18]);RawTotalMade made(OriginalEdition::Patch1078,types,counts,restrictions);
        auto mission=TwoGateState(row[9]);TwoEnvironment environment{mission,counts};environment.depth=Number(row[3]);environment.selected=Number(row[13]);
        environment.now=std::bit_cast<double>(TwoNumber(row[2]));environment.production=Number(row[14])!=0;environment.tellResult=Number(row[15])!=0;environment.mutation=Number(row[8]);
        RawMissionFrameGate gate(mission,environment.GateHooks());auto options=TwoBytes(row[16]);auto deny=Number(row[17]);std::uint32_t workshop=85,archer=71,salvage=0x1230000;
        RawTutorialTwo body(pool,types,made,gate,{options,deny,workshop,archer,salvage},environment.Hooks());
        CHECK(body.Handle(Number(row[1]))==Number(row[22]));CHECK(TwoGateSnapshot(mission)==row[19]);CHECK(environment.depth==Number(row[20]));CHECK(environment.trace==row[21]);
        CHECK(options==TwoBytes(row[23]));CHECK(deny==Number(row[24]));CHECK(restrictions[85]==Number(row[25]));CHECK(environment.selected==Number(row[26]));
    }
    CHECK(count==kTutorialTwoCases);
}

// 필수UI/생산/옵션경계가없거나다른판본자료이면실제효과전에거부한다.
TEST_CASE(TutorialTwo_RejectsMissingBoundary) {
    SidPool pool(OriginalEdition::Patch1078,kTutorialPoolCapacity);std::vector<RiftTypeRecord> types(188);SquidPostPopState counts;
    std::array<std::uint32_t,256> restrictions{};RawTotalMade made(OriginalEdition::Patch1078,types,counts,restrictions);MissionFrameGateState mission;
    TwoEnvironment environment{mission,counts};RawMissionFrameGate gate(mission,environment.GateHooks());std::array<std::uint8_t,8> options{};
    std::uint32_t deny=1,workshop=85,archer=71,salvage=0x1230000;auto hooks=environment.Hooks();hooks.productionContains={};bool rejected=false;
    try { const RawTutorialTwo body(pool,types,made,gate,{options,deny,workshop,archer,salvage},std::move(hooks)); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected);
}
