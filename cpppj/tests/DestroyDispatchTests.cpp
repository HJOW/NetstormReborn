// 다리 vtable +0x10의 거부 조건·단락 평가·공통 몸체 직접 호출을 두 판본에서 검사한다.
#include "RawSceneSupport.h"
#include "o/RawBridgeEvents.h"
#include "o/RawSquidDestroyDispatch.h"
#include "o/RawGraph.h"
#include "o/SquidFactory.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 다리 타입과 raw free/void 비트다. 프레임은 보통 0·hard 1의 합성 입력이다.
constexpr std::uint32_t kBridge=82;
constexpr std::uint8_t kFree=1;
// 가상 판정을 검사하며 공통 pre/post 깊이만 완료하는 최소 삭제 장면이다.
struct DispatchScene {
    SidPool pool;SquidHash hash;std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::vector<RiftTypeRecord> types;std::vector<RiftTypeFrames> frames;std::vector<std::vector<FrameCode>> codes;
    std::array<GraphRecord,Graph::kTableSize> records{};BridgeDecayMode mode;
    std::unique_ptr<SquidUnpop> unpop;std::unique_ptr<RawSquidDestroy> base;
    std::unique_ptr<RawSquidNeighbors> neighbors;std::unique_ptr<RawGraph> graph;
    std::unique_ptr<RawSquidDestroyDispatch> dispatch;SquidDestroyHooks hooks;Sid source{};int calls{};
    // 유효 그래프 0의 signed WORD 수와 현재 프레임을 입력한다. 공간 등록은 판정과 별도다.
    DispatchScene(OriginalEdition edition,std::int16_t surfaces,bool hard,bool graphEnabled=true)
        :pool(edition,32768,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({})),codes(types.size()) {
        auto& type=types[kBridge];type.constructorAddress=TypeConstructorAddress(edition,kBridge);
        type.flags1=0x802;type.flags2=4;type.footX=type.footY=1;
        codes[kBridge]={{'K','P',1,0},{'K','P',2,0x40}};frames[kBridge]=RiftTypeFrames(codes[kBridge]);records[0]={surfaces,1,23};
        unpop=std::make_unique<SquidUnpop>(pool,hash,spots);base=std::make_unique<RawSquidDestroy>(pool,*unpop,types);
        neighbors=std::make_unique<RawSquidNeighbors>(pool,hash,spots,types,frames);
        graph=std::make_unique<RawGraph>(pool,hash,spots,types,codes,records);
        dispatch=std::make_unique<RawSquidDestroyDispatch>(*base,*neighbors,mode,graphEnabled ? graph.get() : nullptr);
        SquidFactory factory(pool,types,false,unpop.get());source=factory.Create(kBridge);
        Put(pool.AllocatedBytes(source),FrameOffset(),hard ? 1U : 0U,FrameWidth());pool.AllocatedBytes(source)[GraphOffset()]=0;
        // 공통 몸체가 넘긴 flags를 보존하고 두 깊이의 완료 계약을 수행한다.
        hooks.emit=[this](const SquidDestroyEvent& event) {
            ++calls;
            if (event.effect==SquidDestroyEffect::PreDestroy) { CHECK(event.flags==0x1234);base->CompletePreDestroy(); }
            else if (event.effect==SquidDestroyEffect::PostDestroy) base->CompletePostDestroy();
            else CHECK(event.effect==SquidDestroyEffect::Transmit);
        };
        hooks.selected=[] { return Sid{}; }; // 선택 UI는 이 판정 장면에서 비어 있다.
    }
    // 판본별 현재 프레임 필드 위치다.
    std::size_t FrameOffset() const { return pool.Edition()==OriginalEdition::Patch1078 ? 36 : 34; }
    // 판본별 현재 프레임 폭이다.
    std::size_t FrameWidth() const { return pool.Edition()==OriginalEdition::Patch1078 ? 4 : 1; }
    // 판본별 graph byte 위치다.
    std::size_t GraphOffset() const { return pool.Edition()==OriginalEdition::Patch1078 ? 30 : 28; }
    // 판본별 extra byte 위치다.
    std::size_t ExtraOffset() const { return pool.Edition()==OriginalEdition::Patch1078 ? 40 : 35; }
    // 거부 시 전체 풀·표·스택·콜백·깊이를 보존하고 전달 시 실제 SID 반납까지 확인한다.
    void Check(bool expected) {
        const auto before=Hex(pool.Bytes());const auto freeBefore=pool.FreeCount();const auto stack=std::vector<std::uint32_t>(graph->FloodStack().begin(),graph->FloodStack().end());
        CHECK(dispatch->Destroy(source,0x1234,hooks)==expected);
        // 인자 0x1234의 client 비트가 Transmit을 억제하므로 공통 pre/post 두 번만 호출된다.
        if (expected) CHECK((pool.Slot(source)[11]&kFree)!=0 && pool.FreeCount()==freeBefore+1 && calls==2);
        else CHECK(Hex(pool.Bytes())==before && pool.FreeCount()==freeBefore && calls==0);
        CHECK(graph->Records()[0].surfaces==records[0].surfaces && graph->Records()[0].reserved==23);
        CHECK(std::equal(stack.begin(),stack.end(),graph->FloodStack().begin()) && base->PreDepth()==0 && base->PostDepth()==0);
    }
};
}
// 4/5 경계·signed 음수·hard/보통·debugKeep·Graph 비활성을 각각 판정한다.
TEST_CASE(destroy_dispatch_bridge_threshold_hard_debug_and_disabled_graph) {
    // 패치 DWORD 프레임과 CD BYTE 프레임을 별도로 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        DispatchScene(edition,4,true).Check(true);DispatchScene(edition,5,true).Check(false);DispatchScene(edition,-1,true).Check(true);
        DispatchScene(edition,5,false).Check(true);DispatchScene debug(edition,5,false);debug.mode.debugKeep=true;debug.Check(false);
        DispatchScene(edition,5,true,false).Check(true);
    }
}
// editor/권한/abstract/buried는 무효 Graph와 프레임 조회를 모두 건너뛴다.
TEST_CASE(destroy_dispatch_short_circuits_graph_and_frame_reads) {
    // 같은 가상 표에서도 각 단락 조건의 생략을 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        // 네 종류의 첫 조건 실패를 각각 독립 장면으로 검사한다.
        for (int gate=0;gate<4;++gate) {
            DispatchScene scene(edition,5,true);scene.pool.AllocatedBytes(scene.source)[scene.GraphOffset()]=Graph::kInvalid;
            Put(scene.pool.AllocatedBytes(scene.source),scene.FrameOffset(),99,scene.FrameWidth());
            if (gate==0) scene.mode.editor=true;
            else if (gate==1) scene.mode.authority=false;
            else scene.pool.AllocatedBytes(scene.source)[scene.ExtraOffset()]|=static_cast<std::uint8_t>(gate==2 ? 1 : 8);
            scene.Check(true);
        }
        DispatchScene small(edition,4,true);Put(small.pool.AllocatedBytes(small.source),small.FrameOffset(),99,small.FrameWidth());small.Check(true);
    }
}
// 살아 있는 raw 프레임을 매번 읽고 모드/가상 표 변경과 공통 몸체 우회를 구별한다.
TEST_CASE(destroy_dispatch_reads_live_vtable_frame_and_keeps_direct_base_call) {
    // 두 판본의 원본 가상 표 기록값을 기준으로 분배한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        DispatchScene frame(edition,5,true);frame.Check(false);Put(frame.pool.AllocatedBytes(frame.source),frame.FrameOffset(),0,frame.FrameWidth());frame.Check(true);
        DispatchScene ordinary(edition,5,true);Put(ordinary.pool.AllocatedBytes(ordinary.source),0,kVtable);ordinary.Check(true);
        DispatchScene direct(edition,5,true);direct.base->Destroy(direct.source,0x1234,direct.hooks);CHECK((direct.pool.Slot(direct.source)[11]&kFree)!=0);
    }
}
// 무효 그래프/프레임은 원본 접근 오류 대신 쓰기 전 예외로 남긴다.
TEST_CASE(destroy_dispatch_invalid_graph_and_frame_preserve_raw_state) {
    // 단락 조건이 모두 통과할 때만 안전 거부가 적용된다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        DispatchScene scene(edition,5,true);scene.pool.AllocatedBytes(scene.source)[scene.GraphOffset()]=Graph::kInvalid;
        const auto before=Hex(scene.pool.Bytes());CHECK(Throws([&] { scene.dispatch->Destroy(scene.source,0x1234,scene.hooks); }));
        CHECK(Hex(scene.pool.Bytes())==before && scene.calls==0);
        scene.pool.AllocatedBytes(scene.source)[scene.GraphOffset()]=255;
        const auto badGraph=Hex(scene.pool.Bytes());CHECK(Throws([&] { scene.dispatch->Destroy(scene.source,0x1234,scene.hooks); }));
        CHECK(Hex(scene.pool.Bytes())==badGraph && scene.calls==0);
        scene.pool.AllocatedBytes(scene.source)[scene.GraphOffset()]=0;Put(scene.pool.AllocatedBytes(scene.source),scene.FrameOffset(),99,scene.FrameWidth());
        const auto badFrame=Hex(scene.pool.Bytes());CHECK(Throws([&] { scene.dispatch->Destroy(scene.source,0x1234,scene.hooks); }));
        CHECK(Hex(scene.pool.Bytes())==badFrame && scene.calls==0);
    }
}
// 서로 다른 풀/Graph 공간의 연결은 가상 삭제를 실행하기 전에 거부한다.
TEST_CASE(destroy_dispatch_rejects_mismatched_pool_and_graph_space) {
    DispatchScene first(OriginalEdition::Patch1078,5,true),second(OriginalEdition::Patch1078,5,true);
    CHECK(Throws([&] { RawSquidDestroyDispatch invalid(*first.base,*second.neighbors,first.mode); }));
    CHECK(Throws([&] { RawSquidDestroyDispatch invalid(*first.base,*first.neighbors,first.mode,second.graph.get()); }));
    SquidHash other;RawGraph otherGraph(first.pool,other,first.spots,first.types,first.codes);
    CHECK(Throws([&] { RawSquidDestroyDispatch invalid(*first.base,*first.neighbors,first.mode,&otherGraph); }));
}
