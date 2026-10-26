// 공통 메뉴 체크 표시가 보호 원본의 J02 그림과 같은 픽셀·핫스폿·클립을 사용하는지 검증한다.
#include "DudeSpawnSupport.h"
#include "client/MenuGumpDraw.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include <algorithm>
#include <array>

using namespace netstorm::client; // 실제 공통 메뉴와 장식 픽셀 처리기를 사용한다.
using namespace netstorm::o; // 보호 자산의 판본을 명시한다.

// 원문0049b060의 첫 번호 표(+130)와 메뉴00476080의+154+2는 J02=60을 선택한다.
// 10.78과10.72의 독립 .type 원문에서 I00=54/I.count=4/J00=58/J02=60을 확인했다.
TEST_CASE(MenuCheckMarker_OriginalJ02PixelsAndClipInBothEditions) {
    constexpr int pitch=83,height=59; // 행 padding까지 보존 여부를 확인하는 실제 표면 크기다.
    constexpr std::uint32_t originalFrame=60; // 보호 fortGump.type의 J02 물리 프레임이며 제품 선택식에서 계산하지 않는다.
    // 두 판본의 같은 그림을 정상 위치/상단·좌측 클립/빈 클립에서 각각 대조한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto& assets=netstorm::test::dudespawn::Assets(edition).game;const auto& art=assets.Find("fortGump");
        const auto type=static_cast<std::uint32_t>(art.block+kFirstAssetTypeNumber);const auto table=art.definition.FrameTable();
        CHECK(table.Run('I').first==54 && table.Run('I').count==4 && table.Run('J').first==58);
        const auto& frame=assets.Shapes().Blocks()[art.block].frames[originalFrame];const auto image=assets.Shapes().Decode(art.block,originalFrame);
        CHECK(frame.rect.left==-6 && frame.rect.top==-6 && image.width==12 && image.height==12);
        // 마커의 기준점은 원문+5/+7 위치이며 이미지의-6/-6 핫스폿을 추가로 반영한다.
        for (const auto point:std::array<ScreenPoint,3>{{{35,24},{10,8},{80,56}}}) {
            GumpDrawState clip;SetGumpDrawClip(clip,{9,7,72,51});std::vector<std::uint8_t> actual(pitch*height,0xa5),expected=actual;
            const GumpPixelTarget target(actual,pitch,height,clip);GumpDecorationPixelTarget pixels(target,assets);MenuGumpDrawHooks hooks;BindMenuGumpDrawAssets(hooks,assets,pixels);hooks.marker(type,point);
            // 기대 픽셀은 보호 SHP의 고정60번을 직접 읽고 원문 클립과 투명도만 적용한다.
            for (std::size_t y=0;y<image.height;++y) for (std::size_t x=0;x<image.width;++x) {
                const auto dx=point.x+frame.rect.left+static_cast<int>(x),dy=point.y+frame.rect.top+static_cast<int>(y);const auto source=y*image.width+x;
                if (dx>=9 && dy>=7 && dx<72 && dy<51 && image.opacity[source]) expected[static_cast<std::size_t>(dy)*pitch+dx]=image.indices[source];
            }
            CHECK(actual==expected);
        }
    }
}

// 메인 옵션/ESC 하위 메뉴가 공유하는 실제 메뉴 트리에서 체크 비트만 바뀔 때 J02만 추가되는지 확인한다.
TEST_CASE(MenuCheckMarker_CommonMenuTreeDrawsOriginalToggleWithoutChangingOtherPixels) {
    const auto& data=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto& assets=data.game;const auto& art=assets.Find("fortGump");FontStore fonts(data.files,"Arial");
    std::array<std::uint32_t,256> logical{}; // 실제 파일 팔레트와 같은 기본색/음영표를 만든다.
    // 각 팔레트 색을 원본 RGB DWORD 순서로 조립한다.
    for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); }
    const auto palette=BuildPaletteColorTable(logical,assets.Edition());const auto shade=BuildGumpShadeMaps(logical);InputEvent released{InputCode::kRelease,0,0};
    RawButtonControls buttons(assets.Edition(),{[] { return 0.0; },[&](auto) { return released; },[] {}});
    const auto frame=assets.FrameIndex(art,"A00",0);const auto metrics=assets.Shapes().SquidMetrics(art.block,frame);
    buttons.SetControls({},{},640,480,{static_cast<std::uint32_t>(art.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(palette)});
    RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,auto& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[&](auto) { return released; };hooks.cursor=[](auto) {};
    auto& menus=buttons.EnableMenus(assets,fonts,std::move(hooks));menus.SetPalette(palette);std::uint32_t token{};
    // 일반 옵션과 게임 중 옵션의 두 대표 문구를 같은 공통 창에 작성한다.
    for (const auto label:{"Sound On","Hide Island Ownership - ~iF7~."}) {
        menus.Open({150,100},[&](const auto& widgets,const auto& controls,const auto&) { token=widgets.BeginVertical();controls.AddChecked(true,label,0x020000a0);controls.AddChecked(false,"Unchecked",0x020000a0); });
        auto* node=static_cast<MenuGumpNode*>(menus.Node(token));CHECK(node && node->parent==buttons.RegisteredGump(0));if (!node) return;
        node->items[0].flags&=~4U;std::vector<std::uint8_t> off(640*480,0xa5);menus.Draw(off,640,480,palette,shade);
        node->items[0].flags|=4U;auto actual=std::vector<std::uint8_t>(640*480,0xa5);menus.Draw(actual,640,480,palette,shade);auto expected=off;
        const ScreenPoint point{static_cast<int>(node->x)+static_cast<int>(node->margins[0]*0.25-(node->targetWidth-node->width))+5,
            static_cast<int>(node->y)+static_cast<int>(node->offsets[0]+node->margins[2]-(node->targetHeight-node->height))+7}; // 원문00476080의 체크 기준점이다.
        const auto& marker=assets.Shapes().Blocks()[art.block].frames[60];const auto image=assets.Shapes().Decode(art.block,60);
        // 독립 원본 J02의 불투명 픽셀만 미체크 메뉴 위에 합성한다. 다른 행/글자/배경의 변경은 허용하지 않는다.
        for (std::size_t y=0;y<image.height;++y) for (std::size_t x=0;x<image.width;++x) {
            const auto source=y*image.width+x;const auto dx=point.x+marker.rect.left+static_cast<int>(x),dy=point.y+marker.rect.top+static_cast<int>(y);
            if (image.opacity[source]) expected[static_cast<std::size_t>(dy)*640+dx]=image.indices[source];
        }
        CHECK(actual!=off);CHECK(actual==expected);menus.Close();
    }
}
