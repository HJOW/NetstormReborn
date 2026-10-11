// 실제 Kernel/form/종속 삭제로 건설 표시 몸체를 연결하는 통합 검사다. 게임/장치는 실행하지 않는다.
#include "ConstructionSupport.h"
#include "o/ConstructionVisualSystem.h"
#include "o/RawConstructionProcess.h"
#include "o/RawContainedLookup.h"
#include "client/ConstructionVisualRenderer.h"

using namespace netstorm::test::construction;
namespace {
// 통합 장면은 합성 타입 하나에 최대 HP 100을 공급한다. 실제 타입/SHP 로더 검사가 아니다.
std::vector<RiftTypeRecord> Types(OriginalEdition edition) {
    std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);
    types[100].flags1=0x10;types[100].maxHitPoints=100;types[100].footX=types[100].footY=1;return types;
}
// 실제 번호 할당과 공통 삭제를 공유하며 장치 경계는 출력/반납을 관찰한다.
struct World {
    bool patch;SidPool pool;SquidHash hash;std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::vector<RiftTypeRecord> types;SquidUnpop unpop;RawSquidDestroy destroy;Kernel kernel;SquidProcessState clock;
    SquidProcessHost host;GameRandom random;ConstructionVisualState visualState;ConstructionFrameState frameState;
    std::array<std::uint32_t,12> colors{0,1,2,3,4,5,6,7,8,9,10,11};ConstructionFramePalette palette{71,11,colors};
    std::vector<ConstructionRelease> released;std::vector<std::uint32_t> allocations;
    std::vector<ConstructionFrameRect> dirty;std::vector<std::pair<std::int32_t,std::int32_t>> draws;
    std::size_t captures{},maximumCalls{};Sid parent{};std::unique_ptr<ConstructionVisualSystem> system;
    // 부모와 form은 각각 서버/client 번호에서 실제로 할당한다. 삭제 장면의 자산 효과는 완료 통지만 처리한다.
    explicit World(OriginalEdition edition):patch(edition==OriginalEdition::Patch1078),pool(edition,kCapacity,true),types(Types(edition)),
        unpop(pool,hash,spots),destroy(pool,unpop,types),host(pool,types,kernel,destroy,clock,{},AssetHooks()) {
        parent=pool.Allocate(0);auto raw=pool.AllocatedBytes(parent);
        Put(raw,0,patch ? 0x501dd0 : 0x5058d8);raw[10]=100;raw[11]=0;raw[patch ? 34 : 32]=1;raw[patch ? 35 : 33]=255;raw[patch ? 40 : 35]=1;
        Put(raw,14,Bits(20.5F));Put(raw,18,Bits(21.25F));Put(raw,26,0);visualState.palette=71;frameState.now=2.0;
    }
    // 부모 삭제가 form 삭제와 같은 카운터를 공유하는지 검사한다. 공간 자산 효과는 이 검사의 대상이 아니다.
    SquidDestroyHooks AssetHooks() {
        SquidDestroyHooks hooks;hooks.selected=[] { return Sid{}; };
        hooks.emit=[this](const SquidDestroyEvent& event) {
            if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();
            if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();
        };return hooks;
    }
    // 원본 목록의 32비트 확보 크기를 기록한다. 부착은 system이 실제 host로 대체한다.
    RenderRegistryHooks RegistryHooks() {
        RenderRegistryHooks hooks;hooks.reserve=[this](std::uint32_t bytes) { allocations.push_back(bytes);return true; };
        hooks.releaseArray=[] {};return hooks;
    }
    // 실제 부착이 캡처보다 먼저 끝났고 생성 중 객체가 목록과 Kernel 양쪽에 존재하는지 검사한다.
    ConstructionVisualHooks VisualHooks() {
        ConstructionVisualHooks hooks;hooks.reserve=[](std::uint32_t) { return true; };hooks.createPalette=[] { return 71U; };
        hooks.gradient=[](std::uint32_t,std::array<double,3>,std::array<double,3>,double) {};hooks.buildPalette=[](std::uint32_t) {};
        hooks.releaseParticles=[] {};
        hooks.capture=[this](Sid sid) {
            CHECK(sid==parent && system->Size()>0 && kernel.Size()>0);
            const Sid form{static_cast<std::uint16_t>(Get(pool.Slot(parent),6,2))};
            auto* process=dynamic_cast<ConstructionVisualProcess*>(host.FromForm(form));CHECK(process && pool.Slot(form)[10]==17);
            if (process) CHECK(system->Slots()[process->Registration().slot]==process->Registration().token);
            ++captures;return ConstructionVisualImage{5,7,2,3,std::vector<std::uint8_t>(35,17)};
        };return hooks;
    }
    // HP/dirty/Draw는 호출 인자와 픽셀을 읽기만 한다. kill은 system 내부에서 host에 연결한다.
    ConstructionFrameHooks FrameHooks() {
        ConstructionFrameHooks hooks;hooks.maxHitPoints=[this](Sid sid) { CHECK(sid==parent);++maximumCalls;return 100; };
        hooks.invalidate=[this](ConstructionFrameRect rectangle) { dirty.push_back(rectangle); };
        hooks.draw=[this](const ConstructionFrameDraw& request) {
            CHECK(request.pitch==5 && request.clip.right==5 && request.clip.bottom==7 && request.pixels.size()==35);
            draws.emplace_back(request.x,request.y);
        };return hooks;
    }
    // 시스템을 마지막에 소유해 host/Kernel/팔레트보다 먼저 종료한다.
    void CreateSystem(ConstructionVisualHooks visual,ConstructionFrameHooks frame,RenderRegistryHooks registry) {
        system=std::make_unique<ConstructionVisualSystem>(pool,host,visualState,frameState,palette,random,
            std::move(registry),std::move(visual),std::move(frame),[this](ConstructionRelease kind) { released.push_back(kind); });
    }
    // 기본 관찰 경계를 사용하는 시스템이다.
    void CreateSystem() { CreateSystem(VisualHooks(),FrameHooks(),RegistryHooks()); }
    // 부모 HP의 판본별 폭을 쓴다.
    void Hp(std::uint32_t value) { Put(pool.AllocatedBytes(parent),26,value,patch ? 4 : 2); }
    // 완료 소리/공간 효과는 명시 경계로 남기고 진행/완료/form 몸체만 연결한다.
    ConstructionProcessHooks BuildHooks() {
        ConstructionProcessHooks hooks;hooks.reserve=[] { return true; };hooks.loadSound=[](std::string_view) { return 0U; };
        hooks.reserveSound=[] { return false; };hooks.attachSound=[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) {};
        hooks.refresh=[] {};hooks.maxHitPoints=[](Sid) { return 100; };hooks.updateDisplay=[](Sid) {};
        hooks.playAt=[](float,float,std::string_view,std::uint32_t) {};hooks.readySound=[](std::uint32_t) { return 0U; };hooks.play=[](std::uint32_t) {};
        hooks.unpop=[this](Sid sid,std::uint32_t) { pool.AllocatedBytes(sid)[11]|=4; };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { CHECK(flags==9);pool.AllocatedBytes(sid)[11]&=0xfb; };
        hooks.push=[](float,float,std::uint32_t,std::uint32_t) {};return MakeConstructionVisualSystemHooks(pool,*system,std::move(hooks));
    }
};
// 각 완성 객체는 소유 버퍼 세 개와 객체 자체를 차례로 반납한다.
bool Released(const std::vector<ConstructionRelease>& releases,std::size_t count) {
    if (releases.size()!=count*4) return false;
    // 반복된 네 단계가 모두 같은 원본 순서인지 비교한다.
    for (std::size_t i=0;i<releases.size();i+=4)
        if (releases[i]!=ConstructionRelease::Source || releases[i+1]!=ConstructionRelease::Output ||
            releases[i+2]!=ConstructionRelease::Particles || releases[i+3]!=ConstructionRelease::Object) return false;
    return true;
}
}

// 두 판본/생성 방식에서 실제 client form·Kernel 실행·현재 카메라 Draw·자기 삭제와 빈 칸 재사용을 검사한다.
TEST_CASE(ConstructionVisualSystem_AttachRunDrawKillAndReuse) {
    // 공유 팔레트/실제 체인은 판본마다 준비한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        World world(edition);world.CreateSystem();
        // 즉시/일반 건설은 같은 Kernel/소멸 경로를 사용한다.
        for (const bool instant:{false,true}) {
            auto* process=world.system->Add(world.parent,instant);const auto form=process->Form();const auto token=process->Registration().token;
            CHECK(form.value>=5 && form.value<world.pool.Layout().serverFirst && process->Parent()==world.parent);
            CHECK(world.host.FromForm(form)==process && world.kernel.Size()==1 && process->Registration().form==form.value);
            CHECK(process->Registration().slot==0 && process->Registration().zOrder==-1 && process->Fields().visual.instant==instant);
            CHECK((world.pool.Slot(form)[11]&12)==8 && Get(world.pool.Slot(form),16,2)==world.parent.value);
            world.Hp(50);world.kernel.RunFrame();CHECK(process->Fields().visual.lastTime==world.frameState.now && !world.dirty.empty());
            CHECK(process->Registration().left==process->Fields().visual.left && process->Registration().right==process->Fields().visual.right);
            world.draws.clear();world.system->Draw();CHECK(world.draws.size()==1 && world.draws[0]==std::make_pair(326,230));
            world.frameState.now+=0.1;world.frameState.cameraX=4.0F;world.frameState.cameraY=5.0F;world.kernel.RunFrame();world.draws.clear();world.system->Draw();
            CHECK(world.draws.size()==1 && world.draws[0]==std::make_pair(322,225));
            world.pool.AllocatedBytes(world.parent)[world.patch ? 40 : 35]=0;world.kernel.RunFrame();
            CHECK(world.kernel.Size()==0 && world.system->Size()==0 && world.system->Slots()[0]==0 && (world.pool.Slot(form)[11]&1));
            CHECK(world.system->Slots().size()==40 && world.visualState.palette==71 && token!=0);
            world.pool.AllocatedBytes(world.parent)[world.patch ? 40 : 35]=1;world.frameState.cameraX=world.frameState.cameraY=0;
        }
        CHECK(Released(world.released,2) && world.allocations==std::vector<std::uint32_t>{160});
    }
}

// Start는 표시를 먼저 붙이고 그 뒤 HP 진행을 붙인다. 완료의 abstract 해제 뒤 다음 Kernel 순회에서 표시가 종료된다.
TEST_CASE(ConstructionVisualSystem_StartRunsBothProcessesToCompletion) {
    // 두 판본과 기본 10초/지정 8초 건설을 같은 실제 커널로 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        World world(edition);world.CreateSystem();RawContainedLookup contained(world.pool,[&](Sid form,std::uint32_t flags) {
            auto* process=world.host.FromForm(form);CHECK(process);if (process) world.host.Kill(*process,flags);
        });
        // 표시 타입 17과 HP 진행 타입 30의 생성 순서도 검사한다.
        for (const auto duration:{0.0F,8.0F}) {
            ConstructionProcessState state;state.now=2.0;
            auto hooks=MakeContainedConstructionHooks(world.pool,contained,world.BuildHooks());
            RawConstructionProcess build(world.pool,world.types,state,std::move(hooks),&world.host);build.Start(world.parent,duration);
            auto* visual=dynamic_cast<ConstructionVisualProcess*>(world.kernel.Get(1));auto* progress=dynamic_cast<ConstructionProcess*>(world.kernel.Get(2));
            CHECK(world.kernel.Size()==2 && visual && progress);if (!visual || !progress) return;
            CHECK(visual->Fields().visual.instant==(duration==0.0F) && Get(world.pool.Slot(progress->Form()),4,2)==visual->Form().value);
            const auto end=duration==0.0F ? 12.0 : 10.0;world.frameState.now=state.now=6.0;world.kernel.RunFrame();
            CHECK(Get(world.pool.Slot(world.parent),26,world.patch ? 4 : 2)==(duration==0.0F ? 40U : 50U));
            world.frameState.now=state.now=end;world.kernel.RunFrame();CHECK(world.kernel.Size()==1 && world.system->Size()==1);
            CHECK(!(world.pool.Slot(world.parent)[world.patch ? 40 : 35]&1));world.kernel.RunFrame();
            CHECK(world.kernel.Size()==0 && world.system->Size()==0 && Get(world.pool.Slot(world.parent),6,2)==0);
            world.Hp(0);world.pool.AllocatedBytes(world.parent)[world.patch ? 40 : 35]=1;
        }
        CHECK(Released(world.released,2));
    }
}

// 실제 contained 조회가 가장 최근 표시만 지우며 다른 표시/Regular/부모 링크는 보존한다.
TEST_CASE(ConstructionVisualSystem_ContainedRemovalPreservesOtherProcesses) {
    World world(OriginalEdition::Patch1078);world.CreateSystem();auto* first=world.system->Add(world.parent,false);const auto firstForm=first->Form();
    auto* second=world.system->Add(world.parent,true);const auto secondForm=second->Form();auto* regular=world.host.AddRegular(world.parent,1,1.0F);
    RawContainedLookup contained(world.pool,[&](Sid sid,std::uint32_t flags) { CHECK(flags==0);world.host.Kill(*world.host.FromForm(sid),flags); });
    CHECK(contained.Find(world.parent,17)==secondForm);static_cast<void>(contained.Find(world.parent,17,0,true));
    CHECK(world.kernel.Size()==2 && world.system->Size()==1 && world.system->Slots()[0]!=0 && world.system->Slots()[1]==0);
    CHECK((world.pool.Slot(secondForm)[11]&1) && Get(world.pool.Slot(regular->Form()),4,2)==firstForm.value);
    static_cast<void>(contained.Find(world.parent,17,0,true));CHECK(world.kernel.Size()==1 && world.system->Size()==0 && Released(world.released,2));
    world.host.Kill(*regular,0);CHECK(Get(world.pool.Slot(world.parent),6,2)==0);
}

// 부모가 지워질 때 공통 종속 삭제→Kernel 소멸→버퍼/등록 제거→form 반납을 모두 거친다.
TEST_CASE(ConstructionVisualSystem_ParentDestructionReleasesAllVisuals) {
    // 부모의 공간 Unpop은 별도 복원 범위이므로 이미 void인 부모의 공통 삭제를 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        World world(edition);world.CreateSystem();const auto first=world.system->Add(world.parent,false)->Form();const auto second=world.system->Add(world.parent,true)->Form();
        world.pool.AllocatedBytes(world.parent)[11]|=4;world.destroy.Destroy(world.parent,0,world.host.Hooks());
        CHECK(world.kernel.Size()==0 && world.system->Size()==0 && world.system->Slots()[0]==0 && world.system->Slots()[1]==0);
        CHECK((world.pool.Slot(first)[11]&1) && (world.pool.Slot(second)[11]&1) && (world.pool.Slot(world.parent)[11]&1));
        CHECK(world.destroy.PreDepth()==0 && world.destroy.PostDepth()==0 && Released(world.released,2));
    }
}

// 어댑터가 먼저 끝나도 살아 있는 host/Kernel에 dangling system 참조나 form을 남기지 않는다.
TEST_CASE(ConstructionVisualSystem_ShutdownDeletesRemainingForms) {
    World world(OriginalEdition::Patch1078);world.CreateSystem();const auto first=world.system->Add(world.parent,false)->Form();const auto second=world.system->Add(world.parent,true)->Form();
    world.system.reset();CHECK(world.kernel.Size()==0 && Get(world.pool.Slot(world.parent),6,2)==0 && Released(world.released,2));
    CHECK((world.pool.Slot(first)[11]&1) && (world.pool.Slot(second)[11]&1) && world.visualState.palette==71);
}

// 부착 전/후 실패는 기존 표시를 보존하고 새 Kernel/form/표시 칸을 정리한다.
TEST_CASE(ConstructionVisualSystem_FailedCreationRollsBackRegistrationAndForm) {
    World world(OriginalEdition::Patch1078);bool failObject=false,failCapture=false;auto hooks=world.VisualHooks();auto capture=hooks.capture;
    hooks.reserve=[&](std::uint32_t bytes) { return !(failObject && bytes==136); };
    hooks.capture=[&](Sid sid) { if (failCapture) throw std::runtime_error("캡처 실패 입력");return capture(sid); };
    world.CreateSystem(hooks,world.FrameHooks(),world.RegistryHooks());auto* alive=world.system->Add(world.parent,false);const auto form=alive->Form();
    failObject=true;CHECK(Throws([&] { world.system->Add(world.parent,false); }));failObject=false;
    failCapture=true;CHECK(Throws([&] { world.system->Add(world.parent,false); }));failCapture=false;
    world.visualState.processType=100;CHECK(Throws([&] { world.system->Add(world.parent,false); }));world.visualState.processType=17;
    CHECK(world.kernel.Size()==1 && world.system->Size()==1 && world.host.FromForm(form)==alive && world.system->Slots()[1]==0);
    CHECK(Get(world.pool.Slot(world.parent),6,2)==form.value);auto* next=world.system->Add(world.parent,true);CHECK(next->Registration().slot==1 && world.system->Size()==2);
}

// 캡처 중 Kernel/Draw 호출은 아직 생성 중인 객체를 실행하지 않고, 중첩 생성 거부 뒤 다음 생성은 정상이다.
TEST_CASE(ConstructionVisualSystem_CreationReentrySkipsIncompleteBuffers) {
    World world(OriginalEdition::Patch1078);auto hooks=world.VisualHooks();auto capture=hooks.capture;
    hooks.capture=[&](Sid sid) {
        const auto calls=world.maximumCalls;const auto draws=world.draws.size();world.kernel.RunFrame();world.system->Draw();
        CHECK(world.maximumCalls==calls && world.draws.size()==draws);
        std::array<RenderCollectionEntry,1> work{};CHECK(world.system->Collect({-1000,-1000,1000,1000},work,1)==0);
        CHECK(Throws([&] { world.system->Add(world.parent,false); }));return capture(sid);
    };world.CreateSystem(hooks,world.FrameHooks(),world.RegistryHooks());world.system->Add(world.parent,false);world.kernel.RunFrame();
    CHECK(world.maximumCalls==1 && world.system->Size()==1);
}

// Draw 콜백이 현재 객체를 지우고 목록을 40→80칸으로 늘려도 해제된 span/객체를 재사용하지 않는다.
TEST_CASE(ConstructionVisualSystem_DrawDeletionAndListGrowth) {
    World world(OriginalEdition::Patch1078);auto hooks=world.FrameHooks();bool changed=false;std::size_t draws=0;ConstructionVisualProcess* first=nullptr;
    hooks.draw=[&](const ConstructionFrameDraw& request) {
        CHECK(request.pixels.size()==35);++draws;if (changed) return;changed=true;world.host.Kill(*first,0);
        // 이미 지난 슬롯 0은 다음 Draw, 이후 슬롯은 같은 Draw에서 제출되는 어댑터 순서다.
        for (std::size_t i=0;i<41;++i) world.system->Add(world.parent,false);
    };world.CreateSystem(world.VisualHooks(),hooks,world.RegistryHooks());first=world.system->Add(world.parent,false);world.system->Draw();
    CHECK(draws==41 && world.system->Size()==41 && world.system->Slots().size()==80 && world.allocations==std::vector<std::uint32_t>({160,320}));
    world.system->Draw();CHECK(draws==82 && Released(world.released,1));world.system.reset();CHECK(world.kernel.Size()==0 && Released(world.released,42));
}

// 실제 프레임이 갱신한 영역을 선별하고 signed 깊이로 정렬해 현재 카메라 Draw에 제출한다.
TEST_CASE(RenderCollection_VisibleConstructionSubmission) {
    // 두 판본에서 같은 host/Kernel/collector/그리기 경로를 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        World world(edition);world.CreateSystem();auto* first=world.system->Add(world.parent,false);
        world.pool.AllocatedBytes(world.parent)[world.patch ? 35 : 33]=7;auto* second=world.system->Add(world.parent,false);
        world.pool.AllocatedBytes(world.parent)[world.patch ? 35 : 33]=0;auto* third=world.system->Add(world.parent,false);
        world.Hp(50);world.kernel.RunFrame();std::array<RenderCollectionEntry,3> work{};
        const RenderWorldRect view{20,20,22,23};
        CHECK(netstorm::client::DrawVisibleConstruction(*world.system,view,work)==3 && world.draws.size()==3);
        CHECK(work[0].token==second->Registration().token && work[1].token==third->Registration().token && work[2].token==first->Registration().token);
        CHECK(work[0].depth==7 && work[1].depth==0 && work[2].depth==-1 && work[0].kind==3);
        world.draws.clear();CHECK(netstorm::client::DrawVisibleConstruction(*world.system,{100,100,110,110},work)==0 && world.draws.empty());
        const auto right=first->Registration().right/world.visualState.scaleX;
        CHECK(netstorm::client::DrawVisibleConstruction(*world.system,{right,20,right+1,23},work)==0 && world.draws.empty());
        world.frameState.cameraX=4;world.frameState.cameraY=5;
        CHECK(netstorm::client::DrawVisibleConstruction(*world.system,view,std::span(work).first(2))==2 && world.draws.size()==2);
        CHECK(work[0].token==second->Registration().token && work[1].token==first->Registration().token && world.draws[0]==std::make_pair(322,225));
        world.draws.clear();CHECK(netstorm::client::DrawVisibleConstruction(*world.system,view,work,false)==3);
        CHECK(work[0].token==first->Registration().token && work[1].token==second->Registration().token && work[2].token==third->Registration().token);
    }
}

// 수집 후 삭제된 토큰은 건너뛰고 재사용된 슬롯의 새 객체를 이전 목록에 섞지 않는다.
TEST_CASE(RenderCollection_SnapshotDeletionAndReuse) {
    World world(OriginalEdition::Patch1078);world.CreateSystem();auto* first=world.system->Add(world.parent,false);auto* second=world.system->Add(world.parent,false);
    std::array<RenderCollectionEntry,3> work{};CHECK(world.system->Collect({20,20,22,23},work,3)==2);const auto oldToken=work[0].token;
    world.host.Kill(*first,0);auto* replacement=world.system->Add(world.parent,false);
    CHECK(replacement->Registration().slot==0 && replacement->Registration().token!=oldToken);
    world.system->DrawCollected(std::span(work).first(2));CHECK(world.draws.size()==1 && work[1].token==second->Registration().token);
    world.draws.clear();work[0].kind=2;CHECK(Throws([&] { world.system->DrawCollected(std::span(work).first(1)); }));CHECK(world.draws.empty());
}

// 동률은 호스트에서 안정 순서를 유지하고 비유한 정렬 좌표는 그리기 전에 거부한다.
TEST_CASE(RenderCollection_StableTiesAndInvalidSortCoordinates) {
    World world(OriginalEdition::Patch1078);world.CreateSystem();auto* first=world.system->Add(world.parent,false);auto* second=world.system->Add(world.parent,false);
    std::array<RenderCollectionEntry,2> work{};const RenderWorldRect view{20,20,22,23};
    CHECK(netstorm::client::DrawVisibleConstruction(*world.system,view,work)==2);
    CHECK(work[0].token==first->Registration().token && work[1].token==second->Registration().token);
    world.draws.clear();Put(world.pool.AllocatedBytes(world.parent),14,0x7fc00000);
    CHECK(Throws([&] { netstorm::client::DrawVisibleConstruction(*world.system,view,work); }));CHECK(world.draws.empty());
}

// 서로 다른 풀이나 누락된 장치 경계는 생성 전에 거부한다. 캡처 중 취소된 객체도 다시 접근하지 않는다.
TEST_CASE(ConstructionVisualSystem_RejectsForeignPoolAndHandlesCaptureCancellation) {
    World world(OriginalEdition::Patch1078);World other(OriginalEdition::Patch1078);auto frame=world.FrameHooks();frame.draw={};
    CHECK(Throws([&] { world.CreateSystem(world.VisualHooks(),frame,world.RegistryHooks()); }));
    CHECK(Throws([&] { ConstructionVisualSystem invalid(other.pool,world.host,world.visualState,world.frameState,world.palette,world.random,
        world.RegistryHooks(),world.VisualHooks(),world.FrameHooks(),[](ConstructionRelease) {}); }));
    auto visual=world.VisualHooks();bool cancel=true;visual.capture=[&](Sid) {
        if (cancel) { const Sid form{static_cast<std::uint16_t>(Get(world.pool.Slot(world.parent),6,2))};world.host.Kill(*world.host.FromForm(form),0); }
        return ConstructionVisualImage{5,7,2,3,std::vector<std::uint8_t>(35,17)};
    };world.CreateSystem(visual,world.FrameHooks(),world.RegistryHooks());CHECK(Throws([&] { world.system->Add(world.parent,false); }));
    CHECK(world.kernel.Size()==0 && world.system->Size()==0 && world.system->Slots()[0]==0);
    cancel=false;world.system->Add(world.parent,false);CHECK(world.kernel.Size()==1 && world.system->Size()==1);
    CHECK(Throws([&] { MakeConstructionVisualSystemHooks(other.pool,*world.system,{}); }));
}
