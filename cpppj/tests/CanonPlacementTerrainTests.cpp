// 후보 지형/모양 종료/지역 getter의 독립 원본 관찰과 실제 finder 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPlacementTerrain.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 세 PE별 원본 관찰 행 수와 합성 배치/후보 타입이다.
constexpr std::size_t kRows=3740,kTotal=3*kRows;
constexpr std::uint32_t kOwn=82,kOther=83;
// 기계어 관찰 파일을 읽으며 기대 출력은 계산하지 않는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_CANONTERRAIN_FIXTURE).rows;return rows;
}
// 원본 실행기의 입력 배열을 복구한다. 기대 배열은 fixture에서 그대로 읽는다.
void Seed(CanonPlacementTerrainState& state,std::uint32_t seed) {
    const std::array<std::uint8_t,6> values{static_cast<std::uint8_t>(state.emptyRegion),255,4,127,128,128};
    state.regions.fill(values.at(seed));if (seed==3) state.regions[0]=4;if (seed==5) state.regions[13]=4;
}
// 동일 입력을 C++ 모듈에 재생하여 전체 지역/raw와 각 상태를 대조한다.
void Replay(std::string_view name) {
    const auto edition=name=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;
    SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
    std::vector<std::uint16_t> islands(65536);CanonPlacementTerrainState state;RawCanonPlacementTerrain terrain(pool,types,frames,islands,state,
        {[](const CanonPlacementQuery&,Sid) { return false; },[](const CanonPlacementQuery&,const CanonPlacementCell&) { return false; },
         [](const CanonPlacementQuery&,CanonPlacementTerrainState&) { return false; }});
    CHECK(Fixture().size()==kTotal);std::size_t count=0;
    // 행마다 입력과 배열을 초기화하여 이전 순회 결과가 다음 행에 섞이지 않게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=name) continue;CHECK(row.size()==28);
        auto& own=types[kOwn];auto& other=types[kOther];own.flags1=Number(row[4]);own.flags2=Number(row[1]);own.group=std::stoi(row[2]);
        own.footX=std::stoi(row[9]);own.footY=std::stoi(row[10]);other.flags2=Number(row[5]);other.footX=std::stoi(row[11]);other.footY=std::stoi(row[12]);
        frames[kOther].frames=RiftTypeFrames({{0,0,0,0},{0,0,0,static_cast<std::uint8_t>(Number(row[6]))}});
        const auto bytes=pool.Slot(kSource);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});
        raw[10]=kOther;Put(raw,8,Number(row[15]),2);Put(raw,14,Number(row[7]));Put(raw,18,Number(row[8]));raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[13]));
        Put(raw,patch ? 36 : 34,1,patch ? 4 : 1);
        std::fill(islands.begin(),islands.end(),std::uint16_t{0});const int x=static_cast<int>(std::bit_cast<float>(Number(row[7]))),y=static_cast<int>(std::bit_cast<float>(Number(row[8])));
        if (x>=0 && x<256 && y>=0 && y<256) islands[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]=Number(row[16]) ? kSource.value : 0;
        state.emptyRegion=Number(row[18]);state.noIslandType=Number(row[14]);const CanonPlacementQuery query{kOwn,kOwn,20,21,0,1,0};terrain.Begin(query);if (row[3]!="B") terrain.BeginShape(query,{0,0,0,20,21,20,21,{}});
        Seed(state,Number(row[17]));state.groundComplete=Number(row[19])!=0;if (row[3]!="B") state.permission=true;std::uint32_t result=0;
        if (row[3]=="B") {}
        else if (row[3]=="C") terrain.Candidate(query,kSource);
        else if (row[3]=="E") CHECK(terrain.EndShape(query));
        else if (row[3]=="F") { state.bridgeOverlap=Number(row[13])!=0;state.noIslandOnly=Number(row[14])!=0;state.permission=Number(row[16])!=0;result=terrain.Finish(query) ? 1U : 0U; }
        else result=terrain.RegionAt(x,y);
        const bool same=result==Number(row[20]) && state.bridgeOverlap==(Number(row[21])!=0) && state.noIslandOnly==(Number(row[22])!=0) &&
            state.groundComplete==(Number(row[23])!=0) && state.permission==(Number(row[24])!=0) && state.canPlaceGround==(Number(row[25])!=0) && Hex(state.regions)==row[26] && Hex(pool.Slot(kSource))==row[27];
        CHECK(same);if (!same) { std::printf("Canon terrain %s 행 %zu (%s) 불일치\n",std::string(name).c_str(),count,row[3].c_str());break; }++count;
    }
    CHECK(count==kRows);
}
}
// 패치판의 실제 다리/섬/지역 getter와 모양 종료를 대조한다.
TEST_CASE(canon_terrain_patch_x86) { Replay("originals"); }
// CD판의 BYTE 프레임과 지역 getter를 별도로 대조한다.
TEST_CASE(canon_terrain_cd_x86) { Replay("originalCD"); }
// 추가 PE도 독립 원본 관찰로 대조한다.
TEST_CASE(canon_terrain_1037_x86) { Replay("original1037"); }
// 실제 받침 decoder/미리보기/finder를 통과한 아홉 모양의 지역·권한·최종 경계를 검사한다.
TEST_CASE(canon_terrain_pattern_preview_finder_and_permission_lifetime) {
    // 판본별 raw 프레임 오프셋과 실제 지역 지도 조회를 같은 장면에 적용한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[157].group=0;types[157].flags2=0x10000;types[157].flags1=2;types[157].footX=types[157].footY=1;
        types[kOther].flags2=2;types[kOther].footX=types[kOther].footY=1;
        std::vector<FrameCode> codes;
        // 실제 3×3 받침의 A..I 방향/변형 1 코드를 만든다.
        for (std::uint8_t side='A';side<='I';++side) codes.push_back({side,'P',1,0});
        frames[157].frames=RiftTypeFrames(std::move(codes));frames[kOther].frames=RiftTypeFrames(std::vector<FrameCode>(1));
        std::vector<std::uint16_t> islands(65536),surfaces(65536);std::vector<std::uint8_t> spots(65536,6);
        // 모양별로 별도의 한 칸 섬 SID를 공급한다.
        for (int x=20;x<=22;++x) {
            // 행마다 현재 raw 지역 번호와 프레임을 설정한다.
            for (int y=21;y<=23;++y) {
                const Sid sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=kOther;Put(raw,8,9,2);
                Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(x)));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(y)));
                // 표면 단계 0은 칸별 버킷이므로 이웃 섬이 같은 머리를 덮어쓰지 않는다.
                Put(raw,patch ? 36 : 34,0,patch ? 4 : 1);hash.Bucket(0,static_cast<float>(x),static_cast<float>(y))=sid.value;islands[y*256+x]=sid.value;
            }
        }
        CanonPlacementTerrainState state;state.noIslandType=kOther;CanonPlacementCollisionState collisionState;
        CanonPlacementPreviewState previewState;PriestPlacementState placementState{0,2,0};PriestPlacementGeometryState geometryState;
        geometryState.patternTypes={107,82,94,157,131,129,140,142};int grants=0,neighbors=0,finishes=0;
        const CanonPlacementQuery query{157,0,20,21,0,2,0xffffffff};
        RawCanonPlacementTerrain terrain(pool,types,frames,islands,state,{
            [&](const CanonPlacementQuery& q,Sid sid) { CHECK(q.owner==2 && q.mode==0xffffffff && q.argument==0 && pool.Slot(sid)[10]==kOther);++grants;return true; },
            [&](const CanonPlacementQuery&,const CanonPlacementCell&) { ++neighbors;return false; },
            [&](const CanonPlacementQuery& q,CanonPlacementTerrainState& value) { CHECK(q.owner==2 && q.mode==0xffffffff);++finishes;return value.permission && value.canPlaceGround; }});
        RawCanonPlacementGeometry geometry(pool,types,frames,geometryState);
        RawCanonPlacementCollision collision(pool,hash,types,collisionState,previewState,MakeCanonGeometryCollisionHooks(pool,geometry,
            [&](const CanonPlacementQuery& q) { return MakeCanonTerrainGeometryHooks(pool,terrain,q); },MakeCanonTerrainCollisionHooks(pool,terrain,{})));
        RawCanonPlacementPreview preview(pool,types,spots,surfaces,previewState,MakeCanonGeometryPreviewHooks(pool,geometry,MakeCanonPlacementCollisionHooks(pool,collision,{})));
        RawCanonPlacement placement(pool,types,placementState,MakeCanonPlacementPreviewHooks(pool,preview,{
            [](std::uint32_t,std::uint32_t,std::uint32_t) { return PriestPlacementRect{0,0,48,33}; },{}}));
        CHECK(placement.MayPlace(query));CHECK(grants==1 && neighbors==0 && finishes==1);
        CHECK(state.permission && state.groundComplete && state.noIslandOnly && state.canPlaceGround && state.regions[0]==9);
        // 지면 종류가 맞지 않으면 후보 권한 조회를 생략하고 매 모양의 주변 경계를 유지한다.
        types[157].flags1=4;grants=neighbors=finishes=0;CHECK(!placement.MayPlace(query));
        CHECK(grants==0 && neighbors==9 && finishes==1 && !state.permission && !state.groundComplete);
        // 충돌 거부는 후보 지형·모양 종료·최종 처리에 도달하지 않는다.
        types[kOther].flags2=0;grants=neighbors=finishes=0;CHECK(!placement.MayPlace(query));
        CHECK(grants==0 && neighbors==0 && finishes==0);
    }
}
// 무권한 모양에서만 주변 조회를 하며 빈 decoder는 이전 배열을 유지한 채 최종 경계를 받는다.
TEST_CASE(canon_terrain_shape_permission_and_empty_decoder) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);
    std::vector<PriestPlainCanonType> frames(188);std::vector<std::uint16_t> islands(65536);CanonPlacementTerrainState state;
    types[kOwn].group=0;int neighbors=0,finishes=0;const CanonPlacementQuery query{kOwn,25,20,21,0,3,7};
    RawCanonPlacementTerrain terrain(pool,types,frames,islands,state,{
        [](const CanonPlacementQuery&,Sid) { return false; },
        [&](const CanonPlacementQuery& q,const CanonPlacementCell& cell) { CHECK(q.argument==25 && q.owner==3 && q.mode==7 && cell.label==11);++neighbors;return true; },
        [&](const CanonPlacementQuery&,CanonPlacementTerrainState& value) { ++finishes;return value.permission; }});
    terrain.Begin(query);terrain.BeginShape(query,{0,11,0,20,21,20,21,{}});CHECK(terrain.EndShape(query));
    terrain.BeginShape(query,{0,12,0,20,21,20,21,{}});CHECK(terrain.EndShape(query));CHECK(terrain.Finish(query));
    CHECK(neighbors==1 && finishes==1 && !state.groundComplete && state.permission);
    frames[kOwn].defaultFrame=-1;PriestPlacementGeometryState geometryState;RawCanonPlacementGeometry geometry(pool,types,frames,geometryState);
    state.regions.fill(33);types[kOwn].group=10;
    CHECK(geometry.Inspect({kOwn,25,0,20,21,false},[](const CanonPlacementCell&) { CHECK(false);return false; },MakeCanonTerrainGeometryHooks(pool,terrain,query)));
    CHECK(state.regions[0]==33 && state.groundComplete && state.permission && neighbors==1 && finishes==2);
}
// 필수 경계·자료·풀과 순회 계약을 검사하고 0 발자국/bridge 우선 지면 누적을 구분한다.
TEST_CASE(canon_terrain_required_boundaries_and_guards) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),otherPool(pool.Edition(),kCapacity,true);
    std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> frames(188);std::vector<std::uint16_t> islands(65536);CanonPlacementTerrainState state;
    CanonPlacementTerrainHooks hooks{[](const CanonPlacementQuery&,Sid) { return false; },[](const CanonPlacementQuery&,const CanonPlacementCell&) { return false; },
        [](const CanonPlacementQuery&,CanonPlacementTerrainState&) { return false; }};
    CHECK(Throws([&] { RawCanonPlacementTerrain missing(pool,types,frames,islands,state,{}); }));
    RawCanonPlacementTerrain terrain(pool,types,frames,islands,state,hooks);const CanonPlacementQuery query{kOwn,0,20,21,0,1,0};
    CHECK(Throws([&] { terrain.EndShape(query); }));terrain.Begin(query);terrain.BeginShape(query,{0,0,0,20,21,20,21,{}});
    CHECK(terrain.EndShape(query));CHECK(!terrain.Finish(query) && !state.groundComplete);
    types[kOwn].flags2=4;terrain.Begin(query);state.groundComplete=false;CHECK(!terrain.Finish(query) && state.canPlaceGround && !state.permission);
    CHECK(Throws([&] { MakeCanonTerrainGeometryHooks(otherPool,terrain,query); }));CHECK(Throws([&] { MakeCanonTerrainCollisionHooks(otherPool,terrain,{}); }));
    types[kOwn].footX=13;CHECK(Throws([&] { terrain.Begin(query); }));types[kOwn].footX=0;terrain.Begin(query);
    CHECK(Throws([&] { terrain.BeginShape(query,{0,0,0,0,0,std::numeric_limits<float>::infinity(),21,{}}); }));
    CHECK(Throws([&] { terrain.Begin({188,0,20,21,0,1,0}); }));
}
