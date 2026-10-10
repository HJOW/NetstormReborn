// 원본 전체 배경/명암 생성 관찰과 실제 8비트 출력 경계를 검사한다. 게임·소리 장치를 실행하지 않는다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/GumpBackground.h"
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace netstorm::client;
namespace {
// 실제 x86이 남긴 팔레트/표 바이트를 읽는다. 기대 번호는 복원 알고리즘으로 계산하지 않는다.
std::vector<std::uint8_t> ReadHex(const std::string& hex) {
    std::vector<std::uint8_t> result(hex.size()/2);
    // 바이트 순서를 유지하여 예약 바이트까지 재생한다.
    for (std::size_t i=0;i<result.size();++i) result[i]=static_cast<std::uint8_t>(std::stoul(hex.substr(i*2,2),nullptr,16));
    return result;
}
// 복원한 출력 계획을 원본 경계 관찰 형식으로 직렬화한다. clip의 저장/복구도 같은 순서로 비교한다.
std::string Events(std::span<const GumpTexturePass> passes,ScreenRect clip) {
    std::ostringstream out;
    // 전체 질감 함수의 호출 순서대로 pass/clip/map/draw 사건을 기록한다.
    for (const auto& pass:passes) {
        if (out.tellp()!=std::streampos(0)) out<<'|';
        out<<"p,"<<pass.rect.left<<','<<pass.rect.top<<','<<pass.rect.right<<','<<pass.rect.bottom<<','<<pass.flags;
        if (!pass.validTile) continue;
        out<<"|c,"<<pass.clip.left<<','<<pass.clip.top<<','<<pass.clip.right<<','<<pass.clip.bottom;
        if (pass.shade) out<<"|m,"<<pass.shade;
        // VFX 호출은 빈 클립에서도 여분 타일을 포함해 원본과 같은 목록을 만든다.
        for (const auto p:pass.origins) out<<"|d,"<<p.x<<','<<p.y<<','<<pass.shade;
        out<<"|c,"<<clip.left<<','<<clip.top<<','<<clip.right<<','<<clip.bottom;
    }
    return out.str();
}
// 실제 창 없이 Screen 팔레트 상태만 검사하는 참조 DC다.
struct ReferenceDc {
    HDC value{CreateCompatibleDC(nullptr)};
    // 화면보다 오래 살아 있는 참조 DC를 마지막에 해제한다.
    ~ReferenceDc() { if (value) DeleteDC(value); }
};
}

// 세 PE·두 x87 정밀도에서 같은 결과를 관찰한 전체 명암 표/배경 호출을 대조한다.
TEST_CASE(GumpBackground_MatchesNativeMapsAndTextureBodies) {
    std::ifstream input(NETSTORM_GUMPBACKGROUND_FIXTURE);CHECK(input.good());std::string line;std::size_t count=0,maps=0;
    // 고정된 독립 fixture의 모든 입력을 재생한다. 원본 PE/Python/장치가 없어도 검사할 수 있다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::string edition,kind;unsigned number{};row>>edition>>kind>>number;
        const auto failures=netstorm::test::FailureCount();
        if (kind=="maps") {
            std::string sourceHex,brightHex,darkHex;row>>sourceHex>>brightHex>>darkHex;
            const auto source=ReadHex(sourceHex),bright=ReadHex(brightHex),dark=ReadHex(darkHex);
            CHECK(source.size()==1024 && bright.size()==256 && dark.size()==256);
            if (source.size()!=1024 || bright.size()!=256 || dark.size()!=256) continue;
            std::array<std::uint32_t,256> logical{};
            // 논리 PALETTEENTRY의 R/G/B/flags 순서 DWORD를 안전하게 조립한다.
            for (std::size_t i=0;i<logical.size();++i) logical[i]=source[i*4]|(std::uint32_t(source[i*4+1])<<8)|(std::uint32_t(source[i*4+2])<<16)|(std::uint32_t(source[i*4+3])<<24);
            const auto result=BuildGumpShadeMaps(logical);
            CHECK(std::equal(bright.begin(),bright.end(),result.bright.begin()));CHECK(std::equal(dark.begin(),dark.end(),result.dark.begin()));++maps;
        } else {
            ScreenRect rect,clip;std::uint32_t flags{};int width{},height{};std::string expected;
            row>>rect.left>>rect.top>>rect.right>>rect.bottom>>clip.left>>clip.top>>clip.right>>clip.bottom>>flags>>width>>height>>expected;
            CHECK(kind=="draw");
            const auto plan=PlanGumpBackground(rect,clip,static_cast<std::int16_t>(width),static_cast<std::int16_t>(height),flags);
            CHECK(Events(plan,clip)==expected);
        }
        CHECK(!row.fail());if (failures!=netstorm::test::FailureCount()) std::fprintf(stderr,"GumpBackground fixture: %s %s %u\n",edition.c_str(),kind.c_str(),number);
        ++count;
    }
    CHECK(count==177);CHECK(maps==9);
}

// 밝음의 제외 범위와 어두움의 후보 허용을 손으로 고른 정확한 목표 RGB로 검사한다.
TEST_CASE(GumpBackground_ShadeExcludesAnimatedBrightCandidates) {
    std::array<std::uint32_t,256> logical{};logical.fill(0xffffff);
    logical[0]=0;logical[10]=0x646464;logical[50]=0x959595;logical[229]=0x969696;logical[245]=0x3c3c3c;
    const auto maps=BuildGumpShadeMaps(logical);
    CHECK(maps.bright[10]==50);CHECK(maps.dark[10]==245);CHECK(maps.bright[0]==0);
    logical[229]=0;logical[10]|=0xff000000;const auto reserved=BuildGumpShadeMaps(logical);
    CHECK(reserved.bright[10]==50);CHECK(reserved.dark[10]==245);
}

// 화면 격자·2픽셀 테두리·pressed 전환·투명 보존을 작은 수동 기대 배열과 비교한다.
TEST_CASE(GumpBackground_DrawsTextureAndSwapsBorderMaps) {
    const IndexedImage texture{3,3,{1,2,1,3,4,3,1,2,1},{255,255,255,255,255,255,255,255,255}};
    GumpShadeMaps maps;
    // 검사 변환표는 소스 번호와 결과를 구별할 수 있도록 일정 번호 이동을 사용한다.
    for (std::size_t i=0;i<256;++i) { maps.bright[i]=static_cast<std::uint8_t>(i+10);maps.dark[i]=static_cast<std::uint8_t>(i+20); }
    // 두 상태는 바탕을 같은 번호로 유지하고 테두리의 명암 방향만 바꾼다.
    for (const bool pressed:{false,true}) {
        IndexedImage canvas{8,8,std::vector<std::uint8_t>(64,9),std::vector<std::uint8_t>(64,255)};
        const auto plan=PlanGumpBackground({1,1,7,7},{0,0,8,8},3,3,GumpBackgroundFlags::kButton|(pressed ? GumpBackgroundFlags::kPressed : 0U));
        DrawGumpBackground(canvas,texture,{0,0},plan,maps);
        CHECK(canvas.indices[3*8+3]==4); // 화면 격자 (2,2)의 질감 중앙. 버튼 상대 격자라면 다른 번호가 된다.
        CHECK(canvas.indices[1*8+3]==(pressed ? 24 : 14));
        CHECK(canvas.indices[6*8+3]==(pressed ? 12 : 22));
        CHECK(canvas.indices[1*8+6]==(pressed ? 23 : 13)); // 위 테두리가 오른쪽 테두리를 나중에 덮는다.
        CHECK(canvas.indices[6*8+1]==(pressed ? 12 : 22)); // 아래 테두리가 왼쪽 테두리를 나중에 덮는다.
        CHECK(canvas.indices[0]==9 && canvas.indices[63]==9);CHECK(canvas.opacity==std::vector<std::uint8_t>(64,255));
    }
    IndexedImage transparent=texture;transparent.opacity.assign(9,0);
    IndexedImage canvas{8,8,std::vector<std::uint8_t>(64,9),std::vector<std::uint8_t>(64,255)};
    const auto plan=PlanGumpBackground({1,1,7,7},{0,0,8,8},3,3,GumpBackgroundFlags::kButton);
    DrawGumpBackground(canvas,transparent,{-1,-1},plan,maps);CHECK(canvas.indices==std::vector<std::uint8_t>(64,9));
    const auto shifted=PlanGumpBackground({1,1,7,7},{3,3,5,5},3,3,GumpBackgroundFlags::kTexture);
    DrawGumpBackground(canvas,texture,{1,1},shifted,maps);
    CHECK(canvas.indices[3*8+3]==1 && canvas.indices[4*8+4]==4);CHECK(canvas.indices[2*8+3]==9 && canvas.indices[3*8+5]==9);
}

// 새 파일 로딩은 명암 표를 갱신하고 번개의 부분 적용/모드 팔레트 재적용은 기존 표를 보존한다.
TEST_CASE(GumpBackground_CachesMapsUntilNextPaletteFile) {
    ReferenceDc dc;CHECK(dc.value!=nullptr);if (!dc.value) return;
    Screen screen(nullptr,dc.value,640,480);screen.Init();std::array<std::uint8_t,776> col{};
    // 중간 회색을 가지는 파일 팔레트를 만든다. 예약 바이트는 이 COL 형식에 없다.
    for (std::size_t i=0;i<256;++i) col[8+i*3]=col[9+i*3]=col[10+i*3]=static_cast<std::uint8_t>(i);
    screen.LoadPalette(GamePalette(col));const auto saved=screen.ShadeMaps();
    ScreenColor temporary{200,200,200,0};screen.SetPalette(80,1,&temporary,true);screen.SetPalette(0,256,nullptr,true);
    CHECK(screen.ShadeMaps().bright==saved.bright && screen.ShadeMaps().dark==saved.dark);
    col[8+80*3]=col[9+80*3]=col[10+80*3]=250;screen.LoadPalette(GamePalette(col));
    CHECK(screen.ShadeMaps().bright!=saved.bright || screen.ShadeMaps().dark!=saved.dark);
}

// UI 계약 밖 역전 경계/fill 경로와 실행이 끝나지 않는 거대한 원본 반복은 안전 API가 거부한다.
TEST_CASE(GumpBackground_RejectsInvalidPathsAndExcessiveLoops) {
    // 각각의 오류 계약을 독립 검사한다.
    for (const auto flags:{0U,0x06000000U,0x0a000000U}) {
        bool rejected=false;try { (void)PlanGumpBackground({0,0,4,4},{0,0,8,8},3,3,flags); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    }
    bool rejected=false;try { (void)PlanGumpBackground({4,0,0,4},{0,0,8,8},3,3,GumpBackgroundFlags::kButton); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    rejected=false;try { (void)PlanGumpBackground({0,0,100000,100000},{0,0,8,8},2,2,GumpBackgroundFlags::kButton); } catch (const std::length_error&) { rejected=true; }CHECK(rejected);
    rejected=false;try { const auto end=std::numeric_limits<int>::max();(void)PlanGumpBackground({end,0,end,4},{0,0,8,8},3,3,GumpBackgroundFlags::kButton); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
