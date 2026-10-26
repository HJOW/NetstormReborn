// 실제 Title GIF와 돌 버튼 자산을 공통 Draw에 함께 넣어 배경 유실·눌림·클립 회귀를 검사한다.
#include <array>
#include <bit>
#include <span>
#include "client/GumpTextDraw.h"

namespace { // 기존 GifScene의 실제 자산과 공통 등록표를 그대로 재사용하는 픽셀 관측이다.
constexpr int kMainPaintPitch=647; // 화면640열 뒤7바이트를 남겨 행 간격을 화면 폭으로 오인하는 쓰기를 찾는다.

// 두 실제 표면의 지정 띠를 비교한다. 버튼 계획이나 질감 타일 계산으로 기대 픽셀을 만들지 않는다.
std::size_t MainPaintDifferences(std::span<const std::uint8_t> first,std::span<const std::uint8_t> second,ScreenRect band) {
    std::size_t differences{}; // 실제 출력 사이에서 다른 색 번호의 개수다.
    // 글자와 부모 외곽선을 제외하도록 호출자가 고른 각 행을 관측한다.
    for (int y=band.top;y<band.bottom;++y) {
        // 같은 화면 좌표와 실제 pitch로 두 원시 표면을 읽는다.
        for (int x=band.left;x<band.right;++x) differences+=first[static_cast<std::size_t>(y)*kMainPaintPitch+x]!=second[static_cast<std::size_t>(y)*kMainPaintPitch+x];
    }
    return differences;
}
}

// 원문 메인 깊이에서 GIF가 먼저, 자식 질감이 다음, 돌 버튼의 글자가 마지막에 남아야 한다.
TEST_CASE(BackgroundGifClient_MainMenuStoneTexturePressedClipAndPitch) {
    GifScene scene; // 창이나 게임 루프 없이 기존 실제 Client/파일/글꼴/공통 UI를 소유한다.
    const auto title=scene.gifs->Create("titleMenu"); // 보호 자산의 실제 BackgroundGifSpec 공장 입력이다.
    scene.menus->Attach(title,0,0,scene.buttons.RootToken(),1);
    scene.gifs->Node(title)->depth=std::bit_cast<std::uint32_t>(std::int32_t{-43}); // Native Uber의 제목 자식 깊이를 독립 입력으로 공급한다. 호스트 root 자체를 합성 Uber로 간주하지 않는다.
    const auto gifOnly=scene.Draw(); // 원시 GIF만 그린 실제 공통 표면을 비교 기준으로 보존한다.
    const auto& gif=scene.gifs->Bitmap(scene.gifs->Node(title)->bitmap); // renderer와 별도로 읽을 실제 해독 픽셀이다.
    const ScreenRect bodyBand{170,172,178,184},bevelBand{165,171,167,185}; // 각각 글자 밖 내부 질감과 왼쪽 자식 입체 테두리다.
    std::size_t gifDifferences{}; // 기준 표면이 실제 제목 원시 픽셀과 다른지 센다.
    // 버튼 내부 띠의 모든 행을 원시 제목 자산과 직접 비교한다.
    for (int y=bodyBand.top;y<bodyBand.bottom;++y) {
        // 실제 GIF 폭639와 표면 pitch647을 각각 사용한다.
        for (int x=bodyBand.left;x<bodyBand.right;++x) gifDifferences+=gifOnly[static_cast<std::size_t>(y)*kMainPaintPitch+x]!=gif.indices[static_cast<std::size_t>(y)*gif.width+x];
    }
    CHECK(gifDifferences==0);

    const auto& fort=scene.assets.game.Find("fortGump"); // 원문 돌 버튼의 실제 질감 TYPE이다.
    const auto frame=scene.assets.game.FrameIndex(fort,"A01",0); // 실제 메인 버튼이 쓰는 원본 클러스터다.
    const auto metrics=scene.assets.game.Shapes().SquidMetrics(fort.block,frame); // 같은 SHP 헤더의 타일 치수다.
    const RawButtonAssets assets{static_cast<std::uint32_t>(fort.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(scene.palette)}; // 가짜 이미지 없이 실제 보호 자산을 생성자에 공급한다.
    const std::array<GumpControl,1> controls{{{0,{164,167,236,189},false,true,false}}}; // 원문 메인의 첫 행 위치에 놓은 실제 돌 버튼이다.
    const std::array<std::string,1> labels{{"Edit"}}; // 내부 띠와 겹치지 않는 실제 글꼴 입력이다.
    scene.buttons.SetControls(controls,labels,640,480,assets,true,false,-83);
    const auto stone=scene.buttons.PickGump({200,178}); // 토큰 배치 산술 없이 실제 적중 대상으로 부모 버튼을 찾는다.
    CHECK(stone!=0 && scene.buttons.GumpDepth(stone)==std::bit_cast<std::uint32_t>(std::int32_t{-83}));
    std::size_t childDepthCount{}; // 부모-83보다 한 단계 뒤의 실제 자식 배경-82가 남는지 관측한다.
    // 공개 serial 조회로 현재 등록 노드만 읽고 토큰 연속성이나 할당 순서를 가정하지 않는다.
    for (std::uint32_t serial=1;serial<1000;++serial) {
        const auto token=scene.buttons.SerialGump(serial); // 원본 고정 등록표의 현재 대상이다.
        if (token && scene.buttons.GumpDepth(token)==std::bit_cast<std::uint32_t>(std::int32_t{-82})) ++childDepthCount;
    }
    CHECK(childDepthCount==1 && scene.gifs->ImageCount()==1);
    const auto normal=scene.Draw(); // GIF와 버튼을 같은 공통 Collect/Sort/Draw에서 출력한다.
    CHECK(MainPaintDifferences(normal,gifOnly,bodyBand)>0);

    const auto texture=scene.assets.game.Shapes().Decode(fort.block,frame); // 기대 색은 버튼 계획이 아닌 실제 보호 SHP 픽셀에서 읽는다.
    std::array<bool,256> textureColors{},paintedColors{}; // 불투명 원본 질감 색과 내부 띠의 실제 색 집합이다.
    // 투명 픽셀을 제외한 원본 자산의 실제 색 번호만 수집한다.
    for (std::size_t index=0;index<texture.indices.size();++index) if (texture.opacity.empty() || texture.opacity[index]) textureColors[texture.indices[index]]=true;
    std::size_t foreignColors{}; // 제목과 달라진 내부 픽셀이 실제 무명암 질감 색에 속하는지 센다.
    // 부모 선·자식 bevel·글자를 피한 내부 띠를 읽는다.
    for (int y=bodyBand.top;y<bodyBand.bottom;++y) {
        // 전역 타일 위상 계산을 복제하지 않고 실제 색 집합과 질감의 변화만 대조한다.
        for (int x=bodyBand.left;x<bodyBand.right;++x) {
            const auto at=static_cast<std::size_t>(y)*kMainPaintPitch+x; // 실제 표면의 현재 바이트 위치다.
            paintedColors[normal[at]]=true;if (normal[at]!=gifOnly[at] && !textureColors[normal[at]]) ++foreignColors;
        }
    }
    CHECK(foreignColors==0 && std::count(paintedColors.begin(),paintedColors.end(),true)>1);

    const auto down=scene.buttons.RouteInput({InputCode::kLeftButton,200,178}); // 원본 Route/실제 캡처/자식 pressed writer를 실행한다.
    CHECK(down.consumed==stone && !down.activated && scene.buttons.Pressed()==0 && scene.buttons.HasCapture());
    const auto pressed=scene.Draw(); // 눌린 실제 자식 배경의 명암을 포함한 출력이다.
    CHECK(MainPaintDifferences(pressed,normal,bevelBand)>0 && MainPaintDifferences(pressed,normal,bodyBand)==0);
    CHECK(MainPaintDifferences(pressed,gifOnly,bodyBand)>0);
    const ScreenRect clip{164,170,180,186}; // 자식 bevel과 내부 질감 일부만 남기는 좁은 변경 영역이다.
    const auto clipped=scene.Draw(clip); // 같은 실제 눌림 상태를 변경 영역 하나에만 그린다.
    std::size_t clipDifferences{},paddingWrites{}; // 클립 밖 및 행 끝의 보존 위반을 각각 관측한다.
    // 전체 높이와 화면 밖 행 padding까지 빠짐없이 검사한다.
    for (int y=0;y<480;++y) {
        // 완성된 실제 표면을 클립 안 기준으로 사용하고 나머지는 초기 sentinel이어야 한다.
        for (int x=0;x<kMainPaintPitch;++x) {
            const auto at=static_cast<std::size_t>(y)*kMainPaintPitch+x; // 동일 pitch의 실제 바이트 위치다.
            const bool inside=x>=clip.left && x<clip.right && y>=clip.top && y<clip.bottom; // 화면 좌표의 반열린 변경 영역이다.
            clipDifferences+=clipped[at]!=(inside ? pressed[at] : std::uint8_t{0x5a});
            if (x>=640) paddingWrites+=normal[at]!=0x5a || pressed[at]!=0x5a || clipped[at]!=0x5a;
        }
    }
    CHECK(clipDifferences==0 && paddingWrites==0);
    const auto up=scene.buttons.RouteInput({InputCode::kLeftButton|InputCode::kRelease,200,178}); // 같은 실제 캡처를 반납하고 부모 명령을 받는다.
    CHECK(up.consumed==stone && up.activated==0 && !scene.buttons.Pressed() && !scene.buttons.HasCapture());
    CHECK(scene.Draw()==normal);

    const std::array<std::string,1> emptyLabel{{""}}; // 배경/깊이는 같고 실제 글자 입력만 없는 독립 관측이다.
    scene.buttons.SetControls(controls,emptyLabel,640,480,assets,true,false,-83);
    const auto withoutLabel=scene.Draw(); // 배경이 글자를 덮으면 라벨 유무의 실제 픽셀 차이가 사라진다.
    CHECK(MainPaintDifferences(normal,withoutLabel,{172,170,228,186})>0 && scene.gifs->ImageCount()==1);

    const auto& font=scene.fonts.Get(0,FontStyle::Normal); // 현재 실제 Font0 캐시에서 측정 폭을 읽는다.
    const auto textWidth=font.Measure(labels[0]),textHeight=FontStore::LogicalHeight(0); // 그리기 전진 폭과 별개인 원문 첫 폭 표/슬롯 높이다.
    CHECK(textWidth>0 && textHeight>0);
    const auto rect=controls[0].rect; // 명시한 버튼의 실제 화면 경계다.
    const ScreenPoint centered{rect.left+(rect.right-rect.left-2-textWidth)/2+2,rect.top+(rect.bottom-rect.top-3-textHeight)/2+1}; // 원문00424ff0의 안쪽 폭/높이와 정상 오프셋으로 얻은 독립 기준점이다.
    auto centeredPixels=withoutLabel; // 같은 실제 배경 위에 기준 글리프만 직접 그린다.
    GumpDrawState glyphClip;SetGumpDrawClip(glyphClip,{0,0,640,480}); // 실제 그리기와 같은 화면 클립이다.
    const GumpPixelTarget glyphTarget(centeredPixels,kMainPaintPitch,480,glyphClip); // 버튼 계획/측정 필드를 건드리지 않는 독립 표면이다.
    GumpGlyphColorState glyphColors; // 실제 글꼴의 색 번호100과 같은 원문 색 변환표다.
    GumpTextDraw glyphs(scene.fonts,MakeGumpGlyphPixelHooks(glyphTarget,glyphColors),OriginalEdition::Patch1078); // 트리의 stone Draw를 우회한 기존 글리프 본체만 재사용한다.
    const auto palette=ButtonColors(scene.palette); // 현재 실제 보호 팔레트의 원문 검정/흰색이다.
    const GumpTextContext normalContext{2,0,palette.white,0,0,palette.black,1,1}; // 원문 정상 stone의 Font0/흰색/오른쪽아래 그림자다.
    glyphs.Draw(std::span<const char>(labels[0].data(),labels[0].size()),centered,normalContext,static_cast<std::int32_t>(labels[0].size()));
    CHECK(centeredPixels==normal);
}
