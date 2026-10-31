// Impl 안에 포함하며 기존 의식/내용물/Path/Kernel을 단일 현재 월드에 연결한다.
void PrepareRitual() {
    runeState.suppressDuration=globals.missionPost; // 원본00594fa4는 룬 시간·다이스 후처리·희생 완료가 함께 읽는 현재 미션 상태다.
    for (std::uint32_t owner=0;owner<=kPlayerCount;++owner) runeState.playerFlags[owner]=players[owner*0xb4+0x50]; // 룬 건설시간도 같은 현재 Player flag를 읽는다.
    ritualState.boss=assetState.Boss();ritualState.priestType=priestState.movement.priest;ritualState.runeType=runeState.runeType;
    for (std::size_t index=0;index<alliances.size();++index) daisState->alliances[index]=std::bit_cast<std::int32_t>(alliances[index]); // 현재 방향별 DWORD 비트를 부호 변환 없이 공유한다.
}
// 원본 new0x28 한 번의 성공을 실제 Regular 저장 공간 한 번의 확보로 보존한다.
bool ReserveRitualRegular() {
    if (pendingRitualRegular) throw std::logic_error("의식 Regular 중첩 예약");
    try { pendingRitualRegular=std::make_unique<RegularProcess>(0U,0.0F);return true; }catch (const std::bad_alloc&) { return false; }
}
// 확보한 객체를 실제 event/payload로 생성하고 같은 Host에 부착한다. type은46 또는61이다.
void AddRitualRegular(Sid sid,std::uint32_t event,float payload,std::uint32_t type) {
    if (!pendingRitualRegular) throw std::logic_error("의식 Regular 실제 예약 누락");
    auto* reserved=pendingRitualRegular.get(); // 같은 확보의 주소를 유지하며 두 번째 할당을 하지 않는다.
    std::destroy_at(reserved);std::construct_at(reserved,event,payload);
    auto* attached=static_cast<RegularProcess*>(core.processes.Attach(std::move(pendingRitualRegular),type,sid,0x50)); // 서버/로컬61·46 Form 정책을 그대로 사용한다.
    if (attached) attached->SetNextTime(core.processState.Now());
}
// 실제452fe0의 세 인자를 단일 클라이언트 입력 큐로 보낸다. 기존 stopVoice hook명은 API 호환을 위해 유지한다.
void RitualCommand(std::uint32_t command,std::uint32_t first,std::uint32_t second) {
    if (!services.assets.queueCommand) throw std::logic_error("다이스 실제 입력 명령 큐 미연결");services.assets.queueCommand(command,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second));
}
// 실제164/165 최초Pop·제단20~24·내용물 부모+24·파생 삭제를 같은 월드에 연결한다.
void ConnectDaisLifecycle() {
    ritualAdvance=std::make_unique<RawFrameAdvance>(core.pool,core.frames,core.frame);
    AltarSacrificeHooks altar; // 실제 같은 난수/프레임/경로/폭발과 명령 큐다.
    altar.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
    altar.reserveShared=[this] { return ReserveRitualRegular(); };
    altar.playAt=[this](float x,float y,std::string_view name,std::uint32_t flags) { if (!services.sound.playOnceAt) throw std::logic_error("제단 실제 위치 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup(name),flags,0); };
    altar.playGlobal=[this](std::string_view name,std::uint32_t loop,std::uint32_t volume,std::uint32_t pan,std::uint32_t priority,std::uint32_t limit) { if (!services.sound.play) throw std::logic_error("제단 실제 전역 음향 미연결");services.sound.play(soundNames.Lookup(name),loop,volume,pan,priority,limit); };
    altar.removeAnimation=ritualExplosion; // 원본4605d0은 일반 폭발 효과이며 피해 전처리와 같은 몸체를 쓴다.
    altar.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };altar.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
    altar.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); }; // 448500의 가상+74(0)는 사제의 원래 소유자 WORD를 해석해야 한다.
    altar.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
    altar.escapeEffect=[this](Sid sid,float distance,std::uint32_t event) { if (!carrierEscape) throw std::logic_error("제단 해방 실제 회피 설치 전 호출");static_cast<void>(carrierEscape->Run(sid,distance,event)); };
    altar.refreshOwner=[this](std::uint32_t owner) { priestParticipation->Refresh(owner); };
    altar=MakeAltarSacrificeHooks(core.pool,core.frame,core.processes,core.factory,assetBindings->Regular(),random,std::move(altar));
    altar.addShared=[this](Sid sid,std::uint32_t event,float payload) { AddRitualRegular(sid,event,payload,61); }; // 앞서 확보한 같은 Regular를 사용한다.
    altarSacrifice=std::make_unique<RawAltarSacrifice>(core.pool,core.hash,core.types,core.frames,core.neighbors,*runePostPop,*templeRitual,priests->Carrier(),*daisRegular,*ritualAdvance,core.processes,*damageablePre,*daisState,altarState,std::move(altar));
    daisLifecycleState=std::make_unique<DaisLifecycleState>(DaisLifecycleState{loginEnd,carrierState});
    DaisLifecycleHooks life; // 같은 현재 Player 기록과 실제 공간·생성·예약의 수명 효과다.
    life.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) { core.frame.Set(sid,frame,flags); };
    life.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };life.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
    life.create=[this](std::uint32_t type,std::uint32_t flags) { return core.factory.Create(type,flags); };life.setOwner=[this](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); };life.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
    life.reserveRegular=[this] { return ReserveRitualRegular(); };life.addRegular=[this](Sid sid,std::uint32_t event,float payload) { AddRitualRegular(sid,event,payload,46); };
    life.surface=[this](float x,float y) { return RawPathSurfaceAt(core.hash,{x,y}); };
    life.hasIsland=[this](Sid sid) { const auto genus=core.types[core.pool.Slot(sid)[10]].flags2;return (genus&2U)!=0 && (genus&0x1000000U)==0; }; // 실제4acdf0은 TYPE genus를 검사한다.
    life.islandNumber=[this](Sid sid) { const auto raw=core.pool.Slot(sid);const auto genus=core.types[raw[10]].flags2;if (!(genus&0x1000006U)) throw std::logic_error("다이스 원본 섬 번호 genus 진단");return (genus&4U) ? 127U : Read(raw,8)&0xffffU; }; // 실제4acda0의 WORD/다리 분기다.
    life.option=[this](std::string_view key) { if (!services.construction.configInt) throw std::logic_error("다이스 현재 설정 조회 미연결");return services.construction.configInt(key,0)!=0; };
    life.chat=[this](std::string_view key) {
        if (!services.priest.shield.tell) throw std::logic_error("홈섬 밖 제단의 실제 Tell 소비자 미연결");services.priest.shield.tell(key);
    }; // 449320의4cf960은 채팅 기록이 아니라 같은 Tell(key,0) 안내 창이다.
    life.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };life.damageablePostDestroy=[this](Sid sid,std::uint32_t flags) { PrepareDamageablePost();damageablePost->PostDestroy(sid,flags); };
    daisLifecycle=std::make_unique<RawDaisLifecycle>(core.pool,core.hash,core.types,core.neighbors,*templeRitual,*altarSacrifice,*carrier,*damageablePre,*daisState,altarState,*daisLifecycleState,std::move(life));core.postPop.SetDaisPostPop(daisLifecycle.get());
    daisNoticeState=std::make_unique<DaisContentNoticeState>(DaisContentNoticeState{priestState.movement.priest,playerState.battleOwner,0x020000d7});
    DaisContentNoticeHooks notice; // plain 제단 프레임/WORD와 원본 입력 명령의 실제 부모 통지다.
    notice.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };notice.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
    notice.stopVoice=[this](std::uint32_t cue,std::uint32_t first,std::uint32_t second) { RitualCommand(cue,first,second); };notice.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
    daisNotice=std::make_unique<RawDaisContentNotice>(core.pool,core.frames,*runePostPop,*daisNoticeState,std::move(notice));
}
// 의식 자격/이동/룬 진행과 로컬 기술 획득을 연결한다. 나머지 보상/UI 미복원은 명시 오류로 남긴다.
void ConnectRitual() {
    ritualLookup=std::make_unique<RawContainedLookup>(core.pool,[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); });
    PriestRitualFrameHooks frame; // 실제 같은 Host/Frame/위치 소리로604를 처리한다.
    frame.setFrame=[this](Sid sid,std::int32_t value,std::uint32_t flags) { core.frame.Set(sid,value,flags); };
    frame.eventExists=[this](Sid sid,std::uint32_t event) { return core.processes.FindEvent(sid,event)!=nullptr; };
    frame.playAt=[this](float x,float y,std::string_view name,std::uint32_t flags) { if (!services.sound.playOnceAt) throw std::logic_error("사제604 위치 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup(name),flags,0); };
    priestRitualFrame=std::make_unique<RawPriestRitualFrame>(core.pool,core.frames,std::move(frame));
    PriestRitualActionHooks action; // 현재 사제 프레임과604 SharedRegular다.
    action.setFrame=[this](Sid sid,std::int32_t value,std::uint32_t flags) { core.frame.Set(sid,value,flags); };
    action.reserveShared=[this] { return ReserveRitualRegular(); };action.addShared=[this](Sid sid,std::uint32_t event,float payload) { AddRitualRegular(sid,event,payload,61); };
    ritualActions=std::make_unique<RawPriestRitualActions>(core.pool,core.frames,std::move(action));
    TempleRitualHooks temple; // 실제 생성/HP/owner/Pop과 위치 음향/307 예약이다.
    temple.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
    temple.playAt=[this](float x,float y,std::string_view name,std::uint32_t flags) { if (!services.sound.playOnceAt) throw std::logic_error("의식 위치 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup(name),flags,0); };
    temple.reserveShared=[this] { return ReserveRitualRegular(); };
    temple.addShared=[this](Sid sid,std::uint32_t event,float payload) { AddRitualRegular(sid,event,payload,61); };
    temple.create=[this](std::uint32_t type,std::uint32_t flags) { return core.factory.Create(type,flags); };
    temple.setHitPoints=[this](Sid sid,std::int32_t value) { SetRawSquidHitPoints(core.pool,sid,value); };
    temple.setOwner=[this](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); };
    templeRitual=std::make_unique<RawTempleRitual>(core.pool,core.hash,core.types,core.frames,core.shapes,core.spots,core.neighbors,hp,*ritualActions,ritualState,std::move(temple));
    RunePostPopHooks rune; // 같은 실제30 건설과 로컬304 예약을 사용한다.
    rune.reserveRegular=[this] { return ReserveRitualRegular(); };rune.addRegular=[this](Sid sid,std::uint32_t event,float payload) { AddRitualRegular(sid,event,payload,46); };
    rune.startConstruction=[this](Sid sid,float duration) { PrepareBuilding(core.processState.Now());building->Start(sid,duration); };
    runePostPop=std::make_unique<RawRunePostPop>(core.pool,core.hash,core.types,runeState,core.postPop,std::move(rune));core.postPop.SetRunePostPop(runePostPop.get());
    daisState=std::make_unique<DaisRegularState>(DaisRegularState{ritualState,runeState,core.processState,rewardState,playerState.battleOwner});
    daisState->BindPlayerRecords(core.pool.Edition(),players);daisState->voiceCue=0x020000df; // PE00542a70의 실제 희생 종료 명령이다.
    DaisRegularHooks dais; // 실제 의식 이동/초기 진행의 필수 효과와 아직 미연결인 후반 경계다.
    dais.setFrame=[this](Sid sid,std::int32_t value,std::uint32_t flags) { core.frame.Set(sid,value,flags); };
    dais.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
    dais.reserveShared=[this] { return ReserveRitualRegular(); };dais.addShared=[this](Sid sid,std::uint32_t event,float payload) { AddRitualRegular(sid,event,payload,61); };
    dais.playAt=[this](float x,float y,std::string_view name,std::uint32_t flags) { if (!services.sound.playOnceAt) throw std::logic_error("다이스 위치 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup(name),flags,0); };
    dais.processType=[this](std::uint32_t type) { return priests->MovementProcessType(type); };
    dais.pathMetadata=[this](Sid sid) { const auto& definition=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition;return RawPathStartType{static_cast<float>(definition.Number("speed").value()),static_cast<float>(definition.Number("turningSpeed").value()),definition.specialFrames.defaultFrame}; };
    dais.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assetBindings->Regular().DamageableHandle(sid,event,count,payload); };
    dais.reserveSound=[this] { if (pendingRitualSound) throw std::logic_error("다이스 지연 소리 중첩 예약");pendingRitualSound.reset(::operator new(sizeof(SoundProcess),std::nothrow));return pendingRitualSound!=nullptr; };
    dais.loadSound=[this](std::string_view name,std::uint32_t alternate,std::uint32_t flags) { if (alternate || flags!=0xc) throw std::logic_error("다이스 실제 지연 소리 인자 오류");return soundNames.Lookup(name); };
    dais.attachSound=[this](Sid sid,std::uint32_t sound,std::uint32_t alternate,std::uint32_t flags)->std::uintptr_t {
        if (!pendingRitualSound) throw std::logic_error("다이스 지연 소리 실제 예약 누락");auto* storage=pendingRitualSound.release();SoundProcess* process{}; // 같은 확보의 소유권을 실제 생성자로 넘긴다.
        try { process=new(storage) SoundProcess(*sounds,sound,alternate,flags); }catch (...) { ::operator delete(storage);throw; }
        auto* attached=core.processes.Attach(std::unique_ptr<SoundProcess>(process),soundState.processType,sid,0x10);return attached ? std::uintptr_t(attached->Form().value) : 0;
    };
    dais.wallSeconds=services.sound.wallSeconds;
    dais.setSoundStart=[this](std::uintptr_t form,double time) { if (form>65535) throw std::out_of_range("다이스 소리 Form 번호 범위");auto* process=dynamic_cast<SoundProcess*>(core.processes.FromForm(Sid{static_cast<std::uint16_t>(form)}));if (!process) throw std::logic_error("다이스 현재 Sound47 누락");process->SetStartTime(time); }; // 같은 절대 시각을 쓰며 벽시계를 다시 읽지 않는다.
    dais.stopVoice=[this](std::uint32_t cue,std::uint32_t first,std::uint32_t second) { RitualCommand(cue,first,second); }; // 기존 hook명과 달리 실제452fe0은 입력 명령 큐다.
    dais.addLocalSp=[this](float delta) { hp.AddSp(playerState.battleOwner,delta); }; // 원본40eec0과 같은 분산 SP/AI지갑/표시 갱신을 사용하며 직접 결과값을 쓰지 않는다.
    dais.refreshOwner=[this](std::uint32_t owner) { priestParticipation->Refresh(owner); }; // 원본495060의 실제 목록/포획 조회와 같은ownerState 갱신이다.
    dais.priestExists=[this](std::uint32_t owner) { if (!outcome) throw std::logic_error("희생 완료의 현재 참여 판정 설치 전 호출");return outcome->PriestDead(owner); }; // 기존hook명과 달리 실제4922e0은 사망 여부다. 원본 호출부의 부정을 유지한다.
    dais.chat=[this](const DaisChat& notice) {
        if (notice.kind!=DaisChat::Kind::Stormpower && notice.kind!=DaisChat::Kind::Upgrade) throw std::logic_error("멀티플레이 희생 사망 안내 미연결");
        if (!services.priest.shield.tell) throw std::logic_error("희생 완료의 실제 Tell 소비자 미연결");services.priest.shield.tell(notice.kind==DaisChat::Kind::Stormpower ? "SacrificeForStormpower" : "AltarUpgrade");
    }; // 원본4cf960은 같은Tell(key,0) wrapper다. 멀티플레이 이름/상태 인자 안내는 별도 경계로 남긴다.
    dais.localType=[this](auto type) { if (!learnNewTechnology) throw std::logic_error("로컬 기술 획득의 현재 Session 진입 미연결");learnNewTechnology(type); }; //Help와 같은 실제492a30을 사용하고 selectedType 쓰기는 원문 상위 부모에 남긴다.
    dais.refreshLocal=[this] { if (!refreshLocalRitual) throw std::logic_error("의식 뒤 현재 Session 갱신 진입 미연결");refreshLocalRitual(); }; //단순 화면 갱신이 아니라 원래 Core/CoreData/전체 snapshot 순서다.
    dais.normalizeForeignSp=[this](std::uint32_t owner) {
        const PlayerSpSetReads reads{[this] { return playerState.playerKnown; },[this] { return playerState.battleOwner; },[this] { return globals.editor; }}; // 각 getter/setter의 실제 조회 시점에 현재 전역을 빌린다.
        GrantForeignRitualSp(owner,[this,&reads](auto player) { return ReadPlayerSpAmount(player,sp,reads); },
            [this,&reads](auto player,auto amount) { SetPlayerSpAmount(player,amount,sp,reads,services.refreshSp); });
    }; // 기존 hook명과 달리 원문은 현재 SP+5000을 절삭한 지정이며 AI 지갑 가산이 아니다.
    dais.grantForeignType=[this](std::uint32_t owner,std::uint32_t type) {
        GrantForeignRitualType({coreDataGlobals->Records(),0},owner,type,[this](auto selected) {
            if (selected>=core.types.size()) throw std::out_of_range("비로컬 의식의 현재 TYPE 물리 범위");return core.types[selected].techBitIndex;
        },SessionMissing<void,std::uint32_t>());
    }; // Player contained 기술 목록이 아니라 실제 Client 장부9×16의 Core에 기록한다.
    FillMissing(dais.localSpecial);FillMissing(dais.multiplayerStatus);
    daisRegular=std::make_unique<RawDaisRegular>(core.pool,core.hash,core.types,core.frames,*templeRitual,*runePostPop,priests->Carrier(),core.processes,*ritualLookup,motion->Paths().Starter(),*daisState,std::move(dais));
    PriestSacrificeRequestHooks request; // 두 wrapper는 실제 같은 Dais/내용물/음악을 호출한다.
    request.allowed=[this](Sid dais,Sid priest) { PrepareRitual();return daisRegular->CanBridge(dais,priest); };
    request.remove=[this](Sid parent,std::uint32_t type,std::uint32_t index,std::uint32_t remove) {
        RawContainedFinder finder(core.pool,contentFinder); // 4b26f0은 프로세스 BYTE타입이 아니라 Form6의 WORD18 종류를 비교한다.
        for (auto sid=finder.Begin(parent);sid.value;sid=finder.Next()) { // 다음 링크를 먼저 저장하는 실제 finder를 재사용한다.
            const auto raw=core.pool.Slot(sid);if ((std::uint32_t(raw[18])|(std::uint32_t(raw[19])<<8))!=type) continue;
            if (!index) { if (!remove) return;if (core.pool.IsServer() || (sid.value>4 && sid.value<core.pool.Layout().serverFirst)) world.Destroy(sid,0); }
            --index; // 삭제 뒤 원본 DWORD 순번의0→ffffffff 감김과 나머지 탐색 순서를 유지한다.
        }
    };
    request.command=[this](Sid dais,Sid priest,std::uint32_t current) { PrepareRitual();daisRegular->BridgeCommand(dais,priest,current); };
    request.music=[this](std::string_view name) { if (!services.requestMusic) throw std::logic_error("희생 의식 실제 음악 요청 미연결");services.requestMusic(name); };
    sacrificeRequest=std::make_unique<RawPriestSacrificeRequest>(core.pool,motionState.cleanup.nuggetType,playerState.battleOwner,std::move(request));PrepareRitual();ConnectDaisLifecycle();
}
