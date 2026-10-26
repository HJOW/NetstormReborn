// 같은 기존 Editor 검사 도우미를 사용하되 입력 Fort 파일 없이 실제 새 manager와 Client 전역을 설치한다.
#include "client/GameMenuCommands.h"
#include "client/ClientSubscriberGlobals.h" // 실제 두 Subscriber DWORD를 검사가 직접 조회한다.

namespace { // 새 Session과 장치의 수명을 동기 검사 범위에 함께 유지한다.
// 원본 파일을 입력으로 가장하지 않고 FortTemplate{}와 지연 설치를 사용한다. 원본 자산은 타입/그림 읽기에만 사용한다.
template<class Body> void WithNewFortSession(Body body,const std::function<void(RawCampaignSessionServices&)>& before={}) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));
    client.StaticStrings()->Initialize(10240);client.SpGlobals()->Store().Initialize();InitializeClientPlayerRecords(client);InitializeClientChatManager(client);client.Configuration().SetInt("registeredSubId",7);
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"new-fort-472"),{assets.root,assets.root.parent_path()/"originalCD"});CheckpointDevices devices;devices.current=devices.pending="Fresh472";auto checkpoint=devices.Effects(client,work);
    work.EnsureDirectory("d"); // 실제 호스트와 같이 저장 디렉터리만 준비하며 Fort 파일은 미리 만들지 않는다.
    auto storage=std::make_shared<CampaignTemplateStorage>();auto services=devices.Make(client,storage,{});services.missionFortImage.reset(); // 없는 파일을 byte stream으로 공급하지 않는다.
    const auto surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
    services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();
    services.templateStateBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa5}); }; // 검사 입력의 미정 할당 바이트를 명시하며 실제 State 필드 쓰기는 제품 본체에 맡긴다.
    services.coreDataGlobals=client.CoreDataGlobals();services.spGlobals=client.SpGlobals();services.localStateGlobals=client.LocalStateGlobals();services.editorPlayerUi=MakeEditorPlayerClientHooks(client);
    services.fortTechnologyUi=[&] { RefreshClientChatRankPrefix(client,assets.game,*surface); };
    if (before) before(services);
    RawCampaignSession session(assets.game,assets.files,FortTemplate{},"new-fort-request",std::move(services));session.SetSp(1,0.0F); // known1의 명시 시작 SP 입력이다. 미정 할당 비트를 정상 SP라고 가정하지 않는다.
    body(session,client,checkpoint,work,*storage,devices);
}
}

// 파일 꼬리 끝에서 멈춰 World 전의 실제 초기 자료/SP/Core/Subscriber/Money 저장과 공유 난수 소비를 대조한다.
TEST_CASE(CampaignNewFort_ActualDataOwnersAndSavedFileBeforeWorld) {
    WithNewFortSession([](auto& session,Client& client,auto checkpoint,EditorFiles& work,CampaignTemplateStorage& storage,auto&) {
        session.SetSp(1,1234.0F);const auto core=client.CoreDataGlobals()->Current();std::fill(core.begin(),core.end(),std::uint8_t{0xa5});client.RandomGlobals()->FortCoreSeed()=0xa55a0000;
        checkpoint.file.title=[] { throw CheckpointStop("new-data-before-world"); };bool stopped{};
        try { session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work); }catch (const CheckpointStop& error) { stopped=std::string_view(error.what())=="new-data-before-world"; }
        CHECK(stopped && !session.ExistingEditorCheckpointReady() && session.CurrentTechnologyParent()==0 && session.IslandCount()==0);
        // 원문0040eec0은 Fort/Editor 모드에서 입력 차감 대신194162로 고정한다. 새 부모가 직접0으로 덮어쓰면 안 된다.
        CHECK(session.Sp(1)==194162.0F && std::bit_cast<float>(client.SpGlobals()->Store().Get(1))==194162.0F);CHECK(client.CoreDataGlobals()->Current().data()==core.data() && client.CoreDataGlobals()->Active()==1);
        CHECK(std::all_of(core.begin(),core.end(),[](auto value) { return value==0; }));CHECK(session.CurrentSubscriber()==7 && client.SubscriberGlobals()->MySubscriber()==7 && session.CurrentHandle()=="Unnamed");
        // 고정 time32=946684800의 원본 CRT103회/공유 난수 명세에서 독립 계산한 두 소비 결과다.
        CHECK(client.RandomGlobals()->FortCoreSeed()==(0xa55a0000U|30897U));CHECK(client.RandomGlobals()->Game().State()==2857353399U);
        CHECK(storage.OwnedImages()==1 && storage.Template(1).Section("CoreData").empty());const auto& fort=storage.Template(1);const auto state=fort.Section("State");CHECK(state.size()>=28 && (state[0]&1) && state[17]==2);
        CHECK(fort.Section("Territory").size()==120 && std::all_of(fort.Section("Territory").begin(),fort.Section("Territory").end(),[](auto value) { return value==0; }));
        const auto money=fort.Section("Money");CHECK(money.size()==4 && std::bit_cast<float>(EditorWireDword(money,0))==502.0F);CHECK(storage.ReadSubscriberName(1)=="Unnamed");
        const auto path=work.ResolvePath("d/Fresh472.fort");const auto bytes=ReadFileBytes(path);const auto parsed=FortTemplate::Parse(bytes,netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game.TypeTable());CHECK(parsed.sections==fort.sections);
        CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
    });
}

// 첫 제목 장치 실패는 그 전 State|1/파일/번역/ID=-1 효과만 남기고 SP/Core/seed와 World는 아직 변경하지 않는다.
TEST_CASE(CampaignNewFort_FirstTitleFailureKeepsOnlyNativePrefix) {
    WithNewFortSession([](auto& session,Client& client,const auto& checkpoint,EditorFiles& work,CampaignTemplateStorage& storage,auto&) {
        session.SetSp(1,900.0F);const auto core=client.CoreDataGlobals()->Current();std::fill(core.begin(),core.end(),std::uint8_t{0x5a});client.RandomGlobals()->FortCoreSeed()=0x56781234;
        bool stopped{};try { session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work); }catch (const CheckpointStop& error) { stopped=std::string_view(error.what())=="new-data-first-title"; }
        CHECK(stopped && session.CurrentSubscriber()==0xffffffffU && session.CurrentHandle()=="Unnamed" && session.Sp(1)==900.0F);
        CHECK(std::all_of(core.begin(),core.end(),[](auto value) { return value==0x5a; }) && client.RandomGlobals()->FortCoreSeed()==0x56781234);
        CHECK(storage.OwnedImages()==1 && storage.Template(1).Section("Subscriber").empty() && (storage.State(1).bytes[0]&1));CHECK(work.Exists(work.ResolvePath("d/Fresh472.fort").string())==5);
        CHECK(!session.ExistingEditorCheckpointReady() && session.CurrentTechnologyParent()==0);
    },[](auto& services) { services.install.refreshWindowTitle=[] { throw CheckpointStop("new-data-first-title"); }; });
}

// 빈 새 맵도 같은 원문 State 확정과 커서/입력 장치 준비를 거치고 섬 추가를 다음 입력 큐에 예약한다.
TEST_CASE(CampaignNewFort_EmptyWorldCheckpointStateCommitAndAddIslandQueue) {
    WithNewFortSession([](auto& session,Client& client,const auto& checkpoint,EditorFiles& work,CampaignTemplateStorage& storage,auto&) {
        CHECK(GameMenuScriptCode("NewFort")==0x02000079U);EditorInstallStateContext scene(client);InputQueue input;std::uint32_t calls{};const auto tail=EditorInstallTail(session,calls);
        LocalMissionStateHooks hooks;hooks.logState=[](auto) {};hooks.parentState=[&](auto selected) {
            CHECK(selected==1);session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);CHECK(session.ExistingEditorCheckpointReady() && session.IslandCount()==0);
            session.WithExistingEditorInstallUi(scene.owner,input,tail,[] {});return LocalMissionParentResult::Commit;
        };
        RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state.Run();session.CommitExistingEditorInstall(scene.owner);
        CHECK(scene.owner.State().next[0]==4 && scene.owner.State().current[0]==0 && session.CurrentTechnologyParent()!=0);
        CHECK(storage.State(1).bytes[17]==2 && !(storage.State(1).bytes[0]&1));CHECK(session.IslandCount()==0);
        session.SnapshotFort(1);const auto saved=ReadFileBytes(work.ResolvePath("d/Fresh472.fort"));const auto parsed=FortTemplate::Parse(saved,netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game.TypeTable());CHECK(parsed.sections==storage.Template(1).sections);
        const auto event=input.Pop(false);CHECK(event.code==0x02000022U); // 현재 실제 Add Island 명령을 같은 입력 큐에서 읽는다.
    });
}
