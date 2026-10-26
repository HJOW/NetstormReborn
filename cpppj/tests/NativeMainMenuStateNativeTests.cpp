// 독립 PE의82관측을 실제 State3 C++ 부모/공통 ret/기존 tail에 대조한다. 창/게임/원본 파일 쓰기는 실행하지 않는다.
#include "TestSupport.h"
#include "client/NativeMainMenuState.h"
#include "client/ExistingEditorAfterCheckpoint.h"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace { //독립 PE가 생성한 입력/관측을 읽으며 원문 예상값을 제품 코드로 만들지 않는다.
using namespace netstorm::client;
using namespace netstorm::o;
// 마지막 빈 필드도 보존하는 구분자 분리다.
std::vector<std::string> Native3Fields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } //원시 관측의 필드 순서를 유지한다.
}
// 같은 BYTE 물리를 소문자 little endian hex로 표현한다.
std::string Native3Bytes(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; //원문 BYTE 두 자리 표기다.
    for (const auto value:bytes) { result+=digits[value>>4];result+=digits[value&15U]; } //NUL 뒤 기록까지 그대로 비교한다.
    return result;
}
// fixture 불일치에 입력 이름/열을 붙여 긴 raw 전체를 로그에 반복하지 않는다.
void Native3Equal(std::string_view actual,std::string_view expected,std::string_view label,std::string_view field) {
    if (actual!=expected) throw std::runtime_error("Native State3 PE 불일치 "+std::string(label)+" / "+std::string(field));
}
// 현재 초안 경로와 나중에 옮길 실제 tests 경로 모두에서 저장소 루트를 찾는다.
std::filesystem::path Native3Fixture() {
    auto directory=std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path();
    while (!std::filesystem::exists(directory/"AGENTS.md")) { const auto parent=directory.parent_path();if (parent==directory) throw std::runtime_error("State3 fixture 저장소 경로 없음");directory=parent; } //파일 생성이나 상위 디렉터리 변경을 하지 않는다.
    return directory/"cpppj/tests/fixtures/native-main-menu-state-x86.tsv";
}
struct Native3Stop { std::string cause; }; //PE의 선택 도달점/중간 실패와 정상 ret를 구별한다.
// 큰 외부 장치의 명시 입력만 제공하고 State3 분기/직접쓰기/공통 ret는 실제 C++ 부모에 맡긴다.
struct Native3FixtureScene {
    std::array<std::uint32_t,11> values{};std::string mutation; //원문 입력11DWORD와 명시 외부 변경/중단 이름이다.
    ClientLocalStateGlobals local;PlayerWorldState player;InputQueue input; //기존 실제5블록/현재Player/원시 입력 고리다.
    std::uint32_t source{},outcome{},first{},updated{},tips{},editor{1}; //fixture가 읽고 쓰는 서로 다른 현재 전역이다.
    bool battle{},server{},boss{},post{},saved{},mission{},simul{},meta{}; //RawState 분배기에 빌리는 현재 모드다.
    std::uint32_t zone{},ticks{},seed{},refresh{},rebuild{};std::uint8_t battleByte{};double end{},seconds{},wall{}; //나머지 Native 분배기의 독립 입력이다.
    std::array<char,80> background{};std::array<std::uint8_t,256> current{},pending{}; //이름의 미작성 이웃 BYTE도 관찰한다.
    std::vector<std::string> events;RawLocalMissionState* running{}; //외부 호출 순서와 동기 범위의 실제 상태 분배기다.
    explicit Native3FixtureScene(std::string_view description) {
        const auto fields=Native3Fields(description,',');if (fields.size()!=12) throw std::runtime_error("State3 fixture 입력 폭");
        for (std::size_t index=0;index<values.size();++index) values[index]=static_cast<std::uint32_t>(std::stoull(fields[index])); //부호를 정규화하지 않고 원문 DWORD 입력을 읽는다.
        mutation=fields[11];source=values[7];outcome=values[8];first=values[3];updated=values[4];tips=values[5];battle=values[9]!=0;
        auto& blocks=local.Blocks();
        for (std::size_t block=0;block<blocks.size();++block) for (std::size_t word=0;word<blocks[block].size();++word) blocks[block][word]=static_cast<std::uint32_t>(block*1000+word); //PE와 같은 독립 입력 패턴이며 출력의 예상을 계산하지 않는다.
        blocks[1][0]=values[6];blocks[3][0]=3;current.fill(0xa5);pending.fill(0xa5);background.fill('?');
        const std::string old="Old",next="Next",previous="Previous";std::copy(old.begin(),old.end(),current.begin());current[old.size()]=0;std::copy(next.begin(),next.end(),pending.begin());pending[next.size()]=0;std::copy(previous.begin(),previous.end(),background.begin());background[previous.size()]=0;
    }
    // 도달한 외부 효과만 기록한 뒤 그 위치의 명시 중단을 발생시킨다.
    void Event(std::string value,std::string_view boundary={}) {
        const auto selected=boundary.empty() ? value : std::string(boundary);events.push_back(std::move(value));if (mutation=="stop:"+selected) throw Native3Stop{"stop:"+selected};
    }
    // 실제 parent의 flags 재읽기는 원문 CPU direct load 관측과 대응한다.
    NativeMainMenuStateGlobals Globals() {
        const auto pendingBytes=mutation=="pending-null" ? std::span<std::uint8_t>{} : std::span<std::uint8_t>(pending);
        return {local.State(),source,outcome,first,battle,editor,background,current,pendingBytes,[this] { events.emplace_back("updated");return updated; },[this] { events.emplace_back("tips");return tips; }};
    }
    // 같은 실제 tail이 HUD/벽시각/음악/팔레트/next4를 실행하며 섬 경계는 State3에서 미선택이어야 한다.
    void Tail() {
        ExistingEditorAfterCheckpointEffects effects;effects.hud=[this](auto command) { CHECK(command==0x0200003dU);Event("hud2"); };effects.wallSeconds=[this] { Event("wall");return 100.5; };
        effects.requestMusic=[this](auto name) { Event("music:"+std::string(name)); };effects.fortPalette=[this] { Event("palette"); };effects.islandCount=[]()->std::uint32_t { throw std::logic_error("State3에서 선택되면 안 되는 섬 조회"); };
        ApplyExistingEditorAfterCheckpoint(local,input,effects);
    }
    // 큰 child의 명시 반환/부분효과만 공급한다. cleanup/실제 UI 완료를 이 단위 검사로 주장하지 않는다.
    NativeMainMenuStateHooks Hooks() {
        NativeMainMenuStateHooks h;h.configInt=[this](auto key) { Event(std::string(key),key=="intro" ? "intro" : "welcome");CHECK(key=="intro" || key=="WelcomeGiven");return std::bit_cast<std::int32_t>(values[key=="intro" ? 0 : 1]); };
        h.request=[this](auto state) { if (!running) throw std::logic_error("State3 실제 분배기 수명 없음");running->Request(state); };
        h.deleteSavedState=[this] { Event("delete"); };
        h.requestMusic=[this](auto name) { Event("music:"+std::string(name));if (name=="fanfare.mus" && mutation=="source2") source=2;if (name=="fanfare.mus" && mutation=="outcome2") outcome=2; };
        h.sourceBackground=[this](auto type)->std::optional<std::string> { Event("source:"+std::to_string(type));CHECK(type==1 || type==2);if (!values[10]) return std::nullopt;return type==1 ? "SourceOne" : "SourceTwo"; };
        h.leaveBattle=[this] { Event("battleLeave"); };
        h.prefix=[this] { Event("prefix");editor=0;battle=false;if (mutation=="prefix-outcome4") outcome=4; }; //PE에서 같은 명시 cleanup child가 제공한 모드 효과다. 자식 알고리즘을 재구현하지 않는다.
        h.hudPrefix=[this] { Event("hud1"); };h.title=[this] { Event("title");if (mutation=="state2") local.State().current[0]=2; };h.closeGuide=[this] { Event("close"); };
        h.tell=[this](auto name) { Event("tell:"+std::string(name));if (name=="UserNotUpdated" && mutation=="tips0") tips=0;if (name=="Blank" && mutation=="updated1") updated=1; };
        h.startup=[this] { Event("startup");if (mutation=="first0") first=0;if (mutation=="tips1") tips=1;if (mutation=="editor1" || mutation=="editor1-stop") editor=1;return values[2]; };
        h.editorPlayers=[this] { Event("editorPlayers");if (mutation=="editor1-stop") throw Native3Stop{"stop:editorPlayers"}; };
        h.changedState=[this] { Event("changedState");throw Native3Stop{"changedState"}; };h.tail=[this] { Tail(); };return h;
    }
    // 기존 실제 공통 Run을 사용해 intro39/정상 ret/외부 예외의 다섯 블록을 관측한다.
    std::string Run() {
        LocalMissionStateHooks hooks;hooks.logState=[this](auto state) { events.push_back("state:"+std::to_string(state));if (state==39) throw Native3Stop{"intro39"}; };
        hooks.parentState=[this](auto selected) { CHECK(selected==3);return InstallNativeMainMenuState(Globals(),Hooks()); };
        RawLocalMissionState state(OriginalEdition::Patch1078,local.State(),{server,boss,battle,post,saved,mission,simul,meta,player,zone,ticks,source,seed,local.State().immediate[33],refresh,rebuild,reinterpret_cast<std::uint8_t&>(background[0]),battleByte,end,local.ActivityEnd(),seconds,wall},std::move(hooks));running=&state;
        try { state.Run();running=nullptr;return "normal"; }catch (const Native3Stop& stop) { running=nullptr;return stop.cause; }
    }
    // 실제 다섯 물리 블록을 원문과 같은 little endian BYTE로 읽는다.
    std::string Blocks() const {
        static_assert(std::endian::native==std::endian::little);std::string result;
        for (const auto& block:local.Blocks()) { if (!result.empty()) result+=':';result+=Native3Bytes({reinterpret_cast<const std::uint8_t*>(block.data()),144}); } //전체140BYTE 부가 인자도 포함한다.
        return result;
    }
    // 관측하는 각 원문 DWORD를 독립 주소에서 현재값으로 직렬화한다.
    std::string Words() const { return std::to_string(source)+','+std::to_string(outcome)+','+std::to_string(first)+','+std::to_string(updated)+','+std::to_string(tips)+','+std::to_string(editor); }
    // 첫BYTE 뒤 기존 자료와 NUL 후 흔적을 함께 대조한다.
    std::string Names() const { return Native3Bytes(current)+':'+Native3Bytes(pending)+':'+Native3Bytes({reinterpret_cast<const std::uint8_t*>(background.data()),background.size()}); }
    // 각 외부 callback의 실제 실행 순서다.
    std::string Trace() const { std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result; }
};
}

// 실제 PE 두 제어워드의82관측을 같은 C++ 부모/공통 ret/기존 tail에 공급하며 fixture를 예상 생성기로 쓰지 않는다.
TEST_CASE(NativeMainMenuState_NativePe82ActualBranchesPartialStopsAndCurrentRereads) {
    std::ifstream fixture(Native3Fixture(),std::ios::binary);CHECK(bool(fixture));std::string line;std::size_t observations{};std::set<std::string> inputs;
    while (std::getline(fixture,line)) { //원본 생성 도구가 고정한 모든 관측을 빠짐없이 소비한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Native3Fields(line,'\t');CHECK(row.size()==9 && (row[1]=="639" || row[1]=="895"));Native3FixtureScene scene(row[2]);const auto status=scene.Run();
        Native3Equal(status,row[3],row[0],"status");Native3Equal(scene.Blocks(),row[4],row[0],"5x144BYTE");Native3Equal(scene.Words(),row[5],row[0],"current DWORDs");Native3Equal(scene.Names(),row[6],row[0],"names/background");
        const auto activity=scene.local.ActivityEnd();Native3Equal(Native3Bytes({reinterpret_cast<const std::uint8_t*>(&activity),8}),row[7],row[0],"activity double");Native3Equal(scene.Trace(),row[8],row[0],"external trace");CHECK(scene.input.Empty());++observations;inputs.insert(row[0]);
    }
    CHECK(observations==82 && inputs.size()==41);
}
