// 신규 로컬 프로세스의 타입 진단 복귀와 생성 실패의 등록/필드 부분 효과를 확인한다.
#include "TestSupport.h"
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "o/SquidProcess.h"
#include "o/SquidFactory.h"
#include "o/SquidUnpop.h"
#include "o/RawBaseProcessAttach.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

TEST_CASE(BaseProcessAttachCurrent_RefreshDiagnosesDeadAndFreeSeparately) { // 갱신은 부모의 두 진단이 정상 복귀하면 현재 Form으로 해제/재부착을 계속한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 실제 전송 장치 없는 서버0 경로다.
        const bool patch=edition==OriginalEdition::Patch1078;bool server=false;SidPool pool(edition,32768,false,&server);auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState state;SquidProcessHost host(pool,types,kernel,destroy,state);const auto parent=factory.Create(74,2);auto* process=host.AddRegular(parent,0x123U,1.0F);CHECK(process);const auto form=process->Form();auto parentRaw=pool.AllocatedBytes(parent);parentRaw[11]|=3;std::vector<std::uint32_t> lines;
        state.assertion=[&](auto expression,auto file,auto line) { CHECK(file==(patch ? "\\Ns\\O\\Baseprocess.cpp" : "\\ns\\o\\BaseProcess.cpp"));lines.push_back(line);if (line==(patch ? 0xb0U : 0xabU)) { CHECK(expression=="!parent.isDead()");parentRaw[11]&=static_cast<std::uint8_t>(~2U); }else { CHECK(line==(patch ? 0xb1U : 0xacU) && expression=="!parent.isFree()");parentRaw[11]&=static_cast<std::uint8_t>(~1U); } }; // 원문의 저장 부모에 대한 두 독립 진단이다.
        bool failed=false;try { host.RefreshForm(*process); } catch (const std::exception&) { failed=true; }
        CHECK(!failed && lines==std::vector<std::uint32_t>({patch ? 0xb0U : 0xabU,patch ? 0xb1U : 0xacU}));CHECK(pool.Slot(form)[11]==8 && Get(parentRaw,6,2)==form.value && Get(pool.Slot(form),16,2)==parent.value && kernel.Size()==1 && !server);kernel.Clear();
    }
}

TEST_CASE(BaseProcessAttachCurrent_IndependentNative48NewLocalInputs) { // Kernel/Create 자식 경계를 포함하는 원문의 최초 호출/필드/슬롯 관측만 기대값으로 읽는다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_BASEPROCESS_ATTACH_CURRENT_FIXTURE).rows) { // 지정Form/수신/MP 분기는 새로 실행하지 않는다.
        CHECK(row.size()==6);if (row.size()!=6) continue;const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");const auto args=Split(row[1],',');CHECK(args.size()==3);const auto state=Number(args[0]),kind=Number(args[1]),profile=Number(args[2]);SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);const auto input=pool.Slot(Sid{5});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(input.data()),input.size()};
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*29+7); // Native의 입력 payload만 동일하게 준비한다.
        Put(raw,0,0x10002000U);raw[10]=2;const auto parent=pool.Slot(Sid{7});const_cast<std::uint8_t*>(parent.data())[11]=static_cast<std::uint8_t>(state);Sid form{},into{};std::string events,cause="returned";const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 호출 순서만 기록한다.
        BaseProcessAttachHooks hooks;hooks.typeEnd=[&] { return patch ? 63U : 62U; };hooks.processFormType=[] { return 2U; };
        hooks.assertion=[&](auto expression,auto file,auto line) { event("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line)); };
        hooks.registerProcess=[&] { event("K");return 7U; };
        hooks.create=[&](std::uint32_t flags) { event("C2,"+std::to_string(flags));if (profile) throw std::string("create");return Sid{5}; };
        hooks.attach=[&](Sid selected,Sid target,std::uint32_t flags) { CHECK(selected==Sid{5});event("A"+std::to_string(target.value)+','+std::to_string(flags)); };
        hooks.onAttached=[&] { event("N"); };
        try { AttachBaseProcess(pool,{form,into},kind,Sid{7},Sid{},0x50,hooks); } catch (const std::string& stop) { cause=stop; }
        CHECK(cause==row[2] && (events.empty()?"-":events)==row[3]);CHECK(std::to_string(form.value)+','+std::to_string(into.value)==row[4] && Hex(pool.Slot(Sid{5}))==row[5]);++count;
    }
    CHECK(count==48);
}

TEST_CASE(BaseProcessAttachCurrent_CurrentVtableSelectsNewLocalAttach) { // 현재 생성자가 쓰는 네 내장 Form VT의 실제 +74를 선택하며 raw TYPE로 고정하지 않는다.
    for (const auto kind:{1U,2U,3U,6U}) { // 모두 신규 로컬 Form0이며 클래스별 실제 생산/포획/수신 행동은 실행하지 않는다.
        bool server=false;SidPool pool(OriginalEdition::Patch1078,32768,false,&server);auto types=netstorm::test::MakeFormTypeInputs(188);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState state;state.processFormType=kind;SquidProcessHost host(pool,types,kernel,destroy,state);const auto parent=factory.Create(74,2);auto* process=host.Attach(std::make_unique<RegularProcess>(0x123U,1.0F),46,parent,0x50);CHECK(process);
        if (process) { const auto raw=pool.Slot(process->Form());CHECK(Get(raw,0)==SquidForms(pool).Vtable(kind) && raw[10]==46 && raw[11]==8 && Get(raw,16,2)==parent.value && Get(pool.Slot(parent),6,2)==process->Form().value && !server); }kernel.Clear();
    }
}

TEST_CASE(BaseProcessAttachCurrent_CreateFailureKeepsRegistrationAndWrittenFields) { // 공통 Create가 실패해도 앞선 Kernel/회전/프로세스 필드는 rollback하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 신규 로컬Form0만 사용하며 지정 수신/MP는 실행하지 않는다.
        const bool patch=edition==OriginalEdition::Patch1078;bool server=false;SidPool pool(edition,32768,false,&server);auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);types[2].constructorAddress=0xfffffff0U;SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState state;SquidProcessHost host(pool,types,kernel,destroy,state);const auto parent=factory.Create(74,2);const auto free=pool.FreeCount();
        CHECK(Throws([&] { host.Attach(std::make_unique<RegularProcess>(0x123U,1.0F),46,parent,0x50); }));auto* registered=dynamic_cast<SquidProcess*>(kernel.Get(1));
        CHECK(kernel.Size()==1 && registered && registered->Form()==Sid{} && registered->Parent()==parent && host.IsRegistered(1,registered) && host.Owns(*registered));
        CHECK(state.typeRotation==(patch ? 1U : 0U) && pool.FreeCount()==free-1 && Get(pool.Slot(parent),6,2)==0 && !server);kernel.Clear();
    }
}

TEST_CASE(BaseProcessAttachCurrent_TypeDiagnosticResumesBeforeFieldWritesAndRegister) { // 무효 프로세스 타입 진단은 정상 복귀 후 같은 입력으로 생성/현재 가상 부착을 계속한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 작은 범위 밖 TYPE9는 실제 전송/특수 행동을 선택하지 않는다.
        const bool patch=edition==OriginalEdition::Patch1078;bool server=false;SidPool pool(edition,32768,false,&server);auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState state;std::size_t diagnostics{};
        state.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="isProcessType(_processType)" && file==(patch ? "\\Ns\\O\\Baseprocess.cpp" : "\\ns\\o\\BaseProcess.cpp") && line==0x1c && kernel.Size()==0);++diagnostics; }; // 첫 진단 이전에는 등록이 없다.
        SquidProcessHost host(pool,types,kernel,destroy,state);const auto parent=factory.Create(74,2);SquidProcess* process{};bool failed=false;try { process=host.Attach(std::make_unique<RegularProcess>(0x123U,1.0F),9,parent,0x50); } catch (const std::exception&) { failed=true; }
        CHECK(!failed && process && diagnostics==1 && kernel.Size()==1);if (process) { const auto raw=pool.Slot(process->Form());CHECK(raw[10]==9 && raw[11]==8 && Get(raw,16,2)==parent.value && Get(pool.Slot(parent),6,2)==process->Form().value && !server); }kernel.Clear();
    }
}
