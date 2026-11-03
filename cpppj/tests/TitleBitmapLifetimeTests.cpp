// 원문startup 큰부모의독립관측과State42의부분효과/정상후반만검사한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "client/TitleBitmapLifetime.h"
#include "client/NativeIntroState.h"
#include "client/ClientTitleBitmaps.h"
#include "client/ClientStaticStrings.h"
#include "client/GifFileRead.h"
#include "FileProbeTestSupport.h"
#include "o/BaseFile.h"
#include <sstream>
using namespace netstorm::client;
namespace { // 기대출력은최초Native fixture에서만읽는다.
constexpr std::uint32_t kBase=0x12003000; //독립CPU의8칸bitmap불투명토큰시작이다.
std::string Decimal(double value) { std::ostringstream out;out<<value;auto text=out.str();if (text.find('.')==text.npos) text+=".0";return text; } //NativePython의이분정확0/1.25배수표기를사용한다.
std::string NativeWords(std::span<const std::uint32_t> words) { std::string text;for (auto value:words) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text; } //현재DWORD표만관측한다.
}

TEST_CASE(TitleBitmapLifetime_Native33SharedDimensionsPaintAndCleanup) { //33개원문입력의전체장치열/8칸/현재/깊이/게임시각을대조한다.
    using namespace netstorm::test::rawscene;std::ifstream input(NETSTORM_TITLE_BITMAP_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { //단축한대기나제품해독기로원문기대값을만들지않는다.
        const auto row=Split(line,'\t');CHECK(row.size()==8);if (row.size()!=8) return;const auto p=Split(row[0],',');CHECK(p.size()==7);if (p.size()!=7) return;
        const auto mode=Number(p[0]),skip=Number(p[3]),fail=Number(p[4]),seed=Number(p[6]);std::int32_t width=static_cast<std::int32_t>(Number(p[1])),height=static_cast<std::int32_t>(Number(p[2])),current=std::bit_cast<std::int32_t>(Number(p[5]));std::uint32_t depth=3;double game=7.25,clock=0.;std::array<std::uint32_t,8> slots{};std::array<std::uint32_t,36> next;next.fill(0xa5a5a5a5U);
        for (std::size_t index=0;index<slots.size();++index) slots[index]=seed || mode==1 ? kBase+static_cast<std::uint32_t>(index)*4U:0U; //새상태의명시입력표다.
        std::uint32_t pops{};std::string events,outcome="ok";const auto event=[&](std::string text) { if (!events.empty()) events+=';';events+=text; };TitleBitmapHooks h;
        h.pump=[&] { event("P"); };h.input=[&] { const auto code=skip && ++pops==skip ? 65U:0U;if (!skip) ++pops;event("I:"+std::to_string(code));return code; };h.game=[&] { event("Gtime:123.5");return 123.5; };h.wall=[&] { const auto value=clock;event("W:"+Decimal(value));clock+=1.25;return value; };
        h.load=[&](auto name,auto& w,auto& hh) { event("N:"+std::string(name));const auto index=name=="title0.gif" ? 0U:name=="title6.gif" ? 6U:7U;event("G:"+std::to_string(index));if (fail==index+1) throw std::runtime_error("load"+std::to_string(index));w=index==0 ? 100:index==6 ? 120:111;hh=index==0 ? 80:index==6 ? 90:81;return kBase+index*4U; }; //6/7은서로다른치수여서같은마지막출력쌍을쓰는원문을확인한다.
        h.palette=[&](auto name) { event("N:"+std::string(name));event("A:"+std::string(name));if (fail==20) throw std::runtime_error("palette"); };
        h.clip=[&](auto r) { event("C:"+NativeWords(std::array<std::uint32_t,4>{std::uint32_t(r.left),std::uint32_t(r.top),std::uint32_t(r.right),std::uint32_t(r.bottom)})); };
        h.lock=[&] { event("L"); };h.unlock=[&] { event("U"); };h.present=[&] { event("V"); };h.flip=[&] { event("F"); };h.frontLock=[&] { event("FL"); };h.frontUnlock=[&] { event("FU"); };
        const auto copy=[&](const TitleBitmapCopy& c,std::string_view name) { const std::array<std::uint32_t,8> words{{std::uint32_t(c.x),std::uint32_t(c.y),c.bitmap,std::uint32_t(c.stride),std::uint32_t(c.source.left),std::uint32_t(c.source.top),std::uint32_t(c.source.right),std::uint32_t(c.source.bottom)}};event(std::string(name)+":"+NativeWords(words)); }; //raw스트라이드/위치/끝값을그대로관측한다.
        h.copy=[&](const auto& c) { copy(c,"B"); };h.frontCopy=[&](const auto& c) { copy(c,"FB"); };h.fill=[&](auto r,auto color) { event("Z:"+NativeWords(std::array<std::uint32_t,5>{std::uint32_t(r.left),std::uint32_t(r.top),std::uint32_t(r.right),std::uint32_t(r.bottom),color})); };h.release=[&](auto token) { event("R:"+std::to_string(token)); };
        try { TitleBitmapGlobals g{slots,current,depth,width,height,game};if (mode==0) RunTitleBitmaps(g,h);else if (mode==1) RedrawTitleBitmap(g,h);else RequestInitialTitleBitmap(next); }catch (const std::runtime_error& failure) { outcome=failure.what(); }
        CHECK(outcome==row[1] && std::uint32_t(current)==Number(row[2]) && depth==Number(row[3]) && game==std::stod(row[4]) && NativeWords(slots)==row[5] && (events.empty()?"-":events)==row[6] && Hex(std::span(reinterpret_cast<const std::uint8_t*>(next.data()),sizeof(next)))==row[7]);++rows;
    }
    CHECK(rows==33);
}

TEST_CASE(TitleBitmapLifetime_State42Requests41AndRestoresPaletteAfterSplash) { //실제case42의cursor/큰부모/요청/flag0/팔레트/복귀순서다.
    std::array<netstorm::o::LocalMissionStateBlock,5> blocks{};netstorm::o::LocalMissionStateStorage state{blocks[0],blocks[1],blocks[2],blocks[3],blocks[4]};state.current[0]=42;state.immediate[35]=0x12345678;bool fort{},server{},boss{};std::array<std::uint8_t,4> pending{'K','e','e','p'};std::string trace;NativeIntroStateHooks h;
    h.titleCursor=[&](bool show) { trace+=show ? 'S':'H'; };h.titleSplash=[&] { trace+='T';CHECK(state.immediate[35]==0x12345678); };h.request=[&](auto selected) { trace+='R';state.immediate[0]=selected; };h.restoreTitlePalette=[&] { trace+='P';CHECK(state.immediate[0]==41 && state.immediate[35]==0); };
    CHECK(InstallNativeIntroState({state,fort,server,boss,pending},h)==netstorm::o::LocalMissionParentResult::Commit && trace=="HTRPS" && !fort && !server && !boss && pending[0]=='K');
    state.immediate[0]=0;state.immediate[35]=0xa5a5a5a5;trace.clear();h.titleSplash=[&] { trace+='T';throw std::runtime_error("명시title파일중단"); };bool stopped{};try { InstallNativeIntroState({state,fort,server,boss,pending},h); }catch (const std::runtime_error&) { stopped=true; }CHECK(stopped && trace=="HT" && !state.immediate[0] && state.immediate[35]==0xa5a5a5a5); //실패앞숨김을정상보임/팔레트복구로롤백하지않는다.
    trace.clear();h.titleSplash=[&] { trace+='T';throw TitleBitmapQuit(); };bool quit{};try { InstallNativeIntroState({state,fort,server,boss,pending},h); }catch (const TitleBitmapQuit&) { quit=true; }CHECK(quit && trace=="HT" && !state.immediate[0] && state.immediate[35]==0xa5a5a5a5); //실제WM_QUIT도State41성공으로만들지않고Run전용종료경계까지전달한다.
}

TEST_CASE(TitleBitmapLifetime_ActualRelativePathsReadSelectedUnicodeInstall) { // 설치 경로를 두 번 붙이던 결함을 실제 파일명 고리와 필수 파일 읽기로 검증한다.
    using namespace netstorm::test::fileprobe;
    const auto base=Work()/"title-path-782";
    for (std::uint8_t color=1;color<=2;++color) { // 서로 다른 설치 루트가 같은 상대 이름으로 각자의 파일만 읽어야 한다.
        const auto root=base/std::to_string(color);
        netstorm::o::BaseFileSystem files(root);
        ClientStaticStrings strings(files.FileNameOwner());
        for (const std::string_view literal:{"title0.gif","title6.gif","title7.gif"}) { // 실제 세 GIF 소비자가 사용하는 이름을 모두 확인한다.
            Write(root/"d"/literal,Gif(2,1,color));
            const auto before=strings.FileNames().Index();
            const auto path=FormatTitleBitmapPath(strings.FileNames(),literal);
            CHECK(path==std::string(".\\d\\")+std::string(literal));
            CHECK(strings.FileNames().Index()==(before+1)%10);
            std::int32_t width=-1,height=-1;
            const auto image=ReadBackgroundGifFileImage(files,path,width,height);
            CHECK(width==2 && height==1 && image.indices==std::vector<std::uint8_t>(2,color));
        }
        for (const std::string_view literal:{"title00.col","title06.col"}) { // 팔레트도 같은 상대 경로를 사용하며 별도 설치 접두를 붙이지 않는다.
            const std::array<std::uint8_t,3> bytes{{color,7,9}};
            Write(root/"d"/literal,bytes);
            const auto path=FormatTitleBitmapPath(strings.FileNames(),literal);
            CHECK(path==std::string(".\\d\\")+std::string(literal));
            CHECK(files.Read(path)==std::vector<std::uint8_t>(bytes.begin(),bytes.end()));
        }
    }
}
