// 원본 무시 함수/후보 구간 관찰과 실제 finder·미리보기·사제 생성 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPlacementCollision.h"
#include "o/SquidFactory.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 새 fixture의 판본별 행 수와 실제 사제 생성자 타입 번호다.
constexpr std::size_t kRows=3840,kTotal=3*kRows;
constexpr std::uint32_t kPriest=158;
// 원본 기대값을 읽기만 한다. C++ 판정으로 기대 mask를 만들지 않는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_PRIESTCOLLISION_FIXTURE).rows;return rows;
}
// 정상 게임 쓰기 API 밖의 reserved/free SID도 독립 실행기와 같은 합성 raw 입력으로 준비한다.
// 풀 자체는 const 객체가 아니며 이 캐스트는 테스트 입력 공급에만 사용한다.
std::span<std::uint8_t> InputSlot(SidPool& pool,Sid sid) {
    const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
}
// 세 판본의 전체 반환 비트값/후보 결과/144바이트 배열/raw 불변성을 대조한다.
void Replay(std::string_view name) {
    const auto edition=name=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    SidPool pool(edition,kCapacity,false);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    PriestPlacementCollisionState state;PriestPlacementPreviewState preview;
    RawPriestPlacementCollision collision(pool,hash,types,state,preview,{[](const PriestPlacementQuery&,const std::function<bool(SquidSearchArea)>&) { return true; },[](const PriestPlacementQuery&,Sid) {}});
    CHECK(Fixture().size()==kTotal);std::size_t count=0;
    // 각 입력 행을 독립적으로 복구하여 앞 행의 표시나 raw 상태가 섞이지 않게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=name) continue;CHECK(row.size()==23);
        auto& own=types[82];auto& other=types[83];own.flags2=Number(row[2]);own.flags1=Number(row[3]);other.flags2=Number(row[4]);
        const auto mode=Number(row[5]);state={Number(row[7]),Number(row[6])};const Sid sid{static_cast<std::uint16_t>(Number(row[8]))};
        own.footX=std::stoi(row[14]);own.footY=std::stoi(row[15]);other.footX=std::stoi(row[16]);other.footY=std::stoi(row[17]);
        auto raw=InputSlot(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});raw[10]=83;
        Put(raw,14,Number(row[12]));Put(raw,18,Number(row[13]));raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=static_cast<std::uint8_t>(Number(row[18]));
        preview.blocked.fill(Number(row[19]) ? 0xa5 : 0);
        std::uint32_t result{};
        if (row[1]=="I") result=PlacementIgnoreValue(edition,own,other,mode,state.ignoredGenus);
        else result=collision.InspectCandidate({82,std::bit_cast<float>(Number(row[10])),std::bit_cast<float>(Number(row[11])),0,1,mode},Number(row[9])!=0,sid) ? 1U : 0U;
        const bool same=result==Number(row[20]) && Hex(preview.blocked)==row[21] && Hex(pool.Slot(sid))==row[22];CHECK(same);
        if (!same) { std::printf("Priest collision %s 행 %zu: 기대 %s / 실제 %u\n",std::string(name).c_str(),count,row[20].c_str(),result);break; }++count;
    }
    CHECK(count==kRows);
}
}
// 10.78의 원본 무시 함수/한 후보 구간을 대조한다.
TEST_CASE(priest_collision_patch_x86) { Replay("originals"); }
// CD의 추가 bridge 비트/전역 반환 마스크와 후보 구간을 대조한다.
TEST_CASE(priest_collision_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 원본 구간도 별도로 대조한다.
TEST_CASE(priest_collision_1037_x86) { Replay("original1037"); }
// 실제 finder의 buried 제외·SID/전역 무시와 첫 거부 이후 탐색 중단을 검사한다.
TEST_CASE(priest_collision_real_finder_stops_after_rejection) {
    // 각 판본의 raw extra 오프셋과 서버 영역 경계를 사용한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[82].flags2=0x200000;types[82].footX=types[82].footY=1;types[83].footX=types[83].footY=1;
        const Sid sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=83;Put(raw,14,std::bit_cast<std::uint32_t>(20.0f));Put(raw,18,std::bit_cast<std::uint32_t>(21.0f));
        Put(raw,4,0xffff,2);hash.Bucket(1,20,21)=sid.value;PriestPlacementCollisionState state;PriestPlacementPreviewState preview;int requests=0;
        RawPriestPlacementCollision collision(pool,hash,types,state,preview,{[&](const PriestPlacementQuery&,const std::function<bool(SquidSearchArea)>& scan) {
            ++requests;CHECK(!scan({20,21,20,21}));CHECK(!scan({20,21,20,21}));return true; },[](const PriestPlacementQuery&,Sid) {}});
        CHECK(!collision.Inspect({82,20,21,0,1,1},true));CHECK(requests==1 && preview.blocked[13]==1);
        Put(raw,4,0,2);raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=8;preview.blocked.fill(0);
        CHECK(collision.InspectArea({82,20,21,0,1,1},true,{20,21,20,21}));CHECK(preview.blocked[13]==0);
        raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=0;state.ignoredGenus=0x200000;types[83].flags2=0x8000;
        const bool patch=edition==OriginalEdition::Patch1078;CHECK(collision.InspectArea({82,20,21,0,1,1},false,{20,21,20,21})==patch);
    }
}
// 후보 충돌 거부를 사제 나선 생성까지 연결하고 다음 위치에서 실제 생성자 실행을 확인한다.
TEST_CASE(priest_collision_spawn_prefix_preview_finder_factory) {
    // 원본 모양/지형과 일반 Pop은 경계이며 접두·미리보기·finder·생성자는 실제 구현이다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        types[kPriest].constructorAddress=TypeConstructorAddress(edition,kPriest);types[kPriest].flags2=0x210000;types[kPriest].footX=types[kPriest].footY=1;types[83].footX=types[83].footY=1;
        const Sid blocker=pool.Allocate();auto raw=pool.AllocatedBytes(blocker);raw[10]=83;Put(raw,14,std::bit_cast<std::uint32_t>(21.0f));Put(raw,18,std::bit_cast<std::uint32_t>(22.0f));hash.Bucket(1,21,22)=blocker.value;
        std::vector<std::uint8_t> spots(65536,6);std::vector<std::uint16_t> surfaces(65536);PriestPlacementPreviewState previewState;PriestPlacementCollisionState state{0,0x200000};PriestPlacementState placementState{0,1,0xffffffff};int scans=0,pops=0;Sid born{};
        RawPriestPlacementCollision collision(pool,hash,types,state,previewState,{[&](const PriestPlacementQuery& query,const std::function<bool(SquidSearchArea)>& scan) {
            ++scans;const auto x=static_cast<int>(query.x),y=static_cast<int>(query.y);return scan({x,y,x,y}); },[](const PriestPlacementQuery&,Sid) {}});
        RawPriestPlacementPreview preview(pool,types,spots,surfaces,previewState,MakePriestPlacementCollisionHooks(pool,collision,{
            [](const PriestPlacementQuery& query) { const auto x=static_cast<int>(query.x),y=static_cast<int>(query.y);return SquidSearchArea{x,y,x,y}; },{}}));
        RawPriestPlacement placement(pool,types,placementState,MakePriestPlacementPreviewHooks(pool,preview,{
            [](std::uint32_t,std::uint32_t,std::uint32_t) { return PriestPlacementRect{0,0,16,11}; },{}}));
        SquidFactory factory(pool,types);PriestSpawnState spawnState;
        RawPriestSpawn spawn(pool,types,spots,spawnState,MakePriestPlacementHooks(pool,placement,{
            {},[&](std::uint32_t type,std::uint32_t flags) { born=factory.Create(type,flags);return born; },[](Sid,std::uint32_t) {},
            [&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(sid==born && x==21 && y==21 && flags==0);++pops; },[](Sid) { CHECK(false); },[&](Sid sid) { CHECK(sid==born); }}));
        spawn.Spawn(20.75f,21.9f,kPriest,0x8101);
        CHECK(scans==2 && pops==1 && born.value==(patch ? 15001 : 6001) && Get(pool.Slot(born),12,2)==0x8101);
    }
}
// 외부 경계 누락과 타입/연결 계약 위반을 안전하게 진단한다.
TEST_CASE(priest_collision_guards_and_region_denial) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),otherPool(OriginalEdition::Patch1078,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(188);
    PriestPlacementCollisionState state;PriestPlacementPreviewState preview;CHECK(Throws([&] { RawPriestPlacementCollision missing(pool,hash,types,state,preview,{}); }));
    RawPriestPlacementCollision collision(pool,hash,types,state,preview,{[](const PriestPlacementQuery&,const std::function<bool(SquidSearchArea)>&) { return false; },[](const PriestPlacementQuery&,Sid) {}});
    CHECK(Throws([&] { collision.Inspect({82,20,21,0,1,0},true); }));types[82].flags2=0x200000;CHECK(!collision.Inspect({82,20,21,0,1,0},true));
    CHECK(Throws([&] { collision.Inspect({82,std::numeric_limits<float>::infinity(),21,0,1,0},true); }));
    CHECK(Throws([&] { MakePriestPlacementCollisionHooks(otherPool,collision,{}); }));
}
// 충돌에서 무시한 후보도 지형 경계를 받고 그 효과 이후 다음 후보의 현재 전역/타입을 읽는다.
TEST_CASE(priest_collision_terrain_boundary_preserves_finder_order) {
    // 실제 finder가 미리 저장한 next는 지형 경계의 현재 raw next 변경으로 덮이지 않는다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[82].flags2=0x210000;types[82].footX=types[82].footY=1;types[83].footX=types[83].footY=1;
        const Sid first=pool.Allocate(),second=pool.Allocate();auto firstRaw=pool.AllocatedBytes(first),secondRaw=pool.AllocatedBytes(second);
        // 같은 위치의 두 후보를 동일 버킷 체인에 둔다.
        for (auto raw:{firstRaw,secondRaw}) { raw[10]=83;Put(raw,14,std::bit_cast<std::uint32_t>(20.0f));Put(raw,18,std::bit_cast<std::uint32_t>(21.0f)); }
        Put(firstRaw,4,second.value,2);hash.Bucket(1,20,21)=first.value;PriestPlacementCollisionState state;PriestPlacementPreviewState preview;std::vector<Sid> visited;
        RawPriestPlacementCollision collision(pool,hash,types,state,preview,{
            [](const PriestPlacementQuery&,const std::function<bool(SquidSearchArea)>& scan) { return scan({20,21,20,21}); },
            [&](const PriestPlacementQuery& request,Sid sid) {
                CHECK(request.mode==0);visited.push_back(sid);
                if (sid==first) { Put(firstRaw,4,0,2);state.ignoredGenus=0x200000; }
            }});
        CHECK(!collision.Inspect({82,20,21,0,1,0},true));CHECK(visited==std::vector<Sid>{first});CHECK(preview.blocked[13]==1);
        // 첫 후보의 mode=0 walker 무시는 지형 효과를 호출했다. 다음 후보는 현재 전역에 의해 실제 충돌로 거부했다.
        CHECK(Get(firstRaw,4,2)==0 && state.ignoredGenus==0x200000);
    }
}
