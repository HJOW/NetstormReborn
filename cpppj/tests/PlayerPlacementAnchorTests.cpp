// Player 조회·거리 선택·그래프와 원본 임시 후보 목록을 독립 fixture로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPlayerPlacementAnchor.h"
#include "o/SquidOwner.h"

namespace {
using namespace netstorm::test::rawscene;
// 각 PE의 collector/그래프/거리 관찰 행 수와 같은 가상 슬롯 범위다.
constexpr std::size_t kRows=1598,kTotal=3*kRows,kSlots=128;
// 탭 fixture의 쉼표 DWORD 목록을 읽되 빈 입력은 빈 목록으로 유지한다.
std::vector<std::uint32_t> Numbers(const std::string& text) {
    std::vector<std::uint32_t> result;if (text.empty()) return result;
    // 입력 번호만 옮기며 순서·중복은 지우지 않는다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// 원본 정상 반환과 후보 목록을 읽으며 기대 판단은 계산하지 않는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_PLAYERANCHOR_FIXTURE).rows;return rows;
}
// 같은 raw/type/목록/지도 입력에서 C++ 반환·후보 순서·읽기 전용 자료를 대조한다.
void Replay(std::string_view name) {
    const bool patch=name=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<std::uint16_t> surfaces(65536);SquidPostPopState bookkeeping;
    SquidPostPopList additional;ContainedFinderState contained;PlayerPlacementAnchorState state;
    RawPlayerPlacementAnchor anchor(pool,types,surfaces,bookkeeping,additional,contained,state);
    const auto bytes=pool.Bytes().first(kSlots*pool.Layout().stride);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
    std::array<std::uint8_t,131072> mapBytes{};CHECK(Fixture().size()==kTotal);std::size_t count=0;
    // 판본별 모든 입력의 물리 자료를 원본 실행기와 같은 초기 상태로 만든다.
    for (const auto& row:Fixture()) {
        if (row[0]!=name) continue;CHECK(row.size()==20);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        types[82].flags1=0x800;types[82].flags2=Number(row[7]);types[83].flags1=types[83].flags2=0;
        state.graphReady=Number(row[4]);state.invalidGraph=Number(row[5]);state.ignoreRestrictions=Number(row[6]);
        const auto owner=Number(row[2]);auto workshops=Numbers(row[12]),extras=Numbers(row[13]);
        bookkeeping.ownerFactories[owner].entries=workshops;bookkeeping.ownerFactories[owner].count=static_cast<std::uint32_t>(workshops.size());
        additional.entries=extras;additional.count=static_cast<std::uint32_t>(extras.size());const std::uint16_t surface=Number(row[9]) ? 116 : 0;
        std::fill(surfaces.begin(),surfaces.end(),surface);
        // native 지도와 같은 little endian 바이트를 checksum 대조용으로 준비한다.
        for (std::size_t i=0;i<surfaces.size();++i) Put(mapBytes,2*i,surface,2);
        // 실제 raw 입력의 head/next와 겹치는 종속 kind/mask까지 같은 순서로 쓴다.
        for (const auto& entry:Split(row[14],';')) {
            const auto node=Numbers(entry);CHECK(node.size()==10);const std::size_t offset=node[0]*pool.Layout().stride;
            auto slot=raw.subspan(offset,pool.Layout().stride);slot[10]=static_cast<std::uint8_t>(node[1]);slot[patch ? 34 : 32]=static_cast<std::uint8_t>(node[2]);slot[patch ? 30 : 28]=static_cast<std::uint8_t>(node[3]);
            Put(slot,14,node[4]);Put(slot,18,node[5]);Put(slot,6,node[6],2);Put(slot,4,node[7],2);
            if (node[1]==5 || node[1]==6) { Put(slot,18,node[8],2);Put(slot,20,node[9],2); }
        }
        const float x=std::bit_cast<float>(Number(row[10])),y=std::bit_cast<float>(Number(row[11]));PlayerAnchorSelection observed;
        if (row[1]=="L") observed=anchor.Inspect(owner,Number(row[3]),x,y);
        else if (row[1]=="G") observed.sid=anchor.GraphAt(x,y);
        else if (row[1]=="S") observed.sid=anchor.GraphFor(Number(row[15]));
        else observed.sid=anchor.Nearest(workshops,owner,x,y);
        const bool same=observed.sid==Number(row[16]) && observed.candidates==Numbers(row[17]) && Adler(raw)==Number(row[18]) && Adler(mapBytes)==Number(row[19]);
        CHECK(same);if (!same) { std::printf("Player anchor %s 행 %zu (%s) 불일치\n",std::string(name).c_str(),count,row[1].c_str());break; }++count;
    }
    CHECK(count==kRows);
}
}
// 패치 전체 collector·실제 거리/그래프/contained 반환을 대조한다.
TEST_CASE(player_anchor_patch_x86) { Replay("originals"); }
// CD 임시 배열 정리와 BYTE graph/owner 반환을 대조한다.
TEST_CASE(player_anchor_cd_x86) { Replay("originalCD"); }
// 추가 PE의 실제 명령으로 기록한 모든 입력을 재생한다.
TEST_CASE(player_anchor_1037_x86) { Replay("original1037"); }
// 실제 소유자 작업장 장부→Player 조회→후보 권한→지형 권한으로 연결한다.
TEST_CASE(player_anchor_owner_bookkeeping_permission_and_terrain) {
    // 판본별 다른 graph/owner BYTE를 가진 같은 수명 장면을 실행한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        std::vector<PriestPlainCanonType> frames(types.size());std::vector<std::uint16_t> surfaces(65536),islands(65536);SquidPostPopState bookkeeping;
        // 소유자 지정이 현재 목록의 저장소에 실제로 추가/제거할 용량을 준비한다.
        for (auto& list:bookkeeping.ownerFactories) list.entries.resize(4);
        types[82].flags1=0x800;types[82].flags2=0x4000;types[84].flags1=0x800;types[84].flags2=2;types[84].footX=types[84].footY=1;
        types[83].group=0;types[83].flags1=2;types[83].footX=types[83].footY=1;frames[84].frames=RiftTypeFrames(std::vector<FrameCode>(1));
        const auto workshop=pool.Allocate(),island=pool.Allocate(),child=pool.Allocate();auto work=pool.AllocatedBytes(workshop),land=pool.AllocatedBytes(island),dependent=pool.AllocatedBytes(child);
        work[10]=82;work[11]=0;work[patch ? 30 : 28]=7;Put(work,14,std::bit_cast<std::uint32_t>(18.0F));Put(work,18,std::bit_cast<std::uint32_t>(20.0F));
        land[10]=84;land[11]=0;land[patch ? 30 : 28]=7;land[patch ? 34 : 32]=1;Put(land,8,9,2);Put(land,14,std::bit_cast<std::uint32_t>(20.0F));Put(land,18,std::bit_cast<std::uint32_t>(21.0F));
        Put(land,patch ? 36 : 34,0,patch ? 4 : 1);surfaces[21*256+20]=island.value;islands[21*256+20]=island.value;
        SquidOwnerMode mode;mode.battle=true;SquidOwner owner(pool,types,bookkeeping,mode);owner.Set(workshop,1);
        SquidPostPopList additional;ContainedFinderState contained;PlayerPlacementAnchorState anchorState;anchorState.graphReady=1;
        RawPlayerPlacementAnchor anchor(pool,types,surfaces,bookkeeping,additional,contained,anchorState);
        CanonPlacementPermissionState permissionState;permissionState.graphReady=1;RawCanonPlacementPermission permission(pool,permissionState,MakePlayerAnchorQuery(pool,anchor));
        CHECK(permission.Candidate(island,1)==workshop.value);CHECK(bookkeeping.ownerFactories[1].count==1);
        CanonPlacementTerrainState terrainState;RawCanonPlacementTerrain terrain(pool,types,frames,islands,terrainState,MakeCanonPermissionTerrainHooks(pool,permission,{{},
            [](const CanonPlacementQuery&,const CanonPlacementCell&) { return false; },
            [](const CanonPlacementQuery&,CanonPlacementTerrainState& state) { return state.permission && state.canPlaceGround; }}));
        const CanonPlacementQuery query{83,25,20,21,0,1,7};terrain.Begin(query);terrain.BeginShape(query,{0,0,0,20,21,20,21,{}});terrain.Candidate(query,island);
        CHECK(terrain.EndShape(query) && terrain.Finish(query) && terrainState.permission);
        owner.Set(workshop,2);CHECK(bookkeeping.ownerFactories[1].count==0 && bookkeeping.ownerFactories[2].count==1);CHECK(permission.Candidate(island,1)==0);
        owner.Set(workshop,1);work[patch ? 30 : 28]=3;Put(work,6,child.value,2);dependent[10]=6;Put(dependent,18,83,2);Put(dependent,20,16,2);
        additional.entries={island.value};additional.count=1;
        CHECK(anchor.Locate(1,83,20,21)==island.value);
        contained.containedType=7;CHECK(anchor.Locate(1,83,20,21)==0);
        anchorState.graphReady=0;CHECK(anchor.GraphAt(20,21)==254 && anchor.GraphFor(workshop.value)==3);
    }
}
// 손상된 자료는 실제 접근 시 진단하고 무효 그래프/빈 목록의 조기 경로는 유지한다.
TEST_CASE(player_anchor_required_data_and_lazy_guards) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(pool.Edition(),kCapacity,true);std::vector<RiftTypeRecord> types(188);
    std::vector<std::uint16_t> surfaces(65536);SquidPostPopState bookkeeping;SquidPostPopList additional;ContainedFinderState contained;PlayerPlacementAnchorState state;
    RawPlayerPlacementAnchor anchor(pool,types,surfaces,bookkeeping,additional,contained,state);
    CHECK(anchor.Locate(0xffffffff,0xffffffff,20,21)==0);CHECK(Throws([&] { MakePlayerAnchorQuery(other,anchor); }));
    CHECK(Throws([&] { RawPlayerPlacementAnchor bad(pool,types,std::span<const std::uint16_t>{},bookkeeping,additional,contained,state); }));
    state.invalidGraph=0;CHECK(Throws([&] { anchor.Locate(9,82,20,21); }));
    additional.count=1;CHECK(anchor.Locate(1,0xffffffff,20,21)==0);
    bookkeeping.ownerFactories[1].count=1;CHECK(Throws([&] { anchor.Locate(1,82,20,21); }));
    CHECK(anchor.GraphFor(0)==0);contained.checkingDead=true;CHECK(Throws([&] { anchor.GraphFor(0); }));
    CHECK(Throws([&] { anchor.GraphFor(65536); }));
}
