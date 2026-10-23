// 원본 메시지 표/기한/idle와 실제 StyleText/TextGump/Renderer 연결을 검사한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "client/MessageBoard.h"
#include "client/WarningMessages.h"
#include "client/DialogScript.h"
#include <cmath>
#include <limits>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// Native 입력 노드 주소와 새 생성 경계의 토큰을 같은 의미로 사용한다.
constexpr std::uint32_t kNodes=0x12000000,kNew=kNodes+0x8000;
// double의 little endian 입력 비트를 십진 반올림 없이 읽는다.
double DoubleBits(std::string_view text) {
    std::uint64_t bits=0;
    // 각 원본 바이트를 지정된 순서로 조합한다.
    for (std::size_t i=0;i<8;++i) bits|=std::uint64_t(std::stoul(std::string(text.substr(i*2,2)),nullptr,16))<<(i*8);
    return std::bit_cast<double>(bits);
}
// 원본 파일 팔레트를 Screen과 같은 논리 RGB 표로 변환한다.
PaletteColorTable WarningPalette(const GameAssets& game) {
    std::array<std::uint32_t,256> logical{};
    // 파일 색 번호의 순서를 바꾸지 않는다.
    for (std::size_t i=0;i<logical.size();++i) {
        const auto c=game.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16);
    }
    logical[0]=0;logical[255]=0xffffff;return BuildPaletteColorTable(logical);
}
}
// 원본154입력의 전체20칸과 생성/삭제/부착 인자·순서를 대조한다.
TEST_CASE(Warnings_BoardNative154) {
    std::size_t count=0;
    // 기대 출력은 실제 PE fixture에서 읽으며 이 검사에서 검색 결과를 계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_WARNINGS_FIXTURE).rows) {
        if (row[0]!="Board") continue;CHECK(row.size()==11);MessageBoardState state;
        const auto serials=Split(row[7],',');CHECK(serials.size()==20);
        // 입력20칸을 원본과 같은 순서로 준비한다.
        for (std::size_t i=0;i<serials.size();++i) state.serials[i]=Number(serials[i]);
        state.suppressed=Number(row[5])!=0;state.screenWidth=1024;
        std::vector<std::pair<MessageBoardRecord,bool>> nodes;
        if (row[8]!="-") for (const auto& encoded:Split(row[8],';')) {
            const auto fields=Split(encoded,',');CHECK(fields.size()==5);
            nodes.push_back({{kNodes+static_cast<std::uint32_t>((nodes.size()+1)*0x100),Number(fields[0]),static_cast<int>(Number(fields[1])),static_cast<int>(Number(fields[2])),static_cast<int>(Number(fields[3]))},Number(fields[4])!=0});
        }
        std::string trace;
        // 원본 명시 경계의 인자와 순서를 동일한 문자열로 기록한다.
        const auto append=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; };
        MessageBoardHooks hooks;hooks.lookup=[&](auto serial)->std::optional<MessageBoardRecord> {
            // 원본 직계 자식 조회의 입력 순서와 dead 제외를 재생한다.
            for (const auto& [node,dead]:nodes) if (node.serial==serial && !dead) return node;
            return std::nullopt;
        };
        hooks.kill=[&](auto token) { append("K,"+std::to_string(token)); };
        hooks.create=[&](auto kind,auto player,auto parameter,std::string_view text) {
            CHECK(text=="warning");append("C,"+std::to_string(kNew)+','+std::to_string(kind)+','+std::to_string(player)+','+std::to_string(parameter));
            return MessageBoardRecord{kNew,999,kind,player,parameter};
        };
        hooks.width=[&](auto token) { CHECK(token==kNew);return static_cast<int>(std::stof(row[6])); };
        hooks.attach=[&](auto token,auto x,auto y) { append("A,"+std::to_string(token)+','+std::to_string(x)+','+std::to_string(y)); };
        MessageBoard(state,std::move(hooks)).Show(static_cast<int>(Number(row[1])),static_cast<int>(Number(row[2])),Number(row[3])!=0,static_cast<int>(Number(row[4])),"warning");
        std::string output;
        // 실제 변경된20칸을 그대로 비교하며 기대 표를 재계산하지 않는다.
        for (const auto serial:state.serials) { if (!output.empty()) output+=',';output+=std::to_string(serial); }
        CHECK(output==row[9]);CHECK((trace.empty() ? "-" : trace)==row[10]);++count;
    }
    CHECK(count==154);
}
// double 시각을 먼저 float으로 줄이는 잘못된 디컴파일 해석을 실제10입력으로 검출한다.
TEST_CASE(Warnings_DeadlineNative10) {
    std::size_t count=0;
    // 원본 기한의 전체64비트를 비교한다.
    for (const auto& row:LoadFixture(NETSTORM_WARNINGS_FIXTURE).rows) if (row[0]=="Deadline") {
        CHECK(std::bit_cast<std::uint64_t>(WarningDeadline(DoubleBits(row[1])))==std::bit_cast<std::uint64_t>(DoubleBits(row[2])));++count;
    }
    CHECK(count==10);
}
// 기한0/경계/NaN/무한대의 실제90입력과 동일한 Kill 여부를 확인한다.
TEST_CASE(Warnings_IdleNative90) {
    std::size_t count=0;
    // 원본이 비교한 두 double의 입력 비트를 그대로 제공한다.
    for (const auto& row:LoadFixture(NETSTORM_WARNINGS_FIXTURE).rows) if (row[0]=="Expired") {
        CHECK(WarningExpired(DoubleBits(row[1]),DoubleBits(row[2]))==(Number(row[3])!=0));++count;
    }
    CHECK(count==90);
}
// 실제 글꼴과 Gump 노드에서 스타일·독립 행·20칸 한도·정확한 만료·빈 칸 재사용을 확인한다.
TEST_CASE(Warnings_ActualStyleAndGumpLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(netstorm::o::OriginalEdition::Patch1078);FontStore fonts(assets.files,"Arial");const auto palette=WarningPalette(assets.game);
    double now=0.1;std::size_t sounds=0;WarningMessages messages(fonts,palette,[&] { return now; },[&] { ++sounds; });messages.SetView(1024,768,palette);
    messages.Add("~B~E~yOur Priest has been Immobilized");messages.Add("~B~E~yOur Priest has been Immobilized");
    auto text=messages.Text();CHECK(text.size()==2);CHECK(sounds==2);
    CHECK(text[0].bytes=="Our Priest has been Immobilized");CHECK(text[0].font==&fonts.Get(0,FontStyle::Bold));CHECK(text[0].shadow && text[0].color==palette.basic[5]);
    CHECK(text[0].y==0 && text[1].y==12);CHECK(text[0].clip && text[0].clip->right==1024);CHECK(text[0].x==text[0].clip->left);
    // 같은 Warning을 추가해도 첫 안내를 교체하지 않으며20개 이후에는 소리도 재생하지 않는다.
    for (int i=0;i<19;++i) messages.Add("~E~yWarning");
    CHECK(sounds==20 && messages.Text().size()==20);CHECK(messages.Idle());
    now=std::nextafter(10.1,-std::numeric_limits<double>::infinity());CHECK(!messages.Idle());CHECK(messages.Text().size()==20);
    now=10.1;CHECK(messages.Idle());CHECK(messages.Text().empty());
    messages.Add("~E~yAgain");CHECK(messages.Text().size()==1 && messages.Text()[0].y==0 && sounds==21);
}
// TextGump 내부 클립을 Renderer가 실제 글자·그림자 픽셀에도 적용한다.
// Pause와 만료 Warning이 같은20칸 검색/실제 글꼴/삭제 수명을 공유하는지 확인한다.
TEST_CASE(Warnings_PauseSharesBoardAndPermanentText) {
    const auto& assets=netstorm::test::dudespawn::Assets(netstorm::o::OriginalEdition::Patch1078);FontStore fonts(assets.files,"Arial");const auto palette=WarningPalette(assets.game);
    double now{};int sounds{},names{},translations{};MessageGumpHooks context;
    context.challenge=[] { return 0U; };context.playerName=[&](auto player) { CHECK(player==0);++names; };
    context.translate=[&](auto text) { CHECK(text=="~E~wGame Paused!");++translations;return std::string(text); };
    WarningMessages messages(fonts,palette,[&] { return now; },[&] { ++sounds; },std::move(context));messages.SetView(640,480,palette);
    messages.PauseStatus(true);messages.Add("~E~yWarning");const auto initial=messages.Text();CHECK(initial.size()==2 && names==1 && translations==1 && sounds==1);
    CHECK(initial[0].bytes=="Game Paused!" && initial[0].y==0 && initial[1].y==12);
    now=100;CHECK(messages.Idle());const auto paused=messages.Text();CHECK(paused.size()==1 && paused[0].bytes=="Game Paused!");
    messages.PauseStatus(false);CHECK(messages.Text().empty());CHECK(messages.Idle());
    messages.PauseStatus(true);CHECK(messages.Text().size()==1 && names==2 && sounds==1);
}
// TextGump 내부 클립을 Renderer가 실제 글자·그림자 픽셀에도 적용한다.
TEST_CASE(Warnings_RendererTextClip) {
    const auto& assets=netstorm::test::dudespawn::Assets(netstorm::o::OriginalEdition::Patch1078);FontStore fonts(assets.files,"Arial");const auto palette=WarningPalette(assets.game);
    WarningMessages messages(fonts,palette,[] { return 0.0; },[] {});messages.SetView(320,40,palette);messages.Add("~B~E~yWarning");auto text=messages.Text();CHECK(text.size()==1);
    const ScreenRect clip{text[0].x+1,1,text[0].x+30,9};text[0].clip=clip;
    Renderer renderer(320,40);renderer.SetScene({},text);std::vector<std::uint8_t> pixels(320*40,7);renderer.Draw(pixels,320,7);std::size_t changed=0;
    // 클립 밖은 배경을 유지하며 내부에는 실제 글리프가 있어야 한다.
    for (int y=0;y<40;++y) for (int x=0;x<320;++x) if (pixels[static_cast<std::size_t>(y)*320+x]!=7) {
        CHECK(x>=clip.left && x<clip.right && y>=clip.top && y<clip.bottom);++changed;
    }
    CHECK(changed>0);
}
// 실제 tell 문법과 인자 안의 쉼표/꺾쇠/스타일을 보존하며 조건으로 숨긴 명령은 실행하지 않는다.
TEST_CASE(Warnings_InlineCommandParser) {
    const auto page=ParseDialogPage("<$Warning,\"~B~E~yOur Priest has been Immobilized\">");
    CHECK(page.warnings==std::vector<std::string>{"~B~E~yOur Priest has been Immobilized"});CHECK(page.paragraphs.empty() && page.items.empty());
    CHECK(ParseDialogPage("<$Warning,\"A, B > C\">").warnings==std::vector<std::string>{"A, B > C"});
    CHECK(ParseDialogPage("<?0><$Warning,\"Hidden\"></?>").warnings.empty());
}
