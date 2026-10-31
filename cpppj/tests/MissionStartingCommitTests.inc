// 실제 World의 State7 몸체가 같은 원문 공통 확정과 진행 안내 위치를 사용하는지 관측한다.

// 이전 phase/요청의 모든144BYTE와 Zone 반납의 current/next/immediate 차이를 구별한다.
TEST_CASE(MissionStartingCommit_WholeBlocksProgressAndDeferredConnection) {
    LocalMissionStateBlock previous{},old{};previous.fill(0x11223344U);previous[0]=4;old.fill(0x55667788U);std::size_t progress{}; // 원문 확정 전 그대로 남아야 할 서로 다른 두 블록이다.
    MissionAsyncScene scene([&](auto& active) {
        ++progress;const auto state=active.state->State();CHECK(state.phase==previous && state.old==old);CHECK(state.current[0]==7 && state.current[32]==0 && state.next[0]==0 && state.next[32]==0 && state.immediate[0]==8 && state.immediate[1]==20 && state.immediate[32]==1);
        CHECK(active.request->consumed && active.request->zone.empty() && active.modes==std::vector<std::uint32_t>{2} && active.state->ActivityEnd()==active.devices.wall+10.0 && !active.gameplay->player.playerKnown && active.packets==0 && !active.world->Session().HasCursorRuntimeForState());
        CHECK(std::any_of(active.palette.begin(),active.palette.end(),[](const auto& color) { return color.red || color.green || color.blue; })); // 진행 안내는 선택 색을 전달하는 실제 팔레트보다 앞이다.
    });
    auto state=scene.state->State();state.phase=previous;state.old=old;scene.request->next[2]=0xa1b2c3d4U;scene.request->next[35]=0x1234abcdU;state.next=scene.request->next; // 인접하지 않은 stale 인자도 전체 복사로 보존해야 한다.
    auto seven=scene.request->next;seven[32]=0;auto eight=scene.request->next;eight[0]=8;eight[1]=20;auto next=eight;next[0]=9;auto cleared=eight;cleared[0]=0; // 원문의 명시 쓰기만 반영한 전체 블록 기대값이다.
    scene.world->StartLocalConnection();CHECK(progress==1 && state.old==seven && state.phase==eight && state.next==next && state.current==cleared && state.immediate==cleared);CHECK(scene.world->Connecting() && scene.packets==0 && !scene.world->Session().HasCursorRuntimeForState());
    CHECK(std::all_of(scene.palette.begin(),scene.palette.end(),[](const auto& color) { return !color.red && !color.green && !color.blue; })); // 안내 복귀 뒤 같은 실제 팔레트 callback이 실행됐음을 구별한다.
    CHECK(scene.world->Session().PollLocalConnectionState());scene.world->Session().PumpLocalConnection();CHECK(scene.world->CompleteLocalConnection() && scene.world->Session().LocalConnectionReady() && progress==1 && scene.closed>0);
}

// 안내 장치 누락/실제 안내 실패는7 확정과8 접속 전에 멈추며 앞선 쓰기/부분 수명을 보존한다.
TEST_CASE(MissionStartingCommit_ProgressFailureRetainsUncommittedWrites) {
    for (const bool missing:{true,false}) { // 누락된 필수 장치와 호출 중 명시 실패를 서로 다른 실제 World에서 확인한다.
        std::size_t progress{};std::function<void(MissionAsyncScene&)> show;if (!missing) show=[&](auto&) { ++progress;throw std::logic_error("집중 검사 진행 안내 생성 실패"); };
        MissionAsyncScene scene(std::move(show));auto state=scene.state->State();state.phase.fill(0x21324354U);state.phase[0]=4;state.old.fill(0x65768798U);const auto previous=state.phase,old=state.old;
        bool rejected{};try { scene.world->StartLocalConnection(); }catch (const std::logic_error& error) { rejected=std::string(error.what()).find(missing ? "실제 미션 로그인 장치 미연결" : "집중 검사 진행 안내 생성 실패")!=std::string::npos; }
        CHECK(rejected && progress==(missing ? 0U : 1U) && state.phase==previous && state.old==old && scene.modes==std::vector<std::uint32_t>{2});CHECK(state.current[0]==7 && state.current[32]==0 && state.next[32]==0 && state.immediate[0]==8 && state.immediate[1]==20 && state.immediate[32]==1);
        CHECK(scene.request->consumed && scene.world->Connecting() && scene.packets==0 && !scene.world->Session().HasCursorRuntimeForState() && !scene.world->CompleteLocalConnection());CHECK(std::any_of(scene.palette.begin(),scene.palette.end(),[](const auto& color) { return color.red || color.green || color.blue; }));
        scene.world.reset();CHECK(!scene.gameplay->Borrowed() && !scene.surface->Borrowed() && state.phase==previous && state.old==old); // 예외 반납이 장기 Native 블록을성공 상태로 바꾸지 않는다.
    }
}
