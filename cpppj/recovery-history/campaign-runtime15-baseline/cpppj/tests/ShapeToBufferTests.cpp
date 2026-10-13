// 두 실제 ShapeToBuffer와 보호 Dude 프레임의 전체 plain RLE 메모리 출력을 대조한다.
#include "RawSceneSupport.h"
#include "client/RawShapeToBuffer.h"
#include "client/SquidRenderer.h"
#include "client/Renderer.h"
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
