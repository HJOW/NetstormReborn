// 접근점의현재checking·선택진단·Graph효과뒤부분쓰기만검사하며실제이동/건설을실행하지않는다.
#include "RawSceneSupport.h"
#include "o/RawBoardApproach.h"
#include "client/ClientGameplayGlobals.h"
#include <filesystem>

using namespace netstorm::o; // 실제접근점과779의공유뷰를사용한다.
using namespace netstorm::test::rawscene; // 새fixture의독립원시필드해독만공유한다.
namespace { // 합성Graph제공자와기대trace의입력해독만소유한다.
void BoardParentEvent(std::string& events,std::string value) { if (!events.empty()) events+=';';events+=value; } // 원문외부효과의순서를직렬화한다.
std::string BoardParentBits(std::span<const float> values) { // 원문float의전체DWORD를손실없이비교한다.
    std::string result;for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(std::bit_cast<std::uint32_t>(value)); }return result; // 선택된실제float만순회한다.
}
struct BoardParentFailure {}; // 선택된실제진단장치가명시적으로실패하는입력이다.
}

TEST_CASE(BoardCheckingParent_Native288CurrentOwnerAndResume) {
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/board-checking-parent-x86.tsv";std::size_t rows{};
    for (const auto& row:LoadFixture(path.string().c_str()).rows) { // 새원문관측288개만사용하고기존696fixture는바꾸지않는다.
        CHECK(row.size()==6);const auto input=Split(row[0],',');CHECK(input.size()==5);std::uint32_t checking=Number(input[0]),calls{},assertions{};std::string events;
        const auto mutation=Number(input[1]),after=Number(input[2]),match=Number(input[3]),flags=Number(input[4]);std::array<float,4> rectangle{20,20,24,24};
        RawBoardApproach approach([&](float x,float y) {
            const auto value=calls==0 || (calls==1 && match) ? 1U : 2U;
            if (calls==1 && mutation) {
                rectangle={10,10,30,30};if (mutation>=2) checking=std::array<std::uint32_t,4>{0,1,99,0xffffffffU}[mutation-2];
                BoardParentEvent(events,"M:"+std::to_string(mutation)+':'+std::to_string(checking));
            }
            BoardParentEvent(events,"G:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(value));++calls;return value;
        },[&] { ++assertions;BoardParentEvent(events,"A:0:565");if (after==1) checking=0;if (after==2) rectangle={11,11,29,29}; });
        const auto point=approach.Find(rectangle,30,10,flags,RawCheckingFlag{checking});const std::array<float,2> result{point.x,point.y};
        CHECK(BoardParentBits(rectangle)==row[1] && BoardParentBits(result)==row[2]);CHECK(checking==Number(row[3]) && assertions==Number(row[4]) && events==row[5]);++rows;
    }
    CHECK(rows==288);
}

TEST_CASE(BoardCheckingParent_ClientDwordRecheckedAfterActualBoundary) {
    netstorm::client::ClientGameplayGlobals globals;auto* identity=&globals.AiDebug().Checking();std::array<float,4> rectangle{20,20,24,24};std::uint32_t calls{},assertions{};
    RawBoardApproach approach([&](float,float) {
        if (calls++==0) return 1U;if (calls==2) { rectangle={10,10,30,30};globals.WriteDebugWord(0x5e4794,99); }return 2U; // 원문 입력0,4,1,0,0처럼 첫 후보에서만 값을 바꾸고 후속 Graph가 진단의0을 덮지 않게 한다.
    },[&] { ++assertions;globals.WriteDebugWord(0x5e4794,0); });
    static_cast<void>(approach.Find(rectangle,30,10,0,globals.AiDebug().Checking()));
    CHECK(assertions>0 && &globals.AiDebug().Checking()==identity); // 진입 시점의0사본이면 선택되지 않을 진단을 실제 현재99에서 읽어야 한다.
    CHECK(globals.ReadDebugWord(0x5e4794)==0); // 진단장치가쓴0을뒤에서강제로99로복구하지않는다.
}

TEST_CASE(BoardCheckingParent_UnselectedAndMissingDiagnosticBoundaries) {
    std::uint32_t checking{},calls{};std::array<float,4> rectangle{20,20,24,24};
    const auto graph=[&](float,float) { if (calls++==0) return 1U;rectangle={10,10,30,30};return 2U; };
    RawBoardApproach disabled(graph);static_cast<void>(disabled.Find(rectangle,30,10,0,RawCheckingFlag{checking}));CHECK(calls>1); // 현재0이면미연결진단장치를요구하지않는다.
    checking=29;calls=0;rectangle={20,20,24,24};bool failed{};
    try { static_cast<void>(disabled.Find(rectangle,30,10,0,RawCheckingFlag{checking})); }catch (const std::logic_error&) { failed=true; }
    CHECK((failed && calls==2 && rectangle==std::array<float,4>{10,10,30,30})); // 선택된실제미연결은Graph효과뒤그자리에서명시실패한다.
}

TEST_CASE(BoardCheckingParent_DiagnosticFailurePreservesCurrentCallerPrefix) {
    std::int32_t checking=99;std::array<float,4> rectangle{20,20,24,24};std::uint32_t calls{};
    RawBoardApproach approach([&](float,float) { if (calls++==0) return 1U;rectangle={10,10,30,30};return 2U; },[&] { checking=-1;throw BoardParentFailure{}; });
    bool failed{};try { static_cast<void>(approach.Find(rectangle,30,10,0,checking)); }catch (const BoardParentFailure&) { failed=true; }
    CHECK((failed && calls==2 && checking==-1 && rectangle==std::array<float,4>{10,10,30,30})); // 진단실패뒤원문아래의좌표복구나가짜성공을실행하지않는다.
}
