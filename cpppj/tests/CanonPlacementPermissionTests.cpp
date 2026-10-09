// 실제 후보 권한/방향 관계의 독립 원본 반환·인자·변화와 지형 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPlacementPermission.h"

namespace {
using namespace netstorm::test::rawscene;
// 세 PE의 관찰 행과 합성 후보 타입 번호다.
constexpr std::size_t kPatchRows=5850,kCdRows=5520,kTotal=kPatchRows+2*kCdRows;
constexpr std::uint32_t kOwn=82,kOther=83;
// 원본이 기록한 기대 출력을 읽으며 C++ 판단으로 다시 계산하지 않는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_CANONPERMISSION_FIXTURE).rows;return rows;
}
// 각 원본 입력의 globals/raw/관계 표를 준비하고 반환과 외부 조회 사건을 대조한다.
void Replay(std::string_view name) {
    const bool patch=name=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
    CanonPlacementPermissionState state;std::array<std::uint32_t,4> arguments{};std::uint32_t calls=0;
    CHECK(Fixture().size()==kTotal);std::size_t count=0;
    // 이전 입력의 전역·콜백 결과·raw 바이트가 다음 입력에 남지 않게 초기화한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=name) continue;CHECK(row.size()==25);
        state.graphReady=Number(row[2]);state.editor=Number(row[3]);state.useAlliances=Number(row[4]);state.allowOtherOwners=Number(row[5]);state.alliances.fill(0);
        const auto current=Number(row[6]),owner=Number(row[7]),sid=Number(row[11]);
        const std::uint32_t forward=current*9U+owner,reverse=owner*9U+current;
        if (forward<state.alliances.size()) state.alliances[forward]=Number(row[8]);
        if (reverse<state.alliances.size() && reverse!=forward) state.alliances[reverse]=Number(row[9]);
        const auto bytes=pool.Slot(Sid{static_cast<std::uint16_t>(sid<128 ? sid : 50)});
        std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});
        raw[patch ? 34 : 32]=static_cast<std::uint8_t>(current);raw[10]=static_cast<std::uint8_t>(Number(row[12]));Put(raw,14,Number(row[13]));Put(raw,18,Number(row[14]));
        calls=0;arguments.fill(0);
        RawCanonPlacementPermission permission(pool,state,[&](std::uint32_t player,std::uint32_t type,float x,float y) {
            ++calls;arguments={player,type,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)};
            if (Number(row[15])) { raw[patch ? 34 : 32]=3;raw[10]=84;Put(raw,14,0x41f00000);Put(raw,18,0x41f80000);state.editor^=1; }
            return Number(row[10]);
        });
        const auto result=row[1]=="R" ? static_cast<std::uint32_t>(permission.Related(current,owner)) : permission.Candidate(Sid{static_cast<std::uint16_t>(sid)},owner);
        std::array<std::uint8_t,324> relationBytes{};
        // 기계어 실행기의 little endian 관계 표 checksum과 같은 바이트 순서로 직렬화한다.
        for (std::size_t i=0;i<state.alliances.size();++i) Put(relationBytes,4*i,state.alliances[i]);
        const bool same=result==Number(row[16]) && calls==Number(row[17]) && arguments[0]==Number(row[18]) && arguments[1]==Number(row[19]) &&
            arguments[2]==Number(row[20]) && arguments[3]==Number(row[21]) && Hex(raw)==row[22] && state.editor==Number(row[23]) && Adler(relationBytes)==Number(row[24]);
        CHECK(same);if (!same) { std::printf("Canon permission %s 행 %zu (%s) 불일치\n",std::string(name).c_str(),count,row[1].c_str());break; }++count;
    }
    CHECK(count==(patch ? kPatchRows : kCdRows));
}
}
// 패치 후보와 방향 관계 helper의 전체 정상 반환을 대조한다.
TEST_CASE(canon_permission_patch_x86) { Replay("originals"); }
// CD 후보 helper의 BYTE 소유자/좌표와 DWORD 반환을 대조한다.
TEST_CASE(canon_permission_cd_x86) { Replay("originalCD"); }
// 추가 PE의 독립 원본 관찰을 대조한다.
TEST_CASE(canon_permission_1037_x86) { Replay("original1037"); }
// 실제 3×3 decoder/미리보기/finder/지형에 권한 helper를 연결하고 모양 사이 누적을 검사한다.
TEST_CASE(canon_permission_pattern_terrain_integration) {
    // 판본마다 서로 다른 소유자 필드를 갖는 같은 장면을 구성한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[157].group=0;types[157].flags2=0x10000;types[157].flags1=2;types[157].footX=types[157].footY=1;
        types[kOther].flags2=2;types[kOther].footX=types[kOther].footY=1;std::vector<FrameCode> codes;
        // 실제 받침의 아홉 방향/변형 1 입력을 준비한다.
        for (std::uint8_t side='A';side<='I';++side) codes.push_back({side,'P',1,0});
        frames[157].frames=RiftTypeFrames(std::move(codes));frames[kOther].frames=RiftTypeFrames(std::vector<FrameCode>(1));
        std::vector<std::uint16_t> islands(65536),surfaces(65536);std::vector<std::uint8_t> spots(65536,6);
        // 각 열에 별도 칸을 만들어 표면 단계 0에 섬을 등록한다.
        for (int x=20;x<=22;++x) {
            // 행마다 별도 SID와 원본 소유자/지역 번호/좌표를 넣는다.
            for (int y=21;y<=23;++y) {
                const Sid sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=kOther;raw[patch ? 34 : 32]=2;Put(raw,8,9,2);
                Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(x)));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(y)));
                Put(raw,patch ? 36 : 34,0,patch ? 4 : 1);hash.Bucket(0,static_cast<float>(x),static_cast<float>(y))=sid.value;islands[y*256+x]=sid.value;
            }
        }
        CanonPlacementPermissionState permissionState;permissionState.graphReady=1;int lookups=0,neighbors=0,finishes=0;
        RawCanonPlacementPermission permission(pool,permissionState,[&](std::uint32_t player,std::uint32_t type,float x,float y) {
            CHECK(player==2 && type==kOther && x>=20 && x<=22 && y>=21 && y<=23);++lookups;return lookups==1 ? 0U : 0xffffffffU;
        });
        CanonPlacementTerrainState state;state.noIslandType=kOther;CanonPlacementCollisionState collisionState;
        CanonPlacementPreviewState previewState;PriestPlacementState placementState{0,2,0};PriestPlacementGeometryState geometryState;
        geometryState.patternTypes={107,82,94,157,131,129,140,142};const CanonPlacementQuery query{157,0,20,21,0,2,0xffffffff};
        RawCanonPlacementTerrain terrain(pool,types,frames,islands,state,MakeCanonPermissionTerrainHooks(pool,permission,{{},
            [&](const CanonPlacementQuery& q,const CanonPlacementCell&) { CHECK(q.owner==2 && q.mode==0xffffffff);++neighbors;return false; },
            [&](const CanonPlacementQuery&,CanonPlacementTerrainState& value) { ++finishes;return value.permission && value.canPlaceGround; }}));
        RawCanonPlacementGeometry geometry(pool,types,frames,geometryState);
        RawCanonPlacementCollision collision(pool,hash,types,collisionState,previewState,MakeCanonGeometryCollisionHooks(pool,geometry,
            [&](const CanonPlacementQuery& q) { return MakeCanonTerrainGeometryHooks(pool,terrain,q); },MakeCanonTerrainCollisionHooks(pool,terrain,{})));
        RawCanonPlacementPreview preview(pool,types,spots,surfaces,previewState,MakeCanonGeometryPreviewHooks(pool,geometry,MakeCanonPlacementCollisionHooks(pool,collision,{})));
        RawCanonPlacement placement(pool,types,placementState,MakeCanonPlacementPreviewHooks(pool,preview,{
            [](std::uint32_t,std::uint32_t,std::uint32_t) { return PriestPlacementRect{0,0,48,33}; },{}}));
        CHECK(placement.MayPlace(query));CHECK(lookups==2 && neighbors==1 && finishes==1 && state.permission && state.groundComplete && state.canPlaceGround);
        // 그래프 비활성 상태는 Player 조회 없이 각 모양의 주변 권한 경계까지 이어진다.
        permissionState.graphReady=0;lookups=neighbors=finishes=0;CHECK(!placement.MayPlace(query));CHECK(lookups==0 && neighbors==9 && finishes==1 && !state.permission);
    }
}
// 원래 요청 DWORD를 유지하고 후보 helper에는 signed BYTE 소유자를 전달한다.
TEST_CASE(canon_permission_signed_owner_adapter_and_current_reads) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);
    raw[34]=1;raw[10]=kOther;Put(raw,14,0x80000000);Put(raw,18,0x7fc12345);CanonPlacementPermissionState state;
    state.graphReady=1;state.useAlliances=1;state.alliances[8]=1;int calls=0;
    RawCanonPlacementPermission permission(pool,state,[&](std::uint32_t owner,std::uint32_t type,float x,float y) {
        ++calls;CHECK(owner==0xffffffff && type==kOther && std::bit_cast<std::uint32_t>(x)==0x80000000 && std::bit_cast<std::uint32_t>(y)==0x7fc12345);
        raw[34]=2;state.useAlliances=0;return 0x80000000U;
    });
    auto hooks=MakeCanonPermissionTerrainHooks(pool,permission,{});const CanonPlacementQuery query{kOwn,25,20,21,0,0xabff,7};
    CHECK(hooks.candidatePermission(query,sid));CHECK(query.owner==0xabff && calls==1);
    CHECK(!hooks.candidatePermission(query,sid) && calls==1);
    state.graphReady=0;CHECK(permission.Candidate(Sid{65535},1)==0);
    state.graphReady=1;CHECK(permission.Candidate(Sid{65535},0)==1 && calls==1);
}
// 원본 밖의 관계 주소/누락 locator/다른 SID 풀은 명확한 오류로 진단한다.
TEST_CASE(canon_permission_required_boundary_and_relation_guard) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(pool.Edition(),kCapacity,true);CanonPlacementPermissionState state;
    CHECK(Throws([&] { RawCanonPlacementPermission missing(pool,state,{}); }));
    RawCanonPlacementPermission permission(pool,state,[](std::uint32_t,std::uint32_t,float,float) { CHECK(false);return 0U; });
    CHECK(Throws([&] { MakeCanonPermissionTerrainHooks(other,permission,{}); }));
    const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[34]=255;state.graphReady=state.useAlliances=state.allowOtherOwners=1;
    CHECK(Throws([&] { permission.Candidate(sid,128); }));CHECK(Throws([&] { permission.Related(255,128); }));
    state.editor=1;CHECK(permission.Related(255,128));
}
