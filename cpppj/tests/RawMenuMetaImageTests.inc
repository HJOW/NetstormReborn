// 실제 공통 등록/부착/수집/클립/출력/지연 소멸에서 같은 비소유 Image를 사용한다.
#include "client/MetaImageGump.h"
#include "client/ProductionColorMaps.h"
#include "client/VFXMappedDraw.h"

// 전용 동적 파생을 실제 메뉴 트리에 붙여 원본 RLE 픽셀과 현재 색표를 비교한다.
TEST_CASE(RawMenuMetaImage_ActualTreePixelsClipAndBorrowedImageLifetime) {
    MenuClient client;RawImage image;const std::vector<std::uint8_t> bits{0,7,7,0,255,0,8,0,9,9,0,0,0,10,0,11,11,0,12,12,0,0,13,0};image.Convert({6,4,bits},false);
    const auto root=client.buttons.RootToken(),token=client.menus->CreateMetaImage(image);auto* node=client.menus->MetaImageNode(token);
    CHECK(node && node->image==&image && node->command==21 && node->baseWidth==0 && node->width==0);if (!node) return;
    client.menus->MetaImageBackend().Resize(token);CHECK(node->baseWidth==7 && node->baseHeight==5);client.menus->Attach(token,20,22,root,0);CHECK(node->parent==root && node->scale==1);
    constexpr int pitch=647,height=480; // 행 padding까지 같은 현재 UI 표면의 경계를 검사한다.
    std::array<std::uint32_t,256> logical{};
    for (std::size_t index=0;index<logical.size();++index) { const auto color=client.assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 원본 RGB 채널 순서다.
    const auto maps=BuildGumpShadeMaps(logical);const auto disabled=BuildProductionColorMaps(logical).disabled;ShapeDatabase database(std::vector<std::uint8_t>(image.Shape().begin(),image.Shape().end()));
    for (std::uint32_t mode=0;mode<2;++mode) { // plain과 고정 mapped색표를 같은 실제 Draw/수집 경로에서 확인한다.
        node->colorMode=mode;const ScreenRect clip{22,24,25,26};std::vector<std::uint8_t> actual(pitch*height,0x5a),expected=actual;
        client.menus->Draw(actual,pitch,height,client.Palette(),maps,clip);
        if (mode) static_cast<void>(DrawMappedShape(expected,pitch,height,database,0,0,26,26,{0,0,pitch-1,height-1},disabled));
        else static_cast<void>(DrawShape(expected,pitch,height,database,0,0,26,26,{0,0,pitch-1,height-1}));
        for (int y=0;y<height;++y) { // 독립 RLE 출력을 공통 half-open clip 밖에서 복원한다.
            for (int x=0;x<pitch;++x) if (x<clip.left || x>=clip.right || y<clip.top || y>=clip.bottom) expected[static_cast<std::size_t>(y)*pitch+x]=0x5a; // padding도 clip 밖이다.
        }
        CHECK(actual==expected && std::any_of(actual.begin(),actual.end(),[](auto value) { return value!=0x5a; }));
    }
    client.buttons.KillGump(token);CHECK(client.menus->MetaImageNode(token)!=nullptr);client.buttons.CleanupGumps();CHECK(!client.menus->MetaImageNode(token) && !image.Shape().empty());
    image.Destroy();image.Destroy(); // 파생 소멸은 빌린 Image를 해제하지 않았으며 소유자가 반납한다.
}
