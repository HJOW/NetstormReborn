// Impl 안에서 포함하여 폭발/은신/마비 주문의 실제 월드·Host·경제·음향을 연결한다.
// 세 Wind 소환 주문은 bc의 소환 부수 효과와 별도 c0를 사용한다.
bool WindSpellTable(std::uint32_t table) const { return table==0x5025d8U || table==0x502e68U || table==0x502f30U; }
//자격 안에서 직접 생성하고 실제 대상 효과는 빈ret4인 세 맨 오 워 주문이다.
bool ManoSpellTable(std::uint32_t table) const { return table==0x5026a0U || table==0x502778U || table==0x502840U; }
//실제 피해/입자/음향·필수 효과 정기까지 연결한 네 직접 피해 주문만 선택한다.
bool DirectStrikeTable(std::uint32_t table) const { return table==0x502908U || table==0x502b70U || table==0x5029d0U || table==0x502aa8U; }
//같은 공용 시전/수신을 사용하는 실제 열다섯 가상 표만 허용한다.
bool ConnectedBombTable(std::uint32_t table) const { return ManoSpellTable(table) || DirectStrikeTable(table) || table==0x5030c0U || table==0x502448U || table==0x502370U || table==0x502510U || table==0x502ff8U || WindSpellTable(table); }
// 상태 주문/만료의 현재 타입 진단 부모를 거쳐 같은 기존 펜스 몸체를 실행한다.
void DispatchFenceType(Sid sid,FenceTypeOperation operation) {
    FenceTypeParentHooks hooks; // 같은 Session의 현재 진단 서비스와 전체 자식이다.
    hooks.assertion=[this](std::string_view expression,std::string_view,std::uint32_t line) { if (!services.ai.assertion) throw std::logic_error("펜스 타입 원문 진단 장치 미연결 "+std::to_string(line));services.ai.assertion(expression); };
    hooks.body=[this,operation](Sid target) { if (operation==FenceTypeOperation::Rebuild) fenceRebuild->Rebuild(target);else fenceLifetime->Refresh(target); };
    RunFenceTypeParent(core.pool.Slot(sid),sid,gunTargetState.sunFenceType,gunState.checking,operation,hooks);
}
// 상태 효과의 적용/만료는 동일한 실제 가상 배치·표시·펜스 소비자를 사용한다.
BombEffectTimerHooks MakeBombTimerHooks() {
    BombEffectTimerHooks hooks;hooks.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };hooks.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
    hooks.refresh=[this](Sid sid) { core.display.Update(core.pool.Slot(sid),0); };hooks.fence=[this](Sid sid) { DispatchFenceType(sid,FenceTypeOperation::Rebuild); };return hooks;
}
// 현재 장치만 연결한 cdecl 기본 생성이다. 수신/시전의 별도 Init 경로와 구분한다.
std::unique_ptr<BombEffectTimerProcess> MakeEmptyBombTimer() { return std::make_unique<BombEffectTimerProcess>(core.pool,core.types,core.processState.Now(),MakeBombTimerHooks()); }
// 7d는 현재 free Form에 새 타이머를 읽고 같은 Host의 지정 SID 부착으로 전달한다.
void ReceiveBombTimer(std::span<const std::uint8_t> packet) {
    const std::uint32_t boss=assetState.Boss();BombTimerReceiveHooks hooks;
    hooks.assertion=[](std::uint32_t line) { throw std::logic_error("원본 상태 타이머 수신 진단 "+std::to_string(line)); };
    hooks.create=[this](std::uint32_t type,std::uint32_t x,std::uint32_t y) { if (type!=57 || x || y || pendingBombTimer) throw std::logic_error("상태 타이머 수신 factory 계약 오류");pendingBombTimer=std::make_unique<BombEffectTimerProcess>(core.pool,core.types,core.processState.Now(),0,std::span<const std::uint32_t>{},0,MakeBombTimerHooks());return 1U; };
    hooks.read=[this](std::uint32_t token,std::span<const std::uint8_t> bytes) { if (token!=1 || !pendingBombTimer) throw std::logic_error("상태 타이머 수신 객체 누락");pendingBombTimer->ReadPacket(bytes); };
    hooks.attach=[this](std::uint32_t token,std::uint32_t type,Sid parent,Sid form,std::uint32_t flags) { if (token!=1 || !pendingBombTimer) throw std::logic_error("상태 타이머 수신 부착 객체 누락");static_cast<void>(core.processes.Attach(std::move(pendingBombTimer),type,parent,flags,form)); };
    ReceiveBombTimerPacket(core.pool,boss,bombTimerType,packet,hooks);
}
// 외부 효과 뒤 다시 들어오는 실제 몸체마다 현재 권한/로컬 owner/동맹을 공급한다.
void PrepareDevastation() {
    devastationTargetState.boss=devastationBlastState.boss=devastationEffectState.boss=assetState.Boss();
    if (!world.Storage()->UsesNativeTransmissionDevices()) devastationRegularState.boss=assetState.Boss(); // Native 정기는 같은 현재 권한 주소를 직접 읽는다.
    if (!world.Storage()->UsesNativeTransmissionDevices()) devastationRegularState.localOwner=playerState.battleOwner;devastationBlastState.alliances=alliances;
}
// 아직 복원하지 않은 파생을 다른 효과로 실행하지 않으며 free/dead의 원본 조기 반환은 유지한다.
void CheckDevastationPacket(std::span<const std::uint8_t> packet) const {
    if (packet.size()<6) throw std::invalid_argument("Devastation 패킷 부모WORD 부족");
    const Sid parent{static_cast<std::uint16_t>(packet[4]|(std::uint16_t(packet[5])<<8))};const auto raw=core.pool.Slot(parent);
    if (!(raw[11]&3U) && !ConnectedBombTable(Read(raw,0))) throw std::logic_error("다른 주문 Session 연결은 미복원");
}
// 현재 원격 비용/SharedRegular를 실제 활성화 몸체에 맡긴다.
void ReceiveDevastationActivation(std::span<const std::uint8_t> packet) { CheckDevastationPacket(packet);PrepareDevastation();devastationActivation->Receive(packet); }
// 현재 대상수/목록/비용 누적/가상효과/소리/삭제를 실제 수신 몸체에 맡긴다.
void ReceiveDevastationEffect(std::span<const std::uint8_t> packet) { CheckDevastationPacket(packet);PrepareDevastation();devastationEffect->Receive(packet); }
// 최초 게임 프레임 이전에 기존 복원 몸체를 조립한다. 저장 자료나 UI 성공 상태를 주입하지 않는다.
void ConnectDevastation() {
    if (devastationRegular) throw std::logic_error("Devastation Session 중복 설치");
    if (!world.Storage()->UsesNativeTransmissionDevices()) devastationRegularTypes.resize(core.types.size());devastationEffectTypes.resize(core.types.size());
    // 실제 불변 타입 표의 번호와 같은 슬롯에 원래 속성만 공급한다.
    for (std::size_t type=0;type<core.types.size();++type) {
        if (!world.Storage()->UsesNativeTransmissionDevices()) devastationRegularTypes[type]={core.frames[type].Run('A').first,carrierRegularTypes[type].castTime};devastationEffectTypes[type].cost=core.types[type].cost;
    }
    // 자산 로드 목록은188TYPE 전체와 길이가 다르므로 실제 block 번호로만 음향을 보충한다.
    for (const auto& asset:assets.game.Types()) { const auto name=asset.definition.String("fireSound");auto& type=devastationEffectTypes.at(70+asset.block);type.sound=name && !name->empty() ? soundNames.Lookup(*name) : 0;type.delay=static_cast<std::int32_t>(asset.definition.Number("effecttime").value_or(0)); }
    BombStatusHooks status;status.refresh=[this](Sid sid) { core.display.Update(core.pool.Slot(sid),0); };status.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };status.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
    status.fence=[this](Sid sid) { DispatchFenceType(sid,FenceTypeOperation::Refresh); };status.flare=[this](Sid sid,std::int32_t count,std::uint32_t event,std::uint32_t type,std::uint32_t direction) { flare->SpawnDirection(sid,count,event,type,direction); };bombStatusState.alliances=alliances;bombStatus=std::make_unique<RawBombStatus>(core.pool,core.types,bombStatusState,std::move(status));
    std::span<const std::int32_t> spellCounts; //Native는 최초 자산 로드 뒤 같은 현재188TYPE spawns를 빌린다.
    if (world.Storage()->UsesNativeTransmissionDevices()) spellCounts=world.Storage()->BombSpawns();
    else { //독립 typed 입력만 기존 자산 표를 소유한다.
        windSpellCounts.resize(core.types.size());
        for (const auto& asset:assets.game.Types()) windSpellCounts.at(70+asset.block)=static_cast<std::int32_t>(asset.definition.Number("spawns").value_or(0)); //같은 자산 block 번호의 signed spawns만 보충한다.
        spellCounts=windSpellCounts;
    }
    windSpellState.counts=spellCounts;WindSpellHooks summons;summons.exists=[this](Sid parent,std::uint32_t type) { return FindAttachedProcess(core.pool,parent,type).value!=0; };
    summons.allocate=[]()->std::optional<WindAviaryProcessFields> { auto* storage=std::malloc(80);if (!storage) return std::nullopt;WindAviaryProcessFields fields;std::memcpy(fields.bytes.data(),storage,80);std::free(storage);return fields; };
    summons.attach=[this](WindAviaryProcessFields fields,Sid parent,std::uint32_t type,std::uint32_t flags) { static_cast<void>(core.processes.Attach(std::make_unique<WindAviaryProcess>(*windAviary,std::move(fields)),type,parent,flags)); };
    windSpell=std::make_unique<RawWindSpell>(core.pool,core.processState.Server(),core.processState.Now(),windSpellState,std::move(summons));
    ManoSpellHooks mano;mano.wallSeconds=services.sound.wallSeconds;mano.spellType=[this](std::uint32_t level) { return gameplayGlobals->GameAux()->ManoSpellType(level); };mano.flyerType=[this] { return gameplayGlobals->GameAux()->RainFlyerType(); }; //현재 세 주문 역할/생성 TYPE를 원문 소비 위치에서 읽는다.
    mano.create=[this](std::uint32_t type,std::uint32_t flags) { return core.factory.Create(type,flags); };mano.owner=[this](Sid created,std::uint32_t,std::uint8_t owner) { ownership->SetOwner(created,owner); }; //실제4af530과 현재 가상74를 기존 본문에 맡긴다.
    mano.header=[this](Sid created,std::uint32_t abstract,bool inlined) {
        const auto type=core.pool.Slot(created)[10];if (type>=core.types.size() || type>=core.shapes.size()) throw std::out_of_range("맨 오 워 주문 현재 헤더 TYPE 물리 범위");
        const auto& record=core.types[type];const auto& shape=core.shapes[type];const auto limit=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(shape.frameCount)*((record.flags1&0x440000U) ? 2U:1U)); //원문의 프레임0 범위는 추상 인수가 아니라 같은 TYPE flags에 따른다.
        if (limit<1) world.Storage()->RecordDisplayFrameError(type,0,abstract);
        if (limit<1 || !shape.loaded || record.maxHitPoints==-0x22222223 || record.maxHitPoints==-0x32323233) world.Storage()->AssertDisplay("frameCheck( abstract, frame, false )",inlined ? "c:\\ns\\o\\rifttype.h":"..\\o\\rifttype.h",0xe1);
        const auto& header=core.shapes[type].frames.at(0);return std::array<std::int16_t,2>{header.height,header.hotspotY}; //진단 복귀 뒤 같은 선택 TYPE의 현재 signed SHP 헤더를 읽는다.
    };
    mano.center=[this](Sid carrier) { return devastationTargets->Center(carrier); };mano.pop=[this](Sid created,std::uint32_t,float x,float y,std::uint32_t flags) { world.Pop(created,x,y,flags); }; //헤더 뒤 현재 중심과 새 객체의 실제 가상90을 사용한다.
    manoSpell=std::make_unique<RawManoSpell>(core.pool,core.processState.Server(),spellCounts,std::move(mano)); //같은 실제 풀/서버·현재 spawns와 Runtime 장치를 빌리며 공통 시전/수신을 복제하지 않는다.
    BombHealHooks heal;heal.hasHitPoints=[this](Sid target) { return hp.HasHitPoints(target); };heal.maximumHitPoints=[this](Sid target) { return hp.MaxHitPoints(target); };
    heal.fenceType=[this] { return gunTargetState.sunFenceType; };heal.burstType=[this] { return healBurstType; }; //현재 전역은 효과가 도달한 위치에서 읽는다.
    heal.fence=[this](Sid target) { DispatchFenceType(target,FenceTypeOperation::Rebuild); };
    heal.unpop=[this](Sid target,std::uint32_t,std::uint32_t flags) { world.Unpop(target,flags); };heal.repop=[this](Sid target,std::uint32_t,std::uint32_t flags) { world.Repop(target,flags); };
    heal.refresh=[this](Sid target,std::uint32_t) { core.display.Update(core.pool.Slot(target),0); };
    heal.create=[this](std::uint32_t type,std::uint32_t flags) { return core.factory.Create(type,flags); }; //가상90 이전에 원래4af530만 실행하며 World::Create의 추가changed 접두는 넣지 않는다.
    heal.pop=[this](Sid created,std::uint32_t,float x,float y,std::uint32_t flags) { world.Pop(created,x,y,flags); };bombHeal=std::make_unique<RawBombHeal>(core.pool,core.types,std::move(heal));
    bombHardener=std::make_unique<RawBombHardener>(core.pool,core.types,core.frames,random,core.frame,[this](Sid target,std::int32_t count,std::uint32_t event) { flare->Spawn(target,count,event); }); //별도 강화 프레임/난수 사본 없이 같은 실제 현재 장치에 연결한다.
    BombStrikeHooks strike;const auto hit=hits->DamageHooks({}).damage; //기존 가상98의 현재 피해 분배를 복제 없이 빌린다.
    bombStrikeState.currentBoss=[this] { return assetState.Boss(); };bombStrikeState.currentSpecialType=[this] { return gameplayGlobals->GameAux()->WindFlyerType(); }; //효과 뒤 같은 권한/특별 대상 전역을 다시 읽는다.
    bombStrikeState.currentFirstEffectType=[this] { return gameplayGlobals->GameAux()->GravityEffectType(); };bombStrikeState.currentHailEffectType=[this] { return gameplayGlobals->GameAux()->MeteorEffectType(); }; //각 생성 직전에 같은 장기 효과TYPE를 조회한다.
    strike.create=[this](std::uint32_t type,std::uint32_t flags) { return core.factory.Create(type,flags); }; //4af530만 실행하고 World::Create의 추가 접두를 넣지 않는다.
    strike.pop=[this](Sid created,std::uint32_t,float x,float y,std::uint32_t flags) { world.Pop(created,x,y,flags); }; //현재 새 객체의 실제 공간/표시·예약 Pop이다.
    strike.destroy=[this](Sid target,std::uint32_t,std::uint32_t flags) { static_cast<void>(world.Destroy(target,flags)); }; //특별 대상은 현재 가상 삭제와 정확한100008h를 사용한다.
    strike.damage=[hit](Sid target,std::uint32_t,std::int32_t amount,std::uint8_t owner,Sid parent) { if (!hit) throw std::logic_error("라이트닝 주문 실제 가상 피해 미연결");hit(target,amount,owner,parent); };
    strike.sound=[this](float x,float y) { const auto call=services.sound.playOnceAt;if (!call) throw std::logic_error("라이트닝 주문 실제 위치 음향 미연결");static_cast<void>(call(x,y,soundNames.Lookup("thunderCrack.wav"),0,0)); };
    strike.beam=[this](Sid source,Sid target) { static_cast<void>(particles->AddBeam(source,target,std::string{"shortZap.wav"})); }; //472700 생성자는 실제 shortZap.wav를 별도로 재생한다.
    strike.project=[this](float x,float y) { return ProjectCameraView(gameplayGlobals->Camera().Snapshot(),x,y); }; //각497220은 같은 현재 카메라 snapshot을 다시 읽는다.
    strike.column=[this](Sid target,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float life) { static_cast<void>(particles->AddScreenBolt(target,from,to,life)); }; //472770의 실제 표시/Host 수명을 공유한다.
    bombStrike=std::make_unique<RawBombStrike>(core.pool,core.types,random,bombStrikeState,std::move(strike)); //같은 현재 난수·장기 전역/실제 하위를 네 주문이 함께 빌린다.
    BombTargetHooks targets;
    // 실제502370의b4는41e3c0이며 다른 표의 반경으로 대체하지 않는다.
    targets.radius=[this](Sid parent,std::uint32_t table) { if (!ConnectedBombTable(table)) throw std::logic_error("주문 반경 표 오류");return devastationTargets->DefaultRadius(parent); };
    targets.eligible=[this](Sid parent,std::uint32_t table,Sid target) { if (!ConnectedBombTable(table)) throw std::logic_error("주문 자격 표 오류");PrepareDevastation();if (ManoSpellTable(table)) return manoSpell->MayTarget(parent,target);if (DirectStrikeTable(table)) return bombStrike->MayTarget(parent,target)!=0;if (table==0x5030c0U) return bombHardener->MayTarget(target)!=0;if (table==0x502448U) return bombHeal->MayTarget(target)!=0;return WindSpellTable(table) ? windSpell->MayTarget(parent) : table==0x502370U ? devastationBlast->MayTarget(target) : bombStatus->MayTarget(parent,target); };
    targets.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Devastation 권한 진단 "+std::to_string(line)); };
    devastationTargets=std::make_unique<RawBombTargets>(core.pool,core.hash,core.types,devastationTargetState,std::move(targets)); //같은원문TYPE+44를직접읽어펜스용사거리사본/조립순서에의존하지않는다.
    devastationBlast=std::make_unique<RawBombBlast>(core.pool,core.types,core.frames,*devastationTargets,devastationBlastState,MakeCampaignBombBlastHooks(world,*hits,*devastationTargets));
    BombActivationHooks activation;
    activation.cost=[this](std::uint32_t kind) { return hp.TypeCostUnits(kind); };activation.subtract=[this](std::uint32_t owner,float amount) { hp.AddSp(owner,amount); };
    activation.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Devastation genus 진단 "+std::to_string(line)); };
    // 확보 실패는 원본처럼 예약을 만들지 않으며 성공 메모리를 생성/부착까지 유지한다.
    activation.allocate=[this](std::uint32_t bytes) { if (bytes!=40 || pendingDevastationRegular) throw std::logic_error("Devastation 주기 확보 순서 오류");try { pendingDevastationRegular=std::make_unique<RegularProcess>(0,0.1F);return true; }catch (const std::bad_alloc&) { return false; } };
    activation.construct=[this](std::uint32_t event,Sid parent,float payload) {
        if (!pendingDevastationRegular || event || payload!=0.1F) throw std::logic_error("Devastation SharedRegular 확보 누락");
        auto* process=static_cast<RegularProcess*>(core.processes.Attach(std::move(pendingDevastationRegular),61,parent,0x50));if (process) process->ResetTask(event,core.processState.Now());
    };
    devastationActivation=std::make_unique<RawBombActivation>(core.pool,core.types,playerState.battleOwner,std::move(activation));
    BombRegularHooks regular;
    regular.notice=[this](Sid carrier,std::uint32_t count,std::uint32_t event) { flare->Spawn(carrier,static_cast<std::int32_t>(count),event); };
    regular.frame=[this](Sid sid,std::uint32_t frame,std::uint32_t flags) { core.frame.Set(sid,std::bit_cast<std::int32_t>(frame),flags); };
    regular.animate=[this](Sid sid,std::uint32_t delta,std::uint32_t flags) { RawFrameAdvance(core.pool,core.frames,core.frame).Advance(sid,std::bit_cast<std::int32_t>(delta),flags); };
    regular.allocate=[this](std::uint32_t bytes) { if (bytes!=400 || devastationSearchMemory) throw std::logic_error("Devastation 검색 배열 확보 순서 오류");try { devastationSearchMemory=std::make_unique<std::uint8_t[]>(bytes);return true; }catch (const std::bad_alloc&) { return false; } };
    regular.targets=[this](Sid parent,std::span<std::uint32_t> storage) { BombTargetList result{storage,static_cast<std::int32_t>(storage.size()),0};devastationTargets->Find(parent,result);return static_cast<std::uint32_t>(result.count); };
    // 원본은 실제 연결의 송신 observer에서만 길이를 확정한다. NULL 연결만 있으면 선언 길이는 그대로다.
    regular.broadcast=[this](std::span<std::uint8_t> packet) {
        for (std::uint32_t owner=1;owner<9;++owner) if ((players[owner*180+0x50]&1U) && services.battlePeers[owner] && owner!=playerState.battleOwner) { FinalizeBombEffectPacket(packet,true);break; }
        const auto length=std::uint32_t(packet[2])|(std::uint32_t(packet[3])<<8);if (length>packet.size()) throw std::out_of_range("Devastation 실제 송신 범위");broadcast->Send(packet.first(length));
    };
    regular.receive=[this](std::uint32_t,std::uint32_t,std::span<const std::uint8_t> packet) { ReceiveDevastationEffect(packet); };
    regular.release=[this](std::uint32_t bytes) { if (bytes!=400 || !devastationSearchMemory) throw std::logic_error("Devastation 검색 배열 해제 순서 오류");devastationSearchMemory.reset(); };
    if (world.Storage()->UsesNativeTransmissionDevices()) devastationRegular=&world.Storage()->BombRegular();
    else { localDevastationRegular=std::make_unique<RawBombRegular>(core.pool,devastationRegularTypes,devastationRegularState,std::move(regular));devastationRegular=localDevastationRegular.get(); } // 기존 독립 Session만 지역 정기 본문을 소유한다.
    BombEffectHooks effect;
    // 425f50은 현재 첫493을 매번 다시 찾는다. 다른 사건이나 내용물 Form을 임의 삭제하지 않는다.
    effect.stopCast=[this](Sid carrier) {
        while (auto* cast=core.processes.FindEvent(carrier,0x493)) {
            const auto pid=core.kernel.Find(*cast);core.processes.Kill(*cast,0);if (core.processes.IsRegistered(pid,cast)) throw std::logic_error("Devastation 시전 종료의 실제 권한/삭제 실패");
        }
    };
    effect.allocate=[this](std::uint32_t bytes) { try { if (!devastationEffectMemory) devastationEffectMemory=std::make_unique<std::uint8_t[]>(bytes);else { if (bytes!=40 || pendingBombTimerStorage || pendingBombTimer) throw std::logic_error("상태 타이머 확보 순서 오류");pendingBombTimerStorage.reset(::operator new(sizeof(BombEffectTimerProcess))); }return true; }catch (const std::bad_alloc&) { return false; } };
    effect.apply=[this](Sid parent,std::uint32_t table,Sid target) { if (!ConnectedBombTable(table)) throw std::logic_error("주문 효과 표 오류");PrepareDevastation();if (ManoSpellTable(table)) return;/*0040b380은 원본 자체의 빈ret4다. 자격 소환과 혼용하지 않는다.*/if (DirectStrikeTable(table)) bombStrike->Apply(parent,target);else if (table==0x5030c0U) bombHardener->Apply(parent,target);else if (table==0x502448U) bombHeal->Apply(parent,target);else if (table==0x502370U) devastationBlast->Apply(parent,target);else if (table!=0x5025d8U) bombStatus->Apply(parent,target); };
    effect.construct=[this](std::int32_t delay,std::uint32_t,std::span<const std::uint32_t> members,std::uint32_t flags) { if (!pendingBombTimerStorage || pendingBombTimer) throw std::logic_error("상태 타이머 생성 메모리 누락");auto* process=::new (pendingBombTimerStorage.get()) BombEffectTimerProcess(core.pool,core.types,core.processState.Now(),delay,members,flags,MakeBombTimerHooks());static_cast<void>(pendingBombTimerStorage.release());pendingBombTimer.reset(process);return 1U; };
    effect.argument=[this](std::uint32_t x,std::uint32_t y) { if (x || y) throw std::logic_error("상태 타이머 예약 인자 오류");return std::uint32_t(BombTimerParent(core.pool,core.types,assetState.Residences().Items()).value); };
    effect.attach=[this](std::uint32_t token,std::uint32_t type,std::uint32_t parent,std::uint32_t form,std::uint32_t flags) { if (token!=1 || !pendingBombTimer || parent>65535 || form>65535) throw std::logic_error("상태 타이머 예약 객체/인자 오류");static_cast<void>(core.processes.Attach(std::move(pendingBombTimer),type,Sid{static_cast<std::uint16_t>(parent)},flags,Sid{static_cast<std::uint16_t>(form)})); };
    effect.sound=[this](std::array<std::uint32_t,6> values) { if (!services.sound.play) throw std::logic_error("Devastation 실제 효과음 미연결");services.sound.play(values[0],values[1],values[2],values[3],values[4],values[5]); };
    effect.destroy=[this](Sid sid,std::uint32_t table,std::uint32_t flags) { if (!ConnectedBombTable(table)) throw std::logic_error("주문 삭제 표 오류");static_cast<void>(world.Destroy(sid,flags)); };
    effect.release=[this] { if (!devastationEffectMemory) throw std::logic_error("Devastation 효과 배열 해제 순서 오류");devastationEffectMemory.reset(); };
    devastationEffect=std::make_unique<RawBombEffect>(core.pool,devastationEffectTypes,devastationEffectState,std::move(effect));
    PrepareDevastation();
    ConnectMeltdown(); // 같은 실제 피해/공간/음향의 설치가 끝난 뒤 연결한다.
}
