// 실제 부모 Poll/Run이 이전 World 반납과 새 후반 설치 사이에서도 최초 복사/로그와7/8 확정을 보유한다.
namespace { // 부모가 빌리는 시각/제목과 현재 장치의 실제 수명을 검사보다 길게 보유한다.
struct MissionParentTime {
    double game{},wall{100.25}; // Native 부모가 참조하는 서로 다른 현재 시각이다.
    std::uint8_t title{0x4d}; // 기본 분모 뒤 지우는 실제 첫BYTE의 명시 장치다.
};
std::unique_ptr<RawLocalMissionState> MakeMissionParent(MissionAsyncScene& scene,MissionParentTime& time,std::vector<std::uint32_t>& phases,std::function<void(std::span<const std::uint8_t>,std::uint32_t)> connect,std::function<LocalMissionParentResult(std::uint32_t)> fortEntry={}) { // 장기 참조는 부모보다 오래 살며 Editor 검사는 같은 State1 큰 부모 경계를 명시 제공한다.
    auto& local=*scene.state;auto& game=*scene.gameplay;auto& storage=*scene.surface;LocalMissionStateHooks hooks;hooks.wallSeconds=[&scene] { return scene.devices.wall; };hooks.connectServer=std::move(connect);
    hooks.parentState=std::move(fortEntry); // 선택한1/3 부모만 사용하며 새 미션7의 기존 입력은 빈 경계를 유지한다.
    hooks.logState=[&scene,&phases](auto value) { phases.push_back(value);if (value==7) { const auto state=scene.state->State();auto copied=state.current;copied[0]=0;CHECK(state.next==copied && state.immediate==copied && !scene.request->starting && scene.world && scene.gameplay->Borrowed()); } }; // 최초 복사/로그는 기존 World가 살아 있고 앞단이 실행되기 전이다.
    return std::make_unique<RawLocalMissionState>(OriginalEdition::Patch1078,local.State(),LocalMissionStateGlobals{game.Server(),game.Boss(),storage.ownerMode.battle,game.world.missionPost,local.SavedGame(),storage.links.mission,local.Simulconnect(),storage.ownerMode.challenge,game.player,local.ZonePending(),local.GameTicks(),local.SourceType(),local.CdPresent(),local.Blocks()[2][33],local.MetaRefresh(),local.MetaRebuild(),time.title,local.BattleByte(),local.ConnectEnd(),local.ActivityEnd(),time.game,time.wall},std::move(hooks));
}
void ReplaceMissionParentWorld(MissionAsyncScene& scene,MissionParentTime& time,std::function<void(MissionAsyncScene&)> progress) { // 실제 장기 물리를 유지하며 앞단/이전 World 반납/새 후반의 수명을 연결한다.
    auto state=scene.state->State();scene.gameplay->Camera().SelectDenominator(0,scene.redraw);scene.request->starting=CaptureLocalMissionStarting(state.current,scene.state->ZonePending(),time.title,scene.state->GameTicks(),scene.state->SourceType());state.current[2]=0xfaceb00cU; // 정리 중 current만 바뀌어도 앞서 복사한 immediate는 이전 인자를 보존해야 한다.
    scene.world.reset();CHECK(!scene.gameplay->Borrowed() && !scene.surface->Borrowed());scene.CreateWorld(std::move(progress));scene.world->PrepareLocalConnectionForState(); // 공통 Run을 새 Session 안에서 중첩하지 않는다.
}
}

// 이전 deferred World가 파괴돼도 같은 부모가 로그7→몸체→7 확정→로그8→접속→8 확정을 실행한다.
TEST_CASE(MissionStartingParent_ReplacesWorldBetweenInitialCopyAndCommit) {
    MissionAsyncScene scene;MissionParentTime time;std::vector<std::uint32_t> phases;auto state=scene.state->State();scene.request->next[2]=0x12345678U;scene.request->next[35]=0x90abcdefU;state.next=scene.request->next;state.phase.fill(0x11223344U);state.phase[0]=4;state.old.fill(0x55667788U);const auto previous=state.phase,older=state.old;scene.state->ZonePending()=scene.request->sequence;std::size_t connects{},progress{};
    auto parent=MakeMissionParent(scene,time,phases,[&](auto address,auto port) { ++connects;CHECK(state.phase[0]==7 && state.current[0]==8 && state.current[2]==0x12345678U && state.phase[2]==0xfaceb00cU && address.size()==20 && address[0]=='z' && port==6799);scene.world->Session().ConnectLocalConnectionForState(address,port); });
    parent->PollStarting([&] { CHECK(phases==std::vector<std::uint32_t>{7});ReplaceMissionParentWorld(scene,time,[&](auto&) { ++progress;CHECK(state.phase==previous && state.old==older && state.current[0]==7 && state.immediate[0]==8 && state.immediate[2]==0x12345678U && scene.packets==0); });CHECK(state.phase==previous && scene.world->Connecting() && !scene.world->Session().HasCursorRuntimeForState()); });
    auto seven=scene.request->next;seven[2]=0xfaceb00cU;seven[32]=0;auto eight=scene.request->next;eight[0]=8;eight[1]=20;auto next=eight;next[0]=9;auto cleared=eight;cleared[0]=0; // 정리 뒤 current7과 최초 복사에서 나온 즉시8의 서로 다른 인자를 보존한다.
    CHECK(phases==std::vector<std::uint32_t>({7,8}) && connects==1 && progress==1 && state.old==seven && state.phase==eight && state.next==next && state.current==cleared && state.immediate==cleared && scene.packets==0);
    CHECK(scene.world->Session().PollLocalConnectionState());scene.world->Session().PumpLocalConnection();CHECK(scene.world->CompleteLocalConnection() && scene.world->Session().LocalConnectionReady() && phases==std::vector<std::uint32_t>({7,8}));
}

// 후반 실패와7 확정 뒤8 접속 실패는 각기 다른 원문 중간 블록을 보존하고 부분 World를 정상 반납한다.
TEST_CASE(MissionStartingParent_BodyAndConnectionFailureKeepNativeBoundary) {
    for (const bool bodyFailure:{true,false}) { // 안내 실패는7 확정 전이며 접속 실패는7 확정 뒤/8 확정 전이다.
        MissionAsyncScene scene;MissionParentTime time;std::vector<std::uint32_t> phases;auto state=scene.state->State();state.next=scene.request->next;state.phase.fill(0x12345678U);state.phase[0]=4;state.old.fill(0x90abcdefU);const auto previous=state.phase,older=state.old;scene.state->ZonePending()=scene.request->sequence;std::size_t connects{};
        auto parent=MakeMissionParent(scene,time,phases,[&](auto,auto) { ++connects;throw std::logic_error("부모 집중 접속 실패"); });bool rejected{};
        try { parent->PollStarting([&] { ReplaceMissionParentWorld(scene,time,[&](auto&) { if (bodyFailure) throw std::logic_error("부모 집중 안내 실패"); }); }); }catch (const std::logic_error& error) { rejected=std::string(error.what()).find(bodyFailure ? "부모 집중 안내 실패" : "부모 집중 접속 실패")!=std::string::npos; }
        CHECK(rejected && scene.world->Connecting() && !scene.world->CompleteLocalConnection() && scene.packets==0 && !scene.world->Session().HasCursorRuntimeForState());
        if (bodyFailure) CHECK(connects==0 && phases==std::vector<std::uint32_t>{7} && state.phase==previous && state.old==older && state.current[0]==7 && state.immediate[0]==8);
        else CHECK(connects==1 && phases==std::vector<std::uint32_t>({7,8}) && state.phase[0]==7 && state.phase[2]==0xfaceb00cU && state.old==previous && state.current[0]==8 && state.immediate[0]==0);
        scene.world.reset();CHECK(!scene.gameplay->Borrowed() && !scene.surface->Borrowed()); // 부모의 Client 참조는 두 World가 모두 반납된 뒤에도 생존한다.
    }
}
