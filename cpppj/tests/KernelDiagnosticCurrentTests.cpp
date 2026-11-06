// 양판본 Kernel 원문의 단언 복귀/소진 부분 효과와 실제 Client 진단 연결을 창 없이 대조한다.
#include "TestSupport.h"
#include "DudeSpawnSupport.h"
#include "FormTypeInputs.h"
#include "client/ClientDiagnostics.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientMain.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawProcessHostStorage.h"
#include "o/Kernel.h"
#include "o/SquidProcess.h"
#include <chrono>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace { // 원문 입력의 콜백 효과와 실제 CRT 파일만 관찰한다.
using namespace netstorm::o;
using namespace netstorm::client;
// 가상 소멸/프레임 효과를 외부에서 기록해 슬롯 갱신 순서와 별도로 비교한다.
class KernelDiagnosticProcess final:public BaseProcess {
public:
    explicit KernelDiagnosticProcess(std::function<void()> destroy={},std::function<void()> frame={}):destroy_(std::move(destroy)),frame_(std::move(frame)) {} // 관찰기만 소유하고 게임 상태는 합성하지 않는다.
    ~KernelDiagnosticProcess() override { if (destroy_) destroy_(); } // 실제 가상 소멸 효과가 실행된 위치만 기록한다.
    void RunFrame() override { if (frame_) frame_(); } // 실행 순회가 선택한 실제 슬롯만 기록한다.
private:
    std::function<void()> destroy_; // 같은 호출에서 대여한 외부 수명 관찰기다.
    std::function<void()> frame_; // 슬롯0은 이 관찰기에 도달하면 안 된다.
};
// 실제 프로세스 요약은 SquidProcess의 Form/Parent 필드만 읽는다.
class KernelSummaryProcess final:public SquidProcess {
public:
    void RunFrame() override {} // 이 검사는 게임 프레임을 진행하지 않는다.
};
// pre 가상 통지 뒤의 현재 PID 재조회가 실제 Client의 빈 슬롯 단언으로 이어지게 한다.
class KernelPidRewriteProcess final:public SquidProcess {
public:
    KernelPidRewriteProcess(SidPool& pool,ProcessId replacement,std::function<void()> destroy):pool_(pool),replacement_(replacement),destroy_(std::move(destroy)) {} // 실제 Form의 DWORD와 종료 관찰기만 빌린다.
    void RunFrame() override {} // 실제 전투/생산/통신을 진행하지 않는다.
    void OnFormDestroy(std::uint32_t) override { auto raw=pool_.AllocatedBytes(Form());for (std::size_t i=0;i<4;++i) raw[18+i]=static_cast<std::uint8_t>(replacement_>>(i*8)); } // 원문 +18의 현재 PID만 바꾸며 내부 등록 번호는 보존한다.
    ~KernelPidRewriteProcess() override { destroy_(); } // 원래 등록 슬롯은 별도 최종 반납에서만 소멸해야 한다.
private:
    SidPool& pool_; // 실제 현재 World가 빌린 장기 물리다.
    ProcessId replacement_; // pre 콜백 뒤 Remove가 읽을 빈0/2 입력이다.
    std::function<void()> destroy_; // 실제 Kernel/World보다 오래 사는 종료 관찰기다.
};
// 선택 실패를 관찰하고 앞선 요약/상세/슬롯 효과를 되돌리지 않는다.
template<class Function> bool KernelDiagnosticRejects(Function&& operation) { try { operation();return false; }catch (const std::exception&) { return true; } }
// 이 검사에서 만든 고정 파일/빈 디렉터리만 반납하며 재귀 삭제를 사용하지 않는다.
struct KernelDiagnosticDirectory {
    std::filesystem::path root=std::filesystem::temp_directory_path()/("netstorm-kernel-diag-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())); // 다른 검사의 출력과 겹치지 않는 자체 경로다.
    ~KernelDiagnosticDirectory() { std::error_code error;std::filesystem::remove(root/"Diagnostics"/"procSumm.txt",error);std::filesystem::remove(root/"Diagnostics",error);std::filesystem::remove(root,error); } // 실제 요약 파일을 닫은 뒤에만 호출된다.
};
}

TEST_CASE(KernelDiagnosticCurrent_RemoveResumesWithCurrentSlotAndEdition) { // 빈 슬롯은 단언 뒤 다시 읽고 새로 등록된 현재 객체만 소멸시킨다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 원문의 경로/행 번호를 독립 상수로 대조한다.
        for (const int mode:{0,1,2}) { // 빈 정상 복귀/슬롯 보수/단언 예외를 구별한다.
            Kernel kernel(2);std::size_t assertions{},destroyed{};KernelDiagnostics devices;devices.edition=edition;
            devices.assertion=[&](auto expression,auto file,auto line) {
                ++assertions;CHECK(expression=="processes[pid] != NULL");CHECK(file==(edition==OriginalEdition::Patch1078 ? "\\Ns\\O\\Kernel.cpp" : "\\ns\\o\\Kernel.cpp"));CHECK(line==(edition==OriginalEdition::Patch1078 ? 0x82U : 0x7fU));
                kernel.BindDiagnostics({}); // 선택된 함수의 수명을 교체해도 현재 호출은 정상적으로 끝나야 한다.
                if (mode==1) CHECK(kernel.Add(std::make_unique<KernelDiagnosticProcess>([&] { CHECK(kernel.Get(1)!=nullptr && kernel.Size()==1);++destroyed; }))==1);
                if (mode==2) throw std::runtime_error("선택된 단언 실패");
            };kernel.BindDiagnostics(devices);
            CHECK(KernelDiagnosticRejects([&] { kernel.Remove(1); })==(mode==2));CHECK(assertions==1 && destroyed==std::size_t(mode==1) && kernel.Size()==0 && !kernel.Get(1));
            kernel.BindDiagnostics(devices);CHECK(KernelDiagnosticRejects([&] { kernel.Remove(3); }));CHECK(assertions==1); // 물리 범위 밖 번호는 단언/배열 접근 전에 거부한다.
        }
    }
}

TEST_CASE(KernelDiagnosticCurrent_ExhaustionUsesCurrentDevicesAndCapturedZero) { // 원문의 요약→상세→단언 순서와 첫 슬롯0을 보존한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // CD의3999/패치39999 상한과 별도로 축소 물리 입력의 실패 분기를 대조한다.
        for (const bool freeDuringAssertion:{false,true}) { // 진단 중 빈 슬롯이 생겨도0을 다른 번호로 재선택하면 안 된다.
            std::size_t destroyed{};Kernel kernel(1);std::string calls,detail;CHECK(kernel.Add(std::make_unique<KernelDiagnosticProcess>([&] { ++destroyed; }))==1);
            KernelDiagnostics devices;devices.edition=edition;devices.summary=[&] { calls+='S';devices.detail=[&](auto text) { calls+='D';detail=text;devices.assertion=[&](auto expression,auto file,auto line) {
                calls+='A';CHECK(expression=="firstSlot != 0");CHECK(file==(edition==OriginalEdition::Patch1078 ? "\\Ns\\O\\Kernel.cpp" : "\\ns\\o\\Kernel.cpp"));CHECK(line==(edition==OriginalEdition::Patch1078 ? 0x7aU : 0x77U));if (freeDuringAssertion) kernel.Remove(1);
            };kernel.BindDiagnostics(devices); };kernel.BindDiagnostics(devices); };kernel.BindDiagnostics(devices);
            auto added=std::make_unique<KernelDiagnosticProcess>([&] { ++destroyed; });const auto* pointer=added.get();CHECK(kernel.Add(std::move(added))==0);
            CHECK(calls=="SDA" && detail=="Out of processes. A process count trace has been dumped\nto procSumm.txt.  Please deliver this file with the bug report.\n");
            CHECK(kernel.Get(0)==pointer && kernel.Size()==(freeDuringAssertion ? 1U : 2U) && destroyed==std::size_t(freeDuringAssertion));CHECK(!kernel.Get(2));
            kernel.DrainOwnedForShutdown();CHECK(destroyed==2 && kernel.Size()==0 && !kernel.Get(0));
        }
    }
}

TEST_CASE(KernelDiagnosticCurrent_ExhaustionFailurePreservesSelectedPartialEffects) { // 각 진단 실패는 그 앞의 효과만 남기고 새 포인터를 슬롯에 기록하지 않는다.
    for (const int stop:{0,1,2}) { // 요약/상세/단언에서 각각 예외를 전달한다.
        std::size_t destroyed{};Kernel kernel(1);kernel.Add(std::make_unique<KernelDiagnosticProcess>());std::string calls;KernelDiagnostics devices;
        devices.summary=[&] { calls+='S';if (stop==0) throw std::runtime_error("요약 실패"); };devices.detail=[&](auto) { calls+='D';if (stop==1) throw std::runtime_error("상세 실패"); };devices.assertion=[&](auto,auto,auto) { calls+='A';throw std::runtime_error("단언 실패"); };kernel.BindDiagnostics(devices);
        CHECK(KernelDiagnosticRejects([&] { kernel.Add(std::make_unique<KernelDiagnosticProcess>([&] { ++destroyed; })); }));CHECK(calls==std::string("SDA").substr(0,static_cast<std::size_t>(stop+1)) && destroyed==1 && kernel.Size()==1 && !kernel.Get(0));
        kernel.Clear();
    }
    Kernel unbound(1);unbound.Add(std::make_unique<KernelDiagnosticProcess>());CHECK(KernelDiagnosticRejects([&] { unbound.Add(std::make_unique<KernelDiagnosticProcess>()); }));CHECK(unbound.Size()==1 && !unbound.Get(0));
}

TEST_CASE(KernelDiagnosticCurrent_ZeroOverwriteSkipsNativePassAndDrainsSelectedOwner) { // 슬롯0 덮어쓰기에는 원문에 없는 소멸을 추가하지 않는다.
    std::string destroyed,frames;Kernel kernel(1);KernelDiagnostics devices;devices.summary=[] {};devices.detail=[](auto) {};devices.assertion=[](auto,auto,auto) {};kernel.BindDiagnostics(devices);
    kernel.Add(std::make_unique<KernelDiagnosticProcess>([&] { destroyed+='N'; },[&] { frames+='N'; }));auto first=std::make_unique<KernelDiagnosticProcess>([&] { destroyed+='A'; },[&] { frames+='A'; });const auto* old=first.get();CHECK(kernel.Add(std::move(first))==0);
    auto second=std::make_unique<KernelDiagnosticProcess>([&] { destroyed+='B'; },[&] { frames+='B'; });const auto* current=second.get();CHECK(kernel.Add(std::move(second))==0);CHECK(destroyed.empty() && kernel.Get(0)==current && kernel.Size()==2);
    kernel.RunFrame();CHECK(frames=="N");kernel.Clear();CHECK(destroyed=="N" && kernel.Size()==1 && kernel.Get(0)==current);
    kernel.DrainOwnedForShutdown([old](const BaseProcess& process) { return &process==old; });CHECK(destroyed=="NA" && kernel.Get(0)==current && kernel.Size()==1);
    kernel.Remove(0);CHECK(destroyed=="NAB" && !kernel.Get(0) && kernel.Size()==0);
}

TEST_CASE(KernelDiagnosticCurrent_ZeroDestructorReplacementKeepsLastNullWrite) { // 가상 소멸 중 새 슬롯0도 마지막 NULL 쓰기 뒤에는 실행 표에 남지 않는다.
    std::size_t oldDestroyed{},newDestroyed{};Kernel kernel(1);KernelDiagnostics devices;devices.summary=[] {};devices.detail=[](auto) {};devices.assertion=[](auto,auto,auto) {};kernel.BindDiagnostics(devices);kernel.Add(std::make_unique<KernelDiagnosticProcess>());
    kernel.Add(std::make_unique<KernelDiagnosticProcess>([&] { ++oldDestroyed;CHECK(kernel.Get(0)!=nullptr);CHECK(kernel.Add(std::make_unique<KernelDiagnosticProcess>([&] { ++newDestroyed; }))==0); }));
    kernel.Remove(0);CHECK(oldDestroyed==1 && newDestroyed==0 && !kernel.Get(0) && kernel.Size()==1);kernel.DrainOwnedForShutdown();CHECK(newDestroyed==1 && kernel.Size()==0);
}

TEST_CASE(KernelDiagnosticCurrent_ClientSummaryUsesUserDirectoryAndSafeLongOwners) { // 실제 시작과 같은 접합 함수가 같은 상세/현재 단언과 CRT 요약만 사용한다.
    KernelDiagnosticDirectory output;std::shared_ptr<Kernel> residual;std::weak_ptr<RawSurfaceStorage> weakSurface;std::weak_ptr<ClientGameplayGlobals> weakGameplay;const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    {
        ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;options.userData=netstorm::platform::UserDataRoot(output.root);Client client(std::move(options));const auto gameplay=client.GameplayGlobals();auto storage=MakeRawSurfaceStorage(assets.game,gameplay->Authority(),gameplay->GameAux());std::size_t assertions{};
        storage->BindDisplayDiagnostics(gameplay->AiDebug().AssertionDetail(),[&](auto expression,auto,auto line) { CHECK(expression=="processes[pid] != NULL" && line==0x82);++assertions; });
        BindClientKernelDiagnostics(client.ProcessKernelOwner(),storage,gameplay,output.root/"Diagnostics",true);client.GetKernel().Remove(0);CHECK(assertions==1 && !gameplay->AiDebug().AssertionDetail());
        residual=std::make_shared<Kernel>(1);BindClientKernelDiagnostics(residual,storage,gameplay,output.root/"Diagnostics",true);weakSurface=storage;weakGameplay=gameplay;
        const auto bytes=storage->pool.Bytes();const_cast<std::uint8_t*>(bytes.data())[10]=46; // 명시 원문 입력: Form SID0의 TYPE을Regular로 놓고 실제 Form/부모 getter는 대체하지 않는다.
        residual->Add(std::make_unique<KernelSummaryProcess>());storage->BindDisplayDiagnostics(gameplay->AiDebug().AssertionDetail(),[&](auto expression,auto,auto line) { CHECK(expression=="firstSlot != 0" && line==0x7a);CHECK(gameplay->AiDebug().AssertionDetail().has_value());++assertions; });
        CHECK(residual->Add(std::make_unique<KernelSummaryProcess>())==0 && assertions==2);std::ifstream file(output.root/"Diagnostics"/"procSumm.txt",std::ios::binary);const std::string text((std::istreambuf_iterator<char>(file)),{});file.close();
        CHECK(text=="pType=regularProcessType sid=0, attachToType=NULL\n");CHECK(!std::filesystem::exists(output.root/"Diagnostics"/"squids.txt"));CHECK(!std::filesystem::exists(output.root/"procSumm.txt"));
        CHECK(gameplay->AiDebug().AssertionDetail()==std::optional<std::string>("Out of processes. A process count trace has been dumped\nto procSumm.txt.  Please deliver this file with the bug report.\n"));
    }
    CHECK(weakSurface.expired() && !weakGameplay.expired());CHECK(KernelDiagnosticRejects([&] { residual->Remove(2); })); // 범위 밖은 종료된 물리에도 접근하지 않는다.
    residual->Remove(0);residual->Remove(1);CHECK(KernelDiagnosticRejects([&] { residual->Remove(0); }));CHECK(KernelDiagnosticRejects([&] { residual->Add(std::make_unique<KernelSummaryProcess>());residual->Add(std::make_unique<KernelSummaryProcess>()); })); // 종료된 물리의 요약/단언을 빈 성공으로 바꾸지 않는다.
    residual.reset();CHECK(weakGameplay.expired());
}

TEST_CASE(KernelDiagnosticCurrent_ClientWorldPreUsesChangedEmptyPidAndFinishesForm) { // 유효한 pre 객체를 먼저 조회한 뒤 PID가 바뀌는 경우는 초기 NULL PID와 다르다.
    for (const ProcessId replacement:{0U,2U}) { // 빈0/정상 범위의 빈2를 같은 실제 Client 장치에서 비교한다.
        std::size_t assertions{},destroyed{};ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));
        auto types=netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());const auto gameplay=client.GameplayGlobals();auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr,gameplay->Authority(),gameplay->GameAux());
        storage->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());BindClientKernelDiagnostics(client.ProcessKernelOwner(),storage,gameplay,{},false);
        storage->BindDisplayDiagnostics(gameplay->AiDebug().AssertionDetail(),[&](auto expression,auto file,auto line) { CHECK(expression=="processes[pid] != NULL" && file=="\\Ns\\O\\Kernel.cpp" && line==0x82);++assertions; });
        {
            BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();const auto parent=core.factory.Create(74,2);
            auto owned=std::make_unique<KernelPidRewriteProcess>(core.pool,replacement,[&] { ++destroyed; });auto* process=owned.get();CHECK(core.processes.Attach(std::move(owned),10,parent,0x50)==process);const auto form=process->Form();const auto registered=core.kernel.Find(*process);const auto free=core.pool.FreeCount();CHECK(registered==1 && &core.kernel==&client.GetKernel());
            CHECK(world.Destroy(form,0x10));CHECK(assertions==1 && destroyed==0 && core.kernel.Get(registered)==process && core.kernel.Size()==1);CHECK((core.pool.Slot(form)[11]&3U)==3U && core.pool.FreeCount()==free+1 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0); // 원문은 바뀐 빈 슬롯만 제거하고 원래 등록 객체를 임의로 추가 삭제하지 않는다.
        }
        CHECK(destroyed==1 && client.GetKernel().Size()==0); // World의 실제 대여 최종 반납은 남은 자기 C++ 소유권을 모듈보다 먼저 회수한다.
    }
}
