// 세 실제 PE의 비패턴 decoder/범위/충돌 인자를 복원 코드와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPlacementGeometry.h"
#include "o/SquidFactory.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 새 독립 관찰의 전체 행 수와 실제 사제 타입 번호다.
constexpr std::size_t kTotal=4352;
constexpr std::uint32_t kPriest=158;
// 원본 관찰을 읽기만 한다. 기대 frame/범위는 구현으로 만들지 않는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_PRIESTGEOMETRY_FIXTURE).rows;return rows;
}
// 네 정수를 원본 관찰의 순서로 직렬화한다.
std::string Area(std::array<int,4> values) {
    return std::to_string(values[0])+':'+std::to_string(values[1])+':'+std::to_string(values[2])+':'+std::to_string(values[3]);
}
// finder 사각형도 같은 형식으로 비교한다.
std::string Area(SquidSearchArea area) { return Area(std::array<int,4>{area.left,area.top,area.right,area.bottom}); }
// 생성·첫 범위·모양별 finder 인자·다음 칸/끝 범위를 대조한다.
void Replay(std::string_view name) {
    const bool patch=name=="originals";const auto edition=patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;CHECK(Fixture().size()==kTotal);std::size_t count=0;
    // 각 행에는 독립적인 프레임 메타/좌표/발자국 입력만 공급한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=name) continue;CHECK(row.size()==22);
        const auto direction=std::bit_cast<std::int32_t>(Number(row[3]));const float x=std::bit_cast<float>(Number(row[4])),y=std::bit_cast<float>(Number(row[5]));
        const int footX=std::stoi(row[9]),footY=std::stoi(row[10]);const RiftTypeFrames frames(std::vector<FrameCode>(Number(row[7])));
        CanonDecoder decoder(frames,std::stoi(row[8]),std::stoi(row[2]),direction,x,y,Number(row[6])!=0,edition);
        const bool first=decoder.Frame()==std::stoi(row[11]) && decoder.Valid()==(Number(row[12])!=0) &&
            std::bit_cast<std::uint32_t>(decoder.X())==Number(row[13]) && std::bit_cast<std::uint32_t>(decoder.Y())==Number(row[14]) && Area(decoder.Bounds(footX,footY))==row[15];
        const std::string area=decoder.Valid() ? Area(RawPriestPlacementGeometry::CollisionArea(decoder.X(),decoder.Y(),footX,footY)) : "none";
        CHECK(first && area==row[16]);decoder.Advance();
        const bool last=decoder.Frame()==std::stoi(row[17]) && decoder.Valid()==(Number(row[18])!=0) &&
            std::bit_cast<std::uint32_t>(decoder.X())==Number(row[19]) && std::bit_cast<std::uint32_t>(decoder.Y())==Number(row[20]) && Area(decoder.Bounds(footX,footY))==row[21];CHECK(last);
        if (!first || !last || area!=row[16]) { std::printf("Priest geometry %s 행 %zu: 기대 %s / 실제 %s\n",std::string(name).c_str(),count,row[16].c_str(),area.c_str());break; }++count;
    }
    CHECK(count==(patch ? 2304U : 1024U));
}
// 지역 몸체를 명시적으로 허용하는 입력 경계다. 실제 지형 구현으로 해석하지 않는다.
PriestPlacementGeometryHooks AllowRegions() {
    return {[](const PriestPlacementQuery&) {},[](const PriestPlacementQuery&,int,float,float) { return true; },[](const PriestPlacementQuery&) { return true; }};
}
}
// 10.78의 홀수 회전/부호 DWORD와 실제 생성/진행/범위를 대조한다.
TEST_CASE(priest_geometry_patch_x86) { Replay("originals"); }
// CD의 짝수 회전과 실제 비패턴/좌표/발자국 계산을 대조한다.
TEST_CASE(priest_geometry_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 관찰도 별도로 대조한다.
TEST_CASE(priest_geometry_1037_x86) { Replay("original1037"); }
// 기본 프레임 누락·경계 거부·현재 발자국 변화와 값 인자 보존을 검사한다.
TEST_CASE(priest_geometry_region_order_current_footprint_and_empty_frame) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> frames(188);PriestPlacementGeometryState state;
    types[kPriest].flags2=0x210000;types[kPriest].footX=types[kPriest].footY=1;frames[kPriest].defaultFrame=4;frames[kPriest].frames=RiftTypeFrames(std::vector<FrameCode>(5));
    PriestPlacementQuery query{kPriest,20.5f,21.5f,0,1,0};std::vector<std::string> events;
    RawPriestPlacementGeometry geometry(pool,types,frames,state,{
        [&](const PriestPlacementQuery& request) { CHECK(request.x==20.5f);events.push_back("begin");query.x=99;types[kPriest].footX=3;types[kPriest].footY=2;frames[kPriest].defaultFrame=-1; },
        [&](const PriestPlacementQuery& request,int frame,float x,float y) { CHECK(request.x==20.5f && frame==4 && x==20 && y==21);events.push_back("end");return true; },
        [&](const PriestPlacementQuery& request) { CHECK(request.x==20.5f);events.push_back("finish");return true; }});
    CHECK(geometry.Inspect(query,[&](SquidSearchArea area) { events.push_back("scan");CHECK(Area(area)=="18:20:21:22");return true; }));
    CHECK(events==std::vector<std::string>({"begin","scan","end","finish"}));events.clear();query.x=20.5f;
    CHECK(Area(geometry.Bounds(query))=="17:19:20:21");CHECK(geometry.Inspect(query,[](SquidSearchArea) { CHECK(false);return true; }));CHECK(events==std::vector<std::string>({"begin","finish"}));
    frames[kPriest].defaultFrame=4;query.x=20.5f;events.clear();CHECK(!geometry.Inspect(query,[](SquidSearchArea) { return false; }));CHECK(events==std::vector<std::string>{"begin"});
}
// 실제 비패턴 모양/미리보기 범위→finder→다음 나선 위치→사제 생성자를 연결한다.
TEST_CASE(priest_geometry_preview_collision_spawn_factory) {
    // 초기 픽셀 모양/SHP·지형·일반 Pop/가상 Carrier 몸체만 입력 경계다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[kPriest].flags2=0x210000;types[kPriest].constructorAddress=TypeConstructorAddress(edition,kPriest);types[kPriest].footX=types[kPriest].footY=1;types[83].footX=types[83].footY=1;
        frames[kPriest].frames=RiftTypeFrames(std::vector<FrameCode>(1));const Sid blocker=pool.Allocate();auto raw=pool.AllocatedBytes(blocker);raw[10]=83;Put(raw,14,std::bit_cast<std::uint32_t>(21.0f));Put(raw,18,std::bit_cast<std::uint32_t>(22.0f));hash.Bucket(1,21,22)=blocker.value;
        std::vector<std::uint8_t> spots(65536,6);std::vector<std::uint16_t> surfaces(65536);PriestPlacementGeometryState geometryState;PriestPlacementPreviewState previewState;PriestPlacementCollisionState collisionState{0,0x200000};PriestPlacementState placementState{0,1,0xffffffff};
        int begins=0,ends=0,finishes=0,pops=0;Sid born{};
        RawPriestPlacementGeometry geometry(pool,types,frames,geometryState,{
            [&](const PriestPlacementQuery&) { ++begins; },[&](const PriestPlacementQuery&,int frame,float x,float y) { CHECK(frame==0 && x==21 && y==21);++ends;return true; },
            [&](const PriestPlacementQuery&) { ++finishes;return true; }});
        RawPriestPlacementCollision collision(pool,hash,types,collisionState,previewState,MakePriestGeometryCollisionHooks(pool,geometry,{{},[](const PriestPlacementQuery&,Sid) {}}));
        RawPriestPlacementPreview preview(pool,types,spots,surfaces,previewState,MakePriestGeometryPreviewHooks(pool,geometry,MakePriestPlacementCollisionHooks(pool,collision,{})));
        RawPriestPlacement placement(pool,types,placementState,MakePriestPlacementPreviewHooks(pool,preview,{
            [](std::uint32_t,std::uint32_t,std::uint32_t) { return PriestPlacementRect{0,0,16,11}; },{}}));
        SquidFactory factory(pool,types);PriestSpawnState spawnState;
        RawPriestSpawn spawn(pool,types,spots,spawnState,MakePriestPlacementHooks(pool,placement,{
            {},[&](std::uint32_t type,std::uint32_t flags) { born=factory.Create(type,flags);return born; },[](Sid,std::uint32_t) {},
            [&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(sid==born && x==21 && y==21 && flags==0);++pops; },[](Sid) { CHECK(false); },[&](Sid sid) { CHECK(sid==born); }}));
        spawn.Spawn(20.75f,21.9f,kPriest,0x8101);
        CHECK(begins==2 && ends==1 && finishes==1 && pops==1 && born.value==(patch ? 15001 : 6001));
        CHECK(Get(pool.Slot(born),12,2)==0x8101 && std::count(previewState.blocked.begin(),previewState.blocked.end(),std::uint8_t{1})==16);
    }
}
// CD 홀수 회전/명시 프레임 assert와 미지원 패턴/자료/풀 연결을 별도 C++ 진단으로 검사한다.
TEST_CASE(priest_geometry_guards) {
    const RiftTypeFrames frames(std::vector<FrameCode>(1));CHECK(Throws([&] { CanonDecoder bad(frames,0,1,0,20,21,true); }));
    CHECK(Throws([&] { CanonDecoder odd(frames,0,0,1,20,21,false,OriginalEdition::Cd1072); }));
    CHECK(Throws([&] { CanonDecoder bad(frames,0,0,8,20,21); }));CHECK(Throws([&] { RawPriestPlacementGeometry::CollisionArea(20,21,0,1); }));
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(pool.Edition(),kCapacity,true);std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> metadata(188);PriestPlacementGeometryState state;
    CHECK(Throws([&] { RawPriestPlacementGeometry missing(pool,types,metadata,state,{}); }));
    RawPriestPlacementGeometry geometry(pool,types,metadata,state,AllowRegions());PriestPlacementQuery query{kPriest,20,21,0,1,0};CHECK(Throws([&] { geometry.Bounds(query); }));
    types[kPriest].flags2=0x200000;state.patternTypes[4]=kPriest;CHECK(Throws([&] { geometry.Bounds(query); }));state.patternTypes[4]=0;
    query.x=std::numeric_limits<float>::infinity();CHECK(Throws([&] { geometry.Bounds(query); }));
    CHECK(Throws([&] { MakePriestGeometryPreviewHooks(other,geometry,{}); }));CHECK(Throws([&] { MakePriestGeometryCollisionHooks(other,geometry,{}); }));
}
