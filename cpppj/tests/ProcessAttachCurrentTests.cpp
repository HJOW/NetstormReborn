// Process 부착의 서버0 진단 복귀와 실제 공통 Form 후단을 확인한다. MP/직렬화/송신은 실행하지 않는다.
#include "TestSupport.h"
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "o/SquidProcess.h"
#include "o/SquidFactory.h"
#include "o/SquidUnpop.h"
#include "o/RawProcessAttach.h"
#include "client/RawSurfaceWorld.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

TEST_CASE(ProcessAttachCurrent_ActualWorldUsesCurrentDiagnosticAndCommonFactory) { // 현재 World 대여의 실제 진단/Unpop/공통 부착/Pop 깊이를 같은 호스트로 연결한다.
    using namespace netstorm::client;
    auto types=netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto authority=std::make_shared<RawAuthorityState>();authority->server=false;auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr,authority);std::optional<std::string> detail;SidPool* pool{};Sid parent{},freeParent{},form{};std::vector<std::uint32_t> lines;
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) {
        CHECK(pool && file=="\\Ns\\O\\Squid.cpp");lines.push_back(line);const auto slot=pool->Slot(form);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()};
        if (line==0xa5bU) { CHECK(expression=="!isVoid()");Put(raw,16,freeParent.value,2); }
        else { CHECK(line==0xa77U && expression=="popFlags & pfPROCESS_DEBUG");Put(pool->AllocatedBytes(parent),6,0,2); }
    }); // 장기 Client 장치가 현재 원문 진단에서만 raw 입력을 수리한다.
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();authority->server=false;CHECK(!core.pool.IsServer());pool=&core.pool;parent=core.factory.Create(74,2);freeParent=core.factory.Create(74,2);auto* process=core.processes.AddRegular(parent,0x123,1);CHECK(process);form=process->Form();auto raw=core.pool.AllocatedBytes(form);core.pool.AllocatedBytes(freeParent)[11]=1;raw[11]|=4; // World 초기화의 서버1 재설정 뒤 검사 입력 서버0을 명시한다.
    core.processes.RefreshForm(*process);CHECK(lines==std::vector<std::uint32_t>({0xa5b,0xa77}) && raw[11]==8 && Get(raw,16,2)==parent.value && Get(core.pool.Slot(parent),6,2)==form.value && core.bookkeeping.depth==0 && !core.pool.IsServer());core.kernel.Clear();pool=nullptr;
}

TEST_CASE(ProcessAttachCurrent_IndependentNative48NonServerInputs) { // 구현 전에 고정한 양판본의 진단/공통 자식 인자와 전체 raw 슬롯을 대조한다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_PROCESS_ATTACH_CURRENT_FIXTURE).rows) { // 모든 입력에서 서버0이며 writer/패킷은 실행하지 않는다.
        CHECK(row.size()==4);if (row.size()!=4) continue;const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");const auto args=Split(row[1],',');CHECK(args.size()==3);const auto stored=Number(args[0]),flags=Number(args[1]),profile=Number(args[2]);SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);const auto slot=pool.Slot(Sid{5});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()};
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*29+7); // Native와 같은 입력 payload만 생성한다.
        Put(raw,16,stored,2);std::string events;const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 선택된 순서만 직렬화한다.
        ProcessAttachHooks hooks;hooks.server=[] { return false; };hooks.lookup=[](Sid) { CHECK(false); };hooks.writeData=[](bool) { CHECK(false);return 0; };hooks.transmit=[](auto) { CHECK(false); };
        hooks.assertion=[&](auto expression,auto file,auto line) { event("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line));if (profile) Put(raw,16,11,2); };
        hooks.attach=[&](Sid form,Sid parent,std::uint32_t value) { CHECK(form==Sid{5});event("A"+std::to_string(parent.value)+','+std::to_string(value)+','+std::to_string(Get(raw,16,2))); };
        AttachProcessForm(pool,Sid{5},Sid{7},flags,hooks);CHECK(events==row[2]);CHECK(Hex(pool.Slot(Sid{5}))==row[3]);++count;
    }
    CHECK(count==48);
}

TEST_CASE(ProcessAttachCurrent_ActualNonServerRefreshResumesAfterParentDiagnostic) { // Unpop이 free 부모에서 반환한 뒤 진단 장치가 체인을 고치면 실제 부착을 계속한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 양판본의 로컬 진단 경로만 실행한다.
        const bool patch=edition==OriginalEdition::Patch1078;bool server=false;SidPool pool(edition,32768,false,&server);auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState state;SquidProcessHost host(pool,types,kernel,destroy,state);
        const auto parent=factory.Create(74,2),freeParent=factory.Create(74,2);auto* process=host.AddRegular(parent,0x123,1);CHECK(process && !pool.IsServer());const auto form=process->Form();auto raw=pool.AllocatedBytes(form);pool.AllocatedBytes(freeParent)[11]=1;raw[11]|=4;
        std::vector<std::uint32_t> diagnostics;state.assertion=[&](auto expression,auto file,auto line) {
            CHECK(file==(patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp"));diagnostics.push_back(line);
            if (line==(patch ? 0xa5bU : 0xa12U)) { CHECK(expression=="!isVoid()");Put(raw,16,freeParent.value,2); }
            else { CHECK(line==(patch ? 0xa77U : 0xa2eU) && expression=="popFlags & pfPROCESS_DEBUG" && (raw[11]&4U));Put(pool.AllocatedBytes(parent),6,0,2); }
        }; // 최초 Unpop 진단과 파생 부착 진단의 저장 부모/현재 WORD를 구별한다.
        bool failed=false;try { host.RefreshForm(*process); } catch (const std::exception&) { failed=true; }
        CHECK(!failed && diagnostics==std::vector<std::uint32_t>({patch ? 0xa5bU : 0xa12U,patch ? 0xa77U : 0xa2eU}));
        CHECK(raw[11]==8 && Get(raw,16,2)==parent.value && Get(raw,4,2)==0 && Get(pool.Slot(parent),6,2)==form.value && host.FromForm(form)==process && kernel.Size()==1 && !server);
        kernel.Clear();
    }
}
