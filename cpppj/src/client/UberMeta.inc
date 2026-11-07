// 바인딩 때 아직 없는 메타 Session/전송 owner는 실제 입력을 선택한 시점에 조회한다.
void UberGump::BindMetaNsInfoInput(RawNsInfoContext& context) {
    context.center=[this](auto sid) {
        auto& camera=client_.GameplayGlobals()->Camera();const auto before=camera.RendererView().pixels;CameraHomeState state;state.width=client_.GetScreen().Width();state.height=client_.GetScreen().Height();
        client_.GameplayGlobals()->RequireSession().CenterCameraOn(sid,state,[this](auto,auto) { CommitCameraNow(); });
        if (camera.RendererView().pixels!=before) { if (world_) world_->CameraApplied();rebuild_=true; }
    };
    context.status=[this](const auto& writer) { if (!editor_ || !editor_->metaTransport) throw std::logic_error("NSInfo 입력의 현재 메타 전송 owner 미연결");editor_->metaTransport->SendMetaStatus(writer); }; //1fh 실제 헤더가 먼저 작성된 뒤 caller가 CS/음향 부분효과를 수행한다.
    context.kickoffSound=[this] { client_.WorldAudio().Sounds().PlayByName("chalKickedoff.wav",0,0,0,1,0); }; //원문 priority1을 일반 메뉴 소리의0으로 바꾸지 않는다.
    context.sendData=[this](auto key,auto data,auto count) { if (!editor_ || !editor_->metaTransport) throw std::logic_error("NSInfo 자료 송신의 현재 메타 전송 owner 미연결");editor_->metaTransport->SendMetaData(key,data,count); }; //실제 옵션 변경 뒤에만 현재67h/TCP11을 선택한다.
    context.menuType=[this](auto address) { return client_.GameplayGlobals()->RequireSession().MetaMenuType(address); };context.globallyExcluded=[this](auto type) { return o::BattleTypeExcluded(*missionOptions_,type)!=0; }; //편집중인CS옵션이아닌같은현재전역87BYTE를읽는다.
    context.optionChoice=[this](auto index,auto selected) {
        const auto number=std::bit_cast<std::uint32_t>(index);const auto address=o::BattleOptionChoiceAddress(number,selected);auto text=o::BattleOptionChoiceText(number,selected,[this](auto key) { return FontBytes(client_.Translate(key)); });const auto& definition=o::BattleOptionText(number);
        const auto identity=definition.translateValue ? client_.TranslationIdentity(definition.values[selected],address):std::uint64_t(address);return BattleOptionMenuText{std::move(text),identity}; //표시바이트와현재소유Xlat정체성을구별하며임시string주소를쓰지않는다.
    };
}
// 실제 State19의 World 정리 뒤 같은 Client Fort/Core/CS/Chat 장치를 선택한다.
o::LocalMissionParentResult UberGump::InstallMetaNativeState(std::uint32_t selected) {
    if (selected!=19 || !editor_) throw std::logic_error("실제 State19 장기 부모 계약");
    auto& local=*client_.LocalStateGlobals();auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();NativeMetaStateHooks hooks;
    hooks.fortPalette=[this] { return client_.Configuration().PathSpec("fortPal"); };
    hooks.applyPalette=[this](auto path) { client_.ChangeFortPalette({reinterpret_cast<const std::uint8_t*>(path.data()),path.size()+1}); };
    hooks.cleanup=[this](auto reason) { CleanupNativeStateWorld(reason); };
    hooks.checkCd=[this] { return client_.CdGlobals()->Check(MakeWindowsCdPresenceHooks(&client_.StaticStrings()->FileNames())); };
    hooks.initializeRenderer=[this] { client_.InitializeNativeRendererView(); };hooks.assertion=[](auto line) { std::clog<<"State.cpp CS->listsInitialized 진단:"<<line<<'\n'; };
    hooks.request=[this](auto next) { editor_->state->Request(next); };
    hooks.selectCore=[this,&game,&surface](auto owner,auto templateOwner) { const std::uint32_t mode=surface.ownerMode.challenge;client_.CoreDataGlobals()->Select(game.player,mode,owner,templateOwner); };
    hooks.currentFortPath=[this] { return std::string(EnsureEditorFortFiles().PrepareCurrent()); }; //같은 사용자 홈/기존 자산 이관 정책에서 실제 현재 파일만 준비한다.
    hooks.loadFort=[this](auto owner,auto path) {
        auto& files=EnsureEditorFortFiles();TemplateFileLoadHooks load;load.wrongName=[] { std::clog<<"메타 Fort 파일명 불일치\n"; };load.tooManyTypes=[] { std::clog<<"메타 Fort TYPE 개수 진단\n"; };
        load.rejectFile=[this](auto name) { editor_->files.Delete(name); };load.typeCount=[this] { return static_cast<std::int32_t>(client_.Assets().TypeTable().Types().size()); };
        load.currentSlot=[this] { return client_.GameplayGlobals()->player.templateOwner; };load.refreshTitle=[this] { RefreshWindowTitle(); };
        templateStorage_->LoadFile(owner,{},path,files.BindLoad(std::move(load))); //동일 reader/manager 수명이며 별도 GameWorld를 만들지 않는다.
    };
    hooks.loadCore=[this,&game,&surface](auto owner,auto minimum) {
        FortCoreDataLoadContext context{game.world.missionPost,game.world.editor,surface.ownerMode.challenge,game.player,*client_.CoreDataGlobals(),templateStorage_->LoadingDepth()};
        context.seed=[this]()->std::uint32_t& { return client_.RandomGlobals()->FortCoreSeed(); };
        context.assertion=[](auto expression,auto file,auto line) { std::clog<<file<<':'<<line<<' '<<expression<<'\n'; };
        context.refreshRank=[this,&surface] { RefreshClientChatRankPrefix(client_,client_.Assets(),surface); };context.invalidData=[&game] { game.PatchDiagnostic()=game.PatchSample(); };
        LoadFortCoreData(*templateStorage_,owner,minimum!=0,std::move(context));PrepareMetaWorldRuntime(); //같은Fort/캐시읽기뒤HUD가필요로하는실제Pool·Form처리기만구성한다.
    };
    hooks.hud=[this](auto command,auto mode,auto,auto) { rawButtons_->NotifyHudMode(command,mode); }; //기존 장치는 원문 인자0/숨김포함1을 그대로 사용한다.
    hooks.wallSeconds=[this] { return client_.WallSeconds(); };hooks.loadNewsTime=[this] { LoadClientSystemNewsTime(client_); };
    hooks.battleChannel=[this](auto mode) { RefreshClientBattleChatChannel(client_,*rawButtons_->Menus(),mode); };
    hooks.inputVisible=[this](auto visible) { const auto token=client_.ChatWindowGlobals()->window.current;if (token) rawButtons_->Menus()->ChatGeometryBackend()->SetInputVisible(token,visible); };
    return netstorm::client::InstallNativeMetaState({local.State(),htmlGlobals_.background,local.MetaCdPresent(),surface.ownerMode.challenge,game.Server(),game.Boss(),local.MetaPending(),game.player,local.HudModeCommand(),local.ActivityEnd(),*client_.ChallengeGlobals()},hooks);
}
// 호스트 처리기 대여는 Native 메타 전역을 다시 초기화하거나 Fort의 빈 복사본을 설치하지 않는다.
void UberGump::PrepareMetaWorldRuntime() {
    if (!editor_ || world_ || editor_->metaWorld || client_.LocalStateGlobals()->State().current[0]!=19) throw std::logic_error("실제 State19 메타 Runtime의 한 번 대여 계약");
    auto services=MakeCampaignClientServices(client_,*rawButtons_,{[this] { RefreshWindowTitle(); },[this](auto name) { Tell(std::string(name)); },[this](auto kind,auto player,auto parameter) { warnings_->Remove(kind,player,parameter);rebuild_=true; },[this](auto packet) { ReceiveChatMessage(packet); }});
    services.installMode=CampaignInstallMode::MetaMap;services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.localLogin.reset();
    services.templateStorage=templateStorage_;services.missionOptions=missionOptions_;services.missionRestrictions=missionRestrictions_;
    editor_->metaWorld=std::make_unique<RawCampaignSession>(client_.Assets(),client_.Files(),o::FortTemplate{},std::string(client_.FortGlobals()->Current()),std::move(services));
    auto* session=editor_->metaWorld.get();editor_->metaDisplay=std::make_unique<RawCampaignFactoryDisplay>(session->Core(),client_.Assets(),[session](auto sid,const auto& draw) { session->DrawTemple(sid,draw); },&client_.GameplayGlobals()->PatchDiagnostic());
    editor_->metaDisplay->BindOwnerColors(client_.ColorSortInput()->OwnerTables());editor_->metaDisplay->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical(),&client_.ColorSortInput()->Maps());
}
RawCampaignFactoryDisplay* UberGump::MetaSpriteDisplay() const { return editor_ ? editor_->metaDisplay.get() : nullptr; } //현재owner를조회만하며출력기나Session을새로만들지않는다.
// 실제19의 World 정리 뒤 같은 Client 물리로만 메타 연결 owner를 처음 만든다.
o::LocalMissionParentResult UberGump::InstallMetaConnectionState(std::uint32_t selected) {
    if (!editor_ || world_ || !((selected>=20 && selected<=27) || (selected>=34 && selected<=38))) throw std::logic_error("World 없는 실제 메타 연결/검색 부모 계약");
    if (!editor_->metaTransport) editor_->metaTransport=std::make_unique<MetaConnectionRuntime>(client_,[this](auto text) { static_cast<void>(ShowLoginWaiting(FontBytes(text))); },[this](auto next) { editor_->state->Request(next); },[this] { editor_->state->Run(); },[this](auto section) {
        const auto result=ShowTellInConfig(client_.HtmlWindowGlobals()->tell,section,0,client_.Configuration(),[this](auto name)->std::optional<std::string> { return mission_ ? mission_->Section(name):std::nullopt; },[this](auto body,auto mode,auto width,auto height) { return rawButtons_->Menus()->ShowHtmlWindow(LoadingHtmlContext(CurrentMapView()),FontBytes(body),mode,width,height); });
        if (!result.found) client_.WriteDiagnosticLog(0x1d,"TELL cannot find \""+std::string(section)+"\".\n");rebuild_=true;
    },[this] { rawButtons_->Menus()->RefreshMetaNsInfo(*client_.NsInfoGlobals()); }); //버전Tell과67h수신의현재NSInfo도같은실제설정/HTML/Gump수명을공유한다.
    return editor_->metaTransport->Run(selected);
}
