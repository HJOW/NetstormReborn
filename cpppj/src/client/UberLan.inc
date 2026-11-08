// 같은 Uber/Client의 실제 Fort와 로컬 대기실을 일반 State7/게임 완료 관문에 연결한다.
struct UberGump::LanRuntime {
    std::unique_ptr<LanBattleLobby> lobby; // 현재 실제 대기실/읽기 Player 주소의 수명을 소유한다.
    std::optional<LanLobbyBattle> battle; // 실제 ACK 뒤 받은 역할/주소/명단이며 새 World 설치 중에도 보유한다.
    std::shared_ptr<GeneralBattleStartRequest> start; // 정리 전 지역값/이미지/사용자 이름을 같은 Session에 인계한다.
    std::uint32_t serial{}; // 자기 HTML 창만 갱신/반납하는 실제 생성 serial이다.
    std::string body; // 동일 명단으로 매 프레임 창을 새로 만들지 않는 호스트 표시 캐시다.
    bool choosing{}; // 주소 입력이 이 선택에서 열렸을 때만 같은 선택으로 돌아간다.
};
namespace { // 대기실의 표시 경계만 담당하는 지역 함수다.
// 현재 Config의 출력 인자 문자열 API를 사용하고 미설정/빈 주소만 loopback 기본값으로 표시한다.
std::string LanServerAddress(o::ConfigInterface& config) { std::string address="127.0.0.1";config.ReadString("lanServerAddress",address);return address.empty() ? "127.0.0.1" : address; }
// 사용자/원격 이름의 HTML 태그와 엔터티 문법을 본문 텍스트로만 표시한다.
std::string LanHtmlText(std::string_view text) {
    std::string result;for (const auto byte:text) { // Native 표시 부호화가 아닌 UTF-8 본문을 문자 바이트 그대로 보존한다.
        switch (byte) { case '&':result+="&amp;";break;case '<':result+="&lt;";break;case '>':result+="&gt;";break;case '"':result+="&quot;";break;default:result+=byte;break; }
    }return result;
}
}
// 자기 대기실 창/소켓만 닫으며 현재 Native 요청/게임 연결표는 건드리지 않는다.
void UberGump::CloseLanLobby() {
    if (!lan_) return;
    if (lan_->start && !lan_->start->consumed && client_.LocalStateGlobals()->State().current[0]!=7) { // 이미 실제 실행 중인 State7은 보존하고 아직 호스트 큐에서 기다리는 자기 시작만 취소한다.
        state_.Cancel("LoadLanBattleWorld");auto& local=*client_.LocalStateGlobals();const bool ownNext=local.State().next[0]==7 && (!lan_->start->storedZone || local.State().next[32]==lan_->start->zoneToken); //Native 수신이 덮어쓴 다른 next7을 자기 대기실 요청으로 취소하지 않는다.
        if (ownNext) { local.State().next[0]=0;local.ZonePending()=0; }if (lan_->start->storedZone) { local.ZonePackets().Release(lan_->start->zoneToken);if (ownNext) local.State().next[32]=0; }lan_->start.reset();lan_->battle.reset(); //자기 취소 요청의 실제 할당만 반납하며 다른 current/next의 token은 해제하지 않는다.
    }
    if (lan_->lobby) { lan_->lobby->Close();lan_->lobby.reset(); }if (lan_->serial) if (const auto token=rawButtons_->SerialGump(lan_->serial)) rawButtons_->KillGump(token);lan_->serial=0;lan_->body.clear();lan_->choosing=false;
}
// 주소/역할 선택도 기존 실제 HTML 공장과 원문 Connect 입력 큐를 사용한다.
void UberGump::OpenLanChoice() {
    if (!world_ || !editor_ || !client_.SurfaceStorage()->ownerMode.fort || client_.GameplayGlobals()->world.editor) throw std::logic_error("로컬 LAN 선택의 실제 일반 Fort 필요");if (!lan_) lan_=std::make_unique<LanRuntime>();CloseLanLobby();lan_->choosing=true;
    const auto address=LanServerAddress(client_.Configuration());
    const auto body="<h2>"+LanHtmlText(client_.Translate("Local TCP/IP Battle"))+"</h2><p>"+LanHtmlText(client_.Translate("Server"))+": "+LanHtmlText(address)+"\n$Button="+client_.Translate("Host Local Battle")+",Connect,2\n$Button="+client_.Translate("Join Local Battle")+",Connect,3\n$Button="+client_.Translate("Server Address")+",Connect,4\n$Button="+client_.Translate("Cancel")+",Connect,6\n";
    lan_->serial=rawButtons_->Menus()->ShowHtmlWindow(LoadingHtmlContext(CurrentMapView()),FontBytes(body),0,440,0);popup_.clear();page_={};state_.phase=ClientPhase::Dialog;pageName_="lan";RebuildControls();
}
// 옵션 행의 독립 주소 편집은 기존 옵션에 남고 LAN 선택의 주소 편집만 선택 화면을 재개한다.
void UberGump::ResumeLanChoiceAfterInput() { if (lan_ && lan_->choosing) OpenLanChoice(); }
// 명시 host/join을 열고 이미 열린 host의 Start 요청은 같은 대기실에서만 수행한다.
void UberGump::StartLanLobby(bool host,bool startOnly) {
    if (!world_ || !editor_ || !client_.SurfaceStorage()->ownerMode.fort || client_.GameplayGlobals()->world.editor) throw std::logic_error("로컬 LAN 대기실의 실제 일반 Fort 필요");if (!lan_) lan_=std::make_unique<LanRuntime>();
    if (lan_->lobby) { if (!host || !lan_->lobby->IsHost()) throw std::logic_error("이미 열린 다른 역할의 LAN 대기실");lan_->lobby->Start();ShowLanLobby();return; }if (startOnly) throw std::logic_error("LAN Start의 실제 열린 host 대기실 없음");
    CloseLanLobby();auto& session=world_->Session();const auto& gamePort=client_.GameplayGlobals()->GameServerPort();std::int32_t lobbyPort=6800;client_.Configuration().ReadInt("lanLobbyPort",lobbyPort); //게임 포트는 같은 장기 DWORD이며 별도 LAN 대기실 포트와 구별한다.
    auto lobby=std::make_unique<LanBattleLobby>(client_.PlayerRecords()->Records(),client_.PlayerRecords(),[this] { return client_.WallSeconds(); });
    try {
        if (host) lobby->Host(session.CurrentSubscriber(),session.CurrentHandle(),std::bit_cast<std::uint32_t>(lobbyPort),gamePort,ClientCurrentUnixSeconds()^session.CurrentSubscriber());
        else lobby->Join(session.CurrentSubscriber(),session.CurrentHandle(),LanServerAddress(client_.Configuration()),std::bit_cast<std::uint32_t>(lobbyPort),gamePort);
        lan_->lobby=std::move(lobby);ShowLanLobby();
    } catch (const std::exception& error) { CloseLanLobby();const auto body="<h2>LAN</h2><p>"+LanHtmlText(error.what())+"\n$Button="+client_.Translate("Back")+",Connect,1\n";lan_->serial=rawButtons_->Menus()->ShowHtmlWindow(LoadingHtmlContext(CurrentMapView()),FontBytes(body),0,440,0);popup_.clear();page_={};state_.phase=ClientPhase::Dialog;pageName_="lan";RebuildControls(); } // 실제 주소/포트/장치 실패를 창에 보여 주며 성공/연결 상태를 합성하지 않는다.
}
// 현재 명단이 바뀐 때만 자기 HTML을 같은 실제 부품으로 교체한다.
void UberGump::ShowLanLobby() {
    if (!lan_ || !lan_->lobby) return;auto& lobby=*lan_->lobby;std::string body="<h2>"+LanHtmlText(client_.Translate("Local TCP/IP Battle"))+"</h2>";
    for (const auto& participant:lobby.Participants()) { // 실제 현재 슬롯/이름/주소를 읽고 원격 문자열을 HTML로 실행하지 않는다.
        const auto name=o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(participant.name.data()),participant.name.size()});body+="<p>"+std::to_string(participant.slot+1)+". "+LanHtmlText(name)+" "+LanHtmlText(participant.address);
    }
    if (!lobby.Error().empty()) body+="<p>"+LanHtmlText(lobby.Error());
    else if (lobby.Starting()) body+="<p>"+LanHtmlText(client_.Translate("Waiting for all players..."));
    else if (lobby.IsHost()) body+="\n$Button="+client_.Translate("Start Battle")+",Connect,5\n";
    else body+="<p>"+LanHtmlText(client_.Translate("Waiting for host..."));
    body+="\n$Button="+client_.Translate("Cancel")+",Connect,6\n";if (body==lan_->body && rawButtons_->SerialGump(lan_->serial)) return;
    if (lan_->serial) if (const auto token=rawButtons_->SerialGump(lan_->serial)) rawButtons_->KillGump(token);lan_->body=body;lan_->serial=rawButtons_->Menus()->ShowHtmlWindow(LoadingHtmlContext(CurrentMapView()),FontBytes(body),0,440,0);popup_.clear();page_={};state_.phase=ClientPhase::Dialog;pageName_="lan";RebuildControls();
}
// 실제 제어 폴링이 끝난 뒤에만 다음 Tick에 일반 시작을 예약한다.
void UberGump::PollLanLobby() {
    if (!lan_ || !lan_->lobby) return;lan_->lobby->Poll();if (auto battle=lan_->lobby->TakeBattle()) { RequestLanBattle(std::move(*battle));return; }ShowLanLobby();
}
// 로컬 명단 합의는 원문과 같은 next7/주소/포트/소유 Zone 표식에 인계한다.
void UberGump::RequestLanBattle(LanLobbyBattle battle) {
    if (!lan_ || !world_ || mission_ || missionRuntime_ || state_.Pending()) throw std::logic_error("LAN 시작의 실제 일반 Fort/단일 호스트 요청 계약");
    auto request=std::make_shared<GeneralBattleStartRequest>();request->handle=world_->Session().CurrentHandle();auto& local=*client_.LocalStateGlobals();request->storedZone=battle.host;if (battle.host) request->zoneToken=local.ZonePackets().Adopt(std::move(battle.zone));else request->zone=std::move(battle.zone);auto next=local.State().next; //현재 일반 LAN 호스트의 같은 byte 할당을 장기 저장소에 인계하고 client/복구의 기존 입력 계약은 유지한다.
    auto bytes=std::span(reinterpret_cast<std::uint8_t*>(next.data()),sizeof(next));if (battle.host) { const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};std::copy(address.begin(),address.end(),bytes.begin()+0x54); }else std::copy(battle.serverAddress.begin(),battle.serverAddress.end(),bytes.begin()+0x54);
    next[0]=7;next[31]=battle.gamePort;next[32]=request->zoneToken;next[33]=0;next[34]=0;local.State().next=next;local.ZonePending()=battle.host ? local.ZonePending()+1U : 0U; //State+80에는 실제 가변 할당의 token을 쓰며 특정 상수1이나 호스트 포인터 절삭으로 대체하지 않는다.
    CloseLanLobby();lan_->battle=std::move(battle);lan_->start=std::move(request);state_.Post({"LoadLanBattleWorld",{}});
}
// World 반납보다 오래 사는 같은 Client의 공통 State7/즉시8 부모다.
void UberGump::LoadLanBattleWorld() {
    if (!lan_ || !lan_->battle || !lan_->start || lan_->start->consumed) throw std::logic_error("LAN State7의 실제 대기실 인계 누락");RunCampaignMissionStarting(client_,[this] { LoadLanBattleWorldBody(); },[this](auto address,auto port) { if (!world_) throw std::logic_error("LAN State8 현재 World 없음");world_->Session().ConnectLocalConnectionForState(address,port); });
}
// 원문 case7의 정리 전 캡처/정리/CD/명령/전투 모드 뒤 실제 일반 Session을 설치한다.
void UberGump::LoadLanBattleWorldBody() { LoadGeneralBattleWorldBody(lan_->start,{},lan_->battle->host); } //기존 대기실의 실제 역할/요청은 그대로 인계한다.
// 실제 현재7의 정리 전 캡처/정리/CD/명령/전투 prefix 뒤 같은 일반 Session을 설치한다.
void UberGump::LoadGeneralBattleWorldBody(const std::shared_ptr<GeneralBattleStartRequest>& request,NativeBattleSource source,std::optional<bool> lanHost) {
    auto network=source.network;const auto meta=source.meta;auto& local=*client_.LocalStateGlobals();auto& surface=*client_.SurfaceStorage();if (local.State().current[0]!=7 || request->starting || (!request->nativeState && !meta && (mission_ || missionRuntime_))) throw std::logic_error("LAN State7 앞단의 실제 현재 요청/비미션 계약");
    state_.phase=ClientPhase::LoadingMission;pageName_="loading";client_.GameplayGlobals()->Camera().SelectDenominator(0,client_.KencloudGlobals()->WorldRedraw());request->starting=o::CaptureLocalMissionStarting(local.State().current,local.ZonePending(),reinterpret_cast<std::uint8_t&>(htmlGlobals_.background[0]),local.GameTicks(),local.SourceType());
    if (request->nativeState) { request->storedZone=request->starting->zonePending!=0;request->zoneToken=request->storedZone ? local.State().current[32]:0U; } //같은 캡처값으로1a의소유Zone과1b의실제Zone부재를구별하며주소/port는변경하지않는다.
    CleanupNativeStateWorld(1,meta);local.CdPresent()=client_.CdGlobals()->Check(MakeWindowsCdPresenceHooks(&client_.StaticStrings()->FileNames()));if (commandGump_) static_cast<void>(rawButtons_->BubbleGump(commandGump_,0x02000088U,0,0));o::RestoreLocalMissionStarting(local.State().current,*request->starting,local.GameTicks(),local.SourceType());surface.ownerMode.battle=true;surface.links.battle=true;if (client_.Audio()) client_.Audio()->StartScene(true);
    if (world_) throw std::logic_error("LAN 정리 뒤 이전 World 대여 잔류");if (editor_->transport) { if (editor_->transport!=network.editor) editor_->transport->CloseListener();editor_->transport.reset(); }static_cast<void>(EnsureEditorFortFiles());
    if (request->nativeState) { const auto& handle=client_.SubscriberGlobals()->Handle();const auto end=std::find(handle.begin(),handle.end(),0);if (end==handle.end()) throw std::out_of_range("메타 State7 현재 이름의 NUL 누락");request->handle.assign(reinterpret_cast<const char*>(handle.data()),static_cast<std::size_t>(end-handle.begin())); } //정리 뒤 같은 현재 이름만 읽으며 미션/대기실 이름을 합성하지 않는다.
    auto services=MakeCampaignClientServices(client_,*rawButtons_,{[this] { RefreshWindowTitle(); },[this](auto name) { Tell(std::string(name)); },[this](auto kind,auto player,auto parameter) { warnings_->Remove(kind,player,parameter);rebuild_=true; },[this](auto packet) { ReceiveChatMessage(packet); }});
    services.installMode=CampaignInstallMode::GeneralBattle;services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.generalBattleStart=request;services.templateStorage=templateStorage_;services.missionOptions=missionOptions_;services.missionRestrictions=missionRestrictions_;services.fortBadges=client_.FortBadges();services.fortTypeConversion=client_.FortTypeConversion();services.fortMission=FortMissionSaveEffects();
    services.missionOutcome.demo=[this] { return missionRuntime_ ? missionRuntime_->gate.demo : 0U; }; // 실제 Guide가 없는 일반 전투는 원문 GetDemo의 자연0이며 간헐천 장치도 같은 getter를 사용한다.
    services.missionFortFileSelect=[this](auto) { const auto path=std::string(EnsureEditorFortFiles().PrepareCurrent());return MissionFortFileSelection{path,[this,path] { return editor_->paths->Read(path); }}; };TemplateFileLoadHooks load;load.wrongName=[] { std::clog<<"LAN Fort 파일명 불일치\n"; };load.tooManyTypes=[] { std::clog<<"LAN Fort TYPE 수 진단\n"; };load.rejectFile=[this](auto path) { editor_->files.Delete(path); };services.missionFortLoad=editor_->paths->BindLoad(std::move(load));
    services.remoteFortFiles=std::shared_ptr<EditorFiles>(editorCatalog_,&editorCatalog_->Files());services.remoteFortPath=[this](auto owner,auto localOwner) { const auto configured=SelectRemoteFortPath(*client_.FortGlobals(),client_.Configuration(),owner,localOwner,[](auto expression) { std::clog<<expression<<'\n'; });return platform::PathToUtf8(editorCatalog_->Resolve(configured)); };
    const auto current=std::string(client_.FortGlobals()->Current());world_=std::make_shared<GameWorld>(client_.Assets(),client_.Files(),o::FortTemplate{},o::MissionPlayers{},current,std::move(services),ScreenPoint{client_.GetScreen().Width(),client_.GetScreen().Height()},true);world_->elapsed=[this] { return client_.Time().delta; };world_->paused=[this] { return client_.Paused(); };world_->frameTime=[this] { return client_.Time(); };world_->BindHideBuildings([this] { return hideBuildings_; });world_->soundViewChanged=[this](auto view) { client_.WorldAudio().SetView(view); };world_->surfaceDisplay={[this] { return client_.GetRenderer().FullRedrawPending(); },[this](auto rect,auto flags) { client_.GetRenderer().Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }};
    world_->Session().BindConnectionTransport(MakeClientCampaignTransport(client_,lanHost.value_or(request->starting->zonePending!=0) ? CompositeTransportRole::LanHost : CompositeTransportRole::LanClient,world_,network,meta),world_);world_->Session().BindFortFileSave(client_.ReadOnlyForts(),editor_->files);world_->PrepareLocalConnectionForState(); // State7 공통 확정/즉시8은 외부 부모로 반환한다.
}
// 실제 일반 Login 완료 뒤 같은 커서/입력/HUD 표시만 공개한다. 별도 Ready/성공 플래그는 없다.
void UberGump::FinishLanBattleWorld() {
    if (!world_ || missionRuntime_ || !world_->Session().LocalConnectionReady()) throw std::logic_error("LAN 실제 phase14/Player 참여 완료 계약");auto& session=world_->Session();auto effects=MakeCampaignCursorClientEffects(client_,*rawButtons_);BindCampaignUserInputServices(client_,*world_,effects.lifecycle.selectCursor,[this](auto section) { Tell(std::string(section)); });effects.send=[&session](auto target,auto packet) { session.SendLocalPacket(std::bit_cast<std::int32_t>(target),packet); };effects.lifecycle.highlight=[this](auto mode) { world_->Highlight(mode); };effects.drop.refresh=[this](auto sid,auto force,auto move) { if (force || move!=1) throw std::logic_error("LAN 커서 선택 인자 계약");world_->Select(sid); };session.Cursor().SetEffects(std::move(effects));session.Cursor().Display().SetExternal([this](auto token) { return rawButtons_->GumpDepth(token); },{});
    userInput_=std::make_unique<UserInput>(client_,*world_);world_->Resize(client_.GetScreen().Width(),client_.GetScreen().Height());world_->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical(),&client_.ColorSortInput()->Maps());state_.phase=ClientPhase::Mission;pageName_="battle";page_={};popup_.clear();RebuildControls();
}
