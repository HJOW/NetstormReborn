// 최종 표면 소유 관계/특수 지역을 세 PE의 독립 관찰과 실제 배치 파이프라인으로 검증한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPlacementRelations.h"
#include "o/RawCanonPlacementSurrounding.h"
#include "o/RawCanonPixelShape.h"
#include "o/RawPlayerPlacementAnchor.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 각 PE의 최종 누적/관계/좌표 입력 수와 원본 checksum 대상 슬롯 수다.
constexpr std::size_t kRows=2307,kSlots=128,kRegions=128;
// 기존 도구/PE 없이 원본 최종 구간의 관찰 행을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_CANONRELATIONS_FIXTURE).rows;return rows;
}
// 실제 최종 반환/거부값/지면·권한·표시 차단과 자료 불변성을 대조한다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
    std::vector<std::uint16_t> surfaces(65536),islands(65536);std::vector<CanonPlacementIslandRegion> regions(kRegions);
    std::array<std::uint8_t,131072> mapBytes{};std::array<std::uint8_t,324> relationBytes{};
    auto bytes=pool.Bytes().first(kSlots*pool.Layout().stride);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
    CanonPlacementPermissionState permissionState;RawCanonPlacementPermission permission(pool,permissionState,[](std::uint32_t,std::uint32_t,float,float) { CHECK(false);return 0U; });
    CanonPlacementRelationsState state;state.islandCount=kRegions;PriestPlacementState placement;
    RawCanonPlacementRelations relations(pool,types,surfaces,regions,state,placement,permission);std::size_t count=0;
    CHECK(Fixture().size()==3*kRows);
    // 같은 raw·지도·관계 표·이전 누적 상태를 각 독립 관찰 행에 공급한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==31);std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});
        raw[50*pool.Layout().stride+(patch ? 34 : 32)]=static_cast<std::uint8_t>(Number(row[8]));
        raw[51*pool.Layout().stride+(patch ? 34 : 32)]=static_cast<std::uint8_t>((Number(row[8])+1)%3);
        const auto surface=static_cast<std::uint16_t>(Number(row[13]));std::fill(surfaces.begin(),surfaces.end(),surface);
        surfaces[21*256+20]=surface;surfaces[21*256+21]=surface==50 ? 51 : surface;
        surfaces[22*256+20]=surface==50 ? 51 : surface;surfaces[22*256+21]=surface;
        types[82].flags2=Number(row[1]);types[82].flags1=Number(row[2]);types[82].footX=types[82].footY=1;
        permissionState.editor=Number(row[10]);permissionState.useAlliances=Number(row[11]);permissionState.alliances.fill(0);
        // 실제 한 방향 칸만 채워 전치 표와 보정 후 다른 표면의 소유자를 구별한다.
        const auto owner=std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Number(row[9])))));
        const std::uint32_t index=Number(row[8])*9U+owner;if (index<permissionState.alliances.size()) permissionState.alliances[index]=Number(row[12]);
        state.bypassGroundPermission=Number(row[7]);placement.blockedRelation=Number(row[19]);
        // 실제 특수 지역 레코드의 제한/존재 값만 명시적으로 입력한다.
        for (auto& region:regions) region={Number(row[18]),1};
        const CanonPlacementQuery query{82,0,std::bit_cast<float>(Number(row[14])),std::bit_cast<float>(Number(row[15])),0,Number(row[9]),0};
        CanonPlacementTerrainState terrain;terrain.groundComplete=Number(row[3])!=0;terrain.noIslandOnly=Number(row[4])!=0;
        terrain.bridgeOverlap=Number(row[5])!=0;terrain.permission=Number(row[6])!=0;terrain.emptyRegion=Number(row[17]);terrain.regions.fill(static_cast<std::uint8_t>(Number(row[16])));
        CanonPlacementRelationResult observed;
        // 기존 지면 누적을 실제 Finish로 수행한 뒤 원본 최종 지역 변수 입력을 관계 모듈에 전달한다.
        RawCanonPlacementTerrain pipeline(pool,types,frames,islands,terrain,{{[](const CanonPlacementQuery&,Sid) { CHECK(false);return false; }},
            [](const CanonPlacementQuery&,const CanonPlacementCell&) { CHECK(false);return false; },
            [&](const CanonPlacementQuery& q,CanonPlacementTerrainState& current) { observed=relations.Inspect(q,current,Number(row[20]));return observed.allowed; }});
        pipeline.Begin(query);
        // Begin의 초기값 대신 실제 앞부분 누적 상태를 회복하고 빈 모양의 Finish만 호출한다.
        terrain.groundComplete=Number(row[3])!=0;terrain.noIslandOnly=Number(row[4])!=0;terrain.bridgeOverlap=Number(row[5])!=0;terrain.permission=Number(row[6])!=0;
        const bool allowed=pipeline.Finish(query);
        // 물리 지도/관계 표도 원본 checksum과 대조하여 입력 자료의 뜻하지 않은 변경을 찾는다.
        for (std::size_t i=0;i<surfaces.size();++i) Put(mapBytes,2*i,surfaces[i],2);
        // 직접 DWORD 관계 표의 행 방향과 비트값을 유지한다.
        for (std::size_t i=0;i<permissionState.alliances.size();++i) Put(relationBytes,4*i,permissionState.alliances[i]);
        const bool same=allowed==(Number(row[21])!=0) && terrain.permission==(Number(row[22])!=0) && terrain.canPlaceGround==(Number(row[23])!=0) &&
            placement.blockedRelation==Number(row[24]) && observed.relationRejected==(Number(row[25])!=0) && observed.foreignRejected==Number(row[26]) && observed.priorRejection==Number(row[27]) &&
            Adler(raw)==Number(row[28]) && Adler(mapBytes)==Number(row[29]) && Adler(relationBytes)==Number(row[30]);
        CHECK(same);if (!same) { std::printf("최종 관계 %s 행 %zu 불일치\n",std::string(edition).c_str(),count);break; }++count;
    }
    CHECK(count==kRows);
}
// 원본 픽셀 getter부터 일반/패턴 decoder·finder·Player·주변/지형·최종 관계를 모두 실제 구현으로 연결한다.
void Pipeline(bool pattern) {
    // 두 판본의 서로 다른 슬롯/원본 특수 지역 분기를 같은 장면으로 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        std::vector<SquidDisplayShape> shapes(types.size());std::vector<PriestTypeHotspot> hotspots(types.size());
        std::vector<std::uint16_t> islands(65536),surfaces(65536);std::vector<std::uint8_t> spots(65536,6);std::vector<CanonPlacementIslandRegion> regions(kRegions,{0,1});
        const std::uint32_t placing=pattern ? 157U : 80U;types[placing].group=0;types[placing].flags1=0x802;types[placing].flags2=pattern ? 0x10000U : 0x4000U;types[placing].footX=types[placing].footY=1;
        types[83].flags1=0x800;types[83].flags2=2;types[83].footX=types[83].footY=1;types[84].flags1=0x800;types[84].flags2=6;
        std::vector<FrameCode> codes;
        // 아홉 받침 패턴의 물리 코드와 일반 자산의 한 프레임을 각각 준비한다.
        for (std::uint8_t side='A';side<=(pattern ? 'I' : 'A');++side) codes.push_back({side,'P',1,0});
        frames[placing].frames=RiftTypeFrames(std::move(codes));frames[83].frames=RiftTypeFrames(std::vector<FrameCode>{{'P','P',1,0}});
        const auto n=frames[placing].frames.Codes().size();shapes[placing]={static_cast<int>(n),true,std::vector<SquidDisplayFrame>(n,{16,11,8,5,0,0})};
        Sid source{};
        // 고정 섬 지도와 현재 표면 지도에 같은 지역/그래프의 독립 칸들을 등록한다.
        for (int x=20;x<=(pattern ? 22 : 20);++x) {
            // 패턴 칸마다 실제 해시 머리와 raw 번호/소유자/좌표를 넣는다.
            for (int y=21;y<=(pattern ? 23 : 21);++y) {
                const Sid sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=83;raw[11]=0;raw[patch ? 34 : 32]=1;raw[patch ? 30 : 28]=7;
                // 일반/특수 genus에서도 후보가 지형 누적에 도달하도록 실제 배치 허용 비트를 켠다.
                raw[patch ? 40 : 35]=1;
                Put(raw,8,9,2);Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(x)));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(y)));Put(raw,patch ? 36 : 34,0,patch ? 4 : 1);
                islands[y*256+x]=surfaces[y*256+x]=sid.value;hash.Bucket(0,static_cast<float>(x),static_cast<float>(y))=sid.value;if (x==20 && y==21) source=sid;
            }
        }
        const Sid work=pool.Allocate();auto workshop=pool.AllocatedBytes(work);workshop[10]=84;workshop[patch ? 34 : 32]=1;workshop[patch ? 30 : 28]=7;
        SquidPostPopState books;books.ownerFactories[1].entries={work.value};books.ownerFactories[1].count=1;SquidPostPopList extras;ContainedFinderState contained;PlayerPlacementAnchorState anchorState;anchorState.graphReady=1;
        RawPlayerPlacementAnchor anchor(pool,types,surfaces,books,extras,contained,anchorState);CanonPlacementPermissionState permissionState;permissionState.graphReady=1;
        RawCanonPlacementPermission permission(pool,permissionState,MakePlayerAnchorQuery(pool,anchor));PriestPlacementState placementState{0,1,0};CanonPlacementRelationsState relationState;relationState.islandCount=kRegions;
        RawCanonPlacementRelations relations(pool,types,surfaces,regions,relationState,placementState,permission);RawCanonPlacementSurrounding surrounding(pool,types,frames,islands,spots,permission);
        CanonPlacementTerrainState terrainState;terrainState.noIslandType=83;
        RawCanonPlacementTerrain terrain(pool,types,frames,islands,terrainState,MakeCanonRelationsTerrainHooks(pool,relations,
            MakeCanonSurroundingTerrainHooks(pool,surrounding,MakeCanonPermissionTerrainHooks(pool,permission,{}))));
        PriestPlacementGeometryState geometryState;geometryState.patternTypes={107,82,94,157,131,129,140,142};PriestPlacementShapeState shapeState;
        RawCanonPlacementGeometry geometry(pool,types,frames,geometryState);RawCanonPixelShape shape(pool,types,frames,shapes,hotspots,geometryState,shapeState);
        CanonPlacementCollisionState collisionState;CanonPlacementPreviewState previewState;
        RawCanonPlacementCollision collision(pool,hash,types,collisionState,previewState,MakeCanonGeometryCollisionHooks(pool,geometry,
            [&](const CanonPlacementQuery& q) { return MakeCanonTerrainGeometryHooks(pool,terrain,q); },MakeCanonTerrainCollisionHooks(pool,terrain,{})));
        RawCanonPlacementPreview preview(pool,types,spots,surfaces,previewState,MakeCanonGeometryPreviewHooks(pool,geometry,MakeCanonPlacementCollisionHooks(pool,collision,{})));
        RawCanonPlacement placement(pool,types,placementState,MakeCanonShapePlacementHooks(pool,shape,MakeCanonPlacementPreviewHooks(pool,preview,{})));
        const CanonPlacementQuery query{placing,0,20,21,0,1,0};
        CHECK(!placement.MayPlace(query) && terrainState.permission && terrainState.groundComplete && placementState.blockedRelation==0);
        permissionState.alliances[10]=1;CHECK(placement.MayPlace(query));permissionState.useAlliances=0;CHECK(placement.MayPlace(query));
        permissionState.graphReady=0;CHECK(!placement.MayPlace(query));permissionState.graphReady=1;
        // 현재 표면만 다른 소유자로 바꿔 고정 섬/Player 권한이 있어도 최종 관계가 거부함을 확인한다.
        const Sid foreign=pool.Allocate();auto other=pool.AllocatedBytes(foreign);other[10]=83;other[patch ? 34 : 32]=2;other[patch ? 30 : 28]=7;surfaces[21*256+20]=foreign.value;
        // 일반 0x4000은 0x4200 표시 초기화 마스크에 포함되므로 내부 거부만 남는다.
        CHECK(!placement.MayPlace(query) && placementState.blockedRelation==(pattern ? 1U : 0U));relationState.bypassGroundPermission=1;CHECK(!placement.MayPlace(query));relationState.bypassGroundPermission=0;
        permissionState.editor=1;CHECK(!placement.MayPlace(query) && placementState.blockedRelation==0);permissionState.alliances[19]=1;CHECK(placement.MayPlace(query));
        permissionState.editor=0;surfaces[21*256+20]=source.value;
        // 특수 지역은 패치에서만 최종 지면을 취소하며 CD는 같은 입력을 허용한다.
        types[placing].flags2=0x200;regions[9].restricted=1;CHECK(placement.MayPlace(query)==!patch);CHECK(terrainState.canPlaceGround==!patch);
        regions[9].restricted=0;CHECK(placement.MayPlace(query));
    }
}
}
// 패치의 실제 특수 지역/표면 helper와 최종 정상 반환을 모두 재생한다.
TEST_CASE(canon_relations_patch_x86) { Replay("originals"); }
// CD의 실제 인라인 판정/이전 거부 저장 차이를 대조한다.
TEST_CASE(canon_relations_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 모든 독립 관찰을 재생한다.
TEST_CASE(canon_relations_1037_x86) { Replay("original1037"); }
// 일반 자산의 모든 복원 단계에 실제 최종 판정을 연결한다.
TEST_CASE(canon_relations_normal_complete_pipeline) { Pipeline(false); }
// 3×3 받침의 실제 픽셀/decoder/finder/권한/지역과 원래 요청 좌표의 최종 판정을 연결한다.
TEST_CASE(canon_relations_pattern_complete_pipeline) { Pipeline(true); }
// 정상 범위 검사와 실제 읽기 전의 생략, 표시 차단/내부 거부 구별을 검사한다.
TEST_CASE(canon_relations_required_data_and_lazy_guards) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(pool.Edition(),kCapacity,true);std::vector<RiftTypeRecord> types(188);
    std::vector<std::uint16_t> surfaces(65536);std::vector<CanonPlacementIslandRegion> regions(10,{0,1});CanonPlacementRelationsState state;state.islandCount=10;
    PriestPlacementState placement;CanonPlacementPermissionState permissions;
    RawCanonPlacementPermission permission(pool,permissions,[](std::uint32_t,std::uint32_t,float,float) { CHECK(false);return 0U; });
    RawCanonPlacementRelations relations(pool,types,surfaces,regions,state,placement,permission);CanonPlacementTerrainState terrain;terrain.groundComplete=terrain.permission=terrain.canPlaceGround=true;terrain.regions.fill(9);
    CanonPlacementQuery query{82,0,20,21,0,1,0};CHECK(Throws([&] { MakeCanonRelationsTerrainHooks(other,relations,{}); }));
    CHECK(Throws([&] { RawCanonPlacementRelations bad(other,types,surfaces,regions,state,placement,permission); }));
    CHECK(Throws([&] { RawCanonPlacementRelations bad(pool,types,{},regions,state,placement,permission); }));
    types[82].flags2=0x200;regions[9].exists=0;CHECK(Throws([&] { relations.Inspect(query,terrain); }));regions[9].exists=1;
    state.islandCount=9;CHECK(Throws([&] { relations.Inspect(query,terrain); }));state.islandCount=10;
    terrain.regions.fill(255);terrain.emptyRegion=0xffffffff;CHECK(relations.Inspect(query,terrain).allowed);terrain.emptyRegion=127;CHECK(Throws([&] { relations.Inspect(query,terrain); }));
    types[82].flags2=0x200000;query.x=std::numeric_limits<float>::quiet_NaN();query.y=std::numeric_limits<float>::infinity();CHECK(relations.Inspect(query,terrain,99).allowed);
    CHECK(Throws([&] { relations.SurfaceAt(query.x,query.y); }));query.x=20;query.y=21;types[82].flags2=0x4200;terrain.regions.fill(9);
    CHECK(!relations.Inspect(query,terrain).allowed && placement.blockedRelation==0);
    types[82].flags2=0;const Sid surface=pool.Allocate();pool.AllocatedBytes(surface)[34]=255;surfaces[21*256+20]=surface.value;
    permissions.editor=1;CHECK(Throws([&] { relations.Inspect(query,terrain); }));
}
