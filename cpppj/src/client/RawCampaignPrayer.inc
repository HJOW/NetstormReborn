// Impl 안에서 같은 현재 기도/이동/내용물/Kernel을 연결한다.
void ConnectPriestPrayer() {
    if (priestPrayer) throw std::logic_error("사제 기도 Session 중복 설치");priestPrayerTypes.resize(core.types.size());
    for (const auto& asset:assets.game.Types()) priestPrayerTypes.at(70+asset.block)={static_cast<std::int32_t>(asset.definition.Number("praytime").value_or(0)),asset.definition.specialFrames.defaultFrame}; // 실제188TYPE 위치에 원본 속성만 보충한다.
    PriestPrayerHooks hooks;
    hooks.hasEvent=[this](Sid sid,std::uint32_t event) { return core.processes.FindEvent(sid,event)!=nullptr; };
    hooks.direction=[this](Sid sid) { const auto raw=core.pool.Slot(sid);return RawPathAnimation::Direction(core.frames[raw[10]],std::bit_cast<std::int32_t>(Read(raw,36))); };
    hooks.movementType=[this](std::uint32_t type) { return priests->MovementProcessType(type); };
    hooks.movement=[this](Sid sid,std::uint32_t type,std::uint32_t ordinal,bool remove) { RawContainedLookup lookup(core.pool,[this](Sid form,std::uint32_t flags) { static_cast<void>(world.Destroy(form,flags)); });return lookup.Find(sid,type,ordinal,remove); };
    hooks.request=[this](std::int32_t target,std::span<const std::uint8_t> packet) { if (!services.carrierRequest) throw std::logic_error("기도6d 현재 요청 전송 미연결");services.carrierRequest(target,packet); };
    hooks.broadcast=[this](std::span<const std::uint8_t> packet) { broadcast->Send(packet); };
    hooks.reserveShared=[this] { return ReserveRitualRegular(); };hooks.addShared=[this](Sid sid,std::uint32_t event,float payload) { AddRitualRegular(sid,event,payload,61); };
    hooks.ritual=[this](Sid sid,std::uint32_t stage) { if (!ritualActions) throw std::logic_error("기도604 실제 의식 미설치");ritualActions->Continue(sid.value,stage); };
    hooks.immobile=[this](Sid sid) { return priests->PriestState().Immobile(sid); };
    hooks.containedType=[this](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t ordinal) { const auto form=priests->Carrier().FindContained(sid,mask,kind,ordinal);return form.value ? (Read(core.pool.Slot(form),18)&0xffffU) : 0U; };
    hooks.discard=[this](Sid sid) {
        const auto form=priests->Carrier().FindContained(sid,1,0,0);if (!form.value) return;const auto kind=(Read(core.pool.Slot(form),18)&0xffffU);static_cast<void>(world.Destroy(form,0));if (kind!=168) ReplenishNuggets(); // 원본4264d0은첫내용물만지우며168이면전역보충을생략한다.
    };
    hooks.create=[this](Sid sid,std::uint32_t kind,std::uint32_t argument,std::uint32_t flags,std::uint32_t attachFlags,std::uint32_t quality) { static_cast<void>(contained->Create(sid,kind,argument,flags,attachFlags,quality)); };
    hooks.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) { core.frame.Set(sid,frame,flags); };
    hooks.panic=[this](Sid sid) { RunPriestPanic(sid); }; // 같은 명령/중립 회복·도착의 실제491db0을 사용한다.
    hooks.parent=[this](Sid sid,std::uint32_t code,std::uint32_t parameter) { if (!carrierDrop) throw std::logic_error("사제 입력의 현재 Carrier 부모 미설치");return carrierDrop->Command(sid,code,Sid{static_cast<std::uint16_t>(parameter)}); };
    priestPrayer=std::make_unique<RawPriestPrayer>(core.pool,priestPrayerTypes,assetState.boss,std::move(hooks));
}
