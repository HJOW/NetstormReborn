// Impl 안에 포함하며 기존 의식/내용물/Path/Kernel을 단일 현재 월드에 연결한다.
void PrepareRitual() {
    for (std::uint32_t owner=0;owner<=kPlayerCount;++owner) runeState.playerFlags[owner]=players[owner*0xb4+0x50]; // 룬 건설시간도 같은 현재 Player flag를 읽는다.
    ritualState.boss=assetState.boss;ritualState.priestType=priestState.movement.priest;ritualState.runeType=runeState.runeType;
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
    if (attached) attached->SetNextTime(core.processState.now);
}
// 의식 자격/이동/초기 Rune 진행을 연결한다. 희생 완료의 장치/UI/파생 수명 미복원은 명시 오류로 남긴다.
void ConnectRitual() {
    ritualLookup=std::make_unique<RawContainedLookup>(core.pool,[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); });
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
    rune.startConstruction=[this](Sid sid,float duration) { PrepareBuilding(core.processState.now);building->Start(sid,duration); };
    runePostPop=std::make_unique<RawRunePostPop>(core.pool,core.hash,core.types,runeState,core.postPop,std::move(rune));core.postPop.SetRunePostPop(runePostPop.get());
    daisState=std::make_unique<DaisRegularState>(DaisRegularState{ritualState,runeState,core.processState,rewardState,playerState.battleOwner});
    DaisRegularHooks dais; // 실제 의식 이동/초기 진행의 필수 효과와 아직 미연결인 후반 경계다.
    dais.setFrame=[this](Sid sid,std::int32_t value,std::uint32_t flags) { core.frame.Set(sid,value,flags); };
    dais.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
    dais.reserveShared=[this] { return ReserveRitualRegular(); };dais.addShared=[this](Sid sid,std::uint32_t event,float payload) { AddRitualRegular(sid,event,payload,61); };
    dais.playAt=[this](float x,float y,std::string_view name,std::uint32_t flags) { if (!services.sound.playOnceAt) throw std::logic_error("다이스 위치 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup(name),flags,0); };
    dais.processType=[this](std::uint32_t type) { return priests->MovementProcessType(type); };
    dais.pathMetadata=[this](Sid sid) { const auto& definition=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition;return RawPathStartType{static_cast<float>(definition.Number("speed").value()),static_cast<float>(definition.Number("turningSpeed").value()),definition.specialFrames.defaultFrame}; };
    dais.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assetBindings->Regular().DamageableHandle(sid,event,count,payload); };
    FillMissing(dais.reserveSound);FillMissing(dais.loadSound);FillMissing(dais.attachSound);FillMissing(dais.wallSeconds);FillMissing(dais.setSoundStart);FillMissing(dais.stopVoice);
    FillMissing(dais.addLocalSp);FillMissing(dais.normalizeForeignSp);FillMissing(dais.grantForeignType);FillMissing(dais.localSpecial);FillMissing(dais.localType);FillMissing(dais.refreshOwner);FillMissing(dais.refreshLocal);FillMissing(dais.priestExists);FillMissing(dais.multiplayerStatus);FillMissing(dais.chat);
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
    sacrificeRequest=std::make_unique<RawPriestSacrificeRequest>(core.pool,motionState.cleanup.nuggetType,playerState.battleOwner,std::move(request));PrepareRitual();
}
