// 저장한 실제 세 PE 관찰과 독립 한 픽셀 캐시로 출력 순서/폭/클립/현재 문맥을 검사한다.
#include "GumpRawSupport.h"
#include "client/GumpTextDraw.h"
#include <chrono>

namespace {
using namespace netstorm::test::gumpraw;
// 실제 FontStore가 조회하는 원본 캐시 파일 이름 구성이다. 검사 소유 임시 경로만 쓴다.
constexpr std::array<int,7> kHeights{14,13,0,20,48,14,12},kWeights{700,0,0,700,0,0,0};
constexpr std::array<const char*,5> kStyles{"normal","italic","bold","strikeout","underline"};
// 첫 폭 99/ABC A -31과 독립 입력의 세 번째 폭, 한 픽셀/특별 빈 프레임을 저장한다.
std::vector<std::uint8_t> Cache(std::span<const std::uint8_t> advances,std::span<const std::uint8_t> mask) {
    if (advances.size()!=1024 || mask.size()!=256) throw std::runtime_error("글리프 입력 표 크기 오류");
    std::vector<std::uint8_t> cache(4132);
    // NUL까지 포함하는 원본 16바이트 캐시 서명이다.
    constexpr std::string_view signature{"BitmapFontData\x1a\0",16};
    std::copy(signature.begin(),signature.end(),cache.begin());
    Put(cache,16,1);Put(cache,20,31);Put(cache,24,26);Put(cache,28,5);Put(cache,32,256);
    std::copy(advances.begin(),advances.end(),cache.begin()+36+2048);
    // 포인터 유무는 실제 캐시 블록 바이트 수에서 FontStore가 복원한다.
    for (std::size_t code=0;code<256;++code) {
        Put(cache,36+code*4,99);Put(cache,36+1024+code*4,0xffffffe1U);
        if (!mask[code]) continue;
        std::vector<std::uint8_t> shape(40);Put(shape,0,0x30312e31);Put(shape,4,1);Put(shape,8,16);
        if (code==32) {
            Put(shape,24,0x7fffffff);Put(shape,28,0x7fffffff);Put(shape,32,0x80000001);Put(shape,36,0x80000001);
        } else shape.insert(shape.end(),{2,100,0});
        Put(cache,36+3072+code*4,static_cast<std::uint32_t>(shape.size()));
        cache.insert(cache.end(),shape.begin(),shape.end());
    }
    return cache;
}
// 검사 입력 캐시를 실제 FontStore에 공급하고 정상 종료/생성 실패 때 소유 파일을 정리한다.
struct GlyphFixture {
    // directory: 디렉터리. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::filesystem::path directory;
    // files: 디스크·보조 디렉터리·아카이브 순서로 게임 파일을 찾는 파일 시스템 (netstorm::o::BaseFileSystem).
    netstorm::o::BaseFileSystem files;
    // fonts: 이름·크기로 찾아 재사용하는 글꼴 저장소. 입력 구성 또는 실행 결과 대조에 사용한다.
    FontStore fonts;
    // cases: 사례. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::vector<std::string>> cases;
    // identities: 식별정보. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<const BitmapFont*,std::pair<std::uint32_t,std::uint32_t>> identities;
    // 기존 경로와 충돌하면 소유권을 얻지 않고 실패한다.
    static std::filesystem::path MakeDirectory() {
        auto path=std::filesystem::temp_directory_path()/std::filesystem::path("netstorm-gumpglyph-"+
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        if (!std::filesystem::create_directory(path)) throw std::runtime_error("글리프 검사 경로 생성 실패");
        return path;
    }
    // 원본 입력 표는 캐시로 쓰고 출력 행은 계산 없이 저장한다.
    GlyphFixture():directory(MakeDirectory()),files(directory),fonts(files,"Arial") {
        try {
            std::filesystem::create_directory(directory/"d");
            std::ifstream input(NETSTORM_GUMPGLYPH_FIXTURE);
            if (!input) throw std::runtime_error("글리프 fixture 없음");
            std::string line;std::vector<std::pair<std::uint32_t,std::uint32_t>> keys;
            // 입력 캐시/관찰 출력 행을 각각 원본 형식 그대로 읽는다.
            while (std::getline(input,line)) {
                if (line.empty() || line[0]=='#') continue;
                auto row=Split(line,'\t');
                if (row[0]=="Case") { cases.push_back(std::move(row));continue; }
                if (row.size()!=5 || row[0]!="Font") throw std::runtime_error("글리프 fixture 입력 행 오류");
                const auto slot=Number(row[1]),style=Number(row[2]);
                const auto cache=Cache(Bytes(row[3]),Bytes(row[4]));
                const auto face=slot==1 ? "Courier New" : "Arial";
                const auto name=std::string("!")+face+'.'+kStyles.at(style)+'.'+std::to_string(kHeights.at(slot))+'.'+std::to_string(kWeights.at(slot))+".chfnt";
                std::ofstream output(directory/"d"/name,std::ios::binary);
                output.write(reinterpret_cast<const char*>(cache.data()),static_cast<std::streamsize>(cache.size()));
                if (!output) throw std::runtime_error("글리프 검사 캐시 저장 실패");
                keys.emplace_back(slot,style);
            }
            // 모든 실제 FontStore 인스턴스의 정체성을 출력 콜백에서 기록한다. 선택 기대값은 계산하지 않는다.
            for (const auto [slot,style]:keys) identities.emplace(&fonts.Get(static_cast<int>(slot),static_cast<FontStyle>(style)),std::pair{slot,style});
        } catch (...) { std::filesystem::remove_all(directory);throw; }
    }
    // 성공적으로 소유한 임시 경로만 지운다. 실제 원본/공유 디렉터리는 대상이 아니다.
    ~GlyphFixture() { std::error_code error;std::filesystem::remove_all(directory,error); }
};
// 독립적인 양수 전진 폭과 한 픽셀 A/특별 빈 공백/코드 0을 가진 폰트다.
std::unique_ptr<BitmapFont> PixelFont() {
    std::vector<std::uint8_t> widths(1024),mask(256);
    Put(widths,65*4,2);Put(widths,32*4,3);Put(widths,0,1);
    mask[65]=mask[32]=mask[0]=1;
    return std::make_unique<BitmapFont>(Cache(widths,mask));
}
}

// 표 선택/빈 표/없는 포인터/전진 폭/그림자/외곽선/길이/상대 위치를 실제 PE의 사건 전수와 대조한다.
TEST_CASE(GumpGlyph_ThreeNativeEditionsMatchFontStoreAndGumpDelegation) {
    GlyphFixture fixture;std::array<std::size_t,3> counts{};
    // 각 행을 독립 저장 입력/기대 사건으로 재생한다. 실제 게임/원본/Python 없이 동작한다.
    for (const auto& row:fixture.cases) {
        CHECK(row.size()==15);if (row.size()!=15) continue;
        const auto edition=row[1]=="originals" ? 0U : row[1]=="originalCD" ? 1U : 2U;
        std::vector<std::string> trace;std::uint8_t color=0;GumpGlyphHooks hooks;
        hooks.color=[&](std::uint8_t value) { color=value;trace.push_back("C,"+std::to_string(value)); };
        hooks.illegalStyle=[&] { trace.emplace_back("E"); };
        // 콜백은 실제로 선택된 FontStore 객체와 원본 좌표 비트를 기록한다.
        hooks.glyph=[&](const BitmapFont& font,std::uint8_t code,ScreenPoint at) {
            const auto [slot,style]=fixture.identities.at(&font);
            trace.push_back("G,"+std::to_string(slot)+','+std::to_string(style)+','+std::to_string(code)+','+
                std::to_string(std::uint32_t(at.x))+','+std::to_string(std::uint32_t(at.y))+','+std::to_string(color));
        };
        const GumpTextContext context{Number(row[5]),Number(row[3]),Number(row[6]),Number(row[4]),0xabcdef01,
            Number(row[7]),std::stoi(row[8]),std::stoi(row[9])};
        const auto raw=Bytes(row[13]);const std::string bytes(raw.begin(),raw.end());
        const ScreenPoint offset{std::stoi(row[10]),std::stoi(row[11])};
        GumpTextDraw draw(fixture.fonts,hooks,edition==0 ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072);
        if (row[2]=="Direct") draw.Draw(bytes,offset,context,std::stoi(row[12]));
        else {
            GumpCreateNode root,node;node.x=-13;node.y=27;node.parent=1;
            node.textContext={context.flags,context.font,context.color,context.style,context.extra,context.shadowColor,
                std::uint32_t(context.shadowX),std::uint32_t(context.shadowY)};
            GumpCollectionState state;state.root=1;
            GumpCollection collection(state,{[&](auto token) -> GumpCollectionNode* { return token==1 ? &root : &node; },{}});
            draw.DrawGump(collection,2,bytes,offset,node.textContext,row[2]=="Default" ? nullptr : &context);
            CHECK(node.cached.x==-13 && node.cached.y==27);
        }
        std::string joined;
        // 768개의 원본 사건 문자열을 순서/개수/모든 필드까지 그대로 비교한다.
        for (const auto& event:trace) { if (!joined.empty()) joined+=';';joined+=event; }
        CHECK(joined==row[14]);++counts[edition];
    }
    CHECK((counts==std::array<std::size_t,3>{256,256,256}));
}

// 한 픽셀 손계산 기대값으로 특별 빈 공백의 전진과 없는 B의 비전진, 첫 폭/ABC A 미사용을 확인한다.
TEST_CASE(GumpGlyph_PixelsUseDrawAdvanceAndPreserveCurrentClipAndColorMap) {
    const auto font=PixelFont();GumpDrawState state;SetGumpDrawClip(state,{0,0,9,3});
    std::vector<std::uint8_t> pixels(30,7);GumpPixelTarget target(pixels,10,3,state);
    GumpGlyphColorState colors;colors.fontColors[17]=81;
    const auto hooks=MakeGumpGlyphPixelHooks(target,colors);
    const std::array<char,5> bytes{'A',' ','B','A',0};
    DrawBitmapGlyphText(bytes,font.get(),{1,1},{0,0,0x123456ab,0,0,0,0,0},hooks);
    CHECK(pixels[11]==171 && pixels[16]==171);
    CHECK(std::count(pixels.begin(),pixels.end(),171)==2);
    CHECK(colors.vfxColors[100]==171 && colors.vfxColors[17]==81 && colors.fontColors[100]==171);
    SetGumpDrawClip(state,{2,1,7,2});std::fill(pixels.begin(),pixels.end(),std::uint8_t{7});
    DrawBitmapGlyphText(bytes,font.get(),{1,1},{0,0,44,0,0,0,0,0},hooks);
    CHECK(pixels[11]==7 && pixels[16]==44 && std::count(pixels.begin(),pixels.end(),44)==1);
}

// 한 픽셀 글자의 그림자/외곽선 네 방향/본문과 특별 빈 문자열의 최종 색을 확인한다.
TEST_CASE(GumpGlyph_ShadowOutlineAndEmptyTextKeepColorInstallation) {
    const auto font=PixelFont();GumpDrawState state;SetGumpDrawClip(state,{0,0,5,5});
    std::vector<std::uint8_t> pixels(25,7);GumpPixelTarget target(pixels,5,5,state);
    GumpGlyphColorState colors;const auto hooks=MakeGumpGlyphPixelHooks(target,colors);
    const std::array<char,2> bytes{'A',0};
    DrawBitmapGlyphText(bytes,font.get(),{2,2},{6,0,211,0,0,33,1,1},hooks);
    CHECK(pixels[12]==211 && pixels[7]==33 && pixels[13]==33 && pixels[17]==33 && pixels[11]==33 && pixels[18]==33);
    CHECK(colors.vfxColors[100]==211);
    const auto saved=pixels;const std::array<char,1> empty{0};
    DrawBitmapGlyphText(empty,nullptr,{0,0},{6,0,77,0,0,55,1,1},hooks);
    CHECK(pixels==saved && colors.vfxColors[100]==77);
}

// 명시 길이는 중간 NUL도 그리며 잘못된 범위는 출력 전에 거부한다. 원본의 다른 음수 반복은 재현하지 않는다.
TEST_CASE(GumpGlyph_ExplicitNulAndInvalidRangesRespectHostContract) {
    const auto font=PixelFont();std::vector<std::uint8_t> colors;std::vector<ScreenPoint> points;
    GumpGlyphHooks hooks;hooks.color=[&](auto value) { colors.push_back(value); };
    hooks.glyph=[&](const auto&,auto,auto at) { points.push_back(at); };
    const std::array<char,3> bytes{'A',0,'A'};GumpTextContext context;context.color=99;
    DrawBitmapGlyphText(bytes,font.get(),{0,0},context,hooks,3);
    CHECK(points.size()==3 && points[0].x==0 && points[1].x==2 && points[2].x==3);
    points.clear();colors.clear();
    CHECK(Throws([&] { DrawBitmapGlyphText(std::span(bytes).first(1),font.get(),{},context,hooks); }));
    CHECK(Throws([&] { DrawBitmapGlyphText(bytes,font.get(),{},context,hooks,4); }));
    CHECK(Throws([&] { DrawBitmapGlyphText(bytes,font.get(),{},context,hooks,-2); }));
    CHECK(points.empty() && colors.empty());
}

// 월드 투영 경계가 기본 문맥을 바꾼 뒤 wrapper가 현재 문맥을 해석하며 클립 밖 글자도 색을 설치한다.
TEST_CASE(GumpGlyph_GumpPositionReadsCurrentDefaultContextAfterProjection) {
    GlyphFixture fixture;GumpCreateNode node;node.flags=0x2000;node.parent=1;node.object=50;
    node.textContext={0,2,11,0,0,0,0,0};GumpCollectionState state;state.root=1;
    GumpCollection collection(state,{[&](auto) -> GumpCollectionNode* { return &node; },[&](auto sid) {
        CHECK(sid==50);node.textContext[2]=77;return ScreenPoint{4,8};
    }});
    std::vector<std::uint8_t> installed;GumpGlyphHooks hooks;
    hooks.color=[&](auto value) { installed.push_back(value); };hooks.glyph=[](const auto&,auto,auto) { CHECK(false); };
    GumpTextDraw draw(fixture.fonts,hooks);const std::array<char,2> bytes{'A',0};
    draw.DrawGump(collection,2,bytes,{2,3},node.textContext);
    CHECK((installed==std::vector<std::uint8_t>{77}));
    GumpTextContext explicitContext;explicitContext.font=2;explicitContext.color=33;
    draw.DrawGump(collection,2,bytes,{2,3},node.textContext,&explicitContext);
    CHECK((installed==std::vector<std::uint8_t>{77,33}));
}

// 기존 메뉴 Renderer도 같은 글리프 폭/특별 빈 프레임/그림자/클립 출력을 사용한다.
TEST_CASE(GumpGlyph_RendererUsesSharedBitmapLoop) {
    const auto font=PixelFont();Renderer renderer(10,3);std::vector<std::uint8_t> pixels(30,7);
    renderer.SetScene({},{{font.get(),"A BA",1,1,171,33,{1,1},true,false}});
    renderer.Draw(pixels,10);renderer.Present([](auto) {});
    CHECK(pixels[11]==171 && pixels[16]==171 && pixels[22]==33 && pixels[27]==33);
}
