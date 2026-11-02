// 실제 Client/UserInput/커서에서 큐 사건 outside와 현재 포인터 폴링 outside를 구별한다.
TEST_CASE(Outside761_ActualUserInputQueueAndCurrentCursor) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyCursorFactoryInput(fort);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();SessionCursorUi ui;auto& cursor=session.Cursor();auto& drop=session.CursorDropSettings();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.SetSp(1,5000);CursorEnergyInput(session);cursor.Take(ReadyCursorFactoryForm(session),0x41);const auto point=CursorPlacementPoint(session,ui);
    ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));UserInput input(client,world);
    client.PushGamePause();const auto pausedDirection=drop.direction;
    // 실제 공통 정지는 inside/outside의 활성 커서 회전/배치보다 먼저 적용된다.
    for (const auto outside:std::array<std::uint32_t,2>{0,InputCode::kOutside}) {
        // 우클릭·좌우 키·왼쪽 배치 모두 정지 중 실제 커서 상태를 바꾸지 않는다.
        for (const auto key:std::array<std::uint32_t,4>{InputCode::kRightButton,0x250000U,0x270000U,InputCode::kLeftButton}) { CHECK(!input.Event({outside|key,ui.mouse.x,ui.mouse.y}));CHECK(drop.direction==pausedDirection && !drop.lockDepth && client.Input().Empty()); }
    }
    client.PopGamePause();
    CHECK(!input.Event({InputCode::kOutside|InputCode::kRightButton,ui.mouse.x,ui.mouse.y}));CHECK(drop.direction==2);
    CHECK(!input.Event({InputCode::kOutside|0x250000U,0,0}));CHECK(drop.direction==0);
    CHECK(cursor.HandleInput({InputCode::kOutside|0x270000U,0,0},0) && drop.direction==2);CHECK(cursor.HandleInput({InputCode::kOutside|InputCode::kRightButton,0,0},1) && drop.direction==0);
    CHECK(!cursor.HandleInput({InputCode::kOutside|InputCode::kRelease|InputCode::kRightButton,0,0},0));CHECK(drop.direction==0);
    input.UpdateCursor({InputCode::kOutside,900,700});CHECK(drop.x==point.x && drop.y==point.y);CHECK(world.SelectionGlobals().mouse.x==900 && world.SelectionGlobals().mouse.y==700); // 원래 폴링 좌표 저장→outside 위치 갱신 생략을 보존한다.
    cursor.Cancel();CHECK(drop.lockDepth==1);
    for (const auto key:std::array<std::uint32_t,5>{'D','Q','H',0x730000U,0x750000U}) { // 각 키는 원문과 같은 일반 큐를 사용하며 현재 활성 커서/위치와 무관하다.
        CHECK(client.Input().Empty());CHECK(!input.Event({key|InputCode::kOutside,37,-19}));const auto event=client.Input().Pop(false);const auto expected=key=='D' ? 0x02000107U : key=='Q' ? 0x020000f4U : key==0x750000U ? 0x020000a0U : 0x020000aeU;CHECK(event.code==expected && !event.x && !event.y && client.Input().Empty());
        CHECK(!input.Event({key|InputCode::kOutside|InputCode::kRelease,37,-19}) && client.Input().Empty());
    }
    client.PushGamePause();CHECK(!input.Event({'D'|InputCode::kOutside,37,-19}) && client.Input().Empty());client.PopGamePause();
}
