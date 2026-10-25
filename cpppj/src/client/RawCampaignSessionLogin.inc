// 동일 Session 내부 저장소를 기존 원본 로그인/패킷/상태/타이머 몸체에 연결한다.
// RawCampaignSession.cpp의 Impl 정의 뒤, 같은 namespace 안에서 포함한다.
struct RawCampaignSession::Impl::Login {
    Impl& s; //유일한 Player/Template/SP/월드/현재 모드 소유자다.
    CampaignLoginServices effects; //현재 실제 Client 설정/시계/표시/로그 장치다.
    std::function<void()> install; //428570 등록을 포함한 같은 Session.Load의 실제 소비자다.
    std::unique_ptr<netstorm::zacket::LocalMissionTransport> transport; //이 로그인과 현재 명령이 공유하는 실제 두끝/송수신 큐다.
    std::unique_ptr<RawMissionLogin> server; //실제0x27의서버 처리기다.
    std::unique_ptr<RawMissionClientPackets> client; //같은Player/SP/상태에8종응답을 적용한다.
    std::unique_ptr<RawMissionServerPackets> metadata; //실제Player/metadata 패킷 writer다.
    std::unique_ptr<RawMissionLoginPackets> forts; //같은현재Template의 실제image/2a writer다.
    std::unique_ptr<RawMissionBattleOptions> options; //같은87BYTE/188TYPE/에너지범위/사용표를 직접 쓴다.
    std::unique_ptr<RawMissionSettingsPacket> settings; //현재Client 설정7DWORD를 송수신한다.
    std::unique_ptr<RawLocalMissionState> state; //상태8→9→10→13→14와전체0x90BYTE 확정이다.
    std::unique_ptr<RawMissionLoginTimer> timer; //실제Ready 집계와사제참여 재조회/시작종료 타이머다.
    std::array<LocalMissionStateBlock,5>& blocks{s.localStateGlobals->Blocks()}; // Editor/State/로그인이 같은720BYTE를 직접 빌리며 독립 Session도 유일한 owner를 쓴다.
    std::array<std::uint32_t,2> zoneWords{}; //원본6BYTE Zone 주소의 실제DWORD와인접WORD를 공유한다.
    std::span<std::uint8_t> cachedCore{s.coreDataGlobals->Cached()};std::span<std::uint8_t>& currentCore{s.coreDataGlobals->Current()}; // 실제 같은 장기10BYTE와 현재 포인터 슬롯을 직접 빌린다.
    std::array<std::uint8_t,256> typeUse{}; //원본256BYTE 타입사용표다.
    std::array<float,4> ranges{14,22,30,38}; //원본5425f4의 실제4float다. 기본3번도 읽고 최대 비교는 원래2번30을 사용한다.
    float range{22}; //원본52f488이며제곱은Session의같은EnergySupplyState에 쓴다.
    bool simul{},adjust{};bool& saved{s.localStateGlobals->SavedGame()};std::uint8_t overrideFlags{},title{},battleByte{}; // 저장0/1은 실제 State owner이며 나머지는 기존 로그인 입력이다.
    std::uint32_t pendingStamp{},activeStamp{},zoneWord{},predictable{},beginSeed{},battleSeed{},burst{},clockStamp{},baseClock{},archipelago{},optionValue{},outpost{}; // 아직 별도로 소유하는 원본 로그인 전역이다.
    std::uint32_t& begin{s.localStateGlobals->Begin()};std::uint32_t& central{s.localStateGlobals->CentralCreated()}; // 실제 Login/Ready/metadata도 Client 수명의 같은 DWORD를 읽고 쓴다.
    std::uint32_t& coreActive{s.coreDataGlobals->Active()}; // Chat/현재 Core getter/패킷이 같은00595348을 직접 사용한다.
    std::uint32_t& fortCount{s.playerRecords->FortCount()}; // 원문 Player 초기화와 같은005951f8이다.
    std::uint32_t& lastPlayer{s.playerRecords->LastLoginPlayer()}; // 같은005951fc를 로그인도 직접 쓴다.
    std::uint32_t& zonePending{s.localStateGlobals->ZonePending()};std::uint32_t& source{s.localStateGlobals->SourceType()}; // Fort 요청/State/Reply가 같은 실제 두 DWORD를 공유한다.
    std::uint32_t ticks{},randomSeed{},metaRefresh{},metaRebuild{},scriptBreak{},requestPhase{},uiMode{},inputMode{}; // 아직 로그인 안에 남은 현재 DWORD 저장소다.
    double& captureEnd{s.localStateGlobals->CaptureEnd()};double& connectEnd{s.localStateGlobals->ConnectEnd()}; // 서로 다른 원본 벽시각 기한을 직접 빌린다.
    double wall{};std::int32_t timeout{}; // 로그인 장치의 현재 벽시각 입력과 설정 값이다.
    double& activityEnd{s.localStateGlobals->ActivityEnd()}; // Editor가 쓴 같은00544d58도 Session 전환에서 보존한다.
    std::array<std::uint32_t,7> settingValues{1,0,0,0,0,1,8}; //실제초기7DWORD이며현재패널/디버그전역을서로별개로보존한다.
    std::vector<std::uint8_t> wireTypes; //실제수신한패킷종류의 진단기록이며 분기를 대신하지 않는다.
    //선택한 실제 장치가 빠지면 그 지점에서 명시 오류를 보인다.
    template<class F> static const F& Need(const F& hook) { if (!hook) throw std::logic_error("실제 미션 로그인 장치 미연결");return hook; }
    //실제6BYTE 주소를 같은DWORD 배열에서 빌린다.
    std::span<std::uint8_t> Zone() { return {reinterpret_cast<std::uint8_t*>(zoneWords.data()),6}; }
    //비정렬원문의DWORD/WORD만 읽는다.
    static std::uint32_t Get(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { std::uint32_t value{};
        //원본의낮은바이트부터조합한다.
        for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[at+i])<<(8*i);return value;
    }
    //원본State의현재WORD/DWORD만 쓴다.
    static void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
        //낮은지정폭의바이트만바꾸며인접필드는보존한다.
        for (std::size_t i=0;i<width;++i) bytes[at+i]=static_cast<std::uint8_t>(value>>(8*i));
    }
    //현재 파일을 실제 VFS로 다시 읽어 같은Template/State/territory cursor를 교체한다.
    void Reload(std::uint32_t owner) {
        const auto name=s.templateStorage->Name(owner);if (owner!=1 || name.empty()) throw std::logic_error("로컬 미션 이외 Template 파일 선택 미연결");
        const auto bytes=s.assets.files.Read("d/"+name+".fort");s.templateStorage->InstallFile(owner,FortTemplate::Parse(bytes,s.assets.game.TypeTable()),bytes,name);
    }
    //현재20BYTE zlocal주소에서 실제클라이언트의포트와주소를 제공한다.
    std::array<std::uint8_t,20> Address() const {
        std::array<std::uint8_t,20> address{};Put(address,0,20,2);const auto port=Need(effects.configInt)("gameServerPort",6799);Put(address,2,((std::uint32_t(port)&255U)<<8)|(std::uint32_t(port)>>8),2);Put(address,16,0x10d);return address;
    }
    //현재같은실제연결gate로만송신하며수신기를동기호출하지않는다.
    void Send(std::int32_t target,std::span<const std::uint8_t> bytes) { if (effects.observePacket && bytes[1]==0x6e) Need(effects.log)("Ready 송신 target"+std::to_string(target)+" mask"+std::to_string(Get(bytes,4))+"\n");static_cast<void>(transport->Send(target,bytes)); }
    //현재 시각/수신 기록과 실제고정연결 수명을 분배기에 공급한다.
    PacketDispatchHooks Device() {
        PacketDispatchHooks hooks;hooks.now=[this] { return static_cast<std::uint32_t>(Need(effects.wallSeconds)()); };
        hooks.record=[this](PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t) { wireTypes.push_back(packet[1]);if (effects.observePacket) effects.observePacket(connection.sender,s.assetState.boss,s.assetState.localOwner,packet); };
        hooks.terminate=[this](PacketConnection& connection) { connection.flags|=0x40;Need(effects.log)("로컬 미션 연결 종료\n"); };
        hooks.close=hooks.release=[](PacketConnection&) { throw std::logic_error("로그인 연결 해제/반납의 실제 수명 처리 미연결"); };return hooks;
    }
    //같은상태9/응답13이사용하는실제시계/연결/Template/UI/타이머 경계다.
    LocalMissionStateHooks StateHooks() {
        LocalMissionStateHooks hooks;hooks.logState=[this](std::uint32_t phase) { Need(effects.log)("로컬 미션 상태 "+std::to_string(phase)+"\n"); };
        hooks.wallSeconds=effects.wallSeconds;
        hooks.connectServer=[this](std::span<const std::uint8_t>,std::uint32_t) { transport->Connect(); };
        hooks.connectionStatus=[this](std::int32_t owner) { return owner==9 && transport->IsOpen() && (transport->Client().flags&2) ? 1 : 0; };
        hooks.connectionOpen=[this](std::int32_t owner) { return owner==9 && transport->IsOpen() && (transport->Client().flags&2) && !(transport->Client().flags&0x40); };
        hooks.sendLogin=[this] { const auto image=RawFortImage(OriginalEdition::Patch1078,s.templateStorage->Template(s.playerState.templateOwner),{[this] { return static_cast<std::uint32_t>(s.core.types.size()); },{}}).Save();const auto packet=netstorm::zacket::MakeLocalMissionLogin(s.globals.mySubscriber,{reinterpret_cast<const char*>(s.handle.data())},image.Bytes());Send(9,packet.Bytes()); };
        hooks.connectedProgress=hooks.closeProgress=effects.refresh;
        hooks.fortAvailable=[this](std::uint32_t owner) { return owner<9 && Get(s.headers,owner*20)!=0; };hooks.loadCurrentFort=[this] { Reload(s.playerState.templateOwner); };
        hooks.fortCore=[this](std::uint32_t owner) { return s.templateState->Get(owner); };hooks.touchFort=[this](std::uint32_t owner) { s.templateState->Changed(owner); };
        hooks.localAddress=[this] { return Address(); };hooks.uiMode=[this](std::uint32_t mode) { uiMode=mode;Need(effects.refresh)(); };
        hooks.refreshFort=s.services.install.refreshWindowTitle;hooks.loginTimer=[this](bool start) { timer->Run(start); };hooks.inputMode=[this](bool mode) { inputMode=mode;Need(effects.refresh)(); };
        return hooks;
    }
    //현재 원본 로그인/패킷/상태/타이머 모듈을 같은 저장소로 조립한다.
    Login(Impl& owner,CampaignLoginServices device,std::function<void()> load):s(owner),effects(std::move(device)),install(std::move(load)) {
        // 첫 실제 접속/설치 전에 Init의22개 기본 옵션/색과 예약 영역 보존을 같은87BYTE에 적용한다.
        if (!s.services.missionOptions) ResetMissionBattleOptions(OriginalEdition::Patch1078,s.battleOptions); // 실제 Client Init이 쓴 전역을 다시 지우지 않는다.
        transport=std::make_unique<netstorm::zacket::LocalMissionTransport>(s.players,s.core.processState.server,Device());
        auto send=[this](std::int32_t target,std::span<const std::uint8_t> bytes) { Send(target,bytes); }; //모든 부모가 공유하는 같은 실제 연결이다.
        options=std::make_unique<RawMissionBattleOptions>(OriginalEdition::Patch1078,MissionBattleOptionState{s.battleOptions,s.world.MutableTypes(),typeUse,ranges,s.geyserType,range,s.energyState.radiusSquared,optionValue});
        MissionServerPacketHooks packets;packets.send=send;packets.peerAddress=[this](std::uint32_t) { return "*:"+std::to_string(Need(effects.configInt)("gameServerPort",6799)); };packets.log=effects.log;
        metadata=std::make_unique<RawMissionServerPackets>(s.players,MissionServerPacketState{s.core.processState.server,s.core.ownerMode.challenge,s.playerState.battleOwner,central,archipelago,Zone()},std::move(packets));
        forts=std::make_unique<RawMissionLoginPackets>(OriginalEdition::Patch1078,s.players,MissionFortPacketReads{[this](std::uint32_t player)->const FortTemplate* { return player<9 && Get(s.headers,player*20) ? &s.templateStorage->Template(player) : nullptr; },[] { return std::uint16_t{4}; }},FortImageHooks{[this] { return static_cast<std::uint32_t>(s.core.types.size()); },{}},MissionFortPacketHooks{[this](std::uint32_t,std::int32_t) { Need(effects.log)("로컬 Fort 전송\n"); },send,{}});
        MissionSettingsPacketHooks setting;setting.current=[this](MissionSettingsField field) { return settingValues.at(static_cast<std::size_t>(field)); };setting.setCurrent=[this](MissionSettingsField field,std::uint32_t value) { settingValues.at(static_cast<std::size_t>(field))=value;if (field==MissionSettingsField::ShootMyFriends) s.gunTargetState.friendlyFire=value!=0; };setting.send=send;settings=std::make_unique<RawMissionSettingsPacket>(OriginalEdition::Patch1078,std::move(setting));
        state=std::make_unique<RawLocalMissionState>(OriginalEdition::Patch1078,LocalMissionStateStorage{blocks[0],blocks[1],blocks[2],blocks[3],blocks[4]},LocalMissionStateGlobals{s.core.processState.server,s.assetState.boss,s.assetState.inBattle,s.globals.missionPost,saved,s.objectState.mission,simul,s.core.ownerMode.challenge,s.playerState,zonePending,ticks,source,randomSeed,blocks[2][33],metaRefresh,metaRebuild,title,battleByte,connectEnd,activityEnd,s.core.processState.now,wall},StateHooks());
        MissionLoginHooks serverHooks;serverHooks.send=send;serverHooks.broadcast=[this](std::span<const std::uint8_t> bytes) { s.broadcast->Send(bytes); };serverHooks.flush=[this] { transport->Flush(); };serverHooks.wallSeconds=effects.wallSeconds;serverHooks.clock=effects.clock;serverHooks.spBits=[this](std::uint32_t player) { return s.sp.Get(player); };
        serverHooks.applyOptions=[this](std::span<const std::uint8_t> bytes) { client->Receive(0,transport->Server(),bytes); };serverHooks.clearTypes=[this] { options->ClearTypes(); };
        serverHooks.sendPlayer=[this](std::uint32_t player,std::uint32_t target) { metadata->SendPlayer(player,static_cast<std::int32_t>(target)); };serverHooks.sendFortMetadata=[this](std::uint32_t target) { metadata->SendMetadata(static_cast<std::int32_t>(target)); };
        serverHooks.loadCurrentFort=[this](std::uint32_t player) { Reload(player); };serverHooks.sendFort=[this](std::uint32_t player,std::uint32_t target,std::uint32_t count) { static_cast<void>(forts->SendFort(player,static_cast<std::int32_t>(target),count)); };serverHooks.loadWorld=install;serverHooks.sendSettings=[this](std::uint32_t target) { settings->Send(static_cast<std::int32_t>(target)); };
        server=std::make_unique<RawMissionLogin>(s.players,*s.participation,MissionLoginGlobals{s.core.processState.server,s.assetState.boss,simul,s.objectState.mission,s.globals.missionPost,s.core.ownerMode.challenge,s.playerState,s.globals.mySubscriber,fortCount,lastPlayer,predictable,begin,beginSeed,battleSeed,burst,clockStamp,baseClock,captureEnd,currentCore,cachedCore,Zone(),s.battleOptions},std::move(serverHooks));
        MissionClientPacketHooks receive;receive.clock=effects.clock;receive.wallSeconds=effects.wallSeconds;receive.runState=[this] { state->Run(); };receive.resetCamera=s.services.install.resetCamera;receive.refreshSp=effects.refresh;
        receive.updateRange=[this] { options->UpdateRange(); };receive.requiredEnergy=[this] { return options->TypeCost(); };receive.setEnergyType=[this](float value) { options->SetSelectedTypeCost(value); };receive.readOptionValue=[this] { return options->CurrentValue(); };
        receive.addChat=s.services.ai.participation.addChat;receive.playerUiBusy=effects.playerUiBusy;receive.playerNotice=effects.playerNotice;receive.send=send;receive.sendReady=[this](std::uint32_t target) { server->SendReady(target); };
        client=std::make_unique<RawMissionClientPackets>(s.players,*s.participation,s.sp,MissionClientPacketState{s.core.processState.server,s.assetState.boss,s.globals.missionPost,saved,s.core.ownerMode.challenge,s.assetState.inBattle,s.globals.editor,outpost,s.playerState,beginSeed,battleSeed,s.revision,optionValue,begin,clockStamp,baseClock,blocks[1][0],blocks[3][0],blocks[3][14],blocks[4][0],source,coreActive,captureEnd,s.loginEnd,currentCore,cachedCore,Zone(),s.battleOptions,s.handle,s.hiddenPlayers},std::move(receive));
        MissionLoginTimerHooks timing;timing.configInt=[this](std::string_view key) { return Need(effects.configInt)(key,30); };timing.wallSeconds=effects.wallSeconds;timing.clock=effects.clock;timing.broadcast=[this](std::span<const std::uint8_t> bytes) { if (effects.observePacket) Need(effects.log)("Timer broadcast local"+std::to_string(s.assetState.localOwner)+"\n");s.broadcast->Send(bytes); };timing.priestRefresh=[this](std::uint32_t player) { s.priestParticipation->Refresh(player); };
        timing.saveReconnect=[this] { if (!s.globals.missionPost) throw std::logic_error("일반전투 재접속 저장 미연결"); };timing.closeProgress=effects.refresh;timing.log=[this](MissionLoginTimerLog,std::string_view text) { Need(effects.log)(text); };
        timer=std::make_unique<RawMissionLoginTimer>(OriginalEdition::Patch1078,MissionLoginTimerState{s.loginEnd,timeout,begin,scriptBreak,requestPhase,baseClock},counts,MissionLoginTimerReads{[this] { return saved; },[this] { return s.core.processState.server; },[this] { return s.objectState.mission; },[this] { return s.globals.missionPost; },[this] { return s.assetState.boss; },[this] { return s.gunState.checking; }},std::move(timing));
        RegisterMissionLogin(transport->Dispatch(),*server);RegisterMissionClientPackets(transport->Dispatch(),*client);RegisterMissionMetadata(transport->Dispatch(),*metadata);RegisterMissionSettingsPacket(transport->Dispatch(),*settings);
        // 원본3d 등록의 observer는0040b380의 빈 반환이고 version1/mismatch0이다.
        transport->Dispatch().Observe(0x3d,[](std::uint32_t) {});transport->Dispatch().Version(0x3d,1,0);
        transport->Dispatch().Register(0x3d,[this](std::uint32_t,PacketConnection&,std::span<const std::uint8_t> bytes) {
            // 이 분배기는 LocalMissionTransport의 mutable vector만 수신한다. 같은 실제 버퍼에 원본 self bit를 쓴다.
            s.participation->ReceiveAlliance({const_cast<std::uint8_t*>(bytes.data()),bytes.size()},[this](std::span<const std::uint8_t> packet) { s.broadcast->Send(packet); });
        });
    }
    MissionLoginPlayerCounts counts{OriginalEdition::Patch1078,s.players}; //같은 Player9의 현재 집계를 타이머에 공급한다.
    //원본 미션 부모의 로컬 Zone을 설치하고 실제 상태8에서 접속한다.
    void Begin() {
        if (!s.services.subscriberGlobals) { s.globals.loginSubscriber=s.services.missionSubscriber.value_or(1);s.globals.mySubscriber=s.globals.loginSubscriber; } //독립fallback만기존복사를수행한다.실제Client는Init현재쓰기뒤Login에서ID를재복사하지않는다.
        s.connectionAssign=[this](std::int32_t pending,std::int32_t player) { transport->AssignConnection(pending,player);s.services.battlePeers.at(static_cast<std::size_t>(player))=transport->Peer(static_cast<std::uint32_t>(player)); };
        s.connectionAlliance=[this](std::uint32_t target,std::uint32_t owner,std::uint32_t mask) {
            std::array<std::uint8_t,9> packet{1,0x3d,9,0}; // 원본0040a780/0052ede2의 version1/type3d/length9다.
            packet[4]=static_cast<std::uint8_t>(owner);Put(packet,5,mask);Send(static_cast<std::int32_t>(target),packet);
        };
        s.services.ai.localTransport=[this] { return transport->IsOpen(); };s.services.carrierRequest=[this](std::int32_t target,std::span<const std::uint8_t> bytes) { Send(target,bytes); };
        auto request=s.services.missionFortRequest;std::optional<netstorm::zacket::LocalMissionPacket> fallback; // Init에서 작성한 같은86BYTE를 사용하며 독립 Session만 기존 작성기를 호출한다.
        if (request) { if (request->consumed || request->next[0]!=7 || request->next[0x80/4]!=1 || request->zone.size()!=86) throw std::logic_error("현재 Init Fort 요청/이미지 계약 위반");blocks[3]=request->next; }
        else fallback=netstorm::zacket::MakeLocalMissionZone();
        const auto zone=request ? std::span<const std::uint8_t>(request->zone) : fallback->Bytes();netstorm::zacket::LocalMissionZoneHooks hooks;hooks.loginTimeout=[this] { return Need(effects.configInt)("loginTimeout",30); };hooks.seconds=effects.wallSeconds;
        hooks.storeName=[](std::uint32_t subscriber,std::string_view name) { SanitizeMissionPlayerName(subscriber,{reinterpret_cast<std::uint8_t*>(const_cast<char*>(name.data())),name.size()+1}); };
        hooks.serve=[this](std::uint32_t port) { transport->Open(port,false); };
        netstorm::zacket::InstallLocalMissionZone(zone,s.players,s.playerState,{s.core.processState.server,s.assetState.boss,s.globals.mySubscriber,pendingStamp,activeStamp,zoneWord,zoneWords[0],captureEnd,adjust,s.objectState.mission,overrideFlags,s.core.ownerMode.challenge,currentCore,cachedCore},std::move(hooks),static_cast<std::uint32_t>(Need(effects.configInt)("gameServerPort",6799)));
        if (request) { std::vector<std::uint8_t>{}.swap(request->zone);request->consumed=true;blocks[3][0x80/4]=0; } // 설치 뒤 실제 할당도 반납한다. 전체 State7 정리 본문을 실행했다고 합산하지 않는다.
        s.playerState.playerKnown=0;blocks[3][0]=8;state->Run();state->Poll();Pump();
        if (!s.loaded || blocks[1][0]!=14 || !(s.players[s.playerState.battleOwner*0xb4+0x50]&1)) throw std::logic_error("실제 로컬 미션 로그인 미완료");
    }
    //현재 송신을flush한 뒤 실제 큐를 분배한다. 프레임 계산은 기존Kernel이 수행한다.
    void Pump() { wall=Need(effects.wallSeconds)();transport->Flush();static_cast<void>(transport->Poll());if (s.loginEnd!=0.0) timer->Run(false); }
};
