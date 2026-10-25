// 실제 World/Pool/Kernel로 장기 Host의 현재 처리기 대여와 Native 단일 Clear 잔류를 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawSurfaceWorld.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "o/RawGameAuxState.h"
#include <array> // free/dead/void의 세 BYTE 입력을 정확한 폭으로 구성한다.
#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <type_traits>
#include <vector>

namespace { // 이 번역 단위의 명시 입력과 수명 관찰기만 소유한다.
using namespace netstorm::client;using namespace netstorm::o; // 기존 실제 제품의 타입을 그대로 사용한다.
static_assert(std::is_nothrow_move_constructible_v<RawProcessHostStorage::Returned> && std::is_nothrow_move_constructible_v<RawProcessHostStorage::ClearPass>); // 실제 반납 소비 뒤 증빙 이동이 새 예외 경계가 되지 않는다.
// 원본 기본 표면 TYPE를 같은 물리에서 사용한다.
BridgeConnectState HostStorageLinks() { BridgeConnectState value;value.bridgeType=82;value.islandType=94;value.stalagType=95;value.connectorType=155;value.noIslandType=157;return value; }
// 같은 실제 Client/독립 Storage에 World 처리기만 한 번 빌린다.
std::unique_ptr<RawSurfaceWorld> HostStorageWorld(const std::shared_ptr<RawSurfaceStorage>& storage,bool prepared=false) {
    return std::make_unique<RawSurfaceWorld>(storage->pool.Edition(),storage->types,storage->frames,storage->shapes,HostStorageLinks(),97,nullptr,storage,prepared);
}
// 실제 Allocate 뒤 현재 슬롯에 Native fixture와 같은 부모 종류/상태 입력만 넣는다.
Sid HostStorageParent(SidPool& pool) { const auto parent=pool.Allocate(2);auto raw=pool.AllocatedBytes(parent);raw[10]=82;raw[11]=0;return parent; }
// 실제 Kernel 가상 소멸자에서만 외부 효과를 수행하는 순서 관찰기다.
class HostStorageFinalAction final:public BaseProcess {
public:
    explicit HostStorageFinalAction(std::function<void()> action):action_(std::move(action)) {} // 같은 명시 외부 효과의 수명을 소유한다.
    ~HostStorageFinalAction() override { action_(); } // Kernel.Remove/Clear가 실제로 삭제할 때 한 번만 실행한다.
    void RunFrame() override {} // 이 관찰기는 게임 계산이나 새 상태를 만들지 않는다.
private:
    std::function<void()> action_; // 소멸 시 같은 Host/Pool 생존을 검사하는 외부 장치다.
};
// 아직 장기 logic 이행을 하지 않은 다른 파생의 반납 거부를 관측하는 실제 Host 부착 객체다.
class HostStorageOtherProcess final:public SquidProcess {
public:
    explicit HostStorageOtherProcess(std::function<void()> action={}):action_(std::move(action)) {} // 실제 최종 소멸을 관측할 때만 외부 장치를 보관한다.
    ~HostStorageOtherProcess() override { if (action_) action_(); } // 실제 자기 binding 종료의 가상 소멸 효과다.
    void RunFrame() override {} // 이 검사는 실행 결과가 아닌 현재 대여/잔류 소유권만 관측한다.
private:
    std::function<void()> action_; // 일반 종료 중 현재 대여의 반납 재진입만 관찰한다.
};
// 가상 통지에서 자신을 먼저 제거한 뒤에도 현재 호출의 대여가 남아 있어야 한다.
class HostStorageNotifyProcess final:public SquidProcess {
public:
    HostStorageNotifyProcess(Kernel& kernel,std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> action):kernel_(&kernel),action_(std::move(action)) {} // 실제 같은 Kernel과 통지 관찰기만 빌린다.
    void RunFrame() override {} // 통지 검사에서 별도 게임 효과를 만들지 않는다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override {
        auto action=action_;auto* kernel=kernel_;const auto id=Pid();kernel->Remove(id);action(event,info,value); // 자기 삭제 뒤 멤버에 접근하지 않고 외곽 가상 호출의 대여만 관측한다.
    }
private:
    Kernel* kernel_; // 부착한 실제 Kernel이며 이 객체보다 오래 산다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> action_; // 자기 삭제 전에 동기 지역 값으로 복사할 외부 관찰기다.
};
// 선택한 계약 실패만 관찰하며 앞선 물리를 복구하지 않는다.
template<class Function> bool HostStorageRejects(Function&& operation) { try { operation();return false; }catch (const std::exception&) { return true; } }
}

// 지난 슬롯 재등록은 같은 Clear에서 남고 old World 반납/Pool reset 뒤에도 같은 PID를 유지한다.
TEST_CASE(ProcessHostStorage_RegularAndSharedSurviveNativeClearAndUseNewWorldHandler) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const bool shared:{false,true}) { // 같은 Native Regular 몸체를 사용하는46/61 두 factory를 각각 검증한다.
        auto storage=MakeRawSurfaceStorage(assets.game);auto world=HostStorageWorld(storage);auto core=world->Core();const auto parent=HostStorageParent(core.pool);auto* host=&core.processes;auto* state=&core.processState;auto* kernel=&core.kernel;RegularProcess* residual{};std::uint32_t calls{};
        core.processState.now=5.25;CHECK(&storage->Processes().State()==state && &storage->Processes().Host()==host && &storage->Processes().Kernel()==kernel);
        CHECK(kernel->Add(std::make_unique<HostStorageFinalAction>([] {}))==1);CHECK(kernel->Add(std::make_unique<HostStorageFinalAction>([&] { residual=shared ? host->AddSharedRegular(parent,0x12345678U,9.5F) : host->AddRegular(parent,0x12345678U,9.5F); }))==2);
        auto pass=world->ClearProcessesForState();CHECK(residual && kernel->Find(*residual)==1 && kernel->Size()==1);if (!residual) return;const auto rotation=state->typeRotation;
        world->ReturnProcessBindingsForState(std::move(pass));CHECK(world->ProcessBindingsReturned());world.reset();CHECK(kernel->Get(1)==residual && storage->Processes().State().typeRotation==rotation && !storage->Borrowed());
        storage->pool.Reset(0);kernel->RunFrame();CHECK(kernel->Get(1)==residual && residual->Count()==0 && residual->Time()==5.25); // free parent에서는 현재 World 조회/삭제가 모두 자연 생략된다.
        { auto next=HostStorageWorld(storage,true);auto current=next->Core();CHECK(&current.processes==host && &current.processState==state && &current.kernel==kernel);const auto reused=HostStorageParent(current.pool);CHECK(reused==parent);
          current.processState.now=11.0;current.processes.SetRegularHandler([&](Sid actual,std::uint32_t event,std::uint32_t count,float payload) { CHECK(actual==parent && event==0x12345678U);if (!calls) CHECK(count==0 && payload==9.5F);++calls;return 1.25F; });kernel->RunFrame();
          CHECK(calls==1 && kernel->Get(1)==residual && residual->Count()==1 && residual->Payload()==1.25F && residual->Time()==12.25 && state->typeRotation==rotation);
          const auto flags=current.types[82].flags2;auto raw=current.pool.AllocatedBytes(parent);raw[40]|=1;next->MutableTypes()[82].flags2&=~0x404200U;current.processState.now=13;kernel->RunFrame();CHECK(calls==1 && residual->Count()==1);next->MutableTypes()[82].flags2|=0x404200U;kernel->RunFrame();CHECK(calls==2 && residual->Count()==2 && residual->Time()==14.25);next->MutableTypes()[82].flags2=flags;raw[40]&=0xfe; } // 같은 장기 Host가 실제 가변 TYPE API로 바꾼 현재값을 관문에서 읽는다.
        CHECK(kernel->Size()==1 && kernel->Get(1)==residual && !storage->Borrowed()); // 새 독립 World의 최종 fallback은 이전 Native 대여의 잔류를 추가 삭제하지 않는다.
    }
}

// free/dead/void와 예약 시각의 기존 관문은 현재 handler를 요구하는 위치보다 앞에 남는다.
TEST_CASE(ProcessHostStorage_InvalidParentAndPendingTimeDoNotRequireRetiredRuntime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);auto world=HostStorageWorld(storage);auto core=world->Core();const auto parent=HostStorageParent(core.pool);RegularProcess* residual{};
    core.kernel.Add(std::make_unique<HostStorageFinalAction>([] {}));core.kernel.Add(std::make_unique<HostStorageFinalAction>([&] { residual=core.processes.AddRegular(parent,71,3.0F); }));auto pass=world->ClearProcessesForState();CHECK(residual && core.kernel.Find(*residual)==1);if (!residual) return;world->ReturnProcessBindingsForState(std::move(pass));world.reset();
    for (const std::uint8_t state:std::array<std::uint8_t,3>{1,2,4}) { // Native가 현재 BYTE 상태를 각 Run에서 다시 읽는다는 입력이다.
        auto bytes=const_cast<std::uint8_t*>(storage->pool.Slot(parent).data());bytes[11]=state;storage->Processes().Kernel().RunFrame();CHECK(storage->Processes().Kernel().Get(1)==residual && residual->Count()==0);
    }
    auto bytes=const_cast<std::uint8_t*>(storage->pool.Slot(parent).data());bytes[11]=0;residual->SetNextTime(100);storage->Processes().State().now=3;storage->Processes().Kernel().RunFrame();CHECK(residual->Time()==100 && residual->Count()==0);
    storage->Processes().State().now=101;CHECK(HostStorageRejects([&] { storage->Processes().Kernel().RunFrame(); }));CHECK(storage->Processes().Kernel().Get(1)==residual && residual->Count()==0); // 유효 부모의 선택된 현재 가상 사건을 빈 성공으로 바꾸지 않는다.
}

// 같은 Clear 증빙을 다른 물리/World에 사용하면 실제 현재 바인딩과 PID를 그대로 남긴다.
TEST_CASE(ProcessHostStorage_ClearPassAndDeviceIdentityRejectForeignOwners) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto first=MakeRawSurfaceStorage(assets.game),second=MakeRawSurfaceStorage(assets.game);auto firstWorld=HostStorageWorld(first),secondWorld=HostStorageWorld(second);const auto firstCore=firstWorld->Core(),secondCore=secondWorld->Core();
    CHECK(&firstCore.kernel!=&secondCore.kernel && &firstCore.processes!=&secondCore.processes && &firstCore.processState!=&secondCore.processState);firstCore.processState.now=7;secondCore.processState.now=19;CHECK(firstCore.processState.Now()==7 && secondCore.processState.Now()==19);
    auto pass=firstWorld->ClearProcessesForState();CHECK(HostStorageRejects([&] { secondWorld->ReturnProcessBindingsForState(std::move(pass)); }));CHECK(!firstWorld->ProcessBindingsReturned() && !secondWorld->ProcessBindingsReturned());firstWorld->ReturnProcessBindingsForState(std::move(pass));CHECK(firstWorld->ProcessBindingsReturned());
    auto foreign=std::make_shared<ProcessHostRuntimeBindings>();foreign->destroy=&secondCore.destroy;CHECK(HostStorageRejects([&] { first->Processes().Bind(foreign); }));CHECK(HostStorageRejects([&] { second->Processes().BindDevices(std::make_shared<Kernel>(),std::make_shared<FrameTime>()); }));CHECK(secondCore.processState.Now()==19 && second->Borrowed());
}

// 실제 Client의 동일 Kernel/FrameTime/Authority와441의 동일 TYPE owner를 직접 빌린다.
TEST_CASE(ProcessHostStorage_ActualClientClockAndAuthorityAreCurrentWithoutSnapshotBackWrites) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
    storage->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());client.FrameClockOwner()->game=31.75;auto world=HostStorageWorld(storage);auto core=world->Core();CHECK(&core.kernel==&client.GetKernel() && &core.processState.Now()==&client.Time().game && &core.processState.Server()==&client.GameplayGlobals()->Server() && &core.processState.PollType()==&client.GameplayGlobals()->GameAux()->GeyserType());
    world->PrepareFrame({.game=3.0,.number=1});CHECK(client.Time().game==31.75 && core.processState.Now()==31.75);client.FrameClockOwner()->game=40.5;world->PrepareFrame({.game=1000.0,.number=2});CHECK(client.Time().game==40.5 && core.processState.Now()==40.5);
    const auto parent=HostStorageParent(core.pool);auto* process=core.processes.AddSharedRegular(parent,19,2);CHECK(process && process->Time()==40.5);if (!process) return;std::uint32_t calls{};core.processes.SetRegularHandler([&](auto,auto,auto,auto) { ++calls;return 0.75F; });client.FrameClockOwner()->game=41;core.kernel.RunFrame();CHECK(calls==1 && process->Time()==41.75);
    auto pass=world->ClearProcessesForState();world->ReturnProcessBindingsForState(std::move(pass));world.reset();CHECK(storage->Processes().State().Now()==41 && client.Time().game==41); // 반납이 실제 Client 프레임을0으로 바꾸지 않는다.
}

// Client를 먼저 반납해도 실제 Kernel/현재 프레임은 Storage가 pin하고 무효 부모 Regular는 살아 있다.
TEST_CASE(ProcessHostStorage_ExternalOwnersOutliveClientWithoutDanglingClockOrKernel) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);std::shared_ptr<RawSurfaceStorage> storage;std::weak_ptr<Kernel> kernel;std::weak_ptr<FrameTime> clock;RegularProcess* residual{};
    { ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());storage->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());kernel=client.ProcessKernelOwner();clock=client.FrameClockOwner();client.FrameClockOwner()->game=23.125;
      auto world=HostStorageWorld(storage);auto core=world->Core();const auto parent=HostStorageParent(core.pool);core.kernel.Add(std::make_unique<HostStorageFinalAction>([] {}));core.kernel.Add(std::make_unique<HostStorageFinalAction>([&] { residual=core.processes.AddRegular(parent,71,2); }));auto pass=world->ClearProcessesForState();world->ReturnProcessBindingsForState(std::move(pass));world.reset();storage->pool.Reset(0); }
    CHECK(!kernel.expired() && !clock.expired() && residual && storage->Processes().State().Now()==23.125);storage->Processes().Kernel().RunFrame();CHECK(storage->Processes().Kernel().Get(1)==residual);storage.reset();CHECK(kernel.expired() && clock.expired()); // 마지막 물리 반납은 Host/Pool보다 먼저 같은 실제 Kernel 잔류를 반납한다.
}

// 독립 fallback의 최종 Kernel 소멸에서도 Host/Pool과 타입 배열이 실제 가상 소멸자보다 오래 산다.
TEST_CASE(ProcessHostStorage_FallbackFinalShutdownKeepsHostAndPoolAlive) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);auto world=HostStorageWorld(storage);auto core=world->Core();std::uint32_t destroyed{};bool alive{};auto pass=world->ClearProcessesForState();
    core.kernel.Add(std::make_unique<HostStorageFinalAction>([host=&core.processes,pool=&core.pool,&destroyed,&alive] { alive=&host->Pool()==pool && pool->Capacity()==32768 && pool->Bytes().size()==32768U*50U;++destroyed; }));world->ReturnProcessBindingsForState(std::move(pass));world.reset();CHECK(destroyed==0);storage.reset();CHECK(destroyed==1 && alive);
}

// 단일 Clear 뒤 다른 파생이 남으면 현재 binding을 반납하지 않고 부분 효과와 실제 PID를 보존한다.
TEST_CASE(ProcessHostStorage_OtherResidualRequiresFurtherLifetimeWorkWithoutForcedDrain) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);auto world=HostStorageWorld(storage);auto core=world->Core();const auto parent=HostStorageParent(core.pool);SquidProcess* residual{};
    core.kernel.Add(std::make_unique<HostStorageFinalAction>([] {}));core.kernel.Add(std::make_unique<HostStorageFinalAction>([&] { residual=core.processes.Attach(std::make_unique<HostStorageOtherProcess>(),10,parent,0x50); }));auto pass=world->ClearProcessesForState();CHECK(residual && core.kernel.Find(*residual)==1 && core.kernel.Size()==1);
    CHECK(HostStorageRejects([&] { world->ReturnProcessBindingsForState(std::move(pass)); }));CHECK(!world->ProcessBindingsReturned() && core.kernel.Get(1)==residual && storage->Borrowed());world.reset();CHECK(core.kernel.Size()==0 && !storage->Borrowed()); // 일반 종료 fallback은 실제 자기 대여가 만든 객체를 모듈보다 먼저 반납한다.
}

// 현재 Regular 효과가 실행 중일 때는 그 호출의 실제 대여를 반납할 수 없고 정상 복귀 후 같은 증빙을 소비한다.
TEST_CASE(ProcessHostStorage_CurrentEffectMustReturnBeforeBindingRetirement) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);auto world=HostStorageWorld(storage);auto core=world->Core();auto pass=world->ClearProcessesForState();const auto parent=HostStorageParent(core.pool);auto* regular=core.processes.AddRegular(parent,83,5);std::uint32_t calls{};
    core.processes.SetRegularHandler([&](auto,auto,auto,auto) { CHECK(HostStorageRejects([&] { world->ReturnProcessBindingsForState(std::move(pass)); }));CHECK(!world->ProcessBindingsReturned());++calls;return 1.0F; });core.kernel.RunFrame();CHECK(calls==1 && regular && regular->Count()==1);world->ReturnProcessBindingsForState(std::move(pass));CHECK(world->ProcessBindingsReturned());world.reset();storage->pool.Reset(0);storage->Processes().Kernel().RunFrame();CHECK(storage->Processes().Kernel().Get(1)==regular);
}

// 실제 Clear의 가상 소멸 callback은 지난 정상 Clear 증빙을 다시 소비할 수 없다.
TEST_CASE(ProcessHostStorage_KernelClearCallRejectsPreviousPassReentry) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);auto world=HostStorageWorld(storage);auto core=world->Core();auto previous=world->ClearProcessesForState();bool rejected{};
    core.kernel.Add(std::make_unique<HostStorageFinalAction>([&] { try { world->ReturnProcessBindingsForState(std::move(previous)); }catch (const std::logic_error& error) { rejected=std::string_view(error.what()).find("진행 중")!=std::string_view::npos; } })); // 실제 소멸 중 예외는 밖으로 던지지 않는다.
    auto current=world->ClearProcessesForState();CHECK(rejected && !world->ProcessBindingsReturned() && core.kernel.Size()==0);world->ReturnProcessBindingsForState(std::move(current));CHECK(world->ProcessBindingsReturned());
}

// Notify가 자신을 제거하고 Regular만 남긴 뒤에도 외곽 가상 통지가 현재 binding을 pin한다.
TEST_CASE(ProcessHostStorage_DirectNotifyPinsBindingAfterSelfRemoval) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);auto world=HostStorageWorld(storage);auto core=world->Core();auto pass=world->ClearProcessesForState();const auto parent=HostStorageParent(core.pool);auto* regular=core.processes.AddRegular(parent,89,1);std::uint32_t called{};bool rejected{};
    auto* watcher=core.processes.Attach(std::make_unique<HostStorageNotifyProcess>(core.kernel,[&](auto event,auto info,auto value) { CHECK(event==2 && info==123 && value==456);++called;try { world->ReturnProcessBindingsForState(std::move(pass)); }catch (const std::logic_error& error) { rejected=std::string_view(error.what()).find("진행 중")!=std::string_view::npos; } }),10,parent,0x50);CHECK(watcher && regular);if (!watcher || !regular) return;const auto form=watcher->Form();
    core.processState.Server()=false;core.processes.NotifyDependency(Sid{65535},Sid{456},Sid{123});CHECK(called==0);core.processState.Server()=true;core.processes.NotifyDependency(form,Sid{456},Sid{123});CHECK(called==1 && rejected && !world->ProcessBindingsReturned() && core.kernel.Size()==1);world->ReturnProcessBindingsForState(std::move(pass));CHECK(world->ProcessBindingsReturned());
}

// 현재 binding 최종 정리도 소멸 효과 동안 반납 증빙의 소비를 금지하고 정상 복귀 뒤에만 허용한다.
TEST_CASE(ProcessHostStorage_OwnedShutdownPinsBindingDuringDestructorEffects) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);auto world=HostStorageWorld(storage);auto core=world->Core();auto pass=world->ClearProcessesForState();const auto parent=HostStorageParent(core.pool);bool rejected{};
    core.processes.Attach(std::make_unique<HostStorageOtherProcess>([&] { try { world->ReturnProcessBindingsForState(std::move(pass)); }catch (const std::logic_error& error) { rejected=std::string_view(error.what()).find("진행 중")!=std::string_view::npos; } }),10,parent,0x50);world->DrainCurrentProcessBindingsForShutdown();CHECK(rejected && !world->ProcessBindingsReturned() && core.kernel.Size()==0);world->ReturnProcessBindingsForState(std::move(pass));CHECK(world->ProcessBindingsReturned());
}
