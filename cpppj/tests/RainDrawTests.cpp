// 원본 가상 Draw 관찰과 실제 자산의 Renderer 합성을 각각 대조한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "o/RawRainVortexDraw.h"
#include "client/Renderer.h"
#include <algorithm>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 검사 입력만 지정 폭의 little endian으로 채운다. 기대 출력은 원본 fixture에서 읽는다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 판본별 frame BYTE/DWORD와 WORD 상태의 이웃 필드를 보존한다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 원본 .type의 순서와 같은 입력 코드 표다. 기대 출력 순서는 fixture에서만 읽는다.
RiftTypeFrames RainFrames() {
    std::vector<FrameCode> codes;
    // 전체29클러스터의 방향만 Draw에 필요한 입력으로 넣는다.
    for (const auto letter:std::string("A")+std::string(14,'B')+std::string(13,'C')+"P") codes.push_back({static_cast<std::uint8_t>(letter),0,0,0});
    return RiftTypeFrames(std::move(codes));
}
// 두 판본의 실제 Draw 호출 순서와 같은 원시 필드 폭을 대조한다.
void CheckRainDraw(OriginalEdition edition) {
    const bool patch=edition==OriginalEdition::Patch1078;const auto frames=RainFrames();std::size_t count=0;
    // 같은 입력을 같은 판본의 C++에만 제공한다.
    for (const auto& row:LoadFixture(NETSTORM_RAIN_DRAW_FIXTURE).rows) {
        if (row[0]!=(patch ? "originals" : "originalCD")) continue;CHECK(row.size()==4);
        std::vector<std::uint8_t> raw(patch ? 50 : 36);Write(raw,0,patch ? 0x5196f0U : 0x501d28U);raw[10]=124;
        Write(raw,12,Number(row[2]),2);Write(raw,patch ? 36 : 34,Number(row[1]),patch ? 4 : 1);const auto before=raw;
        std::string trace;
        // 실제 경계의 호출 순서만 기록하며 기대 프레임을 재계산하지 않는다.
        const auto append=[&](std::string value) { if (!trace.empty()) trace+=',';trace+=value; };
        DrawRainVortex(edition,raw,frames,{[&](std::int32_t frame) { append("F"+std::to_string(frame)); },[&] { append("S"); }});
        CHECK(trace==row[3]);CHECK(raw==before);++count;
    }
    CHECK(count==116);
}
// 압축 해제한 원본 그림을 기준점/투명도에 따라 직접 덮는다. Renderer의 합성 함수를 호출하지 않는다.
std::vector<std::uint8_t> ExpectedRain(const GameAssets& assets,const RenderSprite& sprite,int width,int height,std::uint8_t background) {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width)*height,background);
    // 소스 픽셀 하나씩을 화면/클립과 대조하는 독립 기대 합성이다.
    const auto layer=[&](std::size_t number) {
        const auto& frame=assets.Shapes().Blocks()[sprite.block].frames[number];const auto image=assets.Shapes().Decode(sprite.block,number);
        // 투명 픽셀은 기존 화면 값을 유지한다.
        for (std::size_t y=0;y<image.height;++y) for (std::size_t x=0;x<image.width;++x) {
            const auto dx=sprite.x+frame.rect.left+static_cast<int>(x),dy=sprite.y+frame.rect.top+static_cast<int>(y);const auto index=y*image.width+x;
            if (image.opacity[index] && dx>=0 && dx<width && dy>=0 && dy<height && dx>=sprite.clip.left && dx<sprite.clip.right && dy>=sprite.clip.top && dy<sprite.clip.bottom)
                pixels[static_cast<std::size_t>(dy)*width+dx]=image.indices[index];
        }
    };
    layer(sprite.frame);
    // 같은 객체의 그림 제출 순서를 기대 합성에서도 그대로 보존한다.
    for (const auto number:sprite.followingFrames) layer(number);
    return pixels;
}
}
// PATCH DWORD frame과 전체116입력을 대조한다.
TEST_CASE(RainDraw_PatchActualSequence) { CheckRainDraw(OriginalEdition::Patch1078); }
// CD BYTE frame과 전체116입력을 대조한다.
TEST_CASE(RainDraw_CdActualSequence) { CheckRainDraw(OriginalEdition::Cd1072); }
// 실제 자산의 그림 전체 픽셀·정지 배경 합성·물결 교체/이동 뒤 지움을 검사한다.
TEST_CASE(RainDraw_RendererLayersAndDirtyPixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game;const auto& type=assets.Find("rainVortex");
    RenderSprite sprite{&assets.Shapes(),type.block,0,200,180,{0,0,320,240},{89,116,-1},{},false,{1}};
    Renderer renderer(320,240);std::vector<std::uint8_t> pixels(320*240,7);renderer.SetScene({sprite});renderer.Draw(pixels,320,7);renderer.Present([](ScreenRect) {});
    CHECK(pixels==ExpectedRain(assets,sprite,320,240,7));
    Renderer capture(320,240);capture.SetScene({sprite});CHECK(capture.SceneImage().indices==ExpectedRain(assets,sprite,320,240,0));
    // 물결의 교체와 그림의 이동은 옛 본체 영역까지 지운다.
    for (const auto frame:std::array<std::size_t,3>{14,2,1}) {
        sprite.followingFrames={frame};sprite.x+=9;sprite.y+=3;renderer.SetScene({sprite});renderer.Draw(pixels,320,7);renderer.Present([](ScreenRect) {});
        CHECK(pixels==ExpectedRain(assets,sprite,320,240,7));
    }
    renderer.SetScene({});renderer.Draw(pixels,320,7);CHECK(std::all_of(pixels.begin(),pixels.end(),[](auto value) { return value==7; }));
}
// 첫 그림 밖에 후속 그림만 보일 때도 범위/합성에 포함해야 한다.
TEST_CASE(RainDraw_RendererFollowingFrameExtendsBounds) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game;const auto& type=assets.Find("rainVortex");
    // 기본 그림의 불투명 픽셀(-48,-98)을 화면(0,0)에 놓고 물결의 위쪽은 화면 아래에 둔다.
    RenderSprite sprite{&assets.Shapes(),type.block,1,48,98,{0,0,10,10},{89,116,-1},{},false,{0}};
    Renderer renderer(10,10);std::vector<std::uint8_t> pixels(100);renderer.SetScene({sprite});renderer.Draw(pixels,10);
    CHECK(pixels==ExpectedRain(assets,sprite,10,10,0));CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0; }));
}
