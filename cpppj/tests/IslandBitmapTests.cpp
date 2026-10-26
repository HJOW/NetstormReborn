// 실제 보호 texture/RLE와 원본 bitmap28행의 전체 픽셀·분류 캐시·난수를 비교한다.
#include "DudeSpawnSupport.h"
#include "client/EditorIslandBitmap.h"
#include "client/ClientGameplayGlobals.h"
#include "o/RawTerrainGrowth.h"
#include <algorithm>
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 기대값을 생성하지 않고 Native가 사용한 명시 문자 입력만 준비한다.
// Native와 같은 X열 우선 입력이다. bitmap 출력 기대값은 fixture를 직접 읽는다.
std::array<std::uint8_t,65536> BitmapCodes(std::uint32_t profile) {
    std::array<std::uint8_t,65536> result;result.fill('.');
    for (std::size_t x=0;x<64;++x) { // 사용 영역 밖의 원문 배열도 허공 입력을 유지한다.
        for (std::size_t y=0;y<64;++y) { // 해당 X열의 Y 입력을 Native와 같은 패턴으로 공급한다.
            std::uint8_t value='.';
            if (profile==1) value=x>=2 && x<13 && y>=2 && y<11 ? 'x' : x>=2 && x<13 && y==11 ? '=' : '.';
            else if (profile==2) value=static_cast<std::uint8_t>(std::string_view("x=..Xo")[(x*7+y)%6]);
            else if (profile==3) value=y%8==0 ? 'x' : y%8==1 || y%8==2 ? '=' : '.';
            else if (profile==4) value=y==0 ? 'x' : y==1 ? '=' : '.';
            result[x*256+y]=value;
        }
    }
    return result;
}
}

// 현재 자산 texture0은 Native가 직접 그린 두 보호 SHP와 같은8비트 픽셀이다.
TEST_CASE(IslandBitmap_ReplaysNativeRasterAndActualProtectedRle) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto fixture=LoadFixture(NETSTORM_ISLAND_BITMAP_FIXTURE);std::uint32_t checked{};
    for (const auto& row:fixture.rows) { // 두 x87 관측도 같은 Native 입력/출력 행으로 별도 재생한다.
        if (row[0]=="name") continue;
        const auto width=static_cast<std::int32_t>(Number(row[1])),height=static_cast<std::int32_t>(Number(row[2]));const auto input=BitmapCodes(Number(row[7]));
        TerrainBitmapState state;state.initialized=Number(row[5]);state.classified['x']=static_cast<std::uint8_t>(Number(row[6]));GameRandom random(Number(row[3]));const std::uint32_t island=97;
        std::vector<std::uint32_t> textures;std::uint32_t greenCalls{};
        RawTerrainBitmap body(state,island,{[&] { ++greenCalls;return Number(row[4]); },[&](auto type) { textures.push_back(type);return ReadEditorIslandTexture(assets.game,type); }});
        const auto image=body.Render(input,width,height,random);
        CHECK(image.width==width && image.height==height && image.pixels.size()==static_cast<std::size_t>(width)*height);
        CHECK(random.State()==Number(row[9]) && state.initialized==1 && state.classified['x']==Number(row[10]) && Number(row[11])==0xa5);
        CHECK(textures==std::vector<std::uint32_t>({102,97}) && greenCalls==(Number(row[5]) ? 0U : 1U));
        const auto actual=image.pixels.empty() ? std::string("-") : Hex(image.pixels);if (actual!=row[13]) std::fprintf(stderr,"Bitmap Native행%u %s 픽셀 불일치\n",checked,row[0].c_str());CHECK(actual==row[13]);++checked;
    }
    CHECK(checked==28);
}

// 동일 Client의 최초 분류는 대여/팔레트 교체로 초기화되지 않고 출력은 World를 변경하지 않는다.
TEST_CASE(IslandBitmap_ActualGrowthLocalSeedAndPersistentClassification) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientGameplayGlobals client;auto& state=client.TerrainBitmaps();CHECK(!state.initialized);
    ChunkMap map(14,14);auto& cell=map.At(2,2);cell.island=0;cell.side='A';cell.record={0,1,0x22,37,0,0};const std::vector<ChunkEntry> original(map.Entries().begin(),map.Entries().end());TerrainContextGlobals globals;
    std::uint32_t greenCalls{};const auto green=[&] { ++greenCalls;return 43U; };
    const auto first=BuildEditorIslandBitmap(assets.game,map,1,{0,0,14,14},globals,state,green);
    CHECK(first.width==224 && first.height==224 && first.pixels.size()==50176 && greenCalls==1 && state.initialized==1 && state.classified['x']==43);
    CHECK(std::any_of(first.pixels.begin(),first.pixels.end(),[](auto value) { return value!=0; }));
    { auto use=client.AcquirePrepared();CHECK(&client.TerrainBitmaps()==&state); }CHECK(&client.TerrainBitmaps()==&state && state.classified['x']==43);
    const auto second=BuildEditorIslandBitmap(assets.game,map,1,{0,0,14,14},globals,state,[]()->std::uint32_t { throw std::logic_error("캐시 뒤 다시 읽지 않는 녹색"); });
    CHECK(first.pixels==second.pixels && std::equal(original.begin(),original.end(),map.Entries().begin(),[](const auto& a,const auto& b) { return a.island==b.island && a.side==b.side && a.record==b.record && a.ordinal==b.ordinal; }));
}

// 실제 texture 장치가 실패해도 앞선 최초 분류 완료는 되돌리지 않으며 아직 로컬 난수는 소비하지 않는다.
TEST_CASE(IslandBitmap_TextureFailurePreservesClassificationPrefix) {
    TerrainBitmapState state;state.classified.fill(0xa5);const std::uint32_t island=97;GameRandom random(123);std::uint32_t calls{};
    RawTerrainBitmap body(state,island,{[] { return 43U; },[&](auto type)->TerrainBitmapTexture { ++calls;CHECK(type==102);throw std::logic_error("selected-texture-stop"); }});
    const auto input=BitmapCodes(1);CHECK(Throws([&] { static_cast<void>(body.Render(input,16,16,random)); }));
    CHECK(calls==1 && random.State()==123 && state.initialized==1 && state.classified['x']==43 && state.classified['.']==0 && state.classified['=']==0);
}

// 실제 전체 부모004c1cc0이 생성한 지형/경계/RLE/Bitmap 결과와 합성하지 않은 픽셀을 대조한다.
TEST_CASE(IslandBitmap_ReplaysNativeWholeParentWithContextGrowthAndRle) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto fixture=LoadFixture(NETSTORM_ISLAND_BITMAP_PARENT_FIXTURE);std::uint32_t checked{};
    for (const auto& row:fixture.rows) { // offset bounds와 owner0/1/8 및 실제14×14 크기도 별도 Native 행이다.
        if (row[0]=="name") continue;
        ChunkMap map(14,14);const auto profile=Number(row[6]);const std::vector<std::array<int,2>> points=profile==0 ? std::vector<std::array<int,2>>{} : profile==1 ? std::vector<std::array<int,2>>{{2,2}} : std::vector<std::array<int,2>>{{2,2},{3,2},{2,3},{3,3}};
        for (std::size_t index=0;index<points.size();++index) { // 원본 CPU에 공급한 같은 물리 청크/side/record seed 입력만 작성한다.
            auto& cell=map.At(points[index][0],points[index][1]);cell.island=0;cell.side=static_cast<char>('A'+index);cell.record={0,1,0x22,static_cast<std::uint8_t>(37+index),0,0};
        }
        TerrainBitmapState state;state.initialized=1;state.classified['x']=43;TerrainContextGlobals globals;
        const auto image=BuildEditorIslandBitmap(assets.game,map,Number(row[1]),{std::stoi(row[2]),std::stoi(row[3]),std::stoi(row[4]),std::stoi(row[5])},globals,state,[]()->std::uint32_t { throw std::logic_error("Native 전체 부모의 준비된 최초 분류"); });
        CHECK(image.width==static_cast<std::int32_t>(Number(row[8])) && image.height==static_cast<std::int32_t>(Number(row[9])));
        const auto actual=Hex(image.pixels);if (actual!=row[11]) std::fprintf(stderr,"Bitmap 전체 부모행%u %s 픽셀 불일치\n",checked,row[0].c_str());CHECK(actual==row[11]);++checked;
    }
    CHECK(checked==12);
}
