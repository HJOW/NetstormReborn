// 실제 Frame의 지형/미리보기/배경/월드/UI 순서와 Native clearDirty의 호스트 vector 반영을 검사한다.
#include "TestSupport.h"
#include "client/Renderer.h"
#include "client/ClientRendererFrame.h"
#include "client/ClientRendererFrameGlobals.h"
#include "client/RawRendererPrelude.h"
#include "o/RawKencloudRender.h"
#include <array>
#include <algorithm>
#include <bit>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace netstorm::client; //같은 제품 Renderer와 원문 앞단을 조립한다.
// 원문이 첫 지형 수집기에 실제 전달한 float4개와 행 제한을 세 화면비/부분 dirty/분수 카메라로 대조한다.
TEST_CASE(RendererFrame_Original90ProjectionAndRowLimit) {
    std::ifstream file(NETSTORM_RENDERER_PROJECTION_FIXTURE);CHECK(file.good());std::string line;std::size_t count{};
    while (std::getline(file,line)) { //각 원문 입력을 독립 현재 카메라/화면 사각형으로 재구성한다.
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::array<std::uint32_t,17> values{};
        for (auto& value:values) row>>value; //네 float 기대 비트와 signed 좌표의 원래32비트를 보존한다.
        CHECK(!row.fail());if (row.fail()) continue;
        const auto integer=[&](std::size_t index) { return std::bit_cast<std::int32_t>(values[index]); }; //원문 signed 해석만 적용한다.
        RawCameraViewState view;view.worldBits={values[10],values[11]};
        const auto actual=RendererCollectionViewport({integer(6),integer(7),integer(8),integer(9)},{integer(2),integer(3),integer(4),integer(5)},view);
        CHECK(std::bit_cast<std::uint32_t>(actual.left)==values[12] && std::bit_cast<std::uint32_t>(actual.top)==values[13]);
        CHECK(std::bit_cast<std::uint32_t>(actual.right)==values[14] && std::bit_cast<std::uint32_t>(actual.bottom)==values[15]);
        CHECK(static_cast<std::uint32_t>(RendererCollectionRowLimit(integer(0),integer(1)))==values[16]);++count;
    }
    CHECK(count==90);
}
// 실제 Native 배경은 미리보기 수집 뒤·월드/UI 앞이며 정적 GIF 배경으로 다시 덮지 않는다.
TEST_CASE(RendererFrame_NativeCollectionBackgroundAndOpaqueUiOrder) {
    Renderer renderer(8,8);std::vector<std::uint8_t> pixels(64,33);std::string events;
    auto old=std::make_shared<IndexedImage>(IndexedImage{8,8,std::vector<std::uint8_t>(64,99),std::vector<std::uint8_t>(64,255)});renderer.SetBackground(old);
    RenderSprite sprite;sprite.clip={0,0,8,8};sprite.procedure=RenderProcedure{{0,0,8,8},[&](auto target,int,int,ScreenRect) { events+='W';target[18]=7; }};renderer.SetScene({sprite});
    renderer.SetExtraCollection([&](auto,auto) { events+='E';return std::vector<RenderSprite>{}; });
    renderer.SetOverlayProcedure(RenderProcedure{{0,0,8,8},[&](auto target,int,int,ScreenRect clip) { events+='I';CHECK(clip.right>clip.left);target[18]=8; }});
    RendererWorldDrawing world;
    world.collect=[&](auto,auto flags) { events+='C';return flags&4U ? 0U : 1U; };
    world.background=[&](auto,auto flags,auto covered) { events+='B';CHECK(covered==(flags&4U ? 0U : 1U));if (!(flags&4U)) { CHECK(pixels[0]==33);std::fill(pixels.begin(),pixels.end(),std::uint8_t{2}); } };
    renderer.Draw(pixels,8,0,&world);renderer.Present([](auto) {});CHECK(events=="CEBWI" && pixels[0]==2 && pixels[18]==8);
    events.clear();renderer.Invalidate({2,2,3,3},4);renderer.Draw(pixels,8,0,&world);renderer.Present([](auto) {});
    CHECK(events=="CEBI" && pixels[0]==2 && pixels[18]==8); //UI 전용4는 월드/선택 표시를 다시 그리지 않으며 실제 UI만 갱신한다.
}

// 실제 signed Screen/clip/카메라 필드는 복사 없이 하위 효과 뒤 값을 읽는다.
TEST_CASE(RendererFrame_SignedStorageAndActualDirtyReset) {
    Renderer renderer(1024,768);std::vector<std::uint8_t> pixels(1024*768);renderer.Draw(pixels,1024);renderer.Present([](auto) {});renderer.Invalidate({1,2,3,4});
    ClientRendererFrameGlobals owner;std::uint32_t mode=138,force=1,checking{},dirtyCount=static_cast<std::uint32_t>(renderer.CameraDirtyEntries().size());
    std::int32_t width=1024,height=768,initialHeight=800,cameraY{},left=84,top{},right=1024,bottom=768;std::array<std::uint32_t,2054> clouds{};
    RendererPreludeHooks hooks;hooks.markCloudDirty=[&] { netstorm::o::MarkKencloudDirty(clouds); };
    hooks.queueCloudDirty=[&](auto rect) { AppendRendererCloudDirty(owner.CloudQueue(),rect,checking,{}); };
    hooks.overrideActive=[] { return 0U; };hooks.clearDirty=[&] { renderer.ClearDirty(); };
    hooks.invalidate=[&](auto rect,auto flags) { CHECK(dirtyCount==0);renderer.Invalidate(rect,flags); };
    PrepareRendererFrame({owner.Scan(),renderer.BackgroundRedraw(),dirtyCount,owner.CloudDirtyCount()},{mode,force,width,height,initialHeight,cameraY,left,top,right,bottom},hooks);
    CHECK(renderer.CameraDirtyEntries().size()==1 && !renderer.FullRedrawPending());
    const auto& entry=renderer.CameraDirtyEntries().front();CHECK(entry.rect.left==0 && entry.rect.top==0 && entry.rect.right==1024 && entry.rect.bottom==768 && entry.flags==0);
    CHECK(owner.CloudDirtyCount()==0 && owner.CloudRectangles()[0].right==1024 && owner.CloudRectangles()[1].bottom==768);
    RendererPreludeSignedRef signedRef(width);width=-17;CHECK(signedRef.Read()==-17);std::uint32_t raw=0xffffffffU;RendererPreludeSignedRef rawRef(raw);CHECK(rawRef.Read()==-1);raw=7;CHECK(rawRef.Read()==7);
}
