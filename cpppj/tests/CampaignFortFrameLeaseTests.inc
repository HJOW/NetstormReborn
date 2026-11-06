// 원문 State1/3 준비 중의 실제 GameWorld는 다음 State까지 프레임/커서/패킷을 공개하지 않는다.
#include "client/GameWorld.h"

// 같은 Client/Session의 checkpoint·UI·공통State·최종공개를 실제 GameWorld 어댑터로 이어간다.
TEST_CASE(CampaignFortFrameLease_ActualGameWorldClosesUntilStateCommit) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));client.StaticStrings()->Initialize(10240);InitializeClientChatManager(client);
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"fort-frame-954"),{assets.root,assets.root.parent_path()/"originalCD"});work.Prepare("Checkpoint385",input);CheckpointDevices devices;auto checkpoint=devices.Effects(client,work);auto storage=std::make_shared<CampaignTemplateStorage>();auto services=devices.Make(client,storage,input);std::uint32_t cameraCommits{};services.cameraCommit=[&](const auto&) { ++cameraCommits; }; // 실제 카메라 계산 뒤 외부 표시 적용만 관측한다.
    const auto surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.localStateGlobals=client.LocalStateGlobals();services.editorPlayerUi=MakeEditorPlayerClientHooks(client);services.fortTechnologyUi=[&] { RefreshClientChatRankPrefix(client,assets.game,*surface); };
    GameWorld world(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),{},"deferred.fort",std::move(services),{1024,768},true);auto& session=world.Session();std::uint32_t frames{};world.frameTime=[&] { ++frames;return FrameTime{}; };
    CHECK(world.Connecting() && world.PreparingFort() && !session.HasCursorRuntimeForState());world.PrepareKernelFrame();world.FinishKernelFrame();CHECK(frames==0 && world.Report().find("raw_fort_pending\t0\t0")!=std::string::npos);
    CHECK(CheckpointRejects<std::logic_error>([&] { world.CompleteFortInstall(*client.LocalStateGlobals()); }));CHECK(world.PreparingFort() && world.Connecting());
    EditorInstallStateContext scene(client);InputQueue inputQueue;std::uint32_t tailCalls{};auto tail=EditorInstallTail(session,tailCalls);LocalMissionStateHooks hooks;hooks.logState=[](auto value) { CHECK(value==1); };hooks.parentState=[&](auto selected) {
        CHECK(selected==1 && world.PreparingFort());CHECK(session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work));
        session.WithExistingEditorInstallUi(scene.owner,inputQueue,tail,[&] { world.Resize(1024,768);CHECK(world.PreparingFort() && world.Connecting()); });
        CHECK(CheckpointRejects<std::logic_error>([&] { world.CompleteFortInstall(scene.owner); }));return LocalMissionParentResult::Commit;
    };
    RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state.Run();CHECK(world.PreparingFort() && tailCalls==5);world.CompleteFortInstall(scene.owner);
    CHECK(!world.Connecting() && !world.PreparingFort() && session.HasCursorRuntimeForState() && session.IslandCount()>0 && cameraCommits==1);static_cast<void>(session.Cursor());
    CHECK(CheckpointRejects<std::logic_error>([&] { world.CompleteFortInstall(scene.owner); }));world.PrepareKernelFrame();world.FinishKernelFrame();CHECK(frames==1);
}
