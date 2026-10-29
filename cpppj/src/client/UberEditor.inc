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
// 원본 송금 숫자 창은 같은 문자 입력과 현재 Session의 경제 장치를 빌린다.
void UberGump::OpenSendMoneyDialog(std::uint32_t target) {
    if (!world_) throw std::logic_error("송금 숫자 창의 현재 월드 없음");SaveFortNameDialogContext context;context.editor.now=[this] { return client_.WallSeconds(); };BindTextClipboard(context.editor,textClipboard_);
    context.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };
    context.sendMoney=[this](auto owner,auto text) { if (!world_) throw std::logic_error("송금 확정의 현재 월드 없음");return ConfirmSendMoney(owner,text,world_->Session().MoneyInputHooks()); };
    rawButtons_->Menus()->ShowSendMoneyDialog(std::move(context),target);
}
// 원문 이름 대화창은 현재 작업본과 Template 헤더를 동기 호출 동안 그대로 빌린다.
void UberGump::OpenSaveFortNameDialog(std::uint32_t fortMode) {
    if (fortMode && (!world_ || !client_.GameplayGlobals()->world.editor)) throw std::logic_error("실제 Editor 밖의 Save as 파일 장치 미연결");
    if (!editor_) editor_=std::make_unique<EditorRuntime>(client_); // mode0 새 이름 창은 World/State1 이전에도 같은 작업 파일 장치를 빌린다.
    SaveFortNameDialogContext context;context.work=&editor_->files;context.currentName=client_.FortGlobals()->Current();context.editor.now=[this] { return client_.WallSeconds(); };
    BindTextClipboard(context.editor,textClipboard_); //최초 Chat이 이미 만든 공통 문자 UI와 같은 장치를 계속 빌린다.
    context.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };
    context.fortPath=[this](auto name) { mapFileSpec_.SelectFort();client_.FortGlobals()->SelectFileSpec();const auto configured=client_.Configuration().PathSpec(mapFileSpec_.Current(),name);return editorCatalog_->Resolve(configured).string(); }; //같은 현재 FileSpec의 source/work 절대 경로도 명시 전체 매핑을 사용한다.
    context.header=[this] { const auto owner=client_.GameplayGlobals()->player.templateOwner;auto headers=templateStorage_->Headers();if (owner>=9 || headers.size()<static_cast<std::size_t>(owner+1U)*20U) throw std::out_of_range("Save as의 현재 Template 헤더 범위");return headers.subspan(static_cast<std::size_t>(owner)*20U,20); };
    context.setName=[this](auto name) { SelectCurrentFortName(name); };context.tell=[this](auto section) { Tell(std::string(section)); };
    context.readFort=[this](auto name) { const auto token=std::bit_cast<std::int32_t>(scriptStrings_.Store(name));client_.Input().PushCommand(kReadFortCommand,token,token); }; //원문 mode0은 같은 현재 이름 token을 두 인자에 복제하여 일반 입력 큐에 예약한다.
    rawButtons_->Menus()->ShowSaveFortNameDialog(std::move(context),fortMode,1); //원문 일반 입력은 event.y,1,NULL의 세 인자를 같은 공장에 전달한다.
}
//State1과3이 같은 Native 장치의 공통 준비를 원문 순서로 사용하고 선택 Editor 전역만 구별한다.
void UberGump::PrepareNativeFortState(bool editing) {
    if (!editor_) throw std::logic_error("실제 State1/3 공통 장기 장치 누락");
    if (editing) htmlGlobals_.background[0]=0; //원문004b7016은 State1의 HUD 재생성 전에00565e70 첫BYTE를 지운다. State3이 이미 선택한 titleMenu/승패 배경은 보존한다.
    CleanupNativeStateWorld(0); //실제 연결/AI/Deck/Kernel/Core/풀/HUD를 같은 원문 순서로 반납하고 새 Player를 만든다.
    if (commandGump_) static_cast<void>(rawButtons_->BubbleGump(commandGump_,0x02000088U,0,0)); //0042e440의 현재 Chat 부모 숨김이다.
    auto& e=*editor_;if (e.transport) e.transport->CloseListener(); //앞선 CloseAll이 닫은 이전 양끝 뒤에 이전 listener만 반납한다.
    auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();
    e.transport=std::make_shared<EditorLocalTransport>(client_.PlayerRecords()->Records(),game.Server(),e.Hooks(),client_.ConnectionGlobals());
    std::int32_t port=6799;client_.Configuration().ReadInt("gameServerPort",port);e.transport->Open(std::bit_cast<std::uint32_t>(port)); //004e22c0은 로그인/Player 재배치 없이 client9를 연다.
    surface.ownerMode.fort=true;game.player.fortMode=true; //같은00594fb8을 읽는 기존 Player 어댑터의 호환 별칭에도 현재 모드를 전달한다.
    if (editing) { game.world.editor=1;game.world.missionPost=true;game.FortKeepGenus()=0xffffffffU;game.FullDeck()=1; } //State3은 이 네 쓰기를 실행하지 않고 앞선 전체 cleanup의 현재값을 유지한다.
    game.Server()=true;game.Boss()=true;client_.CoreDataGlobals()->Select(game.player,0,1,1);
    client_.InitializeNativeRendererView();surface.chunks.Reset();surface.chunks.Mode()=1;
    o::ResetMissionBattleOptions(o::OriginalEdition::Patch1078,*missionOptions_);game.StateAuxFC4()=0;
}
//현재 State가1일 때만 실제 Editor 외곽을 준비하며 State 확정은 호출한 RawLocalMissionState에 남긴다.
o::LocalMissionParentResult UberGump::InstallExistingEditorState(std::uint32_t selected) {
    if (selected!=1 || !editor_) throw std::logic_error("실제 Edit State1 외곽 계약");
    PrepareNativeFortState(true);auto& e=*editor_;
    //호스트 Session은 준비된 물리만 빌리고 파일 설치/Template 변경은 아래 실제 checkpoint의 위치까지 지연한다.
    const std::string pending(client_.FortGlobals()->Pending());const auto configured=client_.Configuration().PathSpec("fortSpec",pending);
    const auto working=editorCatalog_->Prepare(configured); //목록을 거치지 않는 직접 설치도 완료된 삭제 항목을 원본에서 다시 복사하지 않는다.
    e.files.EnsureDirectory(working.parent_path().string()); // 새 작업 namespace도 원본 설치의 d 폴더처럼 저장 부모 디렉터리를 먼저 확보한다. Fort 파일은 원문 Notify가 생성한다.
    const auto bytes=client_.Files().TryRead(working.string());
    const auto image=bytes ? std::make_shared<const std::vector<std::uint8_t>>(*bytes) : std::shared_ptr<const std::vector<std::uint8_t>>{};
    o::FortTemplate fort;if (image) fort=o::FortTemplate::Parse(*image,client_.Assets().TypeTable()); // 파일 부재는 새 manager/자료 부모가 처리하며 가짜 빈 Fort 파일을 읽지 않는다.
    auto services=MakeCampaignClientServices(client_,*rawButtons_,{[this] { RefreshWindowTitle(); },[this](auto name) { Tell(std::string(name)); },[this](auto kind,auto player,auto parameter) { warnings_->Remove(kind,player,parameter);rebuild_=true; },[this](auto packet) { ReceiveChatMessage(packet); }});
    services.installMode=CampaignInstallMode::ExistingEditor;services.deferExistingEditorFile=true;services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.localLogin.reset();
    services.templateStorage=templateStorage_;services.missionFortImage=image;services.missionOptions=missionOptions_;services.missionRestrictions=missionRestrictions_;services.editorTransport=e.transport;services.fortMission=FortMissionSaveEffects();
    services.fortBadges=client_.FortBadges();services.fortTypeConversion=client_.FortTypeConversion(); //기존 파일의 실제 장부/변환표를 캠페인과 같은 Client owner에서 빌린다.
    world_=std::make_unique<GameWorld>(client_.Assets(),client_.Files(),std::move(fort),o::MissionPlayers{},pending,std::move(services),ScreenPoint{client_.GetScreen().Width(),client_.GetScreen().Height()},true);
    world_->elapsed=[this] { return client_.Time().delta; };world_->paused=[this] { return client_.Paused(); };world_->frameTime=[this] { return client_.Time(); };
    world_->BindHideBuildings([this] { return hideBuildings_; });world_->soundViewChanged=[this](auto view) { client_.WorldAudio().SetView(view); };
    world_->surfaceDisplay={[this] { return client_.GetRenderer().FullRedrawPending(); },[this](auto rect,auto flags) { client_.GetRenderer().Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }};
    auto& session=world_->Session();session.WithExistingEditorPrefixUi(*client_.LocalStateGlobals(),[this] { ApplyHudModeForState(1); });
    EditorFortPathEffects paths;paths.currentName=[this] { return client_.FortGlobals()->Current(); };paths.selectFileSpec=[this] { mapFileSpec_.SelectFort(); };paths.fileSpecKey=[this] { return mapFileSpec_.Current(); };
    paths.resolve=[this](auto configured) { return editorCatalog_->Resolve(configured); };paths.prepare=[this](auto configured) { static_cast<void>(editorCatalog_->Prepare(configured)); }; //현재 경로와 lazy 준비 모두 같은 이관 장부/삭제 상태를 공유한다.
    paths.assertion=[](auto line) { std::clog<<"Editor 파일 진단 "<<line<<'\n'; };e.paths=std::make_unique<EditorFortFileDevices>(e.files,client_.Files(),client_.Configuration(),std::move(paths));
    CampaignExistingEditorCheckpointServices checkpoint;
    checkpoint.fort.currentName=[this] { return client_.FortGlobals()->Current(); };checkpoint.fort.pendingName=[this] { return client_.FortGlobals()->Pending(); };
    checkpoint.fort.compare=[](auto first,auto second) { return o::CompareDataManagerSectionNames(first,second); };
    checkpoint.fort.setName=[this](auto name) { SelectCurrentFortName(name); };checkpoint.fort.missionName=[this](auto name) { return mapCatalog_->Kind(name); };
    checkpoint.fort.closeGuide=[this] { ReleaseCurrentGuideMission(); }; //원문 부모가 이름 변경/현재 미션/대상 맵 속성을 판정한 뒤에만 보존한 Tutorial 데이터와 Guide를 해제한다.
    checkpoint.fort.setConfig=[this](auto name,auto value) { client_.Configuration().Set(name,o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()})); };
    checkpoint.fort.outside=[](auto branch) { throw std::logic_error("Editor 기존 파일 외의 설치 분기:"+std::to_string(static_cast<int>(branch))); };
    checkpoint.file.path=[&e] { return e.paths->CurrentPath(); };checkpoint.file.exists=[&e](auto path) { return e.paths->Exists(path); };checkpoint.file.title=[this] { RefreshWindowTitle(); };
    checkpoint.file.assertion=[](auto line) { std::clog<<"Editor Template 진단 "<<line<<'\n'; };checkpoint.file.outside=checkpoint.fort.outside;
    TemplateFileLoadHooks load;load.wrongName=[] { std::clog<<"Editor 파일명 불일치\n"; };load.tooManyTypes=[] { std::clog<<"Editor TYPE 개수 진단\n"; };load.rejectFile=[&e](auto path) { e.files.Delete(path); };checkpoint.load=e.paths->BindLoad(std::move(load));
    checkpoint.startup=EditorStartupStateHooks(); //State3과 같은 실제 기본 옵션/파일 부재 관문/벽시각 장치를 빌린다.
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
//원문 startupStateCheck의 실제 선택 장치를 두 Native 부모가 같은 장기 파일 경로로 공유한다.
o::StartupStateCheckHooks UberGump::EditorStartupStateHooks() {
    if (!editor_) throw std::logic_error("State1/3 startup 장기 장치 누락");auto& e=*editor_;o::StartupStateCheckHooks startup;
    startup.configInt=[this](auto name) { return client_.Configuration().GetInt(name); }; //선택된 saved-state reader의 누락을 옵션 강제0으로 숨기지 않는다.
    startup.readSavedState=[this,&e] {
        const auto bytes=SavedStateFilePath(client_.GameDirectory()).u8string();e.savedPath.assign(bytes.begin(),bytes.end());o::SavedStateReadHooks reader; // 원본ANSI와 달리 Windows10/11의 한글 설치 이름을 UTF-8로 보존한다.
        reader.path=[&e] { return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(e.savedPath.c_str()),e.savedPath.size()+1); };
        reader.exists=[](auto path) { const std::filesystem::path actual(std::u8string(path.begin(),path.end()));return GetFileAttributesW(actual.c_str())!=INVALID_FILE_ATTRIBUTES ? 1 : 0; }; // 원본004dd560의 디렉터리 비영 판정은 같다. Windows10/11 한글 경로 때문에 ANSI 대신 W API를 쓴다.
        std::array<std::uint8_t,o::kSavedStateReadScratchBytes> scratch; //부재 경로는 실제 reader가 stack/복원 전역에 접근하지 않는다. 존재하면 아직 미연결인 open 경계에서 명시 중단한다.
        return o::ReadSavedState(scratch,{},reader);
    };
    startup.wallSeconds=[this] { return client_.WallSeconds(); };return startup; //reader0 복귀에서도 원문처럼 현재 벽시각을 읽는다.
}
//실제 State3는 World를 다시 만들지 않고 같은 Client의 Native 부모와 공통 tail을 실행한다.
o::LocalMissionParentResult UberGump::InstallMainMenuNativeState(std::uint32_t selected) {
    if (selected!=3 || !editor_) throw std::logic_error("실제 메인 메뉴 State3 장기 부모 계약");
    auto& local=*client_.LocalStateGlobals();auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();auto& names=*client_.FortGlobals();
    NativeMainMenuStateGlobals globals{local.State(),local.SourceType(),client_.WorldAudio().ResultState(),local.FirstMenuPending(),surface.ownerMode.battle,game.world.editor,htmlGlobals_.background,names.CurrentBytes(),names.PendingBytes(),[&local] { return local.UserUpdated(); },[&local] { return local.TellTips(); }};
    NativeMainMenuStateHooks hooks;hooks.configInt=[this](auto key) { return client_.Configuration().GetInt(key); };
    hooks.request=[this](auto state) { editor_->state->Request(state); }; //같은 실행 중인 Native 분배기의 immediate를 사용한다.
    hooks.deleteSavedState=[this] { static_cast<void>(DeleteSavedStateFile(client_.GameDirectory())); }; // 원문 State3는 경로를 매번 계산하고 CRT 삭제 실패도 상태 전환을 중단시키지 않는다.
    hooks.requestMusic=[this](auto name) { client_.WorldAudio().RequestMusic(name); };
    hooks.sourceBackground=[this](auto type)->std::optional<std::string> {
        const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("State3 source TYPE 이름 범위");const auto name=types[type].name;
        if (!client_.Files().TryRead("d/"+name+".gif")) return std::nullopt;return name; //원문 고정 data 디렉터리의 .gif 존재만 판정하며 없는 파일을 생성하지 않는다.
    };
    hooks.leaveBattle=[this] { if (!world_) throw std::logic_error("State3의 이전 Battle14 실제 Session 없음");const auto packet=MakeBattleLeavePacket();world_->Session().SendLocalPacket(9,packet); }; // 원문처럼 전체 cleanup 전에 같은 로컬9에52h를 예약하며 수신/flush를 강제하지 않는다.
    hooks.prefix=[this] { PrepareNativeFortState(false); };hooks.hudPrefix=[this] { ApplyHudModeForState(1); };
    hooks.title=[this] { RefreshWindowTitle(); };hooks.closeGuide=[this] { ReleaseCurrentGuideMission(); };
    hooks.tell=[this](auto section) { Tell(std::string(section)); };hooks.startup=[this] { return o::CheckStartupState(EditorStartupStateHooks()); };
    hooks.editorPlayers=[this] { client_.Configuration().Set("lastEditFort",client_.FortGlobals()->Current());if (!world_) throw std::logic_error("State3 Tell 뒤 선택된 EditorPlayers의 실제 Session 없음");world_->Session().InstallExistingEditorPlayers(MakeEditorPlayerClientHooks(client_)); };
    hooks.changedState=[] { throw std::logic_error("State3 제목 갱신 뒤 선택된 다른 기존 맵 설치 분기 미연결"); };
    hooks.tail=[this,&local] {
        ExistingEditorAfterCheckpointEffects tail;tail.hud=[this](auto command) { rawButtons_->NotifyHudMode(command,1); };tail.wallSeconds=[this] { return client_.WallSeconds(); };
        tail.requestMusic=[this](auto name) { client_.WorldAudio().RequestMusic(name); };tail.fortPalette=[this] { const auto name=client_.Configuration().PathSpec("fortPal");client_.ChangeFortPalette({reinterpret_cast<const std::uint8_t*>(name.c_str()),name.size()+1}); };
        ApplyExistingEditorAfterCheckpoint(local,client_.Input(),tail); //State3는 원문처럼 섬 수와 추가 명령을 읽지 않고 다음4만 예약한다.
    };
    return netstorm::client::InstallNativeMainMenuState(std::move(globals),hooks);
}
// 실제 영상은 같은 Screen 팔레트/두 정지/ClientAudio를 원문 수명 순서로 빌린다.
RawMovieContext UberGump::MovieDevices() {
    RawMovieContext context;context.now=[this] { return client_.WallSeconds(); };context.audio=client_.MovieAudioEnabled();context.mute=client_.MovieAudioMuted();auto& h=context.devices;
    h.capturePalette=[this](auto palette) { const auto current=client_.GetScreen().Palette();std::copy(current.begin(),current.end(),palette.begin()); };
    h.applyPalette=[this](auto palette) { client_.GetScreen().SetPalette(0,256,palette.data(),true); }; // 파일 팔레트/색 표를 다시 계산하지 않는 원문 임시 팔레트다.
    h.shutdownSound=[this] { client_.WorldAudio().Shutdown(); };h.muteSound=[this] { client_.WorldAudio().PushMute(); };h.unmuteSound=[this] { client_.WorldAudio().PopMute(); };
    h.soundEnabled=[this] { return client_.WorldAudio().SoundEnabled(); };h.restoreSound=[this](auto quality) { client_.RestoreMovieSound(quality); };
    h.pauseUpdates=[this](auto paused) { client_.PauseMovieUpdates(paused); };h.pauseGame=[this](auto paused) { if (paused) client_.PushGamePause();else client_.PopGamePause(); };
    h.showCursor=[this](auto show) { client_.GetScreen().ShowCursor(show); };h.refreshCursor=[this] { client_.GetScreen().RestoreMovieCursor(); };
    h.clear=[this](auto rect,auto color) { client_.GetScreen().Clear(rect,static_cast<std::uint8_t>(color)); };h.black=[this] { return client_.GetScreen().Colors().basic[0]; };
    h.redraw=[this](auto) { Refresh(); };h.assertion=[](auto line) { std::clog<<"MovieGump 진단 "<<line<<'\n'; };
    context.invalidate=[this](auto rect,auto flags) { client_.GetRenderer().Invalidate(rect,flags); };
    context.clearAndPresent=[this] { auto& screen=client_.GetScreen();screen.Clear({0,0,screen.Width(),screen.Height()},0);screen.Update({0,0,screen.Width(),screen.Height()}); }; // Windows10/11의 호환 GDI 출력으로 원본 전체 검정/즉시 출력을 보존한다.
    return context;
}
// 원문의 인트로 준비는 listener/Chunk/옵션 초기화를 하는 State3 prefix와 다른 순서다.
o::LocalMissionParentResult UberGump::InstallIntroNativeState(std::uint32_t selected) {
    if (selected<39 || selected>41 || !editor_) throw std::logic_error("실제 인트로 State39/40/41 장기 부모 계약");
    auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();NativeIntroStateHooks hooks;
    hooks.cleanup=[this] { CleanupNativeStateWorld(0); };
    hooks.hideChatInput=[this] { const auto token=client_.ChatWindowGlobals()->window.current;if (token) rawButtons_->Menus()->ChatGeometryBackend()->SetInputVisible(token,0); };
    hooks.hideChat=[this] { if (commandGump_) static_cast<void>(rawButtons_->BubbleGump(commandGump_,0x02000088U,0,0)); };
    hooks.core=[this,&game] { game.player.fortMode=true;client_.CoreDataGlobals()->Select(game.player,0,1,1); };
    hooks.renderer=[this] { client_.InitializeNativeRendererView(); };hooks.hud=[this] { ApplyHudModeForState(1); };
    hooks.moviePath=[this] { return client_.Configuration().PathSpec("introMovie"); };hooks.markIntro=[this] { client_.Configuration().Set("intro","1"); };
    hooks.openMovie=[this](auto name) {
        const std::filesystem::path path(std::u8string(reinterpret_cast<const char8_t*>(name.data()),name.size()));const auto actual=path.is_absolute() ? path : client_.GameDirectory()/path; // Windows10/11의 한글 설치 경로는 원본 ANSI 대신 UTF-8/W 경로로 연다.
        auto& movies=rawButtons_->Menus()->EnableMovies(MovieDevices());static_cast<void>(movies.Open(actual)); // 원문은 실제 열기 실패도 State40으로 진행한다.
    };
    hooks.movieActive=[this] { const auto* movies=rawButtons_->Menus()->Movies();return movies && movies->Active(); }; // Kill 예약이 아니라 실제 scalar 반납이 singleton을 지운다.
    hooks.request=[this](auto next) { editor_->state->Request(next); };
    return netstorm::client::InstallNativeIntroState({client_.LocalStateGlobals()->State(),surface.ownerMode.fort,game.Server(),game.Boss(),client_.FortGlobals()->PendingBytes()},hooks);
}
// 미션 Login은 기존 분배기에 남기고 Edit1/메인3/인트로39~41과 다음4를 같은 분배기로 처리한다.
void UberGump::PollEditorState() {
    auto& local=*client_.LocalStateGlobals();const auto next=local.State().next[0];
    if (next!=1 && next!=3 && !(editor_ && (next==4 || (next>=39 && next<=41)))) return;
    if (!editor_) editor_=std::make_unique<EditorRuntime>(client_);
    if (!editor_->state) { // 새 이름 창에서 먼저 확보한 파일 장치가 있어도 실제 State 분배기는 별도로 처음 한 번 구성한다.
        auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();
        o::LocalMissionStateHooks hooks;hooks.logState=[](auto value) { std::clog<<"State "<<value<<'\n'; };hooks.parentState=[this](auto selected) { if (selected>=39 && selected<=41) return InstallIntroNativeState(selected);return selected==3 ? InstallMainMenuNativeState(selected) : InstallExistingEditorState(selected); };
        hooks.invalidState=[](auto state) { throw std::logic_error("Editor/메인 분배기의 선택된 Native State 미연결:"+std::to_string(state)); };
        editor_->state=std::make_unique<o::RawLocalMissionState>(client_.Assets().Edition(),local.State(),o::LocalMissionStateGlobals{game.Server(),game.Boss(),surface.ownerMode.battle,game.world.missionPost,local.SavedGame(),surface.links.mission,local.Simulconnect(),surface.ownerMode.challenge,game.player,local.ZonePending(),local.GameTicks(),local.SourceType(),local.RandomSeed(),local.Blocks()[2][33],local.MetaRefresh(),local.MetaRebuild(),reinterpret_cast<std::uint8_t&>(htmlGlobals_.background[0]),local.BattleByte(),local.ConnectEnd(),local.ActivityEnd(),client_.Time().game,client_.Time().wall},std::move(hooks));
    }
    if (next==1 || next==3) editor_->committed=false;editor_->state->Poll();
    if (next==3 || (next>=39 && next<=41)) {
        if (local.State().phase[0]==3 && local.State().next[0]==4) PublishNativeMainMenuState();
        return; // 영상이 살아 있는 State40은 다음 Poll까지 유지하며 메인 메뉴를 먼저 공개하지 않는다.
    }
    if (next==1 && !editor_->committed) {
        world_->Session().CommitExistingEditorInstall(local); //공통 확정 뒤 실제 loaded 공개가 끝나야 호스트 사용자 입력 몸체를 만들 수 있다.
        BindCampaignUserInputServices(client_,*world_,editor_->selectCursor);userInput_=std::make_unique<UserInput>(client_,*world_);
        editor_->committed=true;state_.phase=ClientPhase::Mission;pageName_="editor";page_={};popup_.clear();RebuildControls();
    }
}
