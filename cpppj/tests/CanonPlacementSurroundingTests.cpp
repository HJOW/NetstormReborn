// 모양 기반 flag 8 표면 finder와 주변 권한 구간을 실제 세 PE의 독립 관찰로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPlacementSurrounding.h"
#include "o/RawPlayerPlacementAnchor.h"
#include <cmath>

namespace {
using namespace netstorm::test::rawscene;
// 각 PE의 현재 독립 입력 수와 native 실행기가 checksum을 기록한 슬롯 수다.
constexpr std::size_t kRows=524,kSlots=128;
// 입력/기계어 관찰의 쉼표 목록을 그대로 읽는다.
std::vector<std::uint32_t> Numbers(const std::string& text) {
    std::vector<std::uint32_t> values;
    // 순서와 중복을 유지하고 판단 결과를 계산하지 않는다.
    for (const auto& value:Split(text,',')) values.push_back(Number(value));return values;
}
// 실제 PE에서 기록된 fixture를 한 번 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_CANONSURROUNDING_FIXTURE).rows;return rows;
}
// 원본의 +34/+68..+7c/+64/WORD cache 순서로 C++ 커서를 비교 가능한 목록으로 만든다.
std::vector<std::uint32_t> Snapshot(const RawCanonSurfaceWalk& walk) {
    const auto& state=walk.State();std::vector<std::uint32_t> result{state.current.value,
        static_cast<std::uint32_t>(state.x),static_cast<std::uint32_t>(state.y),static_cast<std::uint32_t>(state.right),
        static_cast<std::uint32_t>(state.top),static_cast<std::uint32_t>(state.left),static_cast<std::uint32_t>(state.bottom),static_cast<std::uint32_t>(state.returned.size())};
    result.insert(result.end(),state.returned.begin(),state.returned.end());return result;
}
// 각 판본의 실제 함수/구간 관찰을 같은 raw/타입/지도 자료로 대조한다.
void Replay(std::string_view edition) {
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
    std::vector<std::uint16_t> islands(65536);std::vector<std::uint8_t> spots(65536);std::array<std::uint8_t,131072> mapBytes{};
    auto bytes=pool.Bytes().first(kSlots*pool.Layout().stride);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
    std::vector<std::vector<std::uint32_t>> anchors;std::uint32_t selected=0,mutation=0;
    // 미래 후보/지도만 바꾸고 이미 반환한 현재 번호와 캐시는 되돌리지 않는다.
    const auto mutate=[&] {
        if (mutation==1) raw[53*pool.Layout().stride+11]=2;
        else if (mutation==2) islands[22*256+20]=0;
        else if (mutation==3) raw[53*pool.Layout().stride+(patch ? 34 : 32)]=2;
        else if (mutation==4) {
            frames[82].frames=frames[83].frames=RiftTypeFrames(std::vector<FrameCode>{{'P','P',1,0},{'P','P',2,0},{'J','P',3,0}});
        }
    };
    CanonPlacementPermissionState permissionState;
    RawCanonPlacementPermission permission(pool,permissionState,[&](std::uint32_t owner,std::uint32_t type,float x,float y) {
        anchors.push_back({owner,type,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});
        if (anchors.size()==1) mutate();return selected;
    });
    RawCanonPlacementSurrounding surrounding(pool,types,frames,islands,spots,permission);
    std::size_t count=0;CHECK(Fixture().size()==3*kRows);
    // 원본 기계어에서 기록한 각 입력의 자료와 조회 상태를 준비한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==28);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        std::fill(islands.begin(),islands.end(),std::uint16_t{});std::fill(spots.begin(),spots.end(),std::uint8_t{});anchors.clear();
        types[82].group=0;types[82].footX=static_cast<int>(Number(row[2]));types[82].footY=static_cast<int>(Number(row[3]));types[82].flags1=0x800;types[82].flags2=Number(row[7]);
        types[83].footX=types[83].footY=1;types[83].flags1=Number(row[9]);types[83].flags2=Number(row[8]);
        frames[82].frames=frames[83].frames=RiftTypeFrames(std::vector<FrameCode>{{'P','P',1,0},{'A','B',2,0},{'J','P',3,0}});
        permissionState.graphReady=Number(row[10]);permissionState.editor=Number(row[11]);permissionState.useAlliances=Number(row[12]);permissionState.allowOtherOwners=Number(row[13]);permissionState.alliances.fill(Number(row[15]));
        selected=Number(row[16]);mutation=Number(row[18]);
        // 입력 슬롯의 판본별 BYTE/DWORD 필드를 실행기와 같은 순서로 쓴다.
        for (const auto& part:Split(row[19],';')) {
            const auto node=Numbers(part);auto slot=raw.subspan(node[0]*pool.Layout().stride,pool.Layout().stride);
            slot[10]=83;slot[11]=static_cast<std::uint8_t>(node[5]);slot[patch ? 34 : 32]=static_cast<std::uint8_t>(node[1]);slot[patch ? 40 : 35]=static_cast<std::uint8_t>(node[6]);
            Put(slot,14,node[2]);Put(slot,18,node[3]);Put(slot,patch ? 36 : 34,node[4],patch ? 4 : 1);
        }
        // 지도는 현재 등록 머리 번호를 그대로 보존한다.
        for (const auto& part:Split(row[20],';')) { const auto cell=Numbers(part);islands[cell[1]*256+cell[0]]=static_cast<std::uint16_t>(cell[2]); }
        if (row[21]!="-") {
            // 기준점 및 전체 발자국의 내부 spot 입력을 옮긴다.
            for (const auto& part:Split(row[21],';')) { const auto cell=Numbers(part);spots[cell[1]*256+cell[0]]=static_cast<std::uint8_t>(cell[2]); }
        }
        const float x=std::bit_cast<float>(Number(row[4])),y=std::bit_cast<float>(Number(row[5]));
        const CanonPlacementQuery query{82,0,x,y,0,Number(row[14]),0};const CanonPlacementCell cell{static_cast<int>(Number(row[6])),0,0,x,y,std::trunc(x),std::trunc(y),{}};
        bool same=true;
        if (row[1]=="W") {
            RawCanonSurfaceWalk walk(pool,types,frames,islands,spots,query,cell);std::vector<std::vector<std::uint32_t>> snapshots{Snapshot(walk)};
            if (walk.Current().value) mutate();
            // 마지막 0 관찰까지 비교하며 그 이후에는 원본 함수를 재호출하지 않는다.
            while (walk.Current().value) { walk.Next();snapshots.push_back(Snapshot(walk)); }
            const auto expected=Split(row[23],';');same=snapshots.size()==expected.size();
            // 행마다 실제 커서/캐시가 같아야 반환 SID 순서의 우연한 일치를 배제할 수 있다.
            for (std::size_t i=0;same && i<snapshots.size();++i) same=snapshots[i]==Numbers(expected[i]);
        } else {
            CanonPlacementTerrainState state;const auto hooks=MakeCanonSurroundingTerrainHooks(pool,surrounding,MakeCanonPermissionTerrainHooks(pool,permission,{{},{},
                [](const CanonPlacementQuery&,CanonPlacementTerrainState& current) { return current.permission; }}));
            RawCanonPlacementTerrain terrain(pool,types,frames,islands,state,hooks);terrain.Begin(query);terrain.BeginShape(query,cell);state.permission=Number(row[17])!=0;
            terrain.EndShape(query);same=terrain.Finish(query)==(Number(row[22])!=0);
        }
        if (row[24]=="-") same=same && anchors.empty();
        else {
            const auto expected=Split(row[24],';');same=same && anchors.size()==expected.size();
            // 실제 Player 경계의 타입/owner/원본 좌표와 계속 순회한 호출 횟수를 확인한다.
            for (std::size_t i=0;same && i<anchors.size();++i) same=anchors[i]==Numbers(expected[i]);
        }
        // 지도 번호를 little endian으로 만들어 전체 입력/변화의 checksum도 비교한다.
        for (std::size_t i=0;i<islands.size();++i) Put(mapBytes,2*i,islands[i],2);
        same=same && Adler(raw)==Number(row[25]) && Adler(mapBytes)==Number(row[26]) && Adler(spots)==Number(row[27]);
        CHECK(same);if (!same) { std::printf("주변 표면 %s 행 %zu (%s) 불일치\n",std::string(edition).c_str(),count,row[1].c_str());break; }++count;
    }
    CHECK(count==kRows);
}
}
// 패치의 전체 표면 finder와 실제 주변 구간의 모든 입력을 재생한다.
TEST_CASE(canon_surrounding_patch_x86) { Replay("originals"); }
// CD의 다른 구조체/프레임/루프를 실제 원본 관찰로 대조한다.
TEST_CASE(canon_surrounding_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE에서 기록한 독립 관찰을 재생한다.
TEST_CASE(canon_surrounding_1037_x86) { Replay("original1037"); }
// 실제 Player 기준점 조회와 주변 표면을 통해 EndShape 권한 누적까지 연결한다.
TEST_CASE(canon_surrounding_actual_player_anchor_and_terrain) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> frames(188);
    std::vector<std::uint16_t> islands(65536),surfaces(65536);std::vector<std::uint8_t> spots(65536);
    types[82].group=0;types[82].flags2=4;types[82].footX=types[82].footY=1;types[83].flags1=0x800;types[83].flags2=2;types[83].footX=types[83].footY=1;
    types[84].flags1=0x800;types[84].flags2=6;frames[82].frames=RiftTypeFrames(std::vector<FrameCode>{{'A','B',1,0}});frames[83].frames=RiftTypeFrames(std::vector<FrameCode>{{'P','P',1,0}});
    const Sid island=pool.Allocate(),workshop=pool.Allocate();auto land=pool.AllocatedBytes(island),work=pool.AllocatedBytes(workshop);
    land[10]=83;land[11]=0;land[34]=1;land[30]=7;Put(land,14,std::bit_cast<std::uint32_t>(20.0F));Put(land,18,std::bit_cast<std::uint32_t>(20.0F));Put(land,36,0);
    work[10]=84;work[34]=1;work[30]=7;islands[20*256+20]=island.value;surfaces[20*256+20]=island.value;
    SquidPostPopState books;books.ownerFactories[1].entries={workshop.value};books.ownerFactories[1].count=1;SquidPostPopList extras;ContainedFinderState contained;PlayerPlacementAnchorState anchorState;anchorState.graphReady=1;
    RawPlayerPlacementAnchor anchor(pool,types,surfaces,books,extras,contained,anchorState);CanonPlacementPermissionState permissions;permissions.graphReady=1;
    RawCanonPlacementPermission permission(pool,permissions,MakePlayerAnchorQuery(pool,anchor));RawCanonPlacementSurrounding surrounding(pool,types,frames,islands,spots,permission);
    CanonPlacementTerrainState state;RawCanonPlacementTerrain terrain(pool,types,frames,islands,state,MakeCanonSurroundingTerrainHooks(pool,surrounding,MakeCanonPermissionTerrainHooks(pool,permission,{{},{},
        [](const CanonPlacementQuery&,CanonPlacementTerrainState& current) { return current.permission && current.canPlaceGround; }})));
    const CanonPlacementQuery query{82,0,20,21,0,1,0};const CanonPlacementCell cell{0,0,0,20,21,20,21,{}};
    terrain.Begin(query);terrain.BeginShape(query,cell);CHECK(!state.permission);CHECK(terrain.EndShape(query) && state.permission && terrain.Finish(query));
    land[34]=0;CHECK(permission.Candidate(island,1)==workshop.value && permission.Surrounding(island,1)==0);
    permissions.allowOtherOwners=1;CHECK(surrounding.Inspect(query,cell));permissions.graphReady=0;CHECK(!surrounding.Inspect(query,cell));
}
// 새로운 자료 경계는 필수이며 잘못된 지도/프레임/발자국을 암묵 허용하지 않는다.
TEST_CASE(canon_surrounding_required_data_guards) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(pool.Edition(),kCapacity,true);std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> frames(188);
    std::vector<std::uint16_t> islands(65536);std::vector<std::uint8_t> spots(65536);CanonPlacementPermissionState state;
    RawCanonPlacementPermission permission(pool,state,[](std::uint32_t,std::uint32_t,float,float) { return 0U; });
    RawCanonPlacementSurrounding surrounding(pool,types,frames,islands,spots,permission);CHECK(Throws([&] { MakeCanonSurroundingTerrainHooks(other,surrounding,{}); }));
    CHECK(Throws([&] { RawCanonPlacementSurrounding wrong(other,types,frames,islands,spots,permission); }));
    const CanonPlacementQuery query{82,0,20,21,0,1,0};const CanonPlacementCell cell{0,0,0,20,21,20,21,{}};
    CHECK(Throws([&] { surrounding.Inspect(query,cell); }));types[82].footX=types[82].footY=1;
    CHECK(Throws([&] { surrounding.Inspect(query,cell); }));frames[82].frames=RiftTypeFrames(std::vector<FrameCode>{{'P','P',1,0}});
    islands.resize(1);CHECK(Throws([&] { RawCanonSurfaceWalk wrong(pool,types,frames,islands,spots,query,cell); }));
}
