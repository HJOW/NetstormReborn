// 실제 Guide 명령·공용 소멸·Client/GameWorld/UserInput을 연결한다. 창·전략 프레임·공격·포획·희생·승리는 실행하지 않는다.
#include "client/ClientPlayerRuntime.h"

namespace { // 기존 같은 TU의 실제 자산/로그인/장치 helper만 빌리고 Guide 해제는 제품 함수를 직접 사용한다.
// 현재 Client Config에 원본 영어 미션/요새 경로만 연결한다. 파일과 진행 저장은 쓰지 않는다.
void ConfigureGuideReleaseScript671(Client& client) {
    client.Configuration().Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
}
// 실제 World는 Guide보다 오래 소유하고 같은 입력 어댑터가 살아 있는지를 관측한다.
struct GuideReleaseWorldScene671 {
    SessionServicesFixture external; // 음향·통지·전송의 기존 명시 장치이며 게임 결과를 공급하지 않는다.
    Client client{[] { auto options=SessionLinksOptions();options.noAudio=true;return options; }()}; // Run/창을 열지 않는 실제 Client의 전역·Kernel·Player owner다.
    std::shared_ptr<CampaignTemplateStorage> templates=std::make_shared<CampaignTemplateStorage>(); // 로그인과 Guide 해제가 공유하는 실제 Template 수명이다.
    std::shared_ptr<std::array<std::uint32_t,256>> restrictions=std::make_shared<std::array<std::uint32_t,256>>(); // 실제 Session과 Guide 소멸이 함께 읽고 쓰는 금지 표다.
    std::shared_ptr<RawSurfaceStorage> surface; // Client의 권한·보조·Kernel과 같은 실제 월드 물리다.
    std::unique_ptr<MissionScript> mission; // 현재 Guide가 소유하는 실제 등록 Config다.
    std::unique_ptr<GameWorld> world; // Session·Pool·표시·선택을 소유하며 Guide 반납 때 유지한다.
    std::unique_ptr<UserInput> input; // 같은 Client/World를 빌리는 실제 사용자 입력이다.
    std::unique_ptr<CampaignGuideRuntime> runtime; // 소멸 순서가 Tutorial→Start→Script가 되는 실제 Guide 필드다.
    State pending;std::shared_ptr<MissionFortLoadRequest> request; // Guide만 반납하는 지연 요청과 호스트 큐다.
    std::uint32_t missionNumber{11},worldMissionToken{671}; // Guide 소멸이0으로 만드는 실제 번호와 미션 설치 세대다.
    std::shared_ptr<std::vector<std::uint32_t>> cursors; // 실제 선택/강조가 선택한 화면 커서 요청만 관측한다.
    GuideReleaseWorldScene671() { // 실제 공유 전역과 원본 파일 로드/로컬 로그인만 실행하며 Start·Tutorial 프레임은 진행하지 않는다.
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigureGuideReleaseScript671(client);
        auto game=client.GameplayGlobals();surface=MakeRawSurfaceStorage(assets.game,game->Authority(),game->GameAux());surface->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner());
        mission=CreateGuideMissionScript(client.Configuration(),"TheWarBegins",surface->links.mission);CHECK(mission && mission->Loaded());
        client.SpGlobals()->Store().Initialize();client.SubscriberGlobals()->MySubscriber()=client.SubscriberGlobals()->LoginSubscriber();
        auto services=SharedKernelServices(client,external);UseWorldTestLogin(services);services.surfaceStorage=surface;services.gameplayGlobals=game;services.subscriberGlobals=client.SubscriberGlobals();services.coreDataGlobals=client.CoreDataGlobals();
        services.localStateGlobals=client.LocalStateGlobals();services.connectionRegistry=client.ConnectionGlobals();services.spGlobals=client.SpGlobals();services.randomGlobals=client.RandomGlobals();services.kencloudGlobals=client.KencloudGlobals();services.connectionActivityGlobals=client.ConnectionActivityGlobals();
        services.connectionFrameNumber=[this] { return static_cast<std::uint32_t>(client.Time().number); };services.removeConnectionStatus=[this](auto,auto) { ++external.notices; };services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; // 실제 생성자의 미정 할당 입력과 외부 상태 통지만 제공한다.
        services.templateStorage=templates;services.missionRestrictions=restrictions;auto fort=FortTemplate::Parse(assets.files.Read(mission->FortPath()),assets.game.TypeTable());
        world=std::make_unique<GameWorld>(assets.game,assets.files,std::move(fort),MissionPlayers{},mission->FortName(),std::move(services),ScreenPoint{1024,768});world->paused=[this] { return client.Paused(); };cursors=BindWorldTestInput(*world);input=std::make_unique<UserInput>(client,*world);
        runtime=std::make_unique<CampaignGuideRuntime>(*mission,0,0,worldMissionToken);CHECK(runtime->guided && world->Session().LocalConnectionPhase()==14 && game->Borrowed());
        CampaignMissionStartEffects start;start.settingsBytes=[](auto,auto) { throw std::logic_error("Guide 소멸 검사에서 Start 설정을 실행함"); };start.gameSeconds=[this] { return client.Time().game; };
        start.refreshSp=start.refreshCommandGump=start.randomGeysers=start.resetCamera=[] { throw std::logic_error("Guide 소멸 검사에서 Start 효과를 실행함"); };
        runtime->start=std::make_unique<RawCampaignMissionStartBindings>(world->Session(),*mission,runtime->gate,runtime->state,world->Session().MissionStartSettings(),world->Session().MissionAiSettings(),std::move(start));
        CampaignTutorialOneEffects tutorial;tutorial.tell=[](auto)->bool { throw std::logic_error("Guide 소멸 검사에서 Tutorial 안내를 실행함"); };tutorial.closeTransient=[] { throw std::logic_error("Guide 소멸 검사에서 Tutorial UI를 실행함"); };
        tutorial.frame.gameSeconds=[this] { return client.Time().game; };tutorial.frame.paused=[this] { return client.Paused(); };tutorial.frame.dialogsClear=tutorial.frame.knowledgeOpen=tutorial.frame.guideOpen=tutorial.frame.blockingGumpOpen=[]()->bool { throw std::logic_error("Guide 소멸 검사에서 Tutorial UI 조회를 실행함"); }; // 생성자의 실제 필수 장치를 제공하되 프레임이 잘못 실행되면 빈 성공으로 넘기지 않는다.
        tutorial.frame.endDemo=[] { throw std::logic_error("Guide 소멸 검사에서 Tutorial 데모 효과를 실행함"); };tutorial.frame.pause=[this] { client.PushGamePause(); };tutorial.frame.resume=[this] { client.PopGamePause(); }; // 시각·정지는 같은 실제 Client 장치이며 이 검사는 Tutorial Frame을 호출하지 않는다.
        runtime->tutorial=std::make_unique<RawCampaignTutorialOneBindings>(world->Session(),*mission,*runtime->start,runtime->gate,runtime->state,std::move(tutorial));
        runtime->gate.tutorialNumber=3;runtime->gate.stage=6;runtime->gate.stageDirty=9;restrictions->fill(0xabcdef01U);game->DenySalvage()=-7;game->DenyAscend()=23;
        request=std::make_shared<MissionFortLoadRequest>();request->fort=mission->FortName();pending.Post({"LoadMissionFort",request->fort}); // 설치 뒤 새 Guide가 예약했으나 아직 Session이 빌리지 않은 요청이다.
    }
    GuideMissionTransitionHooks Transitions() { // Finish는 실제 빈 단계6이며 release 본문은 제품의 단일 공용 함수를 호출한다.
        GuideMissionTransitionHooks hooks;hooks.current=[this] { return runtime!=nullptr; };hooks.finish=[this] { if (!runtime) throw std::logic_error("없는 Guide의 가상 종료");RunTutorialMissionStage(runtime->state,6,{}); };
        hooks.release=[this] { ReleaseCampaignGuideMission(client,pending,mission,runtime,request,*restrictions,missionNumber,worldMissionToken,surface->links.mission); };return hooks;
    }
};
}

//원본 End 명령이 실제 Guide/지상 AI만 닫고 같은 World/UserInput과 후속 선택 경로를 보존하는지 검사한다.
TEST_CASE(CampaignGuideRelease_EndKeepsActualWorldInputAndOrdinaryAiLifetime) {
    GuideReleaseWorldScene671 scene;auto& session=scene.world->Session();auto core=session.Core();auto* world=scene.world.get();auto* input=scene.input.get();const auto playerBase=scene.client.PlayerRecords()->Base();const auto* pool=core.pool.Bytes().data();const auto kernel=scene.client.GetKernel().Size();const auto islands=session.IslandCount();
    CHECK(&session.MissionStartSettings().DenySalvage()==&scene.client.GameplayGlobals()->DenySalvage() && &session.MissionStartSettings().DenyAscend()==&scene.client.GameplayGlobals()->DenyAscend());
    const auto ai=session.CreateAi(2,SessionAiSettings("Guide release AI","sunCannon"));CHECK(ai && session.AiMemory().Contains(ai) && session.Participation().Brain(2)==ai);const auto freed=session.AiMemory().Frees().size();const auto missionPost=scene.client.GameplayGlobals()->world.missionPost;const auto time=scene.client.Time();const auto paused=scene.client.Paused();const auto state=scene.client.LocalStateGlobals()->State().next;std::weak_ptr<MissionFortLoadRequest> request=scene.request;
    GuideMissionCommandHooks commands;commands.end=[&] { EndGuideMission(scene.Transitions()); };CHECK(DispatchGuideMissionCommand({kMissionEndCommand,0,0},commands));
    CHECK(!scene.runtime && !scene.mission && !scene.request && request.expired() && !scene.pending.Pending() && !scene.surface->links.mission && scene.client.Configuration().GetInt("global.inMission")==0);
    CHECK(scene.missionNumber==0 && scene.worldMissionToken==0 && scene.client.GameplayGlobals()->DenySalvage()==0 && scene.client.GameplayGlobals()->DenyAscend()==0);CHECK(std::all_of(scene.restrictions->begin(),scene.restrictions->begin()+255,[](auto value) { return value==0; }) && scene.restrictions->back()==0xabcdef01U);
    CHECK(scene.world.get()==world && scene.input.get()==input && &scene.client.GameplayGlobals()->RequireSession()==&session && scene.surface->Borrowed() && scene.client.PlayerRecords()->Base()==playerBase && core.pool.Bytes().data()==pool && session.IslandCount()==islands && scene.client.GetKernel().Size()==kernel);
    CHECK(!session.Participation().Brain(2) && !session.AiMemory().Contains(ai) && session.AiMemory().Frees().size()==freed+18 && (session.PlayerRecords()[2*180+0x50]&0x20U));CHECK(!session.Participation().BrainReturnedForState(2)); // 일반 Guide 소멸의18할당 반납은 State 증빙 소비가 아니다.
    CHECK(scene.client.GameplayGlobals()->world.missionPost==missionPost && scene.client.Paused()==paused && scene.client.Time().game==time.game && scene.client.Time().wall==time.wall && scene.client.Time().number==time.number && scene.client.LocalStateGlobals()->State().next==state);
    const auto priest=session.FindPriestParticipation(1);const auto* object=scene.world->Object(priest);CHECK(priest && object && scene.world->InputMode()==0 && !scene.client.Paused());if (!priest || !object) return;
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto frame=static_cast<std::size_t>(object->frame);const auto number=static_cast<std::size_t>(object->type);const auto rect=core.display.Bounds(number,static_cast<int>(frame),static_cast<float>(object->x),static_cast<float>(object->y));
    const auto ink=IndependentOpaquePickPoint(assets.game.Shapes().Decode(number-kFirstAssetTypeNumber,frame),{rect.left,rect.top},scene.world->MapView());CHECK(scene.world->Pick(ink)==priest);scene.input->Event({InputCode::kLeftButton,ink.x,ink.y});CHECK(scene.world->Selected()==priest && scene.input.get()==input); // 동일한 실제 UserInput의 좌선택만 실행하며 이동·공격·포획·희생은 하지 않는다.
    const auto next=session.CreateAi(2,SessionAiSettings("After Guide AI","sunCannon"));CHECK(next && session.Participation().Brain(2)==next);session.RemoveAi(2);CHECK(!session.Participation().Brain(2)); // 일반 반납 뒤에도 살아 있는 Session의 AI 생성 관문은 열려 있다.
}

//World가 없을 때도 실제 초기화된 Player9와 등록 Config를 사용하며 자기 시계·missionPost·Player 할당은 보존한다.
TEST_CASE(CampaignGuideRelease_NoWorldUsesActualPlayersAndUnwindRetiresBase) {
    for (const bool invalidBrain:{false,true}) { // 실제 NULL brain과 소유자 없는 비NULL brain 진단의 두 입력을 별도 Client 수명에서 검사한다.
        ClientOptions options=SessionLinksOptions();options.noAudio=true;Client client(std::move(options));ConfigureGuideReleaseScript671(client);InitializeClientPlayerRecords(client);auto game=client.GameplayGlobals();bool nativeMission{};auto mission=CreateGuideMissionScript(client.Configuration(),"TheWarBegins",nativeMission);auto runtime=std::make_unique<CampaignGuideRuntime>(*mission,0,0,671);
        std::array<std::uint32_t,256> restrictions;restrictions.fill(0x12345678U);std::uint32_t number=4,worldToken=671;State pending;auto request=std::make_shared<MissionFortLoadRequest>();pending.Post({"LoadMissionWorld",{}});game->DenySalvage()=7;game->DenyAscend()=-3;game->world.missionPost=true;client.PushGamePause();runtime->gate.scriptPaused=1;const auto playerBase=client.PlayerRecords()->Base(); // 실제 자기 정지 표식이 있어도 원문 소멸에는 Resume가 없다.
        if (invalidBrain) SessionLinksPut(client.PlayerRecords()->Records(),180+0xb0,0x76543210U);const auto players=std::vector<std::uint8_t>(client.PlayerRecords()->Records().begin(),client.PlayerRecords()->Records().end());bool diagnosed{},finished{};GuideMissionTransitionHooks transition;
        transition.current=[&] { return runtime!=nullptr; };transition.finish=[&] { finished=true; };transition.release=[&] { ReleaseCampaignGuideMission(client,pending,mission,runtime,request,restrictions,number,worldToken,nativeMission); };
        try { CloseGuideMission(transition); }catch (const std::logic_error& error) { diagnosed=std::string_view(error.what()).find("비NULL brain")!=std::string_view::npos; }
        CHECK(diagnosed==invalidBrain && !finished && !runtime && !mission && !request && !pending.Pending() && !nativeMission && client.Configuration().GetInt("global.inMission")==0);
        CHECK(!game->Borrowed() && number==0 && worldToken==0 && game->DenySalvage()==0 && game->DenyAscend()==0 && restrictions[0]==0 && restrictions[254]==0 && restrictions[255]==0x12345678U);
        CHECK(client.PlayerRecords()->Base()==playerBase && std::equal(players.begin(),players.end(),client.PlayerRecords()->Records().begin()) && client.Paused() && game->world.missionPost);client.PopGamePause(); // Guide 소멸이 소유하지 않는 실제 정지 깊이는 검사 호출자가 나중에 반납한다.
    }
}

//후속 State가 이미 반납 증빙을 소비한 NULL brain은 전체 삭제에서 자연 반환하며 개별 reset의 중복 관문은 유지한다.
TEST_CASE(CampaignGuideRelease_AllBrainsAfterStateProofPreservesLiveFields) {
    GuideReleaseWorldScene671 scene;auto& session=scene.world->Session();const auto ai=session.CreateAi(2,SessionAiSettings("State proof AI","sunCannon"));CHECK(ai && session.AiMemory().Contains(ai));
    for (std::uint32_t owner=1;owner<9;++owner) { auto pass=session.ResetAiForState(owner);session.ReturnAiForState(std::move(pass));CHECK(session.Participation().BrainReturnedForState(owner)); } // 실제 여덟 owner의 정상 reset 복귀를 같은 모듈에서만 소비한다.
    const auto freed=session.AiMemory().Frees().size();const auto players=std::vector<std::uint8_t>(session.PlayerRecords().begin(),session.PlayerRecords().end());const auto pool=std::vector<std::uint8_t>(session.Core().pool.Bytes().begin(),session.Core().pool.Bytes().end());auto* world=scene.world.get();auto* input=scene.input.get();const auto kernel=scene.client.GetKernel().Size();
    session.Participation().RemoveAllBrains();CloseGuideMission(scene.Transitions());session.Participation().RemoveAllBrains();
    CHECK(!scene.runtime && !scene.mission && scene.world.get()==world && scene.input.get()==input && scene.client.GetKernel().Size()==kernel && session.AiMemory().Frees().size()==freed);
    CHECK(std::equal(players.begin(),players.end(),session.PlayerRecords().begin()) && std::equal(pool.begin(),pool.end(),session.Core().pool.Bytes().begin()));CHECK(Throws([&] { session.ResetAiForState(2); })); // aggregate의 자연 반환을 새 State 증빙이나 전체 World 반납으로 합산하지 않는다.
}
