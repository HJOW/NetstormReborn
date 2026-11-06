// 실제 일반 입력/Poll 경계에서만 저장 reader와 서버 선택을 실행한다.
void UberGump::RequestBattleReconnect(bool reloadFile) {
    if (world_ && world_->Session().ConnectionCallbackActive()) throw std::logic_error("재접속 부모는 연결 callback 밖에서 실행해야 함");auto& local=*client_.LocalStateGlobals();auto& subscribers=*client_.SubscriberGlobals();o::BattleReconnectHooks hooks;
    hooks.battleFinished=[this] { return world_ && world_->Session().BattleFinished(); };hooks.closeAll=[this] { client_.ConnectionGlobals()->CloseAll(); };hooks.readSaved=[this] { static_cast<void>(ReadClientSavedState(client_)); };
    hooks.takeServer=[this] { if (!world_) throw std::logic_error("현재15 서버 인수의 실제 World 없음");world_->Session().TakeServerForState(); };
    hooks.deleteSaved=[this] { static_cast<void>(DeleteSavedStateFile(client_.GameDirectory(),client_.StaticStrings()->FileNames(),client_.UserSavedStateFile())); };
    hooks.setHandle=[this](auto name) { client_.Configuration().Set("fort.mySubHandle",o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()})); }; // 설정만 UTF-8로 변환하고 Native 현재64BYTE는 표시 부호화를 유지한다.
    hooks.gamePort=[this] { std::int32_t port=6799;client_.Configuration().ReadInt("gameServerPort",port);return std::bit_cast<std::uint32_t>(port); };
    hooks.localSp=[this] { const auto& player=client_.GameplayGlobals()->player;return player.playerKnown ? std::bit_cast<float>(client_.SpGlobals()->Store().Get(player.battleOwner)) : 0.0F; };
    hooks.log=[this](auto text) { client_.WriteDiagnosticLog(0x1d,text); };hooks.checking=[this] { return client_.GameplayGlobals()->AiDebug().Checking()!=0; };hooks.assertion=[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,".\\State.cpp",line); };
    hooks.serialAlive=[this](auto serial) { return rawButtons_->SerialGump(serial)!=0; };
    hooks.tellNotRestart=[this] { //004cf960→cf730의 일반 Tell/현재이름·param·전체 HTML을 사용하며 내장4c8f60으로 대체하지 않는다.
        const auto result=ShowTellInConfig(client_.HtmlWindowGlobals()->tell,"NotRestart",0,client_.Configuration(),[this](auto section)->std::optional<std::string> { return mission_ ? mission_->Section(section) : std::nullopt; },[this](auto body,auto mode,auto width,auto height) { return rawButtons_->Menus()->ShowHtmlWindow(LoadingHtmlContext({mission_ ? 85 : 0,0,client_.GetScreen().Width(),client_.GetScreen().Height()}),FontBytes(body),mode,width,height); });
        if (!result.found) client_.WriteDiagnosticLog(0x1d,"TELL cannot find \"NotRestart\".\n");rebuild_=true;return result.serial;
    };
    const auto remote=o::ReconnectBattle(reloadFile,{local.State(),local.AbortPending(),local.ScriptBreak(),local.SourceType(),subscribers.MySubscriber(),local.RestoreOwner(),local.RestoreSubscriber(),local.PreviousHandle(),subscribers.Handle(),local.NoRestartSerial(),[this] { return std::span<const std::uint8_t>(client_.PlayerRecords()->Records()); }},hooks);
    if (remote) QueueReconnectBattleWorld(); // 원문이 이 호출에서 실제 작성한 요청만 다음 호스트 Tick에 인계한다.
}
// 기존 대기실 시작을 반납한 뒤에도 원문이 작성한 전체 다음 상태/Zone 값은 보존한다.
void UberGump::QueueReconnectBattleWorld() {
    if (!lan_) lan_=std::make_unique<LanRuntime>();auto& local=*client_.LocalStateGlobals();const auto next=local.State().next;const auto zonePending=local.ZonePending();CloseLanLobby();local.State().next=next;local.ZonePending()=zonePending;if (state_.Pending()) throw std::logic_error("다른 호스트 전환 대기 중 복구State7 인계");
    auto request=std::make_shared<GeneralBattleStartRequest>();request->reconnect=true;const auto& handle=client_.SubscriberGlobals()->Handle();const auto end=std::find(handle.begin(),handle.end(),0);if (end==handle.end()) throw std::logic_error("복구 현재 이름의 실제 NUL 누락");request->handle.assign(reinterpret_cast<const char*>(handle.data()),static_cast<std::size_t>(end-handle.begin()));
    LanLobbyBattle battle;battle.gamePort=next[31];lan_->battle=std::move(battle);lan_->start=std::move(request);state_.Post({"LoadLanBattleWorld",{}}); // 새 명단/이미지/주소/Zone 표식을 합성하지 않고 실제 saved 문자열을 State8이 읽는다.
}
// 영상 갱신 잠금 없는 Client 프레임에서 같은 수신표/현재 role을 읽으며 World 교체는 다음 Tick에 맡긴다.
void UberGump::PollConnectionHealth() {
    if (world_ && world_->Session().ConnectionCallbackActive()) throw std::logic_error("연결 감시의 callback 재진입");auto& activity=*client_.ConnectionActivityGlobals();o::ConnectionHealthHooks hooks;
    hooks.connectionExists=[this](auto id) { return client_.ConnectionGlobals()->Cleanup().transceivers[id]!=0; };hooks.wallSeconds=[this] { return client_.WallSeconds(); };hooks.frameWallSeconds=[this] { return client_.Time().wall; };hooks.frameNumber=[this] { return static_cast<std::uint32_t>(client_.Time().number); };
    hooks.sendHeartbeat=[this](auto broadcast) { if (!world_) throw std::logic_error("활동 송신의 실제 World 없음");world_->Session().SendConnectionHeartbeat(broadcast); };hooks.battleFinished=[this] { return world_ && world_->Session().BattleFinished(); };
    hooks.status=[this](auto kind,auto player) { warnings_->BattleStatus(static_cast<std::int32_t>(kind),static_cast<std::int32_t>(player),0);rebuild_=true; };hooks.saveServerFailure=[this] { if (!world_) throw std::logic_error("서버 장애 저장의 실제 World 없음");world_->Session().SaveServerFailure(); };
    if (o::PollConnectionHealth({client_.LocalStateGlobals()->State().phase,activity.HeartbeatEnd(),client_.GameplayGlobals()->Authority()->server,client_.GameplayGlobals()->player.battleOwner,activity.AutoRestart(),activity.Tables(),[this] { return std::span<const std::uint8_t>(client_.PlayerRecords()->Records()); }},hooks)) RequestBattleReconnect(false);
    if (world_) world_->Session().RunConnectionLoginTimer(); //439add의 빈4437f0 뒤439ae3의 같은 타이머 호출이다.
}
