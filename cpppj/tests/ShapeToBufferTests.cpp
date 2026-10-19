// 두 실제 ShapeToBuffer와 보호 Dude 프레임의 전체 plain RLE 메모리 출력을 대조한다.
#include "RawSceneSupport.h"
#include "client/RawShapeToBuffer.h"
#include "client/SquidRenderer.h"
#include "client/Renderer.h"
#include "o/RawFactoryDraw.h"
#include <cstdio>
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// native 기대 픽셀은 fixture에서만 읽으며 현재 입력/버퍼 준비에 사용하지 않는다.
const std::vector<std::vector<std::string>>& BufferRows() { static const auto rows=LoadFixture(NETSTORM_SHAPETOBUFFER_FIXTURE).rows;return rows; }
// 두 판본의 실제 TYPE/SHP와 별도 Squid 헤더를 읽기 전용으로 공급한다.
struct BufferAssets {
    std::filesystem::path root; // 실제 판본의 보호 TAFF 경로다.
    BaseFileSystem files; // 기존 실제 자산 읽기 계층이다.
    netstorm::client::GameAssets game; // 같은 보호 TYPE/SHP를 해석한 현재 저장소다.
    std::vector<RiftTypeRecord> types; // 입력 TYPE 기준점만−2/3으로 바꾸는 실제 타입 표다.
    std::vector<SquidDisplayShape> shapes; // 전체 물리 프레임/현재 frameCount의 실제 메타데이터다.
    // 창/장치를 만들지 않고 같은 보호 파일만 읽는다.
    explicit BufferAssets(OriginalEdition edition):root(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")),files(root),game(Register(),edition),types(game.TypeTable().Types().begin(),game.TypeTable().Types().end()),shapes(netstorm::client::SquidRenderer::Shapes(game,edition)) { types[70].hotspotX=-2;types[70].hotspotY=3; }
    // GameAssets 초기화 전에 동일 archive를 등록한다.
    const BaseFileSystem& Register() { files.RegisterArchive(root/"netstorm.tarc");return files; }
};
// 각 판본의 보호 파일을 반복 행에서 다시 읽지 않는다.
const BufferAssets& Assets(OriginalEdition edition) { static const BufferAssets patch(OriginalEdition::Patch1078),cd(OriginalEdition::Cd1072);return edition==OriginalEdition::Patch1078 ? patch : cd; }
// 현재 raw/배율/옵션 입력만 공급하고 실제 픽셀 캡처의 메타데이터/전체 버퍼를 직렬화한다.
std::vector<std::string> RunBuffer(const std::vector<std::string>& row) {
    SidPool pool(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,true);
    // source50은 실제 client 슬롯이며 뒤의 free 상태 입력은 확보한 원문에만 쓴다.
    for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    const auto& assets=Assets(pool.Edition());auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,Number(row[5]));raw[10]=70;Put(raw,pool.Edition()==OriginalEdition::Patch1078 ? 36U : 34U,Number(row[1]),pool.Edition()==OriginalEdition::Patch1078 ? 4U : 1U);raw[11]=static_cast<std::uint8_t>(Number(row[4]));
    netstorm::client::ShapeToBufferState state{std::bit_cast<std::int32_t>(Number(row[2])),Number(row[3])!=0,{}};netstorm::client::RawShapeToBuffer capture(pool,assets.types,assets.shapes,assets.game.Shapes(),state);const auto image=capture.Capture(Sid{50});
    return {std::to_string(image.width)+','+std::to_string(image.height)+','+std::to_string(image.hotspotX)+','+std::to_string(image.hotspotY),Hex(image.pixels).empty() ? "-" : Hex(image.pixels),std::to_string(image.pixels.size()),"1"};
}
// 두 판본의 독립 입력은 실제 실행 후에만 기대 픽셀과 비교한다.
void ReplayBuffer(std::string_view edition) {
    std::size_t count=0,expectedCount=0;int failures=0;
    // 고유 입력행 수만 계산하며 출력 칸을 입력으로 사용하지 않는다.
    for (const auto& row:BufferRows()) if (row[0]==edition) ++expectedCount;
    // 현재 frame/zoom/추가기준점/free/가상표와 마지막 네 실제 출력을 구분한다.
    for (const auto& row:BufferRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==10);const auto output=RunBuffer(row);const auto expected=std::vector<std::string>(row.begin()+6,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 첫 차이는 실제 native/C++ 값과 행 번호를 함께 기록한다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s ShapeToBuffer 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==expectedCount);
}
}
// 실제10.78 ShapeToBuffer/보호SHP/plain RLE와 원래 메모리 문맥 복원을 대조한다.
TEST_CASE(ShapeToBuffer_PatchNativePlainPixels) { ReplayBuffer("originals"); }
// CD10.72의 invalidsource8×8 투명 fallback과 같은 실제 plain RLE를 대조한다.
TEST_CASE(ShapeToBuffer_CdNativePlainPixelsAndFallback) { ReplayBuffer("originalCD"); }
// 원본 Factory 가상 몸체/전체 보호sunFactory RLE의200입력과 기본·추가 그림을 대조한다.
TEST_CASE(ShapeToBuffer_FactoryNative200Pixels) {
    const auto& assets=Assets(OriginalEdition::Patch1078);auto types=assets.types;types[85].hotspotX=-2;types[85].hotspotY=3;
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::array<std::uint32_t,188> maps{}; // 현재 보호 프레임과 원본 TYPE85의 null 색표 입력이다.
    // 같은 실제 로딩 순서의 모든 TYPE 프레임 코드를 조회한다.
    for (const auto& asset:assets.game.Types()) frames[70+asset.block]=asset.definition.FrameTable();
    std::size_t count{},overlays{}; // 실제 C++ 캡처와 추가 RLE 실행 수다.
    // 동결된 원문 입력으로만 준비하고 전체 기대 픽셀을 구현에 전달하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_FACTORY_CAPTURE_FIXTURE).rows) {
        if (row[0]=="vtable") continue;CHECK(row.size()==10);if (row.size()!=10) return;
        SidPool pool(OriginalEdition::Patch1078,32768,true);
        // 원본 격리 입력과 같은 현재 client50 슬롯을 확보한다.
        for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,Number(row[0]));raw[10]=85;raw[11]=static_cast<std::uint8_t>(Number(row[1])<<5);raw[34]=1;raw[40]=static_cast<std::uint8_t>(Number(row[2]));Put(raw,36,Number(row[3]));
        RawFactoryDraw factory(pool,frames);netstorm::client::ShapeToBufferState state;state.zoom=static_cast<std::int32_t>(Number(row[5]));state.addTypeHotspot=Number(row[4])!=0;state.ownerTypeMaps=maps;
        state.mainFrame=[&](Sid sid) { return factory.MainFrame(sid); };
        state.additional=[&](Sid sid,ConstructionVisualImage& image,std::int32_t x,std::int32_t y) {
            factory.Additional(sid,x,y,[&](std::int32_t px,std::int32_t py,std::uint32_t type,std::int32_t frame) {
                CHECK(state.zoom==65536 && type==85);++overlays;
                static_cast<void>(netstorm::client::DrawShape(image.pixels,image.width,image.height,assets.game.Shapes(),type-70,static_cast<std::size_t>(frame),px,py,{0,0,image.width-1,image.height-1}));
            });
        };
        netstorm::client::RawShapeToBuffer capture(pool,types,assets.shapes,assets.game.Shapes(),state);const auto image=capture.Capture(Sid{50});
        const std::vector<std::string> actual{std::to_string(image.width)+','+std::to_string(image.height)+','+std::to_string(image.hotspotX)+','+std::to_string(image.hotspotY),Hex(image.pixels),std::to_string(image.pixels.size()),"1"};
        const auto same=actual==std::vector<std::string>(row.begin()+6,row.end());if (!same) std::printf("Factory 원문 픽셀 차이 %zu VT%s Level%s extra%s frame%s\n",count,row[0].c_str(),row[1].c_str(),row[2].c_str(),row[3].c_str());CHECK(same);++count;
    }
    CHECK(count==200 && overlays==60);
}

// 두 섬 조각의162개 원문과 모든 보호 RLE 픽셀·기준점을 대조한다.
TEST_CASE(ShapeToBuffer_SupportNative162Pixels) {
    const auto& assets=Assets(OriginalEdition::Patch1078);auto types=assets.types;std::array<std::uint32_t,188> maps{};
    types[95].hotspotX=types[157].hotspotX=-2;types[95].hotspotY=types[157].hotspotY=3; // 원문이 선언한TYPE 기준점 입력이다.
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/support-capture-x86.tsv").string();std::size_t count{};
    // 실제 기본/추가 Draw가 공통인 두 정확한 가상 표만 구성한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        const std::uint32_t type=row[0]=="islandStalag" ? 95U : 157U;const std::uint32_t table=type==95 ? 0x506e50U : 0x506f08U;
        SidPool pool(OriginalEdition::Patch1078,32768,true);
        // 원본 격리 장면과 같은 client50 슬롯까지 확보한다.
        for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,table);raw[10]=static_cast<std::uint8_t>(type);raw[34]=static_cast<std::uint8_t>(Number(row[3]));raw[40]=static_cast<std::uint8_t>(Number(row[2]));Put(raw,36,Number(row[1]));
        netstorm::client::ShapeToBufferState state;state.addTypeHotspot=true;state.ownerTypeMaps=maps;
        state.mainFrame=[&](Sid) { return assets.game.Types()[type-70].definition.specialFrames.baseFrame; }; // 실제4acfe0의 TYPE 기본 프레임이다.
        state.additional=[](Sid,ConstructionVisualImage&,std::int32_t,std::int32_t) {}; // 실제41c100은 인자를 읽지 않는ret8이다.
        netstorm::client::RawShapeToBuffer capture(pool,types,assets.shapes,assets.game.Shapes(),state);const auto image=capture.Capture(Sid{50});
        const auto metadata=std::to_string(image.width)+','+std::to_string(image.height)+','+std::to_string(image.hotspotX)+','+std::to_string(image.hotspotY);
        CHECK(row[4]==row[6] && row[5]==row[7] && metadata==row[4] && Hex(image.pixels)==row[5]);++count;
    }
    CHECK(count==162);
}
