// Impl 안에서 포함하여 원본427070/42aa50/444bc0을 같은 Session에 연결한다.
// 요청과수신직전에현재소유자/권한/진단/지도한계를공급한다.
void PrepareCarrierDrop() {
    carrierDropState.localOwner=playerState.battleOwner;carrierDropState.server=core.processState.Server();carrierDropState.checking=gunState.checking;carrierDropState.numTypes=static_cast<std::uint32_t>(core.types.size());carrierDropState.mapLimit=carrierDropPointState.mapLimit=static_cast<std::int32_t>(cursorManaGlobals.mapLimit);
}
// 게임 계산은기존몸체에연결하고실제팝업/전송/선택은현재소비자만사용한다.
void ConnectCarrierDrop() {
    if (carrierDrop) throw std::logic_error("Cast/Drop Session 중복설치");
    objectCommandState.localOwner=[this] { return playerState.battleOwner; };
    objectCommandState.editor=[this] { return globals.editor!=0; };objectCommandState.mission=[this] { return globals.missionPost!=0; };
    ObjectCommandHooks common; // 같은 가상+74/+98/+10을 기존 월드 몸체에 위임한다.
    common.owner=[this](Sid parent,std::uint32_t owner) { ownership->SetOwner(parent,owner); };
    common.hit=[damage=hits->DamageHooks({}).damage](Sid parent,std::int32_t amount,std::uint32_t owner,Sid attacker) { if (!damage) throw std::logic_error("공통 객체 명령의 실제 피해 미연결");damage(parent,amount,static_cast<std::uint8_t>(owner),attacker); };
    common.destroy=[this](Sid parent,std::uint32_t flags) { static_cast<void>(world.Destroy(parent,flags)); };
    common.describe=SessionMissing<void,Sid>(); // 개발용4b0fb0 진단 출력은 별도 복원 대상이다.
    objectCommand=std::make_unique<RawObjectCommand>(core.pool,objectCommandState,std::move(common));
    CarrierDropPointHooks point;point.mayPlace=[this](const CanonPlacementQuery& query) { PrepareConstruction();return MayPlaceGameplay(query); };point.center=[this](Sid sid) { return gunTarget->Center(sid); };
    carrierDropPoint=std::make_unique<RawCarrierDropPoint>(core.pool,core.types,carrierDropPointState,std::move(point));
    CarrierDropHooks hooks;
    hooks.immobile=[this](Sid sid) { return priests->PriestState().GroundImmobile(sid); };hooks.cost=[this](std::uint32_t type) { return hp.TypeCostUnits(type); };
    hooks.afford=[this](float cost) { const auto money=std::bit_cast<float>(sp.Get(playerState.battleOwner));return globals.editor || cost<=money; }; // 원본425e90은 편집모드가아니면현재잔액과비교한다.
    hooks.subtract=[this](std::uint32_t owner,float amount) { hp.AddSp(owner,amount); };
    hooks.project=[this](std::array<float,2> point) { return ProjectCameraView(gameplayGlobals->Camera().Snapshot(),point[0],point[1]); }; // 실제497220은 같은 현재 snapshot 픽셀 원점을 호출 때 읽는다.
    hooks.popup=[this](std::int32_t cost,std::array<std::int32_t,2> point,std::uint32_t reference) { if (!services.carrierDropPopup) throw std::logic_error("Cast/Drop 실제비용Gump미연결");services.carrierDropPopup(cost,point[0],point[1],reference); };
    hooks.regular=[this](Sid parent,float delay) { static_cast<void>(core.processes.AddSharedRegular(parent,0x493,delay)); };
    // ProcessFinder는반환전에next를보존하고현재type51만요청쪽에서통지한다.
    hooks.attached=[this](Sid parent,const std::function<void(Sid)>& visit) {
        Sid current{static_cast<std::uint16_t>(Read(core.pool.Slot(parent),6))};std::vector<bool> visited(core.pool.Capacity());
        while (current.value) {
            if (current.value>=visited.size() || visited[current.value]) throw std::logic_error("Cast/Drop 부착체인손상");visited[current.value]=true;const auto raw=core.pool.Slot(current);const Sid next{static_cast<std::uint16_t>(raw[4]|(std::uint16_t(raw[5])<<8))};if (raw[10]<70) visit(current);current=next;
        }
    };
    hooks.notify=[this](Sid form,std::uint32_t event,std::uint32_t info,std::uint32_t value) { auto* process=core.processes.FromForm(form);if (!process) throw std::logic_error("Cast/Drop 현재프로세스누락");process->Notify(event,info,value); };
    hooks.select=[this](std::uint32_t sid,std::uint32_t force,std::uint32_t activate) { auto current=activeSelection.lock();if (!current || !current->selection) throw std::logic_error("Cast/Drop 실제선택수명미연결");static_cast<void>(current->selection->Select(sid,force,activate)); };
    hooks.send=[this](std::int32_t target,std::span<const std::uint8_t> packet) { if (!services.carrierRequest) throw std::logic_error("Cast/Drop 실제전송미설치");services.carrierRequest(target,packet); };
    hooks.pickup=[this](Sid parent) { carrierPickup->Request(parent); };
    hooks.baseMenu=[this](Sid parent,std::uint32_t command,Sid) { return objectCommand->Command(parent,command); }; // 원본4ae010은 이벤트의명령DWORD만소비한다.
    hooks.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Cast/Drop 진단 "+std::to_string(line)); };hooks.dropPoint=[this](Sid parent,std::uint32_t type) { return carrierDropPoint->Find(parent,type); };
    hooks.queue=[this](std::uint32_t command,std::uint32_t type,std::uint32_t flags) { if (!services.assets.queueCommand) throw std::logic_error("Cast/Drop 실제입력큐미연결");services.assets.queueCommand(command,std::bit_cast<std::int32_t>(type),std::bit_cast<std::int32_t>(flags)); };
    hooks.badOwner=[this](std::uint32_t type,std::uint32_t owner) { PrepareConstruction();construction->Refund(type,owner); };hooks.money=[this](std::uint32_t owner) { return std::bit_cast<float>(sp.Get(owner)); };
    hooks.refund=[this](std::uint32_t type,std::uint32_t owner,std::array<float,2> point,std::uint32_t argument,std::uint32_t direction) { PrepareConstruction();construction->Reject(type,owner,point[0],point[1],argument,direction); };
    hooks.movement=[this](std::uint32_t type) { return priests->MovementProcessType(type); };
    hooks.removeMovement=[this](Sid parent,std::uint32_t type,std::uint32_t ordinal,std::uint32_t remove) { RawContainedLookup lookup(core.pool,[this](Sid form,std::uint32_t flags) { static_cast<void>(world.Destroy(form,flags)); });static_cast<void>(lookup.Find(parent,type,ordinal,remove!=0)); };
    hooks.detach=[this](Sid parent,std::uint32_t flags) { world.Unpop(parent,flags); };hooks.unpop=[this](Sid parent,std::uint32_t flags) { world.Repop(parent,flags); };hooks.destroy=[this](Sid content,std::uint32_t flags) { static_cast<void>(world.Destroy(content,flags)); };
    hooks.create=[this](std::uint32_t type,std::uint32_t flags) { if (type>=core.types.size()) throw std::out_of_range("Cast/Drop 생성TYPE범위");if ((core.types[type].flags2&0x100U) && type!=168 && type!=117 && type!=118 && type!=171 && type!=172 && type!=173) throw std::logic_error("다른주문연결은멀티플레이단계로이월");return world.Create(type,flags); };
    hooks.owner=[this](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); };
    hooks.argument=[this](Sid sid,std::uint32_t value) { auto raw=core.pool.AllocatedBytes(sid);raw[12]=static_cast<std::uint8_t>(value);raw[13]=static_cast<std::uint8_t>(value>>8); };
    hooks.pop=[this](Sid sid,std::array<float,2> point,std::uint32_t flags) { world.Pop(sid,point[0],point[1],flags); };hooks.activate=[this](std::uint32_t,std::uint32_t,std::span<const std::uint8_t> packet) { ReceiveDevastationActivation(packet); };hooks.broadcast=[this](std::span<const std::uint8_t> packet) { broadcast->Send(packet); };
    hooks.place=[this](std::uint32_t type,std::array<float,2> point,std::uint32_t owner,std::uint32_t argument,std::uint32_t direction,std::uint32_t flags,double time,std::uint32_t quality) { PrepareConstruction();const auto bits=std::bit_cast<std::uint64_t>(time);return construction->Request({type,point[0],point[1],owner,argument,direction,flags,static_cast<std::uint32_t>(bits),static_cast<std::uint32_t>(bits>>32),quality}); };
    hooks.standing=[this](Sid parent) { const auto owner=core.pool.Slot(parent)[34];if (owner>=9) throw std::out_of_range("Cast/Drop 현재Player범위");return owner && Read(players,owner*180+0xb0)!=0; }; // 원본40f100은 이동유무가아니라현재Player끝DWORD를읽는다.
    hooks.escape=[this](Sid parent,float delay,std::int32_t event) { if (!services.motion.carrier.escape) throw std::logic_error("Cast/Drop 실제회피미연결");static_cast<void>(services.motion.carrier.escape(parent,delay,std::bit_cast<std::uint32_t>(event))); };
    carrierDrop=std::make_unique<RawCarrierDrop>(core.pool,core.types,contentFinder,carrierDropState,std::move(hooks));PrepareCarrierDrop();
}
