// 원본 메시지 표/기한/idle와 실제 StyleText/TextGump/Renderer 연결을 검사한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "client/MessageBoard.h"
#include "client/WarningMessages.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/GumpTextDraw.h"
#include "client/PaletteShade.h"
#include <sstream>
#include "client/DialogScript.h"
#include "o/GameClock.h"
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
namespace {
// 공개 실제 Report의 생존 행이다. 표시/검색 결과를 별도 상태로 계산하지 않는다.
struct WarningRow {
    std::uint32_t serial{};int x{},y{},width{},height{};double deadline{};std::string source; // 원시 실제 노드 관측이다.
};
// 게임/창 없이 실제 Client와 동일한 등록/트리/삭제/StyleText/가상 Draw를 구성한다.
struct WarningClient {
    const netstorm::test::dudespawn::DudeAssets& assets; // 보호 자산의 읽기 전용 공유 수명이다.
    FontStore fonts;PaletteColorTable palette;RawButtonControls buttons; // 실제 글꼴/색/단일 Gump 소유자다.
    RawMenuControls* menus{};WarningMessages* messages{}; // 위 소유자의 같은 처리기를 빌린다.
    int width{},height{}; // 현재 실제 표면 크기다.
    // 같은 공통 가상 분배가 설치된 뒤에만 안내를 생성한다.
    WarningClient(std::function<double()> now,std::function<void()> sound,MessageGumpHooks context={},int w=1024,int h=768)
        :assets(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078)),fonts(assets.files,"Arial"),palette(WarningPalette(assets.game)),buttons(OriginalEdition::Patch1078),width(w),height(h) {
        RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,auto&) {};hooks.sound=[](auto) {};
        hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));messages=&buttons.EnableWarnings(std::move(now),std::move(sound),std::move(context));Reset();
    }
    // 호스트 페이지 전환도 같은 실제 공통 Kill/Cleanup으로 이전 자식을 반납한다.
    void Reset(bool preserve=false) { buttons.SetControls({}, {},width,height,{},preserve);messages->SetView(width,height,palette); }
    // 기존 RawButtons 원시 필드에서 실제 등록/serial/idle 수를 읽는다.
    std::array<std::uint32_t,6> Fields() const {
        std::istringstream input(buttons.Report());std::string tag;input>>tag;std::array<std::uint32_t,6> result{};
        for (auto& value:result) input>>value; // 실제 출력 순서의 여섯 DWORD다.
        return result;
    }
    // Warning 보고의 실제20칸 순서를 보존해 읽는다.
    std::vector<WarningRow> Rows() const {
        std::vector<WarningRow> result;std::istringstream report(messages->Report());std::string line;
        while (std::getline(report,line)) { // count/registry 행은 메시지로 해석하지 않는다.
            std::istringstream input(line);std::string tag;std::size_t index{};WarningRow row;input>>tag;if (tag!="warning") continue;
            input>>index>>row.serial>>row.x>>row.y>>row.width>>row.height>>row.deadline;input.get();std::getline(input,row.source);result.push_back(std::move(row));
        }
        return result;
    }
    // 현재 Client의 모든 raw 파생을 원본 공통 수집/정렬/클립으로 실제 표면에 그린다.
    std::vector<std::uint8_t> Pixels(std::optional<ScreenRect> clip={}) const {
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width)*height,7);std::array<std::uint32_t,256> logical{};
        for (std::size_t i=0;i<logical.size();++i) { const auto c=assets.game.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 실제 파일 팔레트 순서다.
        menus->Draw(pixels,width,height,palette,BuildGumpShadeMaps(logical),clip);return pixels;
    }
};
// Pause의 현재 실제 모드/이름/번역 경계다. 게임 시계나 Gump를 대신 소유하지 않는다.
MessageGumpHooks PauseContext() {
    MessageGumpHooks context;context.challenge=[] { return 0U; };context.playerName=[](auto) {};context.translate=[](auto text) { return std::string(text); };return context;
}
}
// 실제 공통 글꼴/행/20칸 한도·정확한 만료·빈 칸 재사용을 확인한다.
TEST_CASE(Warnings_ActualStyleAndGumpLifetime) {
    double now=0.1;std::size_t sounds=0;WarningClient client([&] { return now; },[&] { ++sounds; });auto& messages=*client.messages;
    messages.Add("~B~E~yOur Priest has been Immobilized");messages.Add("~B~E~yOur Priest has been Immobilized");
    auto rows=client.Rows();CHECK(rows.size()==2 && sounds==2);CHECK(rows[0].source=="~B~E~yOur Priest has been Immobilized");CHECK(rows[0].y==0 && rows[1].y==12);
    CHECK(client.Fields()[1]==3 && client.Fields()[4]==2);CHECK(messages.Node(client.buttons.SerialGump(rows[0].serial))->parent==client.buttons.RootToken());
    for (int i=0;i<19;++i) messages.Add("~E~yWarning"); // 같은 Warning도 독립 안내이며20개 이후에는 소리도 없다.
    CHECK(sounds==20 && client.Rows().size()==20);client.buttons.IdleGumps();client.buttons.CleanupGumps();
    now=std::nextafter(10.1,-std::numeric_limits<double>::infinity());CHECK(!client.buttons.IdleGumps());CHECK(client.Rows().size()==20);
    now=10.1;CHECK(client.buttons.IdleGumps());CHECK(client.Rows().empty());CHECK(client.Fields()[1]==21 && client.Fields()[4]==0);
    client.buttons.CleanupGumps();CHECK(client.Fields()[1]==1);messages.Add("~E~yAgain");CHECK(client.Rows().size()==1 && client.Rows()[0].y==0 && sounds==21);
}
// Pause와 만료 Warning이 같은20칸/실제 글꼴/공통 삭제 수명을 공유한다.
TEST_CASE(Warnings_PauseSharesBoardAndPermanentText) {
    double now{};int sounds{},names{},translations{};auto context=PauseContext();
    context.playerName=[&](auto player) { CHECK(player==0);++names; };context.translate=[&](auto text) { CHECK(text=="~E~wGame Paused!");++translations;return std::string(text); };
    WarningClient client([&] { return now; },[&] { ++sounds; },std::move(context),640,480);auto& messages=*client.messages;
    messages.PauseStatus(true);messages.Add("~E~yWarning");const auto initial=client.Rows();CHECK(initial.size()==2 && names==1 && translations==1 && sounds==1);
    CHECK(initial[0].source=="~E~wGame Paused!" && initial[0].y==0 && initial[1].y==12);
    now=100;CHECK(client.buttons.IdleGumps());const auto paused=client.Rows();CHECK(paused.size()==1 && paused[0].source=="~E~wGame Paused!");
    messages.PauseStatus(false);CHECK(client.Rows().empty());client.buttons.CleanupGumps();CHECK(client.Fields()[1]==1);
    messages.PauseStatus(true);CHECK(client.Rows().size()==1 && names==2 && sounds==1);
}
// 실제 게임 시계 정지 중에도 같은 공통 Cleanup→Idle에서 벽시계 만료를 처리한다.
TEST_CASE(Warnings_SeparatedFrameStagesExpireWhileGameClockIsPaused) {
    netstorm::o::GameClock clock;std::uint32_t milliseconds=100;std::size_t reads{},sounds{};
    WarningClient client([&] { ++reads;return clock.WallSeconds(milliseconds); },[&] { ++sounds; },PauseContext(),640,480);auto& messages=*client.messages;
    messages.PauseStatus(true);messages.Add("~E~yTimed warning");const auto rows=client.Rows();CHECK(rows.size()==2 && sounds==1);const auto initialReads=reads;
    const auto warningToken=client.buttons.SerialGump(rows[1].serial);const auto warningId=messages.Node(warningToken)->id;
    clock.Pause(milliseconds);milliseconds=10100;CHECK(clock.IsPaused() && clock.GameSeconds(milliseconds)==0.1);
    client.buttons.CleanupGumps();CHECK(reads==initialReads && client.Rows().size()==2);
    CHECK(client.buttons.IdleGumps());const auto permanent=client.Rows();CHECK(reads==initialReads+1 && clock.IsPaused());
    CHECK(permanent.size()==1 && permanent[0].source=="~E~wGame Paused!");CHECK(messages.Node(warningToken) && client.buttons.RegisteredGump(warningId)==warningToken);
    CHECK(messages.Node(warningToken)->flags&0x10000U);CHECK(client.buttons.SerialGump(rows[1].serial)==0);CHECK(!client.buttons.IdleGumps() && reads==initialReads+1);
    client.buttons.CleanupGumps();CHECK(!messages.Node(warningToken) && !client.buttons.RegisteredGump(warningId));CHECK(reads==initialReads+1 && client.Rows().size()==1);
    messages.PauseStatus(false);client.buttons.CleanupGumps();CHECK(clock.IsPaused() && reads==initialReads+1 && client.Rows().empty());
}
// 만료 Idle 뒤 아직 등록된 dead 안내의 칸을 새 메시지가 재사용해도 공통 소멸이 새 체인을 해제하지 않는다.
TEST_CASE(Warnings_SeparatedIdleReusesBoardSlotBeforeFollowingCleanup) {
    double now=0.1;std::size_t reads{},sounds{};WarningClient client([&] { ++reads;return now; },[&] { ++sounds; },{},640,480);auto& messages=*client.messages;
    messages.Add("~E~yFirst row");client.buttons.CleanupGumps();CHECK(client.Rows().size()==1 && reads==1);
    now=10.1;client.buttons.CleanupGumps();CHECK(client.Rows().size()==1 && reads==1);CHECK(client.buttons.IdleGumps());CHECK(client.Rows().empty() && reads==2);
    messages.Add("~E~yReused row");const auto replacement=client.Rows();CHECK(replacement.size()==1 && replacement[0].source=="~E~yReused row" && replacement[0].y==0);CHECK(reads==3 && sounds==2);
    const auto before=client.Pixels();client.buttons.CleanupGumps();CHECK(reads==3 && client.Rows().size()==1 && before==client.Pixels());
    CHECK(!client.buttons.IdleGumps() && reads==4);now=100;CHECK(client.buttons.IdleGumps() && reads==5 && !messages.HasMessages());client.buttons.CleanupGumps();CHECK(client.Fields()[1]==1);
}
// Renderer용 별도 글자 목록 없이 실제 공통 Draw의 스타일·그림자·내부/외부 클립을 비교한다.
TEST_CASE(Warnings_CommonDrawStyleAndClip) {
    WarningClient client([] { return 0.0; },[] {},{},320,40);client.messages->Add("~B~E~yWarning");const auto rows=client.Rows();CHECK(rows.size()==1);
    const ScreenRect clip{rows[0].x+1,1,rows[0].x+30,9};const auto actual=client.Pixels(clip);std::vector<std::uint8_t> expected(320*40,7);
    GumpDrawState draw;SetGumpDrawClip(draw,clip);GumpPixelTarget target(expected,320,40,draw);GumpGlyphColorState colors;GumpTextDraw glyphs(client.fonts,MakeGumpGlyphPixelHooks(target,colors));
    const GumpTextContext context{2,0,client.palette.basic[5],static_cast<std::uint32_t>(FontStyle::Bold),0,client.palette.basic[0],1,1};
    const std::string text="Warning";glyphs.Draw(std::span(text.c_str(),text.size()+1),{rows[0].x,0},context);CHECK(actual==expected);
    std::size_t changed=0;
    for (int y=0;y<40;++y) for (int x=0;x<320;++x) if (actual[static_cast<std::size_t>(y)*320+x]!=7) { CHECK(x>=clip.left && x<clip.right && y>=clip.top && y<clip.bottom);++changed; } // 실제 표면 guard다.
    CHECK(changed>0);
}
// 메뉴 형제와 안내가 같은 serial/등록표를 사용하고 공통 Kill 뒤 실제 free가 일어난다.
TEST_CASE(Warnings_CommonRegistryMixedMenuAndGlobalKill) {
    WarningClient client([] { return 0.0; },[] {});client.menus->Open({200,100},[](const auto& widgets,const auto& controls,const auto&) { widgets.BeginVertical();controls.AddText("Menu sibling"); });
    const auto menuFields=client.Fields();client.messages->Add("~E~ySame Client");const auto row=client.Rows().at(0);const auto token=client.buttons.SerialGump(row.serial);const auto id=client.messages->Node(token)->id;
    CHECK(client.Fields()[1]==menuFields[1]+1 && row.serial>menuFields[2]);CHECK(client.buttons.RegisteredGump(id)==token);CHECK(client.messages->Lookup(row.serial)->token==token);
    CHECK(!client.messages->Lookup(menuFields[2]));CHECK(client.messages->Node(token)->parent==client.buttons.RootToken());const auto menuAndMessage=client.Pixels();
    client.buttons.KillGump(token);CHECK(!client.messages->HasMessages() && client.messages->Node(token));CHECK(client.buttons.RegisteredGump(id)==token);CHECK(client.menus->HasMenus());
    client.buttons.CleanupGumps();CHECK(!client.messages->Node(token) && !client.buttons.RegisteredGump(id));CHECK(client.Fields()[1]==menuFields[1]);CHECK(menuAndMessage!=client.Pixels());
    client.menus->Close();client.buttons.CleanupGumps();CHECK(client.Fields()[1]==1);
}
// 원본 기본 Text(-40)보다 앞 메뉴(-60)가 안내를 가리며 닫으면 같은 체인이 다시 보인다.
TEST_CASE(Warnings_CommonMenuOccludesByNativeDepth) {
    WarningClient client([] { return 0.0; },[] {},PauseContext());client.messages->PauseStatus(true);const auto row=client.Rows().at(0);
    const auto warning=client.buttons.SerialGump(row.serial);CHECK(client.messages->Node(warning)->depth==0xffffffd8U);const auto onlyMessage=client.Pixels();
    const auto open=[&] { client.menus->Open({1024,0},[](const auto& widgets,const auto& controls,const auto&) { widgets.BeginVertical();controls.AddText("Native menu in front"); }); }; // 실제 메뉴 생성/화면 끝 정렬이다.
    open();std::uint32_t front{};
    for (std::uint32_t id=1;id<kGumpRegistryCapacity;++id) { const auto token=client.buttons.RegisteredGump(id);const auto* node=client.menus->Node(token);if (node && node->parent==client.buttons.RootToken()) { front=token;break; } } // 실제 등록표의 직계 메뉴를 찾는다.
    CHECK(front!=0);const auto* menu=client.menus->Node(front);CHECK(menu && menu->depth==0xffffffc4U);CHECK(menu && menu->x<=row.x && menu->x+menu->width>=row.x+row.width && menu->y==0 && menu->height>=row.height);
    const auto covered=client.Pixels();CHECK(covered!=onlyMessage && client.messages->HasMessages());client.menus->Close();client.buttons.CleanupGumps();CHECK(client.Pixels()==onlyMessage && client.messages->HasMessages());
    open();CHECK(client.Pixels()==covered);client.buttons.KillGump(warning);client.buttons.CleanupGumps();CHECK(client.Pixels()==covered && !client.messages->HasMessages());
}
// 같은 페이지 재구성의 보존/전체 정리와 오래된20칸의 후속 재사용을 실제 부모에서 확인한다.
TEST_CASE(Warnings_CommonPageClearAndStaleBoard) {
    std::size_t sounds{};WarningClient client([] { return 0.0; },[&] { ++sounds; });
    for (int i=0;i<20;++i) client.messages->Add("~E~yOld page"); // 원본 표를 실제 안내로 가득 채운다.
    const auto old=client.Rows();CHECK(old.size()==20);client.Reset(true);CHECK(client.Rows().size()==20 && client.Fields()[1]==21);
    client.Reset();CHECK(client.Rows().empty() && client.Fields()[1]==1 && client.Fields()[4]==0);
    client.messages->Add("~E~yNew page");const auto fresh=client.Rows();CHECK(fresh.size()==1 && fresh[0].y==0 && fresh[0].serial>old.back().serial && sounds==21);
    CHECK(client.Pixels()!=std::vector<std::uint8_t>(1024*768,7));
}
// 생성 중 음향 callback이 메뉴를 생성해도 새 serial/조각/현재 루트는 같은 소유자다.
TEST_CASE(Warnings_ReentrantSoundCreatesMenuInSameRegistry) {
    WarningClient* current{};std::size_t sounds{};WarningClient client([] { return 0.0; },[&] {
        ++sounds;current->menus->Open({200,100},[](const auto& widgets,const auto& controls,const auto&) { widgets.BeginVertical();controls.AddText("Created during Warning sound"); });
    });current=&client;client.messages->Add("~E~yAfter nested menu");const auto rows=client.Rows();CHECK(rows.size()==1 && sounds==1 && client.menus->HasMenus());
    const auto token=client.buttons.SerialGump(rows[0].serial);CHECK(token && client.messages->Node(token)->parent==client.buttons.RootToken());CHECK(client.Fields()[2]>rows[0].serial);
    const auto before=client.Pixels();client.menus->Close();client.buttons.CleanupGumps();CHECK(client.Rows().size()==1 && client.Fields()[1]==2 && before!=client.Pixels());
    client.Reset();CHECK(client.Fields()[1]==1 && !client.messages->HasMessages());
}
// 실제 tell 문법과 인자 안의 쉼표/꺾쇠/스타일을 보존하며 조건으로 숨긴 명령은 실행하지 않는다.
TEST_CASE(Warnings_InlineCommandParser) {
    const auto page=ParseDialogPage("<$Warning,\"~B~E~yOur Priest has been Immobilized\">");
    CHECK(page.warnings==std::vector<std::string>{"~B~E~yOur Priest has been Immobilized"});CHECK(page.paragraphs.empty() && page.items.empty());
    CHECK(ParseDialogPage("<$Warning,\"A, B > C\">").warnings==std::vector<std::string>{"A, B > C"});
    CHECK(ParseDialogPage("<?0><$Warning,\"Hidden\"></?>").warnings.empty());
}
