// Process/Gump의 공통 생성 부모와 현재 World 대여만 검사한다. 게임/새 수신·패킷·MP는 실행하지 않는다.
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "client/RawProcessHostStorage.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawGameAuxState.h"
#include "o/GumpFormHost.h"
#include "o/SquidForms.h"
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
            storage->types[7+generation].constructorAddress=0xfffffff0U;const auto free=core.pool.FreeCount();const auto registered=core.kernel.Size();CHECK(Throws([&] { core.processes.AddRegular(lastParent,0x124,1); }) && core.kernel.Size()==registered && core.pool.FreeCount()==free-1); // 현재 공통Create의 할당 뒤 미연결 생성자 실패다.
            storage->types[7+generation].constructorAddress=TypeConstructorAddress(edition,2);
        }
        SquidFactory external(storage->pool,storage->types);const auto detachedParent=external.Create(74);const auto free=storage->pool.FreeCount();CHECK(Throws([&] { saved->AddRegular(detachedParent,0x125,1); }) && storage->pool.FreeCount()==free); // 유효 부모가 있어도 World 없는 장기 Host가 이전 공장으로 빠지지 않는다.
        aux->ResetForGeneralWorld();CHECK(aux->ProcessFormType()==8);
    }
}

TEST_CASE(ProcessCreationCurrent_IndependentHostUsesCommonCreateAndCurrentDword) { // 독립 입력도 고정VT 대신 원문 생성자/번호 관문을 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 독립 TYPE snapshot의 정상 전제조건만 명시한다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);types[7].constructorAddress=TypeConstructorAddress(edition,2);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState state;std::uint32_t number=7;state.currentProcessFormType=&number;SquidProcessHost host(pool,types,kernel,destroy,state);const auto parent=factory.Create(74);
        auto* process=host.AddRegular(parent,0x123,1);CHECK(process && Get(pool.Slot(process->Form()),0)==SquidForms(pool).Vtable(2));number=0xffffffffU;const auto free=pool.FreeCount();const auto registered=kernel.Size();CHECK(Throws([&] { host.AddRegular(parent,0x124,1); }) && pool.FreeCount()==free-1 && kernel.Size()==registered);kernel.Clear(); // 전체DWORD를2나BYTE로 바꾸지 않는다.
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
    CHECK(Throws([&] { storage.Bind(binding); }));binding->factory=nullptr;auto use=storage.Bind(binding);auto& host=storage.Host();const auto parent=factory.Create(74);const auto free=pool.FreeCount();CHECK(Throws([&] { host.AddRegular(parent,0x123,1); }) && pool.FreeCount()==free && storage.Kernel().Size()==0);CHECK(Throws([&] { host.BindFormFactory(factory); }));use.reset();binding->factory=&factory;use=storage.Bind(binding);CHECK(host.AddRegular(parent,0x124,1)!=nullptr);storage.Kernel().Clear();use.reset();CHECK(Throws([&] { host.AddRegular(parent,0x125,1); }));
}
