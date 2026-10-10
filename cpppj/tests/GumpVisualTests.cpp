// 원본 버튼 생성/그리기 관찰과 안전 출력 경계를 검사한다. 원본 게임·소리 장치를 실행하지 않는다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/GumpVisual.h"
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace netstorm::client;
namespace {
// 원본이 남긴 96바이트 문맥을 채널/필드 재배치 없이 읽는다.
std::vector<std::uint8_t> ReadHex(const std::string& hex) {
    std::vector<std::uint8_t> bytes(hex.size()/2);
    // fixture의 각 바이트를 메모리 순서 그대로 변환한다.
    for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(std::stoul(hex.substr(i*2,2),nullptr,16));
    return bytes;
}
// 선 계획을 원본 관찰의 정수/순서 문자열로 직렬화한다. 기대값 계산은 하지 않는다.
std::string Lines(const std::vector<GumpLine>& lines) {
    if (lines.empty()) return "-";
    std::ostringstream result;
    // 전체 DWORD 색 번호와 음수 끝점을 유지한다.
    for (std::size_t i=0;i<lines.size();++i) {
        if (i) result<<'|';const auto& line=lines[i];
        result<<line.first.x<<','<<line.first.y<<','<<line.last.x<<','<<line.last.y<<','<<line.color;
    }
    return result.str();
}
// 검사 화면보다 오래 살아 있는 참조 메모리 DC다.
struct ReferenceDc {
    HDC value{CreateCompatibleDC(nullptr)};
    // 실제 창 없이 만든 참조 DC만 해제한다.
    ~ReferenceDc() { if (value) DeleteDC(value); }
};
}

// 세 PE의 전체 몸체가 실제로 출력한 문맥·선·위치를 대조한다. 낮은 플래그 비트/음수 나눗셈/감김을 포함한다.
TEST_CASE(GumpVisual_MatchesThreeNativeButtonBodies) {
    static_assert(sizeof(GumpTextContext)==32);
    std::ifstream input(NETSTORM_GUMPVISUAL_FIXTURE);CHECK(input.good());std::string line;std::size_t count=0;
    // 각 입력/기대 출력은 원본 x86 관찰에서 읽으며 복원 알고리즘으로 만들지 않는다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::string edition,contextsHex,expectedLines;unsigned number{},context{};
        ButtonVisualState state;ButtonPalette colors;ScreenPoint expected;
        row>>edition>>number>>state.rect.left>>state.rect.top>>state.rect.right>>state.rect.bottom
            >>state.textWidth>>state.textHeight>>state.value>>state.pressed>>state.flags
            >>colors.black>>colors.white>>colors.edge>>contextsHex>>expected.x>>expected.y>>context>>expectedLines;
        CHECK(!row.fail());if (row.fail()) continue;
        const auto failures=netstorm::test::FailureCount();
        const auto bytes=ReadHex(contextsHex);const auto contexts=ButtonTextContexts(colors);
        CHECK(bytes.size()==sizeof(contexts));if (bytes.size()!=sizeof(contexts) || context>=contexts.size()) continue;
        CHECK(std::memcmp(contexts.data(),bytes.data(),bytes.size())==0);
        const auto plan=PlanButtonDraw(state,contexts,colors.edge);
        CHECK(plan.textOffset.x==expected.x && plan.textOffset.y==expected.y);
        CHECK(std::memcmp(&plan.text,bytes.data()+context*32,32)==0);
        CHECK(Lines(plan.lines)==expectedLines);
        if (failures!=netstorm::test::FailureCount()) std::fprintf(stderr,"GumpVisual fixture: %s case %u\n",edition.c_str(),number);
        ++count;
    }
    CHECK(count==192);
}

// 화면이 일시 팔레트를 적용해도 버튼은 파일 로더의 표를 쓴다. 새 파일을 읽으면 새 번호를 사용한다.
TEST_CASE(GumpVisual_UsesLoadedPaletteUntilNextFile) {
    ReferenceDc dc;CHECK(dc.value!=nullptr);if (!dc.value) return;
    Screen screen(nullptr,dc.value,640,480);screen.Init();std::array<std::uint8_t,776> col{};
    // 모든 내부 색을 흰색으로 만들어 정확한 외곽선 색 두 개의 동률과 첫 번호 선택을 드러낸다.
    for (std::size_t i=8;i<col.size();++i) col[i]=255;
    col[8+41*3]=col[8+199*3]=55;col[9+41*3]=col[9+199*3]=51;col[10+41*3]=col[10+199*3]=54;
    // 두 판본의 색 표에서 위치가 달라도 같은 실제 버튼 역할을 읽는다.
    for (const auto edition:{netstorm::o::OriginalEdition::Patch1078,netstorm::o::OriginalEdition::Cd1072}) {
        screen.LoadPalette(GamePalette(col),edition);const auto colors=ButtonColors(screen.Colors());
        CHECK(colors.black==0 && colors.white==1 && colors.edge==41);
        ScreenColor temporary{255,255,255,0};screen.SetPalette(41,1,&temporary,true);
        CHECK(screen.FindColor(55,51,54)==199);CHECK(ButtonColors(screen.Colors()).edge==41);
    }
    col[8+41*3]=col[9+41*3]=col[10+41*3]=255;
    col[8+81*3]=55;col[9+81*3]=51;col[10+81*3]=54;
    screen.LoadPalette(GamePalette(col));CHECK(ButtonColors(screen.Colors()).edge==81);
}

// 출력 선의 끝점 포함·역순·겹침·화면 밖 클리핑을 손으로 지정한 픽셀 배열과 비교한다.
TEST_CASE(GumpVisual_LinePixelsIncludeBothEndsAndClip) {
    IndexedImage canvas{6,5,std::vector<std::uint8_t>(30,9),std::vector<std::uint8_t>(30,255)};
    const std::array<GumpLine,5> lines{{{{4,1},{1,1},257},{{2,-7},{2,3},2},{{-3,4},{1,4},3},
        {{-5,0},{-5,4},4},{{std::numeric_limits<int>::min(),0},{std::numeric_limits<int>::max(),0},5}}};
    DrawGumpLines(canvas,lines);
    const std::vector<std::uint8_t> expected{5,5,5,5,5,5, 9,1,2,1,1,9, 9,9,2,9,9,9, 9,9,2,9,9,9, 3,3,9,9,9,9};
    CHECK(canvas.indices==expected);CHECK(canvas.opacity==std::vector<std::uint8_t>(30,255));
}

// 아직 파일을 읽지 않은 표, 잘못된 그림 크기, 원본 버튼이 쓰지 않는 대각선은 안전 API에서 거부한다.
TEST_CASE(GumpVisual_RejectsInvalidOutputInputs) {
    bool rejected=false;try { (void)ButtonColors({}); } catch (const std::invalid_argument&) { rejected=true; } CHECK(rejected);
    IndexedImage canvas{2,2,{0,0,0}, {}};rejected=false;
    try { DrawGumpLines(canvas,{}); } catch (const std::invalid_argument&) { rejected=true; } CHECK(rejected);
    canvas.indices.push_back(0);const std::array<GumpLine,1> diagonal{{{{0,0},{1,1},7}}};rejected=false;
    try { DrawGumpLines(canvas,diagonal); } catch (const std::invalid_argument&) { rejected=true; } CHECK(rejected);
}
