// 원본 일반 후보 구간과 실제 패턴·finder·미리보기의 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawCanonPlacementCollision.h"
#include "o/RawCanonPlacementGeometry.h"
#include <limits>

namespace {
using namespace netstorm::test::rawscene;
// 일반 genus 9종×후보 7종×문맥 10종×좌표 6종×전역 2종의 판본별 행 수다.
constexpr std::size_t kRows=7560,kTotal=3*kRows;
// 원본 기대값을 읽기만 한다. C++ 판정으로 기대 mask를 만들지 않는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=LoadFixture(NETSTORM_CANONCOLLISION_FIXTURE).rows;return rows;
}
// 정상 게임 쓰기 API 밖의 reserved/free SID도 독립 실행기와 같은 합성 raw 입력으로 준비한다.
// 풀 자체는 const 객체가 아니며 이 캐스트는 테스트 입력 공급에만 사용한다.
std::span<std::uint8_t> InputSlot(SidPool& pool,Sid sid) {
    const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
}
// 세 판본의 후보 결과/144바이트 배열/raw 불변성을 대조한다.
void Replay(std::string_view name) {
    const auto edition=name=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    SidPool pool(edition,kCapacity,false);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    CanonPlacementCollisionState state;CanonPlacementPreviewState preview;
    RawCanonPlacementCollision collision(pool,hash,types,state,preview,{[](const CanonPlacementQuery&,const std::function<bool(SquidSearchArea)>&) { return true; },[](const CanonPlacementQuery&,Sid) {}});
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
        CHECK(row[1]=="C");
        const auto result=collision.InspectCandidate({82,25,std::bit_cast<float>(Number(row[10])),std::bit_cast<float>(Number(row[11])),0,1,mode},Number(row[9])!=0,sid) ? 1U : 0U;
        const bool same=result==Number(row[20]) && Hex(preview.blocked)==row[21] && Hex(pool.Slot(sid))==row[22];CHECK(same);
        if (!same) { std::printf("Canon collision %s 행 %zu: 기대 %s / 실제 %u\n",std::string(name).c_str(),count,row[20].c_str(),result);break; }++count;
    }
    CHECK(count==kRows);
}
}
// 10.78의 일반 후보 구간과 그 안의 실제 무시 helper를 대조한다.
TEST_CASE(canon_collision_patch_x86) { Replay("originals"); }
// CD의 추가 bridge 비트/전역 반환 마스크와 후보 구간을 대조한다.
TEST_CASE(canon_collision_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 원본 구간도 별도로 대조한다.
TEST_CASE(canon_collision_1037_x86) { Replay("original1037"); }
// 실제 finder의 buried 제외·SID/전역 무시와 첫 거부 이후 탐색 중단을 검사한다.
TEST_CASE(canon_collision_real_finder_stops_after_rejection) {
    // 각 판본의 raw extra 오프셋과 서버 영역 경계를 사용한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[82].flags2=0x8000;types[82].footX=types[82].footY=1;types[83].footX=types[83].footY=1;
        const Sid sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=83;Put(raw,14,std::bit_cast<std::uint32_t>(20.0f));Put(raw,18,std::bit_cast<std::uint32_t>(21.0f));
        Put(raw,4,0xffff,2);hash.Bucket(1,20,21)=sid.value;CanonPlacementCollisionState state;CanonPlacementPreviewState preview;int requests=0;
        RawCanonPlacementCollision collision(pool,hash,types,state,preview,{[&](const CanonPlacementQuery&,const std::function<bool(SquidSearchArea)>& scan) {
            ++requests;CHECK(!scan({20,21,20,21}));CHECK(!scan({20,21,20,21}));return true; },[](const CanonPlacementQuery&,Sid) {}});
        CHECK(!collision.Inspect({82,25,20,21,0,1,1},true));CHECK(requests==1 && preview.blocked[13]==1);
        Put(raw,4,0,2);raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=8;preview.blocked.fill(0);
        CHECK(collision.InspectArea({82,25,20,21,0,1,1},true,{20,21,20,21}));CHECK(preview.blocked[13]==0);
        raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=0;state.ignoredGenus=0x8000;types[83].flags2=0x8000;
        const bool patch=edition==OriginalEdition::Patch1078;CHECK(collision.InspectArea({82,25,20,21,0,1,1},false,{20,21,20,21})==patch);
    }
}
// 무시한 후보의 지형 효과 뒤 현재 genus와 미리 저장한 next를 사용하는지 확인한다.
TEST_CASE(canon_collision_current_type_and_saved_finder_next) {
    // 두 판본의 실제 SID/해시와 사제가 아닌 walker genus를 사용한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[82].flags2=0x10000;types[82].footX=types[82].footY=1;types[83].footX=types[83].footY=1;
        const Sid first=pool.Allocate(),second=pool.Allocate();auto firstRaw=pool.AllocatedBytes(first),secondRaw=pool.AllocatedBytes(second);
        // 같은 버킷의 체인에 두 후보를 공급한다.
        for (auto raw:{firstRaw,secondRaw}) { raw[10]=83;Put(raw,14,std::bit_cast<std::uint32_t>(20.0f));Put(raw,18,std::bit_cast<std::uint32_t>(21.0f)); }
        Put(firstRaw,4,second.value,2);hash.Bucket(1,20,21)=first.value;CanonPlacementCollisionState state;CanonPlacementPreviewState preview;std::vector<Sid> visited;
        RawCanonPlacementCollision collision(pool,hash,types,state,preview,{
            [](const CanonPlacementQuery& query,const std::function<bool(SquidSearchArea)>& scan) { CHECK(query.argument==25);return scan({20,21,20,21}); },
            [&](const CanonPlacementQuery& query,Sid sid) { CHECK(query.owner==1 && query.mode==0);visited.push_back(sid);Put(firstRaw,4,0,2);state.ignoredGenus=0xffffffff; }});
        CHECK(!collision.Inspect({82,25,20,21,0,1,0},true));
        CHECK(visited==std::vector<Sid>{first} && preview.blocked[13]==1 && Get(firstRaw,4,2)==0);
    }
}
// 실제 패턴/미리보기/finder를 연결해 두 번째 모양의 거부가 후속 효과를 생략하는지 확인한다.
TEST_CASE(canon_collision_pattern_preview_geometry_stops_at_second_shape) {
    // 받침 패턴의 첫 칸은 허용하고 지형 경계의 전역 변경 뒤 다음 칸을 거부한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        std::vector<PriestPlainCanonType> frames(types.size());PriestPlacementGeometryState geometryState;
        geometryState.patternTypes={107,82,94,157,131,129,140,142};types[157].flags2=0x10000;types[157].footX=types[157].footY=1;types[83].footX=types[83].footY=1;
        std::vector<FrameCode> codes;
        // 받침 3×3 패턴은 방향 A..I의 변형 1을 사용한다.
        for (std::uint8_t side='A';side<='I';++side) codes.push_back({side,'P',1,0});
        frames[157].frames=RiftTypeFrames(std::move(codes));RawCanonPlacementGeometry geometry(pool,types,frames,geometryState);
        const Sid first=pool.Allocate(),second=pool.Allocate();auto firstRaw=pool.AllocatedBytes(first),secondRaw=pool.AllocatedBytes(second);
        firstRaw[10]=secondRaw[10]=83;Put(firstRaw,14,std::bit_cast<std::uint32_t>(20.0f));Put(firstRaw,18,std::bit_cast<std::uint32_t>(21.0f));
        Put(secondRaw,14,std::bit_cast<std::uint32_t>(22.0f));Put(secondRaw,18,std::bit_cast<std::uint32_t>(21.0f));hash.Bucket(1,20,21)=first.value;hash.Bucket(1,22,21)=second.value;
        CanonPlacementCollisionState state;CanonPlacementPreviewState previewState;PriestPlacementState placementState{0,1,0};
        std::vector<std::uint8_t> spots(65536,6);std::vector<std::uint16_t> surfaces(65536);std::vector<std::string> events;
        RawCanonPlacementCollision collision(pool,hash,types,state,previewState,MakeCanonGeometryCollisionHooks(pool,geometry,
            [&](const CanonPlacementQuery& query) {
                CHECK(query.type==157 && query.argument==0 && query.owner==1 && query.mode==0);
                return CanonPlacementGeometryHooks{[&](const CanonTypeQuery&) { events.push_back("begin"); },
                    [&](const CanonTypeQuery&,const CanonPlacementCell&) { events.push_back("shape"); },
                    [&](const CanonTypeQuery&,const CanonPlacementCell&) { events.push_back("end");return true; },
                    [&](const CanonTypeQuery&) { events.push_back("finish");return true; }};
            },{{},[&](const CanonPlacementQuery&,Sid sid) { CHECK(sid==first);events.push_back("terrain");state.ignoredGenus=0xffffffff; }}));
        RawCanonPlacementPreview preview(pool,types,spots,surfaces,previewState,MakeCanonGeometryPreviewHooks(pool,geometry,
            MakeCanonPlacementCollisionHooks(pool,collision,{})));
        RawCanonPlacement placement(pool,types,placementState,MakeCanonPlacementPreviewHooks(pool,preview,{
            [](std::uint32_t type,std::uint32_t argument,std::uint32_t) { CHECK(type==157 && argument==0);return PriestPlacementRect{0,0,48,33}; },{}}));
        CHECK(!placement.MayPlace({157,0,20.5f,21.5f,0,1,0}));
        CHECK(events==std::vector<std::string>({"begin","shape","terrain","end","shape"}));
        CHECK(std::count(previewState.blocked.begin(),previewState.blocked.end(),std::uint8_t{1})==36);
    }
}
// 누락된 정책·잘못된 풀/타입과 원본에서 정의되지 않은 좌표를 구별한다.
TEST_CASE(canon_collision_guards_and_required_regions) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(OriginalEdition::Patch1078,kCapacity,true);SquidHash hash;
    std::vector<RiftTypeRecord> types(188);CanonPlacementCollisionState state;CanonPlacementPreviewState preview;
    CHECK(Throws([&] { RawCanonPlacementCollision missing(pool,hash,types,state,preview,{}); }));
    RawCanonPlacementCollision collision(pool,hash,types,state,preview,{
        [](const CanonPlacementQuery&,const std::function<bool(SquidSearchArea)>&) { return false; },[](const CanonPlacementQuery&,Sid) {}});
    CHECK(!collision.Inspect({82,25,20,21,0,1,0},true));
    CHECK(Throws([&] { collision.Inspect({188,0,20,21,0,1,0},true); }));
    CHECK(Throws([&] { collision.Inspect({82,25,std::numeric_limits<float>::infinity(),21,0,1,0},true); }));
    CHECK(Throws([&] { MakeCanonPlacementCollisionHooks(other,collision,{}); }));
    std::vector<PriestPlainCanonType> frames(types.size());PriestPlacementGeometryState geometryState;RawCanonPlacementGeometry geometry(pool,types,frames,geometryState);
    CHECK(Throws([&] { MakeCanonGeometryCollisionHooks(pool,geometry,{},{}); }));
    CHECK(Throws([&] { MakeCanonGeometryCollisionHooks(other,geometry,[](const CanonPlacementQuery&) { return CanonPlacementGeometryHooks{}; },{}); }));
}
