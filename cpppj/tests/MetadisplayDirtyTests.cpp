// 두 PE의 미니맵 dirty 고유 등록·활성 표시·view 비교·전체/부분 callback 순서를 재생한다.
#include "RawSceneSupport.h"
#include "o/RawMetadisplayDirty.h"
#include "o/SquidFactory.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"
#include <bit>
#include <limits>
#include <sstream>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace {
// 검사가 native와 같은 전체 바이트 해시를 계산하도록 DWORD를 little endian으로 붙인다.
void Append(std::vector<std::uint8_t>& bytes,std::uint32_t value) {
    // FNV의 호스트 엔디언 차이를 없애기 위해 BYTE마다 저장한다.
    for (std::size_t i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
// native 보고서의 순서 보존 FNV32를 복원한다. 구현 결과의 의미를 다시 계산하지 않는다.
std::uint32_t Fnv(std::span<const std::uint8_t> bytes) {
    std::uint32_t value=2166136261U;
    // 전체 큐·bitmap·관찰 사건의 모든 BYTE를 입력한다.
    for (const auto byte:bytes) value=(value^byte)*16777619U;
    return value;
}
// 정밀 body 입력과 callback 변경을 같은 raw 풀의 처리기에 공급하는 검사 장면이다.
struct MetadisplayScene {
    SidPool pool; // 같은 raw SID의 현재 좌표를 조회하는 실제 풀이다.
    MetadisplayDirtyState state; // native와 같은 큐·cache·bitmap·frame 입력이다.
    MetadisplayDirtyView current; // 실제 Refresh의 현재 camera/rectangle 입력이다.
    std::vector<std::uint8_t> events; // 명시 rebuild/cell callback의 순서를 기록한 BYTE다.
    std::uint32_t mutation{}; // callback에서 current count/full을 바꾸는 명시 입력이다.
    bool mutated{}; // 첫 cell에서만 신규 고유 셀을 등록하는 입력 상태다.
    // client 번호5를 확보하되 원본 게임/초기 로딩을 대신했다고 주장하지 않는다.
    explicit MetadisplayScene(bool patch):pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,24000,false) {
        CHECK(pool.Allocate(2)==Sid{5});
    }
    // fixture의 의미 있는 원본 전역·count 밖 오래된 큐 자료도 빠짐없이 공급한다.
    void Prepare(std::span<const std::uint32_t> input) {
        CHECK(input.size()==13);state={};current={};events.clear();mutated=false;mutation=input[10];
        state.displayPresent=input[1]!=0;state.displayFlags=static_cast<std::uint8_t>(input[2]);state.count=static_cast<std::int32_t>(input[3]);
        state.fullRefresh=input[4];state.bitmapWidth=input[5] ? 128 : 0;
        if (input[5]) state.bitmap.assign(16384,0xa5);
        state.initialized=input[6];state.frames=input[7];
        // 원본 첫 두 후보는 중복 검사 입력에 대응하며 count 밖 바이트를 지우지 않는다.
        for (std::int32_t i=0;i<100;++i) state.cells[static_cast<std::size_t>(i)]={i,i+10,0xa5a5a5a5};
        if (input[8]==1) current.camera[0]=0x80000000;
        else if (input[8]==2) state.cached.camera[0]=current.camera[0]=0x7fc00001;
        else if (input[8]==3) current.camera={std::bit_cast<std::uint32_t>(12.5f),std::bit_cast<std::uint32_t>(22.75f)};
        else if (input[8]==4) { state.cached.camera[0]=0x7fc00001;current.camera[0]=0x7fc00002; }
        else if (input[8]==5) state.cached.camera=current.camera={std::bit_cast<std::uint32_t>(12.5f),std::bit_cast<std::uint32_t>(22.75f)};
        if (input[9]==2) state.cached.rectangle={11,22,33,44};
        if (input[9]) current.rectangle={11,22,33,44};
    }
    // 관찰 경계 뒤 현재 full/count를 다시 읽는 원본 body를 검사한다.
    MetadisplayDirtyHooks Hooks() {
        return {
            [this] {
                Append(events,1);Append(events,0);Append(events,0);
                if (mutation==1) state.fullRefresh=0;
                else if (mutation==2) state.fullRefresh=1;
                else if (mutation==3) state.count=2;
            },
            [this](std::int32_t x,std::int32_t y) {
                Append(events,2);Append(events,static_cast<std::uint32_t>(x));Append(events,static_cast<std::uint32_t>(y));
                if (mutation==4 && !mutated) { mutated=true;state.cells[static_cast<std::size_t>(state.count)]={40,41,0};++state.count; }
            }
        };
    }
    // 원본 snapshot 필드 순서와 전체 배열/그림/관찰 순서를 그대로 직렬화한다.
    std::string Snapshot() const {
        std::vector<std::uint32_t> output{static_cast<std::uint32_t>(state.count),state.fullRefresh,static_cast<std::uint32_t>(state.bitmapWidth),state.initialized,state.frames};
        output.insert(output.end(),state.cached.camera.begin(),state.cached.camera.end());
        // 사각형 signed 필드는 원본 DWORD 비트로 옮긴다.
        for (const auto number:state.cached.rectangle) output.push_back(static_cast<std::uint32_t>(number));
        std::vector<std::uint8_t> cells;
        // count 밖의 큐도 원본이 초기화하지 않는 바이트로 비교한다.
        for (const auto& cell:state.cells) { Append(cells,static_cast<std::uint32_t>(cell.x));Append(cells,static_cast<std::uint32_t>(cell.y));Append(cells,cell.unused); }
        output.insert(output.end(),{Fnv(cells),state.bitmap.empty() ? 0U : Fnv(state.bitmap),static_cast<std::uint32_t>(events.size()/12),Fnv(events)});
        std::string result;
        // TSV 원본 숫자 문자열과 같은 순서로 저장한다.
        for (const auto number:output) { if (!result.empty()) result+=',';result+=std::to_string(number); }
        return result;
    }
};
}

// native의 두 판본 고유 입력을 전부 재생하며 두 정밀도 반복은 별도 개수로 세지 않는다.
TEST_CASE(metadisplayDirty_native_x86_replay) {
    const auto fixture=LoadFixture(NETSTORM_METADISPLAYDIRTY_FIXTURE);std::size_t rows=0;
    // 큐 등록·Refresh의 전체 분기와 현재 callback 변경 결과를 비교한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==3);const auto fields=Split(row[1],',');std::vector<std::uint32_t> input;
        // 기대값은 원본 fixture의 결과 열을 사용한다.
        for (const auto& field:fields) input.push_back(Number(field));
        MetadisplayScene scene(row[0]=="originals");scene.Prepare(input);RawMetadisplayDirty dirty(scene.pool,scene.state,scene.Hooks());
        if (input[0]==0) dirty.Enqueue(std::bit_cast<float>(input[11]),std::bit_cast<float>(input[12]));
        else dirty.Refresh(scene.current);
        if (scene.Snapshot()!=row[2]) std::fprintf(stderr,"미니맵 대조 실패 %s %s\n",row[0].c_str(),row[1].c_str());
        CHECK(scene.Snapshot()==row[2]);++rows;
    }
    CHECK(rows==1040);
}

// same-pool 어댑터와 현재 슬롯 좌표가 기존 cache 사본을 사용하지 않는지 확인한다.
TEST_CASE(metadisplayDirty_same_pool_and_live_coordinates) {
    MetadisplayScene scene(true);scene.state.displayPresent=true;scene.state.displayFlags=1;
    RawMetadisplayDirty dirty(scene.pool,scene.state);const auto hook=MakeMetadisplayDirtyHook(scene.pool,dirty);
    auto raw=scene.pool.AllocatedBytes(Sid{5});Put(raw,14,std::bit_cast<std::uint32_t>(7.99f));Put(raw,18,std::bit_cast<std::uint32_t>(-7.99f));
    dirty.Enqueue(Sid{5});CHECK(scene.state.count==1);CHECK(scene.state.cells[0].x==1 && scene.state.cells[0].y==-1);
    Put(raw,14,std::bit_cast<std::uint32_t>(8.0f));dirty.Enqueue(Sid{5});hook(8.25f,-4.0f);
    CHECK(scene.state.count==2);CHECK(scene.state.cells[1].x==2 && scene.state.cells[1].y==-1);
    SidPool other(OriginalEdition::Patch1078,24000,false);CHECK(Throws([&] { MakeMetadisplayDirtyHook(other,dirty); }));
    CHECK(Throws([&] { dirty.Enqueue(std::numeric_limits<float>::infinity(),1); }));
}

// 비활성 자연 반환과 필수 그림 효과의 거부를 구별하며 bitmap 최초 확보 시점을 검사한다.
TEST_CASE(metadisplayDirty_inactive_allocation_and_required_effects) {
    MetadisplayScene scene(true);scene.state.count=1;scene.state.cells[0]={3,4,0};RawMetadisplayDirty dirty(scene.pool,scene.state);
    dirty.Enqueue(std::numeric_limits<float>::infinity(),1);CHECK(scene.state.count==1);
    dirty.Refresh({});CHECK(scene.state.bitmap.size()==16384);CHECK(scene.state.bitmapWidth==128);CHECK(scene.state.fullRefresh==1);
    CHECK(scene.state.count==1 && scene.state.frames==0);
    scene.state.displayPresent=true;scene.state.displayFlags=1;CHECK(Throws([&] { dirty.Refresh({}); }));
    MetadisplayDirtyHooks hooks;hooks.rebuild=[] {};
    RawMetadisplayDirty missingCell(scene.pool,scene.state,hooks);CHECK(Throws([&] { missingCell.Refresh({}); }));
}

// 실제 공간 몸체의 통지 시점·기존 위치·매몰/타입 조건과 pool 연결을 검사한다.
TEST_CASE(metadisplayDirty_actual_pop_unpop_and_overlap_order) {
    // 판본별 같은 raw 풀의 Create→Pop→Unpop을 사용하며 native의 새 관찰 수에는 더하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,24000,false);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[74].flags2=2;types[74].footX=types[74].footY=1;types[74].maxHitPoints=91;
        SquidFactory factory(pool,types);SquidPop pop(pool,hash,spots);SquidUnpop unpop(pool,hash,spots);
        MetadisplayDirtyState state;state.displayPresent=true;state.displayFlags=1;RawMetadisplayDirty dirty(pool,state);
        pop.SetMetadisplayDirty(&dirty);unpop.SetMetadisplayDirty(&dirty);const auto sid=factory.Create(74,2);
        if (edition==OriginalEdition::Patch1078) {
            spots[40*256+30]=2;CHECK(pop.Pop(sid,types[74],1,1,30,40,8)==RawPopResult::Overlap);
            CHECK(state.count==1 && state.cells[0].x==7 && state.cells[0].y==10);
            CHECK(Get(pool.Slot(sid),14)==std::bit_cast<std::uint32_t>(30.f));CHECK((pool.Slot(sid)[11]&4)!=0);
            CHECK(hash.Bucket(0,30,40)==0);spots[40*256+30]=0;
        }
        CHECK(pop.Pop(sid,types[74],1,1,30,40,8)==RawPopResult::Registered);
        CHECK(state.count==1 && state.cells[0].x==7 && state.cells[0].y==10);CHECK(spots[40*256+30]==2);
        state.count=0;unpop.Unpop(sid,types[74],8);
        CHECK(state.count==1 && state.cells[0].x==7 && state.cells[0].y==10);
        CHECK((pool.Slot(sid)[11]&4)!=0 && spots[40*256+30]==0 && hash.Bucket(0,30,40)==0);
        unpop.Unpop(sid,types[74],8);CHECK(state.count==1);
        const auto buried=factory.Create(74,2);pool.AllocatedBytes(buried)[edition==OriginalEdition::Patch1078 ? 40U : 35U]|=8;
        CHECK(pop.Pop(buried,types[74],1,1,40,60,8)==RawPopResult::Registered);CHECK(state.count==1);
        unpop.Unpop(buried,types[74],8);CHECK(state.count==1);
        auto plain=types[74];plain.flags2=0;const auto nonSurface=factory.Create(74,2);
        CHECK(pop.Pop(nonSurface,plain,1,1,80,100,8)==RawPopResult::Registered);CHECK(state.count==1);
        unpop.Unpop(nonSurface,plain,8);CHECK(state.count==1);
        SidPool other(edition,24000,false);MetadisplayDirtyState otherState;RawMetadisplayDirty foreign(other,otherState);
        CHECK(Throws([&] { pop.SetMetadisplayDirty(&foreign); }));CHECK(Throws([&] { unpop.SetMetadisplayDirty(&foreign); }));
    }
}
