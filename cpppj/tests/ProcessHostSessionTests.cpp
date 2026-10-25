// 실제 Session을 교체하며 같은 Client Host의 Native 단일 Clear 잔류와 후반 실패 반납을 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "o/RawGameAuxState.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>

namespace { // 보호 파일은 읽기만 하고 실제 장치의 선택되지 않은 UI를 성공으로 합성하지 않는다.
using namespace netstorm::o;using namespace netstorm::client; // 실제 현재 Session과 원문 프로세스 타입을 사용한다.
struct HostSessionDevices {
    SoundList names{OriginalEdition::Patch1078};std::array<std::uint8_t,4> you{'Y','o','u',0}; // 모든 Session보다 오래 사는 실제 소리표/번역 BYTE다.
    RawCampaignSessionServices Make(Client& client,const std::shared_ptr<RawSurfaceStorage>& storage) {
        names.Initialize();RawCampaignSessionServices value;value.soundNames=&names;value.surfaceStorage=storage;value.processKernel=&client.GetKernel();value.gameplayGlobals=client.GameplayGlobals();value.subscriberGlobals=client.SubscriberGlobals();value.unixSeconds=[] { return 946684800; }; // 같은 Client 권한/Subscriber/물리/현재 Kernel을 공급한다.
        value.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };value.sound.wallSeconds=[] { return 100.25; };
        value.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };value.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };value.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };value.sound.stop=[](SoundHandle) {};value.sound.isPlaying=[](SoundHandle) { return 0U; }; // 창 없는 명시 음향 장치 입력이다.
        value.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };value.install.setConfig=[](auto,auto) {};value.install.refreshWindowTitle=[] {};value.install.resetCamera=[] {};value.ownership.refreshProduction=[] {};value.ownership.themeMode=[] { return 0U; };
        value.assets.postPop.playImpactSound=[](std::uint32_t) {};value.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};value.priest.shield.noticeSound=[](const PriestShieldNotice&) {};value.priest.shield.translate=[](std::string_view key) { return std::string(key); };value.priest.shield.tell=[](std::string_view) {};
        value.playerContent.gump=[](std::uint32_t) { return 0U; };value.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("Host 공유 검사의 미선택 UI"); };value.deletion.selected=[] { return Sid{}; };value.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("Host 공유 검사의 미선택 삭제"); };value.player.log=[](std::uint32_t,std::string_view) {};value.missionOutcome.demo=[] { return 0U; };return value;
    } // 실제 게임 계산이나 Kernel 결과는 Session이 수행한다.
};
// 같은 보호 Fort로 실제 전체 Session 모듈을 구성하며 Load/GUI를 실행하지 않는다.
std::unique_ptr<RawCampaignSession> HostSession(const netstorm::test::dudespawn::DudeAssets& assets,RawCampaignSessionServices services) {
    return std::make_unique<RawCampaignSession>(assets.game,assets.files,FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable()),"thewarbegins",std::move(services));
}
// Kernel의 실제 가상 소멸자에서만 지난 앞 슬롯에 Regular를 등록한다.
class HostSessionSpawn final:public BaseProcess {
public:
    explicit HostSessionSpawn(std::function<void()> action):action_(std::move(action)) {} // 명시 외부 재진입 효과만 소유한다.
    ~HostSessionSpawn() override { action_(); } // 실제 Clear가 이 슬롯에 도달한 때만 실행한다.
    void RunFrame() override {} // 게임 프레임을 대신 실행하지 않는다.
private:
    std::function<void()> action_; // 현재 Host에 실제 AddRegular를 요청하는 장치다.
};
// 실제 SID 할당 뒤 부모의 종류/상태만 독립 입력으로 명시한다.
Sid HostSessionParent(SidPool& pool) { const auto sid=pool.Allocate(2);auto bytes=pool.AllocatedBytes(sid);bytes[10]=82;bytes[11]=0;return sid; }
}

// old Session의 소멸자는 실제 소비된 Process binding을 다시 Drain하지 않으며 다음 현재 handler를 본다.
TEST_CASE(ProcessHostSession_ActualClientSessionRetiresRegularBindingAndReusesSamePid) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());storage->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());HostSessionDevices devices;client.FrameClockOwner()->game=17.25;
    auto session=HostSession(assets,devices.Make(client,storage));auto core=session->Core();CHECK(&core.kernel==&client.GetKernel() && &core.processState.Now()==&client.Time().game && &core.processState.Boss()==&client.GameplayGlobals()->Boss());session->PrepareFrame({.game=1.0,.number=1},true);CHECK(client.Time().game==17.25 && core.processState.Now()==17.25);const auto parent=HostSessionParent(core.pool);RegularProcess* residual{};
    core.kernel.Add(std::make_unique<HostSessionSpawn>([] {}));core.kernel.Add(std::make_unique<HostSessionSpawn>([&] { residual=core.processes.AddSharedRegular(parent,73,4); }));auto pass=session->ClearProcessesForState();CHECK(residual && core.kernel.Find(*residual)==1);if (!residual) return;const auto* host=&core.processes;session->ReturnProcessBindingsForState(std::move(pass));session.reset();
    CHECK(client.GetKernel().Get(1)==residual && !storage->Borrowed() && !client.GameplayGlobals()->Borrowed());storage->pool.Reset(0);client.GetKernel().RunFrame();CHECK(client.GetKernel().Get(1)==residual && residual->Count()==0);
    auto next=HostSession(assets,devices.Make(client,storage));auto current=next->Core();CHECK(&current.processes==host && &current.kernel==&client.GetKernel());const auto reused=HostSessionParent(current.pool);CHECK(reused==parent);std::uint32_t handled{};current.processes.SetRegularHandler([&](auto actual,auto event,auto count,auto payload) { CHECK(actual==parent && event==73 && count==0 && payload==4);++handled;return 2.5F; });client.FrameClockOwner()->game=20;client.GetKernel().RunFrame();CHECK(handled==1 && client.GetKernel().Get(1)==residual && residual->Count()==1 && residual->Time()==22.5);auto* currentOnly=current.processes.AddRegular(parent,75,3);CHECK(currentOnly);if (!currentOnly) return;const auto currentPid=current.kernel.Find(*currentOnly);CHECK(currentPid!=1);next.reset();CHECK(client.GetKernel().Get(1)==residual && client.GetKernel().Get(currentPid)==nullptr && client.GetKernel().Size()==1 && !storage->Borrowed()); // 일반 종료는 자기 현재 binding만 반납한다.
    storage->pool.Reset(0);client.GetKernel().RunFrame();CHECK(client.GetKernel().Get(1)==residual && residual->Count()==1);storage.reset();CHECK(client.GetKernel().Size()==0); // 이전 Native 잔류의 실제 최종 소유자가 마지막에만 반납한다.
    auto fresh=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());fresh->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());
    { auto ordinary=HostSession(assets,devices.Make(client,fresh));auto actual=ordinary->Core();const auto currentParent=HostSessionParent(actual.pool);CHECK(actual.processes.AddRegular(currentParent,77,1) && client.GetKernel().Size()==1); }CHECK(client.GetKernel().Size()==0 && !fresh->Borrowed()); // 이전 Native 잔류가 없는 종전 일반 종료는 같은 Kernel0을 유지한다.
}

// 실제 World 대여 뒤 SoundList 검증 실패는 이전 잔류를 지우지 않고 현재 바인딩만 반납한다.
TEST_CASE(ProcessHostSession_PartialConstructorFailureReleasesCurrentBindingAndPreservesResidual) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());storage->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());HostSessionDevices devices;client.FrameClockOwner()->game=7.5;RegularProcess* residual{};
    { auto first=HostSession(assets,devices.Make(client,storage));auto core=first->Core();const auto parent=HostSessionParent(core.pool);core.kernel.Add(std::make_unique<HostSessionSpawn>([] {}));core.kernel.Add(std::make_unique<HostSessionSpawn>([&] { residual=core.processes.AddRegular(parent,79,6); }));auto pass=first->ClearProcessesForState();first->ReturnProcessBindingsForState(std::move(pass)); }
    CHECK(residual && client.GetKernel().Get(1)==residual && storage->Generation()==1);auto missing=devices.Make(client,storage);missing.soundNames=nullptr;bool rejected{};try { static_cast<void>(HostSession(assets,std::move(missing))); }catch (const std::invalid_argument& error) { rejected=std::string_view(error.what())=="세션의 실제 SoundList 누락"; }
    CHECK(rejected && storage->Generation()==2 && !storage->Borrowed() && !client.GameplayGlobals()->Borrowed() && client.GetKernel().Get(1)==residual && storage->Processes().State().Now()==7.5);client.GetKernel().RunFrame();CHECK(client.GetKernel().Get(1)==residual); // 생성 도중의 Pool reset 때문에 parent는 실제 free이며 handler 조회는 생략된다.
    { auto recovered=HostSession(assets,devices.Make(client,storage));CHECK(storage->Generation()==3 && &recovered->Core().processes==&storage->Processes().Host());const auto core=recovered->Core();const auto parent=HostSessionParent(core.pool);CHECK(core.processes.AddRegular(parent,81,1)); }
    CHECK(client.GetKernel().Get(1)==residual && client.GetKernel().Size()==1 && !storage->Borrowed());storage.reset();CHECK(client.GetKernel().Size()==0);
}

// 완성된 새 Session의 실제 Load가 UI 장치에서 실패해도 이전 잔류와 현재 부착분의 종료 책임을 구별한다.
TEST_CASE(ProcessHostSession_CompletedLoadFailureCleansOnlyCurrentBinding) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());storage->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());HostSessionDevices devices;RegularProcess* residual{};
    { auto first=HostSession(assets,devices.Make(client,storage));auto core=first->Core();const auto parent=HostSessionParent(core.pool);core.kernel.Add(std::make_unique<HostSessionSpawn>([] {}));core.kernel.Add(std::make_unique<HostSessionSpawn>([&] { residual=core.processes.AddRegular(parent,91,2); }));auto pass=first->ClearProcessesForState();first->ReturnProcessBindingsForState(std::move(pass)); }
    CHECK(residual && client.GetKernel().Get(1)==residual);bool entered{};auto failing=devices.Make(client,storage);failing.install.refreshWindowTitle=[&] { entered=true;throw std::logic_error("442 명시 WindowTitle 장치 실패"); };auto next=HostSession(assets,std::move(failing));auto core=next->Core();const auto parent=HostSessionParent(core.pool);auto* current=core.processes.AddRegular(parent,93,4);CHECK(current);if (!current) return;const auto currentPid=core.kernel.Find(*current);CHECK(currentPid!=1);bool rejected{};
    try { next->Load(); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="442 명시 WindowTitle 장치 실패"; }CHECK(entered && rejected && client.GetKernel().Get(1)==residual && client.GetKernel().Get(currentPid)==current);next.reset();CHECK(client.GetKernel().Get(1)==residual && client.GetKernel().Get(currentPid)==nullptr && client.GetKernel().Size()==1 && !storage->Borrowed() && !client.GameplayGlobals()->Borrowed());
    storage->pool.Reset(0);client.GetKernel().RunFrame();CHECK(client.GetKernel().Get(1)==residual && residual->Count()==0);storage.reset();CHECK(client.GetKernel().Size()==0);
}
