// 섬 받침 preDestroy와 noIsland postDestroy 재정의를 원본 기계어 기대값으로 검사하고 실제 지연 낙하 예약에 이어 본다.
#include "RawSceneSupport.h"
#include "o/Kernel.h"
#include "o/RawBridgeLifecycle.h"
#include "o/RawIslandLifecycle.h"
#include "o/RawSquidDestroy.h"
#include "o/SquidProcess.h"
#include "o/SquidUnpop.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 판본마다 기대하는 관찰 행 수다.
constexpr std::size_t kRowsPerKind=260;
// 저장된 독립 기계어 출력을 한 번만 읽는다.
const FixtureData& Fixture() {
    static const auto data=LoadFixture(NETSTORM_ISLANDLIFECYCLE_FIXTURE);
    return data;
}
// 원본 실행기의 대체와 같은 경계다: 호출 사실과 인자만 기록한다.
IslandLifecycleHooks Hooks(Scene& scene) {
    IslandLifecycleHooks hooks;
    hooks.scheduleFall=[&scene](Sid bridge,float x,float y) {
        scene.events.push_back("G:"+std::to_string(bridge.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+
            std::to_string(std::bit_cast<std::uint32_t>(y)));
    };
    hooks.fallWalker=[&scene](Sid walker) { scene.events.push_back("W:"+std::to_string(walker.value)); };
    hooks.basePreDestroy=[&scene](Sid sid,std::uint32_t flags) { scene.events.push_back("X:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
    hooks.basePostDestroy=[&scene](Sid sid,std::uint32_t flags) { scene.events.push_back("Y:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
    return hooks;
}
// 한 판본의 모든 관찰 행을 재생한다. 두 재정의는 풀을 바꾸지 않아야 한다.
void Replay(const char* edition) {
    Scene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::map<std::string,std::size_t> counts;int failures=0;
    // 같은 장면과 flags로 C++을 실행해 사건 순서·인자를 원본 관찰과 비교한다.
    for (const auto& row:Fixture().rows) {
        if (row[1]!=edition) continue;
        CHECK(row.size()==6);
        const auto& kind=row[0];std::string actual;bool unchanged=false;
        try {
            scene.Prepare(Fixture().scenes.at(row[2]));
            const std::vector<std::uint8_t> before(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
            RawIslandLifecycle lifecycle(scene.pool,*scene.neighbors,Hooks(scene));
            if (kind=="IslandPre") lifecycle.IslandPreDestroy(kSource,Number(row[3]));
            else if (kind=="SurfacePost") lifecycle.SurfacePostDestroy(kSource,Number(row[3]));
            else throw std::runtime_error("알 수 없는 행 종류");
            actual=scene.Events();
            unchanged=std::equal(before.begin(),before.end(),scene.pool.Bytes().begin());
        } catch (const std::exception& error) { actual=std::string("예외: ")+error.what(); }
        const bool same=actual==row[5] && unchanged;
        CHECK(same);
        if (!same) {
            std::printf("  %s %s 행 %zu(장면 %s): 기대 %.200s / 실제 %.200s\n",edition,kind.c_str(),counts[kind],row[2].c_str(),row[5].c_str(),actual.c_str());
            if (++failures>=6) break;
        }
        ++counts[kind];
    }
    CHECK(failures==0 && counts["IslandPre"]==kRowsPerKind && counts["SurfacePost"]==kRowsPerKind);
}
}
// 세 PE는 같은 장면이라도 각 실행 파일에서 직접 얻은 관찰을 사용한다.
TEST_CASE(island_lifecycle_patch_x86_predestroy_and_postdestroy) { Replay("originals"); }
// CD 판본은 한 칸 탐색기가 인라인이고 지연 낙하 예약이 cdecl이다.
TEST_CASE(island_lifecycle_cd_x86_predestroy_and_postdestroy) { Replay("originalCD"); }
// 추가 10.37은 별도 PE 입력 SHA를 가진 관찰이다.
TEST_CASE(island_lifecycle_1037_x86_predestroy_and_postdestroy) { Replay("original1037"); }

// 훅 누락과 다른 풀의 탐색기는 생성 때 거부한다.
TEST_CASE(island_lifecycle_rejects_missing_hooks) {
    Scene scene(OriginalEdition::Patch1078);
    scene.Prepare({"50,91,0,0,1106247680,1106247680,3,3,671088643,16777216,8,2","-","0","-","82,1,0,0,1,2,3,4,5,6,7,8,1"});
    auto hooks=Hooks(scene);
    CHECK(Throws([&] { auto broken=hooks;broken.scheduleFall={};RawIslandLifecycle invalid(scene.pool,*scene.neighbors,broken); }));
    CHECK(Throws([&] { auto broken=hooks;broken.basePostDestroy={};RawIslandLifecycle invalid(scene.pool,*scene.neighbors,broken); }));
    Scene other(OriginalEdition::Patch1078);
    CHECK(Throws([&] { RawIslandLifecycle invalid(other.pool,*scene.neighbors,hooks); }));
    // 이웃이 없는 받침도 공통 preDestroy는 항상 부른다.
    RawIslandLifecycle lifecycle(scene.pool,*scene.neighbors,hooks);
    lifecycle.IslandPreDestroy(kSource,0x1234);
    CHECK(scene.Events()=="X:50:4660");
}

// 기록 훅 대신 실제 프로세스 계층에 잇는다: 섬 받침이 사라질 때 닿아 있던 살아 있는 다리에 지연 낙하(0x2692)가 예약되고,
// 다음 Kernel 프레임에 그 다리의 이벤트 처리기가 받침의 중심 칸을 포장한 payload로 불린다.
// 다리 이벤트 처리기 몸체와 공통 삭제는 연결하지 않은 통합 검사다.
TEST_CASE(island_lifecycle_real_process_schedules_bridge_falls) {
    // 실제 프로세스 form이 요구하는 원본 전체 슬롯 수다.
    constexpr std::uint32_t capacity=32768;
    // 실제 타입 번호: bridge 82, isle 97, island 94.
    constexpr std::uint32_t bridgeType=82,isleType=97,islandType=94;
    SidPool pool(OriginalEdition::Patch1078,capacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
    std::vector<RiftTypeRecord> types(188);
    types[bridgeType].flags1=0x00000802;types[bridgeType].flags2=0x00000004;types[bridgeType].footX=types[bridgeType].footY=1;
    types[isleType].flags1=0x28000803;types[isleType].flags2=0x00000002;types[isleType].footX=types[isleType].footY=1;
    types[islandType].flags1=0x28000003;types[islandType].flags2=0x01000000;types[islandType].footX=types[islandType].footY=3;
    // 합성 프레임 표: 번호 1이 K(동서), 2가 A(네 방향)다.
    std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));
    for (const auto number:{bridgeType,isleType,islandType}) frames[number]=Frames(0);
    // 객체를 직접 배치한다: 가상 표 기록값, 타입, 위치, 프레임, 해시 체인. 할당 직후의 void 상태는 지운다.
    const auto place=[&](std::uint32_t type,float x,float y,std::uint32_t frame,int level,std::uint8_t state) {
        const Sid sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);
        Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=state;
        Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));Put(raw,0x24,frame);
        auto& head=hash.Bucket(level,x,y);Put(raw,4,head,2);head=sid.value;
        return sid;
    };
    const Sid island=place(islandType,30.0f,30.0f,8,2,0);
    const Sid east=place(bridgeType,31.0f,29.0f,1,0,0);   // 살아 있는 동서 다리
    const Sid south=place(bridgeType,29.0f,31.0f,2,0,2);  // dead 다리: 예약하지 않는다
    place(isleType,27.0f,30.0f,2,0,0);                    // 다리가 아닌 표면 이웃: 예약하지 않는다
    SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
    Kernel kernel;SquidProcessState processState;processState.now=10.0;
    std::vector<std::string> handled;
    SquidProcessHost host(pool,types,kernel,destroy,processState,
        [&](Sid parent,std::uint32_t event,std::uint32_t,float payload) {
            handled.push_back(std::to_string(parent.value)+':'+std::to_string(event)+':'+std::to_string(static_cast<int>(payload)));
            return 0.0f;
        },SquidDestroyHooks{[](const SquidDestroyEvent&) {},[] { return Sid{}; },{}});
    RawSquidNeighbors neighbors(pool,hash,spots,types,frames);
    std::vector<std::string> order;
    RawIslandLifecycle lifecycle(pool,neighbors,IslandLifecycleHooks{
        [&](Sid bridge,float x,float y) { order.push_back("fall");CHECK(ScheduleBridgeFall(host,bridge,x,y)!=nullptr); },
        [&](Sid) { order.push_back("walker"); },
        [&](Sid,std::uint32_t) { order.push_back("pre"); },
        [&](Sid,std::uint32_t) { order.push_back("post"); }});
    // 0x1000이 있으면 다리를 건드리지 않고 공통 preDestroy만 부른다.
    lifecycle.IslandPreDestroy(island,kIslandDestroyKeepsBridges);
    CHECK(order==std::vector<std::string>({"pre"}) && !HasScheduledBridgeFall(host,east));
    // 없으면 살아 있는 다리에만 예약한 뒤 공통 preDestroy를 부른다.
    order.clear();lifecycle.IslandPreDestroy(island,0);
    CHECK(order==std::vector<std::string>({"fall","pre"}));
    CHECK(HasScheduledBridgeFall(host,east) && !HasScheduledBridgeFall(host,south));
    // 받침 (30,30)·3×3의 중심은 (28.5, 28.5)이고 payload는 (28 & 255) | (28 << 8) = 7196이다.
    kernel.RunFrame();
    CHECK(handled==std::vector<std::string>({std::to_string(east.value)+':'+std::to_string(kBridgeFallEvent)+":7196"}));
}
