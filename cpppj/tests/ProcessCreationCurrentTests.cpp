// Process/Gump의 공통 생성 부모와 현재 World 대여만 검사한다. 게임/새 수신·패킷·MP는 실행하지 않는다.
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "client/RawProcessHostStorage.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawGameAuxState.h"
#include "o/GumpFormHost.h"
#include "o/SquidForms.h"
#include "o/SquidFormDispatch.h" // 실제 Process 클래스의 부모/삭제 효과와 같은 Kernel을 대조한다.
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 실제 생성/대여에 필요한 원문 TYPE 입력만 제공한다.
BridgeConnectState ProcessCreationLinks() { BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;return links; } // World 설치용 정상 번호이며 출력 기대값을 만들지 않는다.
}

TEST_CASE(ProcessCreationCurrent_WorldFactoryCurrentNumberAndSuccessiveLoans) { // 같은 장기 Host가 현재 번호/생성자와 새 World 공장을 조회한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 양판본의 같은 GameAux 주소를 사용하고 실제 게임은 실행하지 않는다.
        const bool patch=edition==OriginalEdition::Patch1078;auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);types[7].constructorAddress=types[8].constructorAddress=TypeConstructorAddress(edition,2);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto aux=std::make_shared<RawGameAuxState>();aux->ProcessFormType()=7;
        auto storage=std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes,nullptr,std::shared_ptr<RawAuthorityState>{},aux);SquidProcessHost* saved{};Sid lastParent{};
        for (std::uint32_t generation=0;generation<2;++generation) { // 반납 뒤 옛 Factory를 직접 사용하지 않고 같은 Host에서 다음 대여를 얻는다.
            aux->ProcessFormType()=7+generation;RawSurfaceWorld world(edition,storage->types,frames,shapes,ProcessCreationLinks(),83,nullptr,storage);auto core=world.Core();if (saved) CHECK(saved==&core.processes);saved=&core.processes;CHECK(core.processState.currentProcessFormType==&aux->ProcessFormType() && aux->ProcessFormType()==7+generation);
            lastParent=core.factory.Create(74);auto* process=core.processes.AddRegular(lastParent,0x123,1);CHECK(process && Get(core.pool.Slot(process->Form()),0)==SquidForms(core.pool).Vtable(2));
            storage->types[7+generation].constructorAddress=0xfffffff0U;const auto free=core.pool.FreeCount();const auto registered=core.kernel.Size();CHECK(Throws([&] { core.processes.AddRegular(lastParent,0x124,1); }) && core.kernel.Size()==registered+1 && core.pool.FreeCount()==free-1); // 현재 공통Create의 할당 뒤 실패에서도 앞선 Kernel 등록은 남는다.
            storage->types[7+generation].constructorAddress=TypeConstructorAddress(edition,2);
        }
        SquidFactory external(storage->pool,storage->types);const auto detachedParent=external.Create(74);const auto free=storage->pool.FreeCount();CHECK(Throws([&] { saved->AddRegular(detachedParent,0x125,1); }) && storage->pool.FreeCount()==free); // 유효 부모가 있어도 World 없는 장기 Host가 이전 공장으로 빠지지 않는다.
        aux->ResetForGeneralWorld();CHECK(aux->ProcessFormType()==8);
    }
}

TEST_CASE(ProcessCreationCurrent_IndependentHostUsesCommonCreateAndCurrentDword) { // 독립 입력도 고정VT 대신 원문 생성자/번호 관문을 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 독립 TYPE snapshot의 정상 전제조건만 명시한다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);types[7].constructorAddress=TypeConstructorAddress(edition,2);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState state;std::uint32_t number=7;state.currentProcessFormType=&number;SquidProcessHost host(pool,types,kernel,destroy,state);const auto parent=factory.Create(74);
        auto* process=host.AddRegular(parent,0x123,1);CHECK(process && Get(pool.Slot(process->Form()),0)==SquidForms(pool).Vtable(2));number=0xffffffffU;const auto free=pool.FreeCount();const auto registered=kernel.Size();CHECK(Throws([&] { host.AddRegular(parent,0x124,1); }) && pool.FreeCount()==free-1 && kernel.Size()==registered+1);kernel.Clear(); // 전체DWORD를2나BYTE로 바꾸지 않는다.
    }
}

TEST_CASE(ProcessCreationCurrent_GumpTakeUsesCurrentConstructorAndKeepsPayload) { // 같은 실제 Gump 부모 해제 뒤 현재 생성자/빈+2c를 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 수신/패킷 없이 같은 객체 수명만 대조한다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);factory.BindDiagnostics({types,{},{}});SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);GumpFormHost gumps(pool,factory,types,destroy);const auto parent=factory.Create(74);const auto form=gumps.Create(0);gumps.SetOwner(form,0x12345678);gumps.Attach(form,parent,0x10);
        types[3].constructorAddress=TypeConstructorAddress(edition,1);const auto free=pool.FreeCount();CHECK(gumps.Take(form)==form);const auto raw=pool.Slot(form);CHECK(Get(raw,0)==SquidForms(pool).Vtable(1) && raw[10]==3 && raw[11]==4 && Get(raw,18)==0x12345678 && Get(pool.Slot(parent),6,2)==0 && pool.FreeCount()==free);
    }
}

TEST_CASE(ProcessCreationCurrent_RuntimeMissingAndForeignFactoryAreExplicit) { // 대여에 공장 주소를 저장하기 전에 Pool을 확인하고 선택된 부재를 우회하지 않는다.
    auto authority=std::make_shared<RawAuthorityState>();SidPool pool(OriginalEdition::Patch1078,32768,true,&authority->server),other(OriginalEdition::Patch1078,32768);auto types=netstorm::test::MakeFormTypeInputs(188);SquidFactory factory(pool,types),foreign(other,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);RawProcessHostStorage storage(pool,types,authority);auto binding=std::make_shared<ProcessHostRuntimeBindings>();binding->destroy=&destroy;binding->factory=&foreign;
    CHECK(Throws([&] { storage.Bind(binding); }));binding->factory=nullptr;auto use=storage.Bind(binding);auto& host=storage.Host();const auto parent=factory.Create(74);const auto free=pool.FreeCount();CHECK(Throws([&] { host.AddRegular(parent,0x123,1); }) && pool.FreeCount()==free && storage.Kernel().Size()==1);CHECK(Throws([&] { host.BindFormFactory(factory); }));storage.Kernel().Clear();use.reset();binding->factory=&factory;use=storage.Bind(binding);CHECK(host.AddRegular(parent,0x124,1)!=nullptr);storage.Kernel().Clear();use.reset();CHECK(Throws([&] { host.AddRegular(parent,0x125,1); }));
}

namespace { // 실제 Kernel 제거 안의 가상 콜백/소멸자 시점을 관찰하는 작은 프로세스다.
class ProcessClassCurrentProbe final : public SquidProcess {
public:
    ProcessClassCurrentProbe(Kernel& kernel,std::vector<std::string>& events):kernel_(kernel),events_(events) {} // 관찰 저장소는 Kernel보다 오래 살아야 한다.
    void RunFrame() override {} // 프레임/전투/통신을 실행하지 않는다.
    void OnParentDying(Sid parent) override { events_.push_back("parent"+std::to_string(parent.value));if (parentEffect) parentEffect(parent); } // 원본 가상+10에서 현재 TYPE를 바꾸는 입력이다.
    void OnFormDestroy(std::uint32_t flags) override { events_.push_back("pre"+std::to_string(flags));if (destroyEffect) destroyEffect(); } // 원본 가상+c 뒤 다음 공통 효과가 현재 클래스를 다시 사용해야 한다.
    ~ProcessClassCurrentProbe() override { events_.push_back(kernel_.Get(Pid())==this ? "destructor-live":"destructor-missing"); } // Kernel은 가상 소멸자가 끝난 뒤 슬롯을 지운다.
    std::function<void(Sid)> parentEffect; // 부모 통지가 실행할 명시적인 바이트 변경이다.
    std::function<void()> destroyEffect; // 삭제 통지가 실행할 명시적인 바이트 변경이다.
private:
    Kernel& kernel_; // 실제 조회/단일 소유권 제거를 담당하는 같은 Kernel이다.
    std::vector<std::string>& events_; // 콜백 순서를 보존하는 외부 관찰 저장소다.
};
SquidDestroyHooks ProcessClassCurrentAssets(RawSquidDestroy& destroy) { // client bare 부모의 실제 공통 깊이만 반납하며 송신은 선택되면 실패한다.
    SquidDestroyHooks hooks;hooks.selected=[] { return Sid{}; };hooks.emit=[&](const SquidDestroyEvent& event) { if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();else if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();else throw std::logic_error("Process 클래스 검사에서 선택하지 않은 자산/전송 효과"); };return hooks;
}
}
// TYPE 별칭과 콜백의 TYPE 변경에도 실제 Process VT/PID가 부모/삭제/소멸자·공통 깊이를 결정한다.
TEST_CASE(ProcessClassCurrent_ActualHostAndDispatchIgnoreTypeAliases) {
    std::size_t inputs{};
    for (const bool narrowEnd:{false,true}) { // 이미 생성한 클래스의 삭제에는 이후의 전역 프로세스 상한 변경을 사용하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 양판본 실제 생성자와 client 로컬 슬롯만 사용한다.
        if (narrowEnd && edition!=OriginalEdition::Patch1078) continue; // 현재 상한 대여를 쓰는 PATCH에만 변경 입력을 추가한다. CD 독립 getter는 고정62다.
        for (const bool destroyParent:{false,true}) { // 직접 Host Kill과 공유 부모 삭제의 가상 release 경로를 각각 사용한다.
            for (const auto type:{1U,2U,3U,6U,7U,46U,63U,69U}) { // Process 범위 밖의 유효 Form TYPE도 현재 클래스와 구별한다.
                SidPool pool(edition,32768,false);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);SquidProcessState state;std::vector<std::string> events;Kernel kernel(edition==OriginalEdition::Patch1078 ? kProcessCapacity:kCdProcessCapacity);SquidFormDispatch dispatch(pool,destroy,ProcessClassCurrentAssets(destroy));SquidProcessHost host(pool,types,kernel,destroy,state,{},dispatch.Hooks());dispatch.Bind(host);CHECK(Throws([&] { dispatch.Bind(host); }));const auto parent=factory.Create(74,2);
                auto owned=std::make_unique<ProcessClassCurrentProbe>(kernel,events);auto* process=owned.get();CHECK(host.Attach(std::move(owned),10,parent,0x50)==process);const auto form=process->Form();const auto pid=kernel.Find(*process);if (narrowEnd) state.processTypeEnd=0;auto raw=pool.AllocatedBytes(form);raw[10]=static_cast<std::uint8_t>(type);
                process->parentEffect=[&](Sid actual) { CHECK(actual==parent && !(pool.Slot(form)[11]&2U));pool.AllocatedBytes(form)[10]=6; }; // 부모 통지는 자식 dead 설정 전에 실행된다.
                process->destroyEffect=[&] { CHECK((pool.Slot(form)[11]&2U) && pool.Slot(form)[10]==(destroyParent ? 6U:type));pool.AllocatedBytes(form)[10]=3; }; // 같은 VT의 후처리를 raw TYPE3의 Gump로 바꾸지 않는다.
                bool failed=false;try { if (destroyParent) CHECK(dispatch.Destroy(parent,0x10));else host.Kill(*process,0x10); } catch (const std::exception& error) { failed=true;std::printf("ProcessClassCurrent edition=%u parent=%u type=%u: %s\n",static_cast<unsigned>(edition),destroyParent?1U:0U,type,error.what()); }
                const auto expected=destroyParent ? std::vector<std::string>{"parent"+std::to_string(parent.value),"pre80","destructor-live"}:std::vector<std::string>{"pre16","destructor-live"};CHECK(!failed && events==expected && !kernel.Get(pid) && kernel.Size()==0);CHECK((pool.Slot(form)[11]&3U)==3U && pool.Slot(form)[10]==3 && destroy.PreDepth()==0 && destroy.PostDepth()==0);
                if (!failed && !destroyParent) { CHECK(Get(pool.Slot(parent),6,2)==0);CHECK(dispatch.Destroy(parent,0x10)); }++inputs;
            }
        }
    }
    }
    CHECK(inputs==48);
}
// 원문 부모 통지의 NULL 생략과 preDestroy의 필수 PID/선택된 가상 장치 경계를 구별한다.
TEST_CASE(ProcessClassCurrent_NullParentNoticeAndForeignKernelClassStayDistinct) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 잘못된 원본 포인터를 실행하지 않고 호스트의 명시 실패/접두만 확인한다.
        for (const bool foreign:{false,true}) { // 빈 PID0과 비어있지 않은 다른 C++ Kernel 클래스를 구별한다.
            SidPool pool(edition,32768,false);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);SquidProcessState state;Kernel kernel(edition==OriginalEdition::Patch1078 ? kProcessCapacity:kCdProcessCapacity);SquidFormDispatch dispatch(pool,destroy,ProcessClassCurrentAssets(destroy));SquidProcessHost host(pool,types,kernel,destroy,state,{},dispatch.Hooks());dispatch.Bind(host);const auto form=factory.Create(2,2);auto raw=pool.AllocatedBytes(form);raw[10]=foreign ? 46:2;const auto pid=foreign ? kernel.Add(std::make_unique<BaseProcess>()):0U;Put(raw,18,pid);const auto free=pool.FreeCount();
            CHECK(Throws([&] { dispatch.Hooks().emit({SquidDestroyEffect::ReleaseDependent,form,Sid{50},0}); })==foreign);CHECK(raw[11]==4 && pool.FreeCount()==free && destroy.PreDepth()==0 && destroy.PostDepth()==0);
            CHECK(Throws([&] { dispatch.Destroy(form,0x10); }));CHECK(raw[11]==6 && pool.FreeCount()==free && destroy.PreDepth()==1 && destroy.PostDepth()==0 && kernel.Size()==(foreign ? 1U:0U)); // PID0/잘못된 장치를 성공으로 숨기거나 이미 쓴 dead/깊이를 되돌리지 않는다.
        }
    }
}
// 실제 World 대여의 같은 Factory/Kernel도 현재 Process 클래스의 TYPE 별칭 수명을 처리한다.
TEST_CASE(ProcessClassCurrent_ActualWorldReturnsAliasedProcessAndKeepsFinderFilter) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 테스트용 TYPE/프레임만 쓰고 실제 게임/창은 실행하지 않는다.
        for (const auto type:{2U,7U,46U,69U}) { // 생성 이후 raw TYPE만 바꾸며 원문의 별도 finder 타입 관문은 유지한다.
            auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto storage=std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes,nullptr);RawSurfaceWorld world(edition,storage->types,frames,shapes,ProcessCreationLinks(),83,nullptr,storage);auto core=world.Core();const auto parent=core.factory.Create(74,2);auto* process=core.processes.AddRegular(parent,0x123,1);CHECK(process);if (!process) return;const auto form=process->Form();const auto pid=core.kernel.Find(*process);core.pool.AllocatedBytes(form)[10]=static_cast<std::uint8_t>(type);CHECK(core.processes.FindEvent(parent,0x123)==(type==46 ? process:nullptr));const auto free=core.pool.FreeCount();
            bool failed=false;try { CHECK(world.Destroy(form,0x10)); } catch (const std::exception& error) { failed=true;std::printf("ProcessClassCurrent World edition=%u type=%u: %s\n",static_cast<unsigned>(edition),type,error.what()); }
            CHECK(!failed && !core.kernel.Get(pid) && core.kernel.Size()==0 && (core.pool.Slot(form)[11]&3U)==3U && core.pool.FreeCount()==free+1);CHECK(Get(core.pool.Slot(parent),6,2)==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
        }
    }
}
