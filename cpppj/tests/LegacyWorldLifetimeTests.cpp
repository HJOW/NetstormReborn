// raw 외부 Kernel을 빌린 World의 최종 소유권과 현재 가상 회수 대여를 창 없이 검사한다.
#include "TestSupport.h"
#include "FormTypeInputs.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawProcessHostStorage.h"
#include "o/ContentFormLifetime.h"
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>

namespace { // 원문 객체를 실제 제품 API로 조립하고 C++ 소유권 경계만 관찰한다.
using namespace netstorm::client;using namespace netstorm::o; // 실제 World/Host/Kernel 타입을 그대로 사용한다.

// 두 판본의 기존 독립 World에 전달하는 표면 연결 번호다.
BridgeConnectState LegacyLinks() { BridgeConnectState result;result.bridgeType=82;result.islandType=94;result.stalagType=95;result.connectorType=155;result.noIslandType=157;return result; }

// 같은 실제 Pool/자산을 소유하며 GUI/원본 파일을 읽지 않는 저장소를 만든다.
std::shared_ptr<RawSurfaceStorage> LegacyStorage() {
    auto types=netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());
    return std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr);
}

// 장기 pin 장치가 없는 raw 외부 Kernel 경로를 명시적으로 선택한다.
std::unique_ptr<RawSurfaceWorld> LegacyWorld(const std::shared_ptr<RawSurfaceStorage>& storage,Kernel& kernel) {
    return std::make_unique<RawSurfaceWorld>(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,LegacyLinks(),83,&kernel,storage);
}

// 실제 할당 뒤 Form 부모의 TYPE/상태만 명시 입력으로 기록한다.
Sid LegacyParent(SidPool& pool) { const auto parent=pool.Allocate(2);auto raw=pool.AllocatedBytes(parent);raw[10]=82;raw[11]=0;return parent; }

// 현재 free-list 머리의 WORD만 변경해 Native 회수 분기를 선택한다.
void LegacyExhaustClient(SidPool& pool) {
    auto* raw=const_cast<std::uint8_t*>(pool.Slot(Sid{0}).data());const auto tail=pool.Tail(true).value;raw[4]=static_cast<std::uint8_t>(tail);raw[5]=static_cast<std::uint8_t>(tail>>8);
}

// 가상 최종 소멸에서 Host/Pool 생존과 소유권 반납 횟수를 관찰한다.
class LegacyOwnedProcess final:public SquidProcess {
public:
    explicit LegacyOwnedProcess(std::function<void()> destroyed):destroyed_(std::move(destroyed)) {} // 동일 검사 장치만 객체 수명 동안 소유한다.
    ~LegacyOwnedProcess() override { if (destroyed_) destroyed_(); } // 실제 Kernel의 가상 삭제에서 한 번만 실행한다.
    void RunFrame() override {} // 소유권 검사에서는 게임 계산을 진행하지 않는다.
    ProcessId RegisteredPid() const { return Pid(); } // 검사 파생 객체가 자기의 보호된 현재 Kernel 번호만 읽는다.
private:
    std::function<void()> destroyed_; // 현재 모듈이 살아 있는지 확인하는 동기 외부 관찰기다.
};

// 어느 World에도 속하지 않는 Client 프로세스의 최종 소멸만 기록한다.
class LegacyForeignProcess final:public BaseProcess {
public:
    explicit LegacyForeignProcess(std::size_t& destroyed):destroyed_(destroyed) {} // 외부 Kernel보다 오래 사는 검사 카운터를 빌린다.
    ~LegacyForeignProcess() override { ++destroyed_; } // World 종료가 이 객체까지 반납했는지 관찰한다.
    void RunFrame() override {} // 실제 프레임은 실행하지 않는다.
private:
    std::size_t& destroyed_; // 외부 소유자의 최종 반납 횟수다.
};

// 기대한 명시 수명 오류만 관찰하고 앞선 상태를 복구하지 않는다.
template<class Function> bool LegacyRejects(Function&& operation) { try { operation();return false; }catch (const std::exception&) { return true; } }
}

TEST_CASE(LegacyWorldLifetime_DirectAndExplicitShutdownPreserveForeignAndOtherHost) { // 같은 외부 Kernel의 두 Host와 Client 소유권을 분리한다.
    std::size_t foreignDestroyed{},firstDestroyed{},secondDestroyed{};Kernel kernel;auto foreign=std::make_unique<LegacyForeignProcess>(foreignDestroyed);auto* foreignPointer=foreign.get();CHECK(kernel.Add(std::move(foreign))==1);
    auto firstStorage=LegacyStorage(),secondStorage=LegacyStorage();auto first=LegacyWorld(firstStorage,kernel),second=LegacyWorld(secondStorage,kernel);auto firstCore=first->Core(),secondCore=second->Core();
    CHECK(&firstCore.kernel==&kernel && &secondCore.kernel==&kernel);const auto firstParent=LegacyParent(firstCore.pool),secondParent=LegacyParent(secondCore.pool);
    auto* firstProcess=firstCore.processes.Attach(std::make_unique<LegacyOwnedProcess>([&] { CHECK(&firstCore.processes.Pool()==&firstStorage->pool && firstStorage->Borrowed());++firstDestroyed; }),10,firstParent,0x50);
    auto* secondProcess=secondCore.processes.Attach(std::make_unique<LegacyOwnedProcess>([&] { CHECK(&secondCore.processes.Pool()==&secondStorage->pool && secondStorage->Borrowed());++secondDestroyed; }),10,secondParent,0x50);
    CHECK(firstProcess && secondProcess && kernel.Size()==3);const auto secondPid=secondProcess ? static_cast<LegacyOwnedProcess*>(secondProcess)->RegisteredPid():0;
    first->DrainCurrentProcessBindingsForShutdown();CHECK(firstDestroyed==1 && secondDestroyed==0 && foreignDestroyed==0 && kernel.Size()==2 && kernel.Get(secondPid)==secondProcess && kernel.Get(1)==foreignPointer);
    second.reset();CHECK(firstDestroyed==1 && secondDestroyed==1 && foreignDestroyed==0 && kernel.Size()==1 && kernel.Get(1)==foreignPointer && !secondStorage->Borrowed());
    first.reset();CHECK(firstDestroyed==1 && secondDestroyed==1 && foreignDestroyed==0 && kernel.Size()==1 && !firstStorage->Borrowed());kernel.Remove(1);CHECK(foreignDestroyed==1);
}

TEST_CASE(LegacyWorldLifetime_ShutdownDrainsOwnedZeroAndRetiredWithoutForeignClear) { // Native 소진 복귀로 슬롯0과 덮어쓴 소유권에 남은 두 객체를 필터 반납한다.
    std::size_t foreignDestroyed{},ownedDestroyed{},summaries{},details{},assertions{};Kernel kernel(1);auto foreign=std::make_unique<LegacyForeignProcess>(foreignDestroyed);auto* foreignPointer=foreign.get();kernel.Add(std::move(foreign));
    kernel.BindDiagnostics({OriginalEdition::Patch1078,[&] { ++summaries; },[&](auto) { ++details; },[&](auto,auto,auto) { ++assertions; }});
    auto storage=LegacyStorage();auto world=LegacyWorld(storage,kernel);auto core=world->Core();const auto parent=LegacyParent(core.pool);
    auto* first=core.processes.Attach(std::make_unique<LegacyOwnedProcess>([&] { CHECK(storage->Borrowed());++ownedDestroyed; }),10,parent,0x50);
    auto* second=core.processes.Attach(std::make_unique<LegacyOwnedProcess>([&] { CHECK(storage->Borrowed());++ownedDestroyed; }),10,parent,0x50);
    CHECK(first && second && kernel.Get(0)==second && kernel.Get(1)==foreignPointer && kernel.Size()==2 && ownedDestroyed==0 && summaries==2 && details==2 && assertions==2);
    world.reset();CHECK(ownedDestroyed==2 && foreignDestroyed==0 && kernel.Get(0)==nullptr && kernel.Get(1)==foreignPointer && kernel.Size()==1 && !storage->Borrowed());kernel.Remove(1);CHECK(foreignDestroyed==1);
}

TEST_CASE(LegacyWorldLifetime_AllocationUsesCurrentWorldAndRejectsRetiredLease) { // 현재 Content 클래스의 실제 가상 삭제를 외부 Kernel World에도 대여한다.
    Kernel kernel;auto storage=LegacyStorage();std::optional<std::string> detail;std::size_t assertions{};storage->BindDisplayDiagnostics(detail,[&](auto,auto,auto) { ++assertions; });
    for (std::size_t generation=0;generation<2;++generation) { // 이전 World를 반납한 뒤 같은 물리를 새 현재 World에 다시 대여한다.
        auto world=LegacyWorld(storage,kernel);auto core=world->Core();ContentLifetimeState state;ContentFormHost contents(core.pool,core.types,[](auto,auto,auto) {});ContentFormLifetime lifetime(core.pool,core.types,contents,core.destroy,state,{},core.forms.Hooks());core.forms.Bind(lifetime);
        const auto first=core.factory.CreateContained(2);const auto before=core.pool.FreeCount();storage->links.bridgeType=6;LegacyExhaustClient(core.pool);const auto replacement=core.factory.CreateContained(2);
        CHECK(first.value==5 && replacement.value==14999 && (core.pool.Slot(first)[11]&1U) && core.pool.FreeCount()==before && core.pool.FirstFree(true)==first && core.pool.Tail(true)==first && assertions==0);
        CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && world->Destroy(replacement,0x10));const auto residual=core.factory.CreateContained(2);CHECK(residual.value==5);world.reset();
        storage->links.bridgeType=6;LegacyExhaustClient(storage->pool);CHECK(LegacyRejects([&] { storage->pool.Allocate(2); }));CHECK(!(storage->pool.Slot(residual)[11]&1U) && !storage->Borrowed() && kernel.Size()==0);
    }
}

TEST_CASE(LegacyWorldLifetime_ReclaimLeaseRejectsOverlapAndPinsSynchronousCall) { // 대여가 선택된 호출 중에는 자기 반납 뒤에도 다른 현재 World를 겹쳐 설치하지 않는다.
    auto storage=LegacyStorage();auto& owner=storage->Processes();std::shared_ptr<const std::function<void(Sid)>> use;std::size_t calls{};bool overlapRejected{};
    CHECK(LegacyRejects([&] { owner.BindLegacyAllocationReclaim({}); }));
    use=owner.BindLegacyAllocationReclaim([&](Sid sid) { CHECK(sid.value==17);++calls;use.reset();overlapRejected=LegacyRejects([&] { owner.BindLegacyAllocationReclaim([](auto) {}); }); });
    CHECK(LegacyRejects([&] { owner.BindLegacyAllocationReclaim([](auto) {}); }));owner.ReclaimForAllocation(Sid{17});CHECK(calls==1 && overlapRejected && !use);
    CHECK(LegacyRejects([&] { owner.ReclaimForAllocation(Sid{17}); }));auto next=owner.BindLegacyAllocationReclaim([&](Sid sid) { CHECK(sid.value==19);++calls; });owner.ReclaimForAllocation(Sid{19});CHECK(calls==2);next.reset();
}
