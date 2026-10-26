// 실제 Edit 선택의 State1 준비/기존 파일/tail과 다음 Poll4를 같은 Client 소유자에 연결한다.
struct UberGump::EditorRuntime {
    Client& client; //State/시계/Player/연결표를 소유하는 실제 장기 Client다.
    EditorFiles files; //읽기 전용 자산과 다른 형제 디렉터리의 편집 작업본만 쓴다.
    std::unique_ptr<EditorFortFileDevices> paths; //현재 Fort 이름/고정 FileSpec의 경로 scratch를 Session보다 오래 유지한다.
    std::shared_ptr<EditorLocalTransport> transport; //prefix에서 연 listener/client9와 뒤 수신기가 같은 실제 양끝을 빌린다.
    std::unique_ptr<o::RawLocalMissionState> state; //Client의 동일 다섯144BYTE를 소비하는 원문 State 분배기다.
    std::string savedPath; //004b5470의 게임 폴더/d/!state.dat 절대 경로를 reader 동기 호출 동안 보존한다.
    std::function<void(std::uint32_t)> selectCursor; //실제 커서 장치를 공통 State 확정 뒤의 호스트 입력 구성에도 같은 callback으로 전달한다.
    bool committed{}; //실제 Session.Commit 복귀의 호스트 관측이며 Native State나 loaded 대신 쓰지 않는다.
    explicit EditorRuntime(Client& value):client(value),files(EditorWorkingRoot(value.GameDirectory()),
        {value.GameDirectory(),EditorWorkingRoot(value.GameDirectory()).parent_path()/"originals",EditorWorkingRoot(value.GameDirectory()).parent_path()/"originalCD"}) {} //실제 자산 디렉터리의 같은 형제 작업본과 보호 루트를 선택한다. 생성만으로 파일/게임 상태를 쓰지 않는다.
    //호스트 메인 메뉴/종료에서 Session 수신기를 반납한 뒤 남은 실제 양끝/리스너를 닫는다.
    void Close() { if (transport) { client.ConnectionGlobals()->CloseAll();transport->CloseListener();transport.reset(); } }
    //큐/노드 소유권은 기존 전송의 SocketClosed/Track이 처리하고 이 경계는 실제 시각과 진단만 제공한다.
    o::PacketDispatchHooks Hooks() {
        o::PacketDispatchHooks hooks;hooks.now=[this] { return static_cast<std::uint32_t>(client.WallSeconds()); };
        hooks.record=[](auto&,auto,auto) {}; //원문의 선택적 패킷 기록 장치이며 게임 처리/수신 성공값을 만들지 않는다.
        hooks.terminate=[this](auto& connection) { transport->Disconnect(std::bit_cast<std::int32_t>(connection.sender)); };
        hooks.close=[](auto&) {};hooks.release=[](auto&) {}; //로컬 장치에는 별도 OS socket/할당이 없다. 실제 양끝 반납은 Track의 SocketClosed/Alive 변경이 수행한다.
        return hooks;
    }
};
//현재 State가1일 때만 실제 Editor 외곽을 준비하며 State 확정은 호출한 RawLocalMissionState에 남긴다.
o::LocalMissionParentResult UberGump::InstallExistingEditorState(std::uint32_t selected) {
    if (selected!=1 || !editor_) throw std::logic_error("실제 Edit State1 외곽 계약");
    CleanupNativeStateWorld(0); //실제 연결/AI/Deck/Kernel/Core/풀/HUD를 같은 원문 순서로 반납하고 새 Player를 만든다.
    if (commandGump_) static_cast<void>(rawButtons_->BubbleGump(commandGump_,0x02000088U,0,0)); //0042e440의 현재 Chat 부모 숨김이다.
    auto& e=*editor_;if (e.transport) e.transport->CloseListener(); //앞선 CloseAll이 닫은 이전 양끝 뒤에 이전 listener만 반납한다.
    auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();
    e.transport=std::make_shared<EditorLocalTransport>(client_.PlayerRecords()->Records(),game.Server(),e.Hooks(),client_.ConnectionGlobals());
    std::int32_t port=6799;client_.Configuration().ReadInt("gameServerPort",port);e.transport->Open(std::bit_cast<std::uint32_t>(port)); //004e22c0은 로그인/Player 재배치 없이 client9를 연다.
    surface.ownerMode.fort=true;game.player.fortMode=true; //같은00594fb8을 읽는 기존 Player 어댑터의 호환 별칭에도 현재 모드를 전달한다.
    game.world.editor=1;game.world.missionPost=true;game.FortKeepGenus()=0xffffffffU;game.FullDeck()=1;
    game.Server()=true;game.Boss()=true;client_.CoreDataGlobals()->Select(game.player,0,1,1);
    client_.InitializeNativeRendererView();surface.chunks.Reset();surface.chunks.Mode()=1;
    o::ResetMissionBattleOptions(o::OriginalEdition::Patch1078,*missionOptions_);game.StateAuxFC4()=0;
    //호스트 Session은 준비된 물리만 빌리고 파일 설치/Template 변경은 아래 실제 checkpoint의 위치까지 지연한다.
    const std::string pending(client_.FortGlobals()->Pending());const auto configured=client_.Configuration().PathSpec("fortSpec",pending);
    const auto relative=std::filesystem::path(configured).is_absolute() ? configured : o::NormalizeGamePath(configured);
    auto working=e.files.ResolvePath(relative);if (!e.files.Exists(working.string())) { //기존 사용자 작업본은 원본을 재읽거나 덮어쓰지 않는다.
        if (const auto bytes=client_.Files().TryRead(configured)) working=e.files.PreparePath(relative,*bytes);
    }
    const auto image=std::make_shared<const std::vector<std::uint8_t>>(client_.Files().Read(working.string()));
    auto fort=o::FortTemplate::Parse(*image,client_.Assets().TypeTable());
    auto services=MakeCampaignClientServices(client_,*rawButtons_,{[this] { RefreshWindowTitle(); },[this](auto name) { Tell(std::string(name)); },[this](auto kind,auto player,auto parameter) { warnings_->Remove(kind,player,parameter);rebuild_=true; }});
    services.installMode=CampaignInstallMode::ExistingEditor;services.deferExistingEditorFile=true;services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.localLogin.reset();
    services.templateStorage=templateStorage_;services.missionFortImage=image;services.missionOptions=missionOptions_;services.missionRestrictions=missionRestrictions_;services.editorTransport=e.transport;services.fortMission=FortMissionSaveEffects();
    services.fortBadges=client_.FortBadges();services.fortTypeConversion=client_.FortTypeConversion(); //기존 파일의 실제 장부/변환표를 캠페인과 같은 Client owner에서 빌린다.
    world_=std::make_unique<GameWorld>(client_.Assets(),client_.Files(),std::move(fort),o::MissionPlayers{},pending,std::move(services),ScreenPoint{client_.GetScreen().Width(),client_.GetScreen().Height()},true);
    world_->elapsed=[this] { return client_.Time().delta; };world_->paused=[this] { return client_.Paused(); };world_->frameTime=[this] { return client_.Time(); };
    world_->BindHideBuildings([this] { return hideBuildings_; });world_->soundViewChanged=[this](auto view) { client_.WorldAudio().SetView(view); };
    world_->surfaceDisplay={[this] { return client_.GetRenderer().FullRedrawPending(); },[this](auto rect,auto flags) { client_.GetRenderer().Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }};
    auto& session=world_->Session();session.WithExistingEditorPrefixUi(*client_.LocalStateGlobals(),[this] { ApplyHudModeForState(1); });
    EditorFortPathEffects paths;paths.currentName=[this] { return client_.FortGlobals()->Current(); };paths.selectFileSpec=[this] { mapFileSpec_.SelectFort(); };paths.fileSpecKey=[this] { return mapFileSpec_.Current(); };
    paths.assertion=[](auto line) { std::clog<<"Editor 파일 진단 "<<line<<'\n'; };e.paths=std::make_unique<EditorFortFileDevices>(e.files,client_.Files(),client_.Configuration(),std::move(paths));
    CampaignExistingEditorCheckpointServices checkpoint;
    checkpoint.fort.currentName=[this] { return client_.FortGlobals()->Current(); };checkpoint.fort.pendingName=[this] { return client_.FortGlobals()->Pending(); };
    checkpoint.fort.compare=[](auto first,auto second) { return o::CompareDataManagerSectionNames(first,second); };
    checkpoint.fort.setName=[this](auto name) { SelectCurrentFortName(name); };checkpoint.fort.missionName=[this](auto name) { return mapCatalog_->Kind(name); };
    checkpoint.fort.closeGuide=[] { throw std::logic_error("Editor 기존 파일의 선택된 Guide 종료 미연결"); };
    checkpoint.fort.setConfig=[this](auto name,auto value) { client_.Configuration().Set(name,o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()})); };
    checkpoint.fort.outside=[](auto branch) { throw std::logic_error("Editor 기존 파일 외의 설치 분기:"+std::to_string(static_cast<int>(branch))); };
    checkpoint.file.path=[&e] { return e.paths->CurrentPath(); };checkpoint.file.exists=[&e](auto path) { return e.paths->Exists(path); };checkpoint.file.title=[this] { RefreshWindowTitle(); };
    checkpoint.file.assertion=[](auto line) { std::clog<<"Editor Template 진단 "<<line<<'\n'; };checkpoint.file.outside=checkpoint.fort.outside;
    TemplateFileLoadHooks load;load.wrongName=[] { std::clog<<"Editor 파일명 불일치\n"; };load.tooManyTypes=[] { std::clog<<"Editor TYPE 개수 진단\n"; };load.rejectFile=[&e](auto path) { e.files.Delete(path); };checkpoint.load=e.paths->BindLoad(std::move(load));
    checkpoint.startup.configInt=[this](auto name) { return client_.Configuration().GetInt(name); }; //선택된 saved-state reader의 누락을 옵션 강제0으로 숨기지 않는다.
    checkpoint.startup.readSavedState=[this,&e] {
        e.savedPath=std::filesystem::absolute(client_.GameDirectory()/"d"/"!state.dat").string();o::SavedStateReadHooks reader;
        reader.path=[&e] { return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(e.savedPath.c_str()),e.savedPath.size()+1); };
        reader.exists=[](auto path) { const std::string actual(path);return GetFileAttributesA(actual.c_str())!=INVALID_FILE_ATTRIBUTES ? 1 : 0; }; //원문004dd560은 디렉터리도 비영이며 파일 내용을 미리 읽지 않는다.
        std::array<std::uint8_t,o::kSavedStateReadScratchBytes> scratch; //부재 경로는 실제 reader가 stack/복원 전역에 접근하지 않는다. 존재하면 아직 미연결인 open 경계에서 명시 중단한다.
        return o::ReadSavedState(scratch,{},reader);
    };
    checkpoint.startup.wallSeconds=[this] { return client_.WallSeconds(); }; //reader0 복귀에서도 원문처럼 현재 벽시각을 읽는다.
    session.InstallExistingEditorCheckpoint(checkpoint,client_.ReadOnlyForts(),e.files);
    ExistingEditorAfterCheckpointEffects tail;tail.hud=[this](auto command) { rawButtons_->NotifyHudMode(command,1); };tail.wallSeconds=[this] { return client_.WallSeconds(); };
    tail.requestMusic=[this](auto name) { client_.WorldAudio().RequestMusic(name); };tail.fortPalette=[this] { const auto name=client_.Configuration().PathSpec("fortPal");client_.ChangeFortPalette({reinterpret_cast<const std::uint8_t*>(name.c_str()),name.size()+1}); };tail.islandCount=[&session] { return session.IslandCount(); };
    session.WithExistingEditorInstallUi(*client_.LocalStateGlobals(),client_.Input(),tail,[this,&session,&e] {
        auto effects=MakeCampaignCursorClientEffects(client_,*rawButtons_);e.selectCursor=effects.lifecycle.selectCursor;
        effects.send=[&session](auto target,auto packet) { session.SendLocalPacket(std::bit_cast<std::int32_t>(target),packet); };
        effects.lifecycle.highlight=[this](auto mode) { world_->Highlight(mode); };effects.drop.refresh=[this](auto sid,auto force,auto move) { if (force || move!=1) throw std::logic_error("Editor 커서 선택 인자 계약");world_->Select(sid); };
        session.Cursor().SetEffects(std::move(effects));session.Cursor().Display().SetExternal([this](auto token) { return rawButtons_->GumpDepth(token); },{});
        InstallHud();world_->Resize(client_.GetScreen().Width(),client_.GetScreen().Height());world_->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical());
    });
    return o::LocalMissionParentResult::Commit;
}
//미션 Login이 소비하는 다른 Native 요청은 그대로 두며 실제 Edit 요청과 그 다음4만 같은 분배기로 처리한다.
void UberGump::PollEditorState() {
    auto& local=*client_.LocalStateGlobals();const auto next=local.State().next[0];
    if (next!=1 && !(editor_ && next==4)) return;
    if (!editor_) {
        editor_=std::make_unique<EditorRuntime>(client_);auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();
        o::LocalMissionStateHooks hooks;hooks.logState=[](auto value) { std::clog<<"State "<<value<<'\n'; };hooks.parentState=[this](auto selected) { return InstallExistingEditorState(selected); };
        editor_->state=std::make_unique<o::RawLocalMissionState>(client_.Assets().Edition(),local.State(),o::LocalMissionStateGlobals{game.Server(),game.Boss(),surface.ownerMode.battle,game.world.missionPost,local.SavedGame(),surface.links.mission,local.Simulconnect(),surface.ownerMode.challenge,game.player,local.ZonePending(),local.GameTicks(),local.SourceType(),local.RandomSeed(),local.Blocks()[2][33],local.MetaRefresh(),local.MetaRebuild(),reinterpret_cast<std::uint8_t&>(htmlGlobals_.background[0]),local.BattleByte(),local.ConnectEnd(),local.ActivityEnd(),client_.Time().game,client_.Time().wall},std::move(hooks));
    }
    if (next==1) editor_->committed=false;editor_->state->Poll();
    if (!editor_->committed) {
        world_->Session().CommitExistingEditorInstall(local); //공통 확정 뒤 실제 loaded 공개가 끝나야 호스트 사용자 입력 몸체를 만들 수 있다.
        BindCampaignUserInputServices(client_,*world_,editor_->selectCursor);userInput_=std::make_unique<UserInput>(client_,*world_);
        editor_->committed=true;state_.phase=ClientPhase::Mission;pageName_="editor";page_={};popup_.clear();RebuildControls();
    }
}
