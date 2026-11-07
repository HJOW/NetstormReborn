// 동일 Session 내부 저장소를 기존 원본 로그인/패킷/상태/타이머 몸체에 연결한다.
// RawCampaignSession.cpp의 Impl 정의 뒤, 같은 namespace 안에서 포함한다.
// 최초 파일 선택/지역 송신 이미지/전역 보존 근거: docs/exe/cpp-login-template-lifetime-20261101.md.
struct RawCampaignSession::Impl::Login {
    Impl& s; //유일한 Player/Template/SP/월드/현재 모드 소유자다.
    CampaignLoginServices effects; //현재 실제 Client 설정/시계/표시/로그 장치다.
    std::function<void()> install; //428570 등록을 포함한 같은 Session.Load의 실제 소비자다.
    std::shared_ptr<netstorm::zacket::LocalMissionTransport> internalTransport; // 기존 단독 미션만 현재 내부 양끝을 소유한다.
    CampaignTransportServices transport; // 명시 LAN/내부 backend의 같은 현재 ID/Registry/분배기만 빌린다.
    std::unique_ptr<RawMissionLogin> server; //실제0x27의서버 처리기다.
    std::unique_ptr<RawMissionClientPackets> client; //같은Player/SP/상태에8종응답을 적용한다.
    std::unique_ptr<RawMissionServerPackets> metadata; //실제Player/metadata 패킷 writer다.
    std::unique_ptr<RawMissionLoginPackets> forts; //같은현재Template의 실제image/2a writer다.
    std::unique_ptr<RawMissionBattleOptions> options; //같은87BYTE/188TYPE/에너지범위/사용표를 직접 쓴다.
    std::unique_ptr<RawMissionSettingsPacket> settings; //현재Client 설정7DWORD를 송수신한다.
    std::unique_ptr<RawLocalMissionState> state; //상태8→9→10→13→14와전체0x90BYTE 확정이다.
    std::unique_ptr<RawMissionLoginTimer> timer; //실제Ready 집계와사제참여 재조회/시작종료 타이머다.
    std::unique_ptr<RawClockSynchronization> clockSynchronization; // 같은 Client70표본·연결 token·게임 보정과 실제5번 수신기를 빌린다.
    std::array<LocalMissionStateBlock,5>& blocks{s.localStateGlobals->Blocks()}; // Editor/State/로그인이 같은720BYTE를 직접 빌리며 독립 Session도 유일한 owner를 쓴다.
    std::array<std::uint32_t,2>& zoneWords{s.localStateGlobals->ZoneWords()}; //원본 Zone6BYTE와 같은 장기 Client 물리를 실제 로그인도 빌린다.
    std::span<std::uint8_t> cachedCore{s.coreDataGlobals->Cached()};std::span<std::uint8_t>& currentCore{s.coreDataGlobals->Current()}; // 실제 같은 장기10BYTE와 현재 포인터 슬롯을 직접 빌린다.
    std::array<std::uint8_t,256> typeUse{}; //원본256BYTE 타입사용표다.
    std::span<const float,4> ranges{s.gameplayGlobals->ManaRanges()}; //원본5425f4의 현재4float다. 기본3번도 읽고 최대 비교는 원래2번30을 사용한다.
    float& range{s.gameplayGlobals->ManaRadius()}; //원본52f488을 같은 커서/AI/메타 옵션과 공유하며 제곱도 실제EnergySupplyState에 쓴다.
    bool& simul{s.localStateGlobals->Simulconnect()};std::uint32_t& adjust{s.gameplayGlobals->GameAux()->TerrainEnabled()};bool& saved{s.localStateGlobals->SavedGame()};std::uint8_t overrideFlags{},localTitle{};std::uint8_t& battleByte{s.localStateGlobals->BattleByte()}; //State의 현재 입력과 생성/Zone 보정은 같은 Client 수명이며 독립 검사만 별도 배경 첫BYTE를 사용한다.
    std::uint8_t& title{s.services.htmlWindowGlobals ? reinterpret_cast<std::uint8_t&>(s.services.htmlWindowGlobals->text.background[0]) : localTitle}; //현재 HTML 배경80BYTE의 첫BYTE만 같은 주소로 빌린다.
    std::uint32_t& activeStamp{s.connectionActivityGlobals->ActiveStamp()};std::uint32_t& zoneWord{s.connectionActivityGlobals->ActiveZoneWord()}; // 두 활동 표 사이005c8540/544의 실제 현재 Zone 출력을 직접 빌린다.
    std::uint32_t pendingStamp{},beginSeed{},battleSeed{},burst{},localOutpost{}; // 독립 Login만 실제 Client 없는 CS0 입력을 자체 보관한다.
    std::uint32_t& outpost{s.services.challengeGlobals ? s.services.challengeGlobals->Current() : localOutpost}; //005467d8의 현재 객체 존재를 메타/일반 Session이 같은 주소에서 읽는다.
    std::uint32_t& predictable{s.core.pool.PredictableCursorValue()}; // 실제 할당/Reset과 같은005424b0을 빌리며 이전 지역 사본0을 사용하지 않는다.
    std::uint32_t& archipelago{s.gameplayGlobals->GameAux()->ArchipelagoPercent()}; // 같은bb4의 metadata와 군도 강제 밀도를 Session 교체 뒤에도 공유한다.
    std::uint32_t& clockStamp{s.gameplayGlobals->BattleClockStamp()};std::uint32_t& baseClock{s.gameplayGlobals->BattleClockOrigin()}; //메시지와같은00594fb0/4의Clientowner를로그인/타이머/수신도직접쓴다.
    std::uint32_t& optionValue{s.gameplayGlobals->GameAux()->RefundPercent()}; // 실제25비율을 Reward/Options와 공유한다.
    std::uint32_t& begin{s.localStateGlobals->Begin()};std::uint32_t& central{s.localStateGlobals->CentralCreated()}; // 실제 Login/Ready/metadata도 Client 수명의 같은 DWORD를 읽고 쓴다.
    std::uint32_t& coreActive{s.coreDataGlobals->Active()}; // Chat/현재 Core getter/패킷이 같은00595348을 직접 사용한다.
    std::uint32_t& fortCount{s.playerRecords->FortCount()}; // 원문 Player 초기화와 같은005951f8이다.
    std::uint32_t& lastPlayer{s.playerRecords->LastLoginPlayer()}; // 같은005951fc를 로그인도 직접 쓴다.
    std::uint32_t& zonePending{s.localStateGlobals->ZonePending()};std::uint32_t& source{s.localStateGlobals->SourceType()}; // Fort 요청/State/Reply가 같은 실제 두 DWORD를 공유한다.
    std::uint32_t& ticks{s.localStateGlobals->GameTicks()}; //State7의 저장/복원과004c1f20이 같은 현재0055b4ac를 사용한다.
    std::uint32_t& cdPresent{s.localStateGlobals->CdPresent()};std::uint32_t& metaRefresh{s.localStateGlobals->MetaRefresh()};std::uint32_t& metaRebuild{s.localStateGlobals->MetaRebuild()}; //Editor와 로그인 State가 같은 CD 결과/두 메타 DWORD를 빌린다.
    std::uint32_t localRequestPhase{},uiMode{},inputMode{}; //독립 fixture의 기존 결과0과 아직 별도로 소유한 두 DWORD다.
    std::uint32_t& requestPhase{effects.requestPhase ? *effects.requestPhase : localRequestPhase}; //실제 Client 로그인은 SceneMusic/State3와 같은005c85ac 주소를 직접 빌린다.
    std::uint32_t& scriptBreak{s.localStateGlobals->ScriptBreak()}; // 00542544의 최초1과 현재 DWORD를 같은 Client 수명에서 빌리고 실제 Timer도 직접 쓴다.
    double& captureEnd{s.localStateGlobals->CaptureEnd()};double& connectEnd{s.localStateGlobals->ConnectEnd()}; // 서로 다른 원본 벽시각 기한을 직접 빌린다.
    double wall{};std::int32_t timeout{}; // 로그인 장치의 현재 벽시각 입력과 설정 값이다.
    double& activityEnd{s.localStateGlobals->ActivityEnd()}; // Editor가 쓴 같은00544d58도 Session 전환에서 보존한다.
    std::array<std::uint32_t,7>& settingValues{s.gameplayGlobals->GameAux()->MissionSettings()}; // 실제 Client의 현재7DWORD를 수명째 빌리며 새 Login이 재설정하지 않는다.
    std::vector<std::uint8_t> wireTypes; //실제수신한패킷종류의 진단기록이며 분기를 대신하지 않는다.
    //선택한 실제 장치가 빠지면 그 지점에서 명시 오류를 보인다.
    template<class F> static const F& Need(const F& hook) { if (!hook) throw std::logic_error("실제 미션 로그인 장치 미연결");return hook; }
    // 생성 도중 실패해 Login 소멸자가 실행되지 않는 때에도 외부 등록 대여를 반납한다.
    struct RegistrationRollback {
        CampaignTransportServices& current; // 실제 외부 등록 반납 장치를 빌린다.
        bool armed; // 정상 생성 완료 전의 외부 대여만 선택한다.
        ~RegistrationRollback() { if (armed) current.detachSession(); } // 반납 장치는 noexcept 계약이며 실패를 성공으로 숨기지 않는다.
    };
    // 장기 TCP/공유 분배기의 수신기가 짧은 Login this를 보관하지 않게 하위 모듈 소멸 전에 반납한다.
    ~Login() { if (effects.transport) transport.detachSession(); } // 외부 등록 owner가 제공하는 noexcept 토큰 반납만 허용한다.
    void SetUiMode(std::uint32_t mode) { // 현재 실제 미션에는466280을 요구하며 상태8 전용 독립 장치는 이전 관측 범위를 유지한다.
        uiMode=mode;if (s.services.adoptInitializedPlayers || effects.uiMode || effects.battlePalette || effects.startingProgress) Need(effects.uiMode)(mode);else Need(effects.refresh)();
    }
    void CloseProgress() { // 실제 Client의 안내 수명은 단순 refresh로 끝내지 않는다. 독립 부분 검사의 기존 경계만 유지한다.
        if (s.services.adoptInitializedPlayers || effects.uiMode || effects.battlePalette || effects.closeProgress || effects.startingProgress) Need(effects.closeProgress)();else Need(effects.refresh)();
    }
    //실제6BYTE 주소를 같은DWORD 배열에서 빌린다.
    std::span<std::uint8_t> Zone() { return s.localStateGlobals->Zone(); }
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
    // 실제10.78의 현재 fortSpec을 I/O 없이 선택한다. 독립 입력 어댑터만 기존 명시 이름/reader를 유지한다.
    MissionFortFileSelection SelectFile() {
        if (s.services.missionFortFileSelect) return s.services.missionFortFileSelect(s.missionFortName);
        const auto name=s.services.missionFortFileName.empty() ? s.missionFortName : s.services.missionFortFileName;
        return {name,[this,name] { return s.services.missionFortRead ? s.services.missionFortRead(name) : s.assets.files.Read("d/"+name+".fort"); }};
    }
    // 원문 Load가 선택한 하위 효과를 같은 현재 타입/owner/제목과 연결한다. 파일 읽기는 Load의 공개/권한 처리 뒤에만 실행한다.
    TemplateFileLoadHooks FileHooks(const MissionFortFileSelection& selected) {
        auto hooks=s.services.missionFortLoad;hooks.read=[read=selected.read](auto) { return Need(read)(); };
        hooks.sameName=[](auto first,auto second) { const std::string left(first),right(second);return _stricmp(left.c_str(),right.c_str())==0; }; // 원본 CRT의 NUL 문자열 비교를 그대로 사용한다.
        if (!hooks.wrongName) hooks.wrongName=[this] { Need(effects.log)("Template.cpp 진단 386: 기존 manager 파일 이름 불일치\n"); };
        hooks.typeCount=[this] { return static_cast<std::int32_t>(s.core.types.size()); };hooks.currentSlot=[this] { return s.playerState.templateOwner; };hooks.refreshTitle=s.services.install.refreshWindowTitle;
        return hooks;
    }
    // 기존 manager는 원문004bf550이 같은 이름 비교 뒤 보존한다. 실제 Objects의 읽기/생성 시점도 앞당기지 않는다.
    void Reload(std::uint32_t owner) {
        if (owner>=9) throw std::out_of_range("현재 미션 Template의 실제9슬롯 범위");const auto selected=SelectFile();if (selected.fileName.empty()) throw std::runtime_error("미션 파일 선택기의 실제 파일 이름 누락");
        s.templateStorage->LoadFile(owner,{},selected.fileName,FileHooks(selected));
    }
    //004b3d30의 자기 ordinary Fort는 고정 헤더를 유지하되 Player 생성 번호는 배지 적재 뒤 현재 값을 다시 읽는다.
    void LocalFort(std::uint32_t owner) {
        Reload(owner);if (!s.runtime || !s.player) throw std::logic_error("자기 일반 Fort의 실제 World/Player 부모 미연결");
        auto children=s.MakeExistingFortChildren(s.runtime->Objects());
        children.selectMeta(s.templateStorage->Template(owner),owner,false);children.meta->LoadBadgesSelected([&]()->FortMetaStream& { return children.metaStream(s.templateStorage->Template(owner),false); });
        static_cast<void>(s.player->Create(std::bit_cast<std::int32_t>(s.playerState.battleOwner)));s.ReadExistingFortCoreData(owner,true);
        children.selectMeta(s.templateStorage->Template(owner),owner,true);children.meta->LoadTechnologySelected([&]()->FortMetaStream& { return children.metaStream(s.templateStorage->Template(owner),true); });
        s.ReadExistingFortDeck(owner); // 수신 이미지 병합/전투섬 생성은 외곽 수신기의 현재 server/hidden 관문 뒤에만 수행한다.
    }
    // 원문004606d0→004bfba0의 원격 준비만 실행한다. 자기 Fort의 Load/선택과 수신 이미지 병합을 앞당기지 않는다.
    void OpenRemoteFort(std::uint32_t owner) {
        if (!s.services.remoteFortFiles) throw std::logic_error("원격 Fort의 사용자 작업 namespace 미연결");
        const auto path=Need(s.services.remoteFortPath)(owner,s.playerState.battleOwner);NewFortManagerDiagnostics diagnostics;
        diagnostics.templateAssertion=[this](auto) { Need(s.services.ai.assertion)("!tm"); };
        diagnostics.initializeAssertion=[this](auto) { Need(s.services.ai.assertion)("원격 Fort manager 초기화 진단"); };
        CreateNewEditorFortManager(*s.templateStorage,*s.services.remoteFortFiles,owner,path,1,diagnostics);
    }
    // 같은 고정 헤더의 실제 Changed 전체 부모에 수신 이미지 읽기를 연결한다. 원문 preventSave=1의 자연 관문을 보존한다.
    void ReadRemoteFort(std::uint32_t owner,std::span<const std::uint8_t> image) {
        TemplateImageLoadHooks hooks;hooks.assertion=[this](auto kind) {
            const auto expression=kind==TemplateImageAssertion::MissingManager ? "tm" : kind==TemplateImageAssertion::NotInBattle ? "inBattleMode" : "valid";Need(s.services.ai.assertion)(expression);
        };
        hooks.typeCount=[this] { return static_cast<std::int32_t>(s.core.types.size()); };hooks.tooManyTypes=[this] { Need(s.services.ai.assertion)("numTypes < 256"); };
        hooks.rejectFile=[this](auto path) { if (!s.services.remoteFortFiles) throw std::logic_error("원격 Fort 서명 거부의 사용자 파일 장치 미연결");s.services.remoteFortFiles->Delete(path); };
        hooks.changed=[this,owner] { s.templateState->Changed(owner); };s.templateStorage->ReadImage(owner,image,s.assetState.inBattle,hooks);
    }
    // 원문 특수 중앙섬은 현재 Fort 선택을 바꾸지 않고 지역 Template에 고정 SURROUNDED 자산을 읽는다.
    void SurroundedCentral() {
        auto temporary=s.templateStorage->CreateSibling();constexpr std::string_view name="\\D\\SURROUNDED.FORT"; //004b3bc0의 실제 고정 원본 파일명이다.
        auto hooks=FileHooks({std::string(name),[this] { return s.assets.files.Read("d/surrounded.fort"); }});hooks.currentSlot=[] { return 9U; };hooks.refreshTitle={}; // 지역 Template는 전역 현재 슬롯과 다르므로 제목을 갱신하지 않는다.
        try { temporary->LoadFile(0,{},name,hooks);s.BuildSurroundedCentral(*temporary);temporary->Clear(0); }
        catch (...) { temporary->Clear(0);throw; } // 호스트 지역 owner만 반납하며 이미 생성한 월드/공유 version을 되돌리지 않는다.
    }
    // 현재 실제 Client9 주소를 읽고 TCP를 내부 wildcard 주소로 바꾸지 않는다.
    std::array<std::uint8_t,20> Address() const { return Need(transport.address)(9); }
    // 실제 현재 노드가 필요한 선택 효과에만 같은 저장소를 빌린다.
    PacketConnection& RequiredConnection(std::int32_t id) const { auto* connection=Need(transport.connection)(id);if (!connection) throw std::logic_error("실제 현재 연결 없는 Login 소비자");return *connection; }
    // mutable observer/선언 길이도 현재 선택 전송에 맡기며 수신을 동기 호출하지 않는다.
    void SendMutable(std::int32_t target,std::span<std::uint8_t> bytes) { static_cast<void>(Need(transport.sendMutable)(target,bytes)); }
    //현재같은실제연결gate로만송신하며수신기를동기호출하지않는다.
    void Send(std::int32_t target,std::span<const std::uint8_t> bytes) { if (effects.observePacket && bytes[1]==0x6e) Need(effects.log)("Ready 송신 target"+std::to_string(target)+" mask"+std::to_string(Get(bytes,4))+"\n");static_cast<void>(Need(transport.send)(target,bytes)); }
    //현재 시각/수신 기록과 실제고정연결 수명을 분배기에 공급한다.
    PacketDispatchHooks Device() {
        PacketDispatchHooks hooks;hooks.now=[this] { return static_cast<std::uint32_t>(Need(effects.wallSeconds)()); };
        hooks.record=[this](PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t) { Record(connection,packet); };
        hooks.terminate=[this](PacketConnection& connection) { connection.flags|=0x40;Need(effects.log)("로컬 미션 연결 종료\n"); };
        hooks.close=[this](PacketConnection&) { Need(effects.log)("로컬 연결 큐/소켓 종료\n"); };hooks.release=[this](PacketConnection&) { Need(effects.log)("로컬 연결 반납\n"); };return hooks; //실제 큐/endpoint 반납은 LocalMissionTransport의 같은 수명 장치가 수행한다.
    }
    // 현재 Login의 실제 진단과 observer를 같은 순서로 기록한다. 장기 복합 backend는 외곽 Session API로 재조회한다.
    void Record(PacketConnection& connection,std::span<const std::uint8_t> packet) {
        if (packet.size()<4) throw std::out_of_range("현재 수신 기록의 실제 헤더 부족");wireTypes.push_back(packet[1]);if (effects.observePacket) effects.observePacket(connection.sender,s.assetState.Boss(),s.assetState.localOwner,packet);
    }
    //같은상태9/응답13이사용하는실제시계/연결/Template/UI/타이머 경계다.
    LocalMissionStateHooks StateHooks() {
        LocalMissionStateHooks hooks;hooks.logState=[this](std::uint32_t phase) { Need(effects.log)("로컬 미션 상태 "+std::to_string(phase)+"\n"); };
        hooks.wallSeconds=effects.wallSeconds;
        hooks.initializeRenderer=effects.initializeRenderer; // 전체 State7 연결에서도 같은 실제 장치를 사용한다. 상태8 전용 독립 장치에 가짜 성공을 추가하지 않는다.
        hooks.connectServer=[this](auto address,auto port) { Connect(address,port); };
        hooks.connectionStatus=[this](std::int32_t owner) { return Need(transport.status)(owner); }; // buffered bit2와 실제 연결 완료를 구별한다.
        hooks.connectionOpen=[this](std::int32_t owner) { return Need(transport.isOpen)(owner); };
        hooks.listenServer=[this] { Need(transport.listen)(std::bit_cast<std::uint32_t>(Need(effects.configInt)("gameServerPort",6799))); }; // 서버 권한과 별개인 simulconnect의 실제 listener만 연다.
        hooks.initializeDensity=[this] { auto& aux=*s.gameplayGlobals->GameAux();options->InitializeDensity(s.random,aux.ArchipelagoMinimum(),aux.ArchipelagoMaximum(),aux.ArchipelagoPercent()); }; // metadata/지형 root와 같은 현재 세 DWORD·게임 난수다.
        hooks.clearChat=s.services.ai.participation.clearChat;hooks.progress=effects.progress;
        hooks.loadReconnectFort=effects.selectReconnectFort; // 기존 일반 State7 몸체도 같은00460980 이름/manager 부모를 사용한다.
        hooks.takeServer=[this] { TakeServer(); }; //State15도 현재 같은 Login/World를 보유하고 원문 공통부에 즉시16을 요청한다.
        hooks.closeConnection=[this](auto id) { Need(transport.close)(id); };hooks.renameConnection=[this](auto pending,auto owner) { Need(transport.assign)(pending,owner); };
        hooks.logConnection=[this](auto line,auto value) { Need(effects.log)("연결 진단 "+std::to_string(line)+":"+std::to_string(value)+"\n"); };
        hooks.serverConnection=[this] { return transport.registry->Cleanup().transceivers[9]; };hooks.attachCommander=effects.attachCommander ? effects.attachCommander : std::function<void(std::uint32_t)>{[this](auto connection) { clockSynchronization->Attach(*transport.dispatch,connection); }}; // 실제 Client는 같은 Commander 표본/기한/연결을 설치하며 기존 명시 외부 장치 대여만 존중한다.
        hooks.installMapHandlers=[this] { clockSynchronization->InitializeServer(*transport.dispatch); }; // 같은 Zone 설치 꼬리의 서버 시간 receiver/observer 등록이다.
        hooks.sendLogin=[this] {
            const auto image=[this] { // 실제 State9은 지역 readonly Template를 읽고 선택된 다섯 섹션의 이미지를 만든 뒤 모두 반납한다.
                if (s.services.missionFortFileSelect) { const auto selected=SelectFile();return s.templateStorage->ReadLoginImage(selected.fileName,FileHooks(selected),s.localStateGlobals->BattleTerritory()); }
                return RawFortImage(OriginalEdition::Patch1078,s.templateStorage->Template(s.playerState.templateOwner),{[this] { return static_cast<std::uint32_t>(s.core.types.size()); },{}}).Save(); // 기존 독립 합성 입력의 부분 프로토콜 검사는 파일 장치를 요구하지 않는다.
            }();const auto packet=netstorm::zacket::MakeLocalMissionLogin(s.globals.mySubscriber,{reinterpret_cast<const char*>(s.handle.data())},image.Bytes());Send(9,packet.Bytes());
        };
        hooks.connectedProgress=[this] { if (effects.progress) effects.progress(s.globals.missionPost ? LocalMissionProgress::MissionTransferring : saved ? LocalMissionProgress::ReconnectTransferring : LocalMissionProgress::Transferring,0);else Need(effects.refresh)(); };hooks.closeProgress=[this] { CloseProgress(); }; // 실제 Client는004b6090의 안내를 쓰고 기존 독립 부분 입력만 refresh를 유지한다.
        hooks.fortAvailable=[this](std::uint32_t owner) { return owner<9 && Get(s.headers,owner*20)!=0; };hooks.loadCurrentFort=[this] { Reload(s.playerState.templateOwner); };
        hooks.fortCore=[this](std::uint32_t owner) { return s.templateState->Get(owner); };hooks.touchFort=[this](std::uint32_t owner) { s.templateState->Changed(owner); };
        hooks.localAddress=[this] { return Address(); };hooks.uiMode=[this](std::uint32_t mode) { SetUiMode(mode); };
        hooks.refreshFort=s.services.install.refreshWindowTitle;hooks.loginTimer=[this](bool start) { timer->Run(start); };hooks.inputMode=[this](bool mode) { inputMode=mode;Need(effects.refresh)(); };
        return hooks;
    }
    // 같은 장기9기록/현재 Player base를 실제 복원 부모에 연결하고 선택된 Chat만 재초기화한다.
    void RestoreCoreData(std::uint32_t owner) {
        PlayerCoreRestoreHooks hooks;
        hooks.assertion=[this](std::string_view expression,std::string_view file,std::uint32_t line) { // 원문 진단 복귀 뒤 현재 base와 owner를 다시 읽게 한다.
            if (!s.services.ai.assertion) throw std::logic_error("CoreData 실제 진단 장치 미연결 "+std::string(file)+":"+std::to_string(line));s.services.ai.assertion(expression);
        };
        hooks.initializeChat=effects.initializeChat;
        const std::uint32_t metamap=s.core.ownerMode.challenge ? 1U : 0U; // Restore는 이 getter 관문을 호출하지 않으며 같은 globals 형식만 빌린다.
        s.coreDataGlobals->Restore(s.core.pool.Edition(),s.playerState,metamap,
            {s.globals.missionPost,saved,RawCheckingFlag{s.gameplayGlobals->AiDebug().Checking()}},owner,hooks);
    }
    // 일반 완료의 실제 Core/Player/옵션을 같은 Template/메시지/입력 장치에 연결한다.
    void InitializeGeneral() {
        OrdinaryBattleStartHooks hooks;hooks.core=[this] { const std::uint32_t metamap=s.core.ownerMode.challenge;PlayerCoreDataHooks diagnostic;diagnostic.assertion=[this](auto) { Need(s.services.ai.assertion)("_myCoreData == &lastKnownCoreData"); };return s.coreDataGlobals->Read(s.playerState,metamap,diagnostic); };
        hooks.writeCore=s.writeCurrentFortCore;hooks.touchFort=[this](auto owner) { s.templateState->Changed(owner); };hooks.status=effects.status;hooks.queueAlliance=effects.queueAlliance;hooks.gameSeconds=[this] { return s.core.processState.Now(); };hooks.log=s.services.player.log;
        hooks.broadcastPlayer=[this](auto owner) { for (std::uint32_t target=1;target<9;++target) if (Get(s.players,target*180U+0x50U,1)&1U) metadata->SendPlayer(owner,static_cast<std::int32_t>(target)); }; // 원문00491270처럼 현재 실제 접속자만 송신 대상으로 재조회한다.
        auto& aux=*s.gameplayGlobals->GameAux();RawOrdinaryBattleStart({s.players,s.battleOptions,s.core.processState.Server(),s.core.ownerMode.battle,s.playerState.templateOwner,s.playerState.battleOwner,aux.SpellCount(),aux.SpellDeadline()},std::move(hooks)).Run();
    }
    //현재 원본 로그인/패킷/상태/타이머 모듈을 같은 저장소로 조립한다.
    Login(Impl& owner,CampaignLoginServices device,std::function<void()> load):s(owner),effects(std::move(device)),install(std::move(load)) {
        // 첫 실제 접속/설치 전에 Init의22개 기본 옵션/색과 예약 영역 보존을 같은87BYTE에 적용한다.
        if (!s.services.missionOptions) ResetMissionBattleOptions(OriginalEdition::Patch1078,s.battleOptions); // 실제 Client Init이 쓴 전역을 다시 지우지 않는다.
        if (effects.transport) {
            transport=*effects.transport;if (!transport.lifetime || !transport.registry || !transport.dispatch || !transport.detachSession || !transport.callbacksActive) throw std::logic_error("외부 Login 전송의 실제 owner/Registry/분배기/Session 등록 반납/콜백 관문 누락");
            if (s.services.connectionRegistry && s.services.connectionRegistry!=transport.registry) throw std::logic_error("Login과 Client State의 서로 다른 현재 연결표");s.services.connectionRegistry=transport.registry;
        } else { internalTransport=std::make_shared<netstorm::zacket::LocalMissionTransport>(s.players,s.core.processState.Server(),Device(),s.services.connectionRegistry);transport=BindInternalCampaignTransport(internalTransport); }
        RegistrationRollback rollback{transport,effects.transport.has_value()}; // 첫 실제 등록보다 먼저 생성 실패 복구를 설치한다.
        auto send=[this](std::int32_t target,std::span<const std::uint8_t> bytes) { Send(target,bytes); }; //모든 부모가 공유하는 같은 실제 연결이다.
        options=std::make_unique<RawMissionBattleOptions>(OriginalEdition::Patch1078,MissionBattleOptionState{s.battleOptions,s.world.MutableTypes(),typeUse,ranges,s.geyserType,range,s.energyState.radiusSquared,optionValue});
        MissionServerPacketHooks packets;packets.send=send;packets.peerAddress=[this](std::uint32_t owner) { return Need(transport.peerAddress)(std::bit_cast<std::int32_t>(owner)); };packets.log=effects.log;
        packets.assertion=[this](auto expression) { Need(s.services.ai.assertion)(expression); };packets.creatingIslands=effects.creatingIslands;
        packets.setArchipelago=[this](auto value) { archipelago=value; };packets.setCentralCreated=[this](auto value) { central=value; };packets.layout=[this] { return s.battleOptions.at(5); };
        packets.randomCentral=[this](auto record) { s.BuildCentralIsland(record); };packets.predictableCount=[this] { return predictable; }; // 실제 Pool 카운터와 같은 현재 중앙섬 부모를 사용한다.
        packets.surroundedCentral=[this] { SurroundedCentral(); }; // layout2도 실제 원문 고정 자산/지역 Template/전체 배치 부모에 연결한다.
        metadata=std::make_unique<RawMissionServerPackets>(s.players,MissionServerPacketState{s.core.processState.Server(),s.core.ownerMode.challenge,s.playerState.battleOwner,central,archipelago,Zone()},std::move(packets));
        forts=std::make_unique<RawMissionLoginPackets>(OriginalEdition::Patch1078,s.players,MissionFortPacketReads{[this](std::uint32_t player)->const FortTemplate* { return player<9 && Get(s.headers,player*20) ? &s.templateStorage->Template(player) : nullptr; },[] { return std::uint16_t{4}; }},FortImageHooks{[this] { return static_cast<std::uint32_t>(s.core.types.size()); },{}},MissionFortPacketHooks{[this](std::uint32_t,std::int32_t) { Need(effects.log)("로컬 Fort 전송\n"); },send,{}});
        MissionSettingsPacketHooks setting;setting.current=[this](MissionSettingsField field) { return settingValues.at(static_cast<std::size_t>(field)); };setting.setCurrent=[this](MissionSettingsField field,std::uint32_t value) { settingValues.at(static_cast<std::size_t>(field))=value; };setting.send=send;settings=std::make_unique<RawMissionSettingsPacket>(OriginalEdition::Patch1078,std::move(setting));
        ClockSynchronizationHooks synchronization;synchronization.gameSeconds=effects.gameSeconds;synchronization.wallSeconds=effects.wallSeconds;
        synchronization.send=[this](auto token,auto packet) { transport.registry->SendDirect(token,packet); }; // 원문 Commander의 고정 this token을 사용하고 Player ID를 재선택하지 않는다.
        synchronization.reply=[this](auto& connection,auto packet,const auto* address) {
            if (address && *address!=connection.address) throw std::logic_error("고정 TCP/내부 peer와 다른 시간 응답 주소는 LAN 범위 밖"); // 현재 TCP/내부 가상+8은 같은 고정 peer에만 보낸다. UDP/인터넷 목적지를 임의 대체하지 않는다.
            transport.registry->SendDirect(transport.registry->TokenFor(connection),packet);
        };
        clockSynchronization=std::make_unique<RawClockSynchronization>(s.localStateGlobals->ClockSynchronization(),s.localStateGlobals->ClockSamples(),std::move(synchronization));
        state=std::make_unique<RawLocalMissionState>(OriginalEdition::Patch1078,LocalMissionStateStorage{blocks[0],blocks[1],blocks[2],blocks[3],blocks[4]},LocalMissionStateGlobals{s.core.processState.Server(),s.assetState.Boss(),s.assetState.inBattle,s.globals.missionPost,saved,s.objectState.mission,simul,s.core.ownerMode.challenge,s.playerState,zonePending,ticks,source,cdPresent,blocks[2][33],metaRefresh,metaRebuild,title,battleByte,connectEnd,activityEnd,s.core.processState.Now(),wall},StateHooks());
        MissionLoginHooks serverHooks;serverHooks.disconnect=[this](auto id) { Need(transport.close)(id); };
        serverHooks.pendingReply=[this](PacketConnection& connection,auto packet) { const auto id=std::bit_cast<std::int32_t>(connection.sender);if (&RequiredConnection(id)!=&connection) throw std::logic_error("거부 응답의 현재 연결 정체성 불일치");Send(id,packet); };
        serverHooks.peerAddress=[this](auto owner) { return Need(transport.peerAddress)(std::bit_cast<std::int32_t>(owner)); };
        serverHooks.send=send;serverHooks.broadcast=[this](std::span<const std::uint8_t> bytes) { s.broadcast->Send(bytes); };serverHooks.flush=[this] { Need(transport.flush)(); };serverHooks.wallSeconds=effects.wallSeconds;serverHooks.clock=effects.clock;serverHooks.spBits=[this](std::uint32_t player) { return s.sp.Get(player); };
        serverHooks.applyOptions=[this](std::span<const std::uint8_t> bytes) { client->Options(0,bytes); };serverHooks.clearTypes=[this] { options->ClearTypes(); };
        serverHooks.sendPlayer=[this](std::uint32_t player,std::uint32_t target) { metadata->SendPlayer(player,static_cast<std::int32_t>(target)); };serverHooks.sendFortMetadata=[this](std::uint32_t target) { metadata->SendMetadata(static_cast<std::int32_t>(target)); };
        serverHooks.loadCurrentFort=[this](std::uint32_t player) { Reload(player); };serverHooks.sendFort=[this](std::uint32_t player,std::uint32_t target,std::uint32_t count) { static_cast<void>(forts->SendFort(player,static_cast<std::int32_t>(target),count)); };serverHooks.loadWorld=install;serverHooks.sendSettings=[this](std::uint32_t target) { settings->Send(static_cast<std::int32_t>(target)); };serverHooks.restoreCoreData=[this](std::uint32_t player) { RestoreCoreData(player); }; // 두 원문 caller가 같은 장기 Core 복원 몸체를 사용한다.
        serverHooks.loadFortImage=[this](auto owner,auto image) { OpenRemoteFort(owner);ReadRemoteFort(owner,image); }; // 원문 신규 원격 로그인은 준비/읽기를 같은 슬롯에 순서대로 수행한다.
        serverHooks.synchronizePeer=[this](auto owner) { s.world.SynchronizePeer(owner); }; // 실제004a7350의 현재 전체 월드/종속/공간 재등록 부모다.
        serverHooks.makeBattleIsland=[this](auto owner) { s.BuildBattleIsland(owner); }; // 일반 전투의 watcher 조건은 RawMissionLogin이 선택하고 이 부모는 실제 생성만 수행한다.
        serverHooks.assertion=[this](auto expression) { Need(s.services.ai.assertion)(expression); }; // 원문 simulconnect/메타맵/예측수 진단도 같은 실제 Client 진단으로 복귀한다.
        serverHooks.setSp=[this](std::uint32_t player,std::int32_t amount) { // 초기 지정도 같은 분산 SP와 현재 로컬/Editor 판정·HUD 갱신 전체를 사용한다.
            SetPlayerSpAmount(std::bit_cast<std::int32_t>(player),amount,s.sp,
                {[this] { return s.playerState.playerKnown; },[this] { return s.playerState.battleOwner; },[this] { return s.globals.editor ? 1U:0U; }},[this] { Need(effects.refresh)(); });
        };
        serverHooks.cleanupReconnectClients=[this] { CleanupReconnectClients(); };serverHooks.checkingAssertions=[this] { return s.gunState.checking; };serverHooks.log=effects.log; //63h도 현재 같은Pool/TYPE/지갑/가상표와checking/진행 로그를 사용한다.
        server=std::make_unique<RawMissionLogin>(s.players,*s.participation,MissionLoginGlobals{s.core.processState.Server(),s.assetState.Boss(),simul,s.objectState.mission,s.globals.missionPost,s.core.ownerMode.challenge,s.playerState,s.globals.mySubscriber,fortCount,lastPlayer,predictable,begin,beginSeed,battleSeed,burst,clockStamp,baseClock,captureEnd,currentCore,cachedCore,Zone(),s.battleOptions,&s.gameplayGlobals->GameAux()->ResourcePool(),&saved,&s.assetState.inBattle},std::move(serverHooks)); // 실제 저장게임/전투 bool 주소를 빌리며 일반 모드를 강제로 켜지 않는다.
        MissionClientPacketHooks receive;receive.clock=effects.clock;receive.wallSeconds=effects.wallSeconds;receive.runState=[this] { state->Run(); };receive.resetCamera=s.services.install.resetCamera;receive.refreshSp=effects.refresh;
        receive.outpostOptions=effects.metaBattleOptions; //State67h의 현재 CS 비영 분기만 실제 Client CS/음향/NSInfo 자식에 연결한다.
        receive.reconnectUi=effects.reconnectUi; //같은 현재 savedGame 선택이004c7880 전체 HUD 재생성을 호출한다.
        receive.localFort=[this](auto owner) { LocalFort(owner); }; // savedGame 여부와 무관한 !missionPost의 실제 자기 Fort 사슬이다.
        receive.openFortImage=[this](auto owner) { OpenRemoteFort(owner); };receive.readFortImage=[this](auto owner,auto image) { ReadRemoteFort(owner,image); }; // packet2a는 두 선택을 분리해 manager를 중복 생성하지 않는다.
        receive.buildFort=[this](auto owner) { s.BuildBattleIsland(owner); }; // 비서버 packet2a의 현재 이미지 뒤 실제 전투섬을 만든다. boss가 아니면 Objects를 자연 생략한다.
        receive.updateRange=[this] { options->UpdateRange(); };receive.requiredEnergy=[this] { return options->TypeCost(); };receive.setEnergyType=[this](float value) { options->SetSelectedTypeCost(value); };receive.readOptionValue=[this] { return options->CurrentValue(); };
        receive.addChat=s.services.ai.participation.addChat;receive.playerUiBusy=effects.playerUiBusy;receive.playerNotice=effects.playerNotice;receive.send=send;receive.sendReady=[this](std::uint32_t target) { server->SendReady(target); };receive.restoreCoreData=[this](std::uint32_t player) { RestoreCoreData(player); }; // 004912d0의 실제 후처리도 같은 복원 부모와 Player/캐시를 빌린다.
        client=std::make_unique<RawMissionClientPackets>(s.players,*s.participation,s.sp,MissionClientPacketState{s.core.processState.Server(),s.assetState.Boss(),s.globals.missionPost,saved,s.core.ownerMode.challenge,s.assetState.inBattle,s.globals.editor,outpost,s.playerState,beginSeed,battleSeed,s.revision,optionValue,begin,clockStamp,baseClock,blocks[1][0],blocks[3][0],blocks[3][14],blocks[4][0],source,coreActive,captureEnd,s.loginEnd,currentCore,cachedCore,Zone(),s.battleOptions,s.handle,s.hiddenPlayers,&s.gameplayGlobals->GameAux()->ResourcePool()},std::move(receive));
        MissionLoginTimerHooks timing;timing.configInt=[this](std::string_view key) { return Need(effects.configInt)(key,30); };timing.wallSeconds=effects.wallSeconds;timing.clock=effects.clock;timing.broadcast=[this](std::span<const std::uint8_t> bytes) { if (effects.observePacket) Need(effects.log)("Timer broadcast local"+std::to_string(s.assetState.localOwner)+"\n");s.broadcast->Send(bytes); };timing.priestRefresh=[this](std::uint32_t player) { s.priestParticipation->Refresh(player); };
        timing.connection=[this](std::int32_t peer) { return peer==9 && Need(transport.exists)(9); }; //004e2b10은 flags2/IsOpen이 아닌 현재 Client9 객체 존재를 조회한다.
        timing.remoteStatus=[this](std::int32_t peer,std::uint32_t,std::uint32_t) { s.connectionActivity->Receive(peer); }; //004b6180의 wire 수신과 같은 몸체이며 미사용 두 Native 인자를 합성하지 않는다.
        timing.saveReconnect=[this] { SaveReconnect(); };timing.closeProgress=[this] { if (effects.closeTimerProgress) effects.closeTimerProgress();else CloseProgress(); };timing.log=[this](MissionLoginTimerLog,std::string_view text) { Need(effects.log)(text); };
        timing.progress=[this] { const auto body=timer->ProgressText(Need(effects.frameWallSeconds)(),Need(effects.translate));s.localStateGlobals->LoginProgressSerial()=Need(effects.showTimerProgress)(body); }; // 같은 현재9 Player/프레임 시각과cf570의 실제 반환 값을 사용한다.
        timing.initializeGeneral=[this] { InitializeGeneral(); };timing.createInitialGeysers=s.createWorldGeysers;timing.abortBattle=effects.abortBattle;timing.resetCamera=s.services.install.resetCamera;timing.serverAssertion=[this] { Need(s.services.ai.assertion)("server"); }; // 미션의 자연 관문은 유지하고 선택된 일반 초기화/중단/카메라만 실제 부모를 요구한다.
        timer=std::make_unique<RawMissionLoginTimer>(OriginalEdition::Patch1078,MissionLoginTimerState{s.loginEnd,timeout,begin,scriptBreak,requestPhase,baseClock},counts,MissionLoginTimerReads{[this] { return saved; },[this] { return s.core.processState.Server(); },[this] { return s.objectState.mission; },[this] { return s.globals.missionPost; },[this] { return s.assetState.Boss(); },[this] { return s.gunState.checking; }},std::move(timing));
        RegisterDeveloperCommandPacket(*transport.dispatch,[this](auto text) { s.ExecuteDeveloperCommand(text); },[storage=s.world.Storage()](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }); // Login 진행 중에도 같은31h 실제 해석기/observer를 사용한다.
        RegisterMissionLogin((*transport.dispatch),*server);RegisterMissionClientPackets((*transport.dispatch),*client);RegisterMissionMetadata((*transport.dispatch),*metadata);RegisterMissionSettingsPacket((*transport.dispatch),*settings);
        RegisterConnectionActivity((*transport.dispatch),*s.connectionActivity); // 실제64h/version1/정책0/빈 observer를 같은 일반 수신 분배기에 등록한다.
        RegisterChatPacket((*transport.dispatch),s.chatGlobals->receivedMask,s.ChatDevices()); // 같은 단독 양끝의41h mutable observer와 중계를 실제 UI 수신에 등록한다.
        RegisterBattleLeavePacket((*transport.dispatch),s.players); // 실제 State3의 종료52h도 같은 큐에서 수신하며 직접 Player flag를 합성하지 않는다.
        // 원본3d 등록의 observer는0040b380의 빈 반환이고 version1/mismatch0이다.
        (*transport.dispatch).Observe(0x3d,[](std::uint32_t) {});(*transport.dispatch).Version(0x3d,1,0);
        (*transport.dispatch).Register(0x3d,[this](std::uint32_t,PacketConnection&,std::span<const std::uint8_t> bytes) {
            // 모든 backend는 callback 동안 소유한 mutable 수신 frame을 제공한다. 같은 실제 버퍼에 원본 self bit를 쓴다.
            s.participation->ReceiveAlliance({const_cast<std::uint8_t*>(bytes.data()),bytes.size()},[this](std::span<const std::uint8_t> packet) { s.broadcast->Send(packet); });
        });
        (*transport.dispatch).Observe(0x78,{});(*transport.dispatch).Version(0x78,1,0);(*transport.dispatch).RequireLength(0x78,6);
        (*transport.dispatch).Register(0x78,[this](std::uint32_t,PacketConnection&,std::span<const std::uint8_t> packet) {
            const auto owner=std::uint32_t(packet[4]),target=std::uint32_t(packet[5]);if (s.assetState.Boss()) { s.participation->AddAlly(owner,target);s.participation->AddAlly(target,owner);s.broadcast->Send(packet); }Need(effects.status)(7,owner,target);
        }); //0048f880은 현재 boss에서 양방향 AddAlly/전체분배를 하고 모든 수신자가 실제 종류7 안내를 연다.
        rollback.armed=false; // 정상 생성 뒤에는 Login 소멸자가 하위 처리기보다 먼저 같은 대여를 반납한다.
    }
    MissionLoginPlayerCounts counts{OriginalEdition::Patch1078,s.players}; //같은 Player9의 현재 집계를 타이머에 공급한다.
    //004b4be0의 실제 가변 Zone을 같은 Player/Core/권한에 설치한다. 단독/일반 모두 이 몸체를 사용한다.
    void InstallZone(std::span<const std::uint8_t> zone) {
        netstorm::zacket::LocalMissionZoneHooks hooks;hooks.loginTimeout=[this] { return Need(effects.configInt)("loginTimeout",30); };hooks.seconds=effects.wallSeconds;
        hooks.storeName=[](std::uint32_t subscriber,std::string_view name) { SanitizeMissionPlayerName(subscriber,{reinterpret_cast<std::uint8_t*>(const_cast<char*>(name.data())),name.size()+1}); };
        hooks.serve=[this](std::uint32_t port) { Need(transport.serve)(port); };
        hooks.assertion=[this](auto expression) { Need(s.services.ai.assertion)(expression); };
        netstorm::zacket::InstallLocalMissionZone(zone,s.players,s.playerState,{s.core.processState.Server(),s.assetState.Boss(),s.globals.mySubscriber,pendingStamp,activeStamp,zoneWord,zoneWords[0],captureEnd,adjust,s.objectState.mission,overrideFlags,s.core.ownerMode.challenge,currentCore,cachedCore},std::move(hooks),static_cast<std::uint32_t>(Need(effects.configInt)("gameServerPort",6799)));
    }
    // 일반 State7의 실제 후반이다. 호스트 Zone의 유무를 캡처값으로 선택하며 client에 합성 미션 명단을 공급하지 않는다.
    void BeginGeneral(bool parentRunning) {
        const auto request=s.services.generalBattleStart;
        if (!parentRunning || !request || request->consumed || !request->starting || blocks[3][0]!=7 || !effects.transport || s.globals.missionPost || s.objectState.mission) throw std::logic_error("일반 LAN의 외부 State7/같은 요청/실제 전송 계약");
        if ((request->starting->zonePending!=0)==request->zone.empty()) throw std::logic_error("일반 LAN의 정리 전 Zone 표식/가변 이미지 불일치");
        if (simul) Need(transport.listen)(std::bit_cast<std::uint32_t>(Need(effects.configInt)("gameServerPort",6799))); // 원문은 서버 권한과 별개로 Zone 설치 전에 simulconnect listener를 연다.
        if (request->starting->zonePending) {
            InstallZone(request->zone);std::vector<std::uint8_t>{}.swap(request->zone);blocks[3][32]=0;blocks[4][32]=0;
            clockSynchronization->InitializeServer(*transport.dispatch); // 원문은 Zone 설치/해제 때만 서버 시간 응답기를 등록한다.
        }
        request->consumed=true;Need(effects.initializeRenderer)();s.playerState.playerKnown=0;SetUiMode(2);ResetAmbientSoundDeadline(activityEnd,effects.wallSeconds);blocks[2][1]=20;state->Request(8);
        if (blocks[3][33]) { saved=true;Need(effects.selectReconnectFort)();Need(effects.progress)(LocalMissionProgress::ReconnectStarting,0); }
        else {
            if (s.core.processState.Server()) { auto& aux=*s.gameplayGlobals->GameAux();options->InitializeDensity(s.random,aux.ArchipelagoMinimum(),aux.ArchipelagoMaximum(),aux.ArchipelagoPercent()); }
            Need(s.services.ai.participation.clearChat)(0x40);Need(s.services.ai.participation.clearChat)(0x80);Need(effects.progress)(LocalMissionProgress::ConnectingStarting,0);
        }
        Need(effects.battlePalette)(); // State7/즉시8의 공통 확정과 접속은 살아 있는 외부 Client 부모가 수행한다.
    }
    //원본 미션 부모의 로컬 Zone을 설치하고 실제 상태8에서 접속한다.
    void Begin(bool parentRunning=false) {
        if (!s.services.subscriberGlobals) { s.globals.loginSubscriber=s.services.missionSubscriber.value_or(1);s.globals.mySubscriber=s.globals.loginSubscriber; } //독립fallback만기존복사를수행한다.실제Client는Init현재쓰기뒤Login에서ID를재복사하지않는다.
        s.connectionAssign=[this](std::int32_t pending,std::int32_t player) { Need(transport.assign)(pending,player);s.services.battlePeers.at(static_cast<std::size_t>(player))=Need(transport.peer)(static_cast<std::uint32_t>(player)); };
        s.connectionAlliance=[this](std::uint32_t target,std::uint32_t owner,std::uint32_t mask) {
            std::array<std::uint8_t,9> packet{1,0x3d,9,0}; // 원본0040a780/0052ede2의 version1/type3d/length9다.
            packet[4]=static_cast<std::uint8_t>(owner);Put(packet,5,mask);Send(static_cast<std::int32_t>(target),packet);
        };
        s.services.ai.localTransport=[this] { return simul; }; // RawPlayerParticipation의 실제0054db50이며 전송 존재와 구별한다.
        s.services.carrierRequest=[this](std::int32_t target,std::span<const std::uint8_t> bytes) { Send(target,bytes); };
        if (s.services.installMode==CampaignInstallMode::GeneralBattle) { BeginGeneral(parentRunning);return; } // 일반 host/client는 단독 미션86BYTE fallback과 요청 검증을 통과시키지 않는다.
        auto request=s.services.missionFortRequest;std::optional<netstorm::zacket::LocalMissionPacket> fallback; // Init에서 작성한 같은86BYTE를 사용하며 독립 Session만 기존 작성기를 호출한다.
        if (s.services.adoptInitializedPlayers && !request) throw std::logic_error("실제 Client 미션의 State7 요청 누락");
        if (request) { if (request->consumed || request->next[0]!=7 || request->next[0x80/4]!=1 || request->zone.size()!=86) throw std::logic_error("현재 Init Fort 요청/이미지 계약 위반");if (request->starting) { if (blocks[3][0]!=7 || blocks[3][32]!=1) throw std::logic_error("정리 뒤 실제 현재 State7/Zone 계약 위반"); }else blocks[3]=request->next; } // 실제 앞단은 현재 블록을 이미 소비했다. 독립 편의 접합만 이전 Init 캡처를 사용한다.
        else fallback=netstorm::zacket::MakeLocalMissionZone();
        const bool startingTail=s.services.adoptInitializedPlayers || effects.uiMode || effects.battlePalette || effects.startingProgress; // 실제 후반 경로만 State7 공통 확정에 연결하고 기존 독립 상태8 범위를 구별한다.
        if (parentRunning && (!startingTail || !request || !request->starting || blocks[3][0]!=7)) throw std::logic_error("외부 State7 부모의 준비된 실제 앞단 누락");
        if (startingTail && !request) blocks[3][0]=7; // 독립 편의 요청도 현재7을 명시한다. 실제 Client는 Init/요청의 같은144BYTE를 사용한다.
        const auto starting=[this,&request,&fallback,startingTail] { // 앞선 호스트 정리 뒤의 실제 Zone/Renderer/안내를 원문 공통부 안에서 수행한다.
        if (!request || !request->starting) title=0; // 실제 앞단은 정리 전에 첫BYTE를 이미 지웠다. 뒤에서 현재 제목을 다시 쓰지 않으며 독립 편의 접합만 이전 경계를 유지한다.
        const auto zone=request ? std::span<const std::uint8_t>(request->zone) : fallback->Bytes();InstallZone(zone);
        if (request) { std::vector<std::uint8_t>{}.swap(request->zone);request->consumed=true;blocks[3][0x80/4]=0;blocks[4][0x80/4]=0; } // 실제 할당 반납 뒤 current/next+80만 지운다. 이미 복사한 immediate+80은 보존한다.
        clockSynchronization->InitializeServer(*transport.dispatch); // 원문 Zone 반납 뒤004c1e70의5번 등록/두 카운터0/서버 응답모드1이다.
        if (s.services.adoptInitializedPlayers || effects.initializeRenderer) Need(effects.initializeRenderer)(); // 실제 State7의4993c0 위치다. 앞선 실제 Player 정리를 채택한 미션은 장치 누락을 거부한다.
        s.playerState.playerKnown=0;
        if (startingTail) { SetUiMode(2);ResetAmbientSoundDeadline(activityEnd,effects.wallSeconds);blocks[2][1]=20;state->Request(8);Need(effects.startingProgress)();Need(effects.battlePalette)(); } // UI2→ 새 기한/인자/즉시8→ 실제 안내→팔레트이며 예외도 앞선 쓰기를 보존한다.
        };
        if (parentRunning) starting(); // 부모의 최초 복사/로그를 유지하며 후반만 반환한다. 외부 공통부가7 확정과8을 이어서 실행한다.
        else if (startingTail) state->RunStarting(starting); // 독립 편의 접합의 같은 원문 공통부가7 전체를 확정한 뒤 즉시8을 실행한다.
        else { starting();blocks[3][0]=8;state->Run(); } // 독립 상태8 검사의 이전 부분 범위이며 전체 State7 실행으로 세지 않는다.
    }
    // 원문 case15의 같은 월드/Player/등록표 인수이며 정리/새 미션 설치로 대체하지 않는다.
    void TakeServer() {
        if (Need(transport.callbacksActive)()) throw std::logic_error("수신 callback 복귀 전 State15 실행 거부"); // 실제 World/전송 역할 교체는 외부 일반 입력/Poll 위치에서만 실행한다.
        Need(s.services.player.log)(0x1d,"Server takeover\n");saved=true;transport.registry->CloseAll();s.core.processState.Server()=true;s.assetState.Boss()=true;Need(effects.clearStatus)(1,0,0);
        Need(transport.serve)(std::bit_cast<std::uint32_t>(Need(effects.configInt)("gameServerPort",6799)));
        std::array<std::uint8_t,20> address{};constexpr std::array<std::uint8_t,9> local{'z','l','o','c','a','l',',','*',0}; // 실제 내부 전송 선택이며 주소 해석은 같은 backend가 수행한다.
        std::copy(local.begin(),local.end(),address.begin());Need(transport.connect)(address,std::bit_cast<std::uint32_t>(Need(effects.configInt)("gameServerPort",6799))); //Native의Serve 뒤 현재 포트를 다시 읽고 실제 client9를 등록한다.
        std::array<std::uint8_t,8> packet{1,0x63,8,0};const auto subscriber=s.globals.mySubscriber; //40adc0의 실제63h/1/8 헤더 뒤 현재subscriber DWORD다.
        for (std::size_t byte=0;byte<4;++byte) packet[4+byte]=static_cast<std::uint8_t>(subscriber>>(8*byte)); // 현재 지역8BYTE를 동기 송신 큐에 복사한다.
        Send(9,packet);
        // 각 실제1..8 기록에서 미로그인 참여자는 단절100을 켜고 모든 flags는 원문702만 보존한다.
        for (std::uint32_t owner=1;owner<9;++owner) {
            const auto at=static_cast<std::size_t>(owner)*0xb4;if (Get(s.players,at+0x2c)) { const auto flags=Get(s.players,at+0x50);if ((flags&2U) && !(flags&1U)) Put(s.players,at+0x50,flags|0x102U); }Put(s.players,at+0x50,Get(s.players,at+0x50)&0x702U);
        }
        if (!s.core.processState.Server()) Need(s.services.ai.assertion)("server");s.core.pool.RebuildServer();timer->Run(true);
        const auto savedTicks=ticks;ResetClockSynchronization(s.localStateGlobals->ClockSynchronization());++s.revision;state->Request(16);ticks=savedTicks; // 원문의 시계3DWORD reset/미니맵 증가/즉시16/같은GameTicks 복구 순서다.
    }
    //004a7510은 현재 client의 유료 객체를 비용 환불한 뒤 현재 TYPE/count/삭제 가상으로 다시 분배한다.
    void CleanupReconnectClients() {
        if (s.core.pool.Capacity()<15000) throw std::logic_error("PATCH 재접속 client SID 물리 누락");
        // 원문5..14999의 고정 순서를 지키고 각 효과 뒤 같은 실제 슬롯의 현재 필드를 다시 읽는다.
        for (std::uint32_t number=5;number<15000;++number) {
            const Sid sid{static_cast<std::uint16_t>(number)};const auto raw=s.core.pool.Slot(sid);if ((raw[11]&7U) || raw[10]<70 || !(raw[40]&1U)) continue;
            s.hp.AddSp(s.playerState.battleOwner,static_cast<float>(s.hp.TypeCostUnits(raw[10]))); // 원문0059ab50의 정수 비용 복호화와 실제 로컬 SP/지갑/표시 부모다.
            if (raw[10]>=s.core.types.size()) throw std::out_of_range("재접속 정리 현재 TYPE 물리 범위");
            if ((s.core.types[raw[10]].flags2&0x4000U) && (raw[40]&8U) && s.RewardCount(sid)>0) continue;
            static_cast<void>(s.world.Destroy(sid,0)); // 현재 가상+10/인자0을 사용하며 별도 SID 반납/부분 효과 복구를 추가하지 않는다.
        }
    }
    //004b54e0의 SP 진단/현재 Player와 같은 Client 소유 복구 슬롯을 실제 File writer에 연결한다.
    void SaveReconnect() {
        auto hooks=effects.reconnectFile.value_or(SavedStateWriteHooks{}); // 미션/미known 자연 반환은 File 장치 부재도 허용한다.
        hooks.diagnose=[this] {
            auto& patch=s.priestState.regen.patch;const auto mismatch=s.sp.OffsetCurrent()!=static_cast<std::uint32_t>(s.sp.TrackingOffsetCurrent());auto fallback=patch.Sample();
            if (mismatch) s.gameplayGlobals->PatchDiagnostic()=fallback;
            if (patch.remaining) {
                const auto amount=s.playerState.playerKnown ? std::bit_cast<float>(s.sp.Get(s.playerState.battleOwner)):0.0F;fallback=patch.Sample(); //40ee90의 현재 known/owner/Get 뒤 현재 진단값을 다시 읽는다.
                if (amount>7000.0F) s.gameplayGlobals->PatchDiagnostic()=fallback;--patch.remaining;
            }
            if (!s.sp.FirstPoolCurrent()) s.gameplayGlobals->PatchDiagnostic()=fallback;if (s.sp.Permutation()[0]==-1) s.gameplayGlobals->PatchDiagnostic()=fallback;
        };
        hooks.localSp=[this] { return s.playerState.playerKnown ? std::bit_cast<float>(s.sp.Get(s.playerState.battleOwner)):0.0F; }; // 저장DWORD는 게임시각이 아니라 현재 SP float의 정수부다.
        WriteSavedState(SavedStateWriteGlobals{[this] { return s.globals.missionPost; },[this] { return s.playerState.playerKnown; },[this] { return s.gunState.checking; },[this] { return s.playerState.battleOwner; },[this] { return s.globals.mySubscriber; },[this] { return std::span<const std::uint8_t>(s.handle); },[this] { return std::span<const std::uint8_t>(s.players); },s.localStateGlobals->RestoreOwner(),s.localStateGlobals->RestoreSubscriber(),s.localStateGlobals->PreviousHandle(),{[this] { return s.gameplayGlobals->GameAux()->SpellCount(); },[this] { return source; },[this] { return ticks; }},s.loginEnd},hooks);
    }
    // 부모/독립 상태8이 같은 로컬 접속 몸체를 사용하며 원문 현재 블록을 검증한다.
    void Connect(std::span<const std::uint8_t> address,std::uint32_t port) {
        if (address.size()!=20 || blocks[3][0]!=8) throw std::logic_error("실제 현재8/typed주소20BYTE 접속 계약 오류");
        if (blocks[3][33]) {
            const auto field=std::span(reinterpret_cast<const std::uint8_t*>(blocks[3].data()),sizeof(blocks[3])).subspan(0x54,40);const auto end=std::find(field.begin(),field.end(),0);if (end==field.end()) throw std::length_error("저장 재접속 주소40BYTE의NUL 누락");
            Need(transport.connectPeerText)({reinterpret_cast<const char*>(field.data()),static_cast<std::size_t>(end-field.begin())},port); // Native4e2620의 실제 문자열과 별도 현재 포트를 전달한다.
        } else Need(transport.connect)(address,port); // 기존 private LAN/단독 시작의 typed 연결은 유지한다.
    }
    // 현재 송신을flush한 뒤 실제 큐를 분배한다. 프레임 계산은 기존Kernel이 수행한다.
    void Pump() { wall=Need(effects.wallSeconds)();Need(transport.flush)();clockSynchronization->Poll();static_cast<void>(Need(transport.poll)());if (!effects.frameLoginTimer && s.loginEnd!=0.0) timer->Run(false); } // 실제 Client의 타이머는439ae3에 두고 독립 Session만 기존 편의 호출을 유지한다.
};
