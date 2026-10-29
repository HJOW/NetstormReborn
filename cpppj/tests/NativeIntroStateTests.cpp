// 실제 원본 CPU 관측의 모든 상태 BYTE/모드/이름/효과 순서와 C++ 부모를 대조한다.
#include "TestSupport.h"
#include "client/NativeIntroState.h"
#include "client/NativeMainMenuState.h"
#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace { // 창/영상/파일 장치를 명시 입력으로 두고 분기/commit은 제품을 실행한다.
using namespace netstorm::client;
using namespace netstorm::o;
struct IntroStop {}; // 메뉴 진입/선택 장치 중단의 원시 관측을 보존하는 검사 전용 신호다.
// 원시 TSV와 블록의 구분자만 분리하며 빈 칸도 보존한다.
std::vector<std::string> SplitIntro(std::string_view text,char separator) {
    std::vector<std::string> values;std::size_t start{}; // 현재 칸의 시작과 실제 필드 목록이다.
    for (;;) { const auto end=text.find(separator,start);values.emplace_back(text.substr(start,end==text.npos ? end : end-start));if (end==text.npos) break;start=end+1; } // 마지막 빈 필드까지 하나씩 읽는다.
    return values;
}
// 실제 물리 BYTE를 순서대로 16진수로 직렬화하여 전체 상태/이름 꼬리를 비교한다.
std::string IntroHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef"; // fixture와 같은 소문자 BYTE 표기다.
    std::string result; // 전체 BYTE의 두 글자 표현이다.
    for (auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // 각 BYTE의 상/하위 nibble을 보존한다.
    return result;
}
}

// 원문 정상/중단17입력의 다섯144BYTE와 효과 순서를 모두 대조한다.
TEST_CASE(NativeIntroState_Native17AllBlocksModesPendingAndPartialEffects) {
    std::ifstream file(NETSTORM_NATIVE_INTRO_FIXTURE);CHECK(file.good());std::string line;std::uint32_t cases{}; // 고정 최초 관측 파일과 실제 재생 수다.
    while (std::getline(file,line)) { // 주석 이외의 각 원시 CPU 관측을 별도 상태에 재생한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto fields=SplitIntro(line,'\t');CHECK(fields.size()==11);
        std::array<LocalMissionStateBlock,5> blocks; // old/phase/immediate/current/next의 실제 별도 물리다.
        for (std::size_t index=0;index<blocks.size();++index) for (std::size_t word=0;word<36;++word) blocks[index][word]=static_cast<std::uint32_t>(index*1000+word); // 원문 입력과 같은 전체 구별값을 설치한다.
        blocks[3][0]=static_cast<std::uint32_t>(std::stoul(fields[0]));blocks[3][35]=static_cast<std::uint32_t>(std::stoul(fields[1]));
        LocalMissionStateStorage storage{blocks[0],blocks[1],blocks[2],blocks[3],blocks[4]};PlayerWorldState player; // 실제 상태 분배기의 외부 Player 물리다.
        bool fort{},server{},boss{},battle{},post{},saved{},mission{},simul{},meta{}; // 서로 다른 현재 모드다.
        std::uint32_t zone{},ticks{},source{},seed{},refresh{},rebuild{};std::uint8_t title{},battleByte{};double connect{},activity{},seconds{},wall{}; // 선택하지 않는 RawState 외부 전역이다.
        std::array<std::uint8_t,16> pending;pending.fill(0xa5);pending[0]='N';pending[1]='e';pending[2]='x';pending[3]='t';pending[4]=0; // 첫 NUL 뒤도 비교한다.
        std::vector<std::string> trace;std::string status="normal"; // 장치/State의 실제 순서와 중단 종류다.
        const auto event=[&](std::string text) { trace.push_back(text);if (fields[5]==text) { status="stop";throw IntroStop{}; } }; // 도달한 효과 직전의 검사 중단이다.
        NativeIntroStateHooks intro;intro.cleanup=[&] { event("cleanup"); };intro.hideChatInput=[&] { event("input"); };intro.hideChat=[&] { event("hide"); };
        intro.core=[&] { event("core"); };intro.renderer=[&] { event("renderer"); };intro.hud=[&] { event("hud"); };
        intro.moviePath=[&] { event("path");return fields[2]=="1" ? std::string("MOVIE/missing.smk") : std::string{}; };
        intro.markIntro=[&] { event("mark"); };intro.openMovie=[&](auto path) { CHECK(path=="MOVIE/missing.smk");event("open"); };intro.movieActive=[&] { return fields[3]!="0"; };
        LocalMissionStateHooks hooks;hooks.logState=[&](auto selected) { trace.emplace_back("state:"+std::to_string(selected));if (selected==3) { status="menu";throw IntroStop{}; } };
        hooks.parentState=[&](auto selected) { if (selected==41) event("pending");return InstallNativeIntroState({storage,fort,server,boss,pending},intro); };
        RawLocalMissionState state(OriginalEdition::Patch1078,storage,{server,boss,battle,post,saved,mission,simul,meta,player,zone,ticks,source,seed,blocks[2][33],refresh,rebuild,title,battleByte,connect,activity,seconds,wall},std::move(hooks));
        intro.request=[&](auto next) { state.Request(next); };try { state.Run(); }catch (const IntroStop&) {}
        std::string raw; // 원문과 같은 순서의 다섯 전체 상태 블록이다.
        for (const auto& block:blocks) { if (!raw.empty()) raw+=':';raw+=IntroHex({reinterpret_cast<const std::uint8_t*>(block.data()),144}); } // 정렬/필드별 기대값으로 축약하지 않는다.
        const auto modes=std::to_string(fort)+','+std::to_string(server)+','+std::to_string(boss);std::string events; // 현재 모드와 효과의 원시 표현이다.
        for (const auto& entry:trace) { if (!events.empty()) events+=';';events+=entry; } // 모든 호출의 순서와 횟수를 보존한다.
        CHECK(status==fields[6] && raw==fields[7] && modes==fields[8] && IntroHex(pending)==fields[9] && events==fields[10]);++cases;
    }
    CHECK(cases==17);
}

// State3 intro0부터 영상 부재/메뉴3/후속4까지 같은 분배기가 동기 재진입 없이 처리한다.
TEST_CASE(NativeIntroState_FreshMenuMarksIntroAfterPathThenCommitsMenuAndPollFour) {
    std::array<LocalMissionStateBlock,5> blocks{};LocalMissionStateStorage storage{blocks[0],blocks[1],blocks[2],blocks[3],blocks[4]}; // 실제 다섯 블록이다.
    bool fort{},server{},boss{},battle{},post{},saved{},mission{},simul{},meta{};PlayerWorldState player; // 현재 모드/Player다.
    std::uint32_t zone{},ticks{},source{},seed{},refresh{},rebuild{},outcome{},first{1},editor{},introValue{};std::uint8_t title{},battleByte{};double connect{},activity{},seconds{},wall{}; // 각 원본 전역이다.
    std::array<char,80> background{};std::array<std::uint8_t,256> current{},pending{};std::vector<std::uint32_t> selected;std::uint32_t prepared{},opened{}; // 실제 이름/준비·열기 횟수다.
    NativeIntroStateHooks intro;NativeMainMenuStateHooks menu; // 큰 하위만 검사 장치로 공급한다.
    intro.cleanup=[&] { ++prepared; };intro.hideChatInput=[] {};intro.hideChat=[] {};intro.core=[] {};intro.renderer=[] {};intro.hud=[] {};
    intro.moviePath=[] { return std::string("missing.smk"); };intro.markIntro=[&] { introValue=1; };intro.openMovie=[&](auto) { ++opened; };intro.movieActive=[] { return false; };
    menu.configInt=[&](auto key) { return key=="intro" ? static_cast<std::int32_t>(introValue) : 0; };menu.prefix=[] {};menu.hudPrefix=[] {};menu.title=[] {};menu.closeGuide=[] {};menu.tell=[](auto) {};menu.tail=[&] { blocks[4][0]=4; };
    LocalMissionStateHooks hooks;hooks.logState=[&](auto value) { selected.push_back(value); };hooks.parentState=[&](auto value) {
        if (value>=39 && value<=41) return InstallNativeIntroState({storage,fort,server,boss,pending},intro);
        return InstallNativeMainMenuState({storage,source,outcome,first,battle,editor,background,current,pending,{},{}},menu);
    };
    RawLocalMissionState state(OriginalEdition::Patch1078,storage,{server,boss,battle,post,saved,mission,simul,meta,player,zone,ticks,source,seed,blocks[2][33],refresh,rebuild,title,battleByte,connect,activity,seconds,wall},std::move(hooks));
    intro.request=[&](auto value) { state.Request(value); };menu.request=intro.request;blocks[4][0]=3;state.Poll();
    CHECK(selected==std::vector<std::uint32_t>({3,39,40,41,3}) && prepared==1 && opened==1 && introValue==1 && blocks[1][0]==3 && blocks[0][0]==41 && blocks[4][0]==4);
    state.Poll();CHECK(blocks[1][0]==4 && blocks[0][0]==3 && !blocks[4][0]);
}
