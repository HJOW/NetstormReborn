// 실제 VFX 픽셀로 첫 패널 원본 보존·새 높이 본문 반복·dirty/명암/불투명 검정을 검사한다.
#include "TestSupport.h"
#include "client/GumpBackgroundDraw.h" // 검사하는 실제 배경 자산/Draw/픽셀 출력기의 선언을 직접 포함한다.
#include "GumpBackgroundTestAssets.h"
#include <algorithm>

namespace { // 원본 파일/게임을 실행하지 않는 독립 SHP 입력과 실제 배경 픽셀 경계다.
using namespace netstorm::client;
using namespace netstorm::test::gumpbackground;
constexpr std::uint32_t kPanelType=92; // 이 독립 SHP 한 블록을 조회할 명시 타입 번호다.
constexpr std::int32_t kWidth=80,kSourceHeight=774,kHeaderRows=25; // 관측한 실제 패널의 폭/첫 전체 타일/예산 뒤 선두 영역과 같은 입력이다.

// 본문 행마다 다른 불투명 색을 주어 재시작/잘못된 전역 위상을 구별한다.
std::uint8_t WidePanelColor(std::int32_t row,std::int32_t header=kHeaderRows) { return row<header ? 0:static_cast<std::uint8_t>(row%31+1); }
// 실제36BYTE Squid 메타와24BYTE VFX 헤더·RLE를 가진 독립 단일 프레임을 만든다.
ShapeDatabase WidePanelShapes(std::int32_t width=kWidth,std::int32_t height=kSourceHeight,std::int32_t header=kHeaderRows,bool transparentHeader=false) {
    std::vector<std::uint8_t> bytes(76);Put(bytes,0,0x30312e31);Put(bytes,4,1);Put(bytes,8,52);Put(bytes,40,static_cast<std::uint32_t>(width+1),2);Put(bytes,42,static_cast<std::uint32_t>(height+1),2);
    Put(bytes,52,static_cast<std::uint32_t>(height-1),2);Put(bytes,54,static_cast<std::uint32_t>(width-1),2);Put(bytes,68,static_cast<std::uint32_t>(width-1));Put(bytes,72,static_cast<std::uint32_t>(height-1));
    for (std::int32_t row=0;row<height;++row) { // 첫 검정/투명 선두와 명시한 각 본문 행을 독립 RLE로 기록한다.
        if (transparentHeader && row<header) bytes.insert(bytes.end(),{1,static_cast<std::uint8_t>(width),0});
        else bytes.insert(bytes.end(),{static_cast<std::uint8_t>(width*2),WidePanelColor(row,header),0});
    }
    return ShapeDatabase(std::move(bytes));
}
// 일반 픽셀 출력기와 같은 clip/자산/색 표를 묶는다. 특수 생산/게임/통신 동작은 만들지 않는다.
struct WidePanelScene {
    GumpCreateState create; // 공통 수집기와 같은 판본/실제 화면 크기다.
    GumpCreateNode root;GumpBackgroundNode node; // 같은 기본 노드 주소만 공급하는 독립 장면이다.
    GumpCreate factory;GumpCollection collection; // 기존 원문 배경 Draw의 위치/저장소 참조다.
    GumpDrawState clip;GumpBackgroundDrawState state; // 현재 logical/VFX clip과 배경 진단/색 전역이다.
    std::vector<std::uint8_t> guarded; // 표면 앞뒤 guard와 실제8BIT 화면 픽셀이다.
    GumpPixelTarget target;ColorMap colors{};GumpShadeMaps maps; // 기존 실제 래스터와 현재 명암 색 표다.
    GumpBackgroundPixelTarget raster;GumpBackgroundDraw draw; // 새 알고리즘이 출력하는 제품의 실제 픽셀 경계다.
    std::int32_t width,height; // 이 장면의 실제 표면 크기이며 고정 소스 타일과 다르다.

    GumpCreateNode* Find(std::uint32_t token) { return token==1 ? &root:token==2 ? &node:nullptr; } // 공통/파생 조회가 같은 주소를 반환한다.
    GumpBackgroundDrawHooks Hooks(const GumpBackgroundAssets& assets) { // 자산 정책·원문 프레임·같은 실제 픽셀을 연결한다.
        GumpBackgroundCreateHooks createHooks;GumpBackgroundDrawHooks hooks;hooks.node=[this](auto token) { return token==2 ? &node:nullptr; };BindGumpBackgroundAssets(createHooks,hooks,assets);return raster.Hooks(clip,std::move(hooks));
    }
    WidePanelScene(const GumpBackgroundAssets& assets,std::int32_t selectedWidth,std::int32_t selectedHeight,ScreenRect selectedClip)
        :factory(create,{[this](auto token) { return Find(token); },[](auto) { return 1U; }}),collection(create.collection,factory.CollectionHooks()),guarded(static_cast<std::size_t>(selectedWidth)*selectedHeight+2,55),target(std::span(guarded).subspan(1,guarded.size()-2),selectedWidth,selectedHeight,clip),maps(Maps()),raster(target,assets,maps,colors),draw(collection,clip,state,Hooks(assets)),width(selectedWidth),height(selectedHeight) { // 생성 도중 실제 출력은 하지 않는다.
        create.screenWidth=width;create.screenHeight=height;factory.InitializeRoot();SetGumpDrawClip(clip,selectedClip);
    }
    void Paint(ScreenRect rect,std::uint32_t flags=GumpBackgroundFlags::kTexture) { draw.Texture(rect,flags,kPanelType,0); } // 같은 제품 Texture에만 명시 영역을 넘긴다.
    std::uint8_t Pixel(std::int32_t x,std::int32_t y) const { return guarded[1+static_cast<std::size_t>(y)*width+x]; } // 표면 밖 guard와 실제 픽셀을 구별한다.
};
}

TEST_CASE(GumpWidePanel_First774RowsStayOriginalAndOnlyBodyRepeats) { // 기존 화면의 모든 픽셀과 여러 번 확장한 실제 본문 행을 함께 대조한다.
    const auto shapes=WidePanelShapes();const std::array<GumpBackgroundAssetBinding,1> legacyBindings{{{kPanelType,0,1,0}}},wideBindings{{{kPanelType,0,1,0,0}}};GumpBackgroundAssets legacy(shapes,legacyBindings),wide(shapes,wideBindings);
    CHECK(legacy.RepeatBodyStart(kPanelType,0)==0);CHECK(wide.RepeatBodyStart(kPanelType,0)==25);constexpr std::int32_t height=1700; // 첫 전체 타일 뒤 본문이 두 번 이상 반복되는 실제 표면이다.
    WidePanelScene before(legacy,80,height,{0,0,80,height}),after(wide,80,height,{0,0,80,height});before.Paint({0,0,80,height});after.Paint({0,0,80,height});
    for (std::int32_t y=0;y<774;++y) for (std::int32_t x=0;x<80;++x) CHECK(before.Pixel(x,y)==after.Pixel(x,y)); // 원문768을 포함하는 최초774행 전체는 비트 단위로 같다.
    CHECK(before.Pixel(0,774)==0 && before.Pixel(79,798)==0);CHECK(after.Pixel(0,0)==0 && after.Pixel(79,24)==0); // 팔레트0은 최초 타일에서 계속 불투명 검정이다.
    constexpr std::array<std::array<std::int32_t,2>,7> rows{{{774,25},{798,49},{799,50},{1080,331},{1522,773},{1523,25},{1699,201}}}; // 독립 행 색으로 재시작/749행 본문 주기를 구별한다.
    for (const auto& row:rows) for (std::int32_t x=0;x<80;++x) CHECK(after.Pixel(x,row[0])==WidePanelColor(row[1])); // 모든 가로 픽셀도 동일한 명시 소스 행을 사용한다.
    CHECK(before.guarded.front()==55 && before.guarded.back()==55 && after.guarded.front()==55 && after.guarded.back()==55);
}

TEST_CASE(GumpWidePanel_DirtyClipAndShadeKeepGlobalBodyPhase) { // 반복 시작보다 아래의 작은 dirty에서도 원점/현재색/guard를 보존한다.
    const auto shapes=WidePanelShapes();const std::array<GumpBackgroundAssetBinding,1> bindings{{{kPanelType,0,1,0,0}}};GumpBackgroundAssets assets(shapes,bindings);const ScreenRect region{3,780,77,805};WidePanelScene scene(assets,80,1080,region);scene.Paint({0,26,80,1080},GumpBackgroundFlags::kTexture|GumpBackgroundFlags::kBright);
    for (std::int32_t y=0;y<1080;++y) for (std::int32_t x=0;x<80;++x) { // dirty 안은 명시 소스31..55행의 밝은 색, 밖은 최초55를 유지한다.
        const auto expected=x>=3 && x<77 && y>=780 && y<805 ? static_cast<std::uint8_t>(WidePanelColor(y-749)+10):std::uint8_t{55};CHECK(scene.Pixel(x,y)==expected);
    }
    CHECK(scene.clip.clip.left==region.left && scene.clip.clip.top==region.top && scene.clip.clip.right==region.right && scene.clip.clip.bottom==region.bottom);CHECK(scene.clip.vfxClip.right==76 && scene.clip.vfxClip.bottom==804);CHECK(scene.guarded.front()==55 && scene.guarded.back()==55);
}

TEST_CASE(GumpWidePanel_UsesSelectedMetricsAndDoesNotTreatTransparentBlackAsHeader) { // 25/774 하드코딩이나 모든 검정 픽셀의 투명화를 구별한다.
    const auto shapes=WidePanelShapes(7,13,3);const std::array<GumpBackgroundAssetBinding,1> bindings{{{kPanelType,0,1,0,0}}};GumpBackgroundAssets assets(shapes,bindings);CHECK(assets.RepeatBodyStart(kPanelType,0)==3);WidePanelScene scene(assets,7,37,{0,0,7,37});scene.Paint({0,0,7,37});CHECK(scene.Pixel(0,0)==0 && scene.Pixel(6,2)==0);CHECK(scene.Pixel(0,13)==WidePanelColor(3,3) && scene.Pixel(6,22)==WidePanelColor(12,3));CHECK(scene.Pixel(0,23)==WidePanelColor(3,3));
    const auto transparent=WidePanelShapes(7,13,3,true);GumpBackgroundAssets unchanged(transparent,bindings);CHECK(unchanged.RepeatBodyStart(kPanelType,0)==0);WidePanelScene original(unchanged,7,37,{0,0,7,37});original.Paint({0,0,7,37});CHECK(original.Pixel(0,0)==55 && original.Pixel(6,13)==55 && original.Pixel(0,16)==WidePanelColor(3,3));
}
