// 실제 Client/Session을 유지한 Init의 Template 정리와 State7 반납을 검사한다. OS 창과 음향 장치는 열지 않는다.
#include "client/ClientStateWorldCleanup.h"
#include "client/ClientAudio.h"
#include "client/ClientSpGlobals.h"
#include "client/ClientConnectionGlobals.h"
#include "client/MissionFortRequest.h"
#include "client/TutorialInit.h"
#include "client/GuideMissionTransitions.h"
#include "client/MissionLifecycle.h"
#include "client/MissionTechnology.h"
#include "client/State.h"

namespace { // 기존 Session/작업 파일 helper의 입력을 재사용하며 게임 성공 상태는 주입하지 않는다.
struct GuideWorldScene484 { // 같은 실제 Client 장기 물리와 작업본 reader를 두 Session 세대에 걸쳐 유지한다.
    Client client{[] { auto options=SessionLinksOptions();options.noAudio=true;return options; }()}; // 화면 없는 Client와 비활성 음향 owner다.
    ReloadSourceScene483 input{"guide-world-484"};SessionServicesFixture external;SessionCursorUi cursorUi; //실제 파일과 OS 밖 음향/통지/커서 화면 요청의 기존 관측 장치다.
    std::shared_ptr<CampaignTemplateStorage> templates{std::make_shared<CampaignTemplateStorage>()}; // Init/cleanup이 정리하는 같은9개 헤더다.
    std::shared_ptr<RawSurfaceStorage> surface;std::unique_ptr<RawCampaignSession> session; // 실제 고정뷰/Kernel의 현재 세대다.
    std::unique_ptr<MissionScript> mission; // 원문 Guide의 현재 Script이며 모드는 같은 surface.links.mission을 직접 쓴다.
    State pending;std::shared_ptr<MissionFortLoadRequest> request; // Guide만 반납하는 호스트 요청 owner다.
    FontStore fonts{input.assets.files,"Arial"};RawButtonControls buttons{OriginalEdition::Patch1078};RawMenuControls* menus{}; // 실제 공통 등록/메뉴 전역이며 HUD 공장은 이 검사의 명시 외부 경계다.
    std::uint32_t replacements{},refreshes{},retirements{}; // UI 외부 효과와 실제 Runtime 반납 위치만 관측한다.
    GuideWorldScene484() { // 실제 Client Kernel/권한/보조/Player/로컬 상태를 같은 표면에 연결한다.
        auto game=client.GameplayGlobals();surface=MakeRawSurfaceStorage(input.assets.game,game->Authority(),game->GameAux());surface->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());
        RawMenuHooks ui;ui.command=[](auto,auto,auto) { throw std::logic_error("수명 검사에서 메뉴 명령을 선택함"); };ui.readInt=[this](auto key,std::int32_t& value) { value=client.Configuration().GetInt(key); };
        ui.sound=[](auto) { throw std::logic_error("수명 검사에서 메뉴 음향을 선택함"); };ui.poll=[](std::uint32_t)->InputEvent { throw std::logic_error("수명 검사에서 OS 입력을 선택함"); };ui.cursor=[](auto) { throw std::logic_error("수명 검사에서 메뉴 커서를 선택함"); };
        menus=&buttons.EnableMenus(input.assets.game,fonts,std::move(ui));client.InitializeNativeAudio();client.SpGlobals()->Store().Initialize(); //실제 공통 시작의 SP 초기화는 Session 생성 이전에 한 번 실행한다.
        mission=CreateGuideMissionScript(input.config,"WorkingReload483",surface->links.mission,[owner=input.workspace](auto path) { return owner->Read(path); });client.GameplayGlobals()->world.missionPost=surface->links.mission;client.SubscriberGlobals()->MySubscriber()=client.SubscriberGlobals()->LoginSubscriber();Load(); //첫 기존 미션의 State7 후 모드와 Init의 현재 Subscriber 복사가 검사 입력이다. 이후 전환은 아래 실제 본체가 갱신한다.
    }
    void Load() { // 최초 설치와 로그인 재로드가 모두 현재 Script의 같은 작업 파일을 읽는다.
        auto services=SharedKernelServices(client,external);UseWorldTestLogin(services);services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.coreDataGlobals=client.CoreDataGlobals();
        services.localStateGlobals=client.LocalStateGlobals();services.connectionRegistry=client.ConnectionGlobals();services.spGlobals=client.SpGlobals();services.randomGlobals=client.RandomGlobals();services.adoptInitializedPlayers=client.PlayerRecords()->Initialized(); //실제 State cleanup이 만든 다음9기록을 새 Session에서 중복 재할당하지 않는다.
        services.kencloudGlobals=client.KencloudGlobals();services.connectionActivityGlobals=client.ConnectionActivityGlobals(); //실제 정리와 Session이 같은 장기 표시/연결 활동 전역을 조회한다.
        services.connectionFrameNumber=[this] { return static_cast<std::uint32_t>(client.Time().number); };services.removeConnectionStatus=[this](auto,auto) { ++external.notices; }; //같은 현재 프레임과 OS 밖 상태 UI 통지 관측이다.
        services.templateStorage=templates;services.missionFortRequest=request;services.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(mission->ReadFortImage(mission->FortName()));services.missionFortRead=mission->FortReader();
        session=std::make_unique<RawCampaignSession>(input.assets.game,input.assets.files,FortTemplate::Parse(*services.missionFortImage,input.assets.game.TypeTable()),mission->FortName(),std::move(services));session->LoadLocalConnection();session->Cursor().SetEffects(cursorUi.Effects()); //State의 실제 Cancel도 게임 본체를 사용하며 화면/하드웨어 요청만 관측한다.
        CHECK(session->LocalConnectionPhase()==14 && session->PlayerKnown()==1 && surface->Borrowed() && client.GameplayGlobals()->Borrowed());
    }
    GuideMissionTransitionHooks Guide() { // 원문 Guide 해제는 Session을 닫지 않고 이전 요청/Script만 반납한다.
        GuideMissionTransitionHooks hooks;hooks.current=[this] { return mission!=nullptr; };hooks.finish=[] { MissionStartState state;RunTutorialMissionStage(state,6,{}); }; //실제 Tutorial의 빈 가상 종료를 사용한다.
        hooks.release=[this] { ReleaseMissionFortLoad(pending,request);ReleaseGuideMissionScript(mission,surface->links.mission,input.config); };
        hooks.create=[this](auto name) { mission=CreateGuideMissionScript(input.config,name,surface->links.mission,[owner=input.workspace](auto path) { return owner->Read(path); });CHECK(mission && mission->Loaded()); };return hooks;
    }
    void Init() { // 같은 실제 Template 정리→요청→Header 순서를 살아 있는 Session에서 실행한다.
        TutorialInitState init;init.name=mission->Name();std::uint32_t flags{},demo{},tutorial{},number{},seen{};std::array<std::uint8_t,87> options{};std::array<std::uint32_t,256> allowed{}; // Init의 명시 전역 입력과 실제 Header 출력이다. Subscriber는 같은 Client DWORD를 직접 쓴다.
        TutorialInitHooks hooks;hooks.clearTemplates=[this] { CHECK(session && surface->Borrowed());templates->ClearAll();CHECK(!templates->OwnedImages()); };
        hooks.get=[this](auto key,std::optional<std::string_view> fallback) { const auto text=mission->Get(key).value_or("");return text.empty() && fallback ? std::string(*fallback) : text; };
        hooks.set=[this](auto key,auto value) { mission->Set(key,value); };hooks.resetOptions=[&] { ResetMissionBattleOptions(OriginalEdition::Patch1078,options); };
        hooks.requestFort=[this](auto name) {
            request=std::make_shared<MissionFortLoadRequest>();MissionFortRequestHooks effects;effects.selectFort=[this](auto fort) { request->fort=fort; };effects.duplicate=[this](auto& packet) { return CaptureMissionFortZone(*request,packet); };
            const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};const std::uint32_t port=6799;auto& globals=*client.LocalStateGlobals(); // 실제 다음144BYTE와 순번을 쓴다.
            WriteMissionFortRequest(name,{globals.State().next,globals.ZonePending(),address,port},effects);request->next=globals.State().next;request->sequence=globals.ZonePending();pending.Post({"LoadMissionFort",request->fort});
        };
        hooks.header=[&] { CHECK(pending.Pending() && request && request->zone.size()==86);ApplyMissionTechnology(*mission,input.assets.game.TypeTable(),std::span(allowed)); };
        InitializeTutorialMission(init,flags,demo,{tutorial,number,seen,client.SubscriberGlobals()->MySubscriber(),client.SubscriberGlobals()->LoginSubscriber()},hooks);CHECK(flags==1 && client.SubscriberGlobals()->MySubscriber()==1 && request && request->next[0]==7);
    }
    void Cleanup(std::uint32_t reason) { // 실제 전체 Native 부모/AI·Deck·Kernel/Core/Pool/Player를 실행한다. HUD 교체/표시 자체는 기존 UI 검사 범위다.
        ClientStateWorldDevices devices;devices.cancelInitialCursor=[] { throw std::logic_error("살아 있는 Session 대신 초기 커서를 선택함"); };
        devices.killGumps=[this] { buttons.KillStateGumps(); };devices.cleanupGumps=[this] { buttons.CleanupGumps(); };
        devices.replaceUber=[this] { ++replacements; };devices.refreshSp=[this] { ++refreshes; };devices.assertion=[](auto) { throw std::logic_error("수명 검사에서 예상하지 않은 모드 진단"); };
        devices.retireRuntime=[this] { CHECK(session && templates->OwnedImages()==0 && pending.Pending() && request && request->zone.size()==86 && mission);++retirements;session.reset(); };
        CleanupClientStateWorld(reason,{client,*surface,*templates,*menus,client.WorldAudio()},surface,devices);
    }
};
}

TEST_CASE(CampaignGuideWorldLifetime_InitClearsActualManagersBeforePriorSessionRetires) { // 실제 살아 있는 객체/프로세스/고정뷰를 Init이 조기 반납하지 않는다.
    GuideWorldScene484 scene;auto* old=scene.session.get();const auto base=scene.client.PlayerRecords()->Base();const auto kernel=scene.client.GetKernel().Size();const auto islands=scene.session->IslandCount();CHECK(kernel>0 && islands>0 && scene.templates->OwnedImages()>0);
    BeginGuideMission("WorkingReload483",scene.Guide());CHECK(scene.session.get()==old && scene.surface->Borrowed() && scene.templates->OwnedImages()>0);
    scene.Init();CHECK(scene.session.get()==old && scene.client.PlayerRecords()->Base()==base && scene.client.GetKernel().Size()==kernel && scene.session->IslandCount()==islands && scene.templates->OwnedImages()==0);
    auto request=scene.request;const auto zone=request->zone;scene.Cleanup(1);CHECK(!scene.session && !scene.surface->Borrowed() && !scene.client.GameplayGlobals()->Borrowed());
    CHECK(scene.request==request && scene.request->zone==zone && !request->consumed && scene.pending.Pending() && scene.mission && scene.surface->links.mission && scene.replacements==1 && scene.retirements==1);
    const auto queued=scene.pending.Take();CHECK(queued && queued->command=="LoadMissionFort" && queued->argument=="WorkingReload483");scene.Load();CHECK(request->consumed && request->zone.empty() && scene.session->IslandCount()>0);
}

TEST_CASE(CampaignGuideWorldLifetime_GuideCloseBeforeStateOneKeepsPriorSessionAndNewZone) { // Close는 원문처럼 finish 없이 Script만 닫고 실제 State1 정리를 기다린다.
    GuideWorldScene484 scene;auto* old=scene.session.get();scene.Init();auto request=scene.request;std::weak_ptr<MissionFortLoadRequest> weak=request;
    auto guide=scene.Guide();bool finished{};guide.finish=[&] { finished=true; };CloseGuideMission(guide);
    CHECK(!finished && !scene.mission && !scene.surface->links.mission && !scene.request && !scene.pending.Pending() && scene.session.get()==old && scene.surface->Borrowed() && !weak.expired());
    scene.pending.Post({"Tell","KeepOtherOwner"});scene.request=request;ReleaseMissionFortLoad(scene.pending,scene.request);const auto other=scene.pending.Take();CHECK(other && other->command=="Tell" && other->argument=="KeepOtherOwner");
    request.reset();CHECK(weak.expired()); // 아직 Session에 설치하지 않은 요청에는 다른 소유권이 없어야 한다.
}

TEST_CASE(CampaignGuideWorldLifetime_RequestReleaseKeepsSessionOwnedImageAlive) { // Session이 이미 빌린 이미지는 Guide 반납만으로 사라지지 않는다.
    State pending;auto request=std::make_shared<MissionFortLoadRequest>();auto sessionOwner=request;std::weak_ptr<MissionFortLoadRequest> weak=request;pending.Post({"LoadMissionWorld",{}});
    ReleaseMissionFortLoad(pending,request);CHECK(!request && !pending.Pending() && !weak.expired());sessionOwner.reset();CHECK(weak.expired());
}
