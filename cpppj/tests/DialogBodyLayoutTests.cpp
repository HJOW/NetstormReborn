// 보호 원본 폰트/팔레트를 읽기만 하여 Tell 본문의 실제 조각·높이·래핑·클립을 검사한다.
#include "TestSupport.h"
#include "client/DialogBodyLayout.h"
#include "client/GameAssets.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <filesystem>
#include <stdexcept>

using namespace netstorm::client;
namespace {
// 원본의 실제 파일 팔레트 RGB를 Screen 초기화와 같은 논리 색 번호 표에 연결한다.
PaletteColorTable BodyPalette(const netstorm::o::BaseFileSystem& files) {
    const GamePalette source(files.Read("d/gifcloud.col"));std::array<std::uint32_t,256> logical{};
    // 파일 색 번호를 바꾸지 않고 정확한 RGB로 색 검색을 수행한다.
    for (std::size_t index=0;index<logical.size();++index) {
        const auto color=source.Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16);
    }
    logical[0]=0;logical[255]=0xffffff;return BuildPaletteColorTable(logical);
}
// 합성 폭 표/GDI 창 없이 같은 원본 VFS·디스크 chfnt·COL을 공급하는 검사 자산이다.
struct BodyAssets {
    netstorm::o::BaseFileSystem files; // 보호 originals와 archive를 읽기 전용으로 조회한다.
    FontStore fonts; // 원본 Arial 캐시를 읽는 실제 글꼴 저장소다.
    PaletteColorTable palette; // 실제 COL의 기본/명명 색 번호 표다.
    std::unique_ptr<GameAssets> game; // 그림 검사 때만 원본 TYPE/SHP 전체를 읽는다.
    // archive를 등록한 뒤에만 같은 VFS로 폰트/팔레트를 읽는다.
    BodyAssets():files(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"),fonts(Register(),"Arial"),palette(BodyPalette(files)) {
        CHECK(files.TryRead("d/!Arial.normal.14.700.chfnt").has_value());CHECK(files.TryRead("d/!Arial.bold.14.700.chfnt").has_value());
    }
    // 생성 초기화 순서에서 원본 archive를 한 번 등록한다.
    const netstorm::o::BaseFileSystem& Register() {
        files.RegisterArchive(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals/netstorm.tarc");return files;
    }
    // TextGump 기본 생성과 같은 normal/font0/흰색/그림자(1,1) 문맥이다.
    std::array<std::uint32_t,8> Context() const { return {0,0,palette.basic[7],0,0,0,1,1}; }
    // 앞서 등록한 같은 VFS를 사용하며 실제 자산을 임의 타입으로 대체하지 않는다.
    GameAssets& Game() { if (!game) game=std::make_unique<GameAssets>(files,netstorm::o::OriginalEdition::Patch1078);return *game; }
};
// 검사마다 원본 파일을 다시 쓰거나 캐시를 중복 생성하지 않는다.
BodyAssets& Assets() { static BodyAssets assets;return assets; }
// 출력 순서의 문자열을 모으며 표시 줄의 LF는 별도 좌표로 보존한다.
std::string Plain(const DialogBodyLayoutResult& result) {
    std::string output;
    // 렌더러에 제출한 조각만 순서대로 읽는다.
    for (const auto& item:result.text) output+=item.bytes;
    return output;
}
// 문자열이 있는 실제 출력 조각을 찾으며 누락은 검사 예외로 표시한다.
const RenderText& Find(const DialogBodyLayoutResult& result,std::string_view bytes) {
    // 표시 순서에서 원문이 정확히 같은 첫 조각만 선택한다.
    const auto found=std::find_if(result.text.begin(),result.text.end(),[&](const auto& item) { return item.bytes==bytes; });
    if (found==result.text.end()) throw std::runtime_error("Tell 검사 출력 조각 누락");return *found;
}
}
// Early1 본문의 edge/open이 같은 행에서 실제 normal색/bold폰트와 정확한 전진 폭을 사용하는지 검사한다.
TEST_CASE(DialogBodyLayout_ActualInlineFontsColorsAndBaseline) {
    auto& assets=Assets();DialogBodyLayout body(assets.fonts,assets.palette);HtmlTextParserState state;HtmlTextParserGlobals globals;
    body.SetBody("walk <i>edge</i> and <b>open</b>.",assets.Context(),state,globals);const auto result=body.Layout(480,{20,30},{20,30,500,200});
    CHECK(Plain(result)=="walk edge and open." && result.lineCount==1 && result.height==14);
    const auto& normal=assets.fonts.Get(0);const auto& bold=assets.fonts.Get(0,FontStyle::Bold);const auto& edge=Find(result,"edge");const auto& open=Find(result,"open");
    const auto baseline=std::max(normal.Ascent(),bold.Ascent());CHECK(edge.font==&normal && open.font==&bold);
    CHECK(edge.x==20+normal.Measure("walk ") && open.x==20+normal.Measure("walk edge and "));
    CHECK(edge.y==30+baseline-normal.Ascent() && open.y==30+baseline-bold.Ascent());CHECK(!edge.shadow && !open.shadow);
    // i 태그가 실제 j색 목표 RGB를 썼는지 같은 원본 팔레트 표에서 조회한다.
    const auto color=std::find_if(assets.palette.named.begin(),assets.palette.named.end(),[](const auto& item) { return item.red==233 && item.green==214 && item.blue==186; });
    CHECK(color!=assets.palette.named.end());if (color!=assets.palette.named.end()) CHECK(edge.color==color->index);
    CHECK(open.color==assets.palette.basic[7]);CHECK(result.maximumWidth==normal.Measure("walk edge and .")+bold.Measure("open"));
    CHECK(edge.clip && edge.clip->left==20 && edge.clip->right==500);
}
// 폭만 바꾸어 기존 원본 버퍼의 실제 word-wrap/전체 높이를 다시 만들고 클립은 측정에 영향을 주지 않는다.
TEST_CASE(DialogBodyLayout_ActualBufferWrapAndIndependentClip) {
    auto& assets=Assets();DialogBodyLayout body(assets.fonts,assets.palette);HtmlTextParserState state;HtmlTextParserGlobals globals;
    body.SetBody("one two three four five six seven",assets.Context(),state,globals);
    const auto wide=body.Layout(480,{0,0},{0,0,480,500});const auto narrow=body.Layout(55,{0,0},{0,0,55,10});
    CHECK(wide.lineCount==1 && narrow.lineCount>wide.lineCount && narrow.height>wide.height);CHECK(Plain(wide)==Plain(narrow));
    const auto shifted=body.Layout(55,{10,25},{13,30,40,38});CHECK(shifted.height==narrow.height && shifted.maximumWidth==narrow.maximumWidth && shifted.lineCount==narrow.lineCount);
    CHECK(shifted.text.size()==narrow.text.size());
    // 같은 폭의 현재 표시 복제를 사용하되 출력 좌표와 clip은 매 요청 값이다.
    for (std::size_t index=0;index<std::min(shifted.text.size(),narrow.text.size());++index) {
        CHECK(shifted.text[index].x==narrow.text[index].x+10 && shifted.text[index].y==narrow.text[index].y+25);
        CHECK(shifted.text[index].clip && shifted.text[index].clip->top==30 && shifted.text[index].clip->bottom==38);
    }
}
// H1과 P/BR이 원본 폰트 슬롯/그림자/빈 표시 줄의 논리 높이를 그대로 보존한다.
TEST_CASE(DialogBodyLayout_ActualHeadingAndHardLineHeights) {
    auto& assets=Assets();DialogBodyLayout body(assets.fonts,assets.palette);HtmlTextParserState state;HtmlTextParserGlobals globals;
    body.SetBody("<h1>Title</h1><p>Body<br>End",assets.Context(),state,globals);const auto result=body.Layout(480,{0,0},{0,0,480,300});
    const auto& title=Find(result,"Title");CHECK(title.font==&assets.fonts.Get(4) && title.shadow && title.shadowOffset.x==1 && title.shadowOffset.y==1);
    CHECK(Find(result,"Body").font==&assets.fonts.Get(0) && !Find(result,"Body").shadow);CHECK(result.lineCount==4);
    CHECK(result.height==FontStore::LogicalHeight(4)+1+3*FontStore::LogicalHeight(0));
    CHECK(Find(result,"End").y-Find(result,"Body").y==FontStore::LogicalHeight(0));
}
// HTML 효과는 설정에서 한 번만 실행하고 미공급 그림/링크/배경·실패 뒤 기존 본문을 검증한다.
TEST_CASE(DialogBodyLayout_EffectsOnceAndFailedReplacementKeepsContent) {
    auto& assets=Assets();DialogBodyLayout body(assets.fonts,assets.palette);HtmlTextParserState state;HtmlTextParserGlobals globals;std::size_t calls=0;
    HtmlTextParserHooks hooks;hooks.command=[&](auto&,auto raw) { CHECK(raw=="raw 7>body");++calls; }; // 실제 큐 대신 호출 시점/원문 계약만 검증한다.
    body.SetBody("<$raw 7>body",assets.Context(),state,globals,hooks);CHECK(calls==1);
    CHECK(Plain(body.Layout(480,{0,0},{0,0,480,100}))=="body");body.Layout(40,{3,4},{3,4,40,50});CHECK(calls==1);
    // 그림 훅을 비우는 경우와 bracket 리소스가 미연결인 경우를 둘 다 거부한다.
    for (const auto* source:{"<!\"pic\">","<a href=\"target\">x","<background=\"cloud\">"}) {
        bool rejected=false;try { body.SetBody(source,assets.Context(),state,globals); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    }
    hooks={};hooks.picture=[](auto&,auto) { return std::string("~[Ipicture.0]"); }; // 실제 그림 공급 없이 성립하는 I bracket을 빈 성공으로 처리하면 실패해야 한다.
    bool rejected=false;try { body.SetBody("<!\"pic\">",assets.Context(),state,globals,hooks); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    CHECK(Plain(body.Layout(480,{0,0},{0,0,480,100}))=="body");
    body.SetBody("new",assets.Context(),state,globals);CHECK(Plain(body.Layout(480,{0,0},{0,0,480,100}))=="new");
}
// RenderText의 실제 폰트 글리프를 메모리 Renderer에 그려 내부 clip 밖 픽셀이 보존되는지 검사한다.
TEST_CASE(DialogBodyLayout_ActualFontRasterUsesContentClip) {
    auto& assets=Assets();DialogBodyLayout body(assets.fonts,assets.palette);HtmlTextParserState state;HtmlTextParserGlobals globals;
    body.SetBody("A long line crossing the content edge",assets.Context(),state,globals);const ScreenRect clip{10,8,55,18};
    const auto result=body.Layout(480,{5,6},clip);Renderer renderer(100,40);renderer.SetScene({},result.text);
    std::vector<std::uint8_t> pixels(4000,7);renderer.Draw(pixels,100,7);bool ink=false;
    // 창/장치를 만들지 않고 모든 픽셀의 clip 안/밖 여부를 직접 확인한다.
    for (int y=0;y<40;++y) for (int x=0;x<100;++x) {
        const auto value=pixels[static_cast<std::size_t>(y*100+x)];
        if (x<clip.left || x>=clip.right || y<clip.top || y>=clip.bottom) CHECK(value==7);else ink|=value!=7;
    }
    CHECK(ink);renderer.Present([](auto) {}); // 메모리 검사에서는 실제 화면으로 내보내지 않는다.
}
// 실제 Early1/2에 있는 모든 그림 형태를 자산 이름/논리 선택/물리 SHP/기준점/클립에 연결한다.
TEST_CASE(DialogBodyLayout_EarlyMissionActualPicturesAndHeaders) {
    auto& assets=Assets();auto& game=assets.Game();DialogBodyLayout body(assets.fonts,assets.palette,game);HtmlTextParserState state;HtmlTextParserGlobals globals;
    HtmlTextParserHooks hooks;BindHtmlPictureText(hooks);
    // 원본 tutorial1/2.english의 그림에 닫는 따옴표가 빠진 형태도 그대로 검사한다.
    for (const auto* source:{"<!\"geyser.*\">","<!\"nugget.*\">","<!\"priest.*\">","<!\"windvortex.*\">","<!\"sunFactory.*\">","<!\"mana.17\">","<!\"mana.11\">","<!\"mana.3\">","<!\"mana.8>","<!\"mana.9>","<!\"mana.10>"}) {
        const auto converted=HtmlPictureText(std::string_view(source).substr(1));const auto dot=converted.find('.');const auto name=converted.substr(3,dot-3);
        const auto& type=game.Find(name);const auto number=game.TypeTable().Find(name);CHECK(number>=0);
        auto frame=converted[dot+1]=='*' ? (type.definition.specialFrames.gumpFrame==-1 ? type.definition.specialFrames.defaultFrame : type.definition.specialFrames.gumpFrame) : std::stoi(converted.substr(dot+1));
        const auto count=static_cast<int>(type.definition.clusters.size());if (std::min(frame,count)<0) frame=0;else if (frame>=count) frame=count;
        body.SetBody(source,assets.Context(),state,globals,hooks);const auto result=body.Layout(480,{20,30},{20,30,500,230});
        if (frame>=count) {
            CHECK(result.sprites.empty() && result.maximumWidth==0 && result.height==14); // 원본 mana는8프레임뿐이므로 8이상은 clamp→Lookup거부다.
            continue;
        }
        if (result.sprites.size()!=1) std::fprintf(stderr,"Tell picture %s type %d selected %d default %d gump %d clusters %zu sprites %zu\n",name.c_str(),number,frame,type.definition.specialFrames.defaultFrame,type.definition.specialFrames.gumpFrame,type.definition.clusters.size(),result.sprites.size());
        CHECK(result.sprites.size()==1);
        if (result.sprites.size()!=1) continue;
        const auto& sprite=result.sprites[0];const auto metrics=game.Shapes().SquidMetrics(type.block,static_cast<std::size_t>(frame));const auto& rawType=game.TypeTable().Types()[static_cast<std::size_t>(number)];
        CHECK(sprite.database==&game.Shapes() && sprite.block==type.block && sprite.frame==static_cast<std::size_t>(frame));
        CHECK(sprite.x==20+metrics.hotspotX-rawType.hotspotX && sprite.y==30+std::max<int>(metrics.hotspotY-rawType.hotspotY,assets.fonts.Get(0).Ascent()));
        CHECK(sprite.clip.left==20 && sprite.clip.bottom==230 && !sprite.shadow && !sprite.colors);CHECK(result.maximumWidth==metrics.width && result.height==std::max<int>(14,metrics.height));
    }
}
// 실제 다중 줄 # 맞춤→외부Clone의 배율1 초기화와 폭 변경/페이지 교체를 같은 그림 저장소에서 수행한다.
TEST_CASE(DialogBodyLayout_ActualPictureBufferCloneAndRasterClip) {
    auto& assets=Assets();auto& game=assets.Game();DialogBodyLayout body(assets.fonts,assets.palette,game);HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;BindHtmlPictureText(hooks);
    body.SetBody("left <!\"geyser.*#\"> right",assets.Context(),state,globals,hooks);const ScreenRect clip{15,12,90,70};const auto result=body.Layout(480,{10,10},clip);CHECK(result.sprites.size()==1 && !result.text.empty());
    const auto narrow=body.Layout(70,{10,10},clip);CHECK(narrow.sprites.size()==1 && narrow.lineCount>=result.lineCount);
    Renderer renderer(120,90);renderer.SetScene(narrow.sprites,narrow.text,false);std::vector<std::uint8_t> pixels(10800,7);renderer.Draw(pixels,120,7);bool ink=false;
    // 그림과 실제 글꼴을 함께 그려도 같은 본문 clip 밖의 모든 픽셀을 보존한다.
    for (int y=0;y<90;++y) for (int x=0;x<120;++x) {
        const auto value=pixels[static_cast<std::size_t>(y*120+x)];if (x<clip.left || x>=clip.right || y<clip.top || y>=clip.bottom) CHECK(value==7);else ink|=value!=7;
    }
    CHECK(ink);renderer.Present([](auto) {}); // 실제 창/장치로 내보내지 않는 메모리 Renderer 검사다.
    body.SetBody("replacement",assets.Context(),state,globals);CHECK(body.Layout(480,{0,0},clip).sprites.empty());
}
