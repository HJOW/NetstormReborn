// 후보 지형/모양 종료/지역 getter의 독립 원본 관찰과 실제 finder 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPlacementTerrain.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 세 PE별 원본 관찰 행 수와 합성 배치/후보 타입이다.
constexpr std::size_t kRows=2240,kTotal=3*kRows;
constexpr std::uint32_t kOwn=82,kOther=83;
// 기계어 관찰 파일을 읽으며 기대 출력은 계산하지 않는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_PRIESTTERRAIN_FIXTURE).rows;return rows;
}
// 원본 실행기의 입력 배열을 복구한다. 기대 배열은 fixture에서 그대로 읽는다.
void Seed(PriestPlacementTerrainState& state,std::uint32_t seed) {
    const std::array<std::uint8_t,6> values{static_cast<std::uint8_t>(state.emptyRegion),255,4,127,128,128};
    state.regions.fill(values.at(seed));if (seed==3) state.regions[0]=4;if (seed==5) state.regions[13]=4;
}
// 동일 입력을 C++ 모듈에 재생하여 전체 지역/raw와 각 상태를 대조한다.
void Replay(std::string_view name) {
    const auto edition=name=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;
    SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
    std::vector<std::uint16_t> islands(65536);PriestPlacementTerrainState state;RawPriestPlacementTerrain terrain(pool,types,frames,islands,state);
    CHECK(Fixture().size()==kTotal);std::size_t count=0;
    // 행마다 입력과 배열을 초기화하여 이전 순회 결과가 다음 행에 섞이지 않게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=name) continue;CHECK(row.size()==26);
        auto& own=types[kOwn];auto& other=types[kOther];own.flags1=Number(row[2]);own.flags2=0x210000;
        own.footX=std::stoi(row[7]);own.footY=std::stoi(row[8]);other.flags2=Number(row[3]);other.footX=std::stoi(row[9]);other.footY=std::stoi(row[10]);
        frames[kOther].frames=RiftTypeFrames({{0,0,0,0},{0,0,0,static_cast<std::uint8_t>(Number(row[4]))}});
        const auto bytes=pool.Slot(kSource);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});
        raw[10]=kOther;Put(raw,8,Number(row[13]),2);Put(raw,14,Number(row[5]));Put(raw,18,Number(row[6]));raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[11]));
        Put(raw,patch ? 36 : 34,1,patch ? 4 : 1);
        std::fill(islands.begin(),islands.end(),std::uint16_t{0});const int x=static_cast<int>(std::bit_cast<float>(Number(row[5]))),y=static_cast<int>(std::bit_cast<float>(Number(row[6])));
        if (x>=0 && x<256 && y>=0 && y<256) islands[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]=Number(row[14]) ? kSource.value : 0;
        state.emptyRegion=Number(row[16]);state.noIslandType=Number(row[12]);const PriestPlacementQuery query{kOwn,20,21,0,1,0};terrain.Begin(query);terrain.BeginShape(query,20,21);
        Seed(state,Number(row[15]));state.groundComplete=Number(row[17])!=0;std::uint32_t result=0;
        if (row[1]=="C") terrain.Candidate(query,kSource);
        else if (row[1]=="E") CHECK(terrain.EndShape(query));
        else if (row[1]=="F") { state.bridgeOverlap=Number(row[11])!=0;state.noIslandOnly=Number(row[12])!=0;state.permission=Number(row[14])!=0;result=terrain.Finish(query) ? 1U : 0U; }
        else result=terrain.RegionAt(x,y);
        const bool same=result==Number(row[18]) && state.bridgeOverlap==(Number(row[19])!=0) && state.noIslandOnly==(Number(row[20])!=0) &&
            state.groundComplete==(Number(row[21])!=0) && state.permission==(Number(row[22])!=0) && state.canPlaceGround==(Number(row[23])!=0) && Hex(state.regions)==row[24] && Hex(pool.Slot(kSource))==row[25];
        CHECK(same);if (!same) { std::printf("Priest terrain %s 행 %zu (%s) 불일치\n",std::string(name).c_str(),count,row[1].c_str());break; }++count;
    }
    CHECK(count==kRows);
}
}
// 패치판의 실제 다리/섬/지역 getter와 모양 종료를 대조한다.
TEST_CASE(priest_terrain_patch_x86) { Replay("originals"); }
// CD판의 BYTE 프레임과 지역 getter를 별도로 대조한다.
TEST_CASE(priest_terrain_cd_x86) { Replay("originalCD"); }
// 추가 PE도 독립 원본 관찰로 대조한다.
TEST_CASE(priest_terrain_1037_x86) { Replay("original1037"); }
// 실제 finder가 무시한 섬/다리도 지형 효과를 받으며 사제의 최종 지면 우회를 보존한다.
TEST_CASE(priest_terrain_real_geometry_finder_and_final_priest_bypass) {
    // 판본별 슬롯 오프셋을 사용하여 같은 장면을 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());std::vector<std::uint16_t> islands(65536);
        types[kOwn].flags2=0x210000;types[kOwn].flags1=2;types[kOwn].footX=types[kOwn].footY=1;types[kOther].flags2=2;types[kOther].footX=types[kOther].footY=1;
        frames[kOwn].frames=RiftTypeFrames(std::vector<FrameCode>(1));frames[kOther].frames=RiftTypeFrames(std::vector<FrameCode>(1));
        const Sid island=pool.Allocate();auto raw=pool.AllocatedBytes(island);raw[10]=kOther;Put(raw,8,9,2);Put(raw,14,std::bit_cast<std::uint32_t>(20.0f));Put(raw,18,std::bit_cast<std::uint32_t>(21.0f));
        Put(raw,patch ? 36 : 34,0,patch ? 4 : 1);hash.Bucket(1,20,21)=island.value;islands[21*256+20]=island.value;
        PriestPlacementTerrainState terrainState;PriestPlacementGeometryState geometryState;PriestPlacementCollisionState collisionState;PriestPlacementPreviewState previewState;
        RawPriestPlacementTerrain terrain(pool,types,frames,islands,terrainState);
        RawPriestPlacementGeometry geometry(pool,types,frames,geometryState,MakePriestTerrainGeometryHooks(pool,terrain));
        RawPriestPlacementCollision collision(pool,hash,types,collisionState,previewState,MakePriestGeometryCollisionHooks(pool,geometry,MakePriestTerrainCollisionHooks(pool,terrain,{})));
        PriestPlacementQuery query{kOwn,20,21,0,1,0};CHECK(collision.Inspect(query,true));CHECK(terrainState.regions[0]==9 && terrainState.groundComplete && !terrainState.noIslandOnly && terrainState.canPlaceGround);
        // 허용 지면이 없어도 충돌 무시와 최종 사제 우회는 유지한다. flags1 0x400은 permission 효과를 가진다.
        types[kOwn].flags1=0x400;CHECK(collision.Inspect(query,true));CHECK(!terrainState.groundComplete && !terrainState.permission && !terrainState.canPlaceGround && terrainState.regions[0]==127);
        types[kOther].flags2=4;CHECK(collision.Inspect(query,true));CHECK(terrainState.bridgeOverlap && terrainState.noIslandOnly && terrainState.canPlaceGround);
        raw[patch ? 40 : 35]=1;CHECK(collision.Inspect(query,true));CHECK(!terrainState.bridgeOverlap);
        // 유효 모양이 없으면 배열을 초기화하지 않고 초기 groundComplete=true로 최종 사제 처리를 한다.
        frames[kOwn].defaultFrame=-1;terrainState.regions.fill(33);CHECK(collision.Inspect(query,false));CHECK(terrainState.regions[0]==33 && terrainState.groundComplete);
    }
}
// 잘못된 순회/자료/타입/프레임/풀 연결과 실제 bridge assert 조건을 진단한다.
TEST_CASE(priest_terrain_guards_and_sticky_shape_state) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),otherPool(pool.Edition(),kCapacity,true);std::vector<RiftTypeRecord> types(188);
    std::vector<PriestPlainCanonType> frames(188);std::vector<std::uint16_t> islands(65536);PriestPlacementTerrainState state;
    CHECK(Throws([&] { RawPriestPlacementTerrain bad(pool,types,frames,{},state); }));RawPriestPlacementTerrain terrain(pool,types,frames,islands,state);
    PriestPlacementQuery query{kOwn,20,21,0,1,0};CHECK(Throws([&] { terrain.Begin(query); }));types[kOwn].flags2=0x210000;types[kOwn].footX=types[kOwn].footY=1;
    CHECK(Throws([&] { terrain.Candidate(query,kSource); }));terrain.Begin(query);CHECK(Throws([&] { terrain.EndShape(query); }));
    CHECK(Throws([&] { terrain.BeginShape(query,std::numeric_limits<float>::infinity(),21); }));terrain.BeginShape(query,20,21);CHECK(terrain.EndShape(query));CHECK(!state.groundComplete);
    terrain.BeginShape(query,20,21);state.regions.fill(9);CHECK(terrain.EndShape(query));CHECK(!state.groundComplete && terrain.Finish(query));
    CHECK(Throws([&] { MakePriestTerrainGeometryHooks(otherPool,terrain); }));CHECK(Throws([&] { MakePriestTerrainCollisionHooks(otherPool,terrain,{}); }));
    types[kOwn].flags2|=0x200;CHECK(Throws([&] { terrain.Begin(query); }));types[kOwn].flags2=0x210000;
    const Sid sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=kOther;types[kOther].flags2=4;types[kOwn].footY=2;
    terrain.Begin(query);terrain.BeginShape(query,20,21);CHECK(Throws([&] { terrain.Candidate(query,sid); }));
    types[kOwn].footY=1;types[kOther].flags2=2;terrain.Begin(query);terrain.BeginShape(query,20,21);CHECK(Throws([&] { terrain.Candidate(query,sid); }));
    islands[0]=sid.value;types[kOther].flags2=0;CHECK(Throws([&] { terrain.RegionAt(0,0); }));
}
