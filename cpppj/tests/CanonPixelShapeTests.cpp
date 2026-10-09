// 실제 PE의 전체 패턴 픽셀 getter 관찰과 현재 SHP/기준점 변경을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPixelShape.h"

namespace {
using namespace netstorm::test::rawscene;
// 실제 타입 전역의 패턴 번호와 마지막 비패턴 사제 번호다.
constexpr std::array<std::uint32_t,9> kTypes{107,82,94,157,131,129,140,142,158};
// 각 독립 입력의 프레임 코드만 구성하며 기대 범위는 계산하지 않는다.
RiftTypeFrames InputFrames(int profile) {
    std::vector<FrameCode> codes;
    // 전체 방향/번호는 실제 FindFrame의 정상 입력 영역이다.
    for (int side='A';side<='P';++side)
        // 원본 셀의 한 자리 번호와 번호 10을 함께 검사한다.
        for (int number=0;number<=10;++number) if (profile!=2) codes.push_back({static_cast<std::uint8_t>(side),'P',static_cast<std::uint8_t>(number),0});
    if (profile==1) std::reverse(codes.begin(),codes.end());
    if (profile!=2) codes.push_back(codes.front());
    return RiftTypeFrames(std::move(codes));
}
// signed WORD의 비트 저장 폭을 유지하며 큰 입력은 정의된 방식으로 감싼다.
std::int16_t Short(int value) { return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value)); }
// 입력 헤더는 선택/전체 순회/동률이 드러나도록 물리 번호마다 다르다.
SquidDisplayFrame Header(int frame,int metric) {
    if (metric==0) return {Short(24+frame%7*3),Short(19+frame%5*2),Short(12+frame%11),Short(7-frame%13),0,0};
    if (metric==1) return {Short(32767+frame*37),Short(-32768+frame*29),Short(32767-frame*31),Short(-32768+frame*17),0,0};
    return {};
}
// 출력 float 여섯 개의 비트를 원본 순서로 보존한다.
std::array<std::uint32_t,6> Bits(const PriestPlacementPixelShape& shape) {
    return {std::bit_cast<std::uint32_t>(shape.bounds.left),std::bit_cast<std::uint32_t>(shape.bounds.top),
        std::bit_cast<std::uint32_t>(shape.bounds.right),std::bit_cast<std::uint32_t>(shape.bounds.bottom),
        std::bit_cast<std::uint32_t>(shape.anchorX),std::bit_cast<std::uint32_t>(shape.anchorY)};
}
// C++ 결과는 독립 기계어 출력에만 비교한다. 원본 getter 결과를 재계산하지 않는다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,true);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
    std::vector<SquidDisplayShape> shapes(types.size());std::vector<PriestTypeHotspot> hotspots(types.size());
    PriestPlacementGeometryState geometry;PriestPlacementShapeState state;RawCanonPixelShape shape(pool,types,frames,shapes,hotspots,geometry,state);
    const auto rows=LoadFixture(NETSTORM_CANONSHAPE_FIXTURE).rows;std::size_t count=0;
    // 각 행은 모든 타입별 패턴 선택과 현재 메타 자료를 독립적으로 구성한다.
    for (const auto& row:rows) {
        if (row[0]!=edition) continue;CHECK(row.size()==19);const auto type=kTypes[Number(row[1])];
        std::copy_n(kTypes.begin(),8,geometry.patternTypes.begin());
        if (row[6]=="1") geometry.patternTypes[0]=geometry.patternTypes[1];
        if (row[6]=="2") geometry.patternTypes[2]=geometry.patternTypes[4];
        frames[type].frames=InputFrames(std::stoi(row[4]));frames[type].defaultFrame=std::stoi(row[7]);
        hotspots[type]={std::stoi(row[11]),std::stoi(row[12])};state={std::bit_cast<float>(Number(row[9])),std::bit_cast<float>(Number(row[10]))};
        auto& shp=shapes[type];shp.loaded=true;shp.frameCount=static_cast<int>(frames[type].frames.Codes().size());shp.frames.clear();
        // 비연속 원본 SHP 주소 표와 같은 물리 순서의 헤더 입력이다.
        for (int frame=0;frame<shp.frameCount;++frame) shp.frames.push_back(Header(frame,std::stoi(row[8])));
        std::array<std::uint32_t,6> expected{};
        // 기대값은 여섯 실제 x86 관찰 비트다.
        for (std::size_t i=0;i<expected.size();++i) expected[i]=Number(row[13+i]);
        const auto actual=Bits(shape.Measure(type,static_cast<std::uint32_t>(std::stoi(row[2])),static_cast<std::uint32_t>(std::stoi(row[3])),row[5]!="0"));CHECK(actual==expected);
        if (actual!=expected) { std::printf("Canon pixel %s 행 %zu kind=%u arg=%s dir=%s metric=%s\n",std::string(edition).c_str(),count,Number(row[1]),row[2].c_str(),row[3].c_str(),row[8].c_str());break; }
        ++count;
    }
    CHECK(count==(patch ? 3150U : 1400U));
}
}
// 10.78의 모든 패턴/회전/별칭/최소·최대·기준점 관찰을 대조한다.
TEST_CASE(canon_pixel_patch_x86) { Replay("originals"); }
// CD의 직접 SHP 조회와 실수 중간 저장/엄격 최댓값 선택을 대조한다.
TEST_CASE(canon_pixel_cd_x86) { Replay("originalCD"); }
// 별도 10.37 PE도 자체 기계어 기대값을 읽는다.
TEST_CASE(canon_pixel_1037_x86) { Replay("original1037"); }
// 일반 자산에는 사제 플래그가 필요 없고 현재 SHP/배율을 매 호출 다시 읽는다.
TEST_CASE(canon_pixel_general_asset_and_current_metadata) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);
    std::vector<PriestPlainCanonType> frames(188);std::vector<SquidDisplayShape> shapes(188);std::vector<PriestTypeHotspot> hotspots(188);
    PriestPlacementGeometryState geometry;PriestPlacementShapeState state;RawCanonPixelShape shape(pool,types,frames,shapes,hotspots,geometry,state);
    // 사제/패턴에 해당하지 않는 내장 번호로 일반 getter 계약을 검사한다.
    constexpr std::uint32_t type=70;
    frames[type].frames=InputFrames(0);shapes[type]={177,true,std::vector<SquidDisplayFrame>(177,{16,11,8,5,0,0})};
    CHECK(shape.Measure(type,type,0).bounds.left==-8);hotspots[type].x=8;CHECK(shape.Measure(type,type,0).bounds.left==0);
    shapes[type].frames[0].width=32;CHECK(shape.Measure(type,type,0).bounds.right==32);
    frames[type].defaultFrame=-1;shapes[type].loaded=false;state={0.5f,1.25f};const auto empty=shape.Measure(type,type,0);
    CHECK(empty.bounds.left==1000 && empty.bounds.right==0 && empty.anchorX==0.5f && empty.anchorY==1.25f);
    CHECK(Throws([&] { shape.Measure(188,0,0); }));CHECK(Throws([&] { RawCanonPixelShape bad(pool,types,frames,{},hotspots,geometry,state); }));
}
