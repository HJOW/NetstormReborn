// Impl 안에서 포함하며 단일 세계의 실제 피해·효과·로컬 요청 큐를 빌린다.
void ConnectMeltdown() {
    if (meltdown) throw std::logic_error("Meltdown 중복 설치");
    meltdownState.boss=[this] { return assetState.Boss(); };meltdownState.localOwner=[this] { return playerState.battleOwner; };meltdownState.alliances=alliances;
    MeltdownHooks hooks;hooks.hasHitPoints=[this](Sid sid) { return hp.HasHitPoints(sid); };hooks.assertion=[](auto line) { throw std::logic_error("원본 Meltdown 권한 진단 "+std::to_string(line)); };
    hooks.find=[this](float x,float y,std::int32_t radius,BombTargetList& list) { meltdown->Find(x,y,radius,list); };
    hooks.send=[this](auto target,std::span<const std::uint8_t> packet) { if (!services.carrierRequest) throw std::logic_error("Meltdown 실제 로컬 요청 전송 미설치");services.carrierRequest(target,packet); };
    hooks.broadcast=[this](std::span<std::uint8_t> packet) {
        for (std::uint32_t owner=1;owner<9;++owner) if ((players[owner*180+0x50]&1U) && services.battlePeers[owner] && owner!=playerState.battleOwner) { RawMeltdown::FinalizePacket(packet,true);break; } // 실제 송신 observer가 도달할 때만 선언 길이를 바꾼다.
        const auto length=std::uint32_t(packet[2])|(std::uint32_t(packet[3])<<8);if (length>packet.size()) throw std::out_of_range("Meltdown 실제 배포 범위");broadcast->Send(packet.first(length));
    };
    hooks.receive=[this](auto,auto,std::span<const std::uint8_t> packet) { meltdown->ReceiveEffect(packet); };
    hooks.destroy=[this](Sid sid,auto flags) { static_cast<void>(world.Destroy(sid,flags)); };
    hooks.damage=[this](Sid sid,std::int32_t amount,std::uint32_t owner,Sid parent) { hits->DamageHooks({}).damage(sid,amount,static_cast<std::uint8_t>(owner),parent); };hooks.ground=[this](float x,float y) { hits->Impact().Ground(x,y); };
    hooks.edge=[this](Sid sid,float x,float y) { core.bridgeEvents.ConvertEnd(sid,x,y); };
    hooks.visual=[this](float x,float y,auto variant,float interval) { hits->Impact().Visual(x,y,variant,interval); };
    hooks.sound=[this](std::string_view name) { if (!services.sound.play) throw std::logic_error("Meltdown 실제 전역 음향 미설치");services.sound.play(soundNames.Lookup(name),0,0,0,0,0); };
    meltdown=std::make_unique<RawMeltdown>(core.pool,core.hash,core.types,core.frames,meltdownState,std::move(hooks));
}
