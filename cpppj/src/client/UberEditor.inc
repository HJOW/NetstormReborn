// 실제 Edit 선택의 State1 준비/기존 파일/tail과 다음 Poll4를 같은 Client 소유자에 연결한다.
struct UberGump::EditorRuntime {
    Client& client; //State/시계/Player/연결표를 소유하는 실제 장기 Client다.
    EditorFiles files; //읽기 전용 자산과 다른 형제 디렉터리의 편집 작업본만 쓴다.
    std::unique_ptr<EditorFortFileDevices> paths; //현재 Fort 이름/고정 FileSpec의 경로 scratch를 Session보다 오래 유지한다.
    std::shared_ptr<EditorLocalTransport> transport; //prefix에서 연 listener/client9와 뒤 수신기가 같은 실제 양끝을 빌린다.
    std::unique_ptr<MetaConnectionRuntime> metaTransport; //World 없는 메타10/11의 실제 TCP/요청/현재 상태 장치다.
    std::unique_ptr<o::RawLocalMissionState> state; //Client의 동일 다섯144BYTE를 소비하는 원문 State 분배기다.
    std::string savedPath; //004b5470의 게임 폴더/d/!state.dat 절대 경로를 reader 동기 호출 동안 보존한다.
    std::function<void(std::uint32_t)> selectCursor; //실제 커서 장치를 공통 State 확정 뒤의 호스트 입력 구성에도 같은 callback으로 전달한다.
    bool committed{}; //실제 Session.Commit 복귀의 호스트 관측이며 Native State나 loaded 대신 쓰지 않는다.
    std::function<void()> multiplayerOptions; // 일반 State6의 실제 옵션 공장을 빌리며 미연결을 빈 창/성공으로 대체하지 않는다.
    std::function<void()> connectLan; // 실제 로컬 host/join 선택을 같은 일반 입력 소비자에 연결한다.
    explicit EditorRuntime(Client& value):client(value),files(ResolveEditorRoots(value.GameDirectory(),value.UserData()).work,ResolveEditorRoots(value.GameDirectory(),value.UserData()).protectedRoots) {} //기존에는 자산 디렉터리의 형제 작업본, 5차 사용자 홈 정책에서는 CustomMaps를 작업 루트로, 같은 보호 루트를 선택한다. 생성만으로 파일/게임 상태를 쓰지 않는다.
    //호스트 메인 메뉴/종료에서 Session 수신기를 반납한 뒤 남은 실제 양끝/리스너를 닫는다.
    void Close() { if (transport) { client.ConnectionGlobals()->CloseAll();transport->CloseListener();transport.reset(); }metaTransport.reset(); } //호스트 종료의 남은 메타 노드도 자기 owner로 반납한다.
    //큐/노드 소유권은 기존 전송의 SocketClosed/Track이 처리하고 이 경계는 실제 시각과 진단만 제공한다.
    o::PacketDispatchHooks Hooks() {
        o::PacketDispatchHooks hooks;hooks.now=[this] { return static_cast<std::uint32_t>(client.WallSeconds()); };
        hooks.record=[](auto&,auto,auto) {}; //원문의 선택적 패킷 기록 장치이며 게임 처리/수신 성공값을 만들지 않는다.
        hooks.terminate=[this](auto& connection) { transport->Disconnect(std::bit_cast<std::int32_t>(connection.sender)); };
        hooks.close=[](auto&) {};hooks.release=[](auto&) {}; //로컬 장치에는 별도 OS socket/할당이 없다. 실제 양끝 반납은 Track의 SocketClosed/Alive 변경이 수행한다.
        return hooks;
    }
};
// 실제 파일 조회가 처음 필요할 때만 장기 작업 namespace의 장치를 만든다. World/Player/State 성공은 쓰지 않는다.
EditorFortFileDevices& UberGump::EnsureEditorFortFiles() {
    if (!editor_) editor_=std::make_unique<EditorRuntime>(client_);if (editor_->paths) return *editor_->paths;auto& e=*editor_;
    EditorFortPathEffects paths;paths.currentName=[this] { return client_.FortGlobals()->Current(); };paths.selectFileSpec=[this] { mapFileSpec_.SelectFort(); };paths.fileSpecKey=[this] { return mapFileSpec_.Current(); };
    paths.resolve=[this](auto configured) { return editorCatalog_->Resolve(configured); };paths.prepare=[this](auto configured) { static_cast<void>(editorCatalog_->Prepare(configured)); }; //현재 경로와 lazy 준비 모두 같은 이관 장부/삭제 상태를 공유한다.
    paths.assertion=[](auto line) { std::clog<<"Editor 파일 진단 "<<line<<'\n'; };e.paths=std::make_unique<EditorFortFileDevices>(e.files,client_.Files(),client_.Configuration(),std::move(paths));
    return *e.paths;
}
// 원본 송금 숫자 창은 같은 문자 입력과 현재 Session의 경제 장치를 빌린다.
void UberGump::OpenSendMoneyDialog(std::uint32_t target) {
    if (!world_) throw std::logic_error("송금 숫자 창의 현재 월드 없음");SaveFortNameDialogContext context;context.editor.now=[this] { return client_.WallSeconds(); };BindTextClipboard(context.editor,textClipboard_);
    context.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };
    context.sendMoney=[this](const auto& owner,auto text) { if (!world_) throw std::logic_error("송금 확정의 현재 월드 없음");return ConfirmSendMoney(owner,text,world_->Session().MoneyInputHooks()); };
    rawButtons_->Menus()->ShowSendMoneyDialog(std::move(context),target);
}
// 원문 이름 대화창은 현재 작업본과 Template 헤더를 동기 호출 동안 그대로 빌린다.
void UberGump::OpenSaveFortNameDialog(std::uint32_t fortMode) {
    if (fortMode && (!world_ || !client_.GameplayGlobals()->world.editor)) throw std::logic_error("실제 Editor 밖의 Save as 파일 장치 미연결");
    if (!editor_) editor_=std::make_unique<EditorRuntime>(client_); // mode0 새 이름 창은 World/State1 이전에도 같은 작업 파일 장치를 빌린다.
    SaveFortNameDialogContext context;context.work=&editor_->files;context.currentName=client_.FortGlobals()->Current();context.editor.now=[this] { return client_.WallSeconds(); };
    BindTextClipboard(context.editor,textClipboard_); //최초 Chat이 이미 만든 공통 문자 UI와 같은 장치를 계속 빌린다.
    context.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };
    context.fortPath=[this](auto name) { mapFileSpec_.SelectFort();client_.FortGlobals()->SelectFileSpec();const auto configured=client_.Configuration().PathSpec(mapFileSpec_.Current(),name);return platform::PathToUtf8(editorCatalog_->Resolve(configured)); }; //같은 현재 FileSpec의 source/work 절대 경로도 명시 전체 매핑을 사용한다. Windows10/11 한글 경로는 원본 ANSI 대신 UTF-8다.
    context.header=[this] { const auto owner=client_.GameplayGlobals()->player.templateOwner;auto headers=templateStorage_->Headers();if (owner>=9 || headers.size()<static_cast<std::size_t>(owner+1U)*20U) throw std::out_of_range("Save as의 현재 Template 헤더 범위");return headers.subspan(static_cast<std::size_t>(owner)*20U,20); };
    context.setName=[this](auto name) { SelectCurrentFortName(name); };context.tell=[this](auto section) { Tell(std::string(section)); };
    context.readFort=[this](auto name) { const auto token=std::bit_cast<std::int32_t>(scriptStrings_.Store(name));client_.Input().PushCommand(kReadFortCommand,token,token); }; //원문 mode0은 같은 현재 이름 token을 두 인자에 복제하여 일반 입력 큐에 예약한다.
    rawButtons_->Menus()->ShowSaveFortNameDialog(std::move(context),fortMode,1); //원문 일반 입력은 event.y,1,NULL의 세 인자를 같은 공장에 전달한다.
}
//State1과3이 같은 Native 장치의 공통 준비를 원문 순서로 사용하고 선택 Editor 전역만 구별한다.
void UberGump::PrepareNativeFortState(bool editing) {
    if (!editor_) throw std::logic_error("실제 State1/3 공통 장기 장치 누락");
    if (editing) PrepareNativeEditorEntry(client_.LocalStateGlobals()->State(),client_.LocalStateGlobals()->SourceType(),htmlGlobals_.background,client_.SurfaceStorage()->ownerMode.battle,[this] { if (!world_) throw std::logic_error("State1의 이전 Battle14 실제 Session 없음");SendBattleLeaveForState(world_->Session()); }); // 공통 출처/첫BYTE/이전 Battle 통지를 cleanup 전에 처리하며 State3의 전용 효과는 반복하지 않는다.
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
//현재 State가1/2일 때 같은 실제 Editor 외곽을 준비하며 State 확정은 호출한 RawLocalMissionState에 남긴다.
o::LocalMissionParentResult UberGump::InstallExistingEditorState(std::uint32_t selected) {
    if ((selected!=1 && selected!=2) || !editor_) throw std::logic_error("실제 Edit State1/2 외곽 계약");
    PrepareNativeFortState(true);return InstallPreparedFortState(true);
}
// 이미 실행한 Native prefix를 반복하지 않고 State1/2의 실제 파일·월드·UI 꼬리만 잇는다.
o::LocalMissionParentResult UberGump::InstallPreparedFortState(bool prefixHud,std::function<void()> battleOver) {
    const auto current=client_.LocalStateGlobals()->State().current[0];
    if (!editor_ || world_ || (current!=1 && current!=2 && current!=6)) throw std::logic_error("준비된 Fort 설치의 실제 State1/2/6·빈 호스트 월드 계약");
    auto& e=*editor_;
    //호스트 Session은 준비된 물리만 빌리고 파일 설치/Template 변경은 아래 실제 checkpoint의 위치까지 지연한다.
    const std::string pending(client_.FortGlobals()->Pending()); // 실제 이름 선택/기본 이름은 checkpoint가 수행하므로 여기서는 파일을 조회하거나 파싱하지 않는다.
    auto services=MakeCampaignClientServices(client_,*rawButtons_,{[this] { RefreshWindowTitle(); },[this](auto name) { Tell(std::string(name)); },[this](auto kind,auto player,auto parameter) { warnings_->Remove(kind,player,parameter);rebuild_=true; },[this](auto packet) { ReceiveChatMessage(packet); }});
    services.missionOutcome.demo=[this] { return missionRuntime_ ? missionRuntime_->gate.demo : 0U; }; // 편집 세션도 원문 Guide의 현재 +68을 조회하며 미션이 없으면 0을 반환한다.
    services.installMode=CampaignInstallMode::ExistingEditor;services.deferExistingEditorFile=true;services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.adoptMainMenuFort=!prefixHud || current==6;services.localLogin.reset(); // 현재6은 State3 continuation과 같은 비편집 Fort 모드를 빌리되 첫 HUD1은 실제로 실행한다.
    services.templateStorage=templateStorage_;services.missionOptions=missionOptions_;services.missionRestrictions=missionRestrictions_;services.editorTransport=e.transport;services.fortMission=FortMissionSaveEffects();
    services.fortBadges=client_.FortBadges();services.fortTypeConversion=client_.FortTypeConversion(); //기존 파일의 실제 장부/변환표를 캠페인과 같은 Client owner에서 빌린다.
    world_=std::make_shared<GameWorld>(client_.Assets(),client_.Files(),o::FortTemplate{},o::MissionPlayers{},pending,std::move(services),ScreenPoint{client_.GetScreen().Width(),client_.GetScreen().Height()},true);
    world_->elapsed=[this] { return client_.Time().delta; };world_->paused=[this] { return client_.Paused(); };world_->frameTime=[this] { return client_.Time(); };
    world_->BindHideBuildings([this] { return hideBuildings_; });world_->soundViewChanged=[this](auto view) { client_.WorldAudio().SetView(view); };
    world_->surfaceDisplay={[this] { return client_.GetRenderer().FullRedrawPending(); },[this](auto rect,auto flags) { client_.GetRenderer().Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }};
    auto& session=world_->Session();if (prefixHud) session.WithExistingEditorPrefixUi(*client_.LocalStateGlobals(),[this] { ApplyHudModeForState(1); }); // State3 제목 뒤의 continuation은 앞서 실제 HUD를 실행했으므로 다시 분배하지 않는다.
    static_cast<void>(EnsureEditorFortFiles()); // 첫 ReadFort에서 이미 만든 장치도 재생성하지 않아 같은 현재 경로/장부/reader 수명을 유지한다.
    CampaignExistingEditorCheckpointServices checkpoint;
    checkpoint.fort.currentName=[this] { return client_.FortGlobals()->Current(); };checkpoint.fort.pendingName=[this] { return client_.FortGlobals()->Pending(); };
    checkpoint.fort.compare=[](auto first,auto second) { return o::CompareDataManagerSectionNames(first,second); };
    checkpoint.fort.setName=[this](auto name) { SelectCurrentFortName(name); };checkpoint.fort.missionName=[this](auto name) { return mapCatalog_->Kind(name); };
    checkpoint.fort.defaultName=[this] { SelectConfiguredNewFort(client_.Configuration(),[this](auto name) { SelectCurrentFortName(name); }); }; // 원문 기본 이름도 현재 Template/작업 namespace의 같은 전체 선택 부모를 사용한다.
    checkpoint.fort.closeGuide=[this] { ReleaseCurrentGuideMission(); }; //원문 부모가 이름 변경/현재 미션/대상 맵 속성을 판정한 뒤에만 보존한 Tutorial 데이터와 Guide를 해제한다.
    checkpoint.fort.setConfig=[this](auto name,auto value) { client_.Configuration().Set(name,o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()})); };
    checkpoint.fort.requestMission=[this,&session] { RequestCurrentGuideFort(session);state_.Post({"LoadMissionWorld",{}}); }; // 같은 Guide/State7 요청을 다음 호스트 작업으로 넘기며 편집 설치 꼬리를 실행하지 않는다.
    checkpoint.fort.outside=[](auto branch) { throw std::logic_error("Editor 기존 파일 외의 설치 분기:"+std::to_string(static_cast<int>(branch))); };
    checkpoint.file.path=[&e] { return e.paths->PrepareCurrent(); };checkpoint.file.exists=[&e](auto path) { return e.paths->Exists(path); };checkpoint.file.title=[this] { RefreshWindowTitle(); }; // 기본 이름/설정 알림 뒤 선택된 실제 파일도 기존 이관 장부로 준비한 다음 존재를 검사한다.
    checkpoint.file.assertion=[](auto line) { std::clog<<"Editor Template 진단 "<<line<<'\n'; };checkpoint.file.outside=checkpoint.fort.outside;
    TemplateFileLoadHooks load;load.wrongName=[] { std::clog<<"Editor 파일명 불일치\n"; };load.tooManyTypes=[] { std::clog<<"Editor TYPE 개수 진단\n"; };load.rejectFile=[&e](auto path) { e.files.Delete(path); };checkpoint.load=e.paths->BindLoad(std::move(load));
    checkpoint.invalid.log=[this](auto channel,auto text) { client_.WriteDiagnosticLog(static_cast<std::int32_t>(channel),text); };
    checkpoint.invalid.isDefaultName=[this] { const auto name=client_.Configuration().PathSpec("newFort");const std::string current(client_.FortGlobals()->Current());return _stricmp(current.c_str(),name.c_str())==0 ? 1 : 0; }; // 설정 해석 뒤의 현재 이름을 실제 CRT로 비교한다.
    checkpoint.invalid.erase=[&e](auto path) { e.files.Delete(path); }; // 원본 자산이 아닌 같은 작업본만 삭제한다.
    checkpoint.startup=EditorStartupStateHooks(); //State3과 같은 실제 기본 옵션/파일 부재 관문/벽시각 장치를 빌린다.
    if (!session.InstallExistingEditorCheckpoint(checkpoint,client_.ReadOnlyForts(),e.files)) return o::LocalMissionParentResult::ReturnWithoutCommit; // 원문 미션 분기는 공통 State1 확정도 생략한다.
    ExistingEditorAfterCheckpointEffects tail;tail.hud=[this](auto command) { rawButtons_->NotifyHudMode(command,1); };tail.wallSeconds=[this] { return client_.WallSeconds(); };
    tail.requestMusic=[this](auto name) { client_.WorldAudio().RequestMusic(name); };tail.fortPalette=[this] { const auto name=client_.Configuration().PathSpec("fortPal");client_.ChangeFortPalette({reinterpret_cast<const std::uint8_t*>(name.c_str()),name.size()+1}); };tail.islandCount=[&session] { return session.IslandCount(); };
    tail.battleOver=std::move(battleOver); // State6 앞단이 보관한 실제 결과/출처와 옵션 공장의 수명을 그대로 전달한다.
    session.WithExistingEditorInstallUi(*client_.LocalStateGlobals(),client_.Input(),tail,[this,&session,&e] {
        auto effects=MakeCampaignCursorClientEffects(client_,*rawButtons_);e.selectCursor=effects.lifecycle.selectCursor;
        effects.send=[&session](auto target,auto packet) { session.SendLocalPacket(std::bit_cast<std::int32_t>(target),packet); };
        effects.lifecycle.highlight=[this](auto mode) { world_->Highlight(mode); };effects.drop.refresh=[this](auto sid,auto force,auto move) { if (force || move!=1) throw std::logic_error("Editor 커서 선택 인자 계약");world_->Select(sid); };
        session.Cursor().SetEffects(std::move(effects));session.Cursor().Display().SetExternal([this](auto token) { return rawButtons_->GumpDepth(token); },{});
        InstallHud();world_->Resize(client_.GetScreen().Width(),client_.GetScreen().Height());world_->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical());
    });
    return o::LocalMissionParentResult::Commit;
}
// State6은 정리 전 결과를 지역값으로 보유하고 같은 일반 Fort의 파일/월드/후반 UI를 이어서 설치한다.
o::LocalMissionParentResult UberGump::InstallBattleReturnNativeState(std::uint32_t selected) {
    if (selected!=6 || !editor_) throw std::logic_error("일반 복귀 State6의 실제 장기 부모 계약");auto& local=*client_.LocalStateGlobals();auto& surface=*client_.SurfaceStorage();
    editor_->multiplayerOptions=[this] { OpenMultiplayerOptions(); }; // 실제 옵션 공장을 현재 Uber/Client 수명에 연결하며 창 생성은 후반 원문 관문까지 지연한다.
    editor_->connectLan=[this] { OpenLanChoice(); }; // 공식 서버 화면 대신 실제 로컬 대기실 선택을 제공한다.
    NativeBattleReturnHooks hooks;hooks.deleteSavedState=[this] { static_cast<void>(DeleteSavedStateFile(client_.GameDirectory(),client_.StaticStrings()->FileNames(),client_.UserSavedStateFile())); };
    hooks.requestMusic=[this](auto name) { client_.WorldAudio().RequestMusic(name); };
    hooks.sourceBackground=[this](auto type)->std::optional<std::string> { const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("일반 복귀 source TYPE 이름 범위");const auto name=types[type].name;if (!client_.Files().TryRead("d/"+name+".gif")) return std::nullopt;return name; };
    hooks.leaveBattle=[this] { if (!world_) throw std::logic_error("State6의 이전 Battle14 실제 Session 없음");SendBattleLeaveForState(world_->Session()); };hooks.prefix=[this] { PrepareNativeFortState(false); };
    hooks.installFort=[this](const auto& saved) { return InstallPreparedFortState(true,[this,saved] { CompleteNativeBattleReturn(saved,client_.WorldAudio().ResultState(),{editor_->multiplayerOptions,[this](auto section) { Tell(std::string(section)); }}); }); }; // 콜백은 보관 지역값을 값으로 소유해 실제 Fort 적재 중 전역 변경과 구별한다.
    return netstorm::client::InstallNativeBattleReturnState({local.State(),local.SourceType(),client_.WorldAudio().ResultState(),surface.ownerMode.battle,htmlGlobals_.background},hooks);
}
// 일반 옵션의 원문 표시/선택 장치는 같은 현재 Fort/사용자 이름을 매 호출마다 조회한다.
void UberGump::OpenMultiplayerOptions() {
    if (!world_ || !editor_) throw std::logic_error("일반 옵션 공장의 실제 Fort/장기 장치 없음");MultiplayerOptionsWindowContext context;context.editor.now=[this] { return client_.WallSeconds(); };BindTextClipboard(context.editor,textClipboard_);
    context.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };context.fortName=[this] { return std::string(client_.FortGlobals()->Current()); };
    context.playerName=[this] { if (!world_) throw std::logic_error("일반 옵션 현재 이름의 실제 Session 없음");return std::string(world_->Session().CurrentHandle()); };
    context.serverName=[this] { std::string address;client_.Configuration().ReadString("lanServerAddress",address);return address.empty() ? FontBytes(client_.Translate("Find Local Server")) : address; }; // 공식 서버 목록 대신 명시한 로컬 TCP 주소만 표시한다.
    context.rememberFort=[this](auto name) { client_.Configuration().Set("lastMultiplayerFort",name); };context.special=[this] { return client_.GameplayGlobals()->player.special!=0; };
    context.action=[this](auto action) {
        switch (action) {
        case MultiplayerOptionsAction::ChooseFort:fortSelectionMode_=0;mapFileSpec_.SelectFort();client_.FortGlobals()->SelectFileSpec();Tell("LoadMultiplayer");break;
        case MultiplayerOptionsAction::ChooseName:client_.Input().PushCommand(0x020000b9U,0,0);break; // 원문 SetName은 같은 일반40칸 고리에 예약한다.
        case MultiplayerOptionsAction::ChooseServer:OpenMultiplayerInput(true);break;
        case MultiplayerOptionsAction::ChooseProtocol:break; // 제품은 원문 TCP/IP 행을 고정하며 SPX/IPX/폐기 인터넷 선택은 비활성이다.
        case MultiplayerOptionsAction::Connect:client_.Input().PushCommand(0x020000c4U,0,1);break; // Native Connect 인자1은 실제 다음 입력 소비자가 처리한다.
        case MultiplayerOptionsAction::Cancel:client_.Input().PushCommand(0x02000095U,0,std::bit_cast<std::int32_t>(scriptStrings_.Store("BLANK")));break;
        }
    };
    multiplayerOptionsToken_=rawButtons_->Menus()->ShowMultiplayerOptions(std::move(context));rebuild_=true;
}
// 기존 Input 공장의 확정 문자열을 같은 실제 Subscriber/로컬 주소 설정에 전달한다.
void UberGump::OpenMultiplayerInput(bool server) {
    if (!world_) throw std::logic_error("일반 옵션 입력의 실제 Fort 없음");SaveFortNameDialogContext context;context.editor.now=[this] { return client_.WallSeconds(); };BindTextClipboard(context.editor,textClipboard_);context.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };
    const std::string key=server ? "LanServerAddress" : "LanPlayerName";
    context.tell=[this,server,key](auto section) {
        const std::string prefix=key+"(";if (!section.starts_with(prefix) || section.empty() || section.back()!=')') { Tell(std::string(section));if (server && o::AsciiLower(section)=="blank") ResumeLanChoiceAfterInput();return; } // 주소 입력 취소도 이 선택에서 열린 경우에만 원래 LAN 선택으로 돌아간다.
        const auto value=section.substr(prefix.size(),section.size()-prefix.size()-1);
        if (server) client_.Configuration().Set("lanServerAddress",std::string(value));
        else { if (!world_) throw std::logic_error("이름 확정의 실제 Fort 없음");std::vector<std::uint8_t> name(value.begin(),value.end());name.push_back(0);world_->Session().ChangeFortSubscriber(world_->Session().CurrentSubscriber(),std::span<const std::uint8_t>(name)); }
        rawButtons_->Menus()->RefreshMultiplayerOptions(multiplayerOptionsToken_);if (server) ResumeLanChoiceAfterInput();rebuild_=true;
    };
    rawButtons_->Menus()->ShowHtmlInputDialog(std::move(context),1,server ? "Local TCP/IP Server Address?" : "Player Name?",key);rebuild_=true;
}
// 일반 입력 큐에서만 이름/연결 명령을 소비하여 Gump callback 안에서 World를 교체하지 않는다.
bool UberGump::MultiplayerOptionsEvent(const InputEvent& event) {
    const auto code=event.code&0x03ffffffU;if (code!=0x020000b9U && code!=0x020000c4U) return false;if (event.code&InputCode::kRelease) return true;
    if (code==0x020000c4U && event.y==0) { fortSelectionMode_=0;const auto name=client_.Configuration().PathSpec("lastMultiplayerFort");FortEditEffects().pending(name);RequestFortRead(client_.LocalStateGlobals()->State().next[0]);return true; } // 원문 Connect의 일반 메뉴 분기처럼 마지막 Fort→pending→next6을 예약한다.
    if (!editor_ || !world_ || !client_.SurfaceStorage()->ownerMode.fort) return false;
    if (code==0x020000b9U) { OpenMultiplayerInput(false);return true; }
    switch (event.y) { //1은 원문 옵션 Connect이며2..6은 종료된 공식 매칭을 대체하는11.01 로컬 선택 인자다.
    case 1:if (!editor_->connectLan) throw std::logic_error("일반 LAN 선택 장치 미연결");editor_->connectLan();break;
    case 2:StartLanLobby(true);break;
    case 3:StartLanLobby(false);break;
    case 4:OpenMultiplayerInput(true);break;
    case 5:StartLanLobby(true,true);break; // 이미 열린 host 대기실에만 Start 합의를 요청한다.
    case 6:CloseLanLobby();OpenMultiplayerOptions();break;
    default:throw std::invalid_argument("로컬 LAN Connect 선택 인자 범위");
    }return true;
}
//원문 startupStateCheck의 실제 선택 장치를 두 Native 부모가 같은 장기 파일 경로로 공유한다.
o::StartupStateCheckHooks UberGump::EditorStartupStateHooks() {
    if (!editor_) throw std::logic_error("State1/3 startup 장기 장치 누락");o::StartupStateCheckHooks startup;
    startup.configInt=[this](auto name) { return client_.Configuration().GetInt(name); }; //선택된 saved-state reader의 누락을 옵션 강제0으로 숨기지 않는다.
    startup.readSavedState=[this] { return ReadClientSavedState(client_); }; // 존재하는 실제 사용자 파일도 같은 Player/Core/복구 슬롯으로 읽는다.
    struct StartupText { // 동기 Check 호출 동안 첫Yes/No/질문과 두 서식 결과의 별도 주소를 유지한다.
        std::array<std::string,3> translated;std::size_t next{}; // 원문의 번역 세 결과를 뒤 소비까지 보관한다.
        std::string buttons,warning; // 원문의 지역256BYTE 버튼과cf550에 넘기는 실제 질문 결과다.
    };
    const auto text=std::make_shared<StartupText>();
    startup.translate=[this,text](auto key)->std::string_view { auto& slot=text->translated[text->next++%3];slot=FontBytes(client_.Translate(key));return slot; };
    startup.formatButtons=[text](auto pattern,auto yes,auto no)->std::string_view {
        const std::string format(pattern),first(yes),second(no);std::array<char,256> output{};const auto count=std::snprintf(output.data(),output.size(),format.c_str(),first.c_str(),second.c_str());
        if (count<0 || static_cast<std::size_t>(count)>=output.size()) throw std::length_error("재접속 질문 버튼256BYTE 초과");text->buttons.assign(output.data(),static_cast<std::size_t>(count));return text->buttons;
    };
    startup.formatWarning=[text](auto pattern,double minutes,auto buttons)->std::string_view {
        std::size_t argument{};
        // 실제 공급한 double/문자열만 CRT가 읽게 하고 %%와 상수 폭/정밀도를 보존한다.
        for (std::size_t at=0;at<pattern.size();++at) if (pattern[at]=='%') {
            if (++at<pattern.size() && pattern[at]=='%') continue;
            while (at<pattern.size() && (pattern[at]=='-' || pattern[at]=='+' || pattern[at]==' ' || pattern[at]=='#' || pattern[at]=='.' || (pattern[at]>='0' && pattern[at]<='9'))) ++at;
            if (argument>=2 || at==pattern.size() || pattern[at]!=(argument==0 ? 'f':'s')) throw std::invalid_argument("재접속 질문 번역의 실제 두 인자 서식 오류");++argument;
        }
        if (argument!=2) throw std::invalid_argument("재접속 질문 번역 인자 개수 오류");const std::string format(pattern),suffix(buttons);const auto count=std::snprintf(nullptr,0,format.c_str(),minutes,suffix.c_str());if (count<0 || count>=8192) throw std::length_error("재접속 질문 실제 HTML8192BYTE 초과");
        std::vector<char> output(static_cast<std::size_t>(count)+1);static_cast<void>(std::snprintf(output.data(),output.size(),format.c_str(),minutes,suffix.c_str()));text->warning.assign(output.data(),static_cast<std::size_t>(count));return text->warning;
    };
    startup.tell=[this](auto body,auto width,auto height) { static_cast<void>(rawButtons_->Menus()->ShowHtmlWindow(LoadingHtmlContext(CurrentMapView()),body,0,width,height));rebuild_=true; }; //cf550은cf270(text,0,width,height)이며 별도 MessageBox/정지를 추가하지 않는다.
    startup.wallSeconds=[this] { return client_.WallSeconds(); };return startup; //reader0 복귀에서도 원문처럼 현재 벽시각을 읽는다.
}
//실제 State3는 World를 다시 만들지 않고 같은 Client의 Native 부모와 공통 tail을 실행한다.
o::LocalMissionParentResult UberGump::InstallMainMenuNativeState(std::uint32_t selected) {
    if (selected!=3 || !editor_) throw std::logic_error("실제 메인 메뉴 State3 장기 부모 계약");
    auto& local=*client_.LocalStateGlobals();auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();auto& names=*client_.FortGlobals();
    NativeMainMenuStateGlobals globals{local.State(),local.SourceType(),client_.WorldAudio().ResultState(),local.FirstMenuPending(),surface.ownerMode.battle,game.world.editor,htmlGlobals_.background,names.CurrentBytes(),names.PendingBytes(),{},[&local] { return local.TellTips(); }};
    globals.referenceValidationNotice=false; // 원문 인터넷 검증은 서버 서비스 종료로 비활성화한다. 성공1을 합성하지 않고 조회/안내를 모두 생략하며 LAN에 검증을 요구하지 않는다.
    NativeMainMenuStateHooks hooks;hooks.configInt=[this](auto key) { return client_.Configuration().GetInt(key); };
    hooks.request=[this](auto state) { editor_->state->Request(state); }; //같은 실행 중인 Native 분배기의 immediate를 사용한다.
    hooks.deleteSavedState=[this] { static_cast<void>(DeleteSavedStateFile(client_.GameDirectory(),client_.StaticStrings()->FileNames(),client_.UserSavedStateFile())); }; // 원문 State3는 경로를 매번 계산하고 CRT 삭제 실패도 상태 전환을 중단시키지 않는다.
    hooks.requestMusic=[this](auto name) { client_.WorldAudio().RequestMusic(name); };
    hooks.sourceBackground=[this](auto type)->std::optional<std::string> {
        const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("State3 source TYPE 이름 범위");const auto name=types[type].name;
        if (!client_.Files().TryRead("d/"+name+".gif")) return std::nullopt;return name; //원문 고정 data 디렉터리의 .gif 존재만 판정하며 없는 파일을 생성하지 않는다.
    };
    hooks.leaveBattle=[this] { if (!world_) throw std::logic_error("State3의 이전 Battle14 실제 Session 없음");SendBattleLeaveForState(world_->Session()); }; // State1과 같은 실제52h 예약이며 수신/flush를 강제하지 않는다.
    hooks.prefix=[this] { PrepareNativeFortState(false); };hooks.hudPrefix=[this] { ApplyHudModeForState(1); };
    hooks.title=[this] { RefreshWindowTitle(); };hooks.closeGuide=[this] { ReleaseCurrentGuideMission(); };
    hooks.tell=[this](auto section) { Tell(std::string(section)); };hooks.startup=[this] { return o::CheckStartupState(EditorStartupStateHooks()); };
    hooks.editorPlayers=[this] { client_.Configuration().Set("lastEditFort",client_.FortGlobals()->Current());if (!world_) throw std::logic_error("State3 Tell 뒤 선택된 EditorPlayers의 실제 Session 없음");world_->Session().InstallExistingEditorPlayers(MakeEditorPlayerClientHooks(client_)); };
    hooks.changedState=[this] { return InstallPreparedFortState(false); }; // 원문 LAB_004b72eb에서 같은 파일/미션 조기 반환/월드/tail을 이어가며 앞단과 메인 전용 안내를 반복하지 않는다.
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
    if (selected<39 || selected>42 || !editor_) throw std::logic_error("실제 인트로 State39/40/41/42 장기 부모 계약");
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
    hooks.titleCursor=[this](bool show) { client_.GetScreen().ShowCursor(show); };hooks.titleSplash=[this] { client_.RunInitialTitleBitmaps(); };
    hooks.restoreTitlePalette=[this] { client_.WriteDiagnosticLog(29,"init palette\n");const auto palette=client_.Configuration().PathSpec("fortPal");const auto path=client_.Configuration().PathSpec("GamePalSpec",palette);client_.LoadPaletteFile(path,1); }; //원문두ConfigPath호출은서로다른지역반환수명을유지한다.
    hooks.request=[this](auto next) { editor_->state->Request(next); };
    return netstorm::client::InstallNativeIntroState({client_.LocalStateGlobals()->State(),surface.ownerMode.fort,game.Server(),game.Boss(),client_.FortGlobals()->PendingBytes()},hooks);
}
// 미션 Login은 기존 분배기에 남기고 Edit1/2·메인3·인트로39~42과 다음4를 같은 분배기로 처리한다.
void UberGump::PollEditorState() {
    auto& local=*client_.LocalStateGlobals();const auto next=local.State().next[0];const bool fort=next==1 || next==2 || next==6; // 일반 복귀6도 같은 Fort 설치/공통 확정을 사용하며 첫 배경/결과만 전용 부모가 처리한다.
    if (!fort && next!=3 && !(next>=19 && next<=27) && !(next>=34 && next<=38) && !(next>=39 && next<=42) && !(editor_ && next==4)) return; //메타19/연결20~27/검색34~38도같은장기분배기를사용한다.37의새서버실행은후속이다.
    if (!editor_) editor_=std::make_unique<EditorRuntime>(client_);
    if (!editor_->state) { // 새 이름 창에서 먼저 확보한 파일 장치가 있어도 실제 State 분배기는 별도로 처음 한 번 구성한다.
        auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();
        o::LocalMissionStateHooks hooks;hooks.logState=[](auto value) { std::clog<<"State "<<value<<'\n'; };hooks.parentState=[this](auto selected) { if (selected>=39 && selected<=42) return InstallIntroNativeState(selected);if (selected==19) return InstallMetaNativeState(selected);if ((selected>=20 && selected<=27) || (selected>=34 && selected<=38)) return InstallMetaConnectionState(selected);if (selected==6) return InstallBattleReturnNativeState(selected);return selected==3 ? InstallMainMenuNativeState(selected) : InstallExistingEditorState(selected); };
        hooks.invalidState=[](auto state) { throw std::logic_error("Editor/메인 분배기의 선택된 Native State 미연결:"+std::to_string(state)); };
        hooks.metaThird=[this] { rawButtons_->Menus()->RefreshMetaNsInfo(*client_.NsInfoGlobals()); }; //0047cab0은별도CS목록이아닌기존00593150 NSInfo를실제Push/Attach한다.
        editor_->state=std::make_unique<o::RawLocalMissionState>(client_.Assets().Edition(),local.State(),o::LocalMissionStateGlobals{game.Server(),game.Boss(),surface.ownerMode.battle,game.world.missionPost,local.SavedGame(),surface.links.mission,local.Simulconnect(),surface.ownerMode.challenge,game.player,local.ZonePending(),local.GameTicks(),local.SourceType(),local.CdPresent(),local.Blocks()[2][33],local.MetaRefresh(),local.MetaRebuild(),reinterpret_cast<std::uint8_t&>(htmlGlobals_.background[0]),local.BattleByte(),local.ConnectEnd(),local.ActivityEnd(),client_.Time().game,client_.Time().wall},std::move(hooks));
    }
    if (fort || next==3) editor_->committed=false;editor_->state->Poll();
    if (next>=39 && next<=42) {
        if (local.State().phase[0]==3 && local.State().next[0]==4) PublishNativeMainMenuState();
        return; // 영상이 살아 있는 State40은 다음 Poll까지 유지하며 메인 메뉴를 먼저 공개하지 않는다.
    }
    if (next==3 && local.State().phase[0]==3 && local.State().next[0]==4) { PublishNativeMainMenuState();return; }
    if ((fort || next==3) && local.State().next[0]==7) return; // 두 진입의 실제 Guide 요청은 입력/loaded 공개 없이 다음 Tick의 지연 미션 부모가 소비한다.
    if ((fort || next==3) && !editor_->committed) {
        world_->CompleteFortInstall(local); //공통 확정 뒤 실제 loaded 공개가 끝나야 호스트 사용자 입력 몸체를 만들 수 있다.
        BindCampaignUserInputServices(client_,*world_,editor_->selectCursor);userInput_=std::make_unique<UserInput>(client_,*world_);
        editor_->committed=true;if (next!=6 || state_.phase!=ClientPhase::Dialog) { state_.phase=ClientPhase::Mission;pageName_=client_.GameplayGlobals()->world.editor ? "editor" : "mission";page_={};popup_.clear(); }RebuildControls(); // 일반 복귀의 실제 BattleOver Tell이 만든 호스트 모달 상태도 보존한다.
    }
}
