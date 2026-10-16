// 실제9개 색표와 mapped/plain 보호 SHP 캡처 전체를 원문 기대값과 비교한다.
#include "RawSceneSupport.h"
#include "client/RawShapeToBuffer.h"
#include "client/OwnerColorMaps.h"
#include "client/SquidRenderer.h"
#include <filesystem>

using namespace netstorm::test::rawscene;
namespace {
// 원본 입력과 출력은 원문 TSV에서만 읽는다.
const std::vector<std::vector<std::string>>& Rows() { static const auto rows=LoadFixture(NETSTORM_OWNER_CAPTURE_FIXTURE).rows;return rows; }
// 독립 hex를 바이트로 읽으며 기대값을 계산하지 않는다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    if (text.size()%2) throw std::invalid_argument("소유자 캡처 hex 폭 오류");std::vector<std::uint8_t> result;
    // 두 문자마다 저장된 바이트 하나를 복원한다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return result;
}
// 실제 네 타입의 현재 기준점 입력만−2/3으로 바꾼 보호 자산이다.
struct Assets {
    std::filesystem::path root; // 수정하지 않는10.78 원본 경로다.
    BaseFileSystem files; // 같은 보호 TAFF 읽기다.
    netstorm::client::GameAssets game; // 실제 TYPE/SHP/팔레트다.
    std::vector<RiftTypeRecord> types; // 캡처 진단과 기준점의 현재 TYPE 입력이다.
    std::vector<SquidDisplayShape> shapes; // 실제 클러스터 수/물리 헤더다.
    // GUI 없이 보호 자산만 읽고 native와 같은 명시 기준점을 설정한다.
    Assets():root(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"),files(root),game(Register(),OriginalEdition::Patch1078),types(game.TypeTable().Types().begin(),game.TypeTable().Types().end()),shapes(netstorm::client::SquidRenderer::Shapes(game,OriginalEdition::Patch1078)) {
        // 이번 명시 입력의 네 타입만 추가 기준점을 바꾼다.
        for (const auto type:{70,73,128,144}) { types[type].hotspotX=-2;types[type].hotspotY=3; }
    }
    // GameAssets보다 먼저 같은 archive를 등록한다.
    const BaseFileSystem& Register() { files.RegisterArchive(root/"netstorm.tarc");return files; }
};
// 의미 상태를 little endian DWORD로 직렬화한다.
void Append(std::vector<std::uint8_t>& bytes,std::int32_t value) {
    // signed 값도 원래32비트의 낮은 바이트부터 출력한다.
    for (std::size_t index=0;index<4;++index) bytes.push_back(static_cast<std::uint8_t>(static_cast<std::uint32_t>(value)>>(index*8)));
}
}
// 원래72DWORD와 다른 후보/높은 비트/다리 색2·7의9×256 전체 원문을 비교한다.
TEST_CASE(OwnerColorMaps_NativeAllRowsAndBridge) {
    std::size_t count=0;
    // 반환9표는 입력 후보에서 직접 생성한 뒤에만 기대값을 읽어 비교한다.
    for (const auto& row:Rows()) {
        if (row[0]!="M") continue;if (row.size()!=5) throw std::runtime_error("소유자 색표 fixture 열 수 오류");const auto input=Decode(row[3]);if (input.size()!=72*4) throw std::runtime_error("소유자 후보 DWORD 폭 오류");
        std::array<std::uint32_t,72> candidates;
        // 높은 비트를 포함한 원본 DWORD 후보를 유지한다.
        for (std::size_t index=0;index<candidates.size();++index) candidates[index]=Get(input,index*4);
        if (!count) CHECK(std::equal(candidates.begin(),candidates.end(),netstorm::client::OriginalOwnerColorCandidates().begin()));
        const auto maps=netstorm::client::BuildStandardOwnerColorMaps(Number(row[1]),Number(row[2]),candidates);std::vector<std::uint8_t> output;
        // 항등0행과 변경되지 않은 모든 색도 포함한다.
        for (const auto& map:maps) output.insert(output.end(),map.begin(),map.end());
        CHECK(output==Decode(row[4]));++count;
    }
    CHECK(count==18);
}
// 네 보호 타입의 현재 소유자/색0/8·zoom·기준점과 실제 전체 픽셀을 비교한다.
TEST_CASE(ShapeToBuffer_NativeMappedOwnerPixels) {
    const Assets assets;SidPool pool(OriginalEdition::Patch1078,32768,true);
    // 실제 source50까지 슬롯을 확보한다.
    for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    std::size_t count=0;
    // 원문480개 캡처 입력을 기대 출력과 분리해 공급한다.
    for (const auto& row:Rows()) {
        if (row[0]!="C") continue;if (row.size()!=9) throw std::runtime_error("소유자 캡처 fixture 열 수 오류");const auto type=Number(row[1]),owner=Number(row[3]),color=Number(row[4]);
        const auto maps=netstorm::client::BuildStandardOwnerColorMaps(type,82);auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,type==144 ? 0x50a388 : 0x501dd0);raw[10]=static_cast<std::uint8_t>(type);raw[34]=static_cast<std::uint8_t>(owner);Put(raw,36,Number(row[2]));
        std::vector<std::uint32_t> tokens(188);tokens[type]=1;std::size_t calls=0;netstorm::client::ShapeToBufferState state;state.zoom=std::bit_cast<std::int32_t>(Number(row[5]));state.addTypeHotspot=Number(row[6])!=0;state.ownerTypeMaps=tokens;
        state.ownerColors=[&](std::uint32_t currentType,std::uint32_t currentOwner)->std::span<const std::uint8_t> { CHECK(currentType==type && currentOwner==owner);++calls;return maps.at(color); };
        netstorm::client::RawShapeToBuffer capture(pool,assets.types,assets.shapes,assets.game.Shapes(),state);const auto image=capture.Capture(Sid{50});std::vector<std::uint8_t> metadata;
        // 캡처 메타데이터는 원본 signed DWORD 순서다.
        for (const auto value:{image.width,image.height,image.hotspotX,image.hotspotY}) Append(metadata,value);
        CHECK(metadata==Decode(row[7]));CHECK(image.pixels==Decode(row[8]));CHECK(calls==(owner ? 1U : 0U));++count;
    }
    CHECK(count==480);
}
