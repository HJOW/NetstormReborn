// 세 실제 PE의 전체 픽셀 모양 getter 관찰과 배치 연결/프레임 오류를 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPlacementShape.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 독립 입력의 총수와 실제 사제 타입 번호다.
constexpr std::size_t kTotal=8064;
constexpr std::uint32_t kPriest=158;
// 원본 관찰을 읽기만 하며 기대 픽셀 좌표를 구현으로 계산하지 않는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_PRIESTSHAPE_FIXTURE).rows;return rows;
}
// 입력 헤더의 signed short 저장 폭을 원본과 같게 유지한다.
std::int16_t Short(int value) { return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value)); }
// 여섯 출력 float의 비트를 고정 순서로 비교한다.
std::array<std::uint32_t,6> Bits(const PriestPlacementPixelShape& shape) {
    return {std::bit_cast<std::uint32_t>(shape.bounds.left),std::bit_cast<std::uint32_t>(shape.bounds.top),
        std::bit_cast<std::uint32_t>(shape.bounds.right),std::bit_cast<std::uint32_t>(shape.bounds.bottom),
        std::bit_cast<std::uint32_t>(shape.anchorX),std::bit_cast<std::uint32_t>(shape.anchorY)};
}
// 실제 getter가 계산한 픽셀 범위/기준점에 메타 자료 입력만 공급한다.
void Replay(std::string_view name) {
    const bool patch=name=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,true);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
    std::vector<SquidDisplayShape> shapes(types.size());std::vector<PriestTypeHotspot> hotspots(types.size());
    PriestPlacementGeometryState geometryState;PriestPlacementShapeState state;types[kPriest].flags2=0x210000;
    RawPriestPlacementShape shape(pool,types,frames,shapes,hotspots,geometryState,state);CHECK(Fixture().size()==kTotal);std::size_t count=0;
    // 각 행의 원본 입력으로 같은 프레임 번호 체계/현재 타입 기준점을 구성한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=name) continue;CHECK(row.size()==22);
        const auto type=Number(row[1]);frames[type].frames=RiftTypeFrames(std::vector<FrameCode>(Number(row[5])));frames[type].defaultFrame=std::stoi(row[6]);types[type].flags1=Number(row[7]);
        hotspots[type]={std::stoi(row[12]),std::stoi(row[13])};state={std::bit_cast<float>(Number(row[14])),std::bit_cast<float>(Number(row[15]))};
        auto& shp=shapes[type];shp.loaded=true;shp.frameCount=static_cast<int>(Number(row[5]));shp.frames.clear();
        // 입력 생성기의 비연속 주소 표와 같은 물리 프레임 순서다. 주소 자체는 호스트로 복사하지 않는다.
        for (int frame=0;frame<8;++frame) shp.frames.push_back({Short(std::stoi(row[8])+frame*2),Short(std::stoi(row[9])-frame),Short(std::stoi(row[10])+frame),Short(std::stoi(row[11])-frame),0,0});
        std::array<std::uint32_t,6> expected{};
        // 기대값은 실제 기계어 fixture의 float 비트만 읽는다.
        for (std::size_t i=0;i<expected.size();++i) expected[i]=Number(row[16+i]);
        const auto actual=Bits(shape.Measure(type,Number(row[2]),Number(row[3]),Number(row[4])!=0));CHECK(actual==expected);
        if (actual!=expected) { std::printf("Priest shape %s 행 %zu\n",std::string(name).c_str(),count);break; }++count;
    }
    CHECK(count==(patch ? 4032U : 2016U));
}
}
// 10.78의 전체 getter·실제 SHP 조회와 픽셀 결과 비트를 대조한다.
TEST_CASE(priest_shape_patch_x86) { Replay("originals"); }
// CD의 다른 float 저장/최댓값 선택·직접 SHP 조회를 대조한다.
TEST_CASE(priest_shape_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 전체 getter 관찰도 별도로 대조한다.
TEST_CASE(priest_shape_1037_x86) { Replay("original1037"); }
// 실제 SHP 크기가 지도 여백 거부를 결정하고 허용된 검사만 기존 geometry로 진행한다.
TEST_CASE(priest_shape_placement_margin_and_current_metadata) {
    // 각 판본에 동일한 SHP 메타 자료를 연결하며 일반 지형 몸체는 입력 경계로 남긴다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        std::vector<PriestPlainCanonType> frames(types.size());std::vector<SquidDisplayShape> shapes(types.size());std::vector<PriestTypeHotspot> hotspots(types.size());
        PriestPlacementGeometryState geometryState;PriestPlacementShapeState shapeState;PriestPlacementState placementState{0,1,1};
        types[kPriest].flags2=0x210000;frames[kPriest].frames=RiftTypeFrames(std::vector<FrameCode>(2));shapes[kPriest]={2,true,{{160,110,80,55,0,0},{16,11,8,5,0,0}}};hotspots[kPriest]={8,5};
        RawPriestPlacementShape shape(pool,types,frames,shapes,hotspots,geometryState,shapeState);int inspections=0;
        RawPriestPlacement placement(pool,types,placementState,MakePriestShapePlacementHooks(pool,shape,{{},[&](const PriestPlacementQuery&,bool local) { CHECK(local);++inspections;return true; }}));
        const PriestPlacementQuery query{kPriest,9,9,0,1,0};CHECK(!placement.MayPlace(query) && inspections==0 && placementState.blockedRelation==0);
        frames[kPriest].defaultFrame=1;CHECK(placement.MayPlace(query) && inspections==1);
        const auto selected=shape.Measure(kPriest,0,0,true);CHECK(selected.bounds.right-selected.bounds.left==160 && selected.bounds.bottom-selected.bounds.top==110);
        frames[kPriest].defaultFrame=-1;shapes[kPriest].loaded=false;const auto empty=shape.Measure(kPriest,kPriest,0);
        CHECK(empty.bounds.left==1000 && empty.bounds.top==1000 && empty.bounds.right==0 && empty.bounds.bottom==0 && empty.anchorX==16 && empty.anchorY==11);
    }
}
// 패치의 frameCheck와 CD의 직접 참조를 구별하고 표 밖 접근을 거부한다.
TEST_CASE(priest_shape_frame_validation_and_connection_guards) {
    // 원본 assert UI와 잘못된 메모리 접근은 C++ 예외로 명시한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true),other(edition,kCapacity,true);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());std::vector<SquidDisplayShape> shapes(types.size());std::vector<PriestTypeHotspot> hotspots(types.size());
        PriestPlacementGeometryState geometryState;PriestPlacementShapeState state;types[kPriest].flags2=0x200000;
        frames[kPriest].frames=RiftTypeFrames(std::vector<FrameCode>(1));frames[kPriest].defaultFrame=1;shapes[kPriest]={1,true,std::vector<SquidDisplayFrame>(2)};
        RawPriestPlacementShape shape(pool,types,frames,shapes,hotspots,geometryState,state);
        if (patch) CHECK(Throws([&] { shape.Measure(kPriest,kPriest,0); }));else CHECK(shape.Measure(kPriest,kPriest,0).bounds.right==0);
        types[kPriest].flags1=0x40000;CHECK(shape.Measure(kPriest,kPriest,0).bounds.right==0);
        types[kPriest].maxHitPoints=-0x22222223;
        if (patch) CHECK(Throws([&] { shape.Measure(kPriest,kPriest,0); }));else CHECK(shape.Measure(kPriest,kPriest,0).bounds.right==0);
        types[kPriest].maxHitPoints=0;frames[kPriest].defaultFrame=2;CHECK(Throws([&] { shape.Measure(kPriest,kPriest,0); }));frames[kPriest].defaultFrame=0;
        shapes[kPriest].loaded=false;CHECK(Throws([&] { shape.Measure(kPriest,kPriest,0); }));shapes[kPriest].loaded=true;
        CHECK(Throws([&] { shape.Measure(kPriest,1,0,true); }));geometryState.patternTypes[2]=kPriest;CHECK(Throws([&] { shape.Measure(kPriest,kPriest,0); }));geometryState.patternTypes[2]=0;
        CHECK(Throws([&] { MakePriestShapePlacementHooks(other,shape,{}); }));CHECK(Throws([&] { RawPriestPlacementShape bad(pool,types,frames,{},hotspots,geometryState,state); }));
        state.scaleX=std::numeric_limits<float>::infinity();CHECK(Throws([&] { shape.Measure(kPriest,kPriest,0); }));
        types[kPriest].flags2=0;CHECK(Throws([&] { shape.Measure(kPriest,kPriest,0); }));
    }
}
