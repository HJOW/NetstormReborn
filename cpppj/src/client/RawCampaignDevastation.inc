// Impl 안에서 포함하여 Devastation의 실제 월드/Host/경제/음향을 같은 Session에 연결한다.
// 외부 효과 뒤 다시 들어오는 실제 몸체마다 현재 권한/로컬 owner/동맹을 공급한다.
void PrepareDevastation() {
    devastationTargetState.boss=devastationBlastState.boss=devastationRegularState.boss=devastationEffectState.boss=assetState.boss;
    devastationRegularState.localOwner=playerState.battleOwner;devastationBlastState.alliances=alliances;
}
// 이월한 다른 주문을 Devastation 효과로 실행하지 않는다. 이미free/dead인 패킷은 원본 조기 반환을 유지한다.
void CheckDevastationPacket(std::span<const std::uint8_t> packet) const {
    if (packet.size()<6) throw std::invalid_argument("Devastation 패킷 부모WORD 부족");
    const Sid parent{static_cast<std::uint16_t>(packet[4]|(std::uint16_t(packet[5])<<8))};const auto raw=core.pool.Slot(parent);
    if (!(raw[11]&3U) && Read(raw,0)!=0x502370U) throw std::logic_error("다른 주문 Session 연결은 멀티플레이 단계로 이월");
}
// 현재 원격 비용/SharedRegular를 실제 활성화 몸체에 맡긴다.
void ReceiveDevastationActivation(std::span<const std::uint8_t> packet) { CheckDevastationPacket(packet);PrepareDevastation();devastationActivation->Receive(packet); }
// 현재 대상수/목록/비용 누적/가상효과/소리/삭제를 실제 수신 몸체에 맡긴다.
void ReceiveDevastationEffect(std::span<const std::uint8_t> packet) { CheckDevastationPacket(packet);PrepareDevastation();devastationEffect->Receive(packet); }
// 최초 게임 프레임 이전에 기존 복원 몸체를 조립한다. 저장 자료나 UI 성공 상태를 주입하지 않는다.
void ConnectDevastation() {
    if (devastationRegular) throw std::logic_error("Devastation Session 중복 설치");
    devastationRegularTypes.resize(core.types.size());devastationEffectTypes.resize(core.types.size());
    // 실제 불변 타입 표의 번호와 같은 슬롯에 원래 속성만 공급한다.
    for (std::size_t type=0;type<core.types.size();++type) {
        devastationRegularTypes[type]={core.frames[type].Run('A').first,carrierRegularTypes[type].castTime};devastationEffectTypes[type].cost=core.types[type].cost;
    }
    // 자산 로드 목록은188TYPE 전체와 길이가 다르므로 실제 block 번호로만 음향을 보충한다.
    for (const auto& asset:assets.game.Types()) { const auto name=asset.definition.String("fireSound");devastationEffectTypes.at(70+asset.block).sound=name && !name->empty() ? soundNames.Lookup(*name) : 0; }
    BombTargetHooks targets;
    // 실제502370의b4는41e3c0이며 다른 표의 반경으로 대체하지 않는다.
    targets.radius=[this](Sid parent,std::uint32_t table) { if (table!=0x502370U) throw std::logic_error("Devastation 반경 표 오류");return devastationTargets->DefaultRadius(parent); };
    targets.eligible=[this](Sid,std::uint32_t table,Sid target) { if (table!=0x502370U) throw std::logic_error("Devastation 자격 표 오류");PrepareDevastation();return devastationBlast->MayTarget(target); };
    targets.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Devastation 권한 진단 "+std::to_string(line)); };
    devastationTargets=std::make_unique<RawBombTargets>(core.pool,core.hash,core.types,fenceRanges,devastationTargetState,std::move(targets));
    devastationBlast=std::make_unique<RawBombBlast>(core.pool,core.types,core.frames,*devastationTargets,devastationBlastState,MakeCampaignBombBlastHooks(world,*hits,*devastationTargets));
    BombActivationHooks activation;
    activation.cost=[this](std::uint32_t kind) { return hp.TypeCostUnits(kind); };activation.subtract=[this](std::uint32_t owner,float amount) { hp.AddSp(owner,amount); };
    activation.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Devastation genus 진단 "+std::to_string(line)); };
    // 확보 실패는 원본처럼 예약을 만들지 않으며 성공 메모리를 생성/부착까지 유지한다.
    activation.allocate=[this](std::uint32_t bytes) { if (bytes!=40 || pendingDevastationRegular) throw std::logic_error("Devastation 주기 확보 순서 오류");try { pendingDevastationRegular=std::make_unique<RegularProcess>(0,0.1F);return true; }catch (const std::bad_alloc&) { return false; } };
    activation.construct=[this](std::uint32_t event,Sid parent,float payload) {
        if (!pendingDevastationRegular || event || payload!=0.1F) throw std::logic_error("Devastation SharedRegular 확보 누락");
        auto* process=static_cast<RegularProcess*>(core.processes.Attach(std::move(pendingDevastationRegular),61,parent,0x50));if (process) process->ResetTask(event,core.processState.now);
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
    devastationRegular=std::make_unique<RawBombRegular>(core.pool,devastationRegularTypes,devastationRegularState,std::move(regular));
    BombEffectHooks effect;
    // 425f50은 현재 첫493을 매번 다시 찾는다. 다른 사건이나 내용물 Form을 임의 삭제하지 않는다.
    effect.stopCast=[this](Sid carrier) {
        while (auto* cast=core.processes.FindEvent(carrier,0x493)) {
            const auto pid=core.kernel.Find(*cast);core.processes.Kill(*cast,0);if (core.processes.IsRegistered(pid,cast)) throw std::logic_error("Devastation 시전 종료의 실제 권한/삭제 실패");
        }
    };
    effect.allocate=[this](std::uint32_t bytes) { if (devastationEffectMemory) throw std::logic_error("Devastation 효과 배열 중첩 확보");try { devastationEffectMemory=std::make_unique<std::uint8_t[]>(bytes);return true; }catch (const std::bad_alloc&) { return false; } };
    effect.apply=[this](Sid parent,std::uint32_t table,Sid target) { if (table!=0x502370U) throw std::logic_error("Devastation 효과 표 오류");PrepareDevastation();devastationBlast->Apply(parent,target); };
    effect.sound=[this](std::array<std::uint32_t,6> values) { if (!services.sound.play) throw std::logic_error("Devastation 실제 효과음 미연결");services.sound.play(values[0],values[1],values[2],values[3],values[4],values[5]); };
    effect.destroy=[this](Sid sid,std::uint32_t table,std::uint32_t flags) { if (table!=0x502370U) throw std::logic_error("Devastation 삭제 표 오류");static_cast<void>(world.Destroy(sid,flags)); };
    effect.release=[this] { if (!devastationEffectMemory) throw std::logic_error("Devastation 효과 배열 해제 순서 오류");devastationEffectMemory.reset(); };
    devastationEffect=std::make_unique<RawBombEffect>(core.pool,devastationEffectTypes,devastationEffectState,std::move(effect));
    PrepareDevastation();
}
